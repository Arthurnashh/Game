#include "../wpch.hpp"
#include "uiViewport.hpp"
#include "../scene.hpp"

static void Intersect(Rect2DAbs &result, const Rect2DAbs &rect1, const Rect2DAbs &rect2)
{
  result.x = floatMax(rect1.x, rect2.x);
  result.y = floatMax(rect1.y, rect2.y);
  result.w = floatMin(rect1.x + rect1.w, rect2.x + rect2.w) - result.x;
  result.h = floatMin(rect1.y + rect1.h, rect2.y + rect2.h) - result.y;
}

static void Intersect(Rect2DFloat &result, const Rect2DFloat &rect1, const Rect2DFloat &rect2)
{
  result.x = floatMax(rect1.x, rect2.x);
  result.y = floatMax(rect1.y, rect2.y);
  result.w = floatMin(rect1.x + rect1.w, rect2.x + rect2.w) - result.x;
  result.h = floatMin(rect1.y + rect1.h, rect2.y + rect2.h) - result.y;
}

class Viewport2D : public UIViewport
{
protected:
  Point2DAbs _origin;
  Rect2DAbs _rect;

public:
  Viewport2D();
  Viewport2D(Point2DAbs &origin, Rect2DAbs &rect);

  UIViewport *CreateChild(const Point2DFloat &offset, const Rect2DFloat &clip);
  void DestroyChild(UIViewport *vp);
  
  void GetDrawArea(Rect2DFloat &rect);

  void DrawText(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DPixel &pos, const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DFloat &pos, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DPixel &pos, const Rect2DPixel &clip, const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DAbs &pos, const Rect2DAbs &clip, const TextDrawAttr &attr, const char *text);
  void DrawTextNoClip(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text);
  void DrawTextNoClip(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text);
  void DrawTextNoClip(const Point2DFloat &pos, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text);
  void DrawTextNoClip(const Point2DAbs &pos, const Rect2DAbs &clip, const TextDrawAttr &attr, const char *text);

  void DrawText(const Point2DFloat &pos, const Point2DFloat &right, const Point2DFloat &down, const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DFloat &pos, const Point2DFloat &right, const Point2DFloat &down, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text);

  void DrawTextVertical(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text);
  void DrawTextVertical(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text);
  void DrawTextVertical(const Point2DFloat &pos, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text);
  void DrawTextVertical(const Point2DAbs &pos, const Rect2DAbs &clip, const TextDrawAttr &attr, const char *text);

  float GetTextWidth(const TextDrawAttr &attr, const char *text);

  void DrawFrame(Texture *corner, PackedColor color, const Rect2DPixel &frame);
  void DrawFrame(Texture *corner, PackedColor color, const Rect2DAbs &frame);
  void DrawFrame(Texture *corner, PackedColor color, const Rect2DPixel &frame, const Rect2DPixel &clip);
  void DrawFrame(Texture *corner, PackedColor color, const Rect2DAbs &frame, const Rect2DAbs &clip);

  void Draw2D(const Draw2DParsExt &pars, const Rect2DAbs &rect);
  void Draw2D(const Draw2DParsExt &pars, const Rect2DPixel &rect);
  void Draw2D(const Draw2DParsExt &pars, const Rect2DAbs &rect, const Rect2DAbs &clip);
  void Draw2D(const Draw2DParsExt &pars, const Rect2DPixel &rect, const Rect2DPixel &clip);
  void Draw2DNoClip(const Draw2DParsExt &pars, const Rect2DAbs &rect);
  void Draw2DNoClip(const Draw2DParsExt &pars, const Rect2DPixel &rect);

  void DrawLine(const Line2DAbs &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat);
  void DrawLine(const Line2DPixel &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat);
  void DrawLine(const Line2DFloat &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat);
  void DrawLine(const Line2DAbs &rect, PackedColor c0, PackedColor c1, const Rect2DAbs &clip, float width, int specFlags, const TLMaterial &mat);
  void DrawLine(const Line2DPixel &rect, PackedColor c0, PackedColor c1, const Rect2DPixel &clip, float width, int specFlags, const TLMaterial &mat);
  void DrawLine(const Line2DFloat &rect, PackedColor c0, PackedColor c1, const Rect2DFloat &clip, float width, int specFlags, const TLMaterial &mat);
  void DrawLineNoClip(const Line2DAbs &rect, PackedColor c0, PackedColor c1, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default);
  void DrawLineNoClip(const Line2DPixel &rect, PackedColor c0, PackedColor c1, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default);
  void DrawLineNoClip(const Line2DFloat &rect, PackedColor c0, PackedColor c1, float width=1.0f, int specFlags=0, const TLMaterial &mat=TLMaterial::_default);

  void DrawLines(const Line2DAbsInfo *lines, int nLines, float width, int specFlags, const TLMaterial &mat);
  void DrawLines(const Line2DPixelInfo *lines, int nLines, float width, int specFlags, const TLMaterial &mat);
  void DrawLines(const Line2DAbsInfo *lines, int nLines, const Rect2DAbs &clip, float width, int specFlags, const TLMaterial &mat);
  void DrawLines(const Line2DPixelInfo *lines, int nLines, const Rect2DPixel &clip, float width, int specFlags, const TLMaterial &mat);

  void DrawPoly(const MipInfo &mip, const Vertex2DAbs *vertices, int nVertices, int specFlags, const TLMaterial &mat);
  void DrawPoly(const MipInfo &mip, const Vertex2DPixel *vertices, int nVertices, int specFlags, const TLMaterial &mat);
  void DrawPoly(const MipInfo &mip, const Vertex2DAbs *vertices, int nVertices, const Rect2DAbs &clip, int specFlags, const TLMaterial &mat);
  void DrawPoly(const MipInfo &mip, const Vertex2DPixel *vertices, int nVertices, const Rect2DPixel &clip, int specFlags, const TLMaterial &mat);

#if _HUD_DRAW_POLYGON
  void DrawPoly(const MipInfo &mip, const Vertex2DFloat *vertices, int nVertices, int specFlags, const TLMaterial &mat) {};
#endif

protected:
  // transformation of coordinates
  template <typename PointType>
  void Transform(Point2DAbs &result, const PointType &input);

  template <typename RectType>
  void Transform(Rect2DAbs &result, const RectType &input);

  template <typename RectType>
  void Clip(Rect2DAbs &result, const RectType &input);

  template <typename LineType>
  void Transform(Line2DAbs &result, const LineType &input);
};

template <typename PointType>
void Viewport2D::Transform(Point2DAbs &result, const PointType &input)
{
  GEngine->Convert(result, input);
  result.x += _origin.x;
  result.y += _origin.y;
}

template <>
void Viewport2D::Transform(Point2DAbs &result, const Point2DAbs &input)
{
  result.x = _origin.x + input.x;
  result.y = _origin.y + input.y;
}

template <typename RectType>
void Viewport2D::Transform(Rect2DAbs &result, const RectType &input)
{
  GEngine->Convert(result, input);
  result.x += _origin.x;
  result.y += _origin.y;
}

template <>
void Viewport2D::Transform(Rect2DAbs &result, const Rect2DAbs &input)
{
  result.x = _origin.x + input.x;
  result.y = _origin.y + input.y;
  result.w = input.w;
  result.h = input.h;
}

template <typename RectType>
void Viewport2D::Clip(Rect2DAbs &result, const RectType &input)
{
  Rect2DAbs temp;
  Transform(temp, input);
  Intersect(result, _rect, temp);
}

template <typename LineType>
void Viewport2D::Transform(Line2DAbs &result, const LineType &input)
{
  Transform(result.beg, input.beg);
  Transform(result.end, input.end);
}

Viewport2D::Viewport2D()
: _rect(Rect2DClipAbs), _origin(0, 0)
{}

Viewport2D::Viewport2D(Point2DAbs &origin, Rect2DAbs &rect)
: _origin(origin), _rect(rect)
{}

UIViewport *Viewport2D::CreateChild(const Point2DFloat &offset, const Rect2DFloat &clip)
{
  Point2DAbs origin;
  Transform(origin, offset);
  Point2DAbs zero;
  GEngine->Convert(zero, Point2DFloat(0, 0));
  origin.x -= zero.x;
  origin.y -= zero.y; 
  Rect2DAbs rect;
  Clip(rect, clip);
  return new Viewport2D(origin, rect);
}

void Viewport2D::DestroyChild(UIViewport *vp)
{
  delete vp;
}

void Viewport2D::GetDrawArea(Rect2DFloat &rect)
{
  GEngine->Convert(rect, _rect);
}

void Viewport2D::DrawText(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  GEngine->DrawText(posT, _rect, attr, text);
}

void Viewport2D::DrawText(const Point2DPixel &pos, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  GEngine->DrawText(posT, _rect, attr, text);
}

void Viewport2D::DrawText(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  GEngine->DrawText(posT, _rect, attr, text);
}

void Viewport2D::DrawText(const Point2DFloat &pos, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  GEngine->DrawText(posT, clipT, attr, text);
}

void Viewport2D::DrawText(const Point2DPixel &pos, const Rect2DPixel &clip, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  GEngine->DrawText(posT, clipT, attr, text);
}

void Viewport2D::DrawText(const Point2DAbs &pos, const Rect2DAbs &clip, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  GEngine->DrawText(posT, clipT, attr, text);
}

void Viewport2D::DrawTextNoClip(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  GEngine->DrawText(posT, Rect2DClipAbs, attr, text);
}

void Viewport2D::DrawTextNoClip(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  GEngine->DrawText(posT, Rect2DClipAbs, attr, text);
}

void Viewport2D::DrawTextNoClip(const Point2DFloat &pos, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  Rect2DAbs clipT;
  Transform(clipT, clip);
  GEngine->DrawText(posT, clipT, attr, text);
}

void Viewport2D::DrawTextNoClip(const Point2DAbs &pos, const Rect2DAbs &clip, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  Rect2DAbs clipT;
  Transform(clipT, clip);
  GEngine->DrawText(posT, clipT, attr, text);
}

void Viewport2D::DrawText(const Point2DFloat &pos, const Point2DFloat &right, const Point2DFloat &down, const TextDrawAttr &attr, const char *text)
{
  Fail("Not implemented");
}

void Viewport2D::DrawText(const Point2DFloat &posF, const Point2DFloat &rightF, const Point2DFloat &downF, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text)
{
  Fail("Not implemented");
}

// TODO: optimize or remove
void Viewport2D::DrawTextVertical(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  Point2DFloat posF;
  GEngine->Convert(posF, posT);
  Rect2DFloat clipF;
  GEngine->Convert(clipF, _rect);
  GEngine->DrawTextVertical(posF, attr._size, clipF, attr._font, attr._color, text);
}

void Viewport2D::DrawTextVertical(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  Point2DFloat posF;
  GEngine->Convert(posF, posT);
  Rect2DFloat clipF;
  GEngine->Convert(clipF, _rect);
  GEngine->DrawTextVertical(posF, attr._size, clipF, attr._font, attr._color, text);
}

void Viewport2D::DrawTextVertical(const Point2DFloat &pos, const Rect2DFloat &clip, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  Point2DFloat posF;
  GEngine->Convert(posF, posT);
  Rect2DFloat clipF;
  GEngine->Convert(clipF, clipT);
  GEngine->DrawTextVertical(posF, attr._size, clipF, attr._font, attr._color, text);
}

void Viewport2D::DrawTextVertical(const Point2DAbs &pos, const Rect2DAbs &clip, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posT;
  Transform(posT, pos);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  Point2DFloat posF;
  GEngine->Convert(posF, posT);
  Rect2DFloat clipF;
  GEngine->Convert(clipF, clipT);
  GEngine->DrawTextVertical(posF, attr._size, clipF, attr._font, attr._color, text);
}

float Viewport2D::GetTextWidth(const TextDrawAttr &attr, const char *text)
{
  return GEngine->GetTextWidth(attr, text);
}

void Viewport2D::DrawFrame(Texture *corner, PackedColor color, const Rect2DPixel &frame)
{
  Rect2DAbs frameT;
  Transform(frameT, frame);
  GEngine->DrawFrame(corner, color, frameT, _rect);
}

void Viewport2D::DrawFrame(Texture *corner, PackedColor color, const Rect2DAbs &frame)
{
  Rect2DAbs frameT;
  Transform(frameT, frame);
  GEngine->DrawFrame(corner, color, frameT, _rect);
}

void Viewport2D::DrawFrame(Texture *corner, PackedColor color, const Rect2DPixel &frame, const Rect2DPixel &clip)
{
  Rect2DAbs frameT;
  Transform(frameT, frame);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  GEngine->DrawFrame(corner, color, frameT, clipT);
}

