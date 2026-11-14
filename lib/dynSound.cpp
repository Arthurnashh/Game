#include "wpch.hpp"
#include "dynSound.hpp"
#include <El/Common/randomGen.hpp>

#include "global.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include "titEffects.hpp"

#include "AI/ai.hpp"
#include "camera.hpp"
#include "engine.hpp"
#include "mbcs.hpp"

#include "Network/network.hpp"

//#include "soldierOld.hpp"

void FindSFX(RString name, SoundEntry &emptySound, AutoArray<SoundEntry> &sounds);
ConstParamEntryPtr FindSound(RString name, SoundPars &pars);

DynSound::DynSound()
{
}

DynSound::DynSound(const char *name)
{
	
	Load(name);
}

void DynSound::Load(const char *name)
{
	_name = name;
	FindSFX(name, _emptySound, _sounds);
}

SoundEntry DynSound::LoadEntry(ParamEntryVal entry, const char *filePath)
{
	// load single sound parameter
	SoundEntry sound;
	GetValue(sound, entry);
  if (entry.GetSize() == 8)
  {
    sound._probab=entry[4];
    sound._min_delay=entry[5];
    sound._mid_delay=entry[6];
    sound._max_delay=entry[7];
  }
	if (sound.name.GetLength()>0)
	{
	  if (*filePath)
	  {
	    sound.name = filePath + sound.name;
	  }
	  sound._duration = GSoundsys->GetWaveDuration(sound.name);
	}
	else
	{
	  sound._duration = 0;
	}
	return sound;
}

bool DynSound::IsOneLoopingSound() const
{
	if (_sounds.Size()!=1) return false;
	if (_sounds[0]._max_delay>0) return false;
	if (_emptySound._max_delay>0) return false;
	// in some cu
	return true;
}

const SoundEntry &DynSound::SelectSound( float probab ) const
{
	for( int i=0; i<_sounds.Size(); i++ )
	{
		probab-=_sounds[i]._probab;
		if( probab<=0 ) return _sounds[i];
	}
	return _emptySound;
}

void SoundObject::StartSpeakingFx(Object *source)
{
  if (_speakingStarted) return;
	if (_hasObject && _sound)
	{
	  SoundPars pars;
	  FindSound(_soundName, pars);
	  if (source->AttachWaveReady(pars.name) && _sound->PreparePlaying())
	  {
    	//LogF("AttachWave %s",cc_cast(_soundName));
		  source->AttachWave(_sound, pars.freq);
		  _sound->Play();
		  _speakingStarted = true;
	  }
	  else
	  {
	    // delay playing the wave until the lipsync is ready
	    _sound->Stop();
	  }
  }	  
}

void SoundObject::StartSound(Object *source)
{

	SoundPars pars;
	FindSound(_soundName, pars);
	
	//LogF("Start sound %s",cc_cast(_soundName));

	if (_hasObject && _pos3D)
	{
	  // open the sound, but do not play it yet
		if (_looped)
			_sound = GSoundScene->OpenAndPlay(
				pars.name, NULL, false, source->WorldPosition(source->FutureVisualState()), source->WorldSpeed(),
				pars.vol, pars.freq, pars.distance
			);
		else
			_sound = GSoundScene->OpenAndPlayOnce(
				pars.name, NULL, false, source->WorldPosition(source->FutureVisualState()), source->WorldSpeed(),
				pars.vol, pars.freq, pars.distance
			);
		if (_sound)
		{
		  StartSpeakingFx(source);
      if (_isSpeechEx) 
      {
        _sound->EnableAccomodation(false);
        _sound->SetKind(WaveSpeechEx); 
        _sound->SetObstruction(1.0f, 1.0f, 0.0f);
      }
		}
	}
	else
	{
		if (_looped)
			_sound = GSoundScene->OpenAndPlay2D(
				pars.name, pars.vol, pars.freq, true
			);
		else
			_sound = GSoundScene->OpenAndPlayOnce2D(
				pars.name, pars.vol, pars.freq, true
			);

    // force speech type - we want to listen sample in all cases
    if (_sound && _isSpeechEx) 
    { 
      _sound->EnableAccomodation(false);
      _sound->SetKind(WaveSpeechEx); 
      _sound->SetObstruction(1.0f, 1.0f, 0.0f);
    }
	}
}

