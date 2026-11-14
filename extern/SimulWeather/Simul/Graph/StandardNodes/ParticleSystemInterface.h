#ifndef SIMUL_META_PARTICLESYSTEMINTERFACE_H
#define SIMUL_META_PARTICLESYSTEMINTERFACE_H
#include "Simul/Math/Vector3.h"
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class ParticleSystemInterface
			{
			public:
				virtual const unsigned GetNumParticles()=0;
				virtual const simul::math::Vector3 &GetParticlePosition(int i) const=0;
				virtual const simul::math::Vector3 &GetParticleVelocity(int i) const=0;
			};
	/*
	To fully implement this interface, you will need:
		virtual const unsigned GetNumParticles();
		virtual const float *GetParticlePosition(unsigned index);
	*/
		}
	}
}
#endif