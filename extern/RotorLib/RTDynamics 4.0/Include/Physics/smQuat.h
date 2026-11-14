/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include <Physics/smVec4r.h>
#include "Util/smEventGeneratorTrait.h"

/**
*	An smQuat represents a quaternion. The internal vector _v
*	includes the quaternion components which are defined as:
*
*		_v[0...3] = [x y z w]
*
*	The smQuat class provides methods for conversions to/from 
*	smMatrix class as well as quaternion interpolations.
*/
class  smQuat : public smEventGeneratorTrait
{

public:
	/** _v[0..3] is defined as x,y,z and w	*/
	smReal  _v[4];

	inline smQuat() { _v[0]=0.0f; _v[1]=0.0f; _v[2]=0.0f; _v[3]=0.0f; }

	inline smQuat( smReal x, smReal y, smReal z, smReal w )
	{
		_v[0]=x;
		_v[1]=y;
		_v[2]=z;
		_v[3]=w;

		callEventHandlers( smEventGeneratorTrait::VALUE_CHANGED ) ;
	}

	inline smQuat( const smVec4r& v )
	{
		_v[0]=v.x();
		_v[1]=v.y();
		_v[2]=v.z();
		_v[3]=v.w();

		callEventHandlers( smEventGeneratorTrait::VALUE_CHANGED ) ;
	}

	inline smQuat( smReal angle, const smVec3r& axis)
	{
		makeRotate(angle,axis);
	}

	inline smQuat( smReal angle1, const smVec3r& axis1,
		smReal angle2, const smVec3r& axis2,
		smReal angle3, const smVec3r& axis3)
	{
		makeRotate(angle1,axis1,angle2,axis2,angle3,axis3);
	}

	inline bool operator == (const smQuat& v) const { return _v[0] == v._v[0] && _v[1] == v._v[1] && _v[2] == v._v[2] && _v[3] == v._v[3]; }
	inline bool operator != (const smQuat& v) const { return _v[0] != v._v[0] || _v[1] != v._v[1] || _v[2] != v._v[2] || _v[3] != v._v[3]; }

	inline smVec4r asVec4() const
	{	return smVec4r(_v[0], _v[1], _v[2], _v[3]);	}

	inline smVec3r asVec3() const
	{	return smVec3r(_v[0], _v[1], _v[2]); }

	inline void set(smReal x, smReal y, smReal z, smReal w)
	{
		_v[0]=x;
		_v[1]=y;
		_v[2]=z;
		_v[3]=w;

		callEventHandlers( smEventGeneratorTrait::VALUE_CHANGED ) ;
	}

	inline void set(const smVec4r& v)
	{
		_v[0]=v.x();
		_v[1]=v.y();
		_v[2]=v.z();
		_v[3]=v.w();

		callEventHandlers( smEventGeneratorTrait::VALUE_CHANGED ) ;
	}

	/** Set this quaternion with the given rotation matrix. */
	void set(const smMatrix& matrix);

	/** Convert this quaternion to a rotation matrix. */
	void get(smMatrix& matrix) const;

	inline smReal & operator [] (size_t i) { return _v[i]; }
	inline smReal   operator [] (size_t i) const { return _v[i]; }

	inline smReal x() const { return _v[0]; }
	inline smReal y() const { return _v[1]; }
	inline smReal z() const { return _v[2]; }
	inline smReal w() const { return _v[3]; }

	inline smReal & x() { return _v[0]; }
	inline smReal & y() { return _v[1]; }
	inline smReal & z() { return _v[2]; }
	inline smReal & w() { return _v[3]; }

	/** return true if the smQuat represents a zero rotation, and therefore can be ignored in computations.*/
	//bool zeroRotation() const { return _v[0]==0.0f && _v[1]==0.0f && _v[2]==0.0f && _v[3]==1.0f; }

	/** Multiply by a scalar value */
	inline const smQuat operator * (float rhs) const
	{
		return smQuat(_v[0]*rhs, _v[1]*rhs, _v[2]*rhs, _v[3]*rhs);
	}

	/** Unary multiplication by a scalar value */
	inline smQuat& operator *= (float rhs)
	{
		_v[0]*=rhs;
		_v[1]*=rhs;
		_v[2]*=rhs;
		_v[3]*=rhs;

		callEventHandlers( smEventGeneratorTrait::VALUE_CHANGED ) ;

		return *this;
	}

	/** Binary multiplication */
	inline const smQuat operator*(const smQuat& rhs) const
	{
		return smQuat( rhs._v[3]*_v[0] + rhs._v[0]*_v[3] + rhs._v[1]*_v[2] - rhs._v[2]*_v[1],
			rhs._v[3]*_v[1] - rhs._v[0]*_v[2] + rhs._v[1]*_v[3] + rhs._v[2]*_v[0],
			rhs._v[3]*_v[2] + rhs._v[0]*_v[1] - rhs._v[1]*_v[0] + rhs._v[2]*_v[3],
			rhs._v[3]*_v[3] - rhs._v[0]*_v[0] - rhs._v[1]*_v[1] - rhs._v[2]*_v[2] );
	}

