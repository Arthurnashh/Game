#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SOUNDSCENE_HPP
#define _SOUNDSCENE_HPP

#include "types.hpp"
#include "paramFileExt.hpp"
#include <El/Math/math3d.hpp>
#include "soundsys.hpp"
#include "handledList.hpp"

//typedef Ref<AbstractWave> SoundHandle;

class Speaker;
class RadioChannel;
class RadioMessage;

enum {ChannelEast,ChannelWest,ChannelGuerrila,NChannels};

struct SoundPars;

#include <El/Time/time.hpp>

/// manager of all playing sounds
class SoundScene
{
  /// list of global sounds (owner by the SoundScene)
	RefArray<AbstractWave> _globalSounds;

	LinkArray<AbstractWave> _all2DSounds;
	LinkArray<AbstractWave> _all3DSounds;

	RefAbstractWave _musicTrack;
	
	/// time elapsed by audio simulation - only non-paused time is considered
	/*** UITime is used here as type to prevent mixing with Time */
	UITime _audioTime;

  // whistling in ears
  struct Whistle
  {
    SoundPars _soundPars;
    RefAbstractWave _sound;
    float _down;
    float _silent;
    float _up;
    Time _downTime;   // time to 
    Time _silentTime; // silence time volume
    Time _upTime;     // restore orig. volume
    float _volume;    // [0, 1]
    bool _active;     // whistling active
    bool _init;       // values initialize
  } _whistle;

  //@{ volume controls for _musicTrack
	float _musicVolume;
	float _musicTargetVolume;
	Time _musicTargetTime;
	//@}

  //@{ sound controls for real 3D sounds with accommodation (effects)
	float _soundVolume;
	float _soundTargetVolume;
	Time _soundTargetTime;
	/// control gain for real sounds (used for silenced headphones simulation)
  float _soundGain;
	//@}

  //@{ sound controls for radio (2D speech/voice sounds)
	float _radioVolume;
	float _radioTargetVolume;
	Time _radioTargetTime;
	//@}

  //@{ sound controls for speech - volume is not affected by accomodation etc.
  float _speechExVolume;
  float _speechExTargetVolume;
  Time _speechExTargetTime;
  //@}
	
	float _musicInternalVolume; // given from config / wave
	float _musicInternalFrequency; // given from config / wave

	AutoArray<RefAbstractWave> _envSounds;
  AutoArray<RefAbstractWave> _envSoundsExt;

  /// how should we adjust the volume to simulate eye adaptation to overall volume
	float _earAccomodation;
	/// volume limit for the sound to be audible in the current environment
	float _limitMuteAbsolute;
	
public:
	SoundScene();
	~SoundScene();
	
	AbstractWave *Open( const char *filename, bool is3D=true, bool accom=true, float distance=0 );
	AbstractWave *OpenAndPlay(
		const char *filename, EntityAI *source, bool isInside, Vector3Par pos, Vector3Par speed,
		float volume=1.0, float frequency=1.0, float distance = 0.0
	);
	AbstractWave *OpenAndPlay(
		const char *filename, Vector3Par pos, Vector3Par speed,
		float volume=1.0, float frequency=1.0, float distance = 0.0
	);
	AbstractWave *OpenAndPlayOnce(
		const char *filename, EntityAI *source, bool isInside, Vector3Par pos, Vector3Par speed,
		float volume=1.0, float frequency=1.0, float distance = 0.0
	);
	AbstractWave *OpenAndPlay2D(
		const char *filename,
		float volume=1.0, float frequency=1.0,
		bool accomodate=true
	);
	AbstractWave *OpenAndPlayOnce2D(
		const char *filename,
		float volume=1.0, float frequency=1.0,
		bool accomodate=true
	);
	void SimulateSpeedOfSound(AbstractWave *sound);

	void AddSound( AbstractWave *wave ); // global sounds (Ref is in SoundScene)
	void DeleteSound( AbstractWave *wave );

  /// check if sound with given volume and position can be audible
  bool CanBeAudible(WaveKind kind, Vector3Par pos, float volume) const;
  
  bool CanBeAudible(Vector3Par pos, float distance) const;

	/// maintain all sounds 
	void AdvanceAll( float deltaT, bool paused, bool quiet );
	void Reset();
	
	UITime GetAudioTime() const;

  // whistling in ears
  void PerformWhistle(bool avtivate, bool paused);

	//@{ environmental sounds
  void SetEnvSound(int i, const SoundPars &pars, float volume, float obstruction, float occlusion);
	void AdvanceEnvSounds(int totalCount);
  void SetEnvSoundExt(int i, const SoundPars &pars, Vector3 &pos, float volume, float obstruction, float occlusion);
	//@}
	
	void StartMusicTrack(const SoundPars &pars, float from=0);
	void StopMusicTrack();

	float GetRadioVolume() const;
	void SetRadioVolume(float vol, float time);

  float GetSpeechExVolume() const;
  void SetSpeechExVolume(float vol, float time);

	float GetMusicVolume() const;
	void SetMusicVolume(float vol, float time);

	float GetSoundVolume() const;
	void SetSoundVolume(float vol, float time);

  float GetEarAccomodation() { return _earAccomodation; }

	// AbstractWave *Say( int speakerID, RString id );
	AbstractWave *SayPause( float time );
	//void Transmit( int channel, int speaker, RadioMessage *msg );

  LSError SerializeState(ParamArchive &ar);
};

extern SoundScene *GSoundScene;

#endif
