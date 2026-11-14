#include "wpch.hpp"
#include "paramFileExt.hpp"
#include "soundsys.hpp"
#include "soundScene.hpp"
#include "paramArchiveExt.hpp"
#include "speaker.hpp"
#include "global.hpp"
#include "world.hpp"
#include "person.hpp"
#include "UI/chat.hpp"
#include <El/Common/perfProf.hpp>
#include <El/Common/randomGen.hpp>

//#include "loadStream.hpp"

#include "fileLocator.hpp"

static RStringB RandomMicOut(RString name)
{
  ParamEntryVal micOuts = Pars >> "CfgVoice" >> name;
  if (micOuts.GetSize()==0) return RStringB();
  int i = toIntFloor(GRandGen.RandomValue() * micOuts.GetSize());
  saturateMin(i, micOuts.GetSize() - 1);
  return micOuts[i];
}

BasicSpeaker::BasicSpeaker( RString speaker )
{
  // TODO: share BasicSpeakers in some bank?
  ParamEntryVal cfg = Pars >> "CfgVoice";
  ConstParamEntryPtr entry = !speaker.IsEmpty() ? cfg.FindEntry(speaker) : ConstParamEntryPtr();
  if (!entry)
  {
    // hot-fix: when bad speaker is given, use the 1st one
    RString newSpeaker;
    if (cfg.FindEntry("default"))
    {
      newSpeaker = (cfg >> "default");
      // when speaker is empty, it is no patching, it is normal default construction - no warning
      if (!speaker.IsEmpty())
      {
        RptF("Speaker %s not found, patched to default %s",cc_cast(speaker),cc_cast(newSpeaker));
      }
    }
    entry = !newSpeaker.IsEmpty() ? cfg.FindEntry(newSpeaker) : ConstParamEntryPtr();
    if (!entry && cfg.FindEntry("voices"))
    {
      RString firstVoice = (cfg>>"voices")[0];
      RptF("Speaker %s not found, patched to first %s",cc_cast(newSpeaker),cc_cast(firstVoice));
      entry = &(cfg>>firstVoice);
    }
  }
  if (entry)
  {
    Load(*entry);
  }
  else
  {
    RptF("No speaker configuration found.");
  }
}

BasicSpeaker::BasicSpeaker(ParamEntryPar cfg)
{
  Load(cfg);
}

void BasicSpeaker::Load(ParamEntryPar cfg)
{
  _protocol = cfg >> "protocol";

  ParamEntryVal dirs = cfg >> "directories";
  int n = dirs.GetSize();
  _directories.Realloc(n);
  _directories.Resize(n);
  for (int i=0; i<n; i++)
  {
    RString dir = dirs[i];
    if (dir[0] == '\\')
      _directories[i] = (const char *)dir + 1;
    else
      _directories[i] = RString("voice\\") + dir;
  }
}


extern RString GetMissionDirectory();

