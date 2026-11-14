//-----------------------------------------------------------------------------
// File: enf_xsoundsystem.h
//
// Desc: SoundSystem interface
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_XSOUNDSYSTEM_H
#define ENF_XSOUNDSYSTEM_H

#include "enf_config.h"
#include "engine/enf_module.h"
#include "common/enf_mathlib.h"
#include "sound/enf_xsoundsource.h"
#include "resource/enf_resource.h"

namespace enf
{
/** @defgroup XSoundSystem Sound system
@{*/
class CInputStream;
class ISoundSystem;
class XSoundSystem;
class XSoundSample;
class XSoundShader;
class XSoundsetDef;
class XSoundset;
class XSoundStreaming;
class SoundShaderClass;
class SoundShaderCollection;

const uint XSoundSystemTypeID = 8500;
const uint XSoundSampleTypeID = 8520;
const uint XSoundsetTypeID		= 8530;
const uint XSoundEnvTypeID		= 8540;
const uint ISoundSystemTypeID = 8501;

extern PUBLIC_API XSoundSystem* g_pXSoundSystem;

ENF_INLINE XSoundSystem* GetSoundSystem()
{
	return g_pXSoundSystem;
}

const float MAX_SOUND_RANGE = 8192;
const uint MAX_SOUNDS_PER_SOURCE = 16;

//-----------------------------------------------------------------------------
enum XSoundFlags
{
	//!Play only once
	SFX_ONCE				= BITF(16),
	//!Music. Has separate volume control, can be replaced by custom music
	SFX_MUSIC			= BITF(17),
	//!Ambient, not positional sound
	SFX_AMBIENT			= BITF(18),
	//!Positional sound
	SFX_3D				= BITF(19),
	//!Don't even start if not hearable. Valid only together with SFX_3D!
	SFX_DISCARDABLE	= BITF(20),
	//!Sound is positional. XSoundSource::GetSoundOrientation is called. Valid only together with SFX_3D!
	SFX_DIRECTIONAL	= BITF(21),
	//!Sound is moveable. XSoundSource::GetSoundVelocity is called. Valid only together with SFX_3D!
	SFX_DOPPLER			= BITF(22),
	//!Position is not changing, XSoundSource::GetSoundPosition/GetSoundOrientation is called only once upon start! Valid only together with SFX_3D!
	SFX_STATIC			= BITF(23),
	//!Don't test hearability. Valid only together with SFX_3D!
	SFX_NOTEST			= BITF(24),
	//!Play effect tails after sound ended
	SFX_PLAYTAILS		= BITF(26),

	//upto 28!
};

struct XSoundSystemStats
{
	uint	NumSamples;
	uint	NumLoadedSamples;
	uint	NumShaders;
	uint	SamplesMem;
	uint	NumVoices;
	uint	NumAmbients;
	uint	NumStreams;
	uint	NumPlaying3DSounds;
	uint	NumStoped3DSounds;
};

enum SoundSampleFlags
{
	SSF_STREAMING		= BITF(0),
	SSF_CANNOT_LOAD	= BITF(30), //<Cannot load
	SSF_LOADED			= BITF(31) //<Loaded
};

//-----------------------------------------------------------------------
class PUBLIC_API XAudioDataSource: public RefCountObject
{
protected:
	virtual ~XAudioDataSource() {}
public:
	XAudioDataSource() {}
	virtual uint BufferSizeInMSecs() const { return 2000; }
	virtual void GetSoundInfo(uint& channels, uint& rate) const=0;
	virtual uint FillData(void* buffer, uint numSamples, bool loop)=0;
	virtual uint DataReady(uint numSamples) const
	{
		return numSamples;
	}

};

//-----------------------------------------------------------------------------
class PUBLIC_API XSoundSample: public Item, public ResourceRef
{
protected:
	virtual ~XSoundSample();
	ISoundSystem*	m_pOwner;
	float				m_fTimeLength;
	uint				m_iNumSamples;
	uint				m_iFlags;
	uint				m_iPlaying;

public:
	ENF_DECLARE_ABSTRACT_ITEM(XSoundSample, Item);
	ENF_DECLARE_SMALLOBJECT(XSoundSample);
	ISoundSystem* GetOwner() const
	{
		return m_pOwner;
	}
	CStr GetName() const
	{
		return Item::GetName();
	}
	bool IsPlaying() const { return m_iPlaying > 0; }
	void BeginPlay()
	{
		m_iPlaying++;
	}
	void EndPlay()
	{
		enf_assert(m_iPlaying > 0);
		m_iPlaying--;
	}
	bool IsNotLoaded() const { return (m_iFlags & SSF_LOADED) == 0; }
	bool IsLoading() const { return ((m_iFlags & SSF_LOADED) == 0) && ResourceRef::IsLoading(); }
	bool IsLoaded() const { return (m_iFlags & SSF_LOADED) != 0; }
	bool CannotLoad() const { return (m_iFlags & SSF_CANNOT_LOAD) != 0; }
	uint SetFlags(uint flags)
	{
		m_iFlags |= flags;
		return m_iFlags;
	}
	uint ClearFlags(uint flags)
	{
		m_iFlags &= ~flags;
		return m_iFlags;
	}
	
