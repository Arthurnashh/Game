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
#ifndef	BT_MATRIX3x3_H
#define BT_MATRIX3x3_H

#include "btVector3.h"
#include "btQuaternion.h"

#ifdef BT_USE_DOUBLE_PRECISION
#define btMatrix3x3Data	btMatrix3x3DoubleData 
#else
#define btMatrix3x3Data	btMatrix3x3FloatData
#endif //BT_USE_DOUBLE_PRECISION

/**@brief The btMatrix3x3 class implements a 3x3 rotation matrix, to perform linear algebra in combination with btQuaternion, btTransform and btVector3.
* Make sure to only include a pure orthogonal matrix without scaling. */
class btMatrix3x3 {

	///Data storage for the matrix, each vector is a row of the matrix
	btVector3 m_el[3];

public:

#ifdef BT_PS3PPU_VEC
	btMatrix3x3(const vec_float4 a, const vec_float4 b, const vec_float4 c)
	{
		m_el[0].mVec = a;
		m_el[1].mVec = b;
		m_el[2].mVec = c;
	};

	SIMD_FORCE_INLINE btVector3&  operator[](int i)
	{ 
		btFullAssert(0 <= i && i < 3);
		return m_el[i]; 
	}
	/** @brief Get a const reference to a row of the matrix as a vector 
	*  @param i Row number 0 indexed */
	SIMD_FORCE_INLINE const btVector3& operator[](int i) const
	{
		btFullAssert(0 <= i && i < 3);
		return m_el[i]; 
	} 

	SIMD_FORCE_INLINE const vec_float4*	get128ptr() const
	{
		return (vec_float4*)m_el;
	}

#endif
	/** @brief No initializaion constructor */
	btMatrix3x3 () {}

	//		explicit btMatrix3x3(const btScalar *m) { setFromOpenGLSubMatrix(m); }

	/**@brief Constructor from Quaternion */
	explicit btMatrix3x3(const btQuaternion& q) { setRotation(q); }
	/*
	template <typename btScalar>
	Matrix3x3(const btScalar& yaw, const btScalar& pitch, const btScalar& roll)
	{ 
	setEulerYPR(yaw, pitch, roll);
	}
	*/
	/** @brief Constructor with row major formatting */

	btMatrix3x3(const btScalar& xx, const btScalar& xy, const btScalar& xz,
		const btScalar& yx, const btScalar& yy, const btScalar& yz,
		const btScalar& zx, const btScalar& zy, const btScalar& zz)
	{ 
		setValue(xx, xy, xz, 
			yx, yy, yz, 
			zx, zy, zz);
	}

	/** @brief Copy constructor */
	SIMD_FORCE_INLINE btMatrix3x3 (const btMatrix3x3& other)
	{
		m_el[0] = other.m_el[0];
		m_el[1] = other.m_el[1];
		m_el[2] = other.m_el[2];
	}
	/** @brief Assignment Operator */
	SIMD_FORCE_INLINE btMatrix3x3& operator=(const btMatrix3x3& other)
	{
		m_el[0] = other.m_el[0];
		m_el[1] = other.m_el[1];
		m_el[2] = other.m_el[2];
		return *this;
	}

#ifdef BT_PS3PPU_VEC
	SIMD_FORCE_INLINE btVector3 getDiagonal() const
	{
		//btVector3::btHitPrint(181);
		return btVector3(vec_vmrghw(vec_vmrghw(m_el[0].mVec, vec_sld(m_el[2].mVec, m_el[2].mVec, 8)), vec_sld(m_el[1].mVec, m_el[1].mVec, 4)));
	}
#endif

	/** @brief Get a column of the matrix as a vector 
	*  @param i Column number 0 indexed */
	SIMD_FORCE_INLINE btVector3 getColumn(int i) const
	{
		return btVector3(m_el[0][i],m_el[1][i],m_el[2][i]);
	}


	/** @brief Get a row of the matrix as a vector 
	*  @param i Row number 0 indexed */
	SIMD_FORCE_INLINE const btVector3 getRow(int i) const
	{
		btFullAssert(0 <= i && i < 3);
		return m_el[i];
	}

	/** @brief Get a mutable reference to a row of the matrix as a vector 
	*  @param i Row number 0 indexed */
#ifndef BT_PS3PPU_VEC
	SIMD_FORCE_INLINE btVector3&  operator[](int i)
	{ 
		btFullAssert(0 <= i && i < 3);
		return m_el[i]; 
	}

	/** @brief Get a const reference to a row of the matrix as a vector 
	*  @param i Row number 0 indexed */
	SIMD_FORCE_INLINE const btVector3& operator[](int i) const
	{
		btFullAssert(0 <= i && i < 3);
		return m_el[i]; 
	}
#endif

	/** @brief Multiply by the target matrix on the right
	*  @param m Rotation matrix to be applied 
	* Equivilant to this = this * m */
	btMatrix3x3& operator*=(const btMatrix3x3& m); 

