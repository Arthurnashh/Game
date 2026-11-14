#ifndef SIMUL_GRAPH_CAMERA_CAMERA_H
#define SIMUL_GRAPH_CAMERA_CAMERA_H
#include "Simul/Graph/Meta/Node.h"
#include "Simul/Graph/StandardNodes/OrientationInterface.h"
#include "Simul/Geometry/Orientation.h"
#include "Simul/Graph/Camera/CameraInterface.h"
#include "Simul/Graph/Camera/Export.h"

namespace simul
{
	namespace graph
	{
		namespace camera
		{
			//! A camera class. The orientation has the z-axis facing backwards, the x-axis right and the y-axis up.
			SIMUL_CAMERA_EXPORT_CLASS Camera :
				public simul::graph::meta::Node,
				public simul::graph::standardnodes::OrientationInterface,
				public simul::graph::camera::CameraInterface
			{
			public:
				Camera();
				~Camera();
				META_BeginStates
				META_EndStates
				META_BeginProperties
					META_ValueRangeProperty(float,HorizontalFieldOfViewInRadians,3.1416f*90.f	,0,3.1416f*179.f,"Horizontal Field Of View In Radians")
					META_ValueRangeProperty(float,VerticalFieldOfViewInRadians	,0				,0,3.1416f*179.f,"Vertical Field Of View In Radians")
					META_ReferenceProperty(simul::geometry::SimulOrientation	,Orientation	,"Orientation")
				META_EndProperties
				Macro_EnableRecursiveStreaming(simul::graph::meta::Node)
				Macro_Node(Camera)
				// virtual from OrientationInterface
				virtual const float *GetOrientationAsPermanentMatrix() const;		//! Permanent: this means that for as long as the interface exists, the address is valid.
				virtual const float *GetRotationAsQuaternion() const;
				virtual const float *GetPosition() const;

				virtual const float *MakeViewMatrix(bool gl_style=false) const;
				virtual const float *MakeProjectionMatrix(float zMin,float zMax,bool left_handed=false) const;

				virtual void SetOrientationAsMatrix(const float *);
				virtual void SetOrientationAsQuaternion(const float *);
				virtual void SetPosition(const float *);
				virtual void LookInDirection(const float *view_dir,const float *view_up);
				virtual void LookInDirection(const float *view_dir);
				virtual void SetPositionAsXYZ(float,float,float);
				virtual void LocalMove(const float *);
				virtual void Rotate(float,const float *);
				virtual void LocalRotate(float,const float *);

				virtual bool TimeStep(float delta_t);
				//
				float GetHorizontalFieldOfViewDegrees() const;
				void SetHorizontalFieldOfViewDegrees(float f);
				float GetVerticalFieldOfViewDegrees() const;
				void SetVerticalFieldOfViewDegrees(float f);
			};
		}
	}
}
#endif
