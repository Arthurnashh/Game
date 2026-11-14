#include "precomp.h"

#define SIM_MATH
#include "Quaternion.h"
//---------------------------------------------------------------------------     
#ifdef WIN32
	//#define SIMD
#endif

#ifdef PLAYSTATION_2
	#define PLAYSTATION2
#endif        

#include <math.h>

using namespace simul;
using namespace math;

Quaternion::Quaternion()
{
	Reset();
}
Quaternion::Quaternion(float S,float X,float Y,float Z,bool normalize)
{
	x=X;
	y=Y;
	z=Z;
	s=S;
	if(normalize)
		this->MakeUnit();
}

Quaternion::Quaternion(const Quaternion &q)
{
	s=q.s;
	x=q.x;
	y=q.y;
	z=q.z;
}
Quaternion::Quaternion(float ss,const Vector3 &vv)
{
	Define(ss,vv);
}
Quaternion::Quaternion(const float *q)
{
	x=q[0];
	y=q[1];
	z=q[2];
	s=q[3];
}
//------------------------------------------------------------------------------
void Quaternion::Reset()
{
	x=0.f;
	y=0.f;
	z=0.f;
	s=1.f;
}
//------------------------------------------------------------------------------
void Quaternion::MakeUnit()
{
	float magnitude;
	magnitude=sqrt((x*x+y*y+z*z+s*s));
	if(magnitude==0)
		return;
	x/=magnitude;
	y/=magnitude;
	z/=magnitude;
	s/=magnitude;
}
//------------------------------------------------------------------------------
void Quaternion::DefineSmall(float ss,const Vector3 &vv)
{
#ifndef PLAYSTATION2
	ss*=0.5f;
	x=ss*vv(0);
	y=ss*vv(1);
	z=ss*vv(2);
	s=1.f;
	float magnitude;
	magnitude=sqrt((x*x+y*y+z*z+s*s));
	if(magnitude==0)
		return;
	x/=magnitude;
	y/=magnitude;
	z/=magnitude;
	s/=magnitude;
#else
	ss;vv;
		asm __volatile__("
			.set noreorder
			add t1,zero,2
			mtc1 t1,$f12
			add t1,zero,1
			mtc1 t1,$f4				// f4 = 1
			lwc1 $f10,0(%1)			// ss   
			//mtc1 %1,$f10		

			add t0,zero,%0			// vv.Values
			cvt.s.w	$f12,$f12
			div.s $f10,$f10,$f12	// f10 = ss/2
			cvt.s.w	$f4,$f4
			
			lwc1		$f1,0(t0)	// load v1[X]
			lwc1		$f2,4(t0)	// load v1[Y]
			lwc1		$f3,8(t0)	// load v1[Z]
			
			mul.s $f1,$f1,$f10
			mul.s $f2,$f2,$f10
			mul.s $f3,$f3,$f10
			
			mula.s	$f1,$f1			// ACC = v1[X] * v2[X]
			madda.s	$f2,$f2			// ACC += v1[Y] * v2[Y]
			madda.s	$f3,$f3			// ACC += v1[Y] * v2[Y]
			madd.s	$f5,$f4,$f4		// rv = ACC + v1[Z] * v2[Z]
			sqrt.s	$f6,$f5
			div.s $f1,$f6
			div.s $f2,$f6
			div.s $f3,$f6
			div.s $f4,$f6
			swc1 $f1, 0(a0)			// store v1[X]
			swc1 $f2, 4(a0)			// store v1[Y]
			swc1 $f3, 8(a0)			// store v1[Z]
			swc1 $f4, 12(a0)			// store v1[Z]
		"					:
							: "r" (vv.Values),"g" (&ss)
							: );
#endif
}
float Quaternion::AngleInDirection(const Vector3 &vv) const
{
	float dp=vv*Vector3(x,y,z);
	float halfangle=asin(dp);
	return halfangle*2.f;
}
float Quaternion::Angle() const
{
	float halfangle=acos(s);
	return halfangle*2.f;
}
void Quaternion::Define(float angle,const Vector3 &vv)
{
#if 1//ndef SIMD
#ifndef PLAYSTATION2
	angle/=2.f;
	s=cos(angle);
	float ss=sin(angle);
	x=ss*vv(0);
	y=ss*vv(1);
	z=ss*vv(2);
#else
	ss*=0.5f;
	s=cosf(ss);
	ss=sinf(ss);
	x=ss*vv(0);
	y=ss*vv(1);
	z=ss*vv(2);
	/*s=0.5f*ss;
	asm __volatile__("
		.set noreorder
		lqc2 vf1,0(%0)
		lqc2 vf2,0(%2)
		vmulw vf1,vf2,vf1w
		sqc2 vf1,0(%0)
		.set reorder
		"					: 
							: "g" (this),"g"(&ss),"g"(&vv)
							: );
	s=1.f;
	MakeUnit();*/
#endif
#else
	_asm
	{
		mov eax,vv
		movaps xmm1,[eax]
		mov ebx,this
		//	mov ecx,[ebp+12]
		fld [ebp+12]//[ebp+12]
		mov ecx,2
		//push ecx
		fidiv [esp]
		fsincos
		fxch st(1)
		fstp [esp]		// ss=sin(angle/2)
		movss xmm2,[esp]	//
		shufps xmm2,xmm2,0		// fill all 4 values in xmm2 with sin(..)
		mulps xmm1,xmm2
		movups [ebx],xmm1	// x,y,z = v cos(angle/2), s=0 probably
		fstp [ebx+12]		// s=cos(..)   
		//pop ecx
	};
#endif
}
//------------------------------------------------------------------------------
void Quaternion::Define(const float ss,const float xx,const float yy,const float zz)
{
	s=ss;
	x=xx;
	y=yy;
	z=zz;
}

