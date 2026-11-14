#ifndef _PLANE_HPP
#define _PLANE_HPP

#ifdef PREPROCESS_DOCUMENTATION
#include "networkObject.hpp"
#endif

#include<El/Math/math3d.hpp>

class Plane
{
	Vector3 _normal;
	float _d; // plane equation is _normal*x+d=0

	public:
	Plane(){}
	Plane( Vector3Par normal, float d )
	:_normal(normal),_d(d)
	{}
	Plane( Vector3Par normal, Vector3Par point );

	void SetNormal( Vector3Par normal, float d ) {_normal=normal,_d=d;}
	void SetD( float d ) {_d=d;}
	Vector3Val Normal() const {return _normal;}
	float D() const {return _d;}
	float Distance( Vector3Par x ) const {return _normal*x+_d;}
	// negative distance means the point is inside

	// transform plane equation using general matrix (including scaled...)
	void Transform( const Matrix4 &trans, const Matrix4 &iTrans );
};

TypeIsSimple(Plane);

#endif
