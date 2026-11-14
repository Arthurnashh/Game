#ifndef SIMUL_META_XYINTERFACE_H
#define SIMUL_META_XYINTERFACE_H

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
// Graph
// 

//! An interface for a 2D object - x and y co-ordinates.
			class XYInterface
			{
			public:
				virtual float GetX()=0;
				virtual float GetY()=0;
				virtual float GetXVelocity()=0;
				virtual float GetYVelocity()=0;
				virtual float GetWidth()=0;
				virtual float GetHeight()=0;
				virtual void SetXVelocity(float xv)=0;
				virtual void SetYVelocity(float xv)=0;
				virtual float GetRotation()=0;
			};
// 
		}
	}
}

#endif // ifndef SIMUL_META_XYINTERFACE_H