Quaternion Quaternion::operator*(const Quaternion &q) const
{
	static Quaternion r;
	r.s=s*q.s-x*q.x-y*q.y-z*q.z;
	r.x=q.s*x+s*q.x+y*q.z-z*q.y;
	r.y=q.s*y+s*q.y+z*q.x-x*q.z;
	r.z=q.s*z+s*q.z+x*q.y-y*q.x;
	return r;
}

Quaternion Quaternion::operator/(const Quaternion &q) const
{
	Quaternion iq=q;
	iq.x*=-1.f;
	iq.y*=-1.f;
	iq.z*=-1.f;
	return (*this)*iq;
}          
Quaternion& Quaternion::operator/=(const Quaternion &q)
{
#ifndef PLAYSTATION2
#ifndef SIMD
	register float X,Y,Z;
	X=s*q.x+q.s*x+q.y*z-q.z*y;
	Y=s*q.y+q.s*y+q.z*x-q.x*z;
	Z=s*q.z+q.s*z+q.x*y-q.y*x;
	s=q.s*s-q.x*x-q.y*y-q.z*z;

	x=X;
	y=Y;
	z=Z;
#else               
	/*register float X,Y,Z;

	X=s*q.x+q.s*x+q.y*z-q.z*y;
	Y=s*q.y+q.s*y+q.z*x-q.x*z;
	Z=s*q.z+q.s*z+q.x*y-q.y*x;
	s=q.s*s-q.x*x-q.y*y-q.z*z;

	x=X;
	y=Y;
	z=Z;   */
// (X,Y,Z,s) => s*q + q.s*this
//			 +  (z,x,y,0)*q.(y,z,x,0) - (y,z,x,0)*q.(z,x,y,0)
//			s -= this*q
// Alternatively:    
// (X,Y,Z,s) => (s,s,s,s)*q + q.(s,s,s,0)*this
//			 +  (z,x,y,0)*q.(y,z,x,0) - (y,z,x,0)*q.(z,x,y,0)
//			s -= (x,y,z).q(x,y,z)
	//static float temp[4];
	//static int mask[]={0,0,0,0xFFFFFFFF};
	//int *mmm=mask;
	//float *tt=temp;
	_asm
	{
	mov eax,this
	mov ebx,q
//	mov edx,mmm
//mov ecx,tt
	movups xmm1,[eax]
	movups xmm2,[ebx]
	movaps xmm3,xmm1
	movaps xmm4,xmm2
	shufps xmm3,xmm3,255	// xmm3 -> s,s,s,s
//movups [ecx],xmm3
	shufps xmm4,xmm4,255			// xmm4 -> q.s,q.s,q.s,q.s
//movups [ecx],xmm4
	mulps xmm3,xmm2       
//movups [ecx],xmm3
	mulps xmm4,xmm1      
//movups [ecx],xmm4
	addps xmm3,xmm4			// xmm3 = total so far, includes scalar multiplied terms.
//movups [ecx],xmm3
	movaps xmm5,xmm1
	movaps xmm6,xmm2   
	movaps xmm7,xmm1
	mulps xmm7,xmm2	  
//movups [ecx],xmm7
	shufps xmm1,xmm1,18		// 2,0,1
	shufps xmm2,xmm2,9		// 1,2,0
	shufps xmm5,xmm5,9		// 1,2,0
	shufps xmm6,xmm6,18		// 2,0,1
	mulps xmm1,xmm2
	mulps xmm5,xmm6
	addps xmm3,xmm1
	subps xmm3,xmm5			// xmm3 contains all terms except the dot product terms.
//movups [ecx],xmm3
	movaps xmm6,xmm7
	shufps xmm6,xmm6,9
//movups [ecx],xmm6
	addps xmm7,xmm6
//movups [ecx],xmm7
	shufps xmm6,xmm6,1
//movups [ecx],xmm6
	addps xmm7,xmm6
//movups [ecx],xmm7
	movss xmm0,[eax+12]
//movups [ecx],xmm0
	mulss xmm0,[ebx+12]
//movups [ecx],xmm0
	subss xmm0,xmm7
//movups [ecx],xmm0
	movups [eax],xmm3		// store 1st 3
	movss [eax+12],xmm0
	};
#endif
#else
	//float *rVal=&(x);
	const float *Q1=&(x);
	const float *Q2=&(q.x);
	asm __volatile__("
			.set noreorder
		lqc2			vf1,0(%1)			// Load v1's float address into vf1
		lqc2			vf2,0(%2)			// Load v2's float address into vf1
		vmul            vf4,vf1,vf2			// xx yy zz ww
		vsubaz          ACC,vf4,vf4z		// ACC.w is now vf4.w - vf4.z, which is ww-zz
		vmsubay         ACC,vf0,vf4y		// ACC.w is now ACC.w*vf0.w - vf4.y, which is ww - zz - yy
		vmsubx	       	vf3,vf0,vf4x		// vf3.w is now ACC.w*vf0.w - vf4.x, which is ww - zz - yy - xx
		vopmula.xyz     ACC,vf2,vf1   		// Start Outerproduct
		vmaddaw       	ACC,vf1,vf2w		// Add w2.xyz1
		vmaddaw			ACC,vf2,vf1w		// Add w1.xyz2
		vopmsub.xyz     vf3,vf1,vf2  		// Finish Outerproduct
		sqc2			vf3,0(%0)			// Load vf3 into r's float address
		.set reorder
		"					: /* Outputs. */
							: /* Inputs */ "r" (this),"r" (Q1), "r" (Q2)
							: /* Clobber */ "$vf1");
#endif
	return *this;
}
namespace simul
{
	namespace math
	{
		void Multiply(Quaternion &r,const Quaternion &q1,const Quaternion &q2)
		{
		#ifndef PLAYSTATION2
			r.s=q1.s*q2.s-q1.x*q2.x-q1.y*q2.y-q1.z*q2.z;
			r.x=q2.s*q1.x+q1.s*q2.x+q1.y*q2.z-q1.z*q2.y;
			r.y=q2.s*q1.y+q1.s*q2.y+q1.z*q2.x-q1.x*q2.z;
			r.z=q2.s*q1.z+q1.s*q2.z+q1.x*q2.y-q1.y*q2.x;
		#else
			//float *rVal=&(r.x);
			const float *Q1=&(q1.x);
			const float *Q2=&(q2.x);
			asm __volatile__("
					.set noreorder
				lqc2			vf1,0(%1)			// Load v1's float address into vf1
				lqc2			vf2,0(%2)			// Load v2's float address into vf1
				vmul            vf4,vf1,vf2			// xx yy zz ww
				vsubaz          ACC,vf4,vf4z		// ACC.w is now vf4.w - vf4.z, which is ww-zz
				vmsubay         ACC,vf0,vf4y		// ACC.w is now ACC.w*vf0.w - vf4.y, which is ww - zz - yy
				vmsubx	       	vf3,vf0,vf4x		// vf3.w is now ACC.w*vf0.w - vf4.x, which is ww - zz - yy - xx
				vopmula.xyz     ACC,vf1,vf2   		// Start Outerproduct
				vmaddaw       	ACC,vf1,vf2w		// Add w2.xyz1
				vmaddaw			ACC,vf2,vf1w		// Add w1.xyz2
				vopmsub.xyz     vf3,vf2,vf1  		// Finish Outerproduct
				sqc2			vf3,0(%0)			// Load vf3 into r's float address
				"					: /* Outputs. */
									: /* Inputs */  "r" (&r),"r" (Q1), "r" (Q2)
									: /* Clobber */ "$vf1");
		#endif
		}           
		//------------------------------------------------------------------------------
		void MultiplyNegativeByQuaternion(Quaternion &r,const Quaternion &q1,const Quaternion &q2)
		{
			r.s=q1.s*q2.s +q1.x*q2.x+q1.y*q2.y+q1.z*q2.z;
			r.x=-q2.s*q1.x+q1.s*q2.x-q1.y*q2.z+q1.z*q2.y;
			r.y=-q2.s*q1.y+q1.s*q2.y-q1.z*q2.x+q1.x*q2.z;
			r.z=-q2.s*q1.z+q1.s*q2.z-q1.x*q2.y+q1.y*q2.x;
		}
		void MultiplyByNegative(Quaternion &r,const Quaternion &q1,const Quaternion &q2)
		{
		#ifndef PLAYSTATION2
			r.s=q1.s*q2.s+q1.x*q2.x+q1.y*q2.y+q1.z*q2.z;
			r.x=q2.s*q1.x-q1.s*q2.x-q1.y*q2.z+q1.z*q2.y;
			r.y=q2.s*q1.y-q1.s*q2.y-q1.z*q2.x+q1.x*q2.z;
			r.z=q2.s*q1.z-q1.s*q2.z-q1.x*q2.y+q1.y*q2.x;
		#else
			//float *rVal=&(r.x);
			const float *Q1=&(q1.x);
			const float *Q2=&(q2.x);
			asm __volatile__("
					.set noreorder
				lqc2			vf1,0(%1)			// Load v1's float address into vf1
				lqc2			vf2,0(%2)			// Load v2's float address into vf1
				vmul            vf4,vf1,vf2			// xx yy zz ww
				vsubaz          ACC,vf4,vf4z		// ACC.w is now vf4.w - vf4.z, which is ww-zz
				vmsubay         ACC,vf0,vf4y		// ACC.w is now ACC.w*vf0.w - vf4.y, which is ww - zz - yy
				vmsubx	       	vf3,vf0,vf4x		// vf3.w is now ACC.w*vf0.w - vf4.x, which is ww - zz - yy - xx
				vopmula.xyz     ACC,vf1,vf2   		// Start Outerproduct
				vmaddaw       	ACC,vf1,vf2w		// Add w2.xyz1
				vmaddaw			ACC,vf2,vf1w		// Add w1.xyz2
				vopmsub.xyz     vf3,vf2,vf1  		// Finish Outerproduct
				sqc2			vf3,0(%0)			// Load vf3 into r's float address
				"					: /* Outputs. */
									: /* Inputs */  "r" (&r),"r" (Q1), "r" (Q2)
									: /* Clobber */ "$vf1");
		#endif
		}
		// v must be a 3-vector, ret must be a 3-vector.
		void Multiply(Vector3 &ret,const Quaternion &q,const Vector3 &v)
		{
			const float &x0=v.Values[0];
			const float &y0=v.Values[1];
			const float &z0=v.Values[2];
			float s1=q.x*x0+q.y*y0+q.z*z0;
			float x1=q.s*x0+q.y*z0-q.z*y0;
			float y1=q.s*y0+q.z*x0-q.x*z0;
			float z1=q.s*z0+q.x*y0-q.y*x0;
			ret.Values[0]=s1*q.x+q.s*x1+q.y*z1-q.z*y1;
			ret.Values[1]=s1*q.y+q.s*y1+q.z*x1-q.x*z1;
			ret.Values[2]=s1*q.z+q.s*z1+q.x*y1-q.y*x1;
		}             
		// v must be a 3-vector, ret must be a 3-vector.
		void AddQuaternionTimesVector(Vector3 &ret,const Quaternion &q,const Vector3 &v)
		{
			const float &x0=v.Values[0];
			const float &y0=v.Values[1];
			const float &z0=v.Values[2];
			float s1=q.x*x0+q.y*y0+q.z*z0;
			float x1=q.s*x0+q.y*z0-q.z*y0;
			float y1=q.s*y0+q.z*x0-q.x*z0;
			float z1=q.s*z0+q.x*y0-q.y*x0;
			ret.Values[0]+=s1*q.x+q.s*x1+q.y*z1-q.z*y1;
			ret.Values[1]+=s1*q.y+q.s*y1+q.z*x1-q.x*z1;
			ret.Values[2]+=s1*q.z+q.s*z1+q.x*y1-q.y*x1;
		}
		void Divide(Vector3 &ret,const Quaternion &q,const Vector3 &v)
		{
			const float &x0=v.Values[0];
			const float &y0=v.Values[1];
			const float &z0=v.Values[2];
			static float s1;
			s1=-q.x*x0-q.y*y0-q.z*z0;
			static float x1;
			x1= q.s*x0-q.y*z0+q.z*y0;
			static float y1;
			y1= q.s*y0-q.z*x0+q.x*z0;
			static float z1;
			z1= q.s*z0-q.x*y0+q.y*x0;
			ret.Values[0]=-s1*q.x+q.s*x1-q.y*z1+q.z*y1,
			ret.Values[1]=-s1*q.y+q.s*y1-q.z*x1+q.x*z1,
			ret.Values[2]=-s1*q.z+q.s*z1-q.x*y1+q.y*x1;
		}
		//------------------------------------------------------------------------------
		void FPUQuaternionToMatrix(Matrix4x4 &M,const Quaternion &q)
		{
			#define X q.x
			#define Y q.y
			#define Z q.z
			#define S q.s
			M.Values[    0]=X*X-Y*Y-Z*Z+S*S;       
			M.Values[    1]=2*(X*Y-Z*S);      // -!!    
			M.Values[    2]=2*(X*Z+Y*S);      // +!!

			M.Values[  4  ]=2*(X*Y+Z*S);      // +
			M.Values[  4+1]=-X*X+Y*Y-Z*Z+S*S;       
			M.Values[  4+2]=2*(Y*Z-X*S);      // was -!!

			M.Values[2*4  ]=2*(X*Z-Y*S);      // -
			M.Values[2*4+1]=2*(Y*Z+X*S);      // was + !!
			M.Values[2*4+2]=-X*X-Y*Y+Z*Z+S*S;
			#undef X
			#undef Y
			#undef Z
			#undef S
		}               
		//------------------------------------------------------------------------------
		#if 0
		void QuaternionToMatrix(Matrix4x4 &M,Quaternion &q)
		{
		#ifndef PLAYSTATION2
		#ifndef SIMD
		#ifdef CHECK_MATRIX_BOUNDS
			if(M.Width!=3||M.Height!=3)
				throw BadSize();
		#endif
			#define X q.x
			#define Y q.y
			#define Z q.z
			#define S q.s
			M.Values[    0]=X*X-Y*Y-Z*Z+S*S;       
			M.Values[    1]=2*(X*Y-Z*S);      // -!!    
			M.Values[    2]=2*(X*Z+Y*S);      // +!!

			M.Values[  4  ]=2*(X*Y+Z*S);      // +     
			M.Values[  4+1]=-X*X+Y*Y-Z*Z+S*S;       
			M.Values[  4+2]=2*(Y*Z-X*S);      // was -!!

			M.Values[2*4  ]=2*(X*Z-Y*S);      // -
			M.Values[2*4+1]=2*(Y*Z+X*S);      // was + !!
			M.Values[2*4+2]=-X*X-Y*Y+Z*Z+S*S;
			#undef X
			#undef Y
			#undef Z
			#undef S
		#else
			static float root2=1.4142135623730950488016887242097f;
			float *r2=&root2;
			//static float temp[4];
			//float *tt=temp;
			_asm
			{
		// put root 2 into xmm4:
				xorps xmm4,xmm4
				mov eax,r2
				movss xmm4,[eax]
				shufps xmm4,xmm4,192	// r2,r2,r2,0
		// M Row 3 is: 2Z (X,Y,Z) + S (-2Y,2X,0) - (0,0,dotp-S*S)
				mov ebx,M
				mov ebx,[ebx]
				mov eax,dword ptr[q]
				movups xmm0,[eax]
		// 1st make the dot product: S squared - v.v
				movaps xmm5,xmm0
		//movups [edx],xmm5
				mulps xmm5,xmm5           
		//movups [edx],xmm5
				movaps xmm2,xmm5	// make a copy
				shufps xmm2,xmm2,249// shift left 1,2,3,3 = 1+8+48+192 = 249
		//movups [edx],xmm2
				addps xmm5,xmm2		// x*x+y*y
				shufps xmm2,xmm2,249// shift left
				addps xmm5,xmm2		// x*x+y*y+z*z
				shufps xmm2,xmm2,249// shift left
				subps xmm5,xmm2		// x*x+y*y+z*z-s*s
		//movups [edx],xmm5
		// Multiply everything by root two:
				mulps xmm0,xmm4     // root2 * (x,y,z,0)
		//movups [edx],xmm0
		// Make the XXX, YYY, ZZZ:
				movaps xmm1,xmm0
				shufps xmm1,xmm1,192	// /2 X,X,X,0    
		//movups [edx],xmm1
				movaps xmm2,xmm0
				shufps xmm2,xmm2,213	// /2 Y,Y,Y,0
		//movups [edx],xmm2
				movaps xmm3,xmm0
				shufps xmm3,xmm3,234	// /2 Z,Z,Z,0
		//movups [edx],xmm3
				mulps xmm1,xmm0		// 2X(X,Y,Z,0)
				mulps xmm2,xmm0		// 2Y(X,Y,Z,0)
				mulps xmm3,xmm0		// 2Z(X,Y,Z,0)
				movaps [ebx   ],xmm1
				movaps [ebx+16],xmm2
				movaps [ebx+32],xmm3
				movaps xmm1,xmm0
				movaps xmm2,xmm0
				movaps xmm3,xmm0
				xorps xmm0,xmm0
				addss xmm0,[eax+12]	// (S,0,0,0)  
		//movups [edx],xmm0
				shufps xmm0,xmm0,97	// (0,S,0,0)    
		//movups [edx],xmm0
				subss xmm0,[eax+12]	// (-S,S,0,0)
		//movups [edx],xmm0
				mulps xmm0,xmm4     // root2 * (-S,S,0,0)
		//movups [edx],xmm0
		// Row 3 first
				shufps xmm3,xmm3,241// /2(Y,X,0,0) 1,0,3,3
		//movups [edx],xmm3
				mulps xmm3,xmm0		// S(-2Y,2X,0,0)
		//movups [edx],xmm3

				shufps xmm0,xmm0,211// root2 (0,-S,S,0) 3,0,1,2 = 3+16+192
		//movups [edx],xmm0
				shufps xmm1,xmm1,219// /2(0,Z,Y,0) 3,2,1,3 = 3+8+16+192 = 219
		//movups [edx],xmm1
				mulps xmm1,xmm0		// S(0,-2Z,2Y,0)
		//movups [edx],xmm1
				shufps xmm0,xmm0,18	// /2(S,0,-S,0) 2,0,1,0
		//movups [edx],xmm0
				shufps xmm2,xmm2,206// /2(Z,0,X,0) 2,3,0,3
		//movups [edx],xmm2
				mulps xmm2,xmm0
		//movups [edx],xmm2

				addps xmm1,[ebx   ]
		//movups [edx],xmm1
				addps xmm2,[ebx+16]
		//movups [edx],xmm2
				addps xmm3,[ebx+32]
		//movups [edx],xmm3
		//movups [edx],xmm1
				subss xmm1,xmm5
		//movups [edx],xmm1
				movaps [ebx   ],xmm1
				movaps [ebx+16],xmm2
				movaps [ebx+32],xmm3
				movss xmm1,[ebx+20]
		//movups [edx],xmm1
				subss xmm1,xmm5
		//movups [edx],xmm1
				movss [ebx+20],xmm1
				movss xmm1,[ebx+40]
		//movups [edx],xmm1
				subss xmm1,xmm5
		//movups [edx],xmm1
				movss [ebx+40],xmm1
			};
			//int stophere=1;
			//stophere;
		#endif
		#else
			float *MVal0=M.Values;
			float *MVal1=&(M.Values[4]);
			float *MVal2=&(M.Values[8]);
			//float *MVal3=&(M.Values[12]);
			const float *Q1=&q.x;
			asm __volatile__("
					.set noreorder
				//loi			1.414213562
				add t2,zero,0x3FB504F3					//1.414213562f = 0x3FB504F3
				ctc2 t2,$vi21							// Transfer t2's contents to the I register. And wait, because of latency...
				lqc2		vf1,0(%3)					// Load v1's float address into vf1
				vmula		ACC,  vf1, vf1           	// xx yy zz
				vsub		vf10, vf00, vf00			// Set matrix line 0 to 0
				vsub		vf11, vf00, vf00			//
				vsub		vf12, vf00, vf00			// Set matrix line 2 to 0
				//vmove		vf13, vf00					// Set bottom line to 0 0 0 1

				vmadd		vf6,vf1,vf1					// 2xx       2yy       2zz
				vaddw		vf5,vf0,vf0					// 1         1         1         (2)

				vmuli		vf2,vf1, I               	// x sqrt2 y sqrt2 z sqrt2 w sqrt2  SHOULD have the right value in the I register by now!
				vopmula.xyz	ACC,vf2,vf2           		// 2yz       2xz       2xy       -
				vmsubw.xyz	vf3,vf2,vf2					// 2yz-2xw   2xz-2yz   2xy-2zw   -
				vmaddw.xyz	vf4,vf2,vf2					// 2yz+2xw   2xz+2yz   2xy+2zw   -
				vaddaw.xyz	ACC,vf0,vf00				// 1         1         1         -
				vmsubax.yz	ACC,vf5,vf6					// 1         1-2xx     1-2xx

				vmsuby.z	vf12,vf5,vf6        		// -         -         1-2xx-2yy -
				vmsubay.x	ACC, vf5,vf6				// 1-2yy     1-2xx     1-2xx-2yy -
				vmsubz.y	vf11,vf5,vf6				// -         1-2xx-2zz -         -
				vmr32.y		vf10,vf3
				vmsubz.x	vf10,vf5,vf6				// 1-2yy-2zz -         -         -
				vmr32.x		vf12,vf3
				vaddy.z		vf10,vf00,vf4
				vmr32.w		vf2, vf3
				vmr32.z		vf11,vf2
				vaddx.y		vf12,vf00,vf4
				vmr32.y		vf4,vf4
				vmr32.x		vf11,vf4

				sqc2	vf10,0(%0)
				sqc2	vf11,0(%1)
				sqc2	vf12,0(%2)
			//		sqc2	vf13,0(%3)
				"					: /* Outputs. */
									: /* Inputs */ "r" (MVal0), "r" (MVal1), "r" (MVal2),"r"(Q1)
									: /* Clobber */ "$vf1","t2","$f1");
		#endif
		}
		#endif
		//------------------------------------------------------------------------------
		void QuaternionToMatrix(Matrix4x4 &M,const Quaternion &q)
		{
		#if 1//ndef PLAYSTATION2
		#ifndef SIMD
			#define X (q.x)
			#define Y (q.y)
			#define Z (q.z)
			#define S (q.s)
			M.Values[    0]=X*X-Y*Y-Z*Z+S*S;//    = (1-2*(Y*Y+Z*Z));= (X*X+Y*Y+Z*Z+S*S-2*(Y*Y+Z*Z))  // X*X+Y*Y+Z*Z+S*S=1.      
			M.Values[    1]=2*(X*Y+Z*S);      // +     
			M.Values[    2]=2*(X*Z-Y*S);      // -

			M.Values[  4+0]=2*(X*Y-Z*S);      // -!!  
			M.Values[  4+1]=-X*X+Y*Y-Z*Z+S*S;        
			M.Values[  4+2]=2*(Y*Z+X*S);      // was + !!

			M.Values[2*4+0]=2*(X*Z+Y*S);      // +!!
			M.Values[2*4+1]=2*(Y*Z-X*S);      // was -!!
			M.Values[2*4+2]=-X*X-Y*Y+Z*Z+S*S;
			#undef X
			#undef Y
			#undef Z
			#undef S
		#else
			static float root2=1.4142135623730950488016887242097f;
			float *r2=&root2;
			//static float temp[4];
			//float *tt=temp;
			_asm
			{
		// put root 2 into xmm4:
				xorps xmm4,xmm4
				mov eax,r2
				movss xmm4,[eax]
				shufps xmm4,xmm4,192	// r2,r2,r2,0
		// M Row 3 is: 2Z (X,Y,Z) + S (-2Y,2X,0) - (0,0,dotp-S*S)
				mov ebx,M
				mov eax,dword ptr[q]
				movups xmm0,[eax]
		// 1st make the dot product: S squared - v.v
				movaps xmm5,xmm0	// X Y Z S
		//movups [edx],xmm5
				mulps xmm5,xmm5     // X2 Y2 Z2 S2
		//movups [edx],xmm5
				movaps xmm2,xmm5	// make a copy
				shufps xmm2,xmm2,249// shift left 1,2,3,3 = 1+8+48+192 = 249	xmm2 = Y2 Z2 S2 S2
		//movups [edx],xmm2
				addps xmm5,xmm2		// X*X+Y*Y , Y*Y+Z*Z, Z*Z+S*S, 2 S*S			
				shufps xmm2,xmm2,249// shift left								xmm2 = Z*Z, S*S, S*S, S*S
				addps xmm5,xmm2		// X*X+Y*Y+Z*Z , Y*Y+Z*Z+S*S,  Z*Z+2 S*S, 3 S*S
				shufps xmm2,xmm2,249// shift left								xmm2 = S*S, S*S, S*S, S*S
				subps xmm5,xmm2		// x*x+y*y+z*z-s*s
		//movups [edx],xmm5
		// Multiply everything by root two:
				mulps xmm0,xmm4     // root2 * (x,y,z,0)
		//movups [edx],xmm0
		// Make the XXX, YYY, ZZZ:
				movaps xmm1,xmm0
				shufps xmm1,xmm1,192	// /2 X,X,X,0    
		//movups [edx],xmm1
				movaps xmm2,xmm0
				shufps xmm2,xmm2,213	// /2 Y,Y,Y,0
		//movups [edx],xmm2
				movaps xmm3,xmm0
				shufps xmm3,xmm3,234	// /2 Z,Z,Z,0
		//movups [edx],xmm3
				mulps xmm1,xmm0		// 2X(X,Y,Z,0)
				mulps xmm2,xmm0		// 2Y(X,Y,Z,0)
				mulps xmm3,xmm0		// 2Z(X,Y,Z,0)
				movaps [ebx   ],xmm1
				movaps [ebx+16],xmm2
				movaps [ebx+32],xmm3
				movaps xmm1,xmm0
				movaps xmm2,xmm0
				movaps xmm3,xmm0
				xorps xmm0,xmm0
				subss xmm0,[eax+12]	// (-S,0,0,0)  //was addss     //
		//movups [edx],xmm0                                        //
				shufps xmm0,xmm0,97	// (0,-S,0,0)                  //
		//movups [edx],xmm0                                        //    Sign change for transpose
				addss xmm0,[eax+12]	// (S,-S,0,0)                  //
		//movups [edx],xmm0                                        //
				mulps xmm0,xmm4     // root2 * (S,-S,0,0)          //
		//movups [edx],xmm0
		// Row 3 first
				shufps xmm3,xmm3,241// /2(Y,X,0,0) 1,0,3,3
		//movups [edx],xmm3
				mulps xmm3,xmm0		// S(-2Y,2X,0,0)
		//movups [edx],xmm3

				shufps xmm0,xmm0,211// root2 (0,-S,S,0) 3,0,1,2 = 3+16+192
		//movups [edx],xmm0
				shufps xmm1,xmm1,219// /2(0,Z,Y,0) 3,2,1,3 = 3+8+16+192 = 219
		//movups [edx],xmm1
				mulps xmm1,xmm0		// S(0,-2Z,2Y,0)
		//movups [edx],xmm1
				shufps xmm0,xmm0,18	// /2(S,0,-S,0) 2,0,1,0
		//movups [edx],xmm0
				shufps xmm2,xmm2,206// /2(Z,0,X,0) 2,3,0,3
		//movups [edx],xmm2
				mulps xmm2,xmm0
		//movups [edx],xmm2

				addps xmm1,[ebx   ]
		//movups [edx],xmm1
				addps xmm2,[ebx+16]
		//movups [edx],xmm2
				addps xmm3,[ebx+32]
		//movups [edx],xmm3
		//movups [edx],xmm1
				subss xmm1,xmm5
		//movups [edx],xmm1
				movaps [ebx   ],xmm1
				movaps [ebx+16],xmm2
				movaps [ebx+32],xmm3
				movss xmm1,[ebx+20]
		//movups [edx],xmm1
				subss xmm1,xmm5
		//movups [edx],xmm1
				movss [ebx+20],xmm1
				movss xmm1,[ebx+40]
		//movups [edx],xmm1
				subss xmm1,xmm5
		//movups [edx],xmm1
				movss [ebx+40],xmm1
			};
			//int stophere=1;
			//stophere;
		#endif
		#else
			float *MVal0=M.Values;
			float *MVal1=&(M.Values[4]);
			float *MVal2=&(M.Values[8]);
			//float *MVal3=&(M.Values[12]);
			const float *Q1=&q.x;
			asm __volatile__("
					.set noreorder
				//loi			1.414213562
				add t2,zero,0x3FB504F3					//1.414213562f = 0x3FB504F3
				ctc2 t2,$vi21							// Transfer t2's contents to the I register. And wait, because of latency...
				lqc2		vf1,0(%3)					// Load v1's float address into vf1
				vmula		ACC,  vf1, vf1           	// xx yy zz
				vsub		vf10, vf00, vf00			// Set matrix line 0 to 0
				vsub		vf11, vf00, vf00			//
				vsub		vf12, vf00, vf00			// Set matrix line 2 to 0
				//vmove		vf13, vf00					// Set bottom line to 0 0 0 1

				vmadd		vf6,vf1,vf1					// 2xx       2yy       2zz
				vaddw		vf5,vf0,vf0					// 1         1         1         (2)

				vmuli		vf2,vf1, I               	// x sqrt2 y sqrt2 z sqrt2 w sqrt2  SHOULD have the right value in the I register by now!
				vopmula.xyz	ACC,vf2,vf2           		// 2yz       2xz       2xy       -
				vmsubw.xyz	vf3,vf2,vf2					// 2yz-2xw   2xz-2yz   2xy-2zw   -
				vmaddw.xyz	vf4,vf2,vf2					// 2yz+2xw   2xz+2yz   2xy+2zw   -
				vaddaw.xyz	ACC,vf0,vf00				// 1         1         1         -
				vmsubax.yz	ACC,vf5,vf6					// 1         1-2xx     1-2xx

				vmsuby.z	vf12,vf5,vf6        		// -         -         1-2xx-2yy -
				vmsubay.x	ACC, vf5,vf6				// 1-2yy     1-2xx     1-2xx-2yy -
				vmsubz.y	vf11,vf5,vf6				// -         1-2xx-2zz -         -
				vmr32.y		vf10,vf3
				vmsubz.x	vf10,vf5,vf6				// 1-2yy-2zz -         -         -
				vmr32.x		vf12,vf3
				vaddy.z		vf10,vf00,vf4
				vmr32.w		vf2, vf3
				vmr32.z		vf11,vf2
				vaddx.y		vf12,vf00,vf4
				vmr32.y		vf4,vf4
				vmr32.x		vf11,vf4

				sqc2	vf10,0(%0)
				sqc2	vf11,0(%1)
				sqc2	vf12,0(%2)
			//		sqc2	vf13,0(%3)
				"					: /* Outputs. */
									: /* Inputs */ "r" (MVal0), "r" (MVal1), "r" (MVal2),"r"(Q1)
									: /* Clobber */ "$vf1","t2","$f1");
		#endif
		}
		void MatrixToQuaternion(Quaternion &q,const Matrix4x4 &M)
		{
			Matrix4x4 Transp;
			M.Transpose(Transp);
		//    Calculate the trace of the matrix T from the equation:
			  float Tr=Transp(0,0)+Transp(1,1)+Transp(2,2)+1;
		//    If the trace of the matrix is greater than zero
			if(Tr>0)
			{           
				float Size=.5f/sqrt(Tr);
				q.s=0.25f/ Size;
				q.x=-(Transp(1,2)-Transp(2,1))*Size;
				q.y=-(Transp(2,0)-Transp(0,2))*Size;
				q.z=-(Transp(0,1)-Transp(1,0))*Size;
			}
			else
			{
				float  r[4];
				int    i, j, k;
				int nxt[3] = {1, 2, 0};
			 // diagonal is negative
				i = 0;
				if (Transp(1,1) > Transp(0,0))
					i = 1;
				if (Transp(2,2) > Transp(i,i))
					i = 2;
				j = nxt[i];
				k = nxt[j];
				float Size = sqrt ((Transp(i,i) - (Transp(j,j) + Transp(k,k))) + 1.0f);
				r[i] = Size * 0.5f;
				if (Size != 0.0f)
					Size = 0.5f / Size;
				r[j] = (Transp(i,j) + Transp(j,i)) * Size;
				r[k] = (Transp(i,k) + Transp(k,i)) * Size;
				r[3] = (Transp(j,k) - Transp(k,j)) * Size;

				q.x = -r[0];
				q.y = -r[1];
				q.z = -r[2];
				q.s = r[3];
			}
			q.MakeUnit();
		}

