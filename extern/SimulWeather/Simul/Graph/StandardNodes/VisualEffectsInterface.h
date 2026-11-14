#ifndef SIMUL_META_VISUALEFFECTSINTERFACE_H
#define SIMUL_META_VISUALEFFECTSINTERFACE_H
#include "Simul/Math/Vector3.h"
#include <string>
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class VisualEffectsInterface
			{
			public:
				virtual void StartEffect(const char *name,const simul::math::Vector3 &x,const simul::math::Vector3 &v)=0;
			};
	/*
	To fully implement this interface, you will need:
		public simul::graph::standardnodes::VisualEffectsInterface

				virtual void StartEffect(const char *name,const simul::math::Vector3 &x,const simul::math::Vector3 &v)=0;
	*/
		}
	}
}
#endif