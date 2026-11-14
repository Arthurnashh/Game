
// font implementation common to all engines
// (C) 1997, Suma
#include "wpch.hpp"

#include "engine.hpp"
#include "scene.hpp"
#include "font.hpp"
#include "textbank.hpp"
#include <El/QStream/qbStream.hpp>
#include <El/Common/perfProf.hpp>
#include <El/Common/perfLog.hpp>
#include <Es/Strings/bString.hpp>
#include <Es/ReportStack/reportStack.hpp>
#include "global.hpp"
#include "scene.hpp"
#include "camera.hpp"
#include <stdarg.h>

#include "mbcs.hpp"
#include <Es/Common/win.h>

#define KOREAN_STANDARD_CHAR	0x0080

void Engine::DrawText
(
  const Point2DAbs &pos, const TextDrawAttr &attr, const char *text
)
{
  DrawText(pos,Rect2DClipAbs,attr,text);
}

void Engine::DrawText
(
  const Point2DFloat &pos, const TextDrawAttr &attr, const char *text
)
{
  Rect2DFloat clip(-1000,-1000,2000,2000);
  DrawText(pos,clip,attr,text);
}

void Engine::DrawText
(
  const Point2DFloat &pos, const Rect2DFloat &clip,
  const TextDrawAttr &attr, const char *text
)
{
  if (!*text) return;

  Point2DAbs posA;
  Convert(posA,pos);
  Rect2DAbs clipA;
  Convert(clipA,clip);
  DrawText(posA,clipA,attr,text);
}

#if _ENABLE_REPORT
struct TextSizeLogItem
{
  RString _name;
  int _size;
  
  bool operator == (const TextSizeLogItem &with) const
  {
    return _name==with._name && _size==with._size;
  }
  TextSizeLogItem(){}
  TextSizeLogItem(RString name, int size):_name(name),_size(size)
  {
  }
};

TypeIsMovableZeroed(TextSizeLogItem)

static FindArrayKey<TextSizeLogItem> TextSizesUsed;

#endif

const FontWithSize *Engine::SelectFontSize(
  const TextDrawAttr &attr, const char *text, bool &mapOneToOne, float &sizeH, float &sizeW
)
{
  mapOneToOne = false;
  sizeH = 1;
  sizeW = 1;
  
  AspectSettings as;
  GetAspectSettings(as);
  
  Font *font = attr._font;
  if (!font->IsLoaded()) return NULL;
  
  float h=Height2D();
  float aspect = Width()* as.topFOV/(Height()*as.leftFOV);
  
  // we need to select smaller of the sizes to make sure we fit it
  float sizeHW = floatMin(h * attr._size, h * attr._size * aspect);
  float sizeHWMax = floatMax(h * attr._size,  h * attr._size * aspect);
  
  // 7 % allows 5/4 to use same resolution as 4/3
  const float thrAspect = 0.07f;
  // we need to have approximately matching aspect to allow 1:1 mapping
  bool aspect11 = aspect >= 1.0f - thrAspect && aspect <= 1.0f + thrAspect;
  // first attempt 1:1 selection
  const FontWithSize *fontWithSize = aspect11 ? font->Select1To1(sizeHW) : NULL;
  if (!fontWithSize) fontWithSize = font->Select(sizeHWMax);

  float sizeDiff = sizeHW-fontWithSize->AbsHeight();
  // we allow a slightly smaller font (up to 10 %)
  if (aspect11 && (fabs(sizeDiff)<1.3f || sizeDiff>0 && sizeDiff<0.1f*sizeHW))
  {
    mapOneToOne = true;
  }
  else
  {
    // we need to reselect the font
    if (aspect11) fontWithSize = font->Select(sizeHWMax);
  
    sizeH = h * attr._size / fontWithSize->AbsHeight();
    sizeW = sizeH * aspect;
#if _ENABLE_REPORT
    float wantedFontHeight = h * attr._size;

    if (TextSizesUsed.AddUnique(TextSizeLogItem(font->Name(),toInt(wantedFontHeight)))>=0)
    {
      LogF(
        "Warning: Font size %s - wanted %.1f:%.3f, now %d (in text '%s')",
        (const char *)fontWithSize->Name(),
        wantedFontHeight,aspect,
        fontWithSize->AbsHeight(),
        text
      );
    }
#endif
  }
  return fontWithSize;
}

void Engine::DrawText
(
  const Point2DAbs &pos, const Rect2DAbs &clip,
  const TextDrawAttr &attr, const char *text
)
{
  if (!*text) return;
  Font *font = attr._font;
  if (!font->IsLoaded()) return;

  DPRIM_STAT_SCOPE_SIMPLE(Tx2D);
  PROFILE_SCOPE(txt2D);
  SCOPE_GRF(text,0);
  
  Point2DAbs posA = pos;
  Rect2DAbs clipA = clip;

  TempWString<> buffer;
  // TODO: Korean support need to be reworked
  /*
  if (font->GetLangID() == Korean)
  {
    buffer.Realloc(strlen(text) + 1);
    MultiByteStringToPseudoCodeString(text, buffer);
  }
  else
  */
  {
    // check the size of output buffer
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    buffer.Realloc(bufferSize);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, buffer.Data(), bufferSize);
  }

#if _ENABLE_PERFLOG 
  if (LogStatesOnce) LogF("Draw text %s",text);