	/** Unary multiplication */
	inline smQuat& operator*=(const smQuat& rhs)
	{
		smReal x = rhs._v[3]*_v[0] + rhs._v[0]*_v[3] + rhs._v[1]*_v[2] - rhs._v[2]*_v[1];
		smReal y = rhs._v[3]*_v[1] - rhs._v[0]*_v[2] + rhs._v[1]*_v[3] + rhs._v[2]*_v[0];
		smReal z = rhs._v[3]*_v[2] + rhs._v[0]*_v[1] - rhs._v[1]*_v[0] + rhs._v[2]*_v[3];
		_v[3]   = rhs._v[3]*_v[3] - rhs._v[0]*_v[0] - rhs._v[1]*_v[1] - rhs._v[2]*_v[2];

		_v[2] = z;
		_v[1] = y;
		_v[0] = x;

		callEventHandlers( smEventGeneratorTrait::VALUE_CHANGED ) ;

		return (*this);
	}

	/** Division by a scalar */
	inline smQuat operator / (float rhs) const
	{
		smReal div = 1.0f/rhs;
		return smQuat(_v[0]*div, _v[1]*div, _v[2]*div, _v[3]*div);
	}

	/** Unary division by a scalar */
	inline smQuat& operator /= (float rhs)
	{
		smReal div = 1.0f/rhs;
		_v[0]*=div;
		_v[1]*=div;
		_v[2]*=div;
		_v[3]*=div;

		callEventHandlers( smEventGeneratorTrait::VALUE_CHANGED ) ;

		return *this;
	}

	/** Binary division	*/
	inline const smQuat operator/(const smQuat& bottom) const
	{
		return ( (*this) * bottom.inverse() );
	}

	/** Unary division	*/
	inline smQuat& operator/=(const smQuat& bottom)
	{
		(*this) = (*this) * bottom.inverse();

		callEventHandlers( smEventGeneratorTrait::VALUE_CHANGED ) ;

		return (*this);
	}

	/** Binary addition	*/
	inline const smQuat operator + (const smQuat& rhs) const
	{
		return smVec4r(_v[0]+rhs._v[0], _v[1]+rhs._v[1],
			_v[2]+rhs._v[2], _v[3]+rhs._v[3]);
	}

	/** Unary addition	*/
	inline smQuat& operator += (const smQuat& rhs)
	{
		_v[0] += rhs._v[0];
		_v[1] += rhs._v[1];
		_v[2] += rhs._v[2];
		_v[3] += rhs._v[3];

		callEventHandlers( smEventGeneratorTrait::VALUE_CHANGED ) ;

		return *this;			// enable nesting
	}

	/** Binary subtraction	*/
	inline const smQuat operator - (const smQuat& rhs) const
	{
		return smQuat(_v[0]-rhs._v[0], _v[1]-rhs._v[1],
			_v[2]-rhs._v[2], _v[3]-rhs._v[3] );
	}

	/** Unary subtraction	*/
	inline smQuat& operator -= (const smQuat& rhs)
	{
		_v[0]-=rhs._v[0];
		_v[1]-=rhs._v[1];
		_v[2]-=rhs._v[2];
		_v[3]-=rhs._v[3];

		callEventHandlers( smEventGeneratorTrait::VALUE_CHANGED ) ;

		return *this;
	}

	/** The negation operator simply negates all components of the quaternion. */
	inline const smQuat operator - () const
	{
		return smQuat (-_v[0], -_v[1], -_v[2], -_v[3]);
	}

	/** Length of the quaternion = sqrt( vec . vec ) */
	smReal length() const
	{
		return static_cast<smReal>( sqrt( _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] + _v[3]*_v[3]) );
	}

	/** Fast Length of the quaternion = vec . vec */
	smReal length2() const
	{
		return _v[0]*_v[0] + _v[1]*_v[1] + _v[2]*_v[2] + _v[3]*_v[3];
	}

	/** Normalizes the given quat into this quat */
	void normalize( const smQuat& quat )
	{
		smReal length = quat.length() ;

		// Here we want to avoid a division by zero
		if ( length != 0.0f )
			for( size_t i = 0 ; i < 4 ; i++ )
				_v[i] = quat[i] / length ;

		callEventHandlers( smEventGeneratorTrait::VALUE_CHANGED ) ;
	}

	/** Conjugation */
	inline smQuat conj () const
	{
		return smQuat( -_v[0], -_v[1], -_v[2], _v[3] );
	}

	/* Return the inverse quaternion of this quaternion */
	inline const smQuat inverse () const
	{
		return conj() / length2();
	}

	/**
	@name Rotation methods
	Set the components of this quaternion so that the quaternion
	performs a rotation around the given axis by the given angle.	

	@param	angle	angle in radians
	*/
	//@{
	void makeRotate( smAngleInDeg angle, smReal  x, smReal  y, smReal  z );
	void makeRotate( smAngleInDeg angle, const smVec3r& vec );

	void makeRotate (	smAngleInDeg  angle1, const smVec3r& axis1,
						smAngleInDeg  angle2, const smVec3r& axis2,
						smAngleInDeg  angle3, const smVec3r& axis3);
	//@}

	/** Make a rotation smQuat which will rotate vec1 to vec2.	*/
	void makeRotate( const smVec3r& vec1, const smVec3r& vec2 );

	/** getRotate returns the angle and vector components of this quaternion.*/
	void getRotation( smReal & angle, smReal & x, smReal & y, smReal & z ) const;

	/** getRotate returns the angle and vector components of this quaternion.*/
	void getRotation( smReal & angle, smVec3r& vec ) const;

	/** Spherical Linear Interpolation.
		As t goes from 0 to 1, the smQuat object goes from "from" to "to". 
	*/
	void slerp   ( smReal  t, const smQuat& from, const smQuat& to);

	/** Debug print he components of this quaternion to a printer. */
	void print( smPrinter* pr, std::string prefix ) const ;
};