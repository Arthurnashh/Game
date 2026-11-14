//-----------------------------------------------------------------------------
// File: enf_xaudio2shader.cpp
//
// Desc: Default XAudio2 sound system
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#include "sound/enf_xaudio2system.h"

namespace enf
{

//-----------------------------------------------------------------------
XAudio2Shader::XAudio2Shader(ISoundSystem* sys, CStr name):
	XSoundShader(sys, name)
{
	MemStats::m_iSoundSystem += sizeof(*this);
}

//-----------------------------------------------------------------------
XAudio2Shader::~XAudio2Shader()
{
	MemStats::m_iSoundSystem -= sizeof(*this);
}

//-----------------------------------------------------------------------
bool XAudio2Shader::InitFromSource(SoundShaderSource* source)
{
	XAudio2System* sys = XAudio2System::Cast(m_pOwner);
	enf_assert(sys);
	Array< ComPtr<XSoundSample> > samples;
	bool diff = true;

	//if we are already initialized, samples must be preserved before referencing the new ones
	if(GetNumSamples())
	{
		diff = GetNumSamples() != source->GetSamples().GetCardinality();

		if(!diff)
		{
			for(uint n = 0; n < GetNumSamples(); n++)
			{
				if(GetSample(n, false)->GetName() != source->GetSamples().GetElement(n).cstr())
				{
					diff = true;
					break;
				}
			}
		}

		if(diff)
		{
			for(uint n = 0; n < GetNumSamples(); n++)
			{
				XSoundSample* samp = GetSample(n, false);
				samp->AddRef();
				samples.Insert(samp);
			}
			FreeSamples();
		}
	}

	m_fVolume = source->GetScalarDef(SoundShaderClass::VOLUME, 1.0f);
	//m_iBPS = source->GetScalarDef(SoundShaderClass::BPS, 16);
	m_iFrequency = source->GetIntegerDef(SoundShaderClass::FREQUENCY, 0);
	m_iPriority = (char)enf_limit(source->GetIntegerDef(SoundShaderClass::PRIORITY, 0), -128, 127);
	m_fRolloff = source->GetScalarDef(SoundShaderClass::ROLLOFF, 1.0f);
	m_fDoppler = source->GetScalarDef(SoundShaderClass::DOPPLER, 1.0f);
	m_iBeginLoop = source->GetIntegerDef(SoundShaderClass::BEGINLOOP, 0);
	m_iEndLoop = source->GetIntegerDef(SoundShaderClass::ENDLOOP, 0);
	m_fRange = source->GetScalarDef(SoundShaderClass::RANGE, MAX_SOUND_RANGE);
	m_fMinRange = source->GetScalarDef(SoundShaderClass::MINRANGE, 16.0f);
	//kurva, proc to nastavujes stejne velky/vetsi? Stejny efekt ma ambient!!!
	if(m_fMinRange >= m_fRange)
		m_fMinRange = m_fRange * 0.75f;

	m_iFlags = 0;
	m_iFlags |= source->GetBooleanDef(SoundShaderClass::NOTEST, false) ? SFX_NOTEST : 0;
	m_iFlags |= source->GetBooleanDef(SoundShaderClass::CYCLE, false) ? SSFX_CYCLE : 0;
	m_iFlags |= source->GetBooleanDef(SoundShaderClass::FREQCONTROL, false) ? SSFX_FREQCONTROL : 0;
	m_iFlags |= source->GetBooleanDef(SoundShaderClass::STREAM, false) ? SSFX_STREAMING : 0;
	m_iFlags |= source->GetBooleanDef(SoundShaderClass::DIRECTIONAL, false) ? SSFX_DIRECTIONAL : 0;

	if(m_iEndLoop < m_iBeginLoop)
		enf_swap(m_iBeginLoop, m_iEndLoop);

	if(m_iBeginLoop != m_iEndLoop)
		m_iFlags |= SSFX_LOOPING;

	m_Cone.InnerAngle = enf_limit(DEG2RAD(source->GetScalarDef(SoundShaderClass::INNERCONEANGLE, 45.0f)), 0.0f, ENF_PI2);
	m_Cone.OuterAngle = enf_limit(enf_max(m_Cone.InnerAngle, DEG2RAD(source->GetScalarDef(SoundShaderClass::OUTERCONEANGLE, 90.0f))), 0.0f, ENF_PI2);
	m_Cone.InnerVolume = enf_limit(source->GetScalarDef(SoundShaderClass::INNERCONEVOLUME, 1.0f), 0.0f, 1.0f);
	m_Cone.OuterVolume = enf_limit(source->GetScalarDef(SoundShaderClass::OUTERCONEVOLUME, 0.708f), 0.0f, 1.0f);
	m_Cone.InnerLPF = enf_limit(source->GetScalarDef(SoundShaderClass::INNERCONELPF, 0.0f), 0.0f, 1.0f);
	m_Cone.OuterLPF = enf_limit(source->GetScalarDef(SoundShaderClass::OUTERCONELPF, 0.25f), 0.0f, 1.0f);
	m_Cone.InnerReverb = enf_limit(source->GetScalarDef(SoundShaderClass::INNERCONEREVERB, 0.708f), 0.0f, 1.0f);
	m_Cone.OuterReverb = enf_limit(source->GetScalarDef(SoundShaderClass::OUTERCONEREVERB, 1.0f), 0.0f, 1.0f);

	uint type = source->GetIntegerDef(SoundShaderClass::TYPE, 0);

	switch(type)
	{
	case 0:
		break;
	case 1:
		m_iFlags |= SSFX_AMBIENT;
		break;
	case 2:
		m_iFlags |= SSFX_3D;
		break;
	case 3:
		m_iFlags |= SSFX_MUSIC;
		break;
	}

	if(diff)
	{
		uint numSamples = source->GetSamples().GetCardinality();
		if(numSamples != 0)
		{
#ifdef ENF_CONSOLE
			//halves number of variants.
			numSamples = enf_max(1U, (numSamples / 2));
			if(numSamples > 1)
				m_pSamples = ENF_NEW XSoundSample*[numSamples];

			for(uint n = 0; n < numSamples; n++)
			{
				XAudio2Sample* sample = sys->FindOrCreateSample(source->GetSamples().GetElement(n * 2));
				if(m_iFlags & SSFX_STREAMING)
					sample->SetFlags(SSF_STREAMING);

				if(numSamples > 1)
					m_pSamples[n] = sample;
				else
					m_pSample = sample;
			}
#else
			if(numSamples > 1)
				m_pSamples = ENF_NEW XSoundSample*[numSamples];

			for(uint n = 0; n < numSamples; n++)
			{
				XAudio2Sample* sample = sys->FindOrCreateSample(source->GetSamples().GetElement(n));
				if(m_iFlags & SSFX_STREAMING)
					sample->SetFlags(SSF_STREAMING);

				if(numSamples > 1)
					m_pSamples[n] = sample;
				else
					m_pSample = sample;
			}
#endif
		}
		m_iNumSamples = numSamples;
	}
	return true;
}

}