		void Slerp(Quaternion &ret,const Quaternion &q1,const Quaternion &q2,float l)
		{
			// v0 and v1 should be unit length or else
			// something broken will happen.

			// Compute the cosine of the angle between the two vectors.
			Quaternion Q2=q2;
			float dot = q1.x*q2.x+q1.y*q2.y+q1.z*q2.z;
			if(dot<0)
			{
				dot=-dot;
				Q2.x*=-1.f;
				Q2.y*=-1.f;
				Q2.z*=-1.f;
				Q2.s*=-1.f;
			}
			dot+=q1.s*Q2.s;
			
			const float DOT_THRESHOLD = 0.9995f;
			if (dot > DOT_THRESHOLD)
			{
				// If the inputs are too close for comfort, linearly interpolate
				// and normalize the result.
				ret.x=q1.x+l*(Q2.x - q1.x);
				ret.y=q1.y+l*(Q2.y-q1.y);
				ret.z=q1.z+l*(Q2.z-q1.z);
				ret.s=q1.s+l*(Q2.s-q1.s);
				ret.MakeUnit();
				return;
			}
			if(dot<-1.f)
				dot=-1.f;
			if(dot>1.f)
				dot=1.f;
			float theta_0 = acos(dot);  // theta_0 = half angle between input vectors
			float theta1 = theta_0*(1.f-l);    // theta = angle between v0 and result 
			float theta2 = theta_0*l;    // theta = angle between v0 and result 

			float s1=sin(theta1);
			float s2=sin(theta2);
			float ss=sin(theta_0);
			/*Quaternion q3;
			q3.x=Q2.x-dot*(q1.x);
			q3.y=Q2.y-dot*(q1.y);
			q3.z=Q2.z-dot*(q1.z);
			q3.s=Q2.s-dot*(q1.s);
			q3.MakeUnit();              // { v0, v2 } is now an orthonormal basis*/

			ret.x=(q1.x*s1+Q2.x*s2)/ss;
			ret.y=(q1.y*s1+Q2.y*s2)/ss;
			ret.z=(q1.z*s1+Q2.z*s2)/ss;
			ret.s=(q1.s*s1+Q2.s*s2)/ss;
			ret.MakeUnit();          
		}

	}
}