	/** @brief Set from a carray of btScalars 
	*  @param m A pointer to the beginning of an array of 9 btScalars */
#ifndef BT_PS3PPU_VEC
	void setFromOpenGLSubMatrix(const btScalar *m)
	{
		m_el[0].setValue(m[0],m[4],m[8]);
		m_el[1].setValue(m[1],m[5],m[9]);
		m_el[2].setValue(m[2],m[6],m[10]);
	}
#endif
	/** @brief Set the values of the matrix explicitly (row major)
	*  @param xx Top left
	*  @param xy Top Middle
	*  @param xz Top Right
	*  @param yx Middle Left
	*  @param yy Middle Middle
	*  @param yz Middle Right
	*  @param zx Bottom Left
	*  @param zy Bottom Middle
	*  @param zz Bottom Right*/
	void setValue(const btScalar& xx, const btScalar& xy, const btScalar& xz, 
		const btScalar& yx, const btScalar& yy, const btScalar& yz, 
		const btScalar& zx, const btScalar& zy, const btScalar& zz)
	{
		m_el[0].setValue(xx,xy,xz);
		m_el[1].setValue(yx,yy,yz);
		m_el[2].setValue(zx,zy,zz);
	}
	/** @brief Set the matrix from a quaternion
	*  @param q The Quaternion to match */  
	void setRotation(const btQuaternion& q) 
	{
		btScalar d = q.length2();
		btFullAssert(d != btScalar(0.0));
		btScalar s = btScalar(2.0) / d;
	
#ifdef BT_PS3PPU_VEC_SLOW
		//btVector3::btHitPrint(138);
		vec_float4 qVec = q.get128();
		vec_float4 basis = vec_madd(vec_float4(s), qVec, btZeroVec());			//xyzw => xs, ys, zs, ws	

		vec_float4 a = vec_madd(basis, qVec, btZeroVec());							//(a0)xxs, (a1)yys, (a2)zzs, (a3)*wws			
		vec_float4 b = vec_madd(basis, vec_sld(qVec, qVec, 4), btZeroVec());		//(b0)xys, (b1)yzs, (b2)wzs, (b3)wxs
		vec_float4 c = vec_madd(basis, vec_sld(qVec, qVec, 8), btZeroVec());		//(c0)xzs, (c1)wys, (c2)xzs, (c3)wys
		
		/*
		x: (1.0 - (a1 + a2), b0 - b2, c02 + c13)
		y: (b0 + b2, 1.0 - (a0 + a2), b1 - b3)
		z: (c02 - c13, b1 + b3, 1.0 - (a0 + a1))
		*/
		vec_float4 tmp2 = vec_sld(a, a, 4);	//a1 a2 ax a0
		vec_float4 tmp = vec_vmrghw(a, tmp2);					//a0 a1 a1 a2 -> 
		vec_float4 x = vec_sub(vec_float4(1.f), vec_add(tmp2, tmp));			//1 - (a0 + a1), 1 - (a1 + a2), x, 1 - (a0 + a2)

		tmp = vec_vmrghw(b, c);					//b0 c0 b1 c1
		vec_float4 tmp1 = vec_vmrglw(b, vec_sld(c, c, 12));	//b2 c3 b3 c0
		vec_float4 y = vec_add(tmp, tmp1);	//(b0 + b2), (c0 + c3), (b1 + b3), (c0  + c1)

		vec_float4 z = vec_sub(tmp, tmp1);

		/*
		a: (x1, z0, y13, x)
		b: (y0, x3, z2, x)
		c: (z1, y2, x0, x)
		*/
		tmp = vec_sld(z, z, 4);		//z1 z2 z3 z0
		tmp1 = vec_vmrghw(tmp, x);	//z1 x0 z2 x1
		tmp2 = vec_sld(y, y, 8);	//y2 y3 y0 y1
		m_el[2].mVec = vec_vmrghw(tmp1, tmp2);

		tmp = vec_vmrglw(tmp2, tmp1);	//y0 z2 y1 x1
		tmp1 = vec_sld(x, x, 12);	//x3, x0, x1, x2
		m_el[1].mVec = vec_vmrghw(tmp, tmp1);

		m_el[0].mVec = vec_vmrghw(vec_vmrglw(tmp1, tmp), z);	//x1 y1 x2 x1 -> x1 z0 y1 z1
#else
		btScalar xs = q.x() * s,   ys = q.y() * s,   zs = q.z() * s;
		btScalar wx = q.w() * xs,  wy = q.w() * ys,  wz = q.w() * zs;
		btScalar xx = q.x() * xs,  xy = q.x() * ys,  xz = q.x() * zs;
		btScalar yy = q.y() * ys,  yz = q.y() * zs,  zz = q.z() * zs;
		setValue(btScalar(1.0) - (yy + zz), xy - wz, xz + wy,
			xy + wz, btScalar(1.0) - (xx + zz), yz - wx,
			xz - wy, yz + wx, btScalar(1.0) - (xx + yy));
#endif
	}


	/** @brief Set the matrix from euler angles using YPR around YXZ respectively
	*  @param yaw Yaw about Y axis
	*  @param pitch Pitch about X axis
	*  @param roll Roll about Z axis 
	*/
	void setEulerYPR(const btScalar& yaw, const btScalar& pitch, const btScalar& roll) 
	{
		setEulerZYX(roll, pitch, yaw);
	}

	/** @brief Set the matrix from euler angles YPR around ZYX axes
	* @param eulerX Roll about X axis
	* @param eulerY Pitch around Y axis
	* @param eulerZ Yaw aboud Z axis
	* 
	* These angles are used to produce a rotation matrix. The euler
	* angles are applied in ZYX order. I.e a vector is first rotated 
	* about X then Y and then Z
	**/
	void setEulerZYX(btScalar eulerX,btScalar eulerY,btScalar eulerZ) { 
		///@todo proposed to reverse this since it's labeled zyx but takes arguments xyz and it will match all other parts of the code
		btScalar ci ( btCos(eulerX)); 
		btScalar cj ( btCos(eulerY)); 
		btScalar ch ( btCos(eulerZ)); 
		btScalar si ( btSin(eulerX)); 
		btScalar sj ( btSin(eulerY)); 
		btScalar sh ( btSin(eulerZ)); 
		btScalar cc = ci * ch; 
		btScalar cs = ci * sh; 
		btScalar sc = si * ch; 
		btScalar ss = si * sh;

		setValue(cj * ch, sj * sc - cs, sj * cc + ss,
			cj * sh, sj * ss + cc, sj * cs - sc, 
			-sj,      cj * si,      cj * ci);
	}

	/**@brief Set the matrix to the identity */
	void setIdentity()
	{ 
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(139);
		m_el[0].mVec = btIdentVec0;
		m_el[1].mVec = btIdentVec1;
		m_el[2].mVec = btIdentVec2;
#else
		setValue(btScalar(1.0), btScalar(0.0), btScalar(0.0), 
			btScalar(0.0), btScalar(1.0), btScalar(0.0), 
			btScalar(0.0), btScalar(0.0), btScalar(1.0)); 
#endif
	}

	static const btMatrix3x3&	getIdentity()
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(140);
		static const btMatrix3x3 identityMatrix(btIdentVec0, btIdentVec1, btIdentVec2);
#else
		static const btMatrix3x3 identityMatrix(btScalar(1.0), btScalar(0.0), btScalar(0.0), 
			btScalar(0.0), btScalar(1.0), btScalar(0.0), 
			btScalar(0.0), btScalar(0.0), btScalar(1.0));
#endif
		return identityMatrix;
	}

	/**@brief Fill the values of the matrix into a 9 element array 
	* @param m The array to be filled */
#ifndef BT_PS3PPU_VEC
	void getOpenGLSubMatrix(btScalar *m) const 
	{
		m[0]  = btScalar(m_el[0].x()); 
		m[1]  = btScalar(m_el[1].x());
		m[2]  = btScalar(m_el[2].x());
		m[3]  = btScalar(0.0); 
		m[4]  = btScalar(m_el[0].y());
		m[5]  = btScalar(m_el[1].y());
		m[6]  = btScalar(m_el[2].y());
		m[7]  = btScalar(0.0); 
		m[8]  = btScalar(m_el[0].z()); 
		m[9]  = btScalar(m_el[1].z());
		m[10] = btScalar(m_el[2].z());
		m[11] = btScalar(0.0); 
	}