/**
@param who3D passed only when real 3D positioned sound is expected
*/
AbstractWave *BasicSpeaker::Say(Person *who3D, RString id, float pitch, bool loop, RString wordsSubclass)
{
  if (id.GetLength()==0) return NULL;
  RString name;
  if (id[0] == '\\')
  {
    // access the file directly
    name = GetSoundName(id);
    RString fullName = GetMissionDirectory() + name;
    if (QFBankQueryFunctions::FileExists(fullName)) name = fullName;
  }
  else
  {
    // select radio protocol
    if (_protocol.IsEmpty()) return NULL;
    ConstParamEntryPtr protocolCls = Pars.FindEntry(_protocol);
    if (!protocolCls) return NULL; // protocol not found

    // search for the word
    RString dir = Glob.config.singleVoice ? _directories[1] : _directories[0]; 
    
    ConstParamEntryPtr wordId;
    if (wordsSubclass.IsEmpty())
    {
      wordId = ((*protocolCls) >> "Words").FindEntry(id);
    }
    else
    {
      ConstParamEntryPtr wordsClass = ((*protocolCls) >> "Words").FindEntry(wordsSubclass);
      if (wordsClass)
      {
        wordId = wordsClass->FindEntry(id);
      }
      if (!wordId)
      {
#define USE_PARENT_WORD_WHEN_NOT_FIND 1
#if USE_PARENT_WORD_WHEN_NOT_FIND
        wordId = ((*protocolCls) >> "Words").FindEntry(id);
#else
        return NULL; //no word found
#endif
      }
    }
    if (!wordId)
    {
      RptF("Protocol %s: Missing word %s",cc_cast(protocolCls->GetContext()),cc_cast(id));
      return NULL;
    }
    
    ParamEntryVal word = *wordId;
    if (word.GetSize() < 1) return NULL; // no sound is given

    RString file = word[0];
    name = file[0]!='\\' ? dir+file : file.Substring(1,INT_MAX);
    if (*GetFileExt(name)==0) // when there is no extension, add default one
      name = name+RString(".wss");

  }

  AbstractWave *wave = NULL;
  float volume=GSoundScene->GetRadioVolume();
  if (who3D)
  {
    const float volume3DCoef = 0.5f;
    const float distance = 100;
    wave = GSoundScene->OpenAndPlayOnce(name, who3D, false, who3D->FutureVisualState().Position(), who3D->FutureVisualState().Speed(), volume*volume3DCoef, pitch, distance);
    who3D->SetDirectSpeaking(wave);
    who3D->AttachWave(wave,pitch);
    if (wave)
    {
      wave->SetKind(WaveSpeechEx);
      wave->EnableAccomodation(true);
      if (loop) wave->Repeat(1000);
    }
  }
  else
  {
    wave = GSoundScene->OpenAndPlayOnce2D(name,volume,pitch,false);
    if (wave)
    {
      wave->SetKind(WaveSpeech);
      wave->SetAccomodation();
      if (loop) wave->Repeat(1000);
    }
  }
  return wave;
}


RadioChannel::RadioChannel
(
  ChatChannel chatChannel, ParamEntryPar cfg
)
{
  _audible=false;
  _chatChannel = chatChannel;
  //_object = object;
  _pauseAfterMessage=0;
  // by default there is no timeout
  _sayingTimeout = TIME_MAX;
  
  _level = cfg >> "level";
  _params.noise = cfg >> "noise";
  _params.micOuts = cfg >> "micOuts";
  _params.pauseAfterWord = cfg >> "pauseAfterWord";
  _params.pauseInNumber = cfg >> "pauseInNumber";
  _params.pauseAfterNumber = cfg >> "pauseAfterNumber";
  _params.pauseInUnitList = cfg >> "pauseInUnitList";
  _params.pauseAfterUnitList = cfg >> "pauseAfterUnitList";

  ConstParamEntryPtr entry = cfg.FindEntry("sound3D");
  if (entry) _params.sound3D = *entry;
  else _params.sound3D = false;
}

bool RadioChannel::Done() const
{
  if( _radioQueue.Size()>0 ) return false;
  return _actualMsg == NULL;
}

/*!
\patch 1.51 Date 4/19/2002 by Ondra
- Fixed: Improved vehicle/group radio channel interleaving.
*/

