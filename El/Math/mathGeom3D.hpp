#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MATH_GEOM_3D_HPP
#define _MATH_GEOM_3D_HPP

#include "math3d.hpp"

/// point on line beg .. end nearest to pos, t gives limits (0,1 represent beg, end endpoints)
static inline float NearestPointDistance( Vector3Par beg, Vector3Par end, Vector3Par pos, float minT, float maxT )
{
  Vector3Val eb = end - beg;
  Vector3Val pb = pos - beg;
  float t = (eb*pb) / eb.SquareSize();
  saturate(t,minT,maxT);
  Vector3 nearest = beg+eb*t;
  return nearest.Distance(pos);

}

/// point on line beg .. end nearest to pos, line without any limits

static inline float NearestPointT( Vector3Par beg, Vector3Par end, Vector3Par pos )
{
  // point on line beg .. end nearest to pos
  Vector3Val eb = end - beg;
  Vector3Val pb = pos - beg;
  float denom2 = eb.SquareSize();
  if (denom2<=0) return 0;
  return (eb*pb) / denom2;
}

static inline Vector3 NearestPoint( Vector3Par beg, Vector3Par end, Vector3Par pos )
{
  // point on line beg .. end nearest to pos
  Vector3Val eb = end - beg;
  Vector3Val pb = pos - beg;
  float denom2 = eb.SquareSize();
  if (denom2<=0) return beg;
  float t = (eb*pb) / denom2;
  saturate(t,0,1);
  return beg+eb*t;
}

static inline Vector3 NearestPointInfinite( Vector3Par beg, Vector3Par end, Vector3Par pos )
{
  // point on line beg .. end nearest to pos
  Vector3Val eb = end - beg;
  Vector3Val pb = pos - beg;
  float denom2 = eb.SquareSize();
  // when line is ill-defined, return anything
  if (denom2<=0) return beg;
  float t = (eb*pb) / denom2;
  return beg+eb*t;
}

/// find t where two moving object reach their nearest point
static inline float NearestPoint(Vector3Val b1, Vector3Val d1, Vector3Val b2, Vector3Val d2)
{
  Vector3 b=b1-b2;
  Vector3 d=d1-d2;
  // search for t so that |b+d*t| is minimal
  // min ( b + d*t )*( b + d*t )
  // (.)
  // sure: this functions has one local minimum, no local maximum
  // the point with derivation = 0 is our solution
  // f(x)=( b + d*t )*( b + d*t )
  // f(x)=( b + d*t )*b + (b + d*t)*d*t
  // f(x)=( b*b + b*d*t ) + (b*d*t + d*t*d*t)
  // f(x)=( b*b + 2*b*d*t + d*d*t*t)
  // f'(x)=2*(b*d) + 2*(d*d)*t
  // (.)
  // 2*(b*d) + 2*(d*d)*t =0
  // t = -(b*d)/(d*d)
  float denom=d*d;
  if( denom>1e-5 ) return -(b*d)/denom;
  return 0;
}

static inline float NearestPointDistance( Vector3Par beg, Vector3Par end, Vector3Par pos )
{
  Vector3Val nearest = NearestPoint(beg,end,pos);
  return nearest.Distance(pos);
}

static inline float NearestPointInfiniteDistance( Vector3Par beg, Vector3Par end, Vector3Par pos )
{
  Vector3Val nearest = NearestPointInfinite(beg,end,pos);
  return nearest.Distance(pos);
}

/// interpolation based on source values, saturated by limits
float Interpolativ( float control, float cMin, float cMax, float vMin, float vMax );

/// Returns x + s(y - x) 
/**
This linearly interpolates between x and y, such that the return value is x when s is 0, and y when s is 1.
*/
static inline float Lerp(float x, float y, float s)
{
  return x + s*(y - x); 
}

float AngleDifference( float a, float b );

/// check line against box intersection
bool IsIntersectionBoxWithLine(Vector3Val boxMin, Vector3Val boxMax, Vector3Val lineBeg, Vector3Val lineEnd);
inline bool IsIntersectionSphereAABB(Vector3Par origin, double radiusSq, Vector3Par mins, Vector3Par maxs)
{
  double d = 0; 
  //find the square of the distance
  //from the sphere to the box

  for( int i = 0; i < 3; i++ ) 
  { 
    if( origin[i] < mins[i] )
    {
      double s = origin[i] - mins[i];
      d += s * s; 
    }
    else if( origin[i] > maxs[i] )
    {
      double s = origin[i] - maxs[i];
      d += s * s; 
    }
  }
  return d <= radiusSq;
}

inline bool IsIntersectionAABBAABB(Vector3Par mins1, Vector3Par maxs1, Vector3Par mins2, Vector3Par maxs2)
{
  return !((mins1[0] > maxs2[0]) ||
    (mins1[1] > maxs2[1]) ||
    (mins1[2] > maxs2[2]) ||
    (maxs1[0] < mins2[0]) ||
    (maxs1[1] < mins2[1]) ||
    (maxs1[2] < mins2[2]));
}

inline bool IsIntersectionAABBPoint(Vector3Par mins, Vector3Par maxs, Vector3Par point)
{
  return !(mins[0] > point[0] ||
    mins[1] > point[1] ||
    mins[2] > point[2] ||
    maxs[0] < point[0] ||
    maxs[1] < point[1] ||
    maxs[2] < point[2]);
}

#endif