void Viewport2D::DrawFrame(Texture *corner, PackedColor color, const Rect2DAbs &frame, const Rect2DAbs &clip)
{
  Rect2DAbs frameT;
  Transform(frameT, frame);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  GEngine->DrawFrame(corner, color, frameT, clipT);
}

void Viewport2D::Draw2D(const Draw2DParsExt &pars, const Rect2DAbs &rect)
{
  Rect2DAbs rectT;
  Transform(rectT, rect);
  GEngine->Draw2D(pars, rectT, _rect);
}

void Viewport2D::Draw2D(const Draw2DParsExt &pars, const Rect2DPixel &rect)
{
  Rect2DAbs rectT;
  Transform(rectT, rect);
  GEngine->Draw2D(pars, rectT, _rect);
}

void Viewport2D::Draw2D(const Draw2DParsExt &pars, const Rect2DAbs &rect, const Rect2DAbs &clip)
{
  Rect2DAbs rectT;
  Transform(rectT, rect);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  GEngine->Draw2D(pars, rectT, clipT);
}

void Viewport2D::Draw2D(const Draw2DParsExt &pars, const Rect2DPixel &rect, const Rect2DPixel &clip)
{
  Rect2DAbs rectT;
  Transform(rectT, rect);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  GEngine->Draw2D(pars, rectT, clipT);
}

void Viewport2D::Draw2DNoClip(const Draw2DParsExt &pars, const Rect2DAbs &rect)
{
  Rect2DAbs rectT;
  Transform(rectT, rect);
  GEngine->Draw2D(pars, rectT, Rect2DClipAbs);
}

void Viewport2D::Draw2DNoClip(const Draw2DParsExt &pars, const Rect2DPixel &rect)
{
  Rect2DAbs rectT;
  Transform(rectT, rect);
  GEngine->Draw2D(pars, rectT, Rect2DClipAbs);
}

void Viewport2D::DrawLine(const Line2DAbs &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectT;
  Transform(rectT, rect);
  GEngine->DrawLine(rectT, c0, c1, _rect);
}

void Viewport2D::DrawLine(const Line2DPixel &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectT;
  Transform(rectT, rect);
  GEngine->DrawLine(rectT, c0, c1, _rect);
}

void Viewport2D::DrawLine(const Line2DFloat &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectT;
  Transform(rectT, rect);
  GEngine->DrawLine(rectT, c0, c1, _rect);
}

void Viewport2D::DrawLine(const Line2DAbs &rect, PackedColor c0, PackedColor c1, const Rect2DAbs &clip, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectT;
  Transform(rectT, rect);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  GEngine->DrawLine(rectT, c0, c1, clipT);
}

void Viewport2D::DrawLine(const Line2DPixel &rect, PackedColor c0, PackedColor c1, const Rect2DPixel &clip, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectT;
  Transform(rectT, rect);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  GEngine->DrawLine(rectT, c0, c1, clipT);
}

void Viewport2D::DrawLine(const Line2DFloat &rect, PackedColor c0, PackedColor c1, const Rect2DFloat &clip, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectT;
  Transform(rectT, rect);
  Rect2DAbs clipT;
  Clip(clipT, clip);
  GEngine->DrawLine(rectT, c0, c1, clipT);
}

void Viewport2D::DrawLineNoClip(const Line2DAbs &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectT;
  Transform(rectT, rect);
  GEngine->DrawLine(rectT, c0, c1, Rect2DClipAbs);
}

void Viewport2D::DrawLineNoClip(const Line2DPixel &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectT;
  Transform(rectT, rect);
  GEngine->DrawLine(rectT, c0, c1, Rect2DClipAbs);
}

void Viewport2D::DrawLineNoClip(const Line2DFloat &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectT;
  Transform(rectT, rect);
  GEngine->DrawLine(rectT, c0, c1, Rect2DClipAbs);
}

void Viewport2D::DrawLines(const Line2DAbsInfo *lines, int nLines, float width, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Line2DAbsInfo, linesT, 16);
  linesT.Resize(nLines);
  for (int i=0; i<nLines; i++)
  {
    const Line2DAbsInfo &src = lines[i];
    Line2DAbsInfo &dst = linesT[i];
    Transform(dst.beg, src.beg);
    Transform(dst.end, src.end);
    dst.c0 = src.c0;
    dst.c1 = src.c1;
  }
  GEngine->DrawLines(linesT.Data(), linesT.Size(), _rect);
}

void Viewport2D::DrawLines(const Line2DPixelInfo *lines, int nLines, float width, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Line2DAbsInfo, linesT, 16);
  linesT.Resize(nLines);
  for (int i=0; i<nLines; i++)
  {
    const Line2DPixelInfo &src = lines[i];
    Line2DAbsInfo &dst = linesT[i];
    Transform(dst.beg, src.beg);
    Transform(dst.end, src.end);
    dst.c0 = src.c0;
    dst.c1 = src.c1;
  }
  GEngine->DrawLines(linesT.Data(), linesT.Size(), _rect);
}

void Viewport2D::DrawLines(const Line2DAbsInfo *lines, int nLines, const Rect2DAbs &clip, float width, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Line2DAbsInfo, linesT, 16);
  linesT.Resize(nLines);
  for (int i=0; i<nLines; i++)
  {
    const Line2DAbsInfo &src = lines[i];
    Line2DAbsInfo &dst = linesT[i];
    Transform(dst.beg, src.beg);
    Transform(dst.end, src.end);
    dst.c0 = src.c0;
    dst.c1 = src.c1;
  }
  Rect2DAbs clipT;
  Clip(clipT, clip);
  GEngine->DrawLines(linesT.Data(), linesT.Size(), clipT);
}

void Viewport2D::DrawLines(const Line2DPixelInfo *lines, int nLines, const Rect2DPixel &clip, float width, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Line2DAbsInfo, linesT, 16);
  linesT.Resize(nLines);
  for (int i=0; i<nLines; i++)
  {
    const Line2DPixelInfo &src = lines[i];
    Line2DAbsInfo &dst = linesT[i];
    Transform(dst.beg, src.beg);
    Transform(dst.end, src.end);
    dst.c0 = src.c0;
    dst.c1 = src.c1;
  }
  Rect2DAbs clipT; Clip(clipT, clip);
  GEngine->DrawLines(linesT.Data(), linesT.Size(), clipT);
}

void Viewport2D::DrawPoly(const MipInfo &mip, const Vertex2DAbs *vertices, int nVertices, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Vertex2DAbs, verticesT, 16);
  verticesT.Resize(nVertices);
  for (int i=0; i<nVertices; i++)
  {
    const Vertex2DAbs &src = vertices[i];
    Vertex2DAbs &dst = verticesT[i];
    Transform((Point2DAbs &)dst, (const Point2DAbs &)src);
    dst.u = src.u;
    dst.v = src.v;
    dst.w = src.w;
    dst.z = src.z;
    dst.color = src.color;
  }
  GEngine->DrawPoly(mip, verticesT.Data(), verticesT.Size(), _rect, specFlags);
}

void Viewport2D::DrawPoly(const MipInfo &mip, const Vertex2DPixel *vertices, int nVertices, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Vertex2DAbs, verticesT, 16);
  verticesT.Resize(nVertices);
  for (int i=0; i<nVertices; i++)
  {
    const Vertex2DPixel &src = vertices[i];
    Vertex2DAbs &dst = verticesT[i];
    Transform((Point2DAbs &)dst, (const Point2DPixel &)src);
    dst.u = src.u;
    dst.v = src.v;
    dst.w = src.w;
    dst.z = src.z;
    dst.color = src.color;
  }
  GEngine->DrawPoly(mip, verticesT.Data(), verticesT.Size(), _rect, specFlags);
}

void Viewport2D::DrawPoly(const MipInfo &mip, const Vertex2DAbs *vertices, int nVertices, const Rect2DAbs &clip, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Vertex2DAbs, verticesT, 16);
  verticesT.Resize(nVertices);
  for (int i=0; i<nVertices; i++)
  {
    const Vertex2DAbs &src = vertices[i];
    Vertex2DAbs &dst = verticesT[i];
    Transform((Point2DAbs &)dst, (const Point2DAbs &)src);
    dst.u = src.u;
    dst.v = src.v;
    dst.w = src.w;
    dst.z = src.z;
    dst.color = src.color;
  }
  Rect2DAbs clipT; Clip(clipT, clip);
  GEngine->DrawPoly(mip, verticesT.Data(), verticesT.Size(), clipT, specFlags);
}

void Viewport2D::DrawPoly(const MipInfo &mip, const Vertex2DPixel *vertices, int nVertices, const Rect2DPixel &clip, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Vertex2DAbs, verticesT, 16);
  verticesT.Resize(nVertices);
  for (int i=0; i<nVertices; i++)
  {
    const Vertex2DPixel &src = vertices[i];
    Vertex2DAbs &dst = verticesT[i];
    Transform((Point2DAbs &)dst, (const Point2DPixel &)src);
    dst.u = src.u;
    dst.v = src.v;
    dst.w = src.w;
    dst.z = src.z;
    dst.color = src.color;
  }
  Rect2DAbs clipT; Clip(clipT, clip);
  GEngine->DrawPoly(mip, verticesT.Data(), verticesT.Size(), clipT, specFlags);
}

UIViewport *Create2DViewport()
{
  static Viewport2D oneInvariantInstanceForEveryone;
  return &oneInvariantInstanceForEveryone;
}

void Destroy2DViewport(UIViewport *vp)
{
  // do nothing - one static instance used
}

/// 3D viewport enabling UI (2D) rendering on a 3D rectangle
class Viewport3D : public UIViewport
{
protected:
  PositionRender _space;
  /// UI viewport have usually artificial lighting, real-worlds ones may want to keep it enabled
  Rect2DFloat _clip;
  LightList _lights;
  Engine::Pars3D _pars3D;
  bool _sunEnabled;

  float _rightSize;
  float _invRightSize;
  float _downSize;
  float _invDownSize;

public:
  Viewport3D(
    int cb, Vector3Par position, Vector3Par right, Vector3Par down,
    const LightList *lightList, bool sunEnabled, bool cameraSpacePos
  );
  Viewport3D(
    int cb, Vector3Par position, Matrix3Par orientation, const Rect2DFloat &clip,
    const LightList *lightList, bool sunEnabled, bool cameraSpacePos
  );

  UIViewport *CreateChild(const Point2DFloat &offset, const Rect2DFloat &clip);
  void DestroyChild(UIViewport *vp);
  
  void GetDrawArea(Rect2DFloat &rect);

  /// get text drawing attributed, consider lighting enabled/disabled
  TextDrawAttr GetTextAttr(const TextDrawAttr &src) const;
  
  void DrawText(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DPixel &pos, const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DFloat &pos, const Rect2DFloat &clip,
    const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DPixel &pos, const Rect2DPixel &clip,
    const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DAbs &pos, const Rect2DAbs &clip,
    const TextDrawAttr &attr, const char *text);
  void DrawTextNoClip(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text);
  void DrawTextNoClip(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text);
  void DrawTextNoClip(const Point2DFloat &pos, const Rect2DFloat &clip,
    const TextDrawAttr &attr, const char *text);
  void DrawTextNoClip(const Point2DAbs &pos, const Rect2DAbs &clip,
    const TextDrawAttr &attr, const char *text);

  void DrawText(const Point2DFloat &pos, const Point2DFloat &right, const Point2DFloat &down, const TextDrawAttr &attr, const char *text);
  void DrawText(const Point2DFloat &pos, const Point2DFloat &right, const Point2DFloat &down, const Rect2DFloat &clip,
    const TextDrawAttr &attr, const char *text);

  void DrawTextVertical(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text);
  void DrawTextVertical(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text);
  void DrawTextVertical(const Point2DFloat &pos, const Rect2DFloat &clip,
    const TextDrawAttr &attr, const char *text);
  void DrawTextVertical(const Point2DAbs &pos, const Rect2DAbs &clip,
    const TextDrawAttr &attr, const char *text);

  float GetTextWidth(const TextDrawAttr &attr, const char *text);

  void DrawFrame(Texture *corner, PackedColor color, const Rect2DPixel &frame);
  void DrawFrame(Texture *corner, PackedColor color, const Rect2DAbs &frame);
  void DrawFrame(Texture *corner, PackedColor color, const Rect2DPixel &frame, const Rect2DPixel &clip);
  void DrawFrame(Texture *corner, PackedColor color, const Rect2DAbs &frame, const Rect2DAbs &clip);