void RadioChannel::Simulate(float deltaT, NetworkObject *object)
{
  // handle speaker's death
  if (_actualMsg && !_actualMsg->CanSenderSpeak())
  {
    // cancel the current message, stop all sounds
    _actualMsg->Canceled();
    _actualMsg.Free();
    _pauseAfterMessage = 0;
    _saying.Free();
    _noise.Free();
  }

  //PROFILE_SCOPE(aiRad);
  if (_saying)
  {
    _saying->PreloadQueue();
    if( !_saying->IsTerminated() && Glob.time<_sayingTimeout) return;
    
    // release wave - to stay safe we execute the handler if they are still pending
    // we should execute the handler for all sounds in the queue, as we are releasing all of them
    _saying.Free();
    _sayingTimeout = TIME_MAX;
    
    // stop noise channel
    if (_audible && _speaker.IsSpeakerValid() && _params.micOuts.GetLength() > 0)
    {
      // 3D noise has no sense
      _noise = _speaker.SayNoPitch(NULL, RandomMicOut(_params.micOuts), false, RString());
    }
    else
    {
      _noise.Free();
    }
    // noise will auto-terminated
  }
  // here _noise must contain _micOut (non-looped)
  if (_noise)
  {
    if (!_noise->IsTerminated())
    {
      return;
    }
    _noise.Free();
  }

  _pauseAfterMessage-=deltaT;
  if( _pauseAfterMessage<=0 ) NextMessage(object);
  return;
}


#include <Es/Memory/normalNew.hpp>

/// context used when part of the message starts playing
struct OnPlayRadioContext: public RefCount
{
  ChatChannel _channel;
  /// which message is considered transmitted
  Link<RadioMessage> _message;
 
  /// which radio channel is this on
  Link<RadioChannel> _radio;

  /// which person should speak
  OLink(Person) _person;
  