	void SetLoaded()
	{
		SetFlags(SSF_LOADED);
	}

	bool IsFlagsSet(uint flags) const
	{
		return (m_iFlags & flags) != 0;
	}
	virtual void Unload()=0;

	XSoundSample(ISoundSystem* sys):
		m_pOwner(sys),
		m_fTimeLength(0),
		m_iNumSamples(0),
		m_iFlags(0),
		m_iPlaying(0)
	{
	}

	ENF_INLINE uint GetNumSamples() const
	{
		return m_iNumSamples;
	}

	ENF_INLINE float GetTimeLength() const
	{
		return m_fTimeLength;
	}

	virtual XSoundStreaming* CreateStreaming(ISoundSystem* sys)=0;
};

//!Either directional sound, or listener cone
struct XSoundCone
{
	float m_fInnerAngle; ///< inner cone angle in radians, must be within [0.0f, ENF_PI2]
	float m_fOuterAngle; ///< outer cone angle in radians, must be within [InnerAngle, ENF_PI2]
	float m_fInnerVolume; ///< volume level scaler on/within inner cone, used only for matrix calculations, must be within [0.0f, 2.0f] when used
	float m_fOuterVolume; ///< volume level scaler on/beyond outer cone, used only for matrix calculations, must be within [0.0f, 2.0f] when used
	float m_fInnerLPF;    ///< LPF (both direct and reverb paths) coefficient subtrahend on/within inner cone, used only for LPF (both direct and reverb paths) calculations, must be within [0.0f, 1.0f] when used
	float m_fOuterLPF;    ///< LPF (both direct and reverb paths) coefficient subtrahend on/beyond outer cone, used only for LPF (both direct and reverb paths) calculations, must be within [0.0f, 1.0f] when used
	float m_fInnerReverb; ///< reverb send level scaler on/within inner cone, used only for reverb calculations, must be within [0.0f, 2.0f] when used
	float m_fOuterReverb; ///< reverb send level scaler on/beyond outer cone, used only for reverb calculations, must be within [0.0f, 2.0f] when used

	//!creates default cone
	XSoundCone():
		m_fInnerAngle(ENF_PI/2),
		m_fOuterAngle(ENF_PI),
		m_fInnerVolume(1.0f),
		m_fOuterVolume(0.708f),
		m_fInnerLPF(0.0f),
		m_fOuterLPF(0.25f),
		m_fInnerReverb(0.708f),
		m_fOuterReverb(1.0f)
	{
	}