  void Draw2D(const Draw2DParsExt &pars, const Rect2DAbs &rect);
  void Draw2D(const Draw2DParsExt &pars, const Rect2DPixel &rect);
  void Draw2D(const Draw2DParsExt &pars, const Rect2DAbs &rect, const Rect2DAbs &clip);
  void Draw2D(const Draw2DParsExt &pars, const Rect2DPixel &rect, const Rect2DPixel &clip);
  void Draw2DNoClip(const Draw2DParsExt &pars, const Rect2DAbs &rect);
  void Draw2DNoClip(const Draw2DParsExt &pars, const Rect2DPixel &rect);

  void DrawLine(const Line2DAbs &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat);
  void DrawLine(const Line2DPixel &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat);
  void DrawLine(const Line2DFloat &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat);
  void DrawLine(const Line2DAbs &rect, PackedColor c0, PackedColor c1, const Rect2DAbs &clip, float width, int specFlags, const TLMaterial &mat);
  void DrawLine(const Line2DPixel &rect, PackedColor c0, PackedColor c1, const Rect2DPixel &clip, float width, int specFlags, const TLMaterial &mat);
  void DrawLine(const Line2DFloat &rect, PackedColor c0, PackedColor c1, const Rect2DFloat &clip, float width, int specFlags, const TLMaterial &mat);
  void DrawLineNoClip(const Line2DAbs &rect, PackedColor c0, PackedColor c1, float width = 1.0f, int specFlags = 0, const TLMaterial &mat=TLMaterial::_default);
  void DrawLineNoClip(const Line2DPixel &rect, PackedColor c0, PackedColor c1, float width = 1.0f, int specFlags = 0, const TLMaterial &mat=TLMaterial::_default);
  void DrawLineNoClip(const Line2DFloat &rect, PackedColor c0, PackedColor c1, float width = 1.0f, int specFlags = 0, const TLMaterial &mat=TLMaterial::_default);

  void DrawLines(const Line2DAbsInfo *lines, int nLines, float width, int specFlags, const TLMaterial &mat);
  void DrawLines(const Line2DPixelInfo *lines, int nLines, float width, int specFlags, const TLMaterial &mat);
  void DrawLines(const Line2DAbsInfo *lines, int nLines, const Rect2DAbs &clip, float width, int specFlags, const TLMaterial &mat);
  void DrawLines(const Line2DPixelInfo *lines, int nLines, const Rect2DPixel &clip, float width, int specFlags, const TLMaterial &mat);

  void DrawPoly(
    const MipInfo &mip, const Vertex2DAbs *vertices, int nVertices,
    int specFlags, const TLMaterial &mat);
  void DrawPoly(
    const MipInfo &mip, const Vertex2DPixel *vertices, int nVertices,
    int specFlags, const TLMaterial &mat);
  void DrawPoly(
    const MipInfo &mip, const Vertex2DAbs *vertices, int nVertices,
    const Rect2DAbs &clip, int specFlags, const TLMaterial &mat);
  void DrawPoly(
    const MipInfo &mip, const Vertex2DPixel *vertices, int nVertices,
    const Rect2DPixel &clip, int specFlags, const TLMaterial &mat);
#if _HUD_DRAW_POLYGON
  void DrawPoly(
    const MipInfo &mip, const Vertex2DFloat *vertices, int nVertices, 
    int specFlags, const TLMaterial &mat);
#endif
protected:
  void Transform(
    Vector3 &position, Vector3 &right, Vector3 &down,
    float &x1c, float &y1c, float &x2c, float &y2c,
    float x, float y, float w, float h, const Rect2DFloat &clip);
};

Viewport3D::Viewport3D(
  int cb, Vector3Par position, Vector3Par right, Vector3Par down,
  const LightList *lightList, bool sunEnabled, bool cameraSpacePos
)
: _pars3D(cb), _clip(0, 0, 1, 1),_sunEnabled(sunEnabled)
{
  float coef = 0.75f;

  _rightSize = right.Size();
  _invRightSize = 1.0f / _rightSize;

  Vector3 dir = coef * _rightSize * down.Normalized();

  _downSize = dir.Size();
  _invDownSize = 1.0f / _downSize;

  Vector3 normal = dir.CrossProduct(right).Normalized();
  _space.position.SetDirection(dir);
  _space.position.SetDirectionAside(right);
  _space.position.SetDirectionUp(normal);
  _space.position.SetPosition(position - 0.002 * normal);
  _space.camSpace = cameraSpacePos;

  _clip.h = down.Size() * _invDownSize;
  if (lightList)
  {
    _lights = *lightList;
  }
  _pars3D._lights = &_lights;
}

Viewport3D::Viewport3D(
  int cb, Vector3Par position, Matrix3Par orientation, const Rect2DFloat &clip,
  const LightList *lightList, bool sunEnabled, bool cameraSpacePos
)
:  _pars3D(cb), _clip(clip),_sunEnabled(sunEnabled)
{
  _space.position.SetPosition(position);
  _space.position.SetOrientation(orientation);
  _space.camSpace = cameraSpacePos;
  _rightSize = orientation.DirectionAside().Size();
  _invRightSize = 1.0f / _rightSize;
  _downSize = orientation.Direction().Size();
  _invDownSize = 1.0f / _downSize;
  if (lightList)
  {
    _lights = *lightList;
  }
  _pars3D._lights = &_lights;
}

UIViewport *Viewport3D::CreateChild(const Point2DFloat &offset, const Rect2DFloat &clip)
{
  Vector3 position = _space.position.Position() + offset.x * _space.position.DirectionAside() + offset.y * _space.position.Direction();

  Rect2DFloat clipT;
  clipT.x = floatMax(clip.x, _clip.x);
  clipT.y = floatMax(clip.y, _clip.y);
  clipT.w = floatMin(clip.x + clip.w, _clip.x + _clip.w) - clipT.x;
  clipT.h = floatMin(clip.y + clip.h, _clip.y + _clip.h) - clipT.y;
  clipT.x -= offset.x;
  clipT.y -= offset.y;

  return new Viewport3D(_pars3D._cb, position, _space.position.Orientation(), clipT, &_lights, _sunEnabled, _space.camSpace);
}

void Viewport3D::DestroyChild(UIViewport *vp)
{
  delete vp;
}

void Viewport3D::GetDrawArea(Rect2DFloat &rect)
{
  rect = _clip;
}

TextDrawAttr Viewport3D::GetTextAttr(const TextDrawAttr &src) const
{
  TextDrawAttr ret = src;
  if (!_sunEnabled) ret._specFlags |= DisableSun;
  return ret;
}


