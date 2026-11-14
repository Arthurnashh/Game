#include "wpch.hpp"

#include "soundScene.hpp"
#include "global.hpp"
#include "speaker.hpp"
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>

#include "diagModes.hpp"
#include <Es/Containers/staticArray.hpp>
#include <Es/Algorithms/qsort.hpp>
#include "world.hpp"

SoundScene *GSoundScene;
AbstractSoundSystem *GSoundsys;

static const float DefaultEarAccomodation = 1.0f;
static const float LimitPlayRelative = 5.623e-3f; // -45 dB
static const float LimitMuteRelative = 3.16e-3f; // -50 dB
static const float LimitPlayMinimal = 1e-4f; // -80 dB
static const float LimitMuteMinimal = 7e-5f; // -83 dB

SoundScene::SoundScene()
{
  _earAccomodation = DefaultEarAccomodation;
  _limitMuteAbsolute = LimitMuteMinimal;

  _whistle._active = false;
  _whistle._volume = 1;

  // load sound parameters from config file
  ParamEntryVal pars = Pars >> "CfgWhistleSound";
  GetValue(_whistle._soundPars, pars >> "sound");

  _whistle._down = pars >> "downTime";
  _whistle._silent = pars >> "silentTime";
  _whistle._up = pars >> "upTime";
}

SoundScene::~SoundScene()
{
  _whistle._active = false;
  _whistle._volume = 1;
  _whistle._sound.Free();
}

AbstractWave *SoundScene::Open( const char *filename, bool is3D, bool accom, float distance )
{
  //ADD_COUNTER(wavOp,1);
  // TODO: more consistent speech channel preallocation
  bool prealloc=!is3D && !accom;
  AbstractWave *wave = GSoundsys->CreateWave(filename,is3D,prealloc);
  
  if( !wave ) return wave;
  if( is3D ) _all3DSounds.Add(wave);
  else _all2DSounds.Add(wave);
  wave->SetMaxDistance(distance);
  wave->EnableAccomodation(accom);
  if( accom ) wave->SetAccomodation(_earAccomodation);
  //Assert( VerifyLinks(wave) );
  return wave;

}
void SoundScene::AddSound( AbstractWave *wave )
{
  if( wave )
  {
    //Log("%f: Add global sound %s",Glob.time,wave->Name());
    _globalSounds.Add(wave);
    //Assert( wave->RefCounter()==1 );
    //Assert( VerifyLinks(wave) );
  }
}

void SoundScene::DeleteSound( AbstractWave *wave )
{
  if( wave )
  {
    //Log("%f: Delete global sound %s",Glob.time,wave->Name());
    _globalSounds.Delete(wave);
  }
}

#if _DEBUG
/// verify the pointers were not invalidated while sorting
#define VERIFY_SS_NULL 1
#else
/// verification has some overhead, do not do it in release build
#define VERIFY_SS_NULL 0
#endif

/// sorting sounds by loudness
struct SortSound
{
#if VERIFY_SS_NULL
  Link<AbstractWave> _wave;
#else
  AbstractWave *_wave;
#endif
  float _loudness;

  SortSound(){}
  explicit SortSound(AbstractWave *wave);
  AbstractWave *operator ->() const {return _wave;}
  operator AbstractWave *() const {return _wave;}
  
  float Loudness() const {return _loudness;}
};

#if VERIFY_SS_NULL
  TypeIsGeneric(SortSound);
#else
  TypeIsSimple(SortSound);
#endif


SortSound::SortSound(AbstractWave *wave)
:_wave(wave)
{
  _loudness=_wave->GetLoudness();
}

/// verify there are no nulls left
static inline bool CheckNoNull(const Array<SortSound> &waves)
{
#if VERIFY_SS_NULL
  for (int i=0; i<waves.Size(); i++)
  {
    if (waves[i]._wave==NULL) return false;
  }
#endif
  return true;
}


static int CompareSounds( const SortSound *w0, const SortSound *w1 )
{
  float diff=w0->_loudness-w1->_loudness;
  if( diff>0 ) return -1;
  if( diff<0 ) return +1;
  return 0;
}

class HistoryTracker
{ // Value type should be movable
  struct HistoryItem
  {
    float value;
    float howOld;
    ClassIsSimple(HistoryItem);
  };
  AutoArray<HistoryItem> _history;
  float _maxTime;
  int _maxItems;

  public:
  HistoryTracker( float maxTime, int maxItems );
  void Add( float value, float howOld=0 );
  void Evaluate
  (
    float &max, float &min, float &avg,
    float minOld=0, float maxOld=FLT_MAX
  );
  void SetLimits( float maxTime, int maxItems )
  {
    _maxTime=maxTime;
    _maxItems=maxItems;
    GuardLimits();
  }
  void AdvanceTime( float deltaT );
  void GuardLimits();
};

