#include "OrientedObject.h"
using namespace simul;
using namespace graph;
using namespace meta;
using namespace standardnodes;
OrientedObject::OrientedObject(const char *what_name):Group()
{
	if(what_name)
		name=what_name;
}

OrientedObject::~OrientedObject()
{
}

bool OrientedObject::TimeStep(float dt)
{
	simul::math::Vector3 pos=frame.GetPosition();
	pos+=dt*frame.GetVelocity();
	frame.SetPosition(pos);
	return true;
}
//NameInterface
const char *OrientedObject::GetNameAsCString()
{
	return name.c_str();
}

const float *OrientedObject::GetOrientationAsPermanentMatrix() const
{
	return frame.T4.RowPointer(0);
}

const float *OrientedObject::GetRotationAsQuaternion() const
{
	return (const float *)(&frame.q.x);
}

const float *OrientedObject::GetPosition() const
{
	return frame.GetPosition();
}
void OrientedObject::SetOrientationAsMatrix(const float *m)
{
	frame.T4=m;
	frame.Define(frame.T4);
}
void OrientedObject::SetOrientationAsQuaternion(const float *q)
{
	frame.Define(simul::math::Quaternion(q));
}
void OrientedObject::SetPosition(const float *x)
{
	frame.SetPosition(x);
}
void OrientedObject::SetPositionAsXYZ(float x,float y,float z)
{
	frame.SetPosition(simul::math::Vector3(x,y,z));
}
void OrientedObject::Rotate(float angle,const float *axis)
{
	frame.Rotate(angle,axis);
}
const float *OrientedObject::GetVelocity() const
{
	return frame.GetVelocity();
}
const float *OrientedObject::GetAngularVelocity() const
{
	return frame.GetAngularVelocity();
}
const float *OrientedObject::GetAcceleration() const
{
	static float a[]={0,0,0,0};
	return a;
}

void OrientedObject::SetVelocityAsXYZ(float vx,float vy,float vz)
{
	frame.SetVe(simul::math::Vector3(vx,vy,vz));
}

void OrientedObject::SetVelocity(const float *v)
{
	frame.SetVe(v);
}