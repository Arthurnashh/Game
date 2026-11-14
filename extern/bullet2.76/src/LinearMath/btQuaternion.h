/*
Copyright (c) 2003-2006 Gino van den Bergen / Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
#ifndef SIMD__QUATERNION_H_
#define SIMD__QUATERNION_H_


#include "btVector3.h"
#include "btQuadWord.h"

/**@brief The btQuaternion implements quaternion to perform linear algebra rotations in combination with btMatrix3x3, btVector3 and btTransform. */
class btQuaternion : public btQuadWord {
public:
  /**@brief No initialization constructor */
	btQuaternion() {}

#ifdef BT_PS3PPU_VEC
	SIMD_FORCE_INLINE btQuaternion(const vec_float4 vec)		
	{
		mVec128 = vec;
	}

	SIMD_FORCE_INLINE btQuaternion(const btVector3 vec)		
	{
		mVec128 = vec.mVec;
	}

#endif
	//		template <typename btScalar>
	//		explicit Quaternion(const btScalar *v) : Tuple4<btScalar>(v) {}
  /**@brief Constructor from scalars */
	btQuaternion(const btScalar& x, const btScalar& y, const btScalar& z, const btScalar& w) 
		: btQuadWord(x, y, z, w) 
	{}
  /**@brief Axis angle Constructor
   * @param axis The axis which the rotation is around
   * @param angle The magnitude of the rotation around the angle (Radians) */
	btQuaternion(const btVector3& axis, const btScalar& angle) 
	{ 
		setRotation(axis, angle); 
	}
  /**@brief Constructor from Euler angles
   * @param yaw Angle around Y unless BT_EULER_DEFAULT_ZYX defined then Z
   * @param pitch Angle around X unless BT_EULER_DEFAULT_ZYX defined then Y
   * @param roll Angle around Z unless BT_EULER_DEFAULT_ZYX defined then X */
	btQuaternion(const btScalar& yaw, const btScalar& pitch, const btScalar& roll)
	{ 
#ifndef BT_EULER_DEFAULT_ZYX
		setEuler(yaw, pitch, roll); 
#else
		setEulerZYX(yaw, pitch, roll); 
#endif 
	}
  /**@brief Set the rotation using axis angle notation 
   * @param axis The axis around which to rotate
   * @param angle The magnitude of the rotation in Radians */
	void setRotation(const btVector3& axis, const btScalar& angle)
	{
		btScalar d = axis.length();
		btAssert(d != btScalar(0.0));
		btScalar s = btSin(angle * btScalar(0.5)) / d;
		setValue(axis.getX() * s, axis.getY() * s, axis.getZ() * s, 
			btCos(angle * btScalar(0.5)));
	}
  /**@brief Set the quaternion using Euler angles
   * @param yaw Angle around Y
   * @param pitch Angle around X
   * @param roll Angle around Z */
	void setEuler(const btScalar& yaw, const btScalar& pitch, const btScalar& roll)
	{
		btScalar halfYaw = btScalar(yaw) * btScalar(0.5);  
		btScalar halfPitch = btScalar(pitch) * btScalar(0.5);  
		btScalar halfRoll = btScalar(roll) * btScalar(0.5);  
		btScalar cosYaw = btCos(halfYaw);
		btScalar sinYaw = btSin(halfYaw);
		btScalar cosPitch = btCos(halfPitch);
		btScalar sinPitch = btSin(halfPitch);
		btScalar cosRoll = btCos(halfRoll);
		btScalar sinRoll = btSin(halfRoll);
		setValue(cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,
			cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw,
			sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,
			cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw);
	}
  /**@brief Set the quaternion using euler angles 
   * @param yaw Angle around Z
   * @param pitch Angle around Y
   * @param roll Angle around X */
	void setEulerZYX(const btScalar& yaw, const btScalar& pitch, const btScalar& roll)
	{
		btScalar halfYaw = btScalar(yaw) * btScalar(0.5);  
		btScalar halfPitch = btScalar(pitch) * btScalar(0.5);  
		btScalar halfRoll = btScalar(roll) * btScalar(0.5);  
		btScalar cosYaw = btCos(halfYaw);
		btScalar sinYaw = btSin(halfYaw);
		btScalar cosPitch = btCos(halfPitch);
		btScalar sinPitch = btSin(halfPitch);
		btScalar cosRoll = btCos(halfRoll);
		btScalar sinRoll = btSin(halfRoll);
		setValue(sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw, //x
                         cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw, //y
                         cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw, //z
                         cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw); //formerly yzx
	}
  /**@brief Add two quaternions
   * @param q The quaternion to add to this one */
	SIMD_FORCE_INLINE	btQuaternion& operator+=(const btQuaternion& q)
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(185);
		mVec128 = vec_add(mVec128, q.mVec128);
#else
		m_floats[0] += q.x(); m_floats[1] += q.y(); m_floats[2] += q.z(); m_floats[3] += q.m_floats[3];
#endif
		return *this;
	}

  /**@brief Subtract out a quaternion
   * @param q The quaternion to subtract from this one */
	btQuaternion& operator-=(const btQuaternion& q) 
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(186);
		mVec128 = vec_sub(mVec128, q.mVec128);
#else
		m_floats[0] -= q.x(); m_floats[1] -= q.y(); m_floats[2] -= q.z(); m_floats[3] -= q.m_floats[3];
#endif
		return *this;
	}

  /**@brief Scale this quaternion
   * @param s The scalar to scale by */
	btQuaternion& operator*=(const btScalar& s)
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(156);
		mVec128 = vec_madd(mVec128, vec_float4(s), btZero);
#else
		m_floats[0] *= s; m_floats[1] *= s; m_floats[2] *= s; m_floats[3] *= s;
#endif
		return *this;
	}

  /**@brief Multiply this quaternion by q on the right
   * @param q The other quaternion 
   * Equivilant to this = this * q */
	btQuaternion& operator*=(const btQuaternion& q)
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(157);
		vec_float4 xklm = vec_sld(q.mVec128, q.mVec128, 12);
		vec_float4 tmp = vec_madd(mVec128, vec_sld(xklm, q.mVec128, 8), btZeroVec());	//al, bm, ck, dl
		vec_float4 bmckal = vec_sld(vec_sld(tmp, tmp, 12), tmp, 8);
		tmp = vec_madd(mVec128, vec_sld(xklm, q.mVec128, 12), btZeroVec()); 	//am, bk, cl, dm
		vec_float4 clambk = vec_sld(vec_sld(tmp, tmp, 12), tmp, 12);

		vec_float4 d = vec_splat(mVec128, 3);
		vec_float4 n = vec_splat(q.mVec128, 3);
		vec_float4 res = vec_madd(q.mVec128, d, vec_madd(mVec128, n, vec_sub(bmckal, clambk)));	//dk + an + bm - cl, dl + bn + ck - am, dm + cn + al - bk

		tmp = vec_madd(mVec128, q.mVec128, btZeroVec());	//ak, bl, cm, dn
		tmp = vec_sub(tmp, vec_sld(tmp, tmp, 4));		//ak - bl, bl - cm, dm - dn, dn - ak
		vec_float4 ret = vec_sel(res, vec_sub(tmp, vec_sld(tmp, tmp, 8)), btSelVec3);		//x, x, x, dn - ak - bl - cm
		mVec128 = ret;
#else
		setValue(m_floats[3] * q.x() + m_floats[0] * q.m_floats[3] + m_floats[1] * q.z() - m_floats[2] * q.y(),
			m_floats[3] * q.y() + m_floats[1] * q.m_floats[3] + m_floats[2] * q.x() - m_floats[0] * q.z(),
			m_floats[3] * q.z() + m_floats[2] * q.m_floats[3] + m_floats[0] * q.y() - m_floats[1] * q.x(),
			m_floats[3] * q.m_floats[3] - m_floats[0] * q.x() - m_floats[1] * q.y() - m_floats[2] * q.z());
#endif		
		return *this;
	}
  /**@brief Return the dot product between this quaternion and another
   * @param q The other quaternion */
	btScalar dot(const btQuaternion& q) const
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(158);
		vec_float4 tmp = vec_madd(mVec128, q.mVec128, btZeroVec());
		vec_float4 result = vec_add(tmp, vec_sld(tmp, tmp, 8));
		return btScalar(vec_extract(vec_add(result, vec_sld(result, result, 4)), 0));
#else
		return m_floats[0] * q.x() + m_floats[1] * q.y() + m_floats[2] * q.z() + m_floats[3] * q.m_floats[3];
#endif
	}

  /**@brief Return the length squared of the quaternion */
	btScalar length2() const
	{
		return dot(*this);
	}

  /**@brief Return the length of the quaternion */
	btScalar length() const
	{
		return btSqrt(length2());
	}

  /**@brief Normalize the quaternion 
   * Such that x^2 + y^2 + z^2 +w^2 = 1 */
	btQuaternion& normalize() 
	{
		return *this /= length();
	}

  /**@brief Return a scaled version of this quaternion
   * @param s The scale factor */
	SIMD_FORCE_INLINE btQuaternion
	operator*(const btScalar& s) const
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(187);
		return btQuaternion(vec_madd(mVec128, vec_float4(s), btZero));
#else
		return btQuaternion(x() * s, y() * s, z() * s, m_floats[3] * s);
#endif
	}


  /**@brief Return an inversely scaled versionof this quaternion
   * @param s The inverse scale factor */
	btQuaternion operator/(const btScalar& s) const
	{
		btAssert(s != btScalar(0.0));
		return *this * (btScalar(1.0) / s);
	}

  /**@brief Inversely scale this quaternion
   * @param s The scale factor */
	btQuaternion& operator/=(const btScalar& s) 
	{
		btAssert(s != btScalar(0.0));
		return *this *= btScalar(1.0) / s;
	}

  /**@brief Return a normalized version of this quaternion */
	btQuaternion normalized() const 
	{
		return *this / length();
	} 
  /**@brief Return the angle between this quaternion and the other 
   * @param q The other quaternion */
	btScalar angle(const btQuaternion& q) const 
	{
		btScalar s = btSqrt(length2() * q.length2());
		btAssert(s != btScalar(0.0));
		return btAcos(dot(q) / s);
	}
  /**@brief Return the angle of rotation represented by this quaternion */
	btScalar getAngle() const 
	{
		btScalar s = btScalar(2.) * btAcos(m_floats[3]);
		return s;
	}

	/**@brief Return the axis of the rotation represented by this quaternion */
	btVector3 getAxis() const
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(159);
		btScalar s_squared = btScalar(1.) - btPow(getW(), btScalar(2.));
		if (s_squared < btScalar(10.) * SIMD_EPSILON) //Check for divide by zero
			return btIdentVec0;  // Arbitrary
		float res = 1/btSqrt(s_squared);
		return btVector3(vec_madd(mVec128, vec_splats(res), btZeroVec()));
#else
		btScalar s_squared = btScalar(1.) - btPow(m_floats[3], btScalar(2.));
		if (s_squared < btScalar(10.) * SIMD_EPSILON) //Check for divide by zero
			return btVector3(1.0, 0.0, 0.0);  // Arbitrary
		btScalar s = btSqrt(s_squared);
		return btVector3(m_floats[0] / s, m_floats[1] / s, m_floats[2] / s);
#endif
	}

	/**@brief Return the inverse of this quaternion */
	btQuaternion inverse() const
	{
		return btQuaternion(-m_floats[0], -m_floats[1], -m_floats[2], m_floats[3]);
	}

  /**@brief Return the sum of this quaternion and the other 
   * @param q2 The other quaternion */
	SIMD_FORCE_INLINE btQuaternion
	operator+(const btQuaternion& q2) const
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(188);
		return btQuaternion(vec_add(mVec128, q2.mVec128));
#else
		const btQuaternion& q1 = *this;
		return btQuaternion(q1.x() + q2.x(), q1.y() + q2.y(), q1.z() + q2.z(), q1.m_floats[3] + q2.m_floats[3]);
#endif
	}

  /**@brief Return the difference between this quaternion and the other 
   * @param q2 The other quaternion */
	SIMD_FORCE_INLINE btQuaternion
	operator-(const btQuaternion& q2) const
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(189);
		return btQuaternion(vec_sub(mVec128, q2.mVec128));
#else
		const btQuaternion& q1 = *this;
		return btQuaternion(q1.x() - q2.x(), q1.y() - q2.y(), q1.z() - q2.z(), q1.m_floats[3] - q2.m_floats[3]);
#endif
	}

  /**@brief Return the negative of this quaternion 
   * This simply negates each element */
	SIMD_FORCE_INLINE btQuaternion operator-() const
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(190);
		return btQuaternion(vec_sub(btZero, mVec128));
#else
		const btQuaternion& q2 = *this;
		return btQuaternion( - q2.x(), - q2.y(),  - q2.z(),  - q2.m_floats[3]);
#endif
	}
  /**@todo document this and it's use */
	SIMD_FORCE_INLINE btQuaternion farthest( const btQuaternion& qd) const 
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(200);
		vec_float4 diff = vec_sub(mVec128, qd.mVec128);
		vec_float4 sum = vec_add(mVec128, qd.mVec128);
		vec_float4 tmp = vec_sub(vec_madd(diff, diff, btZero), vec_madd(sum, sum, btZero));
		if(vec_any_gt(vec_sel(btZeroVec(), vec_add(vec_add(tmp, vec_sld(tmp, tmp, 4)), vec_sld(tmp, tmp, 8)), btSelVec0), btZeroVec()));
			return qd;
		return (-qd);
#else
		btQuaternion diff,sum;
		diff = *this - qd;
		sum = *this + qd;
		if( diff.dot(diff) > sum.dot(sum) )
			return qd;
		return (-qd);
#endif
	}

	/**@todo document this and it's use */
	SIMD_FORCE_INLINE btQuaternion nearest( const btQuaternion& qd) const 
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(201);
		vec_float4 diff = vec_sub(mVec128, qd.mVec128);
		vec_float4 sum = vec_add(mVec128, qd.mVec128);
		vec_float4 tmp = vec_sub(vec_madd(diff, diff, btZero), vec_madd(sum, sum, btZero));
		if(vec_any_lt(vec_sel(btZeroVec(), vec_add(vec_add(tmp, vec_sld(tmp, tmp, 4)), vec_sld(tmp, tmp, 8)), btSelVec0), btZeroVec()));
			return qd;
		return (-qd);
#else
		btQuaternion diff,sum;
		diff = *this - qd;
		sum = *this + qd;
		if( diff.dot(diff) < sum.dot(sum) )
			return qd;
		return (-qd);
#endif
	}


  /**@brief Return the quaternion which is the result of Spherical Linear Interpolation between this and the other quaternion
   * @param q The other quaternion to interpolate with 
   * @param t The ratio between this and q to interpolate.  If t = 0 the result is this, if t=1 the result is q.
   * Slerp interpolates assuming constant velocity.  */
	btQuaternion slerp(const btQuaternion& q, const btScalar& t) const
	{
		btScalar theta = angle(q);
		if (theta != btScalar(0.0))
		{
			btScalar d = btScalar(1.0) / btSin(theta);
			btScalar s0 = btSin((btScalar(1.0) - t) * theta);
			btScalar s1 = btSin(t * theta);   
#ifdef BT_PS3PPU_VEC
			//btVector3::btHitPrint(160);
			if (dot(q) < 0) // Take care of long angle case see http://en.wikipedia.org/wiki/Slerp
			{
				vec_float4 tmp = vec_madd(mVec128, vec_float4(s0), btZero);
				tmp = vec_sub(tmp, vec_madd(q.mVec128, vec_float4(s1), btZero));
            return btQuaternion(vec_madd(tmp, vec_float4(d), btZero));
			}
         else
			{
				vec_float4 tmp = vec_madd(mVec128, vec_float4(s0), vec_madd(q.mVec128, vec_float4(s1), btZero));
				return btQuaternion(vec_madd(tmp, vec_float4(d), btZero));
			}
#else
         if (dot(q) < 0) // Take care of long angle case see http://en.wikipedia.org/wiki/Slerp
            return btQuaternion((m_floats[0] * s0 + -q.getX() * s1) * d,
                                 (m_floats[1] * s0 + -q.getY() * s1) * d,
                                 (m_floats[2] * s0 + -q.getZ() * s1) * d,
                                 (m_floats[3] * s0 + -q.m_floats[3] * s1) * d);
         else
            return btQuaternion((m_floats[0] * s0 + q.getX() * s1) * d,
                                 (m_floats[1] * s0 + q.getY() * s1) * d,
                                 (m_floats[2] * s0 + q.getZ() * s1) * d,
                                 (m_floats[3] * s0 + q.m_floats[3] * s1) * d);
#endif
                        
		}
		else
		{
			return *this;
		}
	}

	static const btQuaternion&	getIdentity()
	{
		static const btQuaternion identityQuat(btScalar(0.),btScalar(0.),btScalar(0.),btScalar(1.));
		return identityQuat;
	}

	SIMD_FORCE_INLINE const btScalar& getW() const { return m_floats[3]; }
	
};

#ifdef BT_PS3PPU_VEC
/**@brief Return the negative of a quaternion */
SIMD_FORCE_INLINE btQuaternion
operator-(const btQuaternion& q)
{
	//btVector3::btHitPrint(161);
	return btQuaternion(vec_sub(btZeroVec(), q.mVec128));
}
#else
/**@brief Return the negative of a quaternion */
SIMD_FORCE_INLINE btQuaternion
operator-(const btQuaternion& q)
{
	return btQuaternion(-q.getX(), -q.getY(), -q.getZ(), -q.getW());
}
#endif