HistoryTracker::HistoryTracker( float maxTime, int maxItems )
{
  _maxTime=maxTime;
  _maxItems=maxItems;
}

void HistoryTracker::GuardLimits()
{
  while
  (
    _history.Size()>_maxItems
    || _history.Size()>0 && _history[0].howOld>_maxTime
  )
  {
    _history.Delete(0);
  }
}

void HistoryTracker::AdvanceTime( float deltaT )
{
  for( int i=0; i<_history.Size(); i++ ) _history[i].howOld+=deltaT;
  GuardLimits();
}

void HistoryTracker::Add( float value, float howOld )
{
  HistoryItem item;
  item.value=value;
  item.howOld=howOld;
  _history.Add(item);
  GuardLimits();
}

void HistoryTracker::Evaluate
(
  float &max, float &min, float &avg,
  float minOld, float maxOld
)
{
  float lMax=FLT_MIN;
  float lMin=FLT_MAX;
  float sum=0;
  int n=0;
  for( int i=0; i<_history.Size(); i++ )
  {
    const HistoryItem &item=_history[i];
    if( item.howOld>maxOld || item.howOld<minOld ) continue;
    float value=item.value;
    sum+=value;
    n++;
    saturateMin(lMin,value);
    saturateMax(lMax,value);
  }
  min=lMin;
  max=lMax;
  avg=( n>0 ? sum/n : avg );
}

bool SoundScene::CanBeAudible(WaveKind kind, Vector3Par pos, float volume) const
{
  // TODO: implement relative muting as well
  // estimate volume of this sound
  float est = GSoundsys->GetLoudness(kind,pos,volume,_earAccomodation);
  return est>=LimitMuteMinimal && est>=_limitMuteAbsolute;
}

bool SoundScene::CanBeAudible(Vector3Par pos, float distance) const
{
  float listEmDist = GSoundsys->GetListenerPosition().Distance(pos);
  return (listEmDist < distance);
}

#define _EAR_WHISTLING 0

#include "AI/ai.hpp"

void SoundScene::PerformWhistle(bool activate, bool paused)
{
  if (activate && !_whistle._active)
  {
    // no sound when player in vehicle
    Transport *tr = GWorld->FocusOn() ? GWorld->FocusOn()->GetVehicleIn() : NULL;
    if (tr) return;

    _whistle._init = true;
    _whistle._active = true;

    _whistle._sound.Free();
    // create sound, outside soundscene management
    _whistle._sound = GSoundsys->CreateWave(_whistle._soundPars.name, false, false);

    if (_whistle._sound)
    {
      _whistle._sound->SetVolume(_whistle._soundPars.vol, 1, true);
      _whistle._sound->Repeat(1);
      _whistle._sound->Play();    // start
    }
  }

  if (_whistle._active)
  {    
    if (paused)
    {
      if (_whistle._sound) _whistle._sound->Stop();
    }
    else
    {
      if (_whistle._sound) _whistle._sound->Play();
    }

    // starting values initialization
    if (_whistle._init)
    {
      _whistle._downTime = Glob.time + _whistle._down;
      _whistle._silentTime = Glob.time + _whistle._silent;
      _whistle._upTime = _whistle._silentTime + _whistle._up;
      _whistle._init = false;
    }

    if (Glob.time < _whistle._downTime)
    {
      // go down with volume
      _whistle._volume = (_whistle._downTime.toFloat() - Glob.time.toFloat()) * 1.0f/_whistle._down;
    }
    else if (Glob.time < _whistle._silentTime)
    {
      // silent part
      _whistle._volume = 0;
    }
    else if (Glob.time < _whistle._upTime)
    {
      // up with volume
      _whistle._volume = 1 - ((_whistle._upTime.toFloat() - Glob.time.toFloat()) * 1.0f/_whistle._up);
      // down with whistle sound
      if (_whistle._sound) _whistle._sound->SetVolume(_whistle._sound->GetVolume() * (1 - _whistle._volume), 1, true);
    }
    else
    {
      // deactivate
      _whistle._volume = 1;
      _whistle._init = false;
      _whistle._active = false;
      _whistle._sound.Free();
    }
  }
}

inline static bool EvalWhistlingSound(const Vector3 &listPos, AbstractWave *wave)
{
  if (wave && wave->CanCauseWhistling())
  {
    // distance from source, test only once per set 
    return ( wave->WhistlingDist() > listPos.Distance(wave->GetWhPos()) );
  }

  return false;
}

UITime SoundScene::GetAudioTime() const
{
  return _audioTime;
}

