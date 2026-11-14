#ifndef SIMUL_GRAPH_CAMERA_CAMERAINTERFACE_H
#define SIMUL_GRAPH_CAMERA_CAMERAINTERFACE_H
#include "Simul/Graph/Camera/Export.h"

namespace simul
{
	namespace math
	{
		class Vector3;
	}
	namespace graph
	{
		namespace camera
		{
			SIMUL_CAMERA_EXPORT_CLASS CameraInterface
			{
			public:
				virtual float GetHorizontalFieldOfViewDegrees() const=0;
				virtual void SetHorizontalFieldOfViewDegrees(float f)=0;
				virtual float GetVerticalFieldOfViewDegrees() const=0;
				virtual void SetVerticalFieldOfViewDegrees(float f)=0;

				virtual const float *MakeViewMatrix(bool gl_style) const=0;
				virtual const float *MakeProjectionMatrix(float zMin,float zMax,bool left_handed=false) const=0;

				virtual void LookInDirection(const float *view_dir,const float *view_up)=0;
				virtual void LookInDirection(const float *view_dir)=0;

				static float GetMaximumOfHorizontalFieldOfViewDegrees() {return 165.f;}
				static float GetMinimumOfHorizontalFieldOfViewDegrees() {return 0;}
				static float GetMaximumOfVerticalFieldOfViewDegrees() {return 165.f;}
				static float GetMinimumOfVerticalFieldOfViewDegrees() {return 0;}
			};
		}
	}
}

#endif