#endif

  bool mapOneToOne = false;
  float sizeH = 1, sizeW = 1;
  const FontWithSize *fontWithSize = SelectFontSize(attr,text,mapOneToOne,sizeH,sizeW);
 
  // we allow a slightly smaller font (up to 10 %)
  if (mapOneToOne)
  {
    static float pixelOffset = 0.0f;
    posA.x = toIntCeil(posA.x)-pixelOffset;
    posA.y = toIntCeil(posA.y)-pixelOffset;

    // update the clipping rectangle (move the left edge, keep the right one)
    clipA.x = toIntCeil(clip.x) - pixelOffset;
    clipA.y = toIntCeil(clip.y) - pixelOffset;
    clipA.w += clip.x - clipA.x;
    clipA.h += clip.y - clipA.y;
    // expand the clipping rectangle by the AA border
    if (fontWithSize->HasAABorder())
    {
      clipA.x--;
      clipA.y--;
      clipA.w += 2;
      clipA.h += 2;
    }
  }

  const CharInfo *baseChar = fontWithSize->GetInfo('o');
  int baseWidth = attr._fixedWidth || !baseChar ? fontWithSize->AbsWidth() : baseChar->wTex;

  // fix: rounding of letter positions only in 1:1 mapping
  float spaceWidth = baseWidth * font->GetSpaceWidth() * sizeW;
  float spacing = baseWidth * font->GetSpacing() * sizeW;
  if (mapOneToOne)
  {
    spaceWidth = toInt(spaceWidth);
    spacing = toInt(spacing);
  }

  // use font to draw given text

  // convert clipping coordinates to pixel coordinates
  Draw2DPars pars;
  //pars.spec=NoZBuf|IsAlpha|ClampU|ClampV|IsAlphaFog;
  pars.spec=NoZBuf|IsAlpha|ClampU|ClampV|IsAlphaFog|FilterLinear;
  if(attr._shadow == 2) pars.spec|=UISHADOW;
/*
  if (font->GetLangID() == Korean)
  {
    pars.spec |= PointSampling;
  }
*/
  pars.SetColor(attr._color);
  pars.SetU(0,1); // u,v range
  pars.SetV(0,1);

  for (wchar_t *ptr=buffer.Data(); *ptr!=0; ptr++)
  {
    int c = *ptr;
    if (font->GetLangID() == Korean && c >= 0x80)
    {
      // TODO: fix aspect for Korean
      float aspect = 1;
      text = DrawComposedChar(
        posA, pars, clipA,
        attr._size, attr._size * aspect, font,
        text - 1
      );
      posA.x += spacing;
      continue;
    }

    const CharInfo *info = fontWithSize->GetInfo(c);
    // special handling for both space and non-breakable space here
    if (!info || c==0x0020 || c==0x00a0)
    {
      posA.x += spaceWidth + spacing; // add space width
      continue;
    }

    Ref<Texture> texture = info->tex.GetRef();
    Assert(texture);

    float tx = info->xTex;
    float ty = info->yTex;
    float ah = info->hTex;
    float aw = info->wTex;
    
    float sha=sizeH*ah;
    float swa=sizeW*aw;
    // texture may be resized - original texture size is _widthPow2
    // only part of texture is actually occupied by character - _width
    pars.mip=TextBank()->UseMipmap(texture,0,0);
    if( !pars.mip.IsOK() ) continue;
    // call wrapped function
    // draw tx,ty,th,tw region of texture
    int texW=1, texH=1;
    if (texture)
    {
      texW=texture->AWidth();
      texH=texture->AHeight();
    }

    // for single char texture: tw==texW, th==texH, tx==0, ty==0
    float invTW=1.0/texW;
    float invTH=1.0/texH;

    if (fontWithSize->HasAABorder() && mapOneToOne)
    {
      // make sure we render AA border as well
      // this is easy with 1:1 mapping, but it may be difficult in other situations

      pars.SetU((tx-1)*invTW,(tx+aw)*invTW); // u,v range
      pars.SetV((ty-1)*invTH,(ty+ah)*invTH);
      Rect2DAbs absPos(posA.x-1,posA.y-1,swa+1,sha+1);
      Draw2D(pars,absPos,clipA);
//       pars.SetU((tx-1)*invTW,(tx+aw+1)*invTW); // u,v range
//       pars.SetV((ty-1)*invTH,(ty+ah+1)*invTH);
//       Rect2DAbs absPos(posA.x-1,posA.y-1,swa+2,sha+2);
//       Draw2D(pars,absPos,clipA);

    }
    else
    { // old rendering - no AA borders
      //tw*xPart th*yPart
      //tw*aw/tw th*ah/th
      pars.SetU(tx*invTW,(tx+aw)*invTW); // u,v range
      pars.SetV(ty*invTH,(ty+ah)*invTH);
      Rect2DAbs absPos(posA.x,posA.y,swa,sha);
      Draw2D(pars,absPos,clipA);

    }
    

    int width = attr._fixedWidth ? fontWithSize->AbsWidth() : info->kw;
    
    posA.x += width * sizeW + spacing;
  }
}

