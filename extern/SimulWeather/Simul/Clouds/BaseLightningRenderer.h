#ifndef SIMUL_CLOUDS_BASELIGHTNINGRENDERER_H
#define SIMUL_CLOUDS_BASELIGHTNINGRENDERER_H
#include "Simul/Clouds/Export.h"
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Clouds/LightningRenderInterface.h"

namespace simul
{
	namespace sky
	{
		struct float4;
	}
	namespace clouds
	{
		//! The main base class for all Lightning renderers, such as SimulGLLightningRenderer, SimulLightningRenderer (DX9) etc.
		SIMUL_CLOUDS_EXPORT_CLASS BaseLightningRenderer
			:public simul::graph::meta::Group
		{
		public:
			BaseLightningRenderer(simul::clouds::LightningRenderInterface *lri=NULL);
			virtual ~BaseLightningRenderer();
			META_BeginProperties
				META_ValueRangeProperty(float,xxx,1.6f,0.f,100.f,"Speed of light rain.")
			META_EndProperties
			Macro_EnableRecursiveStreaming(simul::graph::meta::Group)
			Macro_Node(BaseLightningRenderer)
			virtual bool Render(){return true;}
		protected:
			simul::clouds::LightningRenderInterface *lightningRenderInterface;
		};
	}
}
#endif