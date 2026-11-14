#ifndef SIMUL_OVERCASTCALLBACK_H
#define SIMUL_OVERCASTCALLBACK_H

#include "Simul/Sky/Export.h"

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif
namespace simul
{
	namespace sky
	{
		class OvercastCallback
		{
		public:
			virtual void GetOvercastData(float daytime,float &overcast,float &overcast_base_km,float &overcast_scale_km)=0;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif