#include "precomp.h"

#include "Simul/Graph/Camera/Camera.h"
#include "Simul/Math/Quaternion.h"
#include "Simul/Math/Pi.h"
#include <memory.h>
using namespace simul;
using namespace math;
using namespace graph::camera;
static const float DEG_TO_RAD=pi/180.f;
static const float RAD_TO_DEG=180.f/pi;
Camera::Camera():Orientation()
{
	InitializePropertiesDefinition();
	VerticalFieldOfViewInRadians=60.f*pi/180.f;
	HorizontalFieldOfViewInRadians=0;
	Orientation.Rotate(3.14f/2.f,simul::math::Vector3(1,0,0));
}

Camera::~Camera()
{
}

const float *Camera::MakeViewMatrix(bool ) const
{
	static Matrix4x4 M;
	Orientation.T4.Inverse(M);
/*	if(gl_style)
	{
		Matrix4x4 T(1.f,0.f,0.f,0.f,
					0.f,0.f,1.f,0.f,
					0.f,1.f,0.f,0.f,
					0.f,0.f,0.f,1.f);
		static Matrix4x4 M2;
		Multiply4x4(M2,T,M);
		return M2.RowPointer(0);
	}*/
	return M.RowPointer(0);
}

const float *Camera::MakeProjectionMatrix(float zNear,float zFar,bool left_handed) const
{
// According to the documentation for D3DXMatrixPerspectiveFovLH:
//	xScale     0          0               0
//	0        yScale       0               0
//	0          0       zf/(zf-zn)         1
//	0          0       -zn*zf/(zf-zn)     0
//	where:
//	yScale = cot(fovY/2)
//
//	xScale = yScale / aspect ratio
	float h=HorizontalFieldOfViewInRadians;
	float v=VerticalFieldOfViewInRadians;
	if(!h)
		h=v;
	if(!v)
		v=h;
	float xScale= 1.f/tan(h/2.f);
	float yScale = 1.f/tan(v/2.f);

	static float m[16];
	memset(m,0,16*sizeof(float));
	m[0]=xScale;
					m[4+1]=yScale;
	if(left_handed)
	{
									m[8+2]=		zFar/(zFar-zNear);		m[8+3]=1.f;
									m[12+2]=	-zNear*zFar/(zFar-zNear);
															
	}
	else
	{
									m[8+2]=		zFar/(zNear-zFar);		m[8+3]=-1.f;
									m[12+2]=	zNear*zFar/(zNear-zFar);
	}
	return m;
}

	// virtual from OrientationInterface
const float *Camera::GetOrientationAsPermanentMatrix() const
{
	return (const float *)(&Orientation.T4);
}

const float *Camera::GetRotationAsQuaternion() const
{
	static simul::math::Quaternion q;
	simul::math::MatrixToQuaternion(q,Orientation.T4);
	return (const float *)(&q);
}

const float *Camera::GetPosition() const
{ 
	return (const float *)(&Orientation.T4.GetRowVector(3));
}

void Camera::SetOrientationAsMatrix(const float *m)
{
	Orientation.T4=m;
}

void Camera::SetOrientationAsQuaternion(const float *q)
{
	Orientation.Define(simul::math::Quaternion(q));
}

void Camera::SetPosition(const float *x)
{
	Orientation.SetPosition(simul::math::Vector3(x));
}

void Camera::SetPositionAsXYZ(float x,float y,float z)
{
	Orientation.SetPosition(simul::math::Vector3(x,y,z));
}

void Camera::LocalMove(const float *x)
{
	Orientation.LocalTranslate(x);
}
				
void Camera::Rotate(float x,const float *a)
{
	Orientation.Rotate(x,a);
}		
void Camera::LocalRotate(float x,const float *a)
{
	Orientation.LocalRotate(x,a);
}

bool Camera::TimeStep(float )
{
	return true;
}

float Camera::GetHorizontalFieldOfViewDegrees() const
{
	return HorizontalFieldOfViewInRadians*RAD_TO_DEG;
}

void Camera::SetHorizontalFieldOfViewDegrees(float f)
{
	HorizontalFieldOfViewInRadians=f*DEG_TO_RAD;
}

float Camera::GetVerticalFieldOfViewDegrees() const
{
	return VerticalFieldOfViewInRadians*RAD_TO_DEG;
}

void Camera::SetVerticalFieldOfViewDegrees(float f)
{
	VerticalFieldOfViewInRadians=f*DEG_TO_RAD;
}

void Camera::LookInDirection(const float *view_dir,const float *view_up)
{
	Vector3 d(view_dir);
	d.Normalize();
	Vector3 u(view_up);
	Vector3 x=d^u;
	x.Normalize();
	u=x^d;
	Orientation.DefineFromYZ(u,-d);
}

void Camera::LookInDirection(const float *view_dir)
{
	Vector3 d(view_dir);
	d.Normalize();
	Vector3 x=d^Vector3(0,1.f,0);
	x.Normalize();
	Vector3 u=x^d;
	Orientation.DefineFromYZ(u,-d);
}