#endif
	/**@brief Get the matrix represented as a quaternion 
	* @param q The quaternion which will be set */
	void getRotation(btQuaternion& q) const
	{
#ifdef BT_PS3PPU_VEC_SLOW
		//btVector3::btHitPrint(141);
		vec_float4 cyzox = vec_sld(m_el[2].mVec, m_el[2].mVec, 4);
		vec_float4 aoxyz = vec_sld(m_el[0].mVec, m_el[0].mVec, 12);

		float maxDiag[4] __attribute__ ((aligned (16)));
		vec_stvlx(vec_vmrglw(vec_vmrghw(aoxyz, cyzox), vec_sld(m_el[1].mVec, m_el[1].mVec, 12)), 0, maxDiag);
			
		btScalar tracev = maxDiag[0] + maxDiag[1] + maxDiag[2];
		vec_float4 tmp;

		vec_float4 cyazbx = vec_vmrghw(vec_vmrghw(cyzox, m_el[1].mVec), vec_sld(m_el[0].mVec, m_el[0].mVec, 8));
		vec_float4 bzcxay = vec_vmrghw(vec_vmrglw(m_el[1].mVec, aoxyz), m_el[2].mVec);
		vec_float4 minus = vec_sub(cyazbx, bzcxay);	//cy - bz, az - cx, bx - ay
	
		if(tracev > btScalar(0.0f))
		{
			float s = btSqrt(tracev + btScalar(1.0f));
			float tmp3 = s * 0.5;
			vec_float4 vecs = vec_float4(0.5f / s);
	
			tmp = vec_madd(vecs, minus, btZeroVec());
			tmp = vec_insert(tmp3, tmp, 3);
		}
		else
		{
			int i = maxDiag[0] < maxDiag[1] ? ( (maxDiag[1] < maxDiag[2]) ? 2 : 1) : ( (maxDiag[0] < maxDiag[2]) ? 2 : 0);
			
			float s;
			
			vec_float4 plus = vec_add(cyazbx, bzcxay);	//(cy + bz, az + cx, bx + ay)
		
			vec_float4 pyzox = vec_sld(plus, plus, 4);
			if(i == 0)
			{
				s = btSqrt(maxDiag[0] - maxDiag[1] - maxDiag[2] + 1.0f);
				//tmpi, pz, py, mx
				tmp = vec_sld(vec_vmrghw(pyzox, plus), minus, 4);
			}
			else if(i ==  1)
			{
				s = btSqrt(maxDiag[1] - maxDiag[2] - maxDiag[0] + 1.0f);
				//pz, tmpi, px, my
				tmp = vec_sld(pyzox, vec_sld(minus, minus, 4), 4);
			} 
			else
			{
				s = btSqrt(maxDiag[2] - maxDiag[0] - maxDiag[1] + 1.0f);
				//py, px, tmpi, mz
				tmp = vec_sld(vec_sld(minus, minus, 12), vec_vmrghw(pyzox, plus), 8);		//mz o py px
				tmp = vec_sld(tmp, tmp, 8);
			}
			float tmpi = s * btScalar(0.5f);
			vec_float4 vecs = vec_float4(0.5f / s);
	
			tmp = vec_madd(vecs, tmp, btZeroVec());
			tmp = vec_insert(tmpi, tmp, i);
		}
	
		q.setValue(tmp);
#else
		btScalar trace = btVector3(m_el[0]).getX() + btVector3(m_el[1]).getY() + btVector3(m_el[2]).getZ();
		btScalar temp[4];

		if (trace > btScalar(0.0)) 
		{
			btScalar s = btSqrt(trace + btScalar(1.0));
			temp[3]=(s * btScalar(0.5));
			s = btScalar(0.5) / s;

			temp[0]=((btVector3(m_el[2]).getY() - btVector3(m_el[1]).getZ()) * s);
			temp[1]=((btVector3(m_el[0]).getZ() - btVector3(m_el[2]).getX()) * s);
			temp[2]=((btVector3(m_el[1]).getX() - btVector3(m_el[0]).getY()) * s);
		} 
		else 
		{
			int i = btVector3(m_el[0]).getX() < btVector3(m_el[1]).getY() ? 
				(btVector3(m_el[1]).getY() < btVector3(m_el[2]).getZ() ? 2 : 1) :
				(btVector3(m_el[0]).getX() < btVector3(m_el[2]).getZ() ? 2 : 0); 
			int j = (i + 1) % 3;  
			int k = (i + 2) % 3;
			
			btScalar s = btSqrt(m_el[i][i] - m_el[j][j] - m_el[k][k] + btScalar(1.0));
			temp[i] = s * btScalar(0.5);
			s = btScalar(0.5) / s;

			temp[3] = (m_el[k][j] - m_el[j][k]) * s;
			temp[j] = (m_el[j][i] + m_el[i][j]) * s;
			temp[k] = (m_el[k][i] + m_el[i][k]) * s;
		}
		q.setValue(temp[0],temp[1],temp[2],temp[3]);
#endif
	}

	/**@brief Get the matrix represented as euler angles around YXZ, roundtrip with setEulerYPR
	* @param yaw Yaw around Y axis
	* @param pitch Pitch around X axis
	* @param roll around Z axis */	
//jz: not used at all
#ifndef BT_PS3PPU_VEC		
	void getEulerYPR(btScalar& yaw, btScalar& pitch, btScalar& roll) const
	{
		// first use the normal calculus
		yaw = btScalar(btAtan2(m_el[1].x(), m_el[0].x()));
		pitch = btScalar(btAsin(-m_el[2].x()));
		roll = btScalar(btAtan2(m_el[2].y(), m_el[2].z()));

		// on pitch = +/-HalfPI
		if (btFabs(pitch)==SIMD_HALF_PI)
		{
			if (yaw>0)
				yaw-=SIMD_PI;
			else
				yaw+=SIMD_PI;

			if (roll>0)
				roll-=SIMD_PI;
			else
				roll+=SIMD_PI;
		}
	};

	/**@brief Get the matrix represented as euler angles around ZYX
	* @param yaw Yaw around X axis
	* @param pitch Pitch around Y axis
	* @param roll around X axis 
	* @param solution_number Which solution of two possible solutions ( 1 or 2) are possible values*/	
	void getEulerZYX(btScalar& yaw, btScalar& pitch, btScalar& roll, unsigned int solution_number = 1) const
	{
		//jz: not used at all
		struct Euler
		{
			btScalar yaw;
			btScalar pitch;
			btScalar roll;
		};

		Euler euler_out;
		Euler euler_out2; //second solution
		//get the pointer to the raw data

		// Check that pitch is not at a singularity
		if (btFabs(m_el[2].x()) >= 1)
		{
			euler_out.yaw = 0;
			euler_out2.yaw = 0;

			// From difference of angles formula
			btScalar delta = btAtan2(m_el[0].x(),m_el[0].z());
			if (m_el[2].x() > 0)  //gimbal locked up
			{
				euler_out.pitch = SIMD_PI / btScalar(2.0);
				euler_out2.pitch = SIMD_PI / btScalar(2.0);
				euler_out.roll = euler_out.pitch + delta;
				euler_out2.roll = euler_out.pitch + delta;
			}
			else // gimbal locked down
			{
				euler_out.pitch = -SIMD_PI / btScalar(2.0);
				euler_out2.pitch = -SIMD_PI / btScalar(2.0);
				euler_out.roll = -euler_out.pitch + delta;
				euler_out2.roll = -euler_out.pitch + delta;
			}
		}
		else
		{
			euler_out.pitch = - btAsin(m_el[2].x());
			euler_out2.pitch = SIMD_PI - euler_out.pitch;

			euler_out.roll = btAtan2(m_el[2].y()/btCos(euler_out.pitch), 
				m_el[2].z()/btCos(euler_out.pitch));
			euler_out2.roll = btAtan2(m_el[2].y()/btCos(euler_out2.pitch), 
				m_el[2].z()/btCos(euler_out2.pitch));

			euler_out.yaw = btAtan2(m_el[1].x()/btCos(euler_out.pitch), 
				m_el[0].x()/btCos(euler_out.pitch));
			euler_out2.yaw = btAtan2(m_el[1].x()/btCos(euler_out2.pitch), 
				m_el[0].x()/btCos(euler_out2.pitch));
		}

		if (solution_number == 1)
		{ 
			yaw = euler_out.yaw; 
			pitch = euler_out.pitch;
			roll = euler_out.roll;
		}
		else
		{ 
			yaw = euler_out2.yaw; 
			pitch = euler_out2.pitch;
			roll = euler_out2.roll;
		}
	}
