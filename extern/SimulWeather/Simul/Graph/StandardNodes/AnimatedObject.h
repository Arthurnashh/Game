#pragma once

#include "Simul\Graph\StandardNodes\OrientedObject.h"
#include "Simul\Graph\StandardNodes\AnimatedInterface.h"
#include <string>
#include <map>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class SIMUL_STD_EXPORT AnimatedObject
				:public OrientedObject
				,public simul::graph::standardnodes::AnimatedInterface
			{
			public:
				AnimatedObject(const char *what_name=NULL);
				virtual ~AnimatedObject();
				virtual bool IsAnimEnabled(const char *anim_name) const;
				virtual bool IsAnimLooped(const char *anim_name) const;
			protected:
				std::map<std::string,bool> enabled_anims;
				void EnableAnim(const char *anim_name,bool loop);
				void DisableAnim(const char *anim_name);
			};
		}
	}
}
#include <set>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