	XSoundCone(float innerAngle, float outerAngle, float innerVolume, float outerVolume, float innerLPF, float outerLPF, float innerReverb, float outerReverb):
		m_fInnerAngle(innerAngle),
		m_fOuterAngle(outerAngle),
		m_fInnerVolume(innerVolume),
		m_fOuterVolume(outerVolume),
		m_fInnerLPF(innerLPF),
		m_fOuterLPF(outerLPF),
		m_fInnerReverb(innerReverb),
		m_fOuterReverb(outerReverb)
	{
	}
};

//-----------------------------------------------------------------------------
struct ListenerParameters
{
	Vector3		m_vPos;
	Vector3		m_vForward;
	Vector3		m_vUp;
	Vector3		m_vVelocity;
	XSoundCone	m_Cone;

	ListenerParameters():
		m_vPos(0.0f), m_vForward(1.0f, 0.0f, 0.0f), m_vUp(0.0f, 0.0f, 1.0f), m_vVelocity(0.0f),
		m_Cone(ENF_PI * 5.0f/6.0f, ENF_PI * 11.0f/6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 0.708f, 1.0f)
	{
	}
};


//-----------------------------------------------------------------------------
class PUBLIC_API XSoundSystem: public Module
{
public:
	static const uint MAX_SOUND_SCENES = 8;

	ENF_DECLARE_ABSTRACT_ITEM(XSoundSystem, Module);

	virtual ~XSoundSystem() {}

	void SetCurrent()
	{
		g_pXSoundSystem = this;
	}

	virtual bool LoadShaders(SoundShaderCollection* coll)=0;
	virtual bool LoadShaderSources(SoundShaderCollection* coll, CStr name)=0;

	/*!
	Returns sound shader, or NULL when not exists
	\param shaderName The name
	\param flags Only SSFX_NOERROR/SSFX_FORCELOAD are allowed
	*/
	virtual XSoundShader* GetShader(CStr shaderName, uint flags)=0;
	virtual XSoundShader* CreateShader(CStr shaderName)=0;
	//!Check whether the shader does exist
	virtual bool ExistsShader(CStr shaderName) const=0;
	
	//!Returns number of shaders
	virtual uint GetNumShaders() const=0;
	//!Returns shader. This does not call AddRef!
	virtual XSoundShader* GetShader(uint index) const=0;

	virtual uint GetNumShaderClasses() const=0;
	virtual SoundShaderClass* GetShaderClass(uint index) const=0;
	virtual void AddShaderClass(SoundShaderClass* sclass)=0;
	virtual SoundShaderClass* FindShaderClass(CStr name) const=0;

	/*!
	\param sceneIndex Index of scene, 0...MAX_SOUND_SCENES
	\param shader Sound shader
	\param flags See XSoundFlags
	\param source Source of positional sound. Must be specified for SFX_3D sounds
	\param position Offset in seconds
	\return Handle of sound, or 0 when not started
	*/
	virtual SoundHandle Play(uint sceneIndex, XSoundShader* shader, uint flags, XSoundSource* source, float position = 0.0f)=0;
	virtual SoundHandle PlayFromSource(uint sceneIndex, XSoundShader* shader, uint flags, XSoundSource* source, XAudioDataSource* dataSource = NULL)=0;
	SoundHandle Play(uint sceneIndex, CStr shaderName, uint flags, XSoundSource* source, float position = 0.0f);
	SoundHandle Play(uint sceneIndex, XSoundset* ss, CStr soundId, uint flags, XSoundSource* source, float position = 0.0f);
	SoundHandle Play(uint sceneIndex, XSoundset* ss, uint soundIndex, uint flags, XSoundSource* source, float position = 0.0f);

	/*!
	Exits loop of looped sound. Ignored if played with SFX_ONCE
	\param id Sound handle
	\returns True when successful
	*/
	virtual bool ExitSoundLoop(SoundHandle id)=0;

	/*!
	Temporarily stops sound
	\param id Sound handle
	\param flags Only SFX_PLAYTAILS is allowed
	\return True when success, False, when no playing sound with the id exists
	*/
	virtual bool StopSound(SoundHandle id, uint flags)=0;
	