void SoundObject::LoadSound()
{
	SoundPars pars;
	ConstParamEntryPtr cls = FindSound(_soundName, pars);

	if (cls)
	{
		ConstParamEntryPtr entry = cls->FindEntry("forceTitles");
		if (entry) _forceTitles = *entry;
		if (_forceTitles || Glob.config.showTitles)
		{
			entry = cls->FindEntry("titlesFont");
			if (entry)
			{
				_titlesFont = *entry; 
				_titlesSize = (*cls) >> "titlesSize";
			}

			_titlesStructured = false;
			entry = cls->FindEntry("titlesStructured");
			if (entry) _titlesStructured = *entry;
		
			ParamEntryVal cfg = (*cls) >> "titles";
			int n = cfg.GetSize() / 2;
			for (int i=0; i<n; i++)
			{
				int index = _titles.Add();
        const IParamArrayValue &value = cfg[2 * i];
        float t = 0;
        float duration = -1;
        if (value.IsArrayValue())
        {
          Assert(value.GetItemCount() == 2);
          t = value[0];
          duration = value[1];
        }
        else t = value;
				_titles[index].time = Glob.time + t;
        _titles[index].duration = duration;
				_titles[index].text = cfg[2 * i + 1];
			}
		}
	}
}

static LLinkArray<SoundObject> SpeakingObjects;

RadioChannel *SpeakingOnRadioChannel(AIBrain *sender, RadioChannel *ignore);

bool IsSpeakingDirect(AIBrain *sender)
{
	if (!sender) return false;

	for (int i=0; i<SpeakingObjects.Size();)
	{
		SoundObject *obj = SpeakingObjects[i];
		if (!obj) {SpeakingObjects.Delete(i); continue;}
		if (!obj->IsWaiting() && obj->GetSender() == sender) return true;
		i++;
	}
	return false;
}

SoundObject::SoundObject(RString name, Object *source, bool pos3D, bool looped, float maxTitlesDistance, float speed, Object *target)
{
	_index = 0;
	//_object = source;
	_soundName = name;

	_hasObject = source != NULL;
	_looped = looped;
	_paused = false;

	_forceTitles = false;
	_titlesSize = 0;
	_titlesStructured = false;
  _speakingStarted	= false;
  _pos3D = pos3D;
  _isSpeechEx = false;
  
	_maxTitlesDistance = maxTitlesDistance;
	_speed = speed;

	_waiting = false;
	if (source)
	{
		Person *person = dyn_cast<Person>(source);
		if (person && person->Brain())
		{
			_waiting = true;
			_sender = person->Brain();
			SpeakingObjects.Add(this);
		}
	}
  if (target)
  {
    Person *person = dyn_cast<Person>(target);
    if (person) _receiver = person->Brain();
  }
	if (!_waiting) LoadSound();
	
/*
	SoundPars pars;
	ConstParamEntryPtr cls = FindSound(name, pars);

	if (Glob.config.showTitles && cls)
	{
		ParamEntryVal cfg = (*cls) >> "titles";
		int n = cfg.GetSize() / 2;
		for (int i=0; i<n; i++)
		{
			int index = _titles.Add();
			float t = cfg[2 * i];
			_titles[index].time = Glob.time + t;
			_titles[index].text = cfg[2 * i + 1];
		}
		SimulateTitles();
	}
*/
}

bool SoundObject::Simulate(Object *source, float deltaT, SimulationImportance prec)
{
	if (_hasObject && (!source || source->IsDamageDestroyed()))
	{
		_sound = NULL;
		return false;
	}

	if (_waiting)
	{
    if (GWorld->IsRadioEnabled())
    {
      if (IsSpeakingDirect(_sender) || SpeakingOnRadioChannel(_sender,NULL)) return true;
    }
		_waiting = false;
		LoadSound();
	}

  // simulate titles even when not visible (can be added to the conversation history)
	SimulateTitles(source);

	if (_paused)
	{
		if (_sound) _sound = NULL;
	}
	else
	{
		if (_soundName.GetLength()>0 && !_sound)
		{
			StartSound(source);
		}
		if (_hasObject&& _sound)
		{
		  StartSpeakingFx(source);
			if (source)
			{
				if (_pos3D) _sound->SetPosition(source->WorldPosition(source->FutureVisualState()), source->WorldSpeed());
		  }
			else
			{
				// object destroyed - stop sound
				_sound = NULL;
			}
		}
		if (_sound && _sound->IsTerminated()) _sound = NULL;
	}
	
	return
	(
		_sound != NULL || ((_forceTitles || Glob.config.showTitles) && _index < _titles.Size())
	);
}

//! Show titles
/*!
	\patch 1.01 Date 06/12/2001 by Jirka
	- Fixed: titles was shown on unlimited distance
	- distance is limited to 100 m now
\patch 1.50 Date 4/2/2002 by Jirka
- Added: Enable different fonts for titles
*/

void SoundObject::SimulateTitles(Object *source)
{
	if (_index >= _titles.Size()) return;
	if (Glob.time >= _titles[_index].time)
	{
		// FIX titles are shown only on limited distance
		if
		(
			!source || !GScene->GetCamera() || _maxTitlesDistance == 0 ||
			GScene->GetCamera()->Position().Distance2(source->WorldTransform(source->RenderVisualState()).Position()) <= Square(_maxTitlesDistance)
		)
		{
      RString text = _titles[_index].text;
      if (_forceTitles || Glob.config.showTitles)
      {
        float duration = _titles[_index].duration;
        if (_speed >= 0) duration *= _speed;
        TitleEffect *effect = CreateTitleEffect(TitPlainDown, text, duration, _titlesFont, _titlesSize, _titlesStructured);
        GWorld->SetTitleEffect(effect);
      }
#if _ENABLE_IDENTITIES
      if (_sender && _receiver)
      {
        _sender->GetPerson()->GetIdentity().AddDiaryRecord("Conversation", RString(), text, NULL, TSNone, _receiver, _sender, true);
        _receiver->GetPerson()->GetIdentity().AddDiaryRecord("Conversation", RString(), text, NULL, TSNone, _sender, _sender, true);
      }
#endif
		}
		_index++;
	}
}

void SoundObject::SetSource( Object *source )
{
  if (source)
  {
    Person *person = dyn_cast<Person>(source);
    if (person && person->Brain())
    {
      _waiting = true;
      _sender = person->Brain();
      SpeakingObjects.Add(this);
    }
    _hasObject = true;
  }
}


DEFINE_CASTING(SoundOnVehicle)

SoundOnVehicle::SoundOnVehicle(
  const char *name, Object *source, bool pos3D, float maxTitlesDistance, float speed, Object *target
)
:Vehicle(NULL, VehicleTypes.New("#soundonvehicle"), CreateObjectId()
)
{
  _object = source;
	_sound = new SoundObject(name, source, pos3D, false, maxTitlesDistance, speed, target);
}

#define SOUND_ON_VEHICLE_MSG_LIST(XX) \
  XX(Create, CreateSoundOnVehicle)

DEFINE_NETWORK_OBJECT(SoundOnVehicle, base, SOUND_ON_VEHICLE_MSG_LIST)

#define CREATE_SOUND_ON_VEHICLE_MSG(MessageName, XX) \
  XX(MessageName, OLink(Object), object, NDTRef, NetworkId, NCTNone, DEFVALUENULL, DOC_MSG("Vehicle this is attached to"), TRANSF_REF) \
  XX(MessageName, RString, soundName, NDTString, RString, NCTNone, DEFVALUE(RString, RString()), DOC_MSG("SFX sound name"), TRANSF)