void Viewport3D::Transform(
  Vector3 &position, Vector3 &right, Vector3 &down,
  float &x1c, float &y1c, float &x2c, float &y2c,
  float x, float y, float w, float h, const Rect2DFloat &clip)
{
  position = _space.position.Position() + x * _space.position.DirectionAside() + y * _space.position.Direction();
  right = w * _space.position.DirectionAside();
  down = h * _space.position.Direction();

  // transform clipping rectangle to new space
  float invW = 1.0f / w;
  float invH = 1.0f / h;
  x1c = (clip.x - x) * invW;
  y1c = (clip.y - y) * invH;
  x2c = (clip.x + clip.w - x) * invW;
  y2c = (clip.y + clip.h - y) * invH;
}

void Viewport3D::DrawText(const Point2DFloat &posF, const TextDrawAttr &attr, const char *text)
{
  GEngine->DrawText3D(_space, posF, GetTextAttr(attr), text, _clip, &_pars3D);
}

void Viewport3D::DrawText(const Point2DPixel &pos, const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posA; GEngine->Convert(posA, pos);
  Point2DFloat posF; GEngine->Convert(posF, posA);
  GEngine->DrawText3D(_space, posF, GetTextAttr(attr), text, _clip, &_pars3D);
}

void Viewport3D::DrawText(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text)
{
  Point2DFloat posF; GEngine->Convert(posF, pos);
  GEngine->DrawText3D(_space, posF, GetTextAttr(attr), text, _clip, &_pars3D);
}

void Viewport3D::DrawText(const Point2DFloat &posF, const Rect2DFloat &clipF,
  const TextDrawAttr &attr, const char *text)
{
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawText3D(_space, posF, GetTextAttr(attr), text, clipT, &_pars3D);
}

void Viewport3D::DrawText(const Point2DPixel &pos, const Rect2DPixel &clip,
                          const TextDrawAttr &attr, const char *text)
{
  Point2DAbs posA; GEngine->Convert(posA, pos);
  Point2DFloat posF; GEngine->Convert(posF, posA);
  Rect2DAbs clipA; GEngine->Convert(clipA, clip);
  Rect2DFloat clipF; GEngine->Convert(clipF, clipA);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawText3D(_space, posF, GetTextAttr(attr), text, clipT, &_pars3D);
}

void Viewport3D::DrawText(const Point2DAbs &pos, const Rect2DAbs &clip,
  const TextDrawAttr &attr, const char *text)
{
  Point2DFloat posF; GEngine->Convert(posF, pos);
  Rect2DFloat clipF; GEngine->Convert(clipF, clip);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawText3D(_space, posF, GetTextAttr(attr), text, clipT, &_pars3D);
}

void Viewport3D::DrawTextNoClip(const Point2DFloat &posF, const TextDrawAttr &attr, const char *text)
{
  GEngine->DrawText3D(_space, posF, GetTextAttr(attr), text, Rect2DClipFloat, &_pars3D);
}

void Viewport3D::DrawTextNoClip(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text)
{
  Point2DFloat posF; GEngine->Convert(posF, pos);
  GEngine->DrawText3D(_space, posF, GetTextAttr(attr), text, Rect2DClipFloat, &_pars3D);
}

void Viewport3D::DrawTextNoClip(const Point2DFloat &posF, const Rect2DFloat &clipF,
                          const TextDrawAttr &attr, const char *text)
{
  GEngine->DrawText3D(_space, posF, GetTextAttr(attr), text, clipF, &_pars3D);
}

void Viewport3D::DrawTextNoClip(const Point2DAbs &pos, const Rect2DAbs &clip,
                          const TextDrawAttr &attr, const char *text)
{
  Point2DFloat posF; GEngine->Convert(posF, pos);
  Rect2DFloat clipF; GEngine->Convert(clipF, clip);
  GEngine->DrawText3D(_space, posF, GetTextAttr(attr), text, clipF, &_pars3D);
}

void Viewport3D::DrawText(const Point2DFloat &posF, const Point2DFloat &rightF, const Point2DFloat &downF, const TextDrawAttr &attr, const char *text)
{
  GEngine->DrawText3D(_space, posF, rightF, downF, GetTextAttr(attr), text, _clip, &_pars3D);
}

void Viewport3D::DrawText(const Point2DFloat &posF, const Point2DFloat &rightF, const Point2DFloat &downF, const Rect2DFloat &clipF,
  const TextDrawAttr &attr, const char *text)
{
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawText3D(_space, posF, rightF, downF, GetTextAttr(attr), text, clipT, &_pars3D);
}

void Viewport3D::DrawTextVertical(const Point2DFloat &pos, const TextDrawAttr &attr, const char *text)
{
}

void Viewport3D::DrawTextVertical(const Point2DAbs &pos, const TextDrawAttr &attr, const char *text)
{
}

void Viewport3D::DrawTextVertical(const Point2DFloat &pos, const Rect2DFloat &clip,
  const TextDrawAttr &attr, const char *text)
{
}

void Viewport3D::DrawTextVertical(const Point2DAbs &pos, const Rect2DAbs &clip,
  const TextDrawAttr &attr, const char *text)
{
}

float Viewport3D::GetTextWidth(const TextDrawAttr &attr, const char *text)
{
  return GEngine->GetTextWidth(attr, text);
/*
  // check it
  float aspect = _downSize * _invRightSize;
  Vector3 right = (aspect * attr._size) * _right;
  Vector3 width = GEngine->GetText3DWidth(right, attr._font, text);

LogF("*** Size3D %.3f, Size2D %.3f", width.Size() * _invRightSize, GEngine->GetTextWidth(attr, text));
  return width.Size() * _invRightSize;
*/
}

void Viewport3D::DrawFrame(Texture *corner, PackedColor color, const Rect2DPixel &frame)
{
  Rect2DAbs frameA; GEngine->Convert(frameA, frame);
  Rect2DFloat frameF; GEngine->Convert(frameF, frameA);
  GEngine->DrawFrame3D(_space, corner, color, frameF, _clip);
}

void Viewport3D::DrawFrame(Texture *corner, PackedColor color, const Rect2DAbs &frame)
{
  Rect2DFloat frameF; GEngine->Convert(frameF, frame);
  GEngine->DrawFrame3D(_space, corner, color, frameF, _clip);
}

