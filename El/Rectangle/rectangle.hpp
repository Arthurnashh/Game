// preproc.h: interface for the Preproc class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_RECTANGLE_HPP
#define _EL_RECTANGLE_HPP

#include <El/elementpch.hpp>
#include <Es/Common/fltopts.hpp>

/// GridRectangle arithmetics
struct GridRectangle
{
  // beg inclusive
  int xBeg,zBeg;
  // end exclusive
  int xEnd,zEnd;
  
  GridRectangle(){}
  
  GridRectangle(int xb, int zb, int xe, int ze):
  xBeg(xb),zBeg(zb),xEnd(xe),zEnd(ze)
  {
  }
  
  GridRectangle operator << (int shift) const {return GridRectangle(xBeg<<shift,zBeg<<shift,xEnd<<shift,zEnd<<shift);}
  GridRectangle operator >> (int shift) const {return GridRectangle(xBeg>>shift,zBeg>>shift,xEnd>>shift,zEnd>>shift);}
  
  /// rectangle intersection - result is always a rectangle
  GridRectangle operator & (const GridRectangle &with) const
  {
    return GridRectangle(
      intMax(xBeg,with.xBeg), intMax(zBeg,with.zBeg),
      intMin(xEnd,with.xEnd), intMin(zEnd,with.zEnd)
    );
  }
  /// find a rectangle containing both rectangles
  GridRectangle operator + (const GridRectangle &with) const
  {
    return GridRectangle(
      intMin(xBeg,with.xBeg), intMin(zBeg,with.zBeg),
      intMax(xEnd,with.xEnd), intMax(zEnd,with.zEnd)
    );
  }
  /// check if one rectangle is inside of the other
  bool IsInside(const GridRectangle &rect) const
  {
    return xBeg>=rect.xBeg && xEnd<=rect.xEnd && zBeg>=rect.zBeg && zEnd<=rect.zEnd;
  }
  /// check if one rectangle is outside of the other
  bool IsOutside(const GridRectangle &rect) const
  {
    return xBeg>=rect.xEnd || xEnd<=rect.xBeg || zBeg>=rect.zEnd || zEnd<=rect.zBeg;
  }
  /// compare two rectangles
  /** note: two empty rectangles may differ */
  bool operator == (const GridRectangle &with) const
  {
    // we want to reduce the number of ifs as much as possible
    // as branching is usually slow on modern CPUs
    return ((xBeg-with.xBeg)|(zBeg-with.zBeg)|(xEnd-with.xEnd)|(zEnd-with.zEnd))==0;
    //return xBeg==with.xBeg && zBeg==with.zBeg && xEnd==with.xEnd && zEnd==with.zEnd;
  }
  __forceinline bool operator != (const GridRectangle &with) const
  {
    return !operator ==(with);
  }
  /// check if rectangle is empty
  /** Empty rectangle may be also because of negative end-beg difference */
  bool IsEmpty() const
  {
    return xEnd<=xBeg || zEnd<=zBeg;
  }
  
  /// total number of contained items
  int Area() const {return (xEnd-xBeg)*(zEnd-zBeg);}
  /// x dimension - number of columns
  int Width() const {return xEnd-xBeg;}
  /// z dimension number of rows
  int Height() const {return zEnd-zBeg;}
  
  /**
  @param f once f returns true, foreach is aborted
  */
  template <class Functor>
  bool ForEach(const Functor &f) const
  {
    for (int z=zBeg; z<zEnd; z++) for (int x=xBeg; x<xEnd; x++)
    {
      if (f(x,z)) return true;
    }
    return false;
  }

  /// rectangle complement implemented using rectangle clipping
  template <class Func>
  bool ForEachInComplement(const GridRectangle &i, const Func &f) const
  {
    // E rectangle divides I into 9 regions
    // -------------------I
    // |      123        |
    // |...-----------E..| ... zEdge1
    // | 4 |   5     | 6 |
    // |...-----------...| ... zEdge2
    // |      789        |
    // -------------------
    // handle empty rectangles because of possible unnormalized (end<beg) values
    if (i.IsEmpty()) return false;
    if (IsEmpty() || i.xEnd<=xBeg || xEnd<=i.xBeg || i.zEnd<=zBeg || zEnd<=i.zBeg)
    {
      return i.ForEach(f);
    }
      
    int xEdge1 = intMin(i.xEnd,xBeg);
    int xEdge2 = intMax(i.xBeg,xEnd);

    int zEdge1 = intMin(i.zEnd,zBeg);
    int zEdge2 = intMax(i.zBeg,zEnd);
    
    // regions 123
    if (GridRectangle(i.xBeg,i.zBeg,i.xEnd,zEdge1).ForEach(f))
    {
      return true;
    }
    // regions 456
    int zBeg456 = intMax(zEdge1,i.zBeg);
    int zEnd456 = intMin(zEdge2,i.zEnd);
    // region 4
    if (GridRectangle(i.xBeg,zBeg456,xEdge1,zEnd456).ForEach(f))
    {
      return true;
    }
    // region 5 ignored
    // region 6
    if (GridRectangle(xEdge2,zBeg456,i.xEnd,zEnd456).ForEach(f))
    {
      return true;
    }
    
    // regions 789
    return GridRectangle(i.xBeg,zEdge2,i.xEnd,i.zEnd).ForEach(f);
  }

};