DECLARE_NET_INDICES_EX(CreateSoundOnVehicle, CreateVehicle, CREATE_SOUND_ON_VEHICLE_MSG)
DEFINE_NET_INDICES_EX(CreateSoundOnVehicle, CreateVehicle, CREATE_SOUND_ON_VEHICLE_MSG)

NetworkMessageFormat &SoundOnVehicle::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCCreate:
    base::CreateFormat(cls, format);
    CREATE_SOUND_ON_VEHICLE_MSG(CreateSoundOnVehicle, MSG_FORMAT)
      break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError SoundOnVehicle::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCCreate:
    if (ctx.IsSending())
    {
      TMCHECK(base::TransferMsg(ctx))
    }
    {
      PREPARE_TRANSFER(CreateSoundOnVehicle)

      TRANSF_REF(object)
      if (!ctx.IsSending())
      {
        _sound->SetSource(_object);
      }
      else
      {
        RString soundName = _sound->GetSoundName();
        TRANSF_BASE(soundName,soundName);
      }
    }
    break;
  default:
    TMCHECK(base::TransferMsg(ctx))
      break;
  }
  return TMOK;
}

SoundOnVehicle *SoundOnVehicle::CreateObject(NetworkMessageContext &ctx)
{
  NetworkMessageCreateSoundOnVehicle *msg = static_cast_checked<NetworkMessageCreateSoundOnVehicle *>(ctx.GetNetworkMessage());
  RString name = msg->_soundName;
  auto create = [name](RString type,RString shape){return new SoundOnVehicle(name,nullptr);};
  base *veh = base::CreateObject(ctx,create);
  SoundOnVehicle *sound = dyn_cast<SoundOnVehicle>(veh);
  if (!sound) return NULL;

  if (sound->TransferMsg(ctx) != TMOK) return NULL;
  return sound;
}

bool SoundOnVehicle::SimulationReady(SimulationImportance prec) const
{
  return true;
}
void SoundOnVehicle::Simulate( float deltaT, SimulationImportance prec )
{
	if (!_sound || !_sound->Simulate(_object,deltaT, prec))
		SetDelete();
}

/*!
\patch 1.57 Date 5/16/2002 by Ondra
- Fixed: When one sound with no pauses is played in CfgSFX, it is now correctly looped.
*/

void DynSoundObject::Simulate( Object *source, float deltaT, SimulationImportance prec )
{
	if (prec>SimulateInvisibleNear)
	{
		if (_sound) _sound->LastLoop();
		return;
	}
	_timeToLive-=deltaT;
  // position must be updated, else no 3d sound is updated
  if (_sound && _sound->Get3D()) _sound->SetPosition(source->WorldPosition(source->FutureVisualState()),source->WorldSpeed());
	
  if( _timeToLive>0 ) return; // continue playing or pause
	// select sound source
	float rand=GRandGen.RandomValue();
	const SoundEntry &pars=_dynSound->SelectSound(rand);
	bool loop = _dynSound->IsOneLoopingSound();
	// play selected sound
	AbstractWave *wave=NULL;
	if( pars.name[0] )
	{
		float rndFreq=1;
		float rndVol=1;
		if( pars.freqRnd ) rndFreq=1+GRandGen.RandomValue()*pars.freqRnd-pars.freqRnd*0.5;
		if( pars.volRnd ) rndVol=1+GRandGen.RandomValue()*pars.volRnd-pars.volRnd*0.5;
		if (loop)
		{
			wave=GSoundScene->OpenAndPlay
			(
				pars.name,NULL, false, 
				source->WorldPosition(source->FutureVisualState()),source->WorldSpeed(),
				pars.vol*rndVol,pars.freq*rndFreq, pars.distance
			);
		}
		else
		{
			wave=GSoundScene->OpenAndPlayOnce
			(
				pars.name,NULL, false, 
				source->WorldPosition(source->FutureVisualState()),source->WorldSpeed(),
				pars.vol*rndVol,pars.freq*rndFreq, pars.distance
			);
		}
		if( wave )
		{
			GSoundScene->AddSound(wave);
		}
	}
	// random time to live
	if (!loop)
	{
		float delay = GRandGen.Gauss(pars._min_delay, pars._mid_delay, pars._max_delay);
		_timeToLive = delay+pars._duration;
	}
	else
	{
		_timeToLive = 1e10;
	}
	if (_sound) _sound->LastLoop();
	_sound = wave;
}