#ifdef BT_PS3PPU_VEC
/**@brief Return the product of two quaternions */
SIMD_FORCE_INLINE btQuaternion
operator*(const btQuaternion& q1, const btQuaternion& q2) {
	//btVector3::btHitPrint(162);	//checked

	/*		
	q1 abcd
	q2 xyzw

	dx + aw + bz - cy	//a + b + c - d
	dy + bw + cx - az	//e + f + g - h
	dz + cw + ay - bx	//i + j + k - l
	dw - ax - by - cz	//m - n - o - p
	*/

	vec_float4 nopm = vec_madd(q1.mVec128, q2.mVec128, btZeroVec());
	vec_float4 kcja = vec_madd(q1.mVec128, vec_sld(q2.mVec128, q2.mVec128, 4), btZeroVec());
	vec_float4 hfge = vec_madd(q1.mVec128, vec_sld(q2.mVec128, q2.mVec128, 8), btZeroVec());
	vec_float4 bldi = vec_madd(q1.mVec128, vec_sld(q2.mVec128, q2.mVec128, 12), btZeroVec());

	//aei + bfj + cgk - dhl => agi + bfj + cek - dhl
	vec_float4 agie = vec_vmrglw(vec_vmrglw(kcja, bldi)/*jdai*/, hfge);
	vec_float4 bfjg = vec_vmrghw(vec_vmrghw(bldi, vec_sld(kcja, kcja, 8))/*bjla*/, vec_sld(hfge, hfge, 4));
	vec_float4 cekh = vec_vmrghw(vec_sld(vec_sld(kcja, kcja, 8), kcja, 12)/*ckcj*/, vec_sld(hfge, hfge, 12));
	vec_float4 dhlf =  vec_vmrghw(vec_vmrglw(bldi, vec_sld(bldi, bldi, 12))/*dlid*/, hfge);

	//mnop
	vec_float4 mnop = vec_add(vec_add(nopm, vec_sld(nopm, nopm, 4)), vec_sld(nopm, nopm, 8));
	mnop = vec_sub(nopm, vec_sld(mnop, mnop, 4));

	vec_float4 res = vec_sub(vec_add(vec_add(agie, bfjg), cekh), dhlf);
	
	 return btQuaternion(vec_sel(res, mnop, btSelVec3));
}

SIMD_FORCE_INLINE btQuaternion
operator*(const btQuaternion& q, const btVector3& w)
{
	//btVector3::btHitPrint(163);	//checked

	/*		
	q abcd
	w xyzw

	dx + bz - cy	//a + b - c
	dy + cx - az	//d + e - f
	dz + ay - bx	//g + h - i
	-ax - by - cz	//-j - k - l
	*/

	vec_float4 jklx = vec_madd(q.mVec128, w.mVec, btZeroVec());	//ax, by, cz, dw
	vec_float4 hbxa = vec_madd(q.mVec128, vec_sld(w.mVec, w.mVec, 4), btZeroVec());	//ay, bz, cw, dx
	vec_float4 fxed = vec_madd(q.mVec128, vec_sld(w.mVec, w.mVec, 8), btZeroVec());	//az, bw, cx, dy
	vec_float4 xicg = vec_madd(q.mVec128, vec_sld(w.mVec, w.mVec, 12), btZeroVec());	//aw, bx, cy, dz

	vec_float4 adgf = vec_vmrglw(vec_vmrglw(hbxa, xicg)/*xcag*/, vec_sld(fxed, fxed, 4));
	vec_float4 behd = vec_vmrghw(vec_sld(vec_sld(hbxa, hbxa, 8), hbxa, 12)/*bhbx*/, vec_sld(fxed, fxed, 8));
	vec_float4 cfix = vec_vmrghw(vec_vmrglw(xicg, vec_sld(xicg, xicg, 12))/*cigc*/, fxed);

	vec_float4 res = vec_sub(vec_add(adgf, behd), cfix);
	vec_float4 last = vec_sub(btZeroVec(), jklx);
	last = vec_add(vec_add(vec_sld(last, last, 4), vec_sld(last, last, 8)), vec_sld(last, last, 12));
	vec_float4 ret = vec_sel(res, last, btSelVec3);

	return btQuaternion(ret);
}
#else
/**@brief Return the product of two quaternions */
SIMD_FORCE_INLINE btQuaternion
operator*(const btQuaternion& q1, const btQuaternion& q2) {
	return btQuaternion(q1.getW() * q2.getX() + q1.getX() * q2.getW() + q1.getY() * q2.getZ() - q1.getZ() * q2.getY(),
		q1.getW() * q2.getY() + q1.getY() * q2.getW() + q1.getZ() * q2.getX() - q1.getX() * q2.getZ(),
		q1.getW() * q2.getZ() + q1.getZ() * q2.getW() + q1.getX() * q2.getY() - q1.getY() * q2.getX(),
		q1.getW() * q2.getW() - q1.getX() * q2.getX() - q1.getY() * q2.getY() - q1.getZ() * q2.getZ()); 
}

