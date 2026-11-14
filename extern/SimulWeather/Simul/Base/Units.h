#pragma once
#include <string>
#include "Simul/Base/Export.h"
namespace simul
{
	namespace base
	{
		#define LB_TO_NEWTONS (4.448f)
		#define FEET_TO_METRES (12.f*2.54f/100.f)
		#define BHP_TO_WATTS (745.7f)      
		#define KMH_TO_MS (0.277777778f)
		namespace PhysicalUnits
		{
			extern SIMUL_BASE_EXPORT const float BHP_TO_KILOWATTS; 
			extern SIMUL_BASE_EXPORT const float RPM_TO_RAD_PER_SECOND;
			extern SIMUL_BASE_EXPORT const float DEG_TO_RAD;
			extern SIMUL_BASE_EXPORT const float MPH_TO_MS;
			typedef enum {None,Velocity,AngularVelocity,Torque,Power} Type;
			SIMUL_BASE_EXPORT const char *CommonUnits(Type t);
			extern SIMUL_BASE_EXPORT float RatioToStandard(const std::string &txt);
		}
	}
}