#endif
	/**@brief Create a scaled copy of the matrix 
	* @param s Scaling vector The elements of the vector will scale each column */

	btMatrix3x3 scaled(const btVector3& s) const
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(142);
		return btMatrix3x3(vec_madd(s.mVec, m_el[0].mVec, btZeroVec()), vec_madd(s.mVec, m_el[1].mVec, btZeroVec()), vec_madd(s.mVec, m_el[2].mVec, btZeroVec()));
#else
		return btMatrix3x3(m_el[0].getX() * s.getX(), m_el[0].getY() * s.getY(), m_el[0].getZ() * s.getZ(),
			m_el[1].getX() * s.getX(), m_el[1].getY() * s.getY(), m_el[1].getZ() * s.getZ(),
			m_el[2].getX() * s.getX(), m_el[2].getY() * s.getY(), m_el[2].getZ() * s.getZ());
#endif
	}

	/**@brief Return the determinant of the matrix */
	btScalar            determinant() const;
	/**@brief Return the adjoint of the matrix */
	btMatrix3x3 adjoint() const;
	/**@brief Return the matrix with all values non negative */
	btMatrix3x3 absolute() const;
	/**@brief Return the transpose of the matrix */
	btMatrix3x3 transpose() const;
	/**@brief Return the inverse of the matrix */
	btMatrix3x3 inverse() const; 

	btMatrix3x3 transposeTimes(const btMatrix3x3& m) const;
	btMatrix3x3 timesTranspose(const btMatrix3x3& m) const;

#ifndef BT_PS3PPU_VEC
	SIMD_FORCE_INLINE btScalar tdotx(const btVector3& v) const 
	{
		return m_el[0].getX() * v.getX() + m_el[1].getX() * v.getY() + m_el[2].getX() * v.getZ();
	}
	SIMD_FORCE_INLINE btScalar tdoty(const btVector3& v) const 
	{
		return m_el[0].getY() * v.getX() + m_el[1].getY() * v.getY() + m_el[2].getY() * v.getZ();
	}
	SIMD_FORCE_INLINE btScalar tdotz(const btVector3& v) const 
	{
		return m_el[0].getZ() * v.getX() + m_el[1].getZ() * v.getY() + m_el[2].getZ() * v.getZ();
	}
