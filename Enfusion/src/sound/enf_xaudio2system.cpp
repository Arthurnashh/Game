//-----------------------------------------------------------------------------
// File: enf_xaudio2system.cpp
//
// Desc: Default XAudio2 sound system
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#include "sound/enf_xaudio2system.h"
#include "common/enf_parser.h"
#include "Xaudio2fx.h"
#include "tinyxml/tinyxml.h"
#include "system/enf_profiler.h"
#include "common/enf_diag.h"
#include "enf_main.h"

#ifdef ENF_XENON
	#include <XMAHardwareAbstraction.h>
#endif

#if defined(_DEBUG) && defined(ENF_XENON)
	#define ENF_DEBUG_XAUDIO2
#endif

namespace enf
{

// volume curve definition
static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_Volume_CurvePoints[5] = { 
	0.0f, 1.0f,
	0.25f, 0.8f,
	0.5f, 0.5f,
	0.75f, 0.1f,
	1.0f, 0.0f
};
static const X3DAUDIO_DISTANCE_CURVE       Emitter_Volume_Curve          = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_Volume_CurvePoints[0], 5 };

static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_LFE_CurvePoints[5] = { 0.0f, 0.55f, 0.0625f, 0.25f, 0.125f, 0.1f, 0.25f, 0.05f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE       Emitter_LFE_Curve          = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_LFE_CurvePoints[0], 5 };


//-----------------------------------------------------------------------
XAudio2SoundEnv::XAudio2SoundEnv(CStr name): XSoundEnv(name)
{
	ZeroMem(&m_Properties, sizeof(m_Properties));
}

//-----------------------------------------------------------------------
XAudio2System::SoundScene::SoundScene():
	m_iFlags(0),
	m_pReverbSubmixVoice(NULL),
	m_pReverbEffect(NULL),
	m_fSfxVolume(1.0f),
	m_fMusicVolume(1.0f),
	m_fLocalSfxVolume(1.0f),
	m_fLocalMusicVolume(1.0f)
{
	ZeroMem(&m_Listener, sizeof(X3DAUDIO_LISTENER));
	ZeroMem(&m_ListenerCone, sizeof(X3DAUDIO_CONE));
	m_Listener.pCone = &m_ListenerCone;
	static const XAUDIO2FX_REVERB_I3DL2_PARAMETERS defaultParam = XAUDIO2FX_I3DL2_PRESET_DEFAULT;
	m_ReverbParameters = defaultParam;
	MemStats::m_iSoundSystem += sizeof(*this);
}

//-----------------------------------------------------------------------
XAudio2System::SoundScene::~SoundScene()
{
	MemStats::m_iSoundSystem += sizeof(*this);
}

//-----------------------------------------------------------------------
XSoundSystem* CreateXAudio2System()
{
	return ENF_NEW XAudio2System();
}

//-----------------------------------------------------------------------
void XAudio2System::Commit(uint operSet)
{
	m_pXAudio2->CommitChanges(operSet);
}

//-----------------------------------------------------------------------
void XAudio2System::SetListener(uint sceneIndex, const ListenerParameters& params)
{
	SoundScene* ss = GetScene(sceneIndex);

	enf_assert2(fabs(1.0f - (params.m_vForward.CrossProduct(params.m_vUp)).Size()) < 0.01f, "Listener matrix is not ortho-normalized!");
	
	//check changes and rise flags for them
	X3DAUDIO_LISTENER	tempListener;
	tempListener.OrientFront = Vec3(params.m_vForward);
	tempListener.OrientTop = Vec3(params.m_vUp);
	tempListener.Position = Vec3(params.m_vPos);
	tempListener.Velocity = Vec3(params.m_vVelocity);
	tempListener.pCone = ss->m_Listener.pCone;
	if(memcmp(&ss->m_Listener, &tempListener, sizeof(tempListener)) != 0)
	{
		memcpy(&ss->m_Listener, &tempListener, sizeof(tempListener));
		ss->m_iFlags |= SSF_UPDATE_LISTENER;
	}

	X3DAUDIO_CONE	tempCone;
	tempCone.InnerAngle = params.m_Cone.m_fInnerAngle;
	tempCone.OuterAngle = params.m_Cone.m_fOuterAngle;
	tempCone.InnerVolume = params.m_Cone.m_fInnerVolume;
	tempCone.OuterVolume = params.m_Cone.m_fOuterVolume;
	tempCone.InnerLPF = params.m_Cone.m_fInnerLPF;
	tempCone.OuterLPF = params.m_Cone.m_fOuterLPF;
	tempCone.InnerReverb = params.m_Cone.m_fInnerReverb;
	tempCone.OuterReverb = params.m_Cone.m_fOuterReverb;	
	if(memcmp(&ss->m_ListenerCone, &tempCone, sizeof(tempCone)) != 0)
	{
		memcpy(&ss->m_ListenerCone, &tempCone, sizeof(tempCone));
		ss->m_iFlags |= SSF_UPDATE_LISTENER;
	}
}

//-----------------------------------------------------------------------
class XVoiceCallback : public IXAudio2VoiceCallback
{
protected:
	XAudio2System*	m_pSystem;

public:
	XVoiceCallback(XAudio2System* sys): m_pSystem(sys) {}
	~XVoiceCallback() {}


	STDMETHOD_(void, OnVoiceProcessingPassStart) (THIS_ UINT32 BytesRequired)
	{
	}
	STDMETHOD_(void, OnVoiceProcessingPassEnd) (THIS)
	{
	}
	STDMETHOD_(void, OnStreamEnd) (THIS)
	{
	}
	STDMETHOD_(void, OnBufferStart) (THIS_ void* pBufferContext)
	{
	}
	STDMETHOD_(void, OnBufferEnd) (THIS_ void* pBufferContext)
	{
		if(pBufferContext == NULL)
			return;

		CMutexLock lock(m_pSystem->m_EndedSetMutex);

		XSoundRef* soundRef = reinterpret_cast<XSoundRef*>(pBufferContext);
		XAudio2System::Sound* sound = static_cast<XAudio2System::Sound*>(soundRef->m_pSound);
		if(sound)
		{
			if(sound->IsFlagsSet(SFX_ONCE) && sound->m_pSourceVoice)
			{
				sound->m_pSourceVoice->Stop(0);
			}
			if(IL_READ(sound->m_State) == XAudio2System::SS_STOPPED)
				return; //these are OK
			m_pSystem->EnqueueEndedSound(soundRef);
		}
	}
	STDMETHOD_(void, OnLoopEnd) (THIS_ void* pBufferContext)
	{
		pBufferContext = pBufferContext;
	}
	STDMETHOD_(void, OnVoiceError) (THIS_ void* pBufferContext, HRESULT Error)
	{
	}
};

//-----------------------------------------------------------------------
void XAudio2System::EnqueueEndedSound(XSoundRef* ref)
{
	CMutexLock lock(m_EndedSetMutex);
	SoundScene* ss = GetScene(ref->m_iSceneIndex);
	enf_assert(!m_FreeSounds.Contains(ref));
	ss->m_EndedSounds.Insert(ref);
}

//-----------------------------------------------------------------------
XSoundRef* XAudio2System::DequeueEndedSound(uint sceneIndex)
{
	CMutexLock lock(m_EndedSetMutex);
	SoundScene* ss = GetScene(sceneIndex);
	if(!ss->m_EndedSounds.IsEmpty())
	{
		XSoundRef* ref = ss->m_EndedSounds.RemoveElement(ss->m_EndedSounds.GetCardinality() - 1);
		return ref;
	}
	return NULL;
}

//-----------------------------------------------------------------------
XAudio2System::Sound::~Sound()
{
	enf_assert(m_pSourceVoice == NULL);
}

//-----------------------------------------------------------------------
float XAudio2System::Sound::GetPosition() const
{
	SoundState state = GetState();
	if(state == SS_NOTSTARTED || state == SS_STOPPED)
	{
		return m_fTimePosition;
	}

	XAUDIO2_VOICE_STATE vstate;
	m_pSourceVoice->GetState(&vstate);

	return (float)vstate.SamplesPlayed / GetSample()->m_pFormat->m_Format.nSamplesPerSec;
}

//-----------------------------------------------------------------------
void XAudio2System::Sound::SetPosition(float position, XSoundRef* ref)
{
	if(m_pStreaming)
		return;

	XAudio2Sample* xsample = GetSample();
	position = enf_min(position, xsample->GetTimeLength());
	SoundState state = GetState();
	if(state == SS_NOTSTARTED || state == SS_STOPPED)
	{
		m_fTimePosition = position;
		return;
	}

	if(m_pSourceVoice)
	{
		m_pSourceVoice->FlushSourceBuffers();

		XAUDIO2_BUFFER buff;
		uint beginLoop = enf_min(m_iBeginLoop, xsample->GetNumSamples());
		uint endLoop = enf_min(m_iEndLoop, xsample->GetNumSamples());

		xsample->InitBuffer(buff, m_iFlags, m_fTimePosition, beginLoop, endLoop, ref);

		m_pSourceVoice->SubmitSourceBuffer(&buff, NULL);
	}
}


//-----------------------------------------------------------------------
XAudio2System::Sound* XAudio2System::AllocSound(uint scene, SoundType st, uint flags, XAudio2Shader* shader, SoundHandle& handle)
{
	XSoundRef* ref = m_FreeSounds.GetFirstNode();

	if(ref == NULL)
	{
		handle = SOUND_NOT_PLAYING;
		return NULL;
	}

	uint nref = ref - m_Sounds;
	XAudio2System::Sound* sound = NULL;

	switch(st)
	{
	case ST_SPATIAL:
		sound = ENF_NEW SpatialSound(nref, flags, shader);

		break;
	case ST_AMBIENT:
		sound = ENF_NEW AmbientSound(nref, flags, shader);
		break;
	}
	m_FreeSounds.Remove(ref);
	ref->m_pSound = sound;
	ref->m_iSceneIndex = scene;
	sound->InsertToList(ref, &m_pSoundScenes[scene]->m_StoppedSounds);
	handle = ToHandle(ref);
	return sound;
}

//-----------------------------------------------------------------------
void XAudio2System::FreeSound(XSoundRef* ref)
{
	enf_assert(ref != NULL);

	enf_assert(!m_FreeSounds.Contains(ref));
	CMutexLock lock(m_EndedSetMutex);

	Sound* snd = static_cast<Sound*>(ref->m_pSound);

	//avoid recursion, usually due to callback/delete source/end sounds
	if(snd && snd->IsFlagsSet(SFX_ENDING))
		return;

	{
		SoundScene* ss = GetScene(ref->m_iSceneIndex);
		//just in case..
		ss->m_EndedSounds.Remove(ref);
	}

	SoundHandle handle = ToHandle(ref);
	//invalidate old handle, so it cannot be reused from callback
	ref->InvalidateHandle();

	//should never happen, but it does. Why?
	//enf_assert(snd != NULL);
	if(snd)
	{
		snd->SetFlags(SFX_ENDING);

		if(snd->GetStreaming() != NULL)
		{
			RemoveStreamingSound(ref);
		}

		XSoundSource* source = snd->GetSource();
		if(source)
		{
			RemoveSourceRef(source, handle);
			source->OnSoundEnd(handle);
		}

		DestroySourceVoice(snd);

		snd->RemoveFromList(ref);
		SAFE_DELETE(ref->m_pSound);
	}
	m_FreeSounds.Insert(ref);
}