// describe a position inside of the rectangle
struct Position
{
  int x,z;
  
  Position(){}
  Position(int xpos, int zpos):x(xpos),z(zpos){}
  
  /// advance to the next point
  /** once there is no next point, the position becomes invalid */
  Position Advance(const GridRectangle &rect) const
  {
    int nextX = x+1;
    if (nextX<rect.xEnd)
    {
      return Position(nextX,z);
    }
    else
    {
      return Position(rect.xBeg,z+1);
    }
  }
  bool IsInside(const GridRectangle &rect) const
  {
    return x>=rect.xBeg && x<rect.xEnd && z>=rect.zBeg && z<rect.zEnd;
  }
  bool operator == (const Position &p) const {return x==p.x && z==p.z;}
  bool operator < (const Position &p) const
  {
    if (z<p.z) return true;
    if (z>p.z) return false;
    return x<p.x;
  }
};

/// helper - grid traversal with flexible inner/outer direction and coordinate
/**
This is used when we need to traverse grid
while making sure grids traversed first are not hidden by grids traversed later

Typical loop body looks like:

  FlexibleLoop loop(xMin,zMin,xMax,zMax,xStep,zStep,zMajor);
  FLEXIBLE_LOOP(loop,x,z)

*/

struct FlexibleLoop
{
  struct LoopVar
  {
    int c;
    int beg,end,step;
  };

  /// c[0] is major (outer loop), c[1] minor (inner loop) coordinate
  LoopVar c[2];
  //@{ mapping c to X/Z
  int mapCToX, mapCToZ;
  //@}
  /// record range in both directions in case the caller is interested in it
  int range[2];
  
  
  FlexibleLoop(int xMin, int zMin, int xMax, int zMax, int xStep, int zStep, bool zMajor)
  {
    int xBeg = xStep>0 ? xMin : xMax+xStep; 
    int xEnd = xStep>0 ? xMax : xMin+xStep;
    int zBeg = zStep>0 ? zMin : zMax+zStep; 
    int zEnd = zStep>0 ? zMax : zMin+zStep;

    // determine which coordinate is x and which z
    if (zMajor)
    {
      c[0].beg = zBeg, c[0].end = zEnd, c[0].step = zStep;
      c[1].beg = xBeg, c[1].end = xEnd, c[1].step = xStep;
      
      mapCToX = 1;
      mapCToZ = 0;
      
      range[0] = zMax-zMin;
      range[1] = xMax-xMin;
    }
    else
    {
      c[0].beg = xBeg, c[0].end = xEnd, c[0].step = xStep;
      c[1].beg = zBeg, c[1].end = zEnd, c[1].step = zStep;
      
      mapCToX = 0;
      mapCToZ = 1;

      range[0] = xMax-xMin;
      range[1] = zMax-zMin;
    }
  }
  
  //@{ get starting point coordinates
  int BegX() const {return c[mapCToX].beg;}
  int BegZ() const {return c[mapCToZ].beg;}
  //@}
  
  int GetMajorRange() const {return range[0];}
  int GetMinorRange() const {return range[1];}
};

/// macro to avoid typing the loop
/**
We could use a template, but that would require functors
*/
#define FLEXIBLE_LOOP(loop,x,z) \
    for (loop.c[0].c=loop.c[0].beg; loop.c[0].c!=loop.c[0].end; loop.c[0].c+=loop.c[0].step) \
    for (loop.c[1].c=loop.c[1].beg; loop.c[1].c!=loop.c[1].end; loop.c[1].c+=loop.c[1].step) \
    { \
      int x = loop.c[loop.mapCToX].c; \
      int z = loop.c[loop.mapCToZ].c;

#define FLEXIBLE_LOOP_END }

#endif
