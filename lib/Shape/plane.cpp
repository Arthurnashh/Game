// Poseidon - vehicle class
// (C) 1997, Suma
#include "../wpch.hpp"
#include "plane.hpp"


// plane calculations

Plane::Plane( Vector3Par normal, Vector3Par point )
:_normal(normal),_d(-normal*point)
{}

void Plane::Transform(const Matrix4 &invTrans ) 
{            
  _d += invTrans.Position()*_normal;
  // InverseRotation returns transposed matrix         
  _normal.SetMultiplyLeft(_normal, invTrans.Orientation());
} 

/**
@return return the position of the intesection (in terms of "t")
The section of the line endpoints are defined as:
beg = p1+(p2-p1)*t1
end = p1+(p2-p1)*t2

t1>=t2 means the line was fully clipped

Halfspace defines what should be removed - only part of the line outside of the halfspace is left.
The clipping halfspace is decreased by 'radius' - meaning less is clipped for positive radius.
*/

float Plane::Clip(float &t1, float &t2, Vector3Par p1, Vector3Par p2, float radius) const
{
  Vector3 normal = _normal;
  // Enlarging is done by moving the plane in the normal direction by increasing d.
  float d = _d+radius;
  
  // calculate a line intersection with a plane
  // check if t1 or t2 is outside of the plane

	Vector3 p2MinusP1 = p2-p1;

  // calculate intersection point
	// check if p1..p2 has a tendency to go inside the half space or out
	float cosOutAngle = p2MinusP1*normal;
	if (cosOutAngle<0)
	{
	  // inbound - clip p2
	  // p1 is out, p2 is in
	  // find a point on p1..p2 x so that x*normal+d == 0
    float isect = (p1*normal+d)/-cosOutAngle;
    
	  if (t2>isect) t2 = isect;
	  return isect;
	}
	else if (cosOutAngle>0)
	{
    float isect = (p1*normal+d)/-cosOutAngle;
	  // outbound - clip p1
	  // p2 is out, p1 is in
	  if (t1<isect) t1 = isect;
	  return isect;
	}
	else
	{
	   // parallel - no intersection
	   // note: might still be clipped whole
	   float p1Distance = normal*p1 + d;
	   if (p1Distance<0) t1 = t2; // clip whole by making it invalid
	   return t1;
	}
}