DynSoundBank DynSounds;


// dynamic sounds:
// some technology to place dynamic sounds in landscape

DynSoundObject::DynSoundObject( const char *name )
:_timeToLive(0) // how long current sound will be played
{
	_dynSound=DynSounds.New(name);
}

void DynSoundObject::StopSound()
{
	if (_sound) _sound->LastLoop();
}

DynSoundObject::~DynSoundObject()
{
	StopSound();
}

DEFINE_CASTING(DynSoundSource)

DynSoundSource::DynSoundSource( const char *name )
:Vehicle(NULL,VehicleTypes.New("#dynamicsound"),CreateObjectId())
{
	// TODO: regular creation for serialization, network transport
	if (name) _dynSound=new DynSoundObject(name);
}

DynSoundSource::~DynSoundSource()
{
}

void DynSoundSource::StopSound()
{
	if (_dynSound) _dynSound->StopSound();
}

void DynSoundSource::Simulate( float deltaT, SimulationImportance prec )
{
	if (_dynSound) _dynSound->Simulate(this,deltaT,prec);
}

LSError DynSoundSource::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar))
  if (ar.IsSaving())
  {
    RString name = GetName();
    CHECK(ar.Serialize("name", name, 1, RString()))
  }
  else
  {
    RString name;
    CHECK(ar.Serialize("name", name, 1, RString()))
    if (name.GetLength() > 0) _dynSound = new DynSoundObject(name);
  }
  return LSOK;
}

#define DYN_SOUND_SOURCE_MSG_LIST(XX) \
  XX(Create, CreateDynSoundSource) \
  XX(UpdateGeneric, None) \
  XX(UpdateDamage, None)

DEFINE_NETWORK_OBJECT(DynSoundSource, Vehicle, DYN_SOUND_SOURCE_MSG_LIST)

#define CREATE_DYN_SOUND_SOURCE_MSG(MessageName, XX) \
  XX(MessageName, RString, soundName, NDTString, RString, NCTNone, DEFVALUE(RString, RString()), DOC_MSG("SFX sound name"), TRANSF)

DECLARE_NET_INDICES_EX(CreateDynSoundSource, CreateVehicle, CREATE_DYN_SOUND_SOURCE_MSG)
DEFINE_NET_INDICES_EX(CreateDynSoundSource, CreateVehicle, CREATE_DYN_SOUND_SOURCE_MSG)

NetworkMessageFormat &DynSoundSource::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCCreate:
    base::CreateFormat(cls, format);
    CREATE_DYN_SOUND_SOURCE_MSG(CreateDynSoundSource, MSG_FORMAT);
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError DynSoundSource::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCCreate:
    if (ctx.IsSending())
    {
      TMCHECK(base::TransferMsg(ctx))

      PREPARE_TRANSFER(CreateDynSoundSource)
      
      RString name = GetName();
      TRANSF_EX(soundName, name)
    }
    break;
  default:
    TMCHECK(base::TransferMsg(ctx))
    break;
  }
  return TMOK;
}

DynSoundSource *DynSoundSource::CreateObject(NetworkMessageContext &ctx)
{
  Vehicle *veh = base::CreateObject(ctx);
  DynSoundSource *source = dyn_cast<DynSoundSource>(veh);
  if (!source) return NULL;

  PREPARE_TRANSFER(CreateDynSoundSource)

  RString name;
  if (TRANSF_BASE(soundName, name) != TMOK) return NULL;
  
  if (name.GetLength() > 0) source->_dynSound = new DynSoundObject(name);
  return source;
}

#ifdef _WIN32
template Link<DynSound>;
#endif