void Viewport3D::DrawFrame(Texture *corner, PackedColor color, const Rect2DPixel &frame, const Rect2DPixel &clip)
{
  Rect2DAbs frameA; GEngine->Convert(frameA, frame);
  Rect2DFloat frameF; GEngine->Convert(frameF, frameA);
  Rect2DAbs clipA; GEngine->Convert(clipA, clip);
  Rect2DFloat clipF; GEngine->Convert(clipF, clipA);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawFrame3D(_space, corner, color, frameF, clipT);
}

void Viewport3D::DrawFrame(Texture *corner, PackedColor color, const Rect2DAbs &frame, const Rect2DAbs &clip)
{
  Rect2DFloat frameF; GEngine->Convert(frameF, frame);
  Rect2DFloat clipF; GEngine->Convert(clipF, clip);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawFrame3D(_space, corner, color, frameF, clipT);
}

void Viewport3D::Draw2D(const Draw2DParsExt &pars, const Rect2DAbs &rect)
{
  Rect2DFloat rectF; GEngine->Convert(rectF, rect);
  GEngine->Draw3D(_space, pars, rectF, _clip);
}

void Viewport3D::Draw2D(const Draw2DParsExt &pars, const Rect2DPixel &rect)
{
  Rect2DAbs rectA; GEngine->Convert(rectA, rect);
  Rect2DFloat rectF; GEngine->Convert(rectF, rectA);
  GEngine->Draw3D(_space, pars, rectF, _clip);
}

void Viewport3D::Draw2D(const Draw2DParsExt &pars, const Rect2DAbs &rect, const Rect2DAbs &clip)
{
  Rect2DFloat rectF; GEngine->Convert(rectF, rect);
  Rect2DFloat clipF; GEngine->Convert(clipF, clip);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->Draw3D(_space, pars, rectF, clipT);
}

/*!
  \patch 5105 Date 12/19/2006 by Ondra
  - Fixed: Briefing picture lighting was incorrect.
*/

void Viewport3D::Draw2D(const Draw2DParsExt &pars, const Rect2DPixel &rect, const Rect2DPixel &clip)
{
  Draw2DParsExt vpPars = pars;
  if (!_sunEnabled) vpPars.spec |= DisableSun;
  Rect2DAbs rectA; GEngine->Convert(rectA, rect);
  Rect2DFloat rectF; GEngine->Convert(rectF, rectA);
  Rect2DAbs clipA; GEngine->Convert(clipA, clip);
  Rect2DFloat clipF; GEngine->Convert(clipF, clipA);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->Draw3D(_space, vpPars, rectF, clipT, &_pars3D);
}

void Viewport3D::Draw2DNoClip(const Draw2DParsExt &pars, const Rect2DAbs &rect)
{
  Rect2DFloat rectF; GEngine->Convert(rectF, rect);
  GEngine->Draw3D(_space, pars, rectF, Rect2DClipFloat);
}

void Viewport3D::Draw2DNoClip(const Draw2DParsExt &pars, const Rect2DPixel &rect)
{
  Rect2DAbs rectA; GEngine->Convert(rectA, rect);
  Rect2DFloat rectF; GEngine->Convert(rectF, rectA);
  GEngine->Draw3D(_space, pars, rectF, Rect2DClipFloat);
}

void Viewport3D::DrawLine(const Line2DAbs &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  Line2DFloat rectF; GEngine->Convert(rectF, rect);
  GEngine->DrawLine3D(_space, rectF, c0, c1, _clip, width, specFlags, mat);
}

void Viewport3D::DrawLine(const Line2DPixel &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectA; GEngine->Convert(rectA, rect);
  Line2DFloat rectF; GEngine->Convert(rectF, rectA);
  GEngine->DrawLine3D(_space, rectF, c0, c1, _clip, width, specFlags, mat, &_pars3D);
}

void Viewport3D::DrawLine(const Line2DFloat &rectF, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  GEngine->DrawLine3D(_space, rectF, c0, c1, _clip, width, specFlags, mat, &_pars3D);
}

void Viewport3D::DrawLine(const Line2DAbs &rect, PackedColor c0, PackedColor c1, const Rect2DAbs &clip, float width, int specFlags, const TLMaterial &mat)
{
  Line2DFloat rectF; GEngine->Convert(rectF, rect);
  Rect2DFloat clipF; GEngine->Convert(clipF, clip);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawLine3D(_space, rectF, c0, c1, clipT, width, specFlags, mat, &_pars3D);
}

void Viewport3D::DrawLine(const Line2DPixel &rect, PackedColor c0, PackedColor c1, const Rect2DPixel &clip, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectA; GEngine->Convert(rectA, rect);
  Line2DFloat rectF; GEngine->Convert(rectF, rectA);
  Rect2DAbs clipA; GEngine->Convert(clipA, clip);
  Rect2DFloat clipF; GEngine->Convert(clipF, clipA);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawLine3D(_space, rectF, c0, c1, clipT, width, specFlags, mat, &_pars3D);
}

void Viewport3D::DrawLine(const Line2DFloat &rectF, PackedColor c0, PackedColor c1, const Rect2DFloat &clipF, float width, int specFlags, const TLMaterial &mat)
{
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawLine3D(_space, rectF, c0, c1, clipT, width, specFlags, mat, &_pars3D);
}

void Viewport3D::DrawLineNoClip(const Line2DAbs &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  Line2DFloat rectF; GEngine->Convert(rectF, rect);
  GEngine->DrawLine3D(_space, rectF, c0, c1, Rect2DClipFloat, width, specFlags, mat, &_pars3D);
}

void Viewport3D::DrawLineNoClip(const Line2DPixel &rect, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  Line2DAbs rectA; GEngine->Convert(rectA, rect);
  Line2DFloat rectF; GEngine->Convert(rectF, rectA);
  GEngine->DrawLine3D(_space, rectF, c0, c1, Rect2DClipFloat, width, specFlags, mat, &_pars3D);
}

void Viewport3D::DrawLineNoClip(const Line2DFloat &rectF, PackedColor c0, PackedColor c1, float width, int specFlags, const TLMaterial &mat)
{
  GEngine->DrawLine3D(_space, rectF, c0, c1, Rect2DClipFloat, width, specFlags, mat, &_pars3D);
}