#endif

	/**@brief diagonalizes this matrix by the Jacobi method.
	* @param rot stores the rotation from the coordinate system in which the matrix is diagonal to the original
	* coordinate system, i.e., old_this = rot * new_this * rot^T. 
	* @param threshold See iteration
	* @param iteration The iteration stops when all off-diagonal elements are less than the threshold multiplied 
	* by the sum of the absolute values of the diagonal, or when maxSteps have been executed. 
	* 
	* Note that this matrix is assumed to be symmetric. 
	*/
	void diagonalize(btMatrix3x3& rot, btScalar threshold, int maxSteps)
	{
#ifdef BT_PS3PPU_VEC_TOTEST4
		//btVector3::btHitPrint(143);
		float *el[3];//[4] __attribute__ ((aligned (16)));
		el[0] = m_el[0].m_floats;
		el[1] = m_el[1].m_floats;
		el[2] = m_el[2].m_floats;

		//printf("diagonalize before&after\n (%f%f%f)(%f%f%f)(%f%f%f)\n(%f%f%f)(%f%f%f)(%f%f%f)\n", m_el[0].mVec[0], m_el[0].mVec[1], m_el[0].mVec[2], m_el[1].mVec[0], m_el[1].mVec[1], m_el[1].mVec[2], m_el[2].mVec[0], m_el[2].mVec[1], m_el[2].mVec[2], el[0][0], el[0][1], el[0][2], el[1][0], el[1][1], el[1][2], el[2][0], el[2][1], el[2][2]); 

		rot.setIdentity();
		for (int step = maxSteps; step > 0; step--)
		{
			// find off-diagonal element [p][q] with largest magnitude
			int p = 0;
			int q = 1;
			int r = 2;
			btScalar max = btFabs(el[0][1]);
			btScalar v = btFabs(el[0][2]);
			if (v > max)
			{
				q = 2;
				r = 1;
				max = v;
			}
			v = btFabs(el[1][2]);
			if (v > max)
			{
				p = 1;
				q = 2;
				r = 0;
				max = v;
			}

			btScalar t = threshold * (btFabs(el[0][0]) + btFabs(el[1][1]) + btFabs(el[2][2]));
			if (max <= t)
			{
				if (max <= SIMD_EPSILON * t)
				{
					return;
				}
				step = 1;
			}

			// compute Jacobi rotation J which leads to a zero for element [p][q] 
			btScalar mpq = el[p][q];
			btScalar theta = (el[q][q] - el[p][p]) / (2 * mpq);
			btScalar theta2 = theta * theta;
			btScalar cos;
			btScalar sin;
			if (theta2 * theta2 < btScalar(10 / SIMD_EPSILON))
			{
				t = (theta >= 0) ? 1 / (theta + btSqrt(1 + theta2))
					: 1 / (theta - btSqrt(1 + theta2));
				cos = 1 / btSqrt(1 + t * t);
				sin = cos * t;
			}
			else
			{
				// approximation for large theta-value, i.e., a nearly diagonal matrix
				t = 1 / (theta * (2 + btScalar(0.5) / theta2));
				cos = 1 - btScalar(0.5) * t * t;
				sin = cos * t;
			}

			// apply rotation to matrix (this = J^T * this * J)
			el[p][q] = el[q][p] = 0;
			el[p][p] -= t * mpq;
			el[q][q] += t * mpq;
			btScalar mrp = el[r][p];
			btScalar mrq = el[r][q];
			btScalar rppr = cos * mrp - sin * mrq;
			m_el[r].set(p, rppr);
			m_el[p].set(r, rppr);
			btScalar rqqr = cos * mrq + sin * mrp;
			m_el[r].set(q, rqqr);
			m_el[q].set(r, rqqr);

			// apply rotation to rot (rot = rot * J)
			for (int i = 0; i < 3; i++)
			{
				btVector3 row = btVector3(rot[i]);
				float *srow = row.m_floats;
		
				mrp = srow[p];
				mrq = srow[q];
				srow[p] = cos * mrp - sin * mrq;
				srow[q] = cos * mrq + sin * mrp;

				rot[i] = row;
			}
		}
#else
		rot.setIdentity();
		for (int step = maxSteps; step > 0; step--)
		{
			// find off-diagonal element [p][q] with largest magnitude
			int p = 0;
			int q = 1;
			int r = 2;
			btScalar max = btFabs(m_el[0][1]);
			btScalar v = btFabs(m_el[0][2]);
			if (v > max)
			{
				q = 2;
				r = 1;
				max = v;
			}
			v = btFabs(m_el[1][2]);
			if (v > max)
			{
				p = 1;
				q = 2;
				r = 0;
				max = v;
			}

			btScalar t = threshold * (btFabs(m_el[0][0]) + btFabs(m_el[1][1]) + btFabs(m_el[2][2]));
			if (max <= t)
			{
				if (max <= SIMD_EPSILON * t)
				{
					return;
				}
				step = 1;
			}

			// compute Jacobi rotation J which leads to a zero for element [p][q] 
			btScalar mpq = m_el[p][q];
			btScalar theta = (m_el[q][q] - m_el[p][p]) / (2 * mpq);
			btScalar theta2 = theta * theta;
			btScalar cos;
			btScalar sin;
			if (theta2 * theta2 < btScalar(10 / SIMD_EPSILON))
			{
				t = (theta >= 0) ? 1 / (theta + btSqrt(1 + theta2))
					: 1 / (theta - btSqrt(1 + theta2));
				cos = 1 / btSqrt(1 + t * t);
				sin = cos * t;
			}
			else
			{
				// approximation for large theta-value, i.e., a nearly diagonal matrix
				t = 1 / (theta * (2 + btScalar(0.5) / theta2));
				cos = 1 - btScalar(0.5) * t * t;
				sin = cos * t;
			}

			// apply rotation to matrix (this = J^T * this * J)
			m_el[p][q] = m_el[q][p] = 0;
			m_el[p][p] -= t * mpq;
			m_el[q][q] += t * mpq;
			btScalar mrp = m_el[r][p];
			btScalar mrq = m_el[r][q];
			m_el[r][p] = m_el[p][r] = cos * mrp - sin * mrq;
			m_el[r][q] = m_el[q][r] = cos * mrq + sin * mrp;

			// apply rotation to rot (rot = rot * J)
			for (int i = 0; i < 3; i++)
			{
				btVector3& row = rot[i];
				mrp = row[p];
				mrq = row[q];
				row[p] = cos * mrp - sin * mrq;
				row[q] = cos * mrq + sin * mrp;
			}
		}
#endif
	}




	/**@brief Calculate the matrix cofactor 
	* @param r1 The first row to use for calculating the cofactor
	* @param c1 The first column to use for calculating the cofactor
	* @param r1 The second row to use for calculating the cofactor
	* @param c1 The second column to use for calculating the cofactor
	* See http://en.wikipedia.org/wiki/Cofactor_(linear_algebra) for more details
	*/
#ifndef BT_PS3PPU_VEC
	btScalar cofac(int r1, int c1, int r2, int c2) const 
	{
		return m_el[r1][c1] * m_el[r2][c2] - m_el[r1][c2] * m_el[r2][c1];
	}
#endif

	void	serialize(struct	btMatrix3x3Data& dataOut) const;

	void	serializeFloat(struct	btMatrix3x3FloatData& dataOut) const;

	void	deSerialize(const struct	btMatrix3x3Data& dataIn);

	void	deSerializeFloat(const struct	btMatrix3x3FloatData& dataIn);

	void	deSerializeDouble(const struct	btMatrix3x3DoubleData& dataIn);
};

SIMD_FORCE_INLINE btMatrix3x3& 
btMatrix3x3::operator*=(const btMatrix3x3& m)
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(144);
	btMatrix3x3 mt = transpose();
	
	vec_float4 tmp3, bdg, cfh, aei;
	bdg = cfh = aei = btZeroVec();
	for(int i = 0; i < 3; ++i)
	{
		aei = vec_madd(m.m_el[i].mVec, mt.m_el[i].mVec, aei);
		
		tmp3 = vec_vmrghw(m.m_el[i].mVec, m.m_el[i].mVec);
		bdg = vec_madd(vec_sld(tmp3, tmp3, 12), mt.m_el[i].mVec, bdg); 

		tmp3 = vec_sld(m.m_el[i].mVec, m.m_el[i].mVec, 4);
		tmp3 = vec_vmrghw(tmp3, tmp3);
		cfh = vec_madd(vec_sld(tmp3, tmp3, 8), mt.m_el[i].mVec, cfh);
	}
	
	////final composition
	tmp3 = vec_vmrghw(aei, cfh);	//acef
	vec_float4 tmp = vec_vmrghw(tmp3, bdg);
	
	m_el[0].mVec = tmp;
	m_el[1].mVec = vec_sld(tmp, vec_sld(tmp3, tmp3, 8), 12);
	m_el[2].mVec = vec_vmrglw(vec_sld(aei, vec_vmrglw(bdg, aei), 8), cfh);
#else
	setValue(m.tdotx(m_el[0]), m.tdoty(m_el[0]), m.tdotz(m_el[0]),
		m.tdotx(m_el[1]), m.tdoty(m_el[1]), m.tdotz(m_el[1]),
		m.tdotx(m_el[2]), m.tdoty(m_el[2]), m.tdotz(m_el[2]));
#endif
	return *this;
}

SIMD_FORCE_INLINE btScalar 
btMatrix3x3::determinant() const
{ 
#ifdef BT_PS3PPU_VEC_TOTEST4 
	//btVector3::btHitPrint(145);
	return btTriple(m_el[0], m_el[1], m_el[2]);
#else
	return btTriple((*this)[0], (*this)[1], (*this)[2]);
#endif
}


SIMD_FORCE_INLINE btMatrix3x3 
btMatrix3x3::absolute() const
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(146);
	return btMatrix3x3(
		vec_abs(m_el[0].mVec), vec_abs(m_el[1].mVec), vec_abs(m_el[2].mVec));