	virtual bool IsStopped(SoundHandle id) const=0;

	/*!
	Starts stopped sound
	\param id Sound handle
	\return True when success, False, when no stopped sound with the id exists
	*/
	virtual bool StartSound(SoundHandle id)=0;

	/*!
	Ends all sounds from the source
	\param source The source
	*/
	virtual void EndSounds(XSoundSource* source)=0;
	
	/*!
	Ends the sound
	\param id Sound handle
	\return True when success, False, when no playing sound with the id exists
	*/
	virtual bool EndSound(SoundHandle id)=0;

	/*!
	Sets frequency of sound in Hz
	\param id Sound handle
	\param freq Frequency of sound in Hz
	\return True when success, False, when no playing sound with the id exists
	*/
	virtual bool SetFrequency(SoundHandle id, uint freq)=0;

	/*!
	Returns source frequency of sample. Not the current one!
	\param id Sound handle
	*/
	virtual uint GetSampleFrequency(SoundHandle id)=0;

	/*!
	Sets volume of sound
	\param id Sound handle
	\param volume Multiplier of volume
	\return True when success, False, when no playing sound with the id exists
	*/
	virtual bool SetVolume(SoundHandle id, float volume)=0;

	/*!
	Sets position in seconds
	\param id Sound handle
	\param position Position in seconds
	\return True when success, False, when no playing sound with the id exists
	*/
	virtual bool SetPosition(SoundHandle id, float position)=0;

	/*!
	Gets position in seconds
	\param id Sound handle
	\return Position in seconds, or -1 when no playing sound with the id exists
	*/
	virtual float GetPosition(SoundHandle id) const=0;

	/*!
	Gets length in seconds
	\param id Sound handle
	\return Length in seconds, or -1 when no playing sound with the id exists
	*/
	virtual float GetLength(SoundHandle id) const=0;

	/*!
	Changes cone parameters.
	\param id Sound handle
	\param cone Cone parameters
	\return True when success, false otherwise (not SFX_DIRECTIONAL, or out of range parameters)
	*/
	virtual bool SetSoundCone(SoundHandle id, const XSoundCone& cone)=0;

	virtual void EndSounds(uint sceneIndex)=0;
	virtual void SetSceneEffectsVolume(uint sceneIndex, float volume)=0;
	virtual void SetSceneMusicVolume(uint sceneIndex, float volume)=0;
	virtual void SetMasterMusicVolume(float volume)=0;
	virtual void SetMasterEffectsVolume(float volume)=0;

	virtual float GetMasterMusicVolume() const=0;
	virtual float GetMasterEffectsVolume() const=0;

	virtual bool LoadSoundsetDefinitions(CStr name)=0;
	virtual XSoundset* GetSoundset(CStr name)=0;
	virtual const XSoundsetDef* GetSoundsetDef(CStr name) const=0;

	virtual bool LoadEnvironments(CStr name)=0;
	virtual uint FindEnvironment(CStr name) const=0;
	virtual uint GetNumEnvironment() const=0;
	virtual CStr GetEnvironment(uint index) const=0;
	virtual bool IsInitialized() const=0;
	virtual bool Initialize( void* hWnd, uint speakers, uint dwPrimaryChannels, uint dwPrimaryFreq, uint dwPrimaryBitRate, bool eax )=0;

	virtual void SetListener(uint sceneIndex, const ListenerParameters& params)=0;
	virtual bool SetEnvironment(uint sceneIndex, uint env1, uint env2, float lerp)=0;

	virtual void UpdateSounds(uint sceneIndex)=0;
	virtual void GetStats(XSoundSystemStats& stats)=0;

	/*!
	Returns length of the sample at any cost. It either gets length of already loaded sample
	or loads it just for length realization
	\param sampleName Name of the sample
	\returns Length in seconds
	*/
	virtual float GetSampleTimeLength(CStr sampleName)=0;
};

//@}
}

#endif //ENF_XSOUNDSYSTEM_H