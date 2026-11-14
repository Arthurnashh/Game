// Implementation of mission editor
#include "../wpch.hpp"
#include "uiMap.hpp"
#include <Es/Common/win.h>
#include "../dikCodes.h"

#include "resincl.hpp"
#include "../keyInput.hpp"
#include <El/QStream/qbStream.hpp>
#include <El/ParamArchive/paramArchiveDb.hpp>

#include "../landscape.hpp"
#include "../txtPreload.hpp"
#include "../fileLocator.hpp"

#include <El/QStream/packFiles.hpp>
#include "../progress.hpp"

#if defined _WIN32 && !defined _XBOX
#include <mapi.h>
#endif

#include <El/Debugging/debugTrap.hpp>
#include "../stringtableExt.hpp"

#include <Es/Strings/bString.hpp>

#include <El/Clipboard/clipboard.hpp>

#include <El/PreprocC/preprocC.hpp>

#include "missionDirs.hpp"
#include "../saveVersion.hpp"
#include "../gameDirs.hpp"

/*!
\file
Implementation file for particular displays (maps, briefing, mission editor).
*/

void CStaticMap::DrawEllipse(
  Vector3Val position, float a, float b, float angle, PackedColor color,
  float dashSolid, float dashGap
)
{
  DrawCoord posMap = WorldToScreen(position);
  float cx = posMap.x * _wScreen;
  float cy = posMap.y * _hScreen;

  const float invSizeLand = InvLandSize;
  float aMap = a * invSizeLand * _invScaleX * _wScreen;
  float bMap = b * invSizeLand * _invScaleY * _hScreen;
  
  // TODO: improve clipping
  float r = floatMax(aMap, bMap);
  if (cx + r < _clipRect.x) return;
  if (cy + r < _clipRect.y) return;
  if (cx - r > _clipRect.x + _clipRect.w) return;
  if (cy - r > _clipRect.y + _clipRect.h) return;

  float s = sin(angle);
  float c = cos(angle);

  static const float linePts = 5.0f;
  int nSteps = toIntCeil(2 * H_PI * aMap * (1.0f / linePts));
  saturate(nSteps, 6, 720);
  bool dashy = false;
  if (dashGap>0)
  {
    // TODO: more flexible dashing
    nSteps = (nSteps+1)/2*2;
    dashy = true;
  }
  float step = 2 * H_PI * (1.0f / nSteps);
  float lastX=cx, lastY=cy;
  bool lastValid = false;
  float st = 0;
  float ct = 1;
  float sstep = sin(step);
  float cstep = cos(step);
  // alternate solid/gap when dashy, stay true otherwise
  bool dash = true;
  for (float t=0; t<2*H_PI + 0.5 * step; t+=step)
  {
    float sta = st * aMap;
    float ctb = ct * bMap;
    float x = cx + c * sta + s * ctb;
    float y = cy + s * sta - c * ctb;
    if (lastValid)
    {
      if (dash) GEngine->DrawLine(
        Line2DPixel(lastX, lastY, x, y),
        color, color, _clipRect
      );
      if (dashy) dash = !dash;
    }
    lastX = x;
    lastY = y;
    lastValid = true;
    float stnew = st * cstep + ct * sstep;
    float ctnew = ct * cstep - st * sstep;
    st = stnew; ct = ctnew;
  }
}

void CStaticMap::DrawRectangle(Vector3Val position, float a, float b, float angle, PackedColor color)
{
  DrawCoord posMap = WorldToScreen(position);
  float cx = posMap.x * _wScreen;
  float cy = posMap.y * _hScreen;

  const float invSizeLand = InvLandSize;
  float aMap = a * invSizeLand * _invScaleX * _wScreen;
  float bMap = b * invSizeLand * _invScaleY * _hScreen;

  // TODO: improve clipping
  float r = sqrt(Square(aMap) + Square(bMap));
  if (cx + r < _clipRect.x) return;
  if (cy + r < _clipRect.y) return;
  if (cx - r > _clipRect.x + _clipRect.w) return;
  if (cy - r > _clipRect.y + _clipRect.h) return;

  float s = sin(angle);
  float c = cos(angle);

  float x1 = cx + c * aMap + s * bMap;
  float y1 = cy + s * aMap - c * bMap;
  float x2 = cx + c * aMap + s * (-bMap);
  float y2 = cy + s * aMap - c * (-bMap);
  float x3 = cx + c * (-aMap) + s * (-bMap);
  float y3 = cy + s * (-aMap) - c * (-bMap);
  float x4 = cx + c * (-aMap) + s * bMap;
  float y4 = cy + s * (-aMap) - c * bMap;

  GLOB_ENGINE->DrawLine(Line2DPixel(x1, y1, x2, y2), color, color, _clipRect);
  GLOB_ENGINE->DrawLine(Line2DPixel(x2, y2, x3, y3), color, color, _clipRect);
  GLOB_ENGINE->DrawLine(Line2DPixel(x3, y3, x4, y4), color, color, _clipRect);
  GLOB_ENGINE->DrawLine(Line2DPixel(x4, y4, x1, y1), color, color, _clipRect);
}

inline PackedColor ModAlpha( PackedColor color, float alpha )
{
  int a=toIntFloor(alpha*color.A8());
  saturate(a,0,255);
  return PackedColorRGB(color,a);
}

const static int NoClamp2D=(DefSpecFlags2D|NoClamp)&~(ClampU|ClampV);