//-----------------------------------------------------------------------
XAudio2System::XAudio2System():
	ISoundSystem(),
	m_pCallback(NULL),
	m_pXAudio2(NULL),
	m_pMasteringVoice(NULL),
	m_iOperSet(1),
	m_fMasterSfxVolume(1.0f),
	m_fMasterMusicVolume(1.0f),
	m_StopStreaming(false, false),
	m_StreamThread(this),
	m_iNumActiveVoices(0),
	m_iNumCreatedVoices(0),
	m_iUpdateFrame(0)
{

	m_iMaxAllocated = XAUDIO2_DEFAULT_MAX_ALLOC;

	TimeStamp ctime = Timer::GetTicks();

	//set default parameters
	ListenerParameters listener;
	for(uint s = 0; s < MAX_SOUND_SCENES; s++)
	{
		SoundScene* ss = ENF_NEW SoundScene();
		ss->m_iLastTime = ctime;
		m_pSoundScenes[s] = ss;
		SetListener(s, listener);
	}

	InitSounds();
	m_pCallback = ENF_NEW XVoiceCallback(this);

	AddShaderClass(ENF_NEW SoundShaderClass());

	MemStats::m_iSoundSystem += sizeof(*this);
}

//-----------------------------------------------------------------------
void XAudio2System::EndSounds(uint sceneIndex)
{
	enf_assert(sceneIndex < MAX_SOUND_SCENES);
	SoundScene* ss = static_cast<SoundScene*>(m_pSoundScenes[sceneIndex]);

	ListIterator<XSoundRef> stopped(ss->m_StoppedSounds);

	while(stopped.HasMoreElements())
	{
		XSoundRef* snd = stopped.GetNext();
		FreeSound(snd);
	}

	ListIterator<XSoundRef> playingAmbient(ss->m_PlayingAmbientSounds);

	while(playingAmbient.HasMoreElements())
	{
		XSoundRef* snd = playingAmbient.GetNext();
		FreeSound(snd);
	}

	ListIterator<XSoundRef> playing(ss->m_PlayingSounds);

	while(playing.HasMoreElements())
	{
		XSoundRef* snd = playing.GetNext();
		FreeSound(snd);
	}
}
//-----------------------------------------------------------------------
XAudio2System::~XAudio2System()
{
	m_StreamThread.Kill();

	for(uint s = 0; s < MAX_SOUND_SCENES; s++)
	{
		SoundScene* scene = GetScene(s);
		if(scene)
		{
			EndSounds(s);

			SAFE_RELEASE(scene->m_pReverbEffect);
			if(scene->m_pReverbSubmixVoice)
			{
				scene->m_pReverbSubmixVoice->DestroyVoice();
				scene->m_pReverbSubmixVoice = NULL;
			}
			delete scene;
			m_pSoundScenes[s] = NULL;
		}
	}

	Cleanup();

	if(m_pMasteringVoice)
	{
		m_pMasteringVoice->DestroyVoice();
		m_pMasteringVoice = NULL;
	}

	ForEach<DeleteItem>(m_pFormats);
	for(uint n = 0; n < m_SourceVoices.GetCardinality(); n++)
	{
		for(uint i = 0; i < m_SourceVoices[n]->m_Voices.GetCardinality(); i++)
			m_SourceVoices[n]->m_Voices[i].Key->DestroyVoice();
		delete m_SourceVoices[n];
	}
	SAFE_RELEASE(m_pXAudio2);
	SAFE_DELETE(m_pCallback);
	
#ifdef WIN32
	::CoUninitialize();
#endif
	MemStats::m_iSoundSystem += sizeof(*this);
}

//-----------------------------------------------------------------------
bool XAudio2System::IsHearable(float soundRange, uint sceneIndex, const Vector3& pos, bool wasHearable) const
{
	const SoundScene* scene = GetScene(sceneIndex);
	float sqdist = pos.Distance2(Vec3(scene->m_Listener.Position));
	if(wasHearable)
		soundRange = soundRange * 1.1f;

	if(sqr(soundRange) < sqdist)
		return false;

	return true;
}

//-----------------------------------------------------------------------
void XAudio2System::DestroySourceVoice(Sound* sound)
{
	if(sound->m_pSourceVoice)
	{
		CMutexLock lock(m_VoiceMutex);

		if(sound->m_pSample)
			sound->m_pSample->EndPlay();

		sound->m_pSourceVoice->Stop(0, XAUDIO2_COMMIT_NOW);
		sound->m_pSourceVoice->FlushSourceBuffers();
		SourceVoiceContainer::TSourceVoiceEntry entry;
		entry.Key = sound->m_pSourceVoice;
		entry.Data = m_iUpdateFrame;
		sound->m_pVoiceContainer->m_Voices.Insert(entry);
		sound->m_pSourceVoice = NULL;
		sound->m_pVoiceContainer = NULL;
		IL_DECREMENT(m_iNumActiveVoices);
	}
}

//-----------------------------------------------------------------------
uint Age(uint timeStamp, uint timeNow)
{
	if(timeStamp > timeNow)
		return 0xffffffff;

	return timeNow - timeStamp;
}

//-----------------------------------------------------------------------
IXAudio2SourceVoice* XAudio2System::CreateSourceVoice(Sound* snd, SoundScene* scene, const CommonFormat* fmt, uint flags, bool useFilter, bool useReverb, uint operSet)
{
	CMutexLock lock(m_VoiceMutex);

	XAUDIO2_SEND_DESCRIPTOR sendDescriptors[2];
	uint numSends = 1;

	sendDescriptors[0].Flags = useFilter ? XAUDIO2_SEND_USEFILTER : 0; // LPF direct-path
	sendDescriptors[0].pOutputVoice = m_pMasteringVoice;

	if(useReverb)
	{
		sendDescriptors[1].Flags = useFilter ? XAUDIO2_SEND_USEFILTER : 0; // LPF reverb-path -- omit for better performance at the cost of less realistic occlusion
		sendDescriptors[1].pOutputVoice = scene->m_pReverbSubmixVoice;
		numSends++;
	}

	XAUDIO2_VOICE_SENDS sendList = { numSends, sendDescriptors };

	IXAudio2SourceVoice* sourceVoice = NULL;

	enf_assert2(m_pFormats.GetIndexOf(const_cast<CommonFormat*>(fmt)) != INDEX_NOT_FOUND, "CommonFormat* must be from m_pFormats container");

	SourceVoiceContainer cont;
	cont.m_pFormat = fmt;
	cont.m_iFlags = flags;
	SourceVoiceContainer* pcont = NULL;
	if(m_SourceVoicesHashMap.Find(&cont, pcont) && !pcont->m_Voices.IsEmpty())
	{
		sourceVoice = pcont->m_Voices.Pop().Key;
		sourceVoice->SetOutputVoices(&sendList);
		if((pcont->m_iFlags & XAUDIO2_VOICE_NOPITCH) == 0)
			sourceVoice->SetFrequencyRatio(1.0f, operSet);
		sourceVoice->SetVolume(1.0f, operSet);
	}
	else
	{
		if(pcont == NULL)
		{
			pcont = ENF_NEW SourceVoiceContainer;
			pcont->m_pFormat = fmt;
			pcont->m_iFlags = flags;
			m_SourceVoicesHashMap.Insert(pcont, pcont);
			m_SourceVoices.Insert(pcont);
		}
		snd->m_pVoiceContainer = pcont;

		if(m_iNumCreatedVoices > 256)
		{
			//need to free something!
			uint bestn = -1;
			uint bestage = 0;

			for(uint n = 0; n < m_SourceVoices.GetCardinality(); n++)
			{
				if(!m_SourceVoices[n]->m_Voices.IsEmpty())
				{
					uint age = Age(m_iUpdateFrame, m_SourceVoices[n]->m_Voices[0].Data);
					if(age > bestage)
					{
						bestage = age;
						bestn = n;
					}
				}
			}

			if(bestn != -1)
			{
				m_SourceVoices[bestn]->m_Voices[0].Key->DestroyVoice();
				m_SourceVoices[bestn]->m_Voices.RemoveElementOrdered(0);
				m_iNumCreatedVoices--;
			}
		}

		if(m_pXAudio2->CreateSourceVoice(&sourceVoice, &fmt->m_Format, flags, XAUDIO2_DEFAULT_FREQ_RATIO, m_pCallback, &sendList, NULL) != S_OK)
			sourceVoice = NULL;
		else
		{
			m_iNumCreatedVoices++;
		}
	}

	snd->m_pVoiceContainer = pcont;
	snd->m_pSourceVoice = sourceVoice;
	if(sourceVoice)
	{
		IL_INCREMENT(m_iNumActiveVoices);
	}
	return sourceVoice;
}

//-----------------------------------------------------------------------
CommonFormat::CommonFormat(const CommonFormat& fmt)
{
	memcpy(this, &fmt, sizeof(*this));
	MemStats::m_iSoundSystem += sizeof(*this);
}
//-----------------------------------------------------------------------
CommonFormat::CommonFormat()
{
	memset(this, 0, sizeof(*this));
	MemStats::m_iSoundSystem += sizeof(*this);
}

//-----------------------------------------------------------------------
CommonFormat::~CommonFormat()
{
	MemStats::m_iSoundSystem -= sizeof(*this);
}

//-----------------------------------------------------------------------
const CommonFormat* XAudio2System::GetFormat(const CommonFormat& fmt)
{
	CMutexLock lock(m_VoiceMutex);

/*
	//solve tiny deviations due to XMA loop-compression!
	CommonFormat tempFmt;
	const CommonFormat* theFormat = &fmt;
	DWORD sps[] = {11025, 22050, 44100};
	for(uint n = 0; n < 3; n++)
	{
		int diff = fmt.m_Format.nSamplesPerSec - sps[n];
		if(diff != 0 && abs(diff) < 200)
		{
			tempFmt = fmt;
			tempFmt.m_Format.nSamplesPerSec = sps[n];
			//cannot recompute this shit! It takes in account the compression
			//tempFmt.m_Format.nAvgBytesPerSec = fmt.m_Format.nBlockAlign * sps[n];
			theFormat = &tempFmt;
			break;
		}
	}

	uint index;
	if(!m_pFormatsHashMap.Find(theFormat, index))
	{
		CommonFormat* newFmt = ENF_NEW CommonFormat(*theFormat);
		index = m_pFormats.Insert(newFmt);
		m_pFormatsHashMap.Insert(newFmt, index);

		LogF(LL_NORMAL, "fmt %d ch %d sps %d, size %d", (int)newFmt->m_Format.wFormatTag,
			(int)newFmt->m_Format.nChannels,
			(int)newFmt->m_Format.nSamplesPerSec,
			(int)newFmt->m_Format.cbSize);

	}
	*/
	uint index;
	if(!m_pFormatsHashMap.Find(&fmt, index))
	{
		CommonFormat* newFmt = ENF_NEW CommonFormat(fmt);
		index = m_pFormats.Insert(newFmt);
		m_pFormatsHashMap.Insert(newFmt, index);
	}

	return m_pFormats[index];
}

//-----------------------------------------------------------------------
bool XAudio2System::SpatialSound::UpdateTime(float timeSlice)
{
	//just update time
	float newTime = m_fTimePosition + timeSlice;
	float sampleLength = m_pSample->GetTimeLength();
	
	if(sampleLength != 0.0f && newTime >= sampleLength)
	{
		if(m_iFlags & SFX_ONCE)
		{
			//we are at the end. Remove it
			return false;
		}
		do
		{
			newTime -= sampleLength;
		} while(newTime >= sampleLength);
	}
	m_fTimePosition = newTime;
	return true;
}

//-----------------------------------------------------------------------
bool XAudio2System::SpatialSound::IsHearable(XAudio2System* sys, XSoundRef* ref, bool wasHearable)
{
	if((m_iFlags & SFX_NOTEST) != 0)
		return true;

	Vector3 pos = m_pSource->GetSoundCenter();
	if(!sys->IsHearable(GetRange(), ref->m_iSceneIndex, pos, wasHearable))
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
void XAudio2System::AddStreamingSound(XSoundRef* ref)
{
	CMutexLock lock(m_StreamingSetMutex);
	m_StreamingSounds.Insert(ref);
}

//-----------------------------------------------------------------------
void XAudio2System::RemoveStreamingSound(XSoundRef* ref)
{
	CMutexLock lock(m_StreamingSetMutex);
	m_StreamingSounds.Remove(ref);
}

//-----------------------------------------------------------------------
XAudio2System::SoundState XAudio2System::SpatialSound::Update(XAudio2System* sys, SoundScene* scene, XSoundRef* ref, double time, uint64 timeStamp, uint operSet)
{
	bool doUpdate3D = false;
	bool isHearable = true;

	if(GetState() == SS_ENDED)
		return SS_ENDED;

	if(m_iFlags & SFX_PAUSED)
		return (XAudio2System::SoundState)m_State;

	XAudio2Sample* sample = GetSample();

	if(!m_pStreaming && sample->CannotLoad())
	{
		sys->EnqueueEndedSound(ref);
		IL_EXCHANGE(m_State, SS_ENDED);
		return SS_ENDED;
	}

	SoundState state = GetState();
	if(state == SS_NOTSTARTED || state == SS_STOPPED || state == SS_PLAYING)
	{
		isHearable = IsHearable(sys, ref, state != SS_STOPPED);
	}

	if(state == SS_NOTSTARTED || state == SS_STOPPED)
	{
		//check, if we can start now!
		if(isHearable)
		{
			if(!m_pStreaming)
			{
				if(sample->IsLoading())
				{
					return (XAudio2System::SoundState)m_State;
				}
				else if(!sample->IsLoaded())
				{
					sys->Load(sample, m_iShaderFlags);
					return (XAudio2System::SoundState)m_State;
				}
				float sampleLength = sample->GetTimeLength();

				if(m_fTimePosition >= sampleLength)
				{
					if(m_iFlags & SFX_ONCE)
					{
						//we are at the end. Remove it
						sys->EnqueueEndedSound(ref);
						IL_EXCHANGE(m_State, SS_ENDED);
						return SS_ENDED;
					}
				}
			}

			uint vflags = 0;

			const WAVEFORMATEX& fmt = sample->GetFormat();
			if(m_iCurrentRate != 0 && m_iCurrentRate != fmt.nSamplesPerSec)
			{
				m_iFlags |= SFX_CHANGED_FREQUENCY;
				m_iShaderFlags |= SSFX_FREQCONTROL;
			}
			else
				m_iCurrentRate = fmt.nSamplesPerSec;

			if(IsShaderFlagsSet(SSFX_MUSIC))
				vflags |= XAUDIO2_VOICE_MUSIC;
			if(!IsShaderFlagsSet(SSFX_FREQCONTROL))
				vflags |= XAUDIO2_VOICE_NOPITCH;

			if(m_pSourceVoice == NULL)
			{
				sample->Touch(0);
				if(fmt.nChannels > 1)
				{
					LogF(LL_ERROR, "Cannot play multichannel sample '%s' as 3D", sample->GetName().cstr());
					sys->EnqueueEndedSound(ref);
					IL_EXCHANGE(m_State, SS_ENDED);
					return SS_ENDED;
				}

				m_iFlags |= SFX_CHANGED_VOLUME;

				m_pSourceVoice = sys->CreateSourceVoice(this, scene, sample->m_pFormat, vflags, true, true, operSet);
				if(m_pSourceVoice == NULL)
					return (XAudio2System::SoundState)m_State;

				if(sample)
				{
					sample->BeginPlay();
				}

				if(m_pStreaming == NULL)
				{
					XAUDIO2_BUFFER buff;
					uint beginLoop = enf_min(m_iBeginLoop, sample ? sample->GetNumSamples() : 0);
					uint endLoop = enf_min(m_iEndLoop, sample ? sample->GetNumSamples() : 0);
					sample->InitBuffer(buff, m_iFlags, m_fTimePosition, beginLoop, endLoop, ref);

					m_pSourceVoice->SubmitSourceBuffer(&buff, NULL);
				}
				else
				{
					//streaming sounds can be started *after* they have submited some buffers!
					sys->AddStreamingSound(ref);
				}
			}

			if(m_pStreaming == NULL)
				sys->Start(m_pSourceVoice, 0, operSet);

			sys->DoCommit();
			doUpdate3D = true;
			IL_EXCHANGE(m_State, SS_PLAYING);
			RemoveFromList(ref);
			InsertToList(ref, &scene->m_PlayingSounds);
		}
		else
		{
			if(!UpdateTime(time))
			{
				sys->EnqueueEndedSound(ref);
				IL_EXCHANGE(m_State, SS_ENDED);
				return SS_ENDED;
			}
		}
	}

	if(GetState() == SS_PLAYING)
	{
		//not hearable now, stop this
		if(!isHearable)
		{
			XAUDIO2_VOICE_STATE state;
			m_pSourceVoice->GetState(&state);

			m_fTimePosition = (float)state.SamplesPlayed / GetSample()->GetFormat().nSamplesPerSec;
			sys->DestroySourceVoice(this);

			IL_EXCHANGE(m_State, SS_STOPPED);

			RemoveFromList(ref);
			InsertToList(ref, &scene->m_StoppedSounds);
		}
		else if(m_pSource)
		{
			XAudio2Sample* sample = static_cast<XAudio2Sample*>(GetSample());
			if(sample)
				sample->Touch(0);
			IXAudio2SourceVoice* voice = m_pSourceVoice;
			if(UpdateVolume(scene, operSet))
				sys->DoCommit();

			if(!doUpdate3D)
			{
				if(Timer::Sub(timeStamp, m_iLast3DUpdate) > Timer::Time2Tick(33, Timer::RESOLUTION_MILLISECONDS))
					doUpdate3D = true;
			}

			if(doUpdate3D || scene->m_iFlags & SSF_UPDATE_3D)
			{
				Vector3 position = m_pSource->GetSoundCenter();

				X3DAUDIO_DSP_SETTINGS dspsettings;
				ZeroMem(&dspsettings, sizeof(X3DAUDIO_DSP_SETTINGS));

				FLOAT32 matrix[16];
				FLOAT32 delays[8];
				dspsettings.SrcChannelCount = 1;
				dspsettings.DstChannelCount = sys->m_iOutputChannels;
				dspsettings.pMatrixCoefficients = matrix;
				dspsettings.pDelayTimes = delays;

				DWORD dwCalcFlags = X3DAUDIO_CALCULATE_MATRIX |
					X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_LPF_REVERB |
					X3DAUDIO_CALCULATE_REVERB;

				if(sys->m_iOutputChannelMask & SPEAKER_LOW_FREQUENCY)
					dwCalcFlags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;

				X3DAUDIO_EMITTER emitter;
				if(m_iFlags & SFX_DIRECTIONAL)
				{
					//stupid M$
					emitter.pCone = (X3DAUDIO_CONE *)m_pCone;
					Matrix33 orientation = m_pSource->GetSoundOrientation();
					emitter.OrientFront = Vec3(-orientation.Direction());
					emitter.OrientTop = Vec3(orientation.DirectionUp());
//					m_LastForward = orientation.GetRow(0);
//					m_LastUp = orientation.GetRow(2);
				}
				else
				{
					emitter.pCone = NULL;
					emitter.OrientFront = Vec3(Vector3(1, 0, 0));
					emitter.OrientTop = Vec3(Vector3(0, 0, 1));
				}
//				m_LastPos = position;
				emitter.Position = Vec3(position);
				if(m_iFlags & SFX_DOPPLER)
				{
					Vector3 velocity = m_pSource->GetSoundVelocity() * (1.0f / 32.0f);
					emitter.Velocity = Vec3(velocity);
					//enf::LogF(enf::LL_NORMAL, "%f (%f %f %f)", velocity.Length(), velocity.x, velocity.y, velocity.z);
					dwCalcFlags |= X3DAUDIO_CALCULATE_DOPPLER;
				}
				else
					emitter.Velocity.x = 0; emitter.Velocity.y = 0; emitter.Velocity.z = 0;
				emitter.InnerRadius = 2.0f;
				emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.0f;
				emitter.ChannelCount = 1;
				emitter.ChannelRadius = 1.0f;
				emitter.pChannelAzimuths = NULL; //
				if(GetMinRange() > 0)
				{
					//need to prepare custom curve
					static X3DAUDIO_DISTANCE_CURVE_POINT customCurvePoints[8] = {
						0.0f, 1.0f
					};
					static X3DAUDIO_DISTANCE_CURVE customVolumeCurve = {customCurvePoints, 0};

					float minFrac = enf_limit(GetMinRange() / GetRange(), 0.0f, 1.0f);
					float restScale = 1.0f - minFrac;

					uint n = 0;
					for(; n < Emitter_Volume_Curve.PointCount - 1; n++)
					{
						customCurvePoints[n + 1].Distance = enf_limit(restScale * Emitter_Volume_Curve.pPoints[n].Distance + minFrac, 0.0f, 1.0f);
						customCurvePoints[n + 1].DSPSetting = Emitter_Volume_Curve.pPoints[n].DSPSetting;
					}
					customCurvePoints[n + 1].Distance = 1.0f;
					customCurvePoints[n + 1].DSPSetting = 0.0f;
					customVolumeCurve.PointCount = Emitter_Volume_Curve.PointCount + 1;

					emitter.pVolumeCurve =  (X3DAUDIO_DISTANCE_CURVE*)&customVolumeCurve;
				}
				else
				{
					emitter.pVolumeCurve =  (X3DAUDIO_DISTANCE_CURVE*)&Emitter_Volume_Curve;
				}
				emitter.pLFECurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_LFE_Curve;
				emitter.pLPFDirectCurve = NULL; // use default curve
				emitter.pLPFReverbCurve = NULL; // use default curve
				emitter.pReverbCurve = NULL; //(X3DAUDIO_DISTANCE_CURVE*)&Emitter_Reverb_Curve;
				emitter.CurveDistanceScaler = GetRange();
				emitter.DopplerScaler = GetDoppler();

				X3DAudioCalculate(sys->X3DInstance, &scene->m_Listener, &emitter, dwCalcFlags, &dspsettings);

				// Apply X3DAudio generated DSP settings to XAudio2
				uint newRate = enf_min((uint)m_iCurrentRate, uint(GetSample()->GetFormat().nSamplesPerSec * 2));

				if(dwCalcFlags & X3DAUDIO_CALCULATE_DOPPLER)
				{
					float baseRate = (float)newRate / GetSample()->GetFormat().nSamplesPerSec;
					voice->SetFrequencyRatio( baseRate * dspsettings.DopplerFactor, operSet);
					m_iFlags &= ~SFX_CHANGED_FREQUENCY;
				}
				else
				{
					if(m_iFlags & SFX_CHANGED_FREQUENCY)
					{
						float baseRate = (float)newRate / GetSample()->GetFormat().nSamplesPerSec;
						voice->SetFrequencyRatio( baseRate, operSet);
						m_iFlags &= ~SFX_CHANGED_FREQUENCY;
					}
				}
				voice->SetOutputMatrix(sys->m_pMasteringVoice, 1, sys->m_iOutputChannels, matrix, operSet);
				voice->SetOutputMatrix(scene->m_pReverbSubmixVoice, 1, 1, &dspsettings.ReverbLevel, operSet);

				//voice->SetFilterParameters(
				XAUDIO2_FILTER_PARAMETERS FilterParametersDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI/6.0f * dspsettings.LPFDirectCoefficient), 1.0f }; // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
				voice->SetOutputFilterParameters(sys->m_pMasteringVoice, &FilterParametersDirect, operSet);
				XAUDIO2_FILTER_PARAMETERS FilterParametersReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI/6.0f * dspsettings.LPFReverbCoefficient), 1.0f }; // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
				voice->SetOutputFilterParameters(scene->m_pReverbSubmixVoice, &FilterParametersReverb, operSet);
				sys->DoCommit();
				m_iLast3DUpdate = timeStamp;
			}
		}
	}
	return (XAudio2System::SoundState)m_State;
}

//-----------------------------------------------------------------------
XAudio2System::SpatialSound::SpatialSound(uint nsound, uint flags, XAudio2Shader* shader):
	Sound(nsound, flags, shader->GetFlags()),
	m_fRange(shader->GetRange()),
	m_fMinRange(shader->GetMinRange()),
	m_fDoppler(shader->GetDoppler()),
	m_iLast3DUpdate(0)
{
	m_pCone = (flags & SFX_DIRECTIONAL) ? &X3DAudioDefault_DirectionalCone : NULL;
	MemStats::m_iSoundSystem += sizeof(*this);
}

//-----------------------------------------------------------------------
XAudio2System::SpatialSound::~SpatialSound()
{
	MemStats::m_iSoundSystem -= sizeof(*this);
}

//-----------------------------------------------------------------------
XAudio2System::AmbientSound::AmbientSound(uint nsound, uint flags, XAudio2Shader* shader): Sound(nsound, flags, shader->GetFlags())
{
	MemStats::m_iSoundSystem += sizeof(*this);
}

//-----------------------------------------------------------------------
XAudio2System::AmbientSound::~AmbientSound()
{
	MemStats::m_iSoundSystem -= sizeof(*this);
}

//-----------------------------------------------------------------------
bool XAudio2System::Sound::UpdateVolume(XAudio2System::SoundScene* scene, uint set)
{
	bool isMusic = (m_iFlags & SFX_MUSIC) != 0;
	uint changeMask = isMusic ? SSF_UPDATE_MUSIC_VOLUME : SSF_UPDATE_SFX_VOLUME;

	if(m_iFlags & SFX_CHANGED_VOLUME || scene->m_iFlags & changeMask)
	{
		float globalVolume = isMusic ? scene->m_fLocalMusicVolume : scene->m_fLocalSfxVolume;
		float volume = m_fVolume * globalVolume;
#ifdef ENF_XENON
		volume *= 0.8f;
		volume = enf_min(volume, 0.8f);
#else
		volume = enf_min(volume, 1.0f);
#endif
		m_pSourceVoice->SetVolume(volume, set);
		m_iFlags &= ~SFX_CHANGED_VOLUME;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------
bool XAudio2System::Start(IXAudio2SourceVoice* sourceVoice, uint flags, uint operSet)
{
	enf_assert(sourceVoice);
	HRESULT hr = sourceVoice->Start(flags, operSet);
	enf_assert(hr == S_OK);
	return (hr == S_OK);
}

//-----------------------------------------------------------------------
void XAudio2System::Stop(IXAudio2SourceVoice* sourceVoice, uint flags, uint operSet)
{
	enf_assert(sourceVoice);
	HRESULT hr = sourceVoice->Stop(flags, operSet);
	enf_assert(hr == S_OK);
}

//-----------------------------------------------------------------------
XAudio2System::SoundState XAudio2System::AmbientSound::Update(XAudio2System* sys, SoundScene* scene, XSoundRef* ref, double time, uint64 timeStamp, uint operSet)
{
	if(m_State == SS_ENDED)
		return SS_ENDED;

	if(m_iFlags & SFX_PAUSED)
		return (XAudio2System::SoundState)m_State;

	XAudio2Sample* sample = GetSample();

	if(!m_pStreaming)
	{
		if(sample->CannotLoad())
		{
			sys->EnqueueEndedSound(ref);
			IL_EXCHANGE(m_State, SS_ENDED);
			return SS_ENDED;
		}
		else if(sample->IsLoading())
			return (XAudio2System::SoundState)m_State;
		else if(!sample->IsLoaded())
			return (XAudio2System::SoundState)m_State;
	}

	if(m_State == SS_NOTSTARTED)
	{
		uint vflags = 0;
		const CommonFormat* pformat = NULL;
		if(sample == NULL)
		{
			//we are running from custom data
			CommonFormat fmt;
			m_pStreaming->SetupFormat(fmt);
			pformat = sys->GetFormat(fmt);
		}
		else
		{
			pformat = sample->m_pFormat;
		}

		const WAVEFORMATEX& fmt = pformat->m_Format;
		if(m_iCurrentRate != 0 && m_iCurrentRate != fmt.nSamplesPerSec)
		{
			m_iFlags |= SFX_CHANGED_FREQUENCY;
			m_iShaderFlags |= SSFX_FREQCONTROL;
		}
		else
			m_iCurrentRate = fmt.nSamplesPerSec;

		if(IsShaderFlagsSet(SSFX_MUSIC))
			vflags |= XAUDIO2_VOICE_MUSIC;
		if(!IsShaderFlagsSet(SSFX_FREQCONTROL))
			vflags |= XAUDIO2_VOICE_NOPITCH;

		if(sys->CreateSourceVoice(this, scene, pformat, vflags, false, false, operSet) == NULL)
			return (XAudio2System::SoundState)m_State;


		float matrix[16];
		uint outputChannelMask = static_cast<XAudio2System*>(sys)->m_iOutputChannelMask;
		uint outputChannels = static_cast<XAudio2System*>(sys)->m_iOutputChannels;
		//if(pformat->m_Format.nChannels == 2)
		//{
		//	m_pSourceVoice->GetOutputMatrix(NULL, 2, outputChannels, matrix);
		//}

		for(uint s = 0; s < (pformat->m_Format.nChannels * outputChannels); s++)
			matrix[s] = 1.0f;

		if(pformat->m_Format.nChannels == 2)
		{
			uint channelIndex = 0;

			if(outputChannelMask & (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT))
			{
				//fronts
				matrix[channelIndex * 2 + 0] = 1.0f;
				matrix[channelIndex * 2 + 1] = 0.0f;
				matrix[channelIndex * 2 + 2] = 0.0f;
				matrix[channelIndex * 2 + 3] = 1.0f;
				channelIndex += 2;
			}
			if(outputChannelMask & SPEAKER_FRONT_CENTER)
			{
				//center
				matrix[channelIndex * 2 + 0] = 0.1f;
				matrix[channelIndex * 2 + 1] = 0.1f;
				channelIndex += 1;
			}
			if(outputChannelMask & SPEAKER_LOW_FREQUENCY)
			{
				//LFE
				matrix[channelIndex * 2 + 0] = 1.0f;
				matrix[channelIndex * 2 + 1] = 1.0f;
				channelIndex += 1;
			}
			if(outputChannelMask & (SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT))
			{
				//rears
				if(m_iShaderFlags & SSFX_MUSIC)
				{
					matrix[channelIndex * 2 + 0] = 0.0f;
					matrix[channelIndex * 2 + 1] = 0.0f;
					matrix[channelIndex * 2 + 2] = 0.0f;
					matrix[channelIndex * 2 + 3] = 0.0f;
				}
				else
				{
					matrix[channelIndex * 2 + 0] = 0.8f;
					matrix[channelIndex * 2 + 1] = 0.0f;
					matrix[channelIndex * 2 + 2] = 0.0f;
					matrix[channelIndex * 2 + 3] = 0.8f;
				}
				channelIndex += 2;
			}
		}
		else if(pformat->m_Format.nChannels == 1)
		{
			uint channelIndex = 0;
			if(outputChannelMask & (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT))
			{
				//fronts
				matrix[channelIndex + 0] = 1.0f;
				matrix[channelIndex + 1] = 1.0f;
				channelIndex += 2;
			}
			if(outputChannelMask & SPEAKER_FRONT_CENTER)
			{
				//center
				matrix[channelIndex + 0] = 0.1f;
				channelIndex += 1;
			}
			if(outputChannelMask & SPEAKER_LOW_FREQUENCY)
			{
				//LFE
				matrix[channelIndex + 0] = 1.0f;
				channelIndex += 1;
			}
			if(outputChannelMask & (SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT))
			{
				//rears
				matrix[channelIndex + 0] = 0.8f;
				matrix[channelIndex + 1] = 0.8f;
				channelIndex += 2;
			}
		}

		if(sample)
			sample->BeginPlay();

		if(m_pStreaming == NULL)
		{
			XAUDIO2_BUFFER buff;
			uint beginLoop = enf_min(m_iBeginLoop, sample ? sample->GetNumSamples() : 0);
			uint endLoop = enf_min(m_iEndLoop, sample ? sample->GetNumSamples() : 0);
			sample->InitBuffer(buff, m_iFlags, m_fTimePosition, beginLoop, endLoop, ref);

			m_pSourceVoice->SubmitSourceBuffer(&buff, NULL);
			sys->Start(m_pSourceVoice, 0, operSet);
		}
		else
		{	
			sys->AddStreamingSound(ref);
		}

		sys->DoCommit();
		IL_EXCHANGE(m_State, SS_PLAYING);
		RemoveFromList(ref);
		InsertToList(ref, &scene->m_PlayingAmbientSounds);
	}

	if(GetState() == SS_PLAYING)
	{
		if(UpdateVolume(scene, operSet))
			sys->DoCommit();

		XAudio2Sample* sample = GetSample();
		if(sample)
			sample->Touch(0);
		if(m_iFlags & SFX_CHANGED_FREQUENCY)
		{
			float baseRate = (float)m_iCurrentRate / sample->GetFormat().nSamplesPerSec;
			m_pSourceVoice->SetFrequencyRatio(baseRate, operSet);
			sys->DoCommit();
		}
	}

	return (XAudio2System::SoundState)m_State;
}

//-----------------------------------------------------------------------
void DumpSound(XSoundRef* ref)
{
	if(ref->m_pSound->GetStreaming())
	{
		if(ref->m_pSound->GetSource())
		{
			LogF(LL_NORMAL, " streaming sample '%s' source name '%s'", 
				ref->m_pSound->GetSample()->GetName().cstr(),
				ref->m_pSound->GetSource()->GetSourceName().cstr()
				);
		}
		else
		{
			LogF(LL_NORMAL, " streaming sample '%s'. No source", ref->m_pSound->GetSample()->GetName().cstr());
		}
	}
	else
	{
		if(ref->m_pSound->GetSource())
		{
			LogF(LL_NORMAL, " static sample '%s' source name '%s'", 
				ref->m_pSound->GetSample()->GetName().cstr(),
				ref->m_pSound->GetSource()->GetSourceName().cstr()
				);
		}
		else
		{
			LogF(LL_NORMAL, " static sample '%s'. No source", ref->m_pSound->GetSample()->GetName().cstr());
		}
	}
}

//-----------------------------------------------------------------------
void XAudio2System::DumpActiveSounds()
{
/*	LogF(LL_NORMAL, "Dump active streaming sounds");
	for(uint n = 0; n < m_StreamingSounds.GetCardinality(); n++)
	{
		DumpSound(m_StreamingSounds[n]);
	}
*/
	for(uint s = 0; s < MAX_SOUND_SCENES; s++)
	{
		XSoundScene* scene = m_pSoundScenes[s];

		if(scene == NULL || (scene->m_PlayingAmbientSounds.IsEmpty() && scene->m_PlayingSounds.IsEmpty()))
			continue;

		LogF(LL_NORMAL, "Sound scene %d", s);

		ListIterator<XSoundRef> playingAmb(scene->m_PlayingAmbientSounds);

		if(!scene->m_PlayingAmbientSounds.IsEmpty())
		{
			LogF(LL_NORMAL, "Dump active ambient sounds");
			for(XSoundRef* snd = scene->m_PlayingAmbientSounds.GetFirstNode(); snd; snd = snd->ENF_LIST_NEXT)
			{
				DumpSound(snd);
			}
		}

		if(!scene->m_PlayingSounds.IsEmpty())
		{
			LogF(LL_NORMAL, "Dump active spatial sounds");
			for(XSoundRef* snd = scene->m_PlayingSounds.GetFirstNode(); snd; snd = snd->ENF_LIST_NEXT)
			{
				DumpSound(snd);
			}
		}
	}
}

//-----------------------------------------------------------------------
void XAudio2System::UpdateSounds(uint sceneIndex)
{
	if(m_pXAudio2 == NULL || m_pMasteringVoice == NULL)
		return;
	CMutexLock lock(m_Mutex);

	if(ENF_DIAG_BOOL(DM_DUMP_ACTIVE_SOUNDS))
	{
		ENF_DIAG_SET(DM_DUMP_ACTIVE_SOUNDS, false);
		DumpActiveSounds();
	}

	ENF_PROFILE(PROFILE_SOUNDMANAGER_UPDATE);

	{
		CMutexLock lock(m_VoiceMutex);
		m_iUpdateFrame++;
	}

	SoundScene* ss = GetScene(sceneIndex);

	ss->m_iFlags |= SSF_UPDATE_3D;

	TimeStamp timeStamp = Timer::GetTicks();
	double timeSlice = Timer::Tick2Time(Timer::Sub(timeStamp, ss->m_iLastTime), 1);
	ss->m_iLastTime = timeStamp;

	//process Ended sounds
	while(true)
	{
		XSoundRef* soundRef = DequeueEndedSound(sceneIndex);
		if(soundRef == NULL)
			break;
		FreeSound(soundRef);
	}

	//update playing sounds
	if(!ss->m_PlayingSounds.IsEmpty())
	{
		uint operSet = GetOperationSet();
		//update environment, if necessary
		if(ss->m_iFlags & SSF_UPDATE_REVERB)
		{
			ReverbConvertI3DL2ToNative( &ss->m_ReverbParameters,  &ss->m_NativeReverbParameters );
			HRESULT hr = ss->m_pReverbSubmixVoice->SetEffectParameters( 0, &ss->m_NativeReverbParameters, sizeof(ss->m_NativeReverbParameters), operSet );
			hr=hr;
			ss->m_iFlags &= ~SSF_UPDATE_REVERB;
		}

		XSoundRef* next;
		for(XSoundRef* snd = ss->m_PlayingSounds.GetFirstNode(); snd; snd = next)
		{
			next = snd->ENF_LIST_NEXT;
			static_cast<Sound*>(snd->m_pSound)->Update(this, ss, snd, timeSlice, timeStamp, operSet);
		}
		Commit(operSet);
	}

	//update playing ambient sounds
	ListIterator<XSoundRef> playingAmb(ss->m_PlayingAmbientSounds);
	if(!ss->m_PlayingAmbientSounds.IsEmpty())
	{
		uint operSet = GetOperationSet();
		
		XSoundRef* next;
		for(XSoundRef* snd = ss->m_PlayingAmbientSounds.GetFirstNode(); snd; snd = next)
		{
			next = snd->ENF_LIST_NEXT;
			static_cast<Sound*>(snd->m_pSound)->Update(this, ss, snd, timeSlice, timeStamp, operSet);
		}

		Commit(operSet);
	}

	//update stopped sounds
	if(!ss->m_StoppedSounds.IsEmpty())
	{
		uint operSet = GetOperationSet();
		XSoundRef* next;
		for(XSoundRef* soundRef = ss->m_StoppedSounds.GetFirstNode(); soundRef; soundRef = next)
		{
			next = soundRef->ENF_LIST_NEXT;

			Sound* sound = static_cast<Sound*>(soundRef->m_pSound);

			sound->Update(this, ss, soundRef, timeSlice, timeStamp, operSet);
		}
		Commit(operSet);
	}
	ss->m_iFlags &= ~(SSF_UPDATE_LISTENER|SSF_UPDATE_SFX_VOLUME|SSF_UPDATE_MUSIC_VOLUME|SSF_UPDATE_3D);
}


//-----------------------------------------------------------------------
bool XAudio2System::EnsureFreeVoice(uint sceneIndex, const XSoundShader* xshader, const XSoundSource* source)
{
	SoundScene* ss = GetScene(sceneIndex);

	if(ss->m_PlayingSounds.GetCardinality() < MAX_SOUNDS_PER_SCENE)
		return true;

	//fuck! It's too much!
	//Find something further or lower priority
	Vector3 lpos = Vec3(ss->m_Listener.Position);
	float fardist = FLT_MAX;
	if(source)
	{
		Vector3 sndCenter = source->GetSoundCenter();
		fardist = enf_max(0.0f, (sndCenter - lpos).Size());
	}

	int lowestPriority = xshader->GetPriority();
	XSoundRef* farsnd = NULL;
	for(XSoundRef* snd = ss->m_PlayingSounds.GetFirstNode(); snd; snd = snd->ENF_LIST_NEXT)
	{
		SpatialSound* sps = static_cast<SpatialSound*>(snd->m_pSound);
		if(!sps->IsFlagsSet(SFX_ONCE))
			continue;
		if(sps->GetStreaming())
			continue;
		if(sps->GetPriority() > lowestPriority)
			continue;

		Vector3 pos = sps->m_pSource->GetSoundCenter();
		float sqdist = enf_max(0.0f, (pos - lpos).Size() - sps->GetRange());

		//lowest priority is more important than distance
		if(sps->GetPriority() < lowestPriority)
		{
			lowestPriority = sps->GetPriority();
			fardist = sqdist;
			farsnd = snd;
			continue;
		}

		if(sqdist > fardist)
		{
			fardist = sqdist;
			farsnd = snd;
		}
	}
	
	for(XSoundRef* snd = ss->m_PlayingAmbientSounds.GetFirstNode(); snd; snd = snd->ENF_LIST_NEXT)
	{
		AmbientSound* sps = static_cast<AmbientSound*>(snd->m_pSound);
		if(!sps->IsFlagsSet(SFX_ONCE))
			continue;
		if(sps->GetStreaming())
			continue;

		if(sps->GetPriority() < lowestPriority)
		{
			lowestPriority = sps->GetPriority();
			farsnd = snd;
			continue;
		}
	}

	if(farsnd == NULL)
	{
		//Screw you, Duso!
		return false;
	}

	//kill it!
	EndSound(ToHandle(farsnd));
	return true;
}

//-----------------------------------------------------------------------
SoundHandle XAudio2System::Play(uint sceneIndex, XSoundShader* sshader, uint flags, XSoundSource* source, float position)
{
	if(m_pXAudio2 == NULL || m_pMasteringVoice == NULL)
		return SOUND_NOT_PLAYING;

	CMutexLock lock(m_Mutex);

	enf_assert(sceneIndex < MAX_SOUND_SCENES);
	enf_assert(sshader != NULL);
	enf_assert2((flags & (SFX_AMBIENT|SFX_3D)) != (SFX_AMBIENT|SFX_3D), "Ambient and 3D flags are mutually exclusive");

	XAudio2Shader* xshader = XAudio2Shader::Cast(sshader);

	SoundHandle handle;
	SoundType st;

	//if not preference, choose what is in shader
	if((flags & (SFX_AMBIENT|SFX_3D)) == 0)
	{
		if(xshader->IsFlagSet(SSFX_3D))
			flags |= SFX_3D;
		else //if(xshader->IsFlagSet(SSFX_AMBIENT))
			flags |= SFX_AMBIENT; //default
	}

	if(m_FreeSources.IsEmpty())
		return SOUND_NOT_PLAYING;

	if(source != NULL)
	{
		if(source->GetNumSoundRefs() >= MAX_SOUNDS_PER_SOURCE)
		{
			return SOUND_NOT_PLAYING;
		}
	}
	else
	{
		//when we have no source, force it ambient
		flags &= ~SFX_3D;
		flags |= SFX_AMBIENT;
	}

	XSoundSample* _sample = xshader->GetSample();
	XAudio2Sample* sample = XAudio2Sample::Cast(_sample);
	
	if(sample == NULL || sample->CannotLoad())
		return SOUND_NOT_PLAYING;

	//force it ambient if shader want to
	if(xshader->IsFlagSet(SSFX_AMBIENT))
	{
		flags &= ~SFX_3D;
		flags |= SFX_AMBIENT;
	}
	
	//we cannot interrupt, nor seek streamed samples
	if(xshader->IsFlagSet(SSFX_STREAMING))
		flags |= SFX_NOTEST;

	//accept music feature
	if(xshader->IsFlagSet(SSFX_MUSIC))
		flags |= SFX_MUSIC;

	if(!xshader->IsFlagSet(SSFX_DIRECTIONAL))
		flags &= ~SFX_DIRECTIONAL;

	SoundScene* ss = GetScene(sceneIndex);

	if(flags & SFX_3D)
	{
		//unimportant once-sounds could be ignored completely
		if((flags & (SFX_ONCE|SFX_NOTEST)) == SFX_ONCE || (flags & SFX_DISCARDABLE) != 0)
		{
			Vector3 sndCenter = source->GetSoundCenter();
			if(!IsHearable(xshader->GetRange(), sceneIndex, sndCenter, false))
			{
				return SOUND_NOT_PLAYING;
			}
		}

		st = ST_SPATIAL;
	}
	else
	{
		st = ST_AMBIENT;

		//clear meaningless flags
		flags &= ~(SFX_DISCARDABLE|SFX_DIRECTIONAL|SFX_STATIC|SFX_NOTEST|SFX_DOPPLER);
	}
	if((flags & SFX_NOTEST) == 0 && !EnsureFreeVoice(sceneIndex, xshader, source))
		return SOUND_NOT_PLAYING;

	XSoundStreaming* sstreaming = NULL;
	if(xshader->IsFlagSet(SSFX_STREAMING))
	{
		sstreaming = sample->CreateStreaming(this);

		if(sstreaming == NULL)
			return SOUND_NOT_PLAYING;
	}
	else
	{
		if(sample->CannotLoad())
			return SOUND_NOT_PLAYING;

		if(!sample->IsLoaded())
			Load(sample, xshader->GetFlags());
		else
			sample->Touch(0);
	}

	if(sample->IsLoaded() && sample->GetFormat().nChannels > 1 && flags & SFX_3D)
	{
		enf_assert2(sample->GetFormat().nChannels == 1 || ((flags & SFX_3D) == 0), "Cannot play multichannel sound as 3D");
		return SOUND_NOT_PLAYING;
	}

	Sound* sound = AllocSound(sceneIndex, st, flags, xshader, handle);
	if(sound != NULL)
	{
		if(source)
		{
			bool refed = AddSourceRef(source, handle);
			enf_assert(refed);
		}
		
		sample->AddRef();
		
		if(st == ST_SPATIAL)
		{
			SpatialSound* ss = static_cast<SpatialSound*>(sound);

			if(flags & SFX_DIRECTIONAL)
			{
				ss->m_pCone = &xshader->m_Cone;
			}
		}
		if(xshader->IsFlagSet(SSFX_STREAMING))
		{
			sound->m_pStreaming = sstreaming;
		}
		else
		{
			sound->m_iBeginLoop = xshader->m_iBeginLoop;
			sound->m_iEndLoop = xshader->m_iEndLoop;
		}

		sound->m_pSource = source;
		sound->m_pSample = sample;
		sound->m_fTimePosition = enf_min(position, sample->m_fTimeLength);

		//set it always. There is always another factor (scene volume, master volume, etc..)
		sound->m_fVolume = sshader->GetVolume();
		sound->m_iFlags |= SFX_CHANGED_VOLUME;
		sound->m_iCurrentRate = sshader->GetRate();
	}

	return handle;
}

//-----------------------------------------------------------------------
SoundHandle XAudio2System::PlayFromSource(uint sceneIndex, XSoundShader* sshader, uint flags, XSoundSource* source, XAudioDataSource* dataSource)
{
	if(m_pXAudio2 == NULL || m_pMasteringVoice == NULL || dataSource == NULL)
		return SOUND_NOT_PLAYING;

	CMutexLock lock(m_Mutex);

	enf_assert(sceneIndex < MAX_SOUND_SCENES);
	enf_assert(sshader != NULL);
	enf_assert2((flags & (SFX_AMBIENT|SFX_3D)) != (SFX_AMBIENT|SFX_3D), "Ambient and 3D flags are mutually exclusive");

	XAudio2Shader* xshader = XAudio2Shader::Cast(sshader);

	enf_assert(xshader->IsFlagSet(SSFX_STREAMING));

	SoundHandle handle;
	SoundType st;

	//if not preference, choose what is in shader
	if((flags & (SFX_AMBIENT|SFX_3D)) == 0)
	{
		if(xshader->IsFlagSet(SSFX_3D))
			flags |= SFX_3D;
		else //if(xshader->IsFlagSet(SSFX_AMBIENT))
			flags |= SFX_AMBIENT; //default
	}

	if(source != NULL)
	{
		if(source->GetNumSoundRefs() >= MAX_SOUNDS_PER_SOURCE)
		{
			return SOUND_NOT_PLAYING;
		}
	}
	else
	{
		//when we have no source, force it ambient
		flags &= ~SFX_3D;
		flags |= SFX_AMBIENT;
	}

	if(dataSource == NULL)
		return SOUND_NOT_PLAYING;

	//force it ambient if shader want to
	if(xshader->IsFlagSet(SSFX_AMBIENT))
	{
		flags &= ~SFX_3D;
		flags |= SFX_AMBIENT;
	}

	//we cannot interrupt, nor seek streamed samples
	if(xshader->IsFlagSet(SSFX_STREAMING))
		flags |= SFX_NOTEST;

	//accept music feature
	if(xshader->IsFlagSet(SSFX_MUSIC))
		flags |= SFX_MUSIC;

	if(!xshader->IsFlagSet(SSFX_DIRECTIONAL))
		flags &= ~SFX_DIRECTIONAL;

	SoundScene* ss = GetScene(sceneIndex);

	if(flags & SFX_3D)
	{
		//unimportant once-sounds could be ignored completely
		if((flags & (SFX_ONCE|SFX_NOTEST)) == SFX_ONCE || (flags & SFX_DISCARDABLE) != 0)
		{
			Vector3 sndCenter = source->GetSoundCenter();
			if(!IsHearable(xshader->GetRange(), sceneIndex, sndCenter, false))
			{
				return SOUND_NOT_PLAYING;
			}
		}

		st = ST_SPATIAL;
	}
	else
	{
		st = ST_AMBIENT;

		//clear meaningless flags
		flags &= ~(SFX_DISCARDABLE|SFX_DIRECTIONAL|SFX_STATIC|SFX_NOTEST|SFX_DOPPLER);
	}
	if((flags & SFX_NOTEST) == 0 && !EnsureFreeVoice(sceneIndex, xshader, source))
		return SOUND_NOT_PLAYING;

	XSoundStreaming* sstreaming = NULL;

	sstreaming = CreateStreamingCustomData(dataSource); 

	if(sstreaming == NULL)
		return SOUND_NOT_PLAYING;

	uint channels, rate;
	dataSource->GetSoundInfo(channels, rate);

	if(channels > 1 && flags & SFX_3D)
	{
		enf_assert2(channels == 1 || ((flags & SFX_3D) == 0), "Cannot play multichannel sound as 3D");
		return SOUND_NOT_PLAYING;
	}

	Sound* sound = AllocSound(sceneIndex, st, flags, xshader, handle);
	if(sound != NULL)
	{
		if(source)
		{
			bool refed = AddSourceRef(source, handle);
			enf_assert(refed);
		}
		
		if(st == ST_SPATIAL)
		{
			SpatialSound* ss = static_cast<SpatialSound*>(sound);

			if(flags & SFX_DIRECTIONAL)
			{
				ss->m_pCone = &xshader->m_Cone;
			}
		}
		sound->m_pStreaming = sstreaming;
		sound->m_pSource = source;
		sound->m_pSample = NULL;
		sound->m_fTimePosition = 0;
		sound->m_iCurrentRate = rate;

		//set it always. There is always another factor (scene volume, master volume, etc..)
		sound->m_fVolume = sshader->GetVolume();
		sound->m_iFlags |= SFX_CHANGED_VOLUME;
	}

	return handle;
}

//-----------------------------------------------------------------------
bool XAudio2System::StopSound(SoundHandle id, uint flags)
{
	if(!IsSoundHandleOK(id))
		return false;

	CMutexLock lock(m_Mutex);
	XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return false;

	Sound* snd = static_cast<Sound*>(ref->m_pSound);

	if(!snd->IsFlagsSet(SFX_PAUSED))
	{
		if(snd->m_pSourceVoice != NULL)
		{
			Stop(snd->m_pSourceVoice, 0, XAUDIO2_COMMIT_NOW);
		}
		snd->SetFlags(SFX_PAUSED);
	}
	return true;
}

//-----------------------------------------------------------------------
bool XAudio2System::IsStopped(SoundHandle id) const
{
	if(!IsSoundHandleOK(id))
		return false;

	CMutexLock lock(m_Mutex);
	const XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return false;

	const Sound* snd = static_cast<Sound*>(ref->m_pSound);
	return snd->IsFlagsSet(SFX_PAUSED);
}

//-----------------------------------------------------------------------
bool XAudio2System::StartSound(SoundHandle id)
{
	if(!IsSoundHandleOK(id))
		return false;

	CMutexLock lock(m_Mutex);
	const XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return false;

	Sound* snd = static_cast<Sound*>(ref->m_pSound);
	if(!snd->IsFlagsSet(SFX_PAUSED))
		return false;

	snd->ClearFlags(SFX_PAUSED);
	if(snd->m_pSourceVoice != NULL && snd->m_State != SS_STOPPED)
	{
		if(snd->GetStreaming() == NULL || snd->GetStreaming()->IsStarted())
			Start(snd->m_pSourceVoice, 0, XAUDIO2_COMMIT_NOW);
	}
	return true;
}

//-----------------------------------------------------------------------
void XAudio2System::EndSounds(XSoundSource* source)
{
	CMutexLock lock(m_Mutex);
	if(source->GetNumSoundRefs() == 0)
		return;

	uint index = m_SourcesSet.GetIndexOf(MakePair(source, uint(0)));
	enf_assert(index != INDEX_NOT_FOUND);
	XSoundSourceRef* sref = &m_SourceRefs[m_SourcesSet[index].Data];

	if(!sref->m_Handles.IsEmpty()) //WTF???
	{
		for(int n = sref->m_Handles.GetCardinality() - 1; n >= 0; n--)
		{
			int prevCount = sref->m_Handles.GetCardinality();
			if(EndSound(sref->m_Handles[n]))
			{
				enf_assert(prevCount == (sref->m_Handles.GetCardinality() + 1));
			}
		}
	}
}

//-----------------------------------------------------------------------
bool XAudio2System::ExitSoundLoop(SoundHandle id)
{
	if(!IsSoundHandleOK(id))
		return false;

	CMutexLock lock(m_Mutex);
	XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return false;

	Sound* snd = static_cast<Sound*>(ref->m_pSound);
	if(snd->IsShaderFlagsSet(SSFX_STREAMING))
		return false;
	if(snd->IsFlagsSet(SFX_ONCE))
		return false;

	if(snd->m_pSourceVoice)
	{
		snd->m_pSourceVoice->ExitLoop();
	}
	else
	{
		//we cannot estimate loop when not actually playing.
		//End it gracefully instead ;)
		EndSound(id);
	}

	return true;
}

//-----------------------------------------------------------------------
bool XAudio2System::EndSound(SoundHandle id)
{
	if(!IsSoundHandleOK(id))
		return false;

	CMutexLock lock(m_Mutex);

	XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return false;

	Sound* snd = static_cast<Sound*>(ref->m_pSound);

	if(snd == NULL || snd->GetState() == SS_ENDED)
		return false; //very strange situation

	//must not exists yet
	if(snd->m_pSourceVoice && !snd->IsFlagsSet(SFX_PAUSED))
	{
		Stop(snd->m_pSourceVoice, 0, XAUDIO2_COMMIT_NOW);
	}

	FreeSound(ref);
	return true;
}

//-----------------------------------------------------------------------
uint XAudio2System::GetSampleFrequency(SoundHandle id)
{
	if(!IsSoundHandleOK(id))
		return -1;

	CMutexLock lock(m_Mutex);
	XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return -1;

	Sound* snd = static_cast<Sound*>(ref->m_pSound);
	if(snd->GetSample()->IsLoaded())
		return snd->GetSample()->GetFormat().nSamplesPerSec;
	else
		return 0;
}

//-----------------------------------------------------------------------
bool XAudio2System::SetFrequency(SoundHandle id, uint freq)
{
	if(!IsSoundHandleOK(id))
		return false;

	CMutexLock lock(m_Mutex);
	XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return false;

	Sound* snd = static_cast<Sound*>(ref->m_pSound);
	if(!snd->IsShaderFlagsSet(SSFX_FREQCONTROL))
		return false;

	uint newRate = freq;
	if(newRate != snd->m_iCurrentRate)
	{
		snd->m_iCurrentRate = newRate;
		snd->m_iFlags |= SFX_CHANGED_FREQUENCY;
	}
	return true;
}

//-----------------------------------------------------------------------
bool XAudio2System::SetVolume(SoundHandle id, float volume)
{
	if(!IsSoundHandleOK(id))
		return false;

	CMutexLock lock(m_Mutex);
	enf_assert(!BadFloat(volume));
	volume = enf_limit(volume, 0.0f, 1.0f);

	XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return false;

	Sound* snd = static_cast<Sound*>(ref->m_pSound);
	if(snd->m_fVolume != volume)
	{
		snd->m_fVolume = volume;
		snd->m_iFlags |= SFX_CHANGED_VOLUME;
	}
	return true;
}

//-----------------------------------------------------------------------
bool XAudio2System::SetPosition(SoundHandle id, float position)
{
	if(!IsSoundHandleOK(id))
		return false;

	CMutexLock lock(m_Mutex);
	XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return false;

	Sound* snd = static_cast<Sound*>(ref->m_pSound);
	snd->SetPosition(position, ref);
	return true;
}

//-----------------------------------------------------------------------
float XAudio2System::GetPosition(SoundHandle id) const
{
	if(!IsSoundHandleOK(id))
		return 0;

	CMutexLock lock(m_Mutex);
	const XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return 0;

	Sound* snd = static_cast<Sound*>(ref->m_pSound);
	return snd->GetPosition();
}

//-----------------------------------------------------------------------
bool XAudio2System::SetSoundCone(SoundHandle id, const XSoundCone& cone)
{
	if(!IsSoundHandleOK(id))
		return false;

	CMutexLock lock(m_Mutex);
	const XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return false;
	if(ref->m_pSound->GetType() != ST_SPATIAL)
		return false;
	if(!ref->m_pSound->IsFlagsSet(SFX_DIRECTIONAL))
		return false;

	if(cone.m_fInnerAngle > cone.m_fOuterAngle ||
		cone.m_fInnerAngle < 0.0f || cone.m_fInnerAngle > ENF_PI2 ||
		cone.m_fOuterAngle < 0.0f || cone.m_fOuterAngle > ENF_PI2 ||
		cone.m_fInnerLPF < 0.0f || cone.m_fInnerLPF > 1.0f ||
		cone.m_fOuterLPF < 0.0f || cone.m_fOuterLPF > 1.0f ||
		cone.m_fInnerReverb < 0.0f || cone.m_fInnerReverb > 1.0f ||
		cone.m_fOuterReverb < 0.0f || cone.m_fOuterReverb > 1.0f)
	{
		enf_assert(!"Bad parameters");
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------
float XAudio2System::GetLength(SoundHandle id) const
{
	if(!IsSoundHandleOK(id))
		return 0;

	CMutexLock lock(m_Mutex);
	const XSoundRef* ref = ToSoundRef(id);
	if(ref == NULL)
		return 0;

	return static_cast<Sound*>(ref->m_pSound)->GetSample()->GetTimeLength();
}

//-----------------------------------------------------------------------
void XAudio2System::SetSceneEffectsVolume(uint sceneIndex, float volume)
{
	SoundScene* scene = GetScene(sceneIndex);
	if(scene)
	{
		volume = enf_min(volume, 1.0f);
		if(scene->m_fSfxVolume != volume)
		{
			scene->m_fSfxVolume = volume;
			scene->m_fLocalSfxVolume = volume * m_fMasterSfxVolume;
			scene->m_iFlags |= SSF_UPDATE_SFX_VOLUME;
		}
	}
}

//-----------------------------------------------------------------------
void XAudio2System::SetSceneMusicVolume(uint sceneIndex, float volume)
{
	SoundScene* scene = GetScene(sceneIndex);
	if(scene)
	{
		volume = enf_limit(volume, 0.0f, 1.0f);
		if(scene->m_fMusicVolume != volume)
		{
			scene->m_fMusicVolume = volume;
			scene->m_fLocalMusicVolume = volume * m_fMasterMusicVolume;
			scene->m_iFlags |= SSF_UPDATE_MUSIC_VOLUME;
		}
	}
}

//-----------------------------------------------------------------------
void XAudio2System::SetMasterMusicVolume(float volume)
{
	volume = enf_limit(volume, 0.0f, 1.0f);
	if(m_fMasterMusicVolume != volume)
	{
		m_fMasterMusicVolume = volume;
		for(uint n = 0; n < MAX_SOUND_SCENES; n++)
		{
			SoundScene* scene = GetScene(n);
			if(scene)
			{
				scene->m_fLocalMusicVolume = scene->m_fMusicVolume * m_fMasterMusicVolume;
				scene->m_iFlags |= SSF_UPDATE_MUSIC_VOLUME;
			}
		}
	}
}

//-----------------------------------------------------------------------
void XAudio2System::SetMasterEffectsVolume(float volume)
{
	volume = enf_limit(volume, 0.0f, 1.0f);
	if(m_fMasterSfxVolume != volume)
	{
		m_fMasterSfxVolume = volume;
		for(uint n = 0; n < MAX_SOUND_SCENES; n++)
		{
			SoundScene* scene = GetScene(n);
			if(scene)
			{
				scene->m_fLocalSfxVolume = scene->m_fSfxVolume * m_fMasterSfxVolume;
				scene->m_iFlags |= SSF_UPDATE_SFX_VOLUME;
			}
		}
	}
}

//-----------------------------------------------------------------------
float XAudio2System::GetMasterMusicVolume() const
{
	return m_fMasterMusicVolume;
}

//-----------------------------------------------------------------------
float XAudio2System::GetMasterEffectsVolume() const
{
	return m_fMasterSfxVolume;
}

//-----------------------------------------------------------------------
bool XAudio2System::LoadEnvironments(CStr name)
{
CParser tp;

	if(tp.Parse(name.cstr()) != PE_OK)
   	return false;

XAudio2SoundEnv* env = NULL;

	bool res = true;
	bool first = true;

	for(uint l = 0; l < tp.GetNumLines() && res; l++)
   {
		if(!first)
		{
			if(!tp.NextLine(false))
				break;
		}
		else
			first = false;

		ISTOKEN(tp, "env")
		{
			if(env)
			{
				tp.LError(CParser::SequenceError);
				res = false;
				break;
			}

			if(!tp.NextToken(true))
			{
				res = false;
				continue;
			}

			if(tp.AssertType(TT_QUOTE))
			{
				env = ENF_NEW XAudio2SoundEnv(tp.CToken->Text);

				uint index = m_Environments.Insert(env);
				m_EnvironmentsHash.Insert(env->GetName().cstr(), index);
				continue;
			}
			res = false;
			continue;
		}

      if(!env)
      {
         tp.LError(CParser::SequenceError);
         res = false;
         break;
      }

		if(TTDefEqual(tp.CToken, CParser::TLCurly))
      	continue;

		if(TTDefEqual(tp.CToken, CParser::TRCurly))
      {
         env = NULL;
         continue;
		}
		
		CStr token = tp.CToken->Text;

		if(!tp.NextToken(true) || !tp.AssertType(TT_NUMBER))
		{
			res = false;
			continue;
		}

		if(token == "WetDryMix")
		{
			env->m_Properties.WetDryMix = CStr(tp.CToken->Text).ToFloat();
			continue;
		}
		if(token == "Room")
		{
			env->m_Properties.Room = CStr(tp.CToken->Text).ToInt();
			continue;
		}
		if(token == "RoomHF")
		{
			env->m_Properties.RoomHF = CStr(tp.CToken->Text).ToInt();
			continue;
		}
		if(token == "RoomRolloffFactor")
		{
			env->m_Properties.RoomRolloffFactor = CStr(tp.CToken->Text).ToFloat();
			continue;
		}
		if(token == "DecayTime")
		{
			env->m_Properties.DecayTime = CStr(tp.CToken->Text).ToFloat();
			continue;
		}
		if(token == "DecayHFRatio")
		{
			env->m_Properties.DecayHFRatio = CStr(tp.CToken->Text).ToFloat();
			continue;
		}
		if(token == "Reflections")
		{
			env->m_Properties.Reflections = CStr(tp.CToken->Text).ToInt();
			continue;
		}
		if(token == "ReflectionsDelay")
		{
			env->m_Properties.ReflectionsDelay = CStr(tp.CToken->Text).ToFloat();
			continue;
		}
		if(token == "Reverb")
		{
			env->m_Properties.Reverb = CStr(tp.CToken->Text).ToInt();
			continue;
		}
		if(token == "ReverbDelay")
		{
			env->m_Properties.ReverbDelay = CStr(tp.CToken->Text).ToFloat();
			continue;
		}
		if(token == "Diffusion")
		{
			env->m_Properties.Diffusion = CStr(tp.CToken->Text).ToFloat();
			continue;
		}
		if(token == "Density")
		{
			env->m_Properties.Density = CStr(tp.CToken->Text).ToFloat();
			continue;
		}
		if(token == "HFReference")
		{
			env->m_Properties.HFReference = CStr(tp.CToken->Text).ToFloat();
			continue;
		}
		res = false;
   }

	return res;
}

//-----------------------------------------------------------------------
uint XAudio2System::FindEnvironment(CStr name) const
{
	uint index = 0;
	if(m_EnvironmentsHash.Find(name, index))
		return index;

	return INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------
int mDbLerp(int val1, int val2, float lerp)
{
	float fval1 = log(float(val1) * 0.01f);
	float fval2 = log(float(val2) * 0.01f);
	float res = enf_lerp(fval1, fval2, lerp);
	return (int)(exp(res) * 100.0f);
}

//-----------------------------------------------------------------------
bool XAudio2System::SetEnvironment(uint sceneIndex, uint env1, uint env2, float lerp)
{
	const XAUDIO2FX_REVERB_I3DL2_PARAMETERS* pe1 = NULL;
	const XAUDIO2FX_REVERB_I3DL2_PARAMETERS* pe2 = NULL;

	if(env1 != INDEX_NOT_FOUND)
		pe1 = &static_cast<XAudio2SoundEnv*>(m_Environments[env1])->m_Properties;
	if(env2 != INDEX_NOT_FOUND)
		pe2 = &static_cast<XAudio2SoundEnv*>(m_Environments[env2])->m_Properties;

	XAUDIO2FX_REVERB_I3DL2_PARAMETERS params;

	if(pe1 == NULL && pe2 == NULL)
		return false;

	lerp = enf_limit(lerp, 0.0f, 1.0f);

	if(lerp == 0.0f || pe2 == NULL)
		params = *pe1;
	else if(lerp == 1.0f || pe1 == NULL)
		params = *pe2;
	else
	{
		// Standard I3DL2 parameters
		params.WetDryMix = enf_lerp(pe1->WetDryMix, pe2->WetDryMix, lerp);            // [0, 100] (percentage)
		params.Room = mDbLerp(pe1->Room, pe2->Room, lerp);    // [-10000, 0] in mB (hundredths of decibels)
		params.RoomHF = mDbLerp(pe1->RoomHF, pe2->RoomHF, lerp);    // [-10000, 0] in mB (hundredths of decibels)
		params.RoomRolloffFactor = enf_lerp(pe1->RoomRolloffFactor, pe2->RoomRolloffFactor, lerp);    // [0.0, 10.0]
		params.DecayTime = enf_lerp(pe1->DecayTime, pe2->DecayTime, lerp);    // [0.0, 10.0]
		params.DecayHFRatio = enf_lerp(pe1->DecayHFRatio, pe2->DecayHFRatio, lerp);    // [0.1, 2.0]
		params.Reflections = mDbLerp(pe1->Reflections, pe2->Reflections, lerp);    // [-10000, 1000] in mB (hundredths of decibels)
		params.ReflectionsDelay = enf_lerp(pe1->ReflectionsDelay, pe2->ReflectionsDelay, lerp);    // [0.0, 0.3] in seconds
		params.Reverb = mDbLerp(pe1->Reverb, pe2->Reverb, lerp);    // [-10000, 2000] in mB (hundredths of decibels)
		params.ReverbDelay = enf_lerp(pe1->ReverbDelay, pe2->ReverbDelay, lerp);    // [0.0, 0.1] in seconds
		params.Diffusion = enf_lerp(pe1->Diffusion, pe2->Diffusion, lerp);    // [0.0, 100.0] (percentage)
		params.Density = enf_lerp(pe1->Density, pe2->Density, lerp);    // [0.0, 100.0] (percentage)
		params.HFReference = enf_lerp(pe1->HFReference, pe2->HFReference, lerp);    // [20.0, 20000.0] in Hz
	}

	params.WetDryMix = enf_limit(params.WetDryMix, 0.0f, 100.0f);
	params.Room = enf_limit(params.Room, -10000, 0);
	params.RoomHF = enf_limit(params.RoomHF, -10000, 0);
	params.RoomRolloffFactor = enf_limit(params.RoomRolloffFactor, 0.0f, 10.0f);
	params.DecayTime = enf_limit(params.DecayTime, 0.1f, 20.0f);
	params.DecayHFRatio = enf_limit(params.DecayHFRatio, 0.1f, 2.0f);
	params.Reflections = enf_limit(params.Reflections, -10000, 1000);
	params.ReflectionsDelay = enf_limit(params.ReflectionsDelay, 0.0f, 0.3f);
	params.Reverb = enf_limit(params.Reverb, -10000, 2000);
	params.ReverbDelay = enf_limit(params.ReverbDelay, 0.0f, 0.1f);
	params.Diffusion = enf_limit(params.Diffusion, 0.0f, 100.0f);
	params.Density = enf_limit(params.Density, 0.0f, 100.0f);
	params.HFReference = enf_limit(params.HFReference, 20.0f, 20000.0f);

	SoundScene* scene = GetScene(sceneIndex);
	if(memcmp(&scene->m_ReverbParameters, &params, sizeof(params)) != 0)
	{
		scene->m_ReverbParameters = params;
		scene->m_iFlags |= SSF_UPDATE_REVERB;
	}
	return false;
}

//-----------------------------------------------------------------------
bool XAudio2System::IsInitialized() const
{
	return m_pXAudio2 != NULL && m_pMasteringVoice != NULL;
}

//-----------------------------------------------------------------------
bool XAudio2System::Initialize( void* hWnd, uint speakers, uint dwPrimaryChannels, uint dwPrimaryFreq, uint dwPrimaryBitRate, bool eax )
{
	if(m_pXAudio2 != NULL)
		return true;

#ifdef WIN32
	::CoInitializeEx( NULL, COINIT_MULTITHREADED );
#endif

#ifdef ENF_DEBUG_XAUDIO2
	HRESULT hr = ::XAudio2Create(&m_pXAudio2, XAUDIO2_DEBUG_ENGINE, XAUDIO2_DEFAULT_PROCESSOR);
#else
	HRESULT hr = ::XAudio2Create(&m_pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
#endif

	if(hr != S_OK)
	{
		enf::LogF(enf::LL_ERROR, "XAudio2Create returns %x", (int)hr);
		switch(hr)
		{
		case XAUDIO2_E_INVALID_CALL:
			enf::LogF(enf::LL_ERROR, "XAUDIO2_E_INVALID_CALL");	
			break;
		case XAUDIO2_E_XMA_DECODER_ERROR:
			enf::LogF(enf::LL_ERROR, "XAUDIO2_E_XMA_DECODER_ERROR");	
			break;
		case XAUDIO2_E_XAPO_CREATION_FAILED:
			enf::LogF(enf::LL_ERROR, "XAUDIO2_E_XAPO_CREATION_FAILED");	
			break;
		case XAUDIO2_E_DEVICE_INVALIDATED:
			enf::LogF(enf::LL_ERROR, "XAUDIO2_E_DEVICE_INVALIDATED");	
			break;
		}

		return false;
	}

	XAUDIO2_DEVICE_DETAILS details;
	if ( FAILED(hr = m_pXAudio2->GetDeviceDetails( 0, &details ) ) )
	{
		SAFE_RELEASE( m_pXAudio2 );
		return false;
	}

#define OUTPUTCHANNELS 8

	if ( details.OutputFormat.Format.nChannels > OUTPUTCHANNELS )
	{
		SAFE_RELEASE( m_pXAudio2 );
		return false;
	}

	m_iOutputChannelMask = details.OutputFormat.dwChannelMask;
	m_iOutputChannels = details.OutputFormat.Format.nChannels;

	XAUDIO2_EFFECT_CHAIN* pEffectChain = NULL;

	uint maxRate = enf_min(uint(XAUDIO2FX_REVERB_MAX_FRAMERATE), uint(details.OutputFormat.Format.nSamplesPerSec));

	hr = m_pXAudio2->CreateMasteringVoice(&m_pMasteringVoice,
				XAUDIO2_DEFAULT_CHANNELS,
				maxRate,//XAUDIO2_DEFAULT_SAMPLERATE,
				0,
				0,
				NULL);

	if(hr != S_OK)
		return false;

	if(m_pMasteringVoice == NULL)
		return false;

	XAUDIO2_VOICE_DETAILS voiceDetails;
	m_pMasteringVoice->GetVoiceDetails(&voiceDetails);

	ZeroMem(&X3DInstance, sizeof(X3DInstance));

#ifdef ENF_XENON
	X3DAudioInitialize( SPEAKER_XBOX, X3DAUDIO_SPEED_OF_SOUND, X3DInstance);
#else
	X3DAudioInitialize( m_iOutputChannelMask, X3DAUDIO_SPEED_OF_SOUND, X3DInstance );
#endif

	uint reverbFlags = 0;
	uint rflags = 0;

#ifdef ENF_DEBUG_XAUDIO2
	rflags |= XAUDIO2FX_DEBUG;
	reverbFlags |= XAUDIO2FX_DEBUG;
#endif

	for(uint n = 0; n < MAX_SOUND_SCENES; n++)
	{
		SoundScene* scene = GetScene(n);
		
		if ( SUCCEEDED(hr = XAudio2CreateReverb( &scene->m_pReverbEffect, reverbFlags)))
		{
			// Reverb effect in XAudio2 currently only supports mono channel count
			XAUDIO2_EFFECT_DESCRIPTOR effects[] = { { scene->m_pReverbEffect, true, 1 } };
			XAUDIO2_EFFECT_CHAIN effectChain = { 1, effects };

			if( SUCCEEDED(hr = m_pXAudio2->CreateSubmixVoice( &scene->m_pReverbSubmixVoice, 1,
				voiceDetails.InputSampleRate, 0, 0, NULL, &effectChain ) ) )
			{
				// Set default FX params
				XAUDIO2FX_REVERB_I3DL2_PARAMETERS defaultPreset = XAUDIO2FX_I3DL2_PRESET_FOREST;
				static XAUDIO2FX_REVERB_PARAMETERS native;
				ReverbConvertI3DL2ToNative( &defaultPreset, &native );
				scene->m_pReverbSubmixVoice->SetEffectParameters( 0, &native, sizeof(native) );
			}
		}
	}

	m_StreamThread.Start(5, "XAudio2Stream");

	return true;
}




//-----------------------------------------------------------------------
uint XAudio2System::NotificationProc()
{
	while(true)
	{
		CThread::Sleep(10);

		if(CThread::WaitForSingle(m_StopStreaming, 0) == 0)
			break;

		m_StreamingSetMutex.Lock();
		if(!m_StreamingSounds.IsEmpty())
		{
			uint operSet = 0;
			for(uint n = 0; n < m_StreamingSounds.GetCardinality();)
			{
				XSoundRef* ref = m_StreamingSounds[n];
				Sound* sound = static_cast<Sound*>(ref->m_pSound);
				XSoundStreaming* streaming = sound->m_pStreaming;

				XAUDIO2_VOICE_STATE state;
				sound->m_pSourceVoice->GetState(&state);
				if(sound->m_State == SS_STOPPED ||
					sound->IsFlagsSet(SFX_PAUSED) ||
					state.BuffersQueued >= 2)
				{
					n++;
					continue;
				}

				if(operSet == 0)
					operSet = GetOperationSet();

				if(!streaming->SubmitBuffer(this, ref, !sound->IsFlagsSet(SFX_ONCE), operSet))
					m_StreamingSounds.RemoveElement(n);
				else
					n++;
			}
			if(operSet != 0)
			{
				Commit(operSet);
			}
		}	
		m_StreamingSetMutex.Unlock();
	}
	return 0;
}

//-----------------------------------------------------------------------------
void StreamThread::Run()
{
#ifdef WIN32
	::CoInitializeEx( NULL, COINIT_MULTITHREADED );
#endif
	m_pSystem->NotificationProc();
#ifdef WIN32
	::CoUninitialize();
#endif
}

//-----------------------------------------------------------------------------
void StreamThread::Kill()
{
	m_pSystem->m_StopStreaming.Set();
	WaitForExit();
}

//-----------------------------------------------------------------------------
void XAudio2System::OnInsert(TiXmlElement* xmlnode)
{
	OnChangeSettings(xmlnode);
}
//-----------------------------------------------------------------------------
bool XAudio2System::OnChangeSettings(TiXmlElement* xmlnode)
{
	TiXmlElement* elem = xmlnode->FindOrCreateChildElement("audio");

	bool reverb = elem->FindOrCreateAttribute("reverb", "1").ToInt() != 0;
	CStr cocclusion = elem->FindOrCreateAttribute("occlusion", "1");
	CStr coutputrate = elem->FindOrCreateAttribute("outputrate", "44100");
	CStr cquality = elem->FindOrCreateAttribute("quality", "1");
	CStr cmusicvolume = elem->FindOrCreateAttribute("musicvolume", "100");
	CStr ceffectsvolume = elem->FindOrCreateAttribute("effectsvolume", "100");

	uint primaryRate = coutputrate.ToInt();

	SetMasterMusicVolume((float)enf_limit(cmusicvolume.ToInt(), 0, 100) * 0.01f);
	SetMasterEffectsVolume((float)enf_limit(ceffectsvolume.ToInt(), 0, 100) * 0.01f);

	return true;
}

//-----------------------------------------------------------------------------
uint XAudio2System::GetOperationSet() const
{
	//thus we will never get 0
	return IL_EXCHANGE_ADD(m_iOperSet, 2);
}

//-----------------------------------------------------------------------------
void XAudio2System::GetStats(XSoundSystemStats& stats)
{
	XAUDIO2_PERFORMANCE_DATA perfData;
	if(m_pXAudio2)
		m_pXAudio2->GetPerformanceData(&perfData);

	stats.NumSamples = m_Samples.GetCardinality();
	stats.NumLoadedSamples = m_LoadedSamples.GetCardinality();
	stats.NumShaders = m_Shaders.GetCardinality();
	stats.SamplesMem = m_iAllocatedBytes;
	stats.NumStreams = m_StreamingSounds.GetCardinality();
	stats.NumVoices = m_iNumActiveVoices;
	stats.NumAmbients = 0;
	stats.NumPlaying3DSounds = 0;
	stats.NumStoped3DSounds = 0;

	for(uint n = 0; n < MAX_SOUND_SCENES; n++)
	{
		const SoundScene* scene = GetScene(n);
		stats.NumAmbients += scene->m_PlayingAmbientSounds.GetCardinality();
		stats.NumPlaying3DSounds += scene->m_PlayingSounds.GetCardinality();
		stats.NumStoped3DSounds += scene->m_StoppedSounds.GetCardinality();
	}
};

//-----------------------------------------------------------------------------
float XAudio2System::GetSampleTimeLength(CStr sampleName)
{
	XSoundSample* sample;
	if(m_SampleHash.Find(sampleName, sample))
	{
		if(!sample->IsLoaded())
		{
			Load(sample, SSFX_FORCELOAD);
		}
		return sample->GetTimeLength();
	}

	ComPtr<XSoundSample> tempSample = ENF_NEW XAudio2Sample(this, sampleName);
	Load(tempSample, SSFX_FORCELOAD);
	return tempSample->GetTimeLength();
}

//-----------------------------------------------------------------------------
XAudio2Sample* XAudio2System::FindOrCreateSample(CStr sampleName)
{
	enf_assert(!sampleName.IsEmpty());

	XAudio2Sample* sample;
	XSoundSample* _sample;
	if(!m_SampleHash.Find(sampleName, _sample))
	{
		sample = ENF_NEW XAudio2Sample(this, sampleName);
		m_Samples.Insert(sample);
		m_SampleHash.Insert(sample->GetName().cstr(), sample);
	}
	else
	{
		sample = XAudio2Sample::Cast(_sample);
		sample->AddRef();
	}
	return sample;
}

//-----------------------------------------------------------------------------
XSoundShader* XAudio2System::CreateShader(CStr shaderName)
{
	enf_assert(!ExistsShader(shaderName));

	XAudio2Shader* shader = ENF_NEW XAudio2Shader(this, shaderName);
	m_Shaders.Insert(shader);
	m_ShadersHash.Insert(shader->GetName(), shader);
	return shader;
}

}