#else
	return btMatrix3x3(
		btFabs(m_el[0].getX()), btFabs(m_el[0].getY()), btFabs(m_el[0].getZ()),
		btFabs(m_el[1].getX()), btFabs(m_el[1].getY()), btFabs(m_el[1].getZ()),
		btFabs(m_el[2].getX()), btFabs(m_el[2].getY()), btFabs(m_el[2].getZ()));
#endif
}

SIMD_FORCE_INLINE btMatrix3x3 
btMatrix3x3::transpose() const 
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(147); //- otazne zrychleni - zalezi na kontextu?
	vec_float4 tmp = vec_vmrghw(m_el[0].mVec, m_el[2].mVec);	//ax cx ay cy
	return btMatrix3x3(vec_vmrghw(tmp, m_el[1].mVec), //ax bx cx by
							vec_vmrglw(tmp, vec_sld(m_el[1].mVec, m_el[1].mVec, 12)),	//ay by cy bz
							vec_vmrghw(vec_vmrglw(m_el[0].mVec, m_el[2].mVec), vec_sld(m_el[1].mVec, m_el[1].mVec,	8))); //az bz cz x
#else
	return btMatrix3x3(m_el[0].getX(), m_el[1].getX(), m_el[2].getX(),
		m_el[0].getY(), m_el[1].getY(), m_el[2].getY(),
		m_el[0].getZ(), m_el[1].getZ(), m_el[2].getZ());
#endif
}

SIMD_FORCE_INLINE btMatrix3x3 
btMatrix3x3::adjoint() const 
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(148);
	vec_float4 coxyz = vec_sld(m_el[2].mVec, m_el[2].mVec, 12);

	vec_float4 byzx = vec_sld(vec_sld(m_el[1].mVec, m_el[1].mVec, 12), m_el[1].mVec, 8);
	//cyzx = vec_sld(coxyz, m_el[2].mVec, 8)
	//czxy = vec_sld(coxyz, m_el[2].mVec, 12)
	vec_float4 icf = vec_madd(m_el[0].mVec, byzx, btZeroVec());
	vec_float4 gad = vec_madd(m_el[1].mVec, vec_sld(coxyz, m_el[2].mVec, 8), btZeroVec());
	vec_float4 ehb = vec_madd(m_el[0].mVec, vec_sld(coxyz, m_el[2].mVec, 12), btZeroVec());

	vec_float4 aoxyz = vec_sld(m_el[0].mVec, m_el[0].mVec, 12);

	//byzx = vec_sld(vec_sld(m_el[1].mVec, m_el[1].mVec, 12), m_el[1].mVec, 8)
	//ayzx = vec_sld(aoxyz, m_el[0].mVec, 8)
	//azxy = vec_sld(aoxyz, m_el[0].mVec, 12)
	icf = vec_sub(icf, vec_madd(m_el[1].mVec, vec_sld(aoxyz, m_el[0].mVec, 8), btZeroVec()));
	gad = vec_sub(gad, vec_madd(m_el[2].mVec, byzx, btZeroVec()));
	ehb = vec_sub(ehb, vec_madd(m_el[2].mVec, vec_sld(aoxyz, m_el[0].mVec, 12), btZeroVec()));

	vec_float4 tmp = vec_vmrghw(gad, icf);		//giac
	
	//printf("jz check it: sakra tohle se snad nikde nevola..takova pekna prace zrovna ... prozkoumat jestli jo a zkontrolovat spravnost");
	
	return btMatrix3x3(vec_vmrglw(tmp, ehb),
					vec_vmrghw(vec_vmrglw(gad, icf), ehb),
					vec_vmrghw(tmp, vec_sld(ehb, ehb, 4)));
#else
	return btMatrix3x3(cofac(1, 1, 2, 2), cofac(0, 2, 2, 1), cofac(0, 1, 1, 2),
		cofac(1, 2, 2, 0), cofac(0, 0, 2, 2), cofac(0, 2, 1, 0),
		cofac(1, 0, 2, 1), cofac(0, 1, 2, 0), cofac(0, 0, 1, 1));
#endif
}

SIMD_FORCE_INLINE btMatrix3x3 
btMatrix3x3::inverse() const
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(149);
	/*
	cox * s, (az * cy - ay * cz) * s, (ay * bz - az * by) * s		//A, B, C
	coy * s, (ax * cz - az * cx) * s, (az * bx - ax * bz) * s		//D, E, F
	coz * s, (ay * cx - ax * cy) * s, (ax * by - ay * bx) * s		//G, H, I
	*/

	vec_float4 coxyz = vec_sld(m_el[2].mVec, m_el[2].mVec, 12);
	vec_float4 czxy = vec_sld(coxyz, m_el[2].mVec, 12);
	vec_float4 co = vec_madd(m_el[1].mVec, vec_sld(coxyz, m_el[2].mVec, 8), btZeroVec());
	vec_float4 cooxyz = vec_sld(co, co, 12);
	co = vec_sld(cooxyz, co, 8);
	vec_float4 tmp = vec_madd(m_el[1].mVec, czxy, btZeroVec());
	cooxyz = vec_sld(tmp, tmp, 12);
	co = vec_sub(co, vec_sld(cooxyz, tmp, 12));

	tmp = vec_madd(m_el[0].mVec, co, btZeroVec());
	vec_float4 s = vec_re(vec_splat(vec_add(vec_add(tmp, vec_sld(tmp, tmp, 4)), vec_sld(tmp, tmp, 8)), 0));
	co = vec_madd(co, s, btZeroVec());	//A D G

	vec_float4 ehb = vec_madd(m_el[0].mVec, czxy, btZeroVec());
	tmp = vec_sld(m_el[1].mVec, m_el[1].mVec, 12);	//boxyz
	vec_float4 icf = vec_madd(m_el[0].mVec, vec_sld(tmp, m_el[1].mVec, 8), btZeroVec());

	tmp = vec_sld(m_el[0].mVec, m_el[0].mVec, 12); //aoxyz
	ehb = vec_madd(s, vec_sub(ehb, vec_madd(m_el[2].mVec, vec_sld(tmp, m_el[0].mVec, 12), btZeroVec())), btZeroVec());	
	icf = vec_madd(s, vec_sub(icf, vec_madd(m_el[1].mVec, vec_sld(tmp, m_el[0].mVec, 8), btZeroVec())), btZeroVec());	

	//final composition
	vec_float4 cfxi = vec_sld(icf, icf, 4);
	vec_float4 bxeh = vec_sld(ehb, ehb, 8);

	return btMatrix3x3(vec_vmrghw(vec_vmrghw(co, cfxi), bxeh),
						vec_vmrglw(vec_vmrghw(co, cfxi), bxeh),
						vec_vmrghw(vec_vmrglw(co, vec_sld(icf, icf, 8)), vec_sld(ehb, ehb, 4)));
