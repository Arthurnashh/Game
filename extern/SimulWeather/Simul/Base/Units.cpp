#include "precomp.h"

#include "Simul/Base/Units.h"

using namespace simul::base;
using namespace PhysicalUnits;
namespace simul
{
	namespace base
	{
		namespace PhysicalUnits
		{
			const float RPM_TO_RAD_PER_SECOND=2*3.1415926536f/60.f;
			const float DEG_TO_RAD=3.1415926536f/180.f;
			const float MPH_TO_MS=(0.44704f);
			const float BHP_TO_KILOWATTS=(.7457f);
			using std::string;
			const char *UnitText[] = {"","Velocity","AngularVelocity","Torque","Power"};
			const char *CommonText[] = {"","mph","RPM","Nm","bhp"};

			const char *CommonUnits(Type t)
			{
				return CommonText[(int)t];
			}

			float RatioToStandard(const std::string &txt)
			{                         
				int pos=(int)txt.find("Percent");
				if(pos>=0)
					return 0.01f;
				pos=(int)txt.find("RPM");
				if(pos>=0)
					return RPM_TO_RAD_PER_SECOND;
				pos=(int)txt.find("rpm");
				if(pos>=0)
					return RPM_TO_RAD_PER_SECOND;
				pos=(int)txt.find("lbft");
				if(pos>=0)
					return LB_TO_NEWTONS*FEET_TO_METRES;
				pos=(int)txt.find("bhp");
				if(pos>=0)
					return BHP_TO_WATTS;
				pos=(int)txt.find("km/h");
				if(pos>=0)
					return KMH_TO_MS;
				pos=(int)txt.find("mph");
				if(pos>=0)
					return MPH_TO_MS;
				pos=(int)txt.find("deg");
				if(pos>=0)
					return DEG_TO_RAD;
				pos=(int)txt.find("degrees");
				if(pos>=0)
					return DEG_TO_RAD;
				return 1.f;
			}
		}
	}
}
