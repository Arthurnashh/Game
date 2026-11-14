#ifdef _MSC_VER
#pragma once
#endif

#ifndef _DYN_SOUND_HPP
#define _DYN_SOUND_HPP

#include "paramFileExt.hpp"
#include "vehicle.hpp"

class AIBrain;

/// one item of dynamically sellected sound list
class SoundEntry: public SoundPars
{
	// inherited: sfxfile, vol, frq
	friend class DynSound;
	friend class DynSoundSource;
	friend class DynSoundObject;
	protected:
	float _probab;
	float _min_delay,_mid_delay,_max_delay;
	/// remember sound duration to avoid querying it
	float _duration;
};


TypeIsMovableZeroed(SoundEntry);

/// Randomly (dynamically) selected sounds

class DynSound: public RefCountWithLinks
{
	friend class DynSoundBank;

	RString _name;
	AutoArray<SoundEntry> _sounds;
	SoundEntry _emptySound;

	public:
	DynSound();
	DynSound( const char *name );
	void Load( const char *name );

	static SoundEntry LoadEntry(ParamEntryVal entry, const char *filePath);

	const SoundEntry &SelectSound( float probab ) const;
	bool IsOneLoopingSound() const;
	RString GetName() const {return _name;}
};

TypeIsMovable(DynSound);

#include <Es/Containers/bankArray.hpp>


template <>
struct BankTraits<DynSound>: DefLinkBankTraits<DynSound>
{
};

class DynSoundBank: public BankArray<DynSound>
{
};

extern DynSoundBank DynSounds;

struct TitlesItem
{
	Time time;
  float duration;
	RString text;
};
TypeIsMovableZeroed(TitlesItem);

class SoundObject: public RemoveLLinks
{
	protected:
	RefAbstractWave _sound;
	OLinkPermNO(AIBrain) _sender;
  OLinkPermNO(AIBrain) _receiver;
	RString _soundName;
	bool _hasObject;
	bool _looped;
	bool _paused;
	bool _waiting;
	bool _forceTitles;
	bool _titlesStructured;
	bool _speakingStarted;
  bool _pos3D;
  bool _isSpeechEx;
  
	AutoArray <TitlesItem> _titles;
	int _index;

	RString _titlesFont;
	float _titlesSize;

	float _maxTitlesDistance;
	float _speed;

	void LoadSound();
	void StartSound(Object *source);
	void StartSpeakingFx(Object *source);

	public:
	SoundObject(
	  RString name, Object *source, bool pos3D=true, bool looped = false, float maxTitlesDistance = 100.0f, float speed = -1.0f, Object *target = NULL
	);
	bool Simulate(Object *source, float deltaT, SimulationImportance prec);

	void SimulateTitles(Object *source);

	AbstractWave *GetWave() {return _sound;}
  RString GetSoundName() const {return _soundName;}
	void Pause(bool pause = true) {_paused = pause;}

  void MarkSoundAsSpeechEx(bool isSpeechEx) { _isSpeechEx = isSpeechEx; }

	bool IsWaiting() const {return _waiting;}
  const AIBrain *GetSender() const {return _sender;}
  void SetSource( Object *source );
};

class SoundOnVehicle : public Entity
{
  typedef Entity base;

protected:
  OLink(Object) _object;
	Ref<SoundObject> _sound;

public:
	SoundOnVehicle(const char *name, Object *source, bool pos3D=true, float maxTitlesDistance = 100.0f, float speed = 1.0f, Object *target = NULL);
  bool SimulationReady(SimulationImportance prec) const;
	void Simulate( float deltaT, SimulationImportance prec );
	bool MustBeSaved() const {return false;}
	Visible VisibleStyle() const {return ObjInvisible;}
  void SoundIsSpeechEx(bool isSpeechEx) { if (_sound) _sound->MarkSoundAsSpeechEx(isSpeechEx); }

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat ( NetworkMessageClass cls, NetworkMessageFormat &format );
  static SoundOnVehicle *CreateObject(NetworkMessageContext &ctx);
  TMError TransferMsg(NetworkMessageContext &ctx) override;

  USE_CASTING(base);
};

class DynSoundObject: public RefCount
{
	protected:
	Ref<DynSound> _dynSound;
	Link<AbstractWave> _sound;

	float _timeToLive; // how long current sound will be played

	public:
	DynSoundObject( const char *name );
	~DynSoundObject();

	void Simulate( Object *source, float deltaT, SimulationImportance prec );
	void StopSound();
  RString GetName() const {return _dynSound ? _dynSound->GetName() : RString();}
};

class DynSoundSource: public Vehicle
{
  typedef Vehicle base;

	Ref<DynSoundObject> _dynSound;

	public:
	DynSoundSource( const char *name );
	~DynSoundSource();

  RString GetName() const {return _dynSound ? _dynSound->GetName() : RString();}

	void Simulate( float deltaT, SimulationImportance prec );
	void StopSound();

	// load / save
	bool MustBeSaved() const {return true;}
  LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
  (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
  static DynSoundSource *CreateObject(NetworkMessageContext &ctx);
  TMError TransferMsg(NetworkMessageContext &ctx);

  USE_CASTING(base)
};

#endif