#else
	btVector3 co(cofac(1, 1, 2, 2), cofac(1, 2, 2, 0), cofac(1, 0, 2, 1));
	btScalar det = (*this)[0].dot(co);
	btFullAssert(det != btScalar(0.0));
	btScalar s = btScalar(1.0) / det;
	return btMatrix3x3(co.getX() * s, cofac(0, 2, 2, 1) * s, cofac(0, 1, 1, 2) * s,
		co.getY() * s, cofac(0, 0, 2, 2) * s, cofac(0, 2, 1, 0) * s,
		co.getZ() * s, cofac(0, 1, 2, 0) * s, cofac(0, 0, 1, 1) * s);
#endif
}

SIMD_FORCE_INLINE btMatrix3x3 
btMatrix3x3::transposeTimes(const btMatrix3x3& m) const
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(150);
	vec_float4 aei, dhc, gbf;
	vec_float4 tmp;
	aei = dhc = gbf = btZeroVec();

	for(int i = 0; i < 3; ++i)
	{
		aei = vec_madd(m_el[i].mVec, m.m_el[i].mVec, aei);

		vec_float4 oxyz = vec_sld(m_el[i].mVec, m_el[i].mVec, 12);	//o x y z
		tmp = vec_sld(oxyz, m_el[i].mVec, 4);								//x y z x
		
		dhc = vec_madd(vec_sld(tmp, tmp, 4), m.m_el[i].mVec, dhc);			//ay*kx az*ky ax *kz x
		gbf = vec_madd(vec_vmrghw(vec_vmrglw(m_el[i].mVec, oxyz), m_el[i].mVec), m.m_el[i].mVec, gbf);
	}
	
	tmp = vec_sld(gbf, gbf, 8);
	return btMatrix3x3(vec_vmrghw(vec_vmrghw(aei, vec_sld(dhc, dhc, 8)), vec_sld(gbf, gbf, 4)),
							vec_vmrghw(vec_vmrghw(dhc, tmp), vec_sld(aei, aei, 4)),
							vec_vmrghw(vec_vmrglw(tmp, aei), vec_sld(dhc, dhc, 4)));
#else
	return btMatrix3x3(
		m_el[0].getX() * m[0].getX() + m_el[1].getX() * m[1].getX() + m_el[2].getX() * m[2].getX(),
		m_el[0].getX() * m[0].getY() + m_el[1].getX() * m[1].getY() + m_el[2].getX() * m[2].getY(),
		m_el[0].getX() * m[0].getZ() + m_el[1].getX() * m[1].getZ() + m_el[2].getX() * m[2].getZ(),
		m_el[0].getY() * m[0].getX() + m_el[1].getY() * m[1].getX() + m_el[2].getY() * m[2].getX(),
		m_el[0].getY() * m[0].getY() + m_el[1].getY() * m[1].getY() + m_el[2].getY() * m[2].getY(),
		m_el[0].getY() * m[0].getZ() + m_el[1].getY() * m[1].getZ() + m_el[2].getY() * m[2].getZ(),
		m_el[0].getZ() * m[0].getX() + m_el[1].getZ() * m[1].getX() + m_el[2].getZ() * m[2].getX(),
		m_el[0].getZ() * m[0].getY() + m_el[1].getZ() * m[1].getY() + m_el[2].getZ() * m[2].getY(),
		m_el[0].getZ() * m[0].getZ() + m_el[1].getZ() * m[1].getZ() + m_el[2].getZ() * m[2].getZ());
#endif
}

SIMD_FORCE_INLINE btMatrix3x3 
btMatrix3x3::timesTranspose(const btMatrix3x3& m) const
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(151);
	btMatrix3x3 mxtmp;
	btMatrix3x3 ret;
	for(int i = 0; i < 3; ++i)
	{
		mxtmp = btMatrix3x3(vec_madd(m_el[i].mVec, m.m_el[0].mVec, btZeroVec()),
											vec_madd(m_el[i].mVec, m.m_el[1].mVec, btZeroVec()),
											vec_madd(m_el[i].mVec, m.m_el[2].mVec, btZeroVec()));

		mxtmp = mxtmp.transpose();
	
		ret.m_el[i].mVec = vec_add(vec_add(mxtmp.m_el[0].mVec, mxtmp.m_el[1].mVec), mxtmp.m_el[2].mVec);
	}
	
	return ret;
#else
	return btMatrix3x3(
		m_el[0].dot(m[0]), m_el[0].dot(m[1]), m_el[0].dot(m[2]),
		m_el[1].dot(m[0]), m_el[1].dot(m[1]), m_el[1].dot(m[2]),
		m_el[2].dot(m[0]), m_el[2].dot(m[1]), m_el[2].dot(m[2]));
#endif
}

SIMD_FORCE_INLINE btVector3 
operator*(const btMatrix3x3& m, const btVector3& v) 
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(152);
	btMatrix3x3 tmp;
	tmp[0] = vec_madd(m[0].mVec, v.mVec, btZeroVec());
	tmp[1] = vec_madd(m[1].mVec, v.mVec, btZeroVec());
	tmp[2] = vec_madd(m[2].mVec, v.mVec, btZeroVec());

	tmp = tmp.transpose();

	return vec_add(vec_add(tmp[0].mVec, tmp[1].mVec), tmp[2].mVec);
#else
	return btVector3(m[0].dot(v), m[1].dot(v), m[2].dot(v));
#endif
}

SIMD_FORCE_INLINE btVector3
operator*(const btVector3& v, const btMatrix3x3& m)
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(153);
	btMatrix3x3 mt = m.transpose();

	btMatrix3x3 back = btMatrix3x3(
						vec_madd(mt[0].mVec, v.mVec, btZeroVec()),
						vec_madd(mt[1].mVec, v.mVec, btZeroVec()),
						vec_madd(mt[2].mVec, v.mVec, btZeroVec()));

	mt = back.transpose();
	return btVector3(vec_add(mt[2].mVec, vec_add(mt[0].mVec, mt[1].mVec)));
#else
	return btVector3(m.tdotx(v), m.tdoty(v), m.tdotz(v));
#endif
}