/*!
\patch 1.34 Date 12/03/2001 by Ondra
- Fixed: When radio volume was turned off in sound control panel,
messages were not transmitted at all.
\patch_internal 1.47 Date 3/11/2002 by Ondra
- Optimized: Inaudible sounds are muted in less distance from the camera.
\patch 1.79 Date 7/29/2002 by Ondra
- Improved: Compression of sound dynamic range less aggressive.
\patch 5123 Date 1/24/2007 by Ondra
- Fixed: Silent sound cut off too early when load sound was near
\patch 5128 Date 2/15/2007 by Ondra
- Fixed: Dedicated server radio processing was broken, resulting in AI often not doing what expected.
*/
void SoundScene::AdvanceAll( float deltaT, bool paused, bool quiet )
{
  PROFILE_SCOPE_EX(ssAdv,sound);
  
  if (!paused)
  {
    _audioTime += deltaT;
  }

#if _EAR_WHISTLING
  bool whistle = false;
  const Vector3 &listPos = GSoundsys->GetListenerPosition();
#endif

  // Mastering voice simulation
  GSoundsys->UpdateGlobalSoundVolume(_soundVolume);
  GSoundsys->UpdateGlobalRadioVolume(_radioVolume);
  GSoundsys->UpdateGlobalSpeechExVolume(_speechExVolume);
  GSoundsys->SimulateMasterVoice(deltaT, false/*GWorld->IsCameraInside()*/);

  int i;  
  for( i=0; i<_globalSounds.Size(); )
  { 
    // remove terminated global sounds
    AbstractWave *wave=_globalSounds[i];

    if( !wave || wave->CanBeDeleted())
    {
      _globalSounds.Delete(i);
    }
    else
    {
#if _EAR_WHISTLING
      if (!whistle) whistle = EvalWhistlingSound(listPos, wave);
      if (_whistle._active) wave->SetEarWhistling(_whistle._volume, true);      
#endif
      i++;
    }
  }
  _all3DSounds.RemoveNulls();
  _all2DSounds.RemoveNulls();
  
  bool playStickyOnly = paused;
  // on dedicated server we need to pretend we play all sounds
  // however we do not want any sound to be really played
  bool playMuted = quiet;
#if _ENABLE_DEDICATED_SERVER
  if (IsDedicatedServer()) playMuted = true;
#endif
  //@{ _DEDICATED_CLIENT
  if ( IsDedicatedClient() )
    playMuted = true;
  //@}

  AUTO_STATIC_ARRAY(SortSound,sort,128);
  // sort sounds by intensity (as heard by the listener) 
  for( i=0; i<_all3DSounds.Size(); i++ )
  {
    AbstractWave *wave=_all3DSounds[i];
    DoAssert( wave );
    if (!wave) continue;

#if _EAR_WHISTLING
    if (!whistle) whistle = EvalWhistlingSound(listPos, wave);
    if (_whistle._active) wave->SetEarWhistling(_whistle._volume, true);      
#endif

    SortSound ss(wave);
    if (!playStickyOnly || wave->GetSticky())
    {
      if (ss.Loudness()>=LimitMuteMinimal && !wave->IsWaiting() && !playMuted && !wave->IsTerminated())
      {
        sort.Add(ss);
      }
      else
      {        
        if (wave->AccomodationEnabled())
        {
          wave->SetAccomodation(_earAccomodation);
        }
        wave->Stop();
        wave->Skip(deltaT); // waiting is also stopped
      }
    }
    else
    {
      // Stop only - no need to skip, time is paused
      wave->Stop();
    }
  }
  // play only first N 3D sounds - all other should be stopped and advanced
  QSort(sort.Data(),sort.Size(),CompareSounds);

  DoAssert(CheckNoNull(sort));

  // any sound which is below a certain fraction of total power cannot be audible
  // to avoid frequent stopping / playing, use hysteresis
  float limitPlayAbsolute = LimitPlayMinimal; // -80 dB
  float limitMuteAbsolute = LimitMuteMinimal; // -83 dB
  
  int maxPlayedSamples = GSoundsys->GetSamplesCount();

  int played=sort.Size();
  if (played>maxPlayedSamples) played = maxPlayedSamples;

  // compute total loudness of all sounds which are "played"
  /*
  float playedLoudness = 0;
  for (i=0; i<played; i++)
  {
    AbstractWave *wave=sort[i];
    if (wave->AccomodationEnabled())
    {
      wave->SetAccomodation(_earAccomodation);

      float loudness = sort[i].Loudness();

      // no gain possible - perceived loudness cannot be over 1
      if (loudness>1) { loudness = 1; }

      if (loudness<playedLoudness * LimitPlayRelative)
      {
        limitPlayAbsolute = playedLoudness*LimitPlayRelative;
        limitMuteAbsolute = playedLoudness*LimitMuteRelative;
        break;
      }

      playedLoudness += loudness;
    }
  }*/
  
  _limitMuteAbsolute = limitMuteAbsolute;
  
  DoAssert(CheckNoNull(sort));
  
  // first stop all channels that should be muted
  for( i=played; i<sort.Size(); i++ )
  {
    AbstractWave *wave=sort[i];
    wave->Stop();
    wave->Skip(deltaT);
    // we need to recalculate accommodation - sound may become audible now
    if( wave->AccomodationEnabled() )
    {
      wave->SetAccomodation(_earAccomodation);
    }

    DoAssert(CheckNoNull(sort));
  }
  
  #if _ENABLE_CHEATS
  static InitVal<int,-1> handles3D[100];
  static InitVal<int,-1> handles2D[100];
  static InitVal<int,-1> handlesEnv[100];

  if (CHECK_DIAG(DESound))
  {
    DIAG_MESSAGE(
      200,
      Format(
        "global: %d, all 3D %d, mute limit %.1f dB",_globalSounds.Size(),_all3DSounds.Size(),
        log10(_limitMuteAbsolute)*20
      )
    );
  }
  #endif

  //DIAG_MESSAGE(100, Format("play: %.5f, mute: %.5f", limitPlayAbsolute, limitMuteAbsolute));

  float totLoudness=0;
  for (i=0; i<played; i++ )
  {
    AbstractWave *wave=sort[i];
    
    // update waves w/o handles
    if (wave->AutoUpdateEnabled()) wave->UpdatePosition();

    if( wave->AccomodationEnabled() )
    {
      wave->SetAccomodation(_earAccomodation);
      float loudness=sort[i].Loudness();
      totLoudness+=loudness;
      // check if we need to stop/start the channel
      // we cannot play more than played
      // in typical scenarios we want to play less
  
      const char *state = "=";
      if (loudness>limitPlayAbsolute)
      {
        RString name = wave->Name();
        state = "+";
        wave->Play();
        DoAssert(CheckNoNull(sort));
      }
      else if(loudness<limitMuteAbsolute)
      {
        state = "-";
        wave->Stop();
        DoAssert(CheckNoNull(sort));
      }
      else
      {
        // if wave is playing, let it play
        // if it is stopped, let it be stopped
        // we might want to prepare the sound, but it is not necessary
        //wave->PreparePlaying();
      }

      #if _ENABLE_CHEATS
        if (CHECK_DIAG(DESound))
        {
          if (wave->Get3D())
          {
            Vector3Val pos = GSoundsys->GetListenerPosition();
            GlobalDiagMessage(
              handles3D[i],0,200,
              Format(
                " %s - %.1f m (%.1f m), dB %.1f, %s%s, obs %.1f/%.1f dB %s",
                (const char *)wave->Name(),
                wave->GetLoudness(),wave->GetPosition().Distance(pos),
                log10(loudness)*20,state,wave->IsStopped() ? "m" : "p",
                log10(wave->GetObstruction())*20,log10(wave->GetOcclusion())*20,cc_cast(wave->GetVolumeDiag())
              )
            );
          }
          else
          {
            GlobalDiagMessage(
              handles3D[i],0,200,
              Format(
                "i3D %s, vol %.1f, dB %.1f, %s%s, obs %.1f/%.1f dB %s",
                (const char *)wave->Name(),
                wave->GetVolume(),
                log10(loudness)*20,state,wave->IsStopped() ? "m" : "p",
                log10(wave->GetObstruction())*20,log10(wave->GetOcclusion())*20,cc_cast(wave->GetVolumeDiag())
              )
            );
          }
        }
      #endif
    }
    else
    {
      #if _ENABLE_CHEATS
        float loudness=sort[i].Loudness();
        if (CHECK_DIAG(DESound))
        {
          GlobalDiagMessage(
            handles3D[i],0,200,Format(
              " 3D %s - %.4f, dB %.1f %s",(const char *)wave->Name(),loudness,
              log10(loudness)*20,cc_cast(wave->GetVolumeDiag())
            )
          );
        }
      #endif
      wave->Play();
      DoAssert(CheckNoNull(sort));
    }
    
    if( wave->IsStopped() )
    { // there may be no sound system channels left
      wave->Skip(deltaT);
      DoAssert(CheckNoNull(sort));
    }
  }

  // play all 2D sounds
  for( i=0; i<_all2DSounds.Size(); i++ )
  {
    AbstractWave *wave=_all2DSounds[i];

#if _EAR_WHISTLING
    if (!whistle) whistle = EvalWhistlingSound(listPos, wave);
    if (_whistle._active) wave->SetEarWhistling(_whistle._volume, true);      
#endif

    if( wave->AccomodationEnabled() )
    {
      wave->SetAccomodation(_earAccomodation);
      float loudness=wave->GetVolume()/wave->Distance2D();
      totLoudness+=loudness;
      #if _ENABLE_CHEATS
        if (CHECK_DIAG(DESound))
        {
          GlobalDiagMessage(
            handles2D[i],0,200,Format(
              " 2D %s - %.4f, dB %.2f, obs %.2f/%.2f %s",
              (const char *)wave->Name(),wave->GetVolume()*10,log10(loudness)*20,
              log10(wave->GetObstruction())*20,log10(wave->GetOcclusion())*20,cc_cast(wave->GetVolumeDiag())
            )
          );
        }
      #endif
    }
    else if (wave->GetKind()==WaveSpeech)
    {
      // we need to respect natural volume of the wave
      // this is 1 (normal) or 0 (muted)
      if (wave->GetVolume()>0)
      {
        wave->SetVolume(GetRadioVolume());
      }
    }
    else if (wave->GetKind() == WaveSpeechEx)
    {
      if (wave->GetVolume()>0)
      {
#if _ENABLE_CHEATS
        if (CHECK_DIAG(DESound))
        {
          GlobalDiagMessage(handles2D[i], 0, 200, 
            Format(" Ex %s - %.4f, dB %.2f, ", (const char *)wave->Name(), wave->GetVolume(), _speechExVolume));
        }
#endif
        wave->SetVolume(_speechExVolume);
      }
    }
    if ( !playStickyOnly || wave->GetSticky() )
    {
      if (wave->GetVolume()>0 && !playMuted)
      {
        wave->Play();
      }
      else
      {
        // no need to play muted sound - it is enough to advance them
        wave->Stop();
      }
      wave->Advance(deltaT); // advance sounds that may be waiting
    }
    else
    {
      wave->Stop();
    }
  }

  // environment sounds - 2d
  for (i = 0; i < _envSounds.Size(); i++)
  {
    AbstractWave *wave = _envSounds[i];
    if (!wave) continue;

#if _EAR_WHISTLING
    if (!whistle) whistle = EvalWhistlingSound(listPos, wave);
    if (_whistle._active) wave->SetEarWhistling(_whistle._volume, true);      
#endif

    if (wave->AccomodationEnabled()) 
    {
      wave->SetAccomodation(_earAccomodation);
      float loudness = wave->GetVolume()/wave->Distance2D();
      totLoudness += loudness;

#if _ENABLE_CHEATS
      if (CHECK_DIAG(DESound))
      {
        DIAG_MESSAGE_ID(200, i, "En %s - %.4f, dB %.2f", (const char*) wave->Name(), loudness, log10(loudness)*20);
      }
#endif
    }

    if (playStickyOnly || wave->IsTerminated())
    {
      wave->Unload();
      _envSounds[i].Free();
    }
  }

  // advanced random environment sounds - 3d
  for (i = 0; i < _envSoundsExt.Size(); i++)
  {
    AbstractWave *wave=_envSoundsExt[i];
    if (!wave) continue;
    
#if _EAR_WHISTLING
    if (!whistle) whistle = EvalWhistlingSound(listPos, wave);
    if (_whistle._active) wave->SetEarWhistling(_whistle._volume, true);      
#endif

    if (wave->AccomodationEnabled())
    {
      wave->SetAccomodation(_earAccomodation);
      float loudness = wave->GetLoudness();
      totLoudness += loudness;

      // force 3d calculation - by set position - position si same but channel ratio is changed
      wave->SetPosition(wave->GetPosition(), VZero, true);

      if (loudness <= LimitMuteMinimal)
      {
        wave->Stop();
        wave->Skip(deltaT);
      }

#if _ENABLE_CHEATS
      if (CHECK_DIAG(DESound))
      {
        DIAG_MESSAGE_ID(200, i, "AdvEn %s - %.4f, dB %.2f", cc_cast(wave->Name()), loudness, log10(loudness)*20);
      }
#endif
    }

    if (playStickyOnly || wave->IsTerminated())
    {
      wave->Unload();
      _envSoundsExt[i].Free();
    }
  }

  if (!playStickyOnly || _musicTrack && _musicTrack->GetSticky())
  {
    // musicTrack playback (Play/Stop) is controlled the same way
    // as any other 2D sound (musicTrack is listed in _all2DSounds)
    // simulate fade in/out
    if (Glob.time>=_musicTargetTime)
    {
      _musicVolume = _musicTargetVolume;
    }
    else
    {
      float timeRest = _musicTargetTime-Glob.time;
      float chSpeed = (_musicTargetVolume-_musicVolume)/timeRest;
      _musicVolume += deltaT*chSpeed;
      saturateMax(_musicVolume,0);
    }
#if _ENABLE_CHEATS
    if (CHECK_DIAG(DESound) && _musicTrack)
    {
      float loudness = _musicTrack->GetLoudness();
      DIAG_MESSAGE(
        200,Format("Music %s - %.4f - %.4f",(const char *)_musicTrack->Name(),loudness,log10(loudness))
        );
    }
#endif
  }
  if (!playStickyOnly)
  {
    // simulate fade in/out
    if (Glob.time>=_soundTargetTime)
    {
      _soundVolume = _soundTargetVolume;
    }
    else
    {
      float timeRest = _soundTargetTime-Glob.time;
      float chSpeed = (_soundTargetVolume-_soundVolume)/timeRest;
      _soundVolume += deltaT*chSpeed;
      saturateMax(_soundVolume,0);
    }

    // simulate fade in/out
    if (Glob.time>=_radioTargetTime)
    {
      _radioVolume = _radioTargetVolume;
    }
    else
    {
      float timeRest = _radioTargetTime-Glob.time;
      float chSpeed = (_radioTargetVolume-_radioVolume)/timeRest;
      _radioVolume += deltaT*chSpeed;
      saturateMax(_radioVolume,0);
    }
    // simulate fade in/out
    if (Glob.time >= _speechExTargetTime)
    {
      _speechExVolume = _speechExTargetVolume;
    }
    else
    {
      float timeRest = _speechExTargetTime-Glob.time;
      float chSpeed = (_speechExTargetVolume-_speechExVolume)/timeRest;
      _speechExVolume += deltaT*chSpeed;
      saturateMax(_speechExVolume, 0);
    }
  }
  if (_musicTrack)
  {
    _musicTrack->SetVolume
      (
      _musicInternalVolume*_musicVolume,_musicInternalFrequency
      );
  } 
#if _EAR_WHISTLING
  // ear whistling
  
  {
    PerformWhistle(whistle, paused || quiet);
  }
#endif


  // compress dynamics
  // total loudness is calculated
  // adjust volume to accomodate

  // aggregated loudness
  static HistoryTracker history(2.5f,250);
  history.Add(totLoudness);

  float maxLoud,minLoud,avgLoud;
  history.AdvanceTime(deltaT);
  history.Evaluate(maxLoud,minLoud,avgLoud);

  float aggLoud = maxLoud;
  float compressedLoudness=floatMax(aggLoud,1e-20);

  //float cLoudnessWanted=0.4;
  // very loud volume should be still loud after compression
  // very soft volume should be still soft after compression
  // 10->2
  // 0.1->0.4
  // ideal loudness is somewhere around 1?
  //float cLoudnessWanted = Interpolativ(aggLoud,0.01,10,0.1,2);
  float cLoudnessWanted = Interpolativ(aggLoud,0.01,10,0.1,4);

  float accLoudness = compressedLoudness*_earAccomodation*(1.0/cLoudnessWanted);
  float logAccLoudness=-1000;
  if( accLoudness>0 ) logAccLoudness=-log(accLoudness);
  saturate(logAccLoudness,-0.05f*deltaT,+0.25f*deltaT);

  float eaWanted = _earAccomodation*exp(logAccLoudness);

  //DIAG_MESSAGE(200, Format("ea: %.4f, eaW: %.4f, expLogAccL: %f.", _earAccomodation, eaWanted, exp(logAccLoudness)));

#if _ENABLE_CHEATS
  if (CHECK_DIAG(DESound))
  {
    DIAG_MESSAGE
      (
      200,Format(
      "Log Acc %.4f, absV %.4f->%.4f, want %.4f",
      log10(_earAccomodation),log10(aggLoud),log10(aggLoud*_earAccomodation),
      log10(cLoudnessWanted)
      )
      );
    DIAG_MESSAGE
      (
      200,Format(
      "Lin Acc %.4f, absV %.4f->%.4f, want %.4f",
      _earAccomodation,aggLoud,aggLoud*_earAccomodation,
      cLoudnessWanted
      )
      );
  }
#endif

  const float maxEarAccomodation=4;
  const float minEarAccomodation=1.0/256;
  saturate(eaWanted,minEarAccomodation,maxEarAccomodation);
  _earAccomodation=eaWanted;
}

