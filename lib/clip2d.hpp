#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CLIP2D_HPP
#define _CLIP2D_HPP

inline float InsideLeftPixel( const Vertex2DPixel &v, const Rect2DPixel &rect )
{
	return v.x-rect.x;
}
inline float InsideRightPixel( const Vertex2DPixel &v, const Rect2DPixel &rect )
{
	return rect.x+rect.w-v.x;
}
inline float InsideTopPixel( const Vertex2DPixel &v, const Rect2DPixel &rect )
{
	return v.y-rect.y;
}
inline float InsideBottomPixel( const Vertex2DPixel &v, const Rect2DPixel &rect )
{
	return rect.y+rect.h-v.y;
}

inline float InsideLeftAbs( const Vertex2DAbs &v, const Rect2DAbs &rect )
{
	return v.x-rect.x;
}
inline float InsideRightAbs( const Vertex2DAbs &v, const Rect2DAbs &rect )
{
	return rect.x+rect.w-v.x;
}
inline float InsideTopAbs( const Vertex2DAbs &v, const Rect2DAbs &rect )
{
	return v.y-rect.y;
}
inline float InsideBottomAbs( const Vertex2DAbs &v, const Rect2DAbs &rect )
{
	return rect.y+rect.h-v.y;
}

inline float InsideLeftFloat(const Vertex2DFloat &v, const Rect2DFloat &rect)
{
  return v.x-rect.x;
}
inline float InsideRightFloat(const Vertex2DFloat &v, const Rect2DFloat &rect)
{
  return rect.x+rect.w-v.x;
}
inline float InsideTopFloat(const Vertex2DFloat &v, const Rect2DFloat &rect)
{
  return v.y-rect.y;
}
inline float InsideBottomFloat(const Vertex2DFloat &v, const Rect2DFloat &rect)
{
  return rect.y+rect.h-v.y;
}

//////////////////////////////////////////////////////////////////////////

inline void InterpolateVertexPixel(Vertex2DPixel &d, const Vertex2DPixel &a, float aFactor, const Vertex2DPixel &b, float bFactor)
{
  d.x = a.x * aFactor + b.x * bFactor;
  d.y = a.y * aFactor + b.y * bFactor;
  d.z = a.z * aFactor + b.z * bFactor;
  d.w = a.w * aFactor + b.w * bFactor;
  d.u = a.u * aFactor + b.u * bFactor;
  d.v = a.v * aFactor + b.v * bFactor;
  d.color = PackedColor(Color(a.color) * aFactor + Color(b.color) * bFactor);
}

inline void InterpolateVertexAbs(Vertex2DAbs &d, const Vertex2DAbs &a, float aFactor, const Vertex2DAbs &b, float bFactor)
{
  d.x = a.x * aFactor + b.x * bFactor;
  d.y = a.y * aFactor + b.y * bFactor;
  d.z = a.z * aFactor + b.z * bFactor;
  d.w = a.w * aFactor + b.w * bFactor;
  d.u = a.u * aFactor + b.u * bFactor;
  d.v = a.v * aFactor + b.v * bFactor;
  d.color = PackedColor(Color(a.color) * aFactor + Color(b.color) * bFactor);
}

inline void InterpolateVertexFloat(Vertex2DFloat &d, const Vertex2DFloat &a, float aFactor, const Vertex2DFloat &b, float bFactor)
{
  d.x = a.x * aFactor + b.x * bFactor;
  d.y = a.y * aFactor + b.y * bFactor;
  d.z = a.z * aFactor + b.z * bFactor;
  d.w = a.w * aFactor + b.w * bFactor;
  d.u = a.u * aFactor + b.u * bFactor;
  d.v = a.v * aFactor + b.v * bFactor;
  d.color = PackedColor(Color(a.color) * aFactor + Color(b.color) * bFactor);
}

template <class Rect, class Vertex, class InsideF, class InterpolateF>
int Clip2D
(
 const Rect &rect,
 Vertex *dest, const Vertex *vertices, int n, InsideF inside, InterpolateF interpolate
 )
{
  int dn=0;
  const Vertex *p=vertices+n-1;
  float pIn=inside(*p,rect);
  for( int i=0; i<n; i++ )
  {
    const Vertex *a=vertices+i;
    // TODO: real clipping
    float aIn=inside(*a,rect);
    if( (aIn<0)!=(pIn<0) )
    {
      // only one is out
      // calculate edge intersection
      //float avg=aIn
      float aa=fabs(aIn);
      float pp=fabs(pIn);
      float pFactor=aa/(aa+pp);
      float aFactor=1-pFactor;
      Vertex &d=dest[dn++];
      interpolate(d, *a, aFactor, *p, pFactor);
    }
    if( aIn>=0 )
    {
      dest[dn++]=*a;
    }

    p=a;
    pIn=aIn;
  }
  return dn;
}

#endif