SIMD_FORCE_INLINE btQuaternion
operator*(const btQuaternion& q, const btVector3& w)
{
	return btQuaternion( q.getW() * w.getX() + q.getY() * w.getZ() - q.getZ() * w.getY(),
		q.getW() * w.getY() + q.getZ() * w.getX() - q.getX() * w.getZ(),
		q.getW() * w.getZ() + q.getX() * w.getY() - q.getY() * w.getX(),
		-q.getX() * w.getX() - q.getY() * w.getY() - q.getZ() * w.getZ()); 
}

SIMD_FORCE_INLINE btQuaternion
operator*(const btVector3& w, const btQuaternion& q)
{
	return btQuaternion( w.getX() * q.getW() + w.getY() * q.getZ() - w.getZ() * q.getY(),
		w.getY() * q.getW() + w.getZ() * q.getX() - w.getX() * q.getZ(),
		w.getZ() * q.getW() + w.getX() * q.getY() - w.getY() * q.getX(),
		-w.getX() * q.getX() - w.getY() * q.getY() - w.getZ() * q.getZ()); 
}
#endif
/**@brief Calculate the dot product between two quaternions */
SIMD_FORCE_INLINE btScalar 
dot(const btQuaternion& q1, const btQuaternion& q2) 
{ 
	return q1.dot(q2); 
}


/**@brief Return the length of a quaternion */
SIMD_FORCE_INLINE btScalar
length(const btQuaternion& q) 
{ 
	return q.length(); 
}

/**@brief Return the angle between two quaternions*/
SIMD_FORCE_INLINE btScalar
angle(const btQuaternion& q1, const btQuaternion& q2) 
{ 
	return q1.angle(q2); 
}

/**@brief Return the inverse of a quaternion*/
SIMD_FORCE_INLINE btQuaternion
inverse(const btQuaternion& q) 
{
	return q.inverse();
}

/**@brief Return the result of spherical linear interpolation betwen two quaternions 
 * @param q1 The first quaternion
 * @param q2 The second quaternion 
 * @param t The ration between q1 and q2.  t = 0 return q1, t=1 returns q2 
 * Slerp assumes constant velocity between positions. */
SIMD_FORCE_INLINE btQuaternion
slerp(const btQuaternion& q1, const btQuaternion& q2, const btScalar& t) 
{
	return q1.slerp(q2, t);
}

SIMD_FORCE_INLINE btVector3 
quatRotate(const btQuaternion& rotation, const btVector3& v) 
{
	btQuaternion q = rotation * v;
	q *= rotation.inverse();
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(165);
	return btVector3(q.get128());
#else
	return btVector3(q.getX(),q.getY(),q.getZ());
#endif
}

SIMD_FORCE_INLINE btQuaternion  
shortestArcQuat(const btVector3& v0, const btVector3& v1) // Game Programming Gems 2.10. make sure v0,v1 are normalized
{
	btVector3 c = v0.cross(v1);
	btScalar  d = v0.dot(v1);

	if (d < -1.0 + SIMD_EPSILON)
	{
		btVector3 n,unused;
		btPlaneSpace1(v0,n,unused);
		return btQuaternion(n.getX(),n.getY(),n.getZ(),0.0f); // just pick any vector that is orthogonal to v0
	}

	btScalar tmp = 1.0f + d;
	btScalar  s = btSqrt(tmp + tmp);
	btScalar rs = 1.0f / s;

	return btQuaternion(c.getX()*rs,c.getY()*rs,c.getZ()*rs,s * 0.5f);
}

SIMD_FORCE_INLINE btQuaternion 
shortestArcQuatNormalize2(btVector3& v0,btVector3& v1)
{
	v0.normalize();
	v1.normalize();
	return shortestArcQuat(v0,v1);
}

#endif