AbstractWave *SoundScene::OpenAndPlay
(
  const char *filename, EntityAI *source, bool isInside, 
  Vector3Par pos, Vector3Par speed,
  float volume, float frequency, float distance
)
{
  AbstractWave *wave=Open(filename);
  if( wave )
  {
    wave->SetVolume(volume,frequency);
    wave->SetMaxDistance(distance);
    wave->SetPosition(pos,speed,true);
    float obstruction = 1,occlusion = 1;
    // we assume this must be some kind of external sound
    GWorld->CheckSoundObstruction(source,isInside,obstruction,occlusion);
    wave->SetObstruction(obstruction,occlusion,0);
  }
  return wave;
}
AbstractWave *SoundScene::OpenAndPlay(
  const char *filename,
  Vector3Par pos, Vector3Par speed,
  float volume, float frequency, float distance
)
{
  AbstractWave *wave=Open(filename);
  if( wave )
  {
    wave->SetVolume(volume,frequency);
    wave->SetMaxDistance(distance);
    wave->SetPosition(pos,speed,true);
  }
  return wave;
}
AbstractWave *SoundScene::OpenAndPlayOnce(
  const char *filename, EntityAI *source, bool isInside, 
  Vector3Par pos, Vector3Par speed,
  float volume, float frequency, float distance
)
{
  AbstractWave *wave=Open(filename);
  if( wave )
  {
    wave->SetVolume(volume,frequency);
    wave->SetPosition(pos,speed,true);
    float obstruction = 1,occlusion = 1;
    // we assume this must be some kind of external sound
    GWorld->CheckSoundObstruction(source,isInside,obstruction,occlusion);
    wave->SetObstruction(obstruction,occlusion,0);
    wave->SetMaxDistance(distance);
    wave->Repeat(1);
  }
  return wave;
}

