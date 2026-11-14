#ifndef SIMUL_META_PARTICLELAUNCHERINTERFACE_H
#define SIMUL_META_PARTICLELAUNCHERINTERFACE_H
#include "Simul/Math/Vector3.h"
#include <string>
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class ParticleLauncherInterface
			{
			public:
				typedef int ParticleType;
				typedef int EmitterId;
				virtual ParticleType GetParticleTypeFromString(const char *type_string)=0;
				virtual void LaunchParticle(ParticleType t,const simul::math::Vector3 &x,const simul::math::Vector3 &v,float life)=0;
				/// The caller requests a new emitter id to be created, which it then controls.
				virtual EmitterId GetNewEmitterId(ParticleType t)=0;
				/// The caller releases an emitter id it has control of.
				virtual void FreeEmitterId(EmitterId id)=0;
				/// The caller sets the emission strength for its emitter id.
				virtual void EmitParticles(EmitterId id,const simul::math::Vector3 &x,const simul::math::Vector3 &v,float strength)=0;
			};
	/*
	To fully implement this interface, you will need:
		public simul::graph::standardnodes::ParticleLauncherInterface

			virtual ParticleType GetParticleTypeFromString(const char *type_string);
			virtual void LaunchParticle(ParticleType t,const simul::math::Vector3 &x,const simul::math::Vector3 &v,float life);
			virtual EmitterId GetNewEmitterId(ParticleType t);
			virtual void FreeEmitterId(EmitterId id);
			virtual void EmitParticles(EmitterId id,const simul::math::Vector3 &x,const simul::math::Vector3 &v,float strength);
	*/
		}
	}
}
#endif