#ifndef SIMUL_META_ANIMATEDINTERFACE_H
#define SIMUL_META_ANIMATEDINTERFACE_H

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
//! An interface for anything that has a name returnable as text.
			class AnimatedInterface
			{
			public:
				virtual bool IsAnimEnabled(const char *anim_name) const=0;
				virtual bool IsAnimLooped(const char *anim_name) const=0;
			};
		}
	}
}


#endif