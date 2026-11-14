#ifdef _MSC_VER
#pragma once
#endif

#ifndef _UI_VIEWPORT_HPP
#define _UI_VIEWPORT_HPP

#include "../textbank.hpp"
#include "../engine.hpp"

class UIViewport
{
public:
  virtual ~UIViewport() {}
  
  virtual UIViewport *CreateChild(const Point2DFloat &offset, const Rect2DFloat &clip) = 0;
  virtual void DestroyChild(UIViewport *vp) = 0;
  virtual void GetDrawArea(Rect2DFloat &rect) = 0;

  virtual void DrawText(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawText(const Point2DPixel &pos, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawText(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawText(const Point2DFloat &pos, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawText(const Point2DPixel &pos, const Rect2DPixel &clip, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawText(const Point2DAbs &pos, const Rect2DAbs &clip, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawTextNoClip(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawTextNoClip(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawTextNoClip(const Point2DFloat &pos, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawTextNoClip(const Point2DAbs &pos, const Rect2DAbs &clip, const TextDrawAttr &attr, const char *text) = 0;

  virtual void DrawTextVertical(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawTextVertical(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawTextVertical(const Point2DFloat &pos, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawTextVertical(const Point2DAbs &pos, const Rect2DAbs &clip,  const TextDrawAttr &attr, const char *text) = 0;

  /// oriented text
  virtual void DrawText(const Point2DFloat &pos, const Point2DFloat &right, const Point2DFloat &down, const TextDrawAttr &attr, const char *text) = 0;
  virtual void DrawText(const Point2DFloat &pos, const Point2DFloat &right, const Point2DFloat &down, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text) = 0;

  virtual float GetTextWidth(const TextDrawAttr &attr, const char *text) = 0;

  virtual void DrawFrame(Texture *corner, PackedColor color, const Rect2DPixel &frame) = 0;
  virtual void DrawFrame(Texture *corner, PackedColor color, const Rect2DAbs &frame) = 0;
  virtual void DrawFrame(Texture *corner, PackedColor color, const Rect2DPixel &frame, const Rect2DPixel &clip) = 0;
  virtual void DrawFrame(Texture *corner, PackedColor color, const Rect2DAbs &frame, const Rect2DAbs &clip) = 0;

  virtual void Draw2D(const Draw2DParsExt &pars, const Rect2DAbs &rect) = 0;
  virtual void Draw2D(const Draw2DParsExt &pars, const Rect2DPixel &rect) = 0;
  virtual void Draw2D(const Draw2DParsExt &pars, const Rect2DAbs &rect, const Rect2DAbs &clip) = 0;
  virtual void Draw2D(const Draw2DParsExt &pars, const Rect2DPixel &rect, const Rect2DPixel &clip) = 0;
  virtual void Draw2DNoClip(const Draw2DParsExt &pars, const Rect2DAbs &rect) = 0;
  virtual void Draw2DNoClip(const Draw2DParsExt &pars, const Rect2DPixel &rect) = 0;

  virtual void DrawLine(const Line2DAbs &rect, PackedColor c0, PackedColor c1, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawLine(const Line2DPixel &rect, PackedColor c0, PackedColor c1, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawLine(const Line2DFloat &rect, PackedColor c0, PackedColor c1, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawLine(const Line2DAbs &rect, PackedColor c0, PackedColor c1, const Rect2DAbs &clip, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawLine(const Line2DPixel &rect, PackedColor c0, PackedColor c1, const Rect2DPixel &clip, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawLine(const Line2DFloat &rect, PackedColor c0, PackedColor c1, const Rect2DFloat &clip, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawLineNoClip(const Line2DAbs &rect, PackedColor c0, PackedColor c1, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawLineNoClip(const Line2DPixel &rect, PackedColor c0, PackedColor c1, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawLineNoClip(const Line2DFloat &rect, PackedColor c0, PackedColor c1, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;

  virtual void DrawLines(const Line2DAbsInfo *lines, int nLines, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawLines(const Line2DPixelInfo *lines, int nLines, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawLines(const Line2DAbsInfo *lines, int nLines, const Rect2DAbs &clip, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawLines(const Line2DPixelInfo *lines, int nLines, const Rect2DPixel &clip, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default) = 0;

  virtual void DrawPoly(const MipInfo &mip, const Vertex2DAbs *vertices, int nVertices,
    int specFlags=DefSpecFlags2D, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawPoly(const MipInfo &mip, const Vertex2DPixel *vertices, int nVertices,
    int specFlags=DefSpecFlags2D, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawPoly(const MipInfo &mip, const Vertex2DAbs *vertices, int nVertices,
    const Rect2DAbs &clip, int specFlags=DefSpecFlags2D, const TLMaterial &mat=TLMaterial::_default) = 0;
  virtual void DrawPoly(const MipInfo &mip, const Vertex2DPixel *vertices, int nVertices,
    const Rect2DPixel &clip, int specFlags=DefSpecFlags2D, const TLMaterial &mat=TLMaterial::_default) = 0;
#if _HUD_DRAW_POLYGON
  virtual void DrawPoly(const MipInfo &mip, const Vertex2DFloat *vertices, int nVertices, int specFlags, const TLMaterial &mat) = 0;
#endif
};

// whole screen
UIViewport *Create2DViewport();
void Destroy2DViewport(UIViewport *vp);

// selection on object
UIViewport *Create3DViewport(int cb, Vector3Par pos, Vector3Par right, Vector3Par down, const LightList *lightList, bool sunEnabled, bool cameraSpacePos);
void Destroy3DViewport(UIViewport *vp);

#endif