#include "scene.hpp"
#include "camera.hpp"
#include "engine.hpp"

/*!
\patch 1.28 Date 10/19/2001 by Ondra
- Fixed: no speed of sound delay on any sounds (bug since 1.27)
*/

void SoundScene::SimulateSpeedOfSound(AbstractWave *sound)
{
  float distance=sound->GetPosition().Distance(GScene->GetCamera()->Position());
  
  const float MinDistance = SpeedOfSound / 10;
  if (distance < MinDistance) return; // no skip for small distance - for shooting audio update not react fast enough

  // sound will not start playing between frames
  // it is therefore more accurate to subtract half of expected frame duration
  // from wait time. This way we are effectively rounding to nearest frame instead
  // of rounding to next one
  float halfFrame = GEngine->GetAvgFrameDuration(4)*(0.001f*0.5f);
  float skip = -distance*InvSpeedOfSound + halfFrame;
  // avoid skipping sound start, only delay is allowed
  if (skip<0) sound->Skip(skip);
}

AbstractWave *SoundScene::OpenAndPlayOnce2D(
  const char *filename, float volume, float frequency, bool accomodate
)
{
  AbstractWave *handle=Open(filename,false,accomodate);
  if( handle )
  {
    if (accomodate)
    {
      // if accommodation is enabled, it is some kind of environment sound
      float obstruction = 1,occlusion = 1;
      GWorld->CheckSoundObstruction(NULL,false,obstruction,occlusion);
      handle->SetObstruction(obstruction,occlusion,0);
    }
    handle->SetVolume(volume,frequency,true);
    handle->Repeat(1);
  }
  return handle;
}