void CStaticMap::FillEllipse(Vector3Val position, float a, float b, float angle, PackedColor color, Texture *texture)
{
  DrawCoord posMap = WorldToScreen(position);
  float cx = posMap.x * _wScreen;
  float cy = posMap.y * _hScreen;

  const float invSizeLand = InvLandSize;
  float aMap = a * invSizeLand * _invScaleX * _wScreen;
  float bMap = b * invSizeLand * _invScaleY * _hScreen;
  
  // TODO: improve clipping
  float r = floatMax(aMap, bMap);
  if (cx + r < _clipRect.x) return;
  if (cy + r < _clipRect.y) return;
  if (cx - r > _clipRect.x + _clipRect.w) return;
  if (cy - r > _clipRect.y + _clipRect.h) return;

  float s = sin(angle);
  float c = cos(angle);

  PackedColor fillColor = color;

  float coefU = 0;
  float coefV = 0;
  float offsetX = (_mapX + _x) * _wScreen;
  float offsetY = (_mapY + _y) * _hScreen;
  if (texture)
  {
    coefU = 1.0 / texture->AWidth();
    coefV = 1.0 / texture->AHeight();
  }
  else
  {
    // special case
    fillColor = ModAlpha(color, 0.5);
  }
  MipInfo mip = GEngine->TextBank()->UseMipmap(texture, 0, 0);

  const int n = 3;
  Vertex2DPixel vs[n];
  // 0
  vs[0].x = cx;
  vs[0].y = cy;
  vs[0].u = coefU * (vs[0].x - offsetX);
  vs[0].v = coefV * (vs[0].y - offsetY);
  vs[0].color = fillColor;
  // 1
  vs[1].color = fillColor;
  // 2
  vs[2].color = fillColor;

  static const float linePts = 5.0f;
  int nSteps = toIntCeil(2 * H_PI * aMap * (1.0f / linePts));
  saturate(nSteps, 6, 720);
  float step = 2 * H_PI * (1.0f / nSteps);
  float lastX = cx, lastY = cy;
  bool lastValid = false;
  float st = 0;
  float ct = 1;
  float sstep = sin(step);
  float cstep = cos(step);
  for (float t=0; t<2*H_PI + 0.5 * step; t+=step)
  {
    float sta = st * aMap;
    float ctb = ct * bMap;
    float x = cx + c * sta + s * ctb;
    float y = cy + s * sta - c * ctb;
    if (lastValid)
    {
      // 1
      vs[1].x = lastX;
      vs[1].y = lastY;
      vs[1].u = coefU * (vs[1].x - offsetX);
      vs[1].v = coefV * (vs[1].y - offsetY);
      // 2
      vs[2].x = x;
      vs[2].y = y;
      vs[2].u = coefU * (vs[2].x - offsetX);
      vs[2].v = coefV * (vs[2].y - offsetY);
      GEngine->DrawPoly(mip, vs, n, _clipRect,NoClamp2D);
    }
    lastX = x;
    lastY = y;
    lastValid = true;
    float stnew = st * cstep + ct * sstep;
    float ctnew = ct * cstep - st * sstep;
    st = stnew; ct = ctnew;
  }
}

void CStaticMap::FillRectangle(Vector3Val position, float a, float b, float angle, PackedColor color, Texture *texture)
{
  DrawCoord posMap = WorldToScreen(position);
  float cx = posMap.x * _wScreen;
  float cy = posMap.y * _hScreen;

  const float invSizeLand = InvLandSize;
  float aMap = a * invSizeLand * _invScaleX * _wScreen;
  float bMap = b * invSizeLand * _invScaleY * _hScreen;

  // TODO: improve clipping
  float r = sqrt(Square(aMap) + Square(bMap));
  if (cx + r < _clipRect.x) return;
  if (cy + r < _clipRect.y) return;
  if (cx - r > _clipRect.x + _clipRect.w) return;
  if (cy - r > _clipRect.y + _clipRect.h) return;

  float s = sin(angle);
  float c = cos(angle);

  PackedColor fillColor = color;

  float coefU = 0;
  float coefV = 0;
  float offsetX = (_mapX + _x) * _wScreen;
  float offsetY = (_mapY + _y) * _hScreen;
  if (texture)
  {
    coefU = 1.0 / texture->AWidth();
    coefV = 1.0 / texture->AHeight();
  }
  else
  {
    // special case
    fillColor = ModAlpha(color, 0.5);
  }

  const int n = 4;
  Vertex2DPixel vs[n];
  // 0
  vs[0].x = cx + c * aMap + s * bMap;
  vs[0].y = cy + s * aMap - c * bMap;
  vs[0].u = coefU * (vs[0].x - offsetX);
  vs[0].v = coefV * (vs[0].y - offsetY);
  vs[0].color = fillColor;
  // 1
  vs[1].x = cx + c * aMap + s * (-bMap);
  vs[1].y = cy + s * aMap - c * (-bMap);
  vs[1].u = coefU * (vs[1].x - offsetX);
  vs[1].v = coefV * (vs[1].y - offsetY);
  vs[1].color = fillColor;
  // 2
  vs[2].x = cx + c * (-aMap) + s * (-bMap);
  vs[2].y = cy + s * (-aMap) - c * (-bMap);
  vs[2].u = coefU * (vs[2].x - offsetX);
  vs[2].v = coefV * (vs[2].y - offsetY);
  vs[2].color = fillColor;
  // 3
  vs[3].x = cx + c * (-aMap) + s * bMap;
  vs[3].y = cy + s * (-aMap) - c * bMap;
  vs[3].u = coefU * (vs[3].x - offsetX);
  vs[3].v = coefV * (vs[3].y - offsetY);
  vs[3].color = fillColor;

  MipInfo mip = GEngine->TextBank()->UseMipmap(texture, 0, 0);
  GEngine->DrawPoly(mip, vs, n, _clipRect, NoClamp2D);
}

