#ifndef _PLANE_HPP
#define _PLANE_HPP

/// plane and halfspace equation
class Plane
{
  /// plane normal - faces outside off the halfspace
	Vector3 _normal;
	/// d factor of the plane equation: _normal*x+d=0
	float _d;
  
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
	/// distance between point and plane
	/** negative distance means the point is inside */
	float Distance(Vector3Par x) const {return _normal*x+_d;}
	/// clip a section of a line by a plane
	float Clip(float &t1, float &t2, Vector3Par p1, Vector3Par p2, float radius) const;
	/** transform plane equation using general matrix (including scaled...)
    * @param trans - transformation matrix; 
    * @param iTrans - inverse matrix of trans.Orientation
    */  
	void Transform(const Matrix4 &invTrans );
};

TypeIsSimple(Plane);

#endif