const char *Engine::DrawComposedChar
(
  Point2DAbs &pos,
  Draw2DPars &pars, const Rect2DAbs &clip,
  float h, float w, Font *font,
  const char *text
)
{
  short picturecode[3];
  if (!PseudoCodeToPictureCode(text, picturecode))
  {
    pos.x += w;
    // consistency with GetTextWidth
    return *(text + 1) ? text + 2 : text + 1;
  }

  if (picturecode[0] != 0x0080)
    DrawComposedChar
    (
      pos, w, h,
      pars, clip, 0, 0, font,
      picturecode[0]
    );
  if (picturecode[1] != 0x0080)
    DrawComposedChar
    (
      pos, w, h,
      pars, clip, 0, 0, font,
      picturecode[1]
    );
  if (picturecode[2] != 0x0080)
    DrawComposedChar
    (
      pos, w, h,
      pars, clip, 0, 0, font,
      picturecode[2]
    );

  pos.x += w;
  return text + 2;
}

void Engine::DrawComposedChar
(
  const Point2DAbs &pos, float w, float h,
  Draw2DPars &pars, const Rect2DAbs &clip, float sizeH, float sizeW, Font *font,
  short c
)
{
  const FontWithSize *fontWithSize = font->Select(h);
  
  const CharInfo *info = fontWithSize->GetInfo(c);
  if (!info) return;

  Ref<Texture> texture = info->tex.GetRef();
  Assert(texture);

  float tx = info->xTex;
  float ty = info->yTex;

  float ah = info->hTex;
  float aw = info->wTex;

  pars.mip = TextBank()->UseMipmap(texture, 0, 0);
  if (pars.mip.IsOK())
  {
    // call wrapped function
    // draw tx,ty,th,tw region of texture
    int texW = 1, texH = 1;
    if (texture)
    {
      texW = texture->AWidth();
      texH = texture->AHeight();
    }

    // for single char texture: tw==texW, th==texH, tx==0, ty==0
    float invTW = 1.0 / texW;
    float invTH = 1.0 / texH;

    float u0 = tx * invTW;
    float u1 = (tx + aw) * invTW;
    float v0 = ty * invTH;
    float v1 = (ty + ah) * invTH;

    pars.SetU(u0, u1);
    pars.SetV(v0, v1);
    
    Draw2D(pars, Rect2DAbs(pos, w, h), clip);
  }
}

void Engine::Draw3D(
  int cb, Vector3Par pos, Vector3Par down, Vector3Par dir, ClipFlags clip,
  PackedColor color, int spec, Texture *tex,
  float x1c, float y1c, float x2c, float y2c
)
{
  REPORTSTACKITEM(RString("Engine::Draw3D"));

  static Ref<ObjectColored> obj;
  if (!obj)
  {

    Ref<LODShape> lShape=new LODShape();
    Shape *shape=new Shape;
    lShape->AddShape(shape,0);

    // initalize lod level
    shape->ReallocTable(4);
    const ClipFlags clip=ClipAll;
    shape->SetPos(0)=VZero;
    shape->SetPos(1)=VZero;
    shape->SetPos(2)=VZero;
    shape->SetPos(3)=VZero;
    shape->SetClipAll(clip);
    shape->SetNorm(0)=Vector3Compressed::Up;
    shape->SetNorm(1)=Vector3Compressed::Up;
    shape->SetNorm(2)=Vector3Compressed::Up;
    shape->SetNorm(3)=Vector3Compressed::Up;
    // precalculate hints for possible optimizations
    shape->CalculateHints();
    lShape->CalculateHints();
    // change face parameters
    Poly face;
    face.Init();
    face.SetN(4);
    face.Set(0,0);
    face.Set(1,1);
    face.Set(2,3);
    face.Set(3,2);
  	shape->ReserveFaces(1);
    shape->AddFace(face);
    
    //shape->RegisterTexture(tex,NULL);
    Assert( shape->NVertex()==4 );
    Assert( shape->NFaces()==1 );

    lShape->CalculateMinMax(true);
    lShape->OrSpecial(IsColored|IsOnSurface|IsAlpha|IsAlphaFog|BestMipmap);

    obj=new ObjectColored(lShape,VISITOR_NO_ID);
    obj->SetOrientation(M3Identity);
  }
  LODShape *lShape = obj->GetShape();

  obj->SetConstantColor(color);
  obj->SetSpecial(spec);

  // use global object
  // TODO: check if modification of shape is legal
  ShapeUsed lock = lShape->Level(0);
  Assert(lock.NotNull());
  Shape *shape = lock.InitShape();

  shape->SetPos(0) = x1c * dir + y1c * down;
  shape->SetPos(1) = x2c * dir + y1c * down;
  shape->SetPos(2) = x1c * dir + y2c * down;
  shape->SetPos(3) = x2c * dir + y2c * down;
    
  Vector3Compressed normal(down.CrossProduct(dir).Normalized());
  shape->SetNorm(0) = normal;
  shape->SetNorm(1) = normal;
  shape->SetNorm(2) = normal;
  shape->SetNorm(3) = normal;

  lShape->SetAutoCenter(false);
  lShape->CalculateMinMax(true);

  {
    float u0 = x1c;
    float u1 = x2c;
    float v0 = y1c;
    float v1 = y2c;

    UVPair minUV(u0, v0), maxUV(u1, v1), invUV;
    if (minUV.u > maxUV.u) Swap(minUV.u, maxUV.u);
    if (minUV.v > maxUV.v) Swap(minUV.v, maxUV.v);
    shape->InitUV(invUV, minUV, maxUV);

    shape->SetUV(0, invUV, u0, v0);
    shape->SetUV(1, invUV, u1, v0);
    shape->SetUV(2, invUV, u0, v1);
    shape->SetUV(3, invUV, u1, v1);
  }
  // TODO:MC: shape changes and dynamic VBs not compatible with MC

  ShapeSectionInfo prop;
  prop.Init();
  prop.SetSpecial(ClampU|ClampV|lShape->Special());
  prop.SetTexture(tex);

  shape->SetAsOneSection(prop);
  shape->RecalculateAreas();

  lShape->OptimizeLevelRendering(0,true);

  obj->SetPosition(pos);
#if _HELISIM_RENDER_TO_TEXTURE
  DoAssert(CheckMainThread());
  obj->Draw(cb, 0, clip, DrawParameters::_default, InstanceParameters::_default, 0, obj->GetFrameBase(), NULL
    , GScene->GetDrawObjectIndex());
#else
  obj->Draw(cb, 0, clip, DrawParameters::_default, InstanceParameters::_default, 0, obj->GetFrameBase(), NULL);
#endif

  prop.SetSpecial(spec);
  prop.SetTexture(NULL); // object is static and we need to make sure texture is not kept over

  shape->SetAsOneSection(prop);
}

