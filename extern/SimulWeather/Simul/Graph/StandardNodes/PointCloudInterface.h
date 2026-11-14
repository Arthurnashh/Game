#ifndef SIMUL_META_POINTCLOUD_INTERFACE_H
#define SIMUL_META_POINTCLOUD_INTERFACE_H
#include "Simul/Math/Vector3.h"
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			//! An interface for an object that can store a list of points.
			class PointCloudInterface
			{
			public:
				//! The number of points to draw this frame.
				virtual unsigned GetPointCount() const=0;
				virtual const float *GetPointPosition(unsigned) const=0;
				virtual const float *GetPointColour(unsigned) const=0;
				virtual float GetPointSize(unsigned) const=0;
				virtual bool UseSprites() const=0;
				virtual const char *GetSpriteFilename() const=0;
				virtual unsigned GetSpriteSize()const=0;
				virtual unsigned GetNumTriangles()const=0;
				virtual const unsigned *GetTriangle(unsigned)const=0;
				virtual unsigned GetNumTrianglePoints()=0;
				virtual const float *GetTrianglePointPosition(unsigned)=0;
			};
			class LineCloudInterface
			{
			public:
				virtual unsigned GetLineCount() const =0;
				virtual const float *GetLinePositions(unsigned) const =0;
			};
		}
	}
}
#endif