SIMD_FORCE_INLINE btMatrix3x3 
operator*(const btMatrix3x3& m1, const btMatrix3x3& m2)
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(154);
/*
m2					m1
ax ay az x		kx ky kz x
bx by bz x		lx ly lz x
cx cy cz x		mx my mz x

ax*kx + bx*ky + cx*kz, ay*kx + by*ky + cy*kz, az*kx + bz*ky + cz*kz	//A B C
ax*lx + bx*ly + cx*lz, ay*lx + by*ly + cy*lz, az*lx + bz*ly + cz*lz	//D E F
ax*mx + bx*my + cx*mz, ay*mx + by*my + cy*mz, az*mx + bz*my + cz*mz	//G H I
*/
	btMatrix3x3 mt = m1.transpose();
	
	/*	vec_madd(m2[0], mt[0]);	//axkx, aylx, azmx, x -> A0, E0, I0, x
		vec_madd(m2[1], mt[1]);	//bxky, byly, bzmy, x -> A1, E1, I1, x
		vec_madd(m2[2], mt[2]);	//cxkz, cylz, czmz, x -> A2, E2, I2, x	*/
	vec_float4 aei = vec_madd(m2[2].mVec, mt[2].mVec, vec_madd(m2[1].mVec, mt[1].mVec, vec_madd(m2[0].mVec, mt[0].mVec, btZeroVec())));	//A, E, I

	vec_float4 tmp3;
	vec_float4 bdg = btZeroVec();
	vec_float4 cfh = btZeroVec();
	for(int i = 0; i < 3; ++i)
	{
		tmp3 = vec_vmrghw(m2[i].mVec, m2[i].mVec);
		bdg = vec_madd(vec_sld(tmp3, tmp3, 12), mt[i].mVec, bdg); //B, D, G

		tmp3 = vec_sld(m2[i].mVec, m2[i].mVec, 4);
		tmp3 = vec_vmrghw(tmp3, tmp3);
		cfh = vec_madd(vec_sld(tmp3, tmp3, 8), mt[i].mVec, cfh);	//C, F, H
	}
	
	////final composition
	tmp3 = vec_vmrghw(aei, cfh);	//acef
	vec_float4 tmp = vec_vmrghw(tmp3, bdg);
	
	return btMatrix3x3(
		tmp,
		vec_sld(tmp, vec_sld(tmp3, tmp3, 8), 12),
		vec_vmrglw(vec_sld(aei, vec_vmrglw(bdg, aei), 8), cfh));
#else
	return btMatrix3x3(
		m2.tdotx( m1[0]), m2.tdoty( m1[0]), m2.tdotz( m1[0]),
		m2.tdotx( m1[1]), m2.tdoty( m1[1]), m2.tdotz( m1[1]),
		m2.tdotx( m1[2]), m2.tdoty( m1[2]), m2.tdotz( m1[2]));
#endif
}

/*
SIMD_FORCE_INLINE btMatrix3x3 btMultTransposeLeft(const btMatrix3x3& m1, const btMatrix3x3& m2) {
return btMatrix3x3(
m1[0][0] * m2[0][0] + m1[1][0] * m2[1][0] + m1[2][0] * m2[2][0],
m1[0][0] * m2[0][1] + m1[1][0] * m2[1][1] + m1[2][0] * m2[2][1],
m1[0][0] * m2[0][2] + m1[1][0] * m2[1][2] + m1[2][0] * m2[2][2],
m1[0][1] * m2[0][0] + m1[1][1] * m2[1][0] + m1[2][1] * m2[2][0],
m1[0][1] * m2[0][1] + m1[1][1] * m2[1][1] + m1[2][1] * m2[2][1],
m1[0][1] * m2[0][2] + m1[1][1] * m2[1][2] + m1[2][1] * m2[2][2],
m1[0][2] * m2[0][0] + m1[1][2] * m2[1][0] + m1[2][2] * m2[2][0],
m1[0][2] * m2[0][1] + m1[1][2] * m2[1][1] + m1[2][2] * m2[2][1],
m1[0][2] * m2[0][2] + m1[1][2] * m2[1][2] + m1[2][2] * m2[2][2]);
}
*/

/**@brief Equality operator between two matrices
* It will test all elements are equal.  */
SIMD_FORCE_INLINE bool operator==(const btMatrix3x3& m1, const btMatrix3x3& m2)
{
#ifdef BT_PS3PPU_VEC_TOTEST4
	//btVector3::btHitPrint(155);
	vec_float4 m10 = vec_sel(m1[0].mVec, m2[0].mVec, btSelVec3);
	vec_float4 m11 = vec_sel(m1[1].mVec, m2[1].mVec, btSelVec3);
	vec_float4 m12 = vec_sel(m1[2].mVec, m2[2].mVec, btSelVec3);
	return vec_all_eq(m10, m2[0].mVec) && vec_all_eq(m11, m2[1].mVec) && vec_all_eq(m12, m2[2].mVec);
#else
	return ( m1[0][0] == m2[0][0] && m1[1][0] == m2[1][0] && m1[2][0] == m2[2][0] &&
		m1[0][1] == m2[0][1] && m1[1][1] == m2[1][1] && m1[2][1] == m2[2][1] &&
		m1[0][2] == m2[0][2] && m1[1][2] == m2[1][2] && m1[2][2] == m2[2][2] );
#endif
}

///for serialization
#ifdef BT_PS3PPU_VEC
struct	btMatrix3x3FloatData
{
	//vec_float4 m_el[3];
	btVector3 m_el[3];
};
#else
struct	btMatrix3x3FloatData
{
	btVector3FloatData m_el[3];
};
#endif

///for serialization
struct	btMatrix3x3DoubleData
{
	btVector3DoubleData m_el[3];
};


	
SIMD_FORCE_INLINE	void	btMatrix3x3::serialize(struct	btMatrix3x3Data& dataOut) const
{
	for (int i=0;i<3;i++)
#ifdef BT_PS3PPU_VEC 
		dataOut.m_el[i] = m_el[i];
#else
		m_el[i].serialize(dataOut.m_el[i]);
#endif
}

SIMD_FORCE_INLINE	void	btMatrix3x3::serializeFloat(struct	btMatrix3x3FloatData& dataOut) const
{
	for (int i=0;i<3;i++)
#ifdef BT_PS3PPU_VEC 
		dataOut.m_el[i] = m_el[i];
#else
		m_el[i].serializeFloat(dataOut.m_el[i]);
#endif
}


SIMD_FORCE_INLINE	void	btMatrix3x3::deSerialize(const struct	btMatrix3x3Data& dataIn)
{
	for (int i=0;i<3;i++)
#ifdef BT_PS3PPU_VEC 
		m_el[i] = dataIn.m_el[i];
#else
		m_el[i].deSerialize(dataIn.m_el[i]);
#endif
}

SIMD_FORCE_INLINE	void	btMatrix3x3::deSerializeFloat(const struct	btMatrix3x3FloatData& dataIn)
{
	for (int i=0;i<3;i++)
#ifdef BT_PS3PPU_VEC 
		m_el[i] = dataIn.m_el[i];
#else
		m_el[i].deSerializeFloat(dataIn.m_el[i]);
#endif
}

SIMD_FORCE_INLINE	void	btMatrix3x3::deSerializeDouble(const struct	btMatrix3x3DoubleData& dataIn)
{
#ifndef BT_PS3PPU_VEC
	for (int i=0;i<3;i++)
		m_el[i].deSerializeDouble(dataIn.m_el[i]);
#else
	assert(0);
#endif

}


#endif //BT_MATRIX3x3_H