AbstractWave *SoundScene::OpenAndPlay2D
(
  const char *filename, float volume, float frequency, bool accomodate
)
{
  AbstractWave *handle=Open(filename,false,accomodate);
  if( handle )
  {
    handle->SetVolume(volume,frequency,true);
  }
  return handle;
}

void SoundScene::Reset()
{
  // first of all stop all sounds
  for( int i=0; i<_envSounds.Size(); i++ )
  {
    if (_envSounds[i]) _envSounds[i]->Unload();
  }
  for (int i = 0; i < _envSoundsExt.Size(); i++)
  {
    if (_envSoundsExt[i]) _envSoundsExt[i]->Unload();
  }
  for( int i=0; i<_globalSounds.Size(); i++ )
  {
    if (_globalSounds[i]) _globalSounds[i]->Unload();
  }
  if (_musicTrack)
  {
    _musicTrack->Unload();
  }
  _earAccomodation = DefaultEarAccomodation;
  // all sounds stopped, we can free them now
  for( int i=0; i<_envSounds.Size(); i++ )
  {
    _envSounds[i].Free();
  }
  _envSounds.Clear();
  _envSoundsExt.Clear();
  _globalSounds.Clear();

  _whistle._active = false;
  _whistle._init = false;
  _whistle._volume = 1;
  _whistle._sound.Free();

  _musicVolume = 0.5; // middle value
  _musicTargetVolume = 0.5;
  if (_musicTrack)
  {
    _musicTrack->Unload();
    _musicTrack.Free(); // remove musical track
  }
  _musicTargetTime = TIME_MIN;

  _soundVolume = 1; // middle value
  _soundTargetVolume = 1;
  _soundTargetTime = TIME_MIN;
  
  _soundGain = 1;

  _radioVolume = 1; // middle value
  _radioTargetVolume = 1;
  _radioTargetTime = TIME_MIN;

  _speechExVolume = 1.0f;
  _speechExTargetVolume = 1.0f;
  _speechExTargetTime = TIME_MIN;
}