  public:
  OnPlayRadioContext(ChatChannel channel, RadioMessage *msg, Person *person, RadioChannel *radio)
  : _channel(channel), _message(msg), _person(person), _radio(radio) {}

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

DEFINE_FAST_ALLOCATOR(OnPlayRadioContext)

static void OnPlayRadio(AbstractWave *wave, RefCount *context)
{
  if (context)
  {

    OnPlayRadioContext *ctx = static_cast<OnPlayRadioContext *>(context);
    RadioMessage *msg = ctx->_message;
    if (msg && !msg->IsTransmitted())
    {
      msg->Transmitted(ctx->_channel);
      msg->SetTransmitted();
    }
    if (ctx->_radio)
    {
      // we rather presume playback can take significantly longer
      static float mul = 2.0f;
      static float add = 10.0f;
      ctx->_radio->SetTimeout(Glob.time + wave->GetLength()*mul+add);
    }
    #if 1
    if (ctx->_person && !ctx->_person->IsDamageDestroyed())
    {
      //Log("OnPlay %s",cc_cast(wave->Name()));
      // we want the copy of the wave to start playing using 3D sound
      Ref<AbstractWave> wave3D = GSoundScene->Open(wave->Name());
      if (wave3D)
      {
        const float volume3DCoef = 0.5f;
        const float distance = 60;
        float volume=GSoundScene->GetRadioVolume();
        wave3D->SetVolume(volume*volume3DCoef, wave->GetFrequency());
        wave3D->SetMaxDistance(distance);
        wave3D->SetKind(WaveEffect);
        wave3D->SetPosition(ctx->_person->FutureVisualState().Position(), ctx->_person->FutureVisualState().Speed());

        // eliminate echo when two same samples played with small time shift
        AbstractWave *wave2D = ctx->_radio ? ctx->_radio->Saying() : NULL;
        float volume2D = wave2D ? (wave2D->GetVolume() * 1.1f) : 0.0f;

        // when 3D wave is audible than stop 2D wave, mult by 1.1 to avoid oscillations when volumes are almost same
        if (volume2D < wave3D->GetLoudness())
        {
          if (wave2D)
          {
            wave2D->SetVolume(0);
            wave2D->Stop();
          }

          float obstruction = 1, occlusion = 1;
          // we do not know if inside or not - let CheckSoundObstruction handle it 
          GWorld->CheckSoundObstruction(ctx->_person,true,obstruction,occlusion);
          // we do not know any time delta here
          wave3D->SetObstruction(obstruction,occlusion);

          wave3D->Repeat(1);
          // this is called during sound scene commit
          // we cannot rely upon Man::Sound being called on the sound
          // it will start playing as needed - sound scene will start it

          wave3D->Play();
          ctx->_person->SetSpeaking3D(wave3D);
          ctx->_person->AttachWave(wave3D,wave->GetFrequency());
          
        }
        else  
        {
          ctx->_person->AttachWave(wave,wave->GetFrequency());
          wave3D.Free();
        }

      }
    }
    #endif
  }
}


void RadioChannel::Say(Person *who, Speaker *speaker, RString id, float pauseAfter, bool transmit, RString wordsSubclass)
{
  if (!speaker) return;

  Person *who3D = NULL;
  Person *who2D = who;
  // when echo is played for the player, no noise and radio effects should be done
  bool supressRadioEffects = who && who->Brain()==GWorld->FocusOn();
  // if the channel is already 3D, no need to drive 3D replica
  switch (_chatChannel)
  {
    case CCDirect:
    #if _ENABLE_DIRECT_MESSAGES
    case CCDirectSpeaking:
    #endif
      supressRadioEffects = true;
      who3D = who;
      who2D = NULL;
      break;
  }
  if( !_saying )
  {
    // start queue
    _speaker = *speaker;
    // until proven otherwise, assume very long timeout
    _sayingTimeout = Glob.time + 2*60;
    _saying = speaker->Say(who3D, id, wordsSubclass);
    if( _saying )
    {
      if (transmit) _saying->SetOnPlay(OnPlayRadio, new OnPlayRadioContext(_chatChannel, _actualMsg, who2D, this));
      else _saying->SetOnPlay(OnPlayRadio, new OnPlayRadioContext(_chatChannel, NULL, who2D, this));
      if (!_audible)
      {
        // mute
        _saying->SetVolume(0);
      }
      _saying->Play(); // start playback (once)
      //LogF("Started wave %s: %x",(const char *)_saying->Name(),_saying);
      // start noise
      if (_params.noise.GetLength() > 0 && !supressRadioEffects)
      {
        _saying->SetRadio(true);
      }
      if (_audible && _params.noise.GetLength() > 0 && !supressRadioEffects)
      {
        // radio noise played as 2d
        _noise = _speaker.SayNoPitch(NULL, _params.noise, true, wordsSubclass);
        // when there is a noise, it is a radio
      }
      else
      {
        _noise.Free();
      }
    }
  }
  else
  {
    AbstractWave *wave = speaker->Say(who3D, id, wordsSubclass);
    if( wave )
    {
      if (transmit) wave->SetOnPlay(OnPlayRadio, new OnPlayRadioContext(_chatChannel, _actualMsg, who2D, this));
      else wave->SetOnPlay(OnPlayRadio, new OnPlayRadioContext(_chatChannel, NULL, who2D, this));
      if (!_audible)
      {
        // mute
        wave->SetVolume(0);
      }
      wave->Queue(_saying);
      //LogF("Queued wave %s: %x",(const char *)wave->Name(),wave);
      if (_params.noise.GetLength() > 0 && !supressRadioEffects)
      {
        wave->SetRadio(true);
      }
    }
  }
  if (_saying && pauseAfter>0.01)
  {
    AbstractWave *wave=GSoundScene->SayPause(pauseAfter);
    if( wave )
    {
      wave->Queue(_saying);
      //LogF("Queued pause %.2f: %x",pauseAfter,wave);
    }
  }
}

void RadioChannel::Transmit(RadioMessage *msg)
{
  // add single message to message queue
  int priority = 0; // default priority
  float timeout = 1000.0f; // never
  Speaker *speaker = msg->GetSpeaker();
  if (speaker) 
  {
    RString protocol = speaker->GetProtocol();
    ConstParamEntryPtr protocolCls = protocol.GetLength() > 0 ? Pars.FindEntry(protocol) : ParamEntryPtr(NULL);
    if (protocolCls)
    {
      RString priorityClass = msg->GetPriorityClass();
      priority = (*protocolCls) >> priorityClass >> "priority";
      timeout = (*protocolCls) >> priorityClass >> "timeout";
    }
  }
  msg->SetPriority(priority);
  msg->SetTimeOut(Glob.time + timeout);

  int n = _radioQueue.Size();
  for (int i=0; i<n; i++)
  {
    if (priority > _radioQueue[i]->GetPriority())
    {
      _radioQueue.Insert(i, msg);
      return;
    }
  }
  _radioQueue.Add(msg);
}

void RadioChannel::Cancel( RadioMessage *msg, NetworkObject *object )
{
  // remove from queue/current message
  if( msg==_actualMsg )
  {
    msg->Canceled();    
    _actualMsg.Free();
    NextMessage(object);
    return;
  }
  for( int i=0; i<_radioQueue.Size(); i++ )
  {
    if( _radioQueue[i]==msg )
    {
      msg->Canceled();
      _radioQueue.Delete(i);
      return;
    }
  }
  Fail("Canceled message never transmitted.");
}

void RadioChannel::FinishActualMessage()
{
  if (_actualMsg)
  {
    // we canceled the message, but for the purpose of the game we consider it transmitted
    _actualMsg->Canceled();
    _actualMsg.Free();
    _pauseAfterMessage = 0;
    _saying.Free();
    _noise.Free();
  }
}

void RadioChannel::CancelActualMessage()
{
  if (_actualMsg)
  {
    // we canceled the message, but for the purpose of the game we consider it transmitted
    if (!_actualMsg->IsTransmitted())
    {
      _actualMsg->Transmitted(_chatChannel);
    }
    _actualMsg.Free();
    _pauseAfterMessage = 0;
    _saying.Free();
    _noise.Free();
  }
}


void RadioChannel::InterruptActualMessage()
{
  if (_actualMsg)
  {
    // if the message is not consider transmitted yet, return it into the waiting queue
    if (!_actualMsg->IsTransmitted())
    {
      _radioQueue.Insert(0,_actualMsg);
    }
    _actualMsg.Free();
    _pauseAfterMessage = 0;
    _saying.Free();
    _noise.Free();
  }
}

#include "AI/ai.hpp"
#include "sentences.hpp"

void RadioChannel::StopSpeaking(AIBrain *sender)
{
  if (_actualMsg && _actualMsg->GetSender()==sender)
  {
    LogF("Stop speaking %s", cc_cast(_actualMsg->GetDebugName()));
    InterruptActualMessage();
  }
  // if the message was not actual yet, we do not need to do anything
}

void RadioChannel::CancelAllMessages(NetworkObject *object)
{
  for (int i=0; i<_radioQueue.Size(); i++) _radioQueue[i]->Canceled();
  _radioQueue.Resize(0);
  if (_actualMsg) _actualMsg->Canceled();
  _actualMsg.Free();
  NextMessage(object);
}

void RadioChannel::Replace( RadioMessage *msg, RadioMessage *with )
{
  // replace in queue/current message
  if( msg==_actualMsg )
  {
    // do not change words of actual message, only text displayed
    _actualMsg->Canceled();
    _actualMsg=with;
    return;
  }
  for( int i=0; i<_radioQueue.Size(); i++ )
  {
    if( _radioQueue[i]==msg )
    {
      _radioQueue[i]=with;
      return;
    }
  }
  Fail("Replaced message never transmitted.");
}

void RadioChannel::SetAudible( bool audible )
{
  if( _audible && !audible )
  {
    CancelActualMessage();
  }
  _audible=audible;
}

LSError RadioChannel::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("audible", _audible, 1))
  CHECK(ar.Serialize("actualMsg", _actualMsg, 1))
  CHECK(ar.Serialize("messageQueue", _radioQueue, 1))
  if (ar.IsLoading() && ar.GetPass()==ParamArchive::PassSecond)
  {
    _radioQueue.RemoveNulls();
  }
  return LSOK;
}

bool RadioChannel::IsSilent() const
{
  if (!IsEmpty()) return false;
  return !_saying && !_noise;
}