void Viewport3D::DrawLines(const Line2DAbsInfo *lines, int nLines, float width, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Line2DFloatInfo, linesF, 16);
  linesF.Resize(nLines);
  for (int i=0; i<nLines; i++)
  {
    GEngine->Convert(linesF[i], lines[i]);
    linesF[i].c0 = lines[i].c0;
    linesF[i].c1 = lines[i].c1;
  }
  GEngine->DrawLines3D(_space, linesF.Data(), nLines, _clip, width, specFlags, mat, &_pars3D);
}

void Viewport3D::DrawLines(const Line2DPixelInfo *lines, int nLines, float width, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Line2DFloatInfo, linesF, 16);
  linesF.Resize(nLines);
  for (int i=0; i<nLines; i++)
  {
    Line2DAbs line;
    GEngine->Convert(line, lines[i]);
    GEngine->Convert(linesF[i], line);
    linesF[i].c0 = lines[i].c0;
    linesF[i].c1 = lines[i].c1;
  }
  GEngine->DrawLines3D(_space, linesF.Data(), nLines, _clip, width, specFlags, mat, &_pars3D);
}

void Viewport3D::DrawLines(const Line2DAbsInfo *lines, int nLines, const Rect2DAbs &clip, float width, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Line2DFloatInfo, linesF, 16);
  linesF.Resize(nLines);
  for (int i=0; i<nLines; i++)
  {
    GEngine->Convert(linesF[i], lines[i]);
    linesF[i].c0 = lines[i].c0;
    linesF[i].c1 = lines[i].c1;
  }
  Rect2DFloat clipF; GEngine->Convert(clipF, clip);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawLines3D(_space, linesF.Data(), nLines, clipT, width, specFlags, mat);
}

void Viewport3D::DrawLines(const Line2DPixelInfo *lines, int nLines, const Rect2DPixel &clip, float width, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Line2DFloatInfo, linesF, 16);
  linesF.Resize(nLines);
  for (int i=0; i<nLines; i++)
  {
    Line2DAbs line;
    GEngine->Convert(line, lines[i]);
    GEngine->Convert(linesF[i], line);
    linesF[i].c0 = lines[i].c0;
    linesF[i].c1 = lines[i].c1;
  }
  Rect2DAbs clipA; GEngine->Convert(clipA, clip);
  Rect2DFloat clipF; GEngine->Convert(clipF, clipA);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawLines3D(_space, linesF.Data(), nLines, clipT, width, specFlags, mat);
}

void Viewport3D::DrawPoly(
  const MipInfo &mip, const Vertex2DAbs *vertices, int nVertices, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Vertex2DFloat, verticesF, 16);
  verticesF.Resize(nVertices);
  for (int i=0; i<nVertices; i++)
  {
    GEngine->Convert(verticesF[i], vertices[i]);
    verticesF[i].z = vertices[i].z;
    verticesF[i].w = vertices[i].w;
    verticesF[i].u = vertices[i].u;
    verticesF[i].v = vertices[i].v;
    verticesF[i].color = vertices[i].color;
  }
  GEngine->DrawPoly3D(_space, mip, verticesF.Data(), nVertices, _clip, specFlags, mat);
}
#if _HUD_DRAW_POLYGON
void Viewport3D::DrawPoly(
  const MipInfo &mip, const Vertex2DFloat *vertices, int nVertices, int specFlags, const TLMaterial &mat)
{
  specFlags |= NoZWrite|IsAlpha|ClampU|ClampV|IsAlphaFog;

  GEngine->DrawPoly3D(_space, mip, vertices, nVertices, _clip, specFlags, mat, &_pars3D);
}
#endif
void Viewport3D::DrawPoly(
  const MipInfo &mip, const Vertex2DPixel *vertices, int nVertices, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Vertex2DFloat, verticesF, 16);
  verticesF.Resize(nVertices);
  for (int i=0; i<nVertices; i++)
  {
    Point2DAbs point;
    GEngine->Convert(point, vertices[i]);
    GEngine->Convert(verticesF[i], point);
    verticesF[i].z = vertices[i].z;
    verticesF[i].w = vertices[i].w;
    verticesF[i].u = vertices[i].u;
    verticesF[i].v = vertices[i].v;
    verticesF[i].color = vertices[i].color;
  }
  GEngine->DrawPoly3D(_space, mip, verticesF.Data(), nVertices, _clip, specFlags, mat);
}

void Viewport3D::DrawPoly(
  const MipInfo &mip, const Vertex2DAbs *vertices, int nVertices, const Rect2DAbs &clip, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Vertex2DFloat, verticesF, 16);
  verticesF.Resize(nVertices);
  for (int i=0; i<nVertices; i++)
  {
    GEngine->Convert(verticesF[i], vertices[i]);
    verticesF[i].z = vertices[i].z;
    verticesF[i].w = vertices[i].w;
    verticesF[i].u = vertices[i].u;
    verticesF[i].v = vertices[i].v;
    verticesF[i].color = vertices[i].color;
  }
  Rect2DFloat clipF; GEngine->Convert(clipF, clip);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawPoly3D(_space, mip, verticesF.Data(), nVertices, clipT, specFlags, mat);
}

void Viewport3D::DrawPoly(
  const MipInfo &mip, const Vertex2DPixel *vertices, int nVertices, const Rect2DPixel &clip, int specFlags, const TLMaterial &mat)
{
  AUTO_STATIC_ARRAY(Vertex2DFloat, verticesF, 16);
  verticesF.Resize(nVertices);
  for (int i=0; i<nVertices; i++)
  {
    Point2DAbs point;
    GEngine->Convert(point, vertices[i]);
    GEngine->Convert(verticesF[i], point);
    verticesF[i].z = vertices[i].z;
    verticesF[i].w = vertices[i].w;
    verticesF[i].u = vertices[i].u;
    verticesF[i].v = vertices[i].v;
    verticesF[i].color = vertices[i].color;
  }
  Rect2DAbs clipA; GEngine->Convert(clipA, clip);
  Rect2DFloat clipF; GEngine->Convert(clipF, clipA);
  Rect2DFloat clipT; Intersect(clipT, _clip, clipF);
  GEngine->DrawPoly3D(_space, mip, verticesF.Data(), nVertices, clipT, specFlags, mat);
}

UIViewport *Create3DViewport(
  int cb, Vector3Par pos, Vector3Par right, Vector3Par down, const LightList *lightList, bool sunEnabled, bool cameraSpacePos
)
{
  return new Viewport3D(cb,pos, right, down, lightList, sunEnabled, cameraSpacePos);
}

void Destroy3DViewport(UIViewport *vp)
{
  // do nothing - one static instance used
  delete vp;
}

