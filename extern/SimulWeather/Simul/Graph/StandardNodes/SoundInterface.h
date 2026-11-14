#ifndef SIMUL_META_SOUNDINTERFACE_H
#define SIMUL_META_SOUNDINTERFACE_H
#include "Simul/Graph/Meta/Node.h"
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class SoundInterface
			{
			public:
				virtual int GetNumSounds()=0;
				virtual int GetSoundIdent(int sound)=0;
				virtual float GetVolumeForSound(int sound)=0;
				virtual int GetNumValuesForSound(int sound)=0;
				virtual float GetValueForSound(int sound,int slider)=0;
				virtual const char *GetValueNameForSound(int sound,int slider)=0;
				virtual const char *GetSoundNameAsCString(int index)=0;
			};
			class SoundResourceInterface
			{
			public:
				virtual int CreateSound(simul::graph::meta::Node *,const char *sound_name)=0;	//< return -1 if not created.
				virtual void StartSound(simul::graph::meta::Node *,int)=0;						//< Start the sound indicated.
				virtual void EndSound(simul::graph::meta::Node *,int)=0;						//< End the sound indicated.
			};
		}
	}
}
#endif