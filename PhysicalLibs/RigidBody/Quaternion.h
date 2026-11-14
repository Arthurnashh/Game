#ifdef _MSC_VER
#pragma once
#endif

#ifndef __QUATERNION_H__
#define __QUATERNION_H__

class QuaternionRB 
{
private:
	union 
	{
		struct {
			float _w,_x,_y,_z; // TODO nice representation could be float w, CVector3 v
		};
		float _m[4];
	};
public:
	QuaternionRB(const QuaternionRB& cQuat): _w(cQuat._w), _x(cQuat._x), _y(cQuat._y), _z(cQuat._z) {};
	QuaternionRB(float w, float x, float y, float z): _w(w), _x(x), _y(y), _z(z) {};
	QuaternionRB(): _w(1.0f), _x(0.0f), _y(0.0f), _z(0.0f) {};
	QuaternionRB(const Vector3& cVector): _w(0.0f), _x(cVector[0]), _y(cVector[1]), _z(cVector[2]) {};
  QuaternionRB(Matrix3Par mat) {FromMatrixRotation(mat);};  
  QuaternionRB(float angleRad, const Vector3& axis) {angleRad /= 2; _w = cosf(angleRad); float s = sinf(angleRad); 
  _x = axis[0] * s; _y = axis[1] * s; _z = axis[2] * s;};


	// Set Functions
	void Set(float *m) {memcpy(_m, m, 4 * sizeof(m[1]));};
	void Set(float w,float x,float y,float z) {_w = w; _x = x; _y = y; _z = z;};

	// Get Fuctions
	float * Getm() {return _m;};
    const float * Getm() const {return _m;};
	float Getw() const {return _w;};
	float Getx() const {return _x;};
	float Gety() const {return _y;};
	float Getz() const {return _z;};

	__forceinline void Normalize();
  __forceinline float Size();

	// Operators
	__inline const QuaternionRB& operator=( const QuaternionRB& cSecond);
	__inline const QuaternionRB& operator=(  Vector3& cSecond);

	__inline QuaternionRB operator+( const QuaternionRB& cSecond) const;
	__inline QuaternionRB operator-( const QuaternionRB& cSecond) const;
	__inline QuaternionRB operator*( const QuaternionRB& cSecond) const;
	__inline QuaternionRB operator*( float fSecond) const;

	__inline const QuaternionRB& operator+=( const QuaternionRB& cSecond);
	__inline const QuaternionRB& operator-=( const QuaternionRB& cSecond);
	__inline const QuaternionRB& operator*=( const QuaternionRB& cSecond);
	__inline const QuaternionRB& operator*=( float fSecond);
  __inline bool operator==( const QuaternionRB& cSecond) const;

  __inline float& operator[](unsigned int i) {return _m[i];};
  __inline float operator[](unsigned int i) const {return _m[i];};

	// Functions
	__inline Matrix3 GetMatrix() const;
  __inline void FromMatrixRotation(Matrix3Par m);
  __inline QuaternionRB GetTransposed() const {return QuaternionRB(_w, -_x, -_y, -_z);};
};

#include "Quaternion.inl"


#endif //__QUATERNION_H__