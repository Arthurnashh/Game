#pragma once

#include "Simul\Graph\meta\Group.h"
#include "Simul\Graph\StandardNodes\RigidBodyInterface.h"
#include "Simul\Graph\StandardNodes\Export.h"
#include "Simul\Geometry\ReferenceFrame.h"
#include "Simul\Graph\StandardNodes\NameInterface.h"
#include "Simul\Graph\StandardNodes\AnimatedInterface.h"
#include <string>

#ifdef _MSC_VER
// C4251 "class '...' needs to have dll-interface to be used by clients of ..."
// This warning can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			//! A node that has an orientation (position and rotation).
			class SIMUL_STD_EXPORT OrientedObject
				:public simul::graph::meta::Group
				,public simul::graph::standardnodes::RigidBodyInterface
				,public simul::graph::standardnodes::NameInterface
			{
				std::string name;
			public:
				OrientedObject(const char *what_name=NULL);
				virtual ~OrientedObject();
				// Node
				virtual bool TimeStep(float dt);
				//NameInterface
				const char *GetNameAsCString();
				// RigidBodyInterface
				const float *GetOrientationAsPermanentMatrix() const;		//! Permanent: this means that for as long as the interface exists, the address is valid.
				const float *GetRotationAsQuaternion() const;
				const float *GetPosition() const;
				void SetOrientationAsMatrix(const float *);
				void SetOrientationAsQuaternion(const float *);
				void SetPosition(const float *);
				void SetPositionAsXYZ(float,float,float);
				void Rotate(float,const float *);
				const float *GetVelocity() const;
				const float *GetAngularVelocity() const;
				const float *GetAcceleration() const;
				void SetVelocityAsXYZ(float vx,float vy,float vz);
				void SetVelocity(const float *);
			protected:
				simul::geometry::SimulReferenceFrame frame;
			};
		}
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif