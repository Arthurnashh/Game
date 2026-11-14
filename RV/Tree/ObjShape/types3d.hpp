#ifndef _TYPES3D_HPP
#define _TYPES3D_HPP

#include <math.h>
#include <float.h>
#include <El/Math/math3d.hpp>

class Point2D
{
    public:
    int x,y;
    
    Point2D( int xx, int yy ){x=xx,y=yy;}
    Point2D( float xx, float yy ){x=toInt(xx),y=toInt(yy);}
    int X() const {return x;}
    int Y() const {return y;}
};

typedef Vector3 Point3D;


class Volume3D
{
    private:
    Point3D _min,_max;
    
    public:
    Volume3D():
    _min(-FLT_MAX,-FLT_MAX,-FLT_MAX),
    _max(+FLT_MAX,+FLT_MAX,+FLT_MAX)
    {
    }
    Volume3D( const Point3D &min, const Point3D &max ):
    _min(min),
    _max(max)
    {
    }
    //const Point3D &Min() const {return _min;}
    //const Point3D &Max() const {return _max;}
    Volume3D Intersect( const Volume3D &with ) const;
    Volume3D Combine( const Volume3D &with ) const;
    bool Contains( const Point3D &point ) const;
    //FLYWString Description( const Matrix4 &viewToModel, int viewType ) const;
};

#endif