static const float SizeFont3D = 48;

bool Engine::IsFontReady3D(Font *font) const
{
  if (!font->IsLoaded()) return false;

  const FontWithSize *fontWithSize = font->Select(SizeFont3D);
  return fontWithSize->IsReady();
}

/*!
\patch_internal 1.59 Date 5/23/2002 by Ondra
- Fixed: Fonts in some dialogs or briefing were sometimes blurred.
\patch 1.78 Date 7/17/2002 by Ondra
- Fixed: Texts in various dialogs were sometimes invisible or dimmed.
\patch 1.90 Date 11/1/2002 by Ondra
- Fixed: Improved text drawing in atypic resolutions like 1600x900.
*/

void Engine::DrawText3D(
  int cb, Vector3Par pos, Vector3Par up, Vector3Par dir, ClipFlags clip,
  Font *font, PackedColor color, int spec, const char *text,
  float x1c, float y1c, float x2c, float y2c
)
{
  REPORTSTACKITEM(RString("Engine::DrawText3D"));

  if (!*text) return;
  if (!font->IsLoaded()) return;

  // this is not really compatible with CBs, and making it compatible seems quite hard
  Assert(cb<0);
  
  DPRIM_STAT_SCOPE_SIMPLE(Tx3D);
  PROFILE_SCOPE(txt3D);
  SCOPE_GRF(text,0);
  
  const FontWithSize *fontWithSize = font->Select(SizeFont3D);

  TempWString<> buffer;
  // TODO: Korean support need to be reworked
  /*
  if (font->GetLangID() == Korean)
  {
    buffer.Realloc(strlen(text) + 1);
    MultiByteStringToPseudoCodeString(text, buffer);
  }
  else
  */
  {
    // check the size of output buffer
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    buffer.Realloc(bufferSize);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, buffer.Data(), bufferSize);
  }

#if _ENABLE_PERFLOG 
  if (LogStatesOnce) LogF("Draw text %s",text);