const float EnvMagicConstant = 30;

float SoundScene::GetSpeechExVolume() const { return _speechExVolume; }

void SoundScene::SetSpeechExVolume(float vol, float time)
{
  _speechExTargetVolume = vol;
  _speechExTargetTime = Glob.time + time;
  if (time <= 0.01f)
  {
    _speechExVolume =_speechExTargetVolume;
  }
}

float SoundScene::GetMusicVolume() const
{
  return _musicTargetVolume;
}

void SoundScene::SetMusicVolume(float vol, float time)
{
  _musicTargetVolume = vol;
  _musicTargetTime = Glob.time+time;
  if (time<=0.01)
  {
    _musicVolume = _musicTargetVolume;
  }
}

float SoundScene::GetSoundVolume() const
{
  return _soundTargetVolume;
}

void SoundScene::SetSoundVolume(float vol, float time)
{
  _soundTargetVolume = vol;
  _soundTargetTime = Glob.time+time;
  if (time<=0.01)
  {
    _soundVolume = _soundTargetVolume;
    saturate(_soundVolume, 0.0f, 1.0f);
  }
}

float SoundScene::GetRadioVolume() const
{
  return _radioTargetVolume;
}

void SoundScene::SetRadioVolume(float vol, float time)
{
  _radioTargetVolume = vol;
  _radioTargetTime = Glob.time+time;
  if (time<=0.01)
  {
    _radioVolume = _radioTargetVolume;
  }
}

