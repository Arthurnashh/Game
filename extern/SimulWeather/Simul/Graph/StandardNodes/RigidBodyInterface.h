#ifndef SIMUL_STD_RIGIDBODYINTERFACE_H
#define SIMUL_STD_RIGIDBODYINTERFACE_H
#include "OrientationInterface.h"
namespace simul
{
	namespace graph
	{
			namespace standardnodes
	{
// Graph
// 
		class RigidBodyInterface:public OrientationInterface
		{
		public:
			virtual const float *GetVelocity() const=0;
			virtual const float *GetAngularVelocity() const=0;
			virtual const float *GetAcceleration() const=0;
			virtual void SetVelocity(const float *)=0;
			virtual void SetVelocityAsXYZ(float,float,float)=0;
		};
// 

	/*
	To fully implement this interface, you will need:
		const float *GetVelocity();
		const float *GetAngularVelocity();
		const float *GetAcceleration();
		const float *GetOrientationAsPermanentMatrix();
		const float *GetRotationAsQuaternion();
		const float *GetPosition();
		void SetOrientationAsMatrix(const float *);
		void SetPosition(const float *);
		void SetPositionAsXYZ(float,float,float);
		void SetVelocity(const float *);
		void SetVelocityAsXYZ(float,float,float);
	*/
		}
	}
}
#endif