#endif

  // TODO: build multiple characters buffer and change vertex positions
  // simple implementation (single polygon mesh for each character)
  static Ref<ObjectColored> objText3D;
  if (!objText3D)
  {

    Ref<LODShape> lShape=new LODShape();
    Shape *shape=new Shape;
    lShape->AddShape(shape,0);

    // initalize lod level
    shape->ReallocTable(4);
    const ClipFlags clip=ClipAll;
    shape->SetClipAll(clip);
    shape->SetNorm(0)=Vector3Compressed::Up;
    shape->SetNorm(1)=Vector3Compressed::Up;
    shape->SetNorm(2)=Vector3Compressed::Up;
    shape->SetNorm(3)=Vector3Compressed::Up;
    // precalculate hints for possible optimizations
    shape->CalculateHints();
    lShape->CalculateHints();
    // change face parameters
    Poly face;
    face.Init();
    face.SetN(4);
    face.Set(0,0);
    face.Set(1,1);
    face.Set(2,3);
    face.Set(3,2);
  	shape->ReserveFaces(1);
    shape->AddFace(face);
    Assert( shape->NVertex()==4 );
    Assert( shape->NFaces()==1 );

    lShape->OrSpecial(IsColored|IsOnSurface|IsAlpha|IsAlphaFog|NoZWrite);

    objText3D=new ObjectColored(lShape,VISITOR_NO_ID);
    objText3D->SetOrientation(M3Identity);
  }

  DoAssert((clip&ClipUserMask)/ClipUserStep==0);
  
  objText3D->SetConstantColor(color);
  objText3D->SetSpecial(spec);
  // use global char object
  LODShape *lShape = objText3D->GetShape();
  
  // TODO: check if modification of shape is legal
  ShapeUsed lock = lShape->Level(0);
  Assert(lock.NotNull());
  Shape *shape = lock.InitShape();

  shape->SetClipAll(clip|ClipAll);
  shape->CalculateHints();

  // calculate z2 factor
  float z2=GScene->GetCamera()->Position().Distance2(pos);

  float fontMH = fontWithSize->AbsHeight();
  float invMH = 1.0 / fontMH; // TODO: precalc
  float y1cf = y1c * fontMH;
  float y2cf = y2c * fontMH;

  Char3DContext ctx;
  ctx.dir = dir;
  ctx.up = up;
  ctx.font = font;
  ctx.obj = objText3D;
  ctx.z2 = z2;
  ctx.x1c = x1c;
  ctx.x2c = x2c;
  ctx.y1c = y1c;
  ctx.y2c = y2c;
  ctx.clip = clip;
  ctx.spec = spec;

  float aotFactor = GScene->GetCamera()->GetAOTFactor();

  const CharInfo *baseChar = fontWithSize->GetInfo('o');
  // float baseWidth = attr._fixedWidth || !baseChar ? fontWithSize->AbsWidth() : baseChar->wTex;
  int baseWidth = !baseChar ? fontWithSize->AbsWidth() : baseChar->wTex;

  float spaceWidth = baseWidth * font->GetSpaceWidth() * invMH;
  float spacing = baseWidth * font->GetSpacing() * invMH;

  float xPos = 0;
  Vector3 lPos = pos;
  for (wchar_t *ptr=buffer.Data(); *ptr!=0; ptr++)
  {
    int c = *ptr;
    if (font->GetLangID() == Korean && c >= 0x80)
    {
      text = DrawComposedChar3D(cb,xPos, lPos, ctx, text - 1);
      xPos += spacing;
      lPos += spacing * dir;
      continue;
    }

    const CharInfo *info = fontWithSize->GetInfo(c);
    // special handling for both space and non-breakable space here
    if (!info || c==0x0020 || c==0x00a0)
    {
      // white character
      xPos += spaceWidth + spacing;
      lPos += (spaceWidth + spacing) * dir;
      continue;
    }

    float wp = info->wTex * invMH;
    if (xPos + wp >= x1c && xPos < x2c)
    {
      float x1cf = 0;
      float x2cf = 1;
      if (x1c > xPos) x1cf = (x1c - xPos) * (1.0 / wp);
      if (x2c < xPos + wp) x2cf = (x2c - xPos) * (1.0 / wp);

      //Poly &face = shape->Face(shape->BeginFaces());

      Ref<Texture> texture = info->tex.GetRef();

      //float th=font->_hTex[c];
      //float tw=font->_wTex[c];

      // whole texture size
      int texW=1, texH=1;
      if (texture)
      {
        texW=texture->AWidth();
        texH=texture->AHeight();
      }

      float tx=info->xTex; // single character position
      float ty=info->yTex;
      float ah=info->hTex;
      float aw=info->wTex;

      // draw tx,ty,ah,aw region of texture
      float invTW=1.0/texW;
      float invTH=1.0/texH;


      float u0=0, v0=0, u1=0, v1=0;
      if (texture)
      {
        float bottom = floatMin(y2cf, ah);
        u0 = (tx+x1cf*aw)*invTW;
        v0 = (ty+y1cf)*invTH;
        u1 = (tx+x2cf*aw)*invTW;
        v1 = (ty+bottom)*invTH;

        // calculate texture area and area
        float area = dir.CrossProduct(up).Size();
        //float areaOTex = area*invMH*invMH *texW*texH*0.33;
        float uvArea = (u1-u0)*(v1-v0);
        float areaOTex = area/uvArea;
        //float uvAre2 = 1/(invMH*invMH *texW*texH*3);
        // note: areaOTex should be constant accross whole font
        
        texture->PrepareTextureScr(z2,spec,areaOTex*aotFactor);
      }



      float rh = -floatMin(ah * invMH, y2c);
      float rw = aw * invMH;
      shape->SetPos(0) = dir * (x1cf * rw) - up * y1c;
      shape->SetPos(1) = dir * (x2cf * rw) - up * y1c;
      shape->SetPos(2) = dir * (x1cf * rw) + up * rh;
      shape->SetPos(3) = dir * (x2cf * rw) + up * rh;

      Vector3Compressed normal(dir.CrossProduct(up).Normalized());
      shape->SetNorm(0)=normal;
      shape->SetNorm(1)=normal;
      shape->SetNorm(2)=normal;
      shape->SetNorm(3)=normal;

      UVPair minUV(u0, v0), maxUV(u1, v1), invUV;
      if (minUV.u > maxUV.u) Swap(minUV.u, maxUV.u);
      if (minUV.v > maxUV.v) Swap(minUV.v, maxUV.v);
      shape->InitUV(invUV, minUV, maxUV);

      shape->SetUV(0, invUV, u0, v0);
      shape->SetUV(1, invUV, u1, v0);
      shape->SetUV(2, invUV, u0, v1);
      shape->SetUV(3, invUV, u1, v1);

      ShapeSectionInfo prop;
      prop.Init();
      prop.SetSpecial(ClampU|ClampV|lShape->Special());
      prop.SetTexture(texture);

      shape->SetAsOneSection(prop);
      shape->RecalculateAreas();

      // for each letter:
      lShape->SetAutoCenter(false);
      lShape->CalculateMinMax(true);
      lShape->OptimizeLevelRendering(0,true);
      objText3D->SetPosition(lPos);

#if _HELISIM_RENDER_TO_TEXTURE
      DoAssert(CheckMainThread());
      objText3D->Draw(cb,0, clip&ClipAll, DrawParameters::_default, InstanceParameters::_default, 0, objText3D->GetFrameBase(), NULL
        , GScene->GetDrawObjectIndex()
        );
#else
      objText3D->Draw(cb,0, clip&ClipAll, DrawParameters::_default, InstanceParameters::_default, 0, objText3D->GetFrameBase(), NULL);
#endif
      
      prop.SetSpecial(spec);
      prop.SetTexture(NULL); // object is static and we need to make sure texture is not kept over
      shape->SetAsOneSection(prop);
    }
    xPos += wp + spacing;
    lPos += (wp + spacing) * dir;
  }

}

void Engine::DrawText3DCenteredAsync(
  Vector3Par center, Vector3Par up, Vector3Par aside, ClipFlags clip,
  Font *font, PackedColor color, int spec, const char *text,
  float x1c, float y1c, float x2c, float y2c
)
{
  }

const char *Engine::DrawComposedChar3D(int cb, float &xPos, Vector3 &lPos, Char3DContext &ctx, const char *text)
{
  const FontWithSize *fontWithSize = ctx.font->Select(SizeFont3D);

  float fontMH = fontWithSize->AbsHeight();
  float invMH = 1.0 / fontMH; // TODO: precalc

  const CharInfo *info = fontWithSize->GetInfo(KOREAN_STANDARD_CHAR);
  if (info)
  {
    float h = info->hTex * invMH;
    float w = info->wTex * invMH;

    short picturecode[3];
    if (!PseudoCodeToPictureCode(text, picturecode))
    {
      xPos += w;
      lPos += w * ctx.dir;
      // consistency with GetTextWidth
      return *(text + 1) ? text + 2 : text + 1;
    }

    if (picturecode[0] != 0x0080)
      DrawComposedChar3D(cb,xPos, lPos, w, h, ctx, picturecode[0]);
    if (picturecode[1] != 0x0080)
      DrawComposedChar3D(cb,xPos, lPos, w, h, ctx, picturecode[1]);
    if (picturecode[2] != 0x0080)
      DrawComposedChar3D(cb,xPos, lPos, w, h, ctx, picturecode[2]);
    xPos += w;
    lPos += w * ctx.dir;
  }

  return text + 2;
}

void Engine::DrawComposedChar3D(
  int cb, float xPos, Vector3Par lPos, float w, float h, Char3DContext &ctx, short c
)
{
  REPORTSTACKITEM(RString("Engine::DrawComposedChar3D"));

  const FontWithSize *fontWithSize = ctx.font->Select(SizeFont3D);
  const CharInfo *info = fontWithSize->GetInfo(c);
  if (!info) return;

  if (xPos + w >= ctx.x1c && xPos <= ctx.x2c)
  {
    LODShape *lShape = ctx.obj->GetShape();
    // TODO: check if modification of shape is legal
    ShapeUsed lock = lShape->Level(0);
    Assert(lock.NotNull());
    Shape *shape = lock.InitShape();

    float fontMH = fontWithSize->AbsHeight();
    // float invMH = 1.0 / fontMH; // TODO: precalc
    float y1cf = ctx.y1c * fontMH;
    float y2cf = ctx.y2c * fontMH;

    float x1cf = 0;
    float x2cf = 1;
    if (ctx.x1c > xPos) x1cf = (ctx.x1c - xPos) * (1.0 / w);
    if (ctx.x2c < xPos + w) x2cf = (ctx.x2c - xPos) * (1.0 / w);

    //Poly &face = shape->Face(shape->BeginFaces());

    Ref<Texture> texture = info->tex.GetRef();

    //float th=font->_hTex[c];
    //float tw=font->_wTex[c];

    // whole texture size
    int texW=1, texH=1;
    if (texture)
    {
      texW=texture->AWidth();
      texH=texture->AHeight();
    }

    float tx = info->xTex; // single character position
    float ty = info->yTex;
    float ah = info->hTex;
    float aw = info->wTex;

    // draw tx,ty,ah,aw region of texture
    float invTW=1.0/texW;
    float invTH=1.0/texH;


    float u0=0, v0=0, u1=0, v1=0;
    if (texture)
    {
      float bottom = floatMin(y2cf, ah);
      u0 = (tx+x1cf*aw)*invTW;
      v0 = (ty+y1cf)*invTH;
      u1 = (tx+x2cf*aw)*invTW;
      v1 = (ty+bottom)*invTH;

      // calculate texture area and area
      float area = ctx.dir.CrossProduct(ctx.up).Size();
      // float areaOTex = area*invMH*invMH *texW*texH;
      float uvArea = (u1-u0)*(v1-v0);
      float areaOTex = area/uvArea;
      // note: areaOTex should be constant across whole font
      float aotFactor = GScene->GetCamera()->GetAOTFactor();
      
      texture->PrepareTextureScr(ctx.z2,ctx.spec,areaOTex*aotFactor);
    }


    float rh = -floatMin(h, ctx.y2c);
    float rw = w;

    shape->SetPos(0) = ctx.dir * (x1cf * rw) - ctx.up * ctx.y1c;
    shape->SetPos(1) = ctx.dir * (x2cf * rw) - ctx.up * ctx.y1c;
    shape->SetPos(2) = ctx.dir * (x1cf * rw) + ctx.up * rh;
    shape->SetPos(3) = ctx.dir * (x2cf * rw) + ctx.up * rh;

    Vector3Compressed normal(ctx.dir.CrossProduct(ctx.up).Normalized());
    shape->SetNorm(0)=normal;
    shape->SetNorm(1)=normal;
    shape->SetNorm(2)=normal;
    shape->SetNorm(3)=normal;

    UVPair minUV(u0, v0), maxUV(u1, v1), invUV;
    if (minUV.u > maxUV.u) Swap(minUV.u, maxUV.u);
    if (minUV.v > maxUV.v) Swap(minUV.v, maxUV.v);
    shape->InitUV(invUV, minUV, maxUV);

    shape->SetUV(0, invUV, u0, v0);
    shape->SetUV(1, invUV, u1, v0);
    shape->SetUV(2, invUV, u0, v1);
    shape->SetUV(3, invUV, u1, v1);

    ShapeSectionInfo prop;
    prop.Init();
    prop.SetSpecial(ClampU|ClampV|lShape->Special());
    prop.SetTexture(texture);

    shape->SetAsOneSection(prop);
    shape->RecalculateAreas();

    // for each letter:
    lShape->SetAutoCenter(false);
    lShape->CalculateMinMax(true);
    lShape->OptimizeLevelRendering(0,true);

    // for each letter:
    ctx.obj->SetPosition(lPos);
#if _HELISIM_RENDER_TO_TEXTURE
    DoAssert(CheckMainThread());
    ctx.obj->Draw(cb,0, ctx.clip&ClipAll, DrawParameters::_default, InstanceParameters::_default, 0, ctx.obj->GetFrameBase(), NULL
      , GScene->GetDrawObjectIndex());
#else
    ctx.obj->Draw(cb,0, ctx.clip&ClipAll, DrawParameters::_default, InstanceParameters::_default, 0, ctx.obj->GetFrameBase(), NULL);
#endif
  }
}

Vector3 Engine::GetText3DWidth(
  Vector3Par dir, Font *font, const char *text
)
{
  if (!font->IsLoaded()) return VZero;
  const FontWithSize *fontWithSize = font->Select(SizeFont3D);

  TempWString<> buffer;
  // TODO: Korean support need to be reworked
  /*
  if (font->GetLangID() == Korean)
  {
    buffer.Realloc(strlen(text) + 1);
    MultiByteStringToPseudoCodeString(text, buffer);
  }
  else
  */
  {
    // check the size of output buffer
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    buffer.Realloc(bufferSize);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, buffer.Data(), bufferSize);
  }

  const CharInfo *baseChar = fontWithSize->GetInfo('o');
  // int baseWidth = attr._fixedWidth || !baseChar ? fontWithSize->AbsWidth() : baseChar->wTex;
  int baseWidth = !baseChar ? fontWithSize->AbsWidth() : baseChar->wTex;

  float spaceWidth = baseWidth * font->GetSpaceWidth();
  float spacing = baseWidth * font->GetSpacing();

  float width = 0;
  for (wchar_t *ptr=buffer.Data(); *ptr!=0; ptr++)
  {
    int c = *ptr;
    if (font->GetLangID() == Korean && c >= 0x80)
    {
      const CharInfo *info = fontWithSize->GetInfo(KOREAN_STANDARD_CHAR);
      if (info) width += info->wTex + spacing;
      if (*text) text++;
      continue;
    }

    const CharInfo *info = fontWithSize->GetInfo(c);
    // special handling for both space and non-breakable space here
    if (!info || c==0x0020 || c==0x00a0)
    {
      width += spaceWidth + spacing; // add space width
    }
    else
    {
      width += info->wTex + spacing;
    }
  }
  float invMH = 1.0 / fontWithSize->AbsHeight(); // TODO: precalc
  return width * invMH * dir;
}

Vector3 CCALL Engine::GetText3DWidthF
(
  Vector3Par dir, Font *font, const char *text, ...
)
{
  BString<2048> buf;
  va_list arglist;
  va_start(arglist, text);
  vsprintf(buf, text, arglist);
  va_end(arglist);
  return GetText3DWidth(dir, font, buf);
}

void CCALL Engine::DrawTextF
(
  const Point2DAbs &pos, float size,
  Font *font, PackedColor color, int shadow, const char *text, ...
)
{
  BString<2048> buf;
  va_list arglist;
  va_start( arglist, text );
  vsprintf( buf, text, arglist );
  va_end( arglist );
  DrawText(pos,size,font,color,buf, shadow);
}

void CCALL Engine::DrawTextF
(
  const Point2DFloat &pos, float size,
  Font *font, PackedColor color, int shadow, const char *text, ...
)
{
  BString<2048> buf;
  va_list arglist;
  va_start( arglist, text );
  vsprintf( buf, text, arglist );
  va_end( arglist );
  DrawText(pos,size,font,color,buf, shadow);
}

void CCALL Engine::DrawTextF
(
  const Point2DAbs &pos, const TextDrawAttr &attr, const char *text, ...
)
{
  BString<2048> buf;
  va_list arglist;
  va_start( arglist, text );
  vsprintf( buf, text, arglist );
  va_end( arglist );
  DrawText(pos,attr,buf);
}