void SoundScene::StartMusicTrack(const SoundPars &pars, float from)
{
  AbstractWave *wave = GSoundScene->OpenAndPlayOnce2D
  (
    pars.name,pars.vol,pars.freq,false
  );
  if (!wave) return;
  wave->SetKind(WaveMusic);
  // register is at musical track
  _musicTrack=wave;
  _musicInternalVolume = pars.vol;
  _musicInternalFrequency = pars.freq;
  wave->Skip(from);
}

void SoundScene::StopMusicTrack()
{
  // TODO: fade out
  _musicTrack.Free();
}

/*!
\patch 1.43 Date 1/31/2002 by Ondra
- Fixed: Long environmental sounds streaming was not working.
*/

void SoundScene::SetEnvSound(int i, const SoundPars &pars, float volume, float obstruction, float occlusion)
{
  PROFILE_SCOPE_EX(ssEnS,sound);

  // see if sound can be reused
  _envSounds.Access(i);

  if (pars.name.GetLength()==0 || volume*pars.vol<1e-6f)
  {
    _envSounds[i].Free();
    return;
  }

  DoAssert(pars.name.IsLowCase());
  
  AbstractWave *snd=_envSounds[i];
  if( !snd || strcmp(snd->Name(), pars.name))
  {
    snd = GSoundsys->CreateWave(pars.name, false);
    _envSounds[i] = snd;
  }
  if (snd)
  {
    // sound matched
    // set volume and return
    //snd->Set3D(false);
    snd->SetVolume(volume*pars.vol,pars.freq);
    snd->SetAccomodation(_earAccomodation);
    snd->SetObstruction(obstruction,occlusion);
    // call play, this ensures streaming sounds are streaming
    snd->Play();
  }
}

void SoundScene::AdvanceEnvSounds(int totalCount)
{
  for( int i=totalCount; i<_envSounds.Size(); i++ )
  {
    // sounds that were not renewed die
    if (_envSounds[i])
    {
      _envSounds[i].Free();
    }
  }
  _envSounds.Resize(totalCount);

  for (int i = totalCount; i < _envSoundsExt.Size(); i++)
  {
    if (_envSoundsExt[i])
    {
      _envSoundsExt[i].Free();
    }
  }
  _envSoundsExt.Resize(totalCount);
}

void SoundScene::SetEnvSoundExt(int i, const SoundPars &pars, Vector3 &pos, float volume, float obstruction, float occlusion)
{
  // empty sound pars
  if (pars.name.GetLength() == 0)
    return;

  // see if sound can be reused
  _envSoundsExt.Access(i);

  // sample should be stopped
  if (_envSoundsExt[i].NotNull())
  {
    _envSoundsExt[i].Free();
  }

  AbstractWave *snd = _envSoundsExt[i];
  if (!snd || strcmpi(snd->Name(), pars.name))
  {
    snd = GSoundsys->CreateWave(pars.name, true);
    _envSoundsExt[i] = snd;
  }
  if (snd)
  {
    snd->Repeat();
    snd->Set3D(true);
    snd->EnableAccomodation(true);
    snd->SetVolume(volume, pars.freq);
    snd->SetMaxDistance(pars.distance);
    snd->SetAccomodation(_earAccomodation);
    snd->SetObstruction(obstruction, occlusion);
    snd->SetPosition(pos, VZero);
    snd->Play();
  }
}

AbstractWave *SoundScene::SayPause( float duration )
{
  AbstractWave *wave = GSoundsys->CreateEmptyWave(duration);
  if (wave)
  {
    _all2DSounds.Add(wave);
  }
  return wave;
}

LSError SoundScene::SerializeState(ParamArchive &ar)
{
  ar.Serialize("soundTargetVolume", _soundTargetVolume, 1, 1.0f);
  ar.Serialize("speechTargetVolume", _radioTargetVolume, 1, 1.0f);
  ar.Serialize("musicTargetVolume", _musicTargetVolume, 1, 1.0f);
  ar.Serialize("speechExTargetVolume", _speechExTargetVolume, 1, 1.0f);

  return LSOK;
}

