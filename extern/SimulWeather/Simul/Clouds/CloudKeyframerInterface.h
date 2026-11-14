#ifndef SIMUL_CLOUDS_CLOUDKEYFRAMERINTERFACE_H
#define SIMUL_CLOUDS_CLOUDKEYFRAMERINTERFACE_H
#include "Simul/Clouds/Export.h"
#include "Simul/Clouds/CloudRenderCallback.h"
#include "Simul/Base/PropertyMacros.h"
namespace simul
{
	namespace sky
	{
		class SkyInterface;
	}
	namespace clouds
	{
		class CloudInterface;
		//! A virtual interface for cloud keyframers.
		SIMUL_CLOUDS_EXPORT_CLASS CloudKeyframerInterface
		{
		public:
			//! Set the object that the keyframer will send texture data to.
			virtual void SetRenderCallback(CloudRenderCallback *cb)=0;
			//! Set the cloud interface.
			virtual void SetCloudInterface(CloudInterface *ci)=0;
			//! Set the sky - this is used to light the clouds over time.
			void SetSkyInterface(simul::sky::SkyInterface *si);
			//! Once-per-frame update for the keyframer, call this with an absolute time value (not a time step).
			virtual void Update(float new_time)=0;
		};
	}
}

#endif