void CCALL Engine::DrawTextF
(
  const Point2DFloat &pos, const TextDrawAttr &attr, const char *text, ...
)
{
  BString<2048> buf;
  va_list arglist;
  va_start( arglist, text );
  vsprintf( buf, text, arglist );
  va_end( arglist );
  DrawText(pos,attr,buf);
}

void Engine::DrawText3DF
(
  int cb, Vector3Par pos, Vector3Par up, Vector3Par dir, ClipFlags clip,
  Font *font, PackedColor color, int spec, const char *text, ...
)
{
  BString<2048> buf;
  va_list arglist;
  va_start( arglist, text );
  vsprintf( buf, text, arglist );
  va_end( arglist );
  DrawText3D(cb,pos,up,dir,clip,font,color,spec,buf);
}

void CCALL Engine::DrawTextF
(
  const Point2DFloat &pos, float size,
  const Rect2DFloat &clip,
  Font *font, PackedColor color, int shadow, const char *text, ...
)
{
  BString<2048> buf;
  va_list arglist;
  va_start( arglist, text );
  vsprintf( buf, text, arglist );
  va_end( arglist );
  DrawText(pos,size,clip,font,color,buf, shadow);
}

void CCALL Engine::DrawTextF
(
 const Point2DFloat &pos, const Rect2DFloat &clip,
 const TextDrawAttr &attr, const char *text, ...
 )
{
  BString<2048> buf;
  va_list arglist;
  va_start( arglist, text );
  vsprintf( buf, text, arglist );
  va_end( arglist );
  DrawText(pos,clip,attr,buf);
}

float CCALL Engine::GetTextWidthF
(
  float size, Font *font, const char *text, ...
)
{
  BString<2048> buf;
  va_list arglist;
  va_start( arglist, text );
  vsprintf( buf, text, arglist );
  va_end( arglist );
  return GetTextWidth(size,font,buf);
}

void Engine::DrawTextVertical
(
  const Point2DFloat &pos, float size, Font *font, PackedColor color, const char *text
)
{
  Rect2DFloat clip(-1000,-1000,2000,2000);
  DrawTextVertical(pos,size,clip,font,color,text);
}

void Engine::DrawTextVertical
(
  const Point2DFloat &pos, float sizeEx,
  const Rect2DFloat &clip,
  Font *font, PackedColor color, const char *text
)
{
  Fail("Obsolete, sizeH and sizeW calculation missing");
}

void CCALL Engine::DrawTextVerticalF
(
  const Point2DFloat &pos, float size,
  Font *font, PackedColor color, const char *text, ...
)
{
  char buf[2048];
  va_list arglist;
  va_start( arglist, text );
  vsnprintf( buf, sizeof(buf), text, arglist );
  va_end( arglist );
  DrawTextVertical(pos,size,font,color,buf);
}

void CCALL Engine::DrawTextVerticalF
(
  const Point2DFloat &pos, float size,
  const Rect2DFloat &clip,
  Font *font, PackedColor color, const char *text, ...
)
{
  char buf[2048];
  va_list arglist;
  va_start( arglist, text );
  vsnprintf( buf, sizeof(buf), text, arglist );
  va_end( arglist );
  DrawTextVertical(pos,size,clip,font,color,buf);
}

float Engine::GetTextWidth(const TextDrawAttr &attr, const char *text)
{
  Font *font = attr._font;
  if (!font->IsLoaded()) return 0;
  float w=Width2D();
  float x=0;

  bool mapOneToOne = false;
  float sizeH = 1, sizeW = 1;
  const FontWithSize *fontWithSize = SelectFontSize(attr,text,mapOneToOne,sizeH,sizeW);
 
  TempWString<> buffer;
  if (font->GetLangID() == Korean)
  {
    Fail("Korean support needs to be reworked");
    //buffer.Realloc(strlen(text) + 1);
    //MultiByteStringToPseudoCodeString(text, buffer);
  }
  //else
  {
    // check the size of output buffer
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    buffer.Realloc(bufferSize);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, buffer.Data(), bufferSize);
  }

  const CharInfo *baseChar = fontWithSize->GetInfo('o');
  int baseWidth = attr._fixedWidth || !baseChar ? fontWithSize->AbsWidth() : baseChar->wTex;

  // fix: rounding of letter positions only in 1:1 mapping
  float spaceWidth = baseWidth * font->GetSpaceWidth() * sizeW;
  float spacing = baseWidth * font->GetSpacing() * sizeW;
  if (mapOneToOne)
  {
    spaceWidth = toInt(spaceWidth);
    spacing = toInt(spacing);
  }

  for (wchar_t *ptr=buffer.Data(); *ptr!=0; ptr++)
  {
    int c = *ptr;
    if (font->GetLangID() == Korean && c >= 0x80)
    {
      // TODO: fix aspect for Korean
      float aspect = 1;
      float w = attr._size * aspect;
      x += toInt(w) + spacing;
      if (*text) text++;
      continue;
    }

    const CharInfo *info = fontWithSize->GetInfo(c);
    // special handling for both space and non-breakable space here
    if (!info || c==0x0020 || c==0x00a0)
    {
      x += spaceWidth + spacing;
    }
    else
    {
      int width = attr._fixedWidth ? fontWithSize->AbsWidth() : info->kw;
      x += width * sizeW + spacing;
    }
  }
  return x/w;
}
