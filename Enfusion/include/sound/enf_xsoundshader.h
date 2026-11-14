//-----------------------------------------------------------------------------
// File: enf_xsoundshader.h
//
// Desc: XSoundSystem interface
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_XSOUNDSHADER_H
#define ENF_XSOUNDSHADER_H

#include "enf_config.h"
#include "engine/enf_item.h"
#include "common/enf_mathlib.h"
#include "common/enf_baseclass.h"

namespace enf
{
/** @addtogroup XSoundSystem
@{*/
class XSoundSystem;
class ISoundSystem;
class XSoundSample;
class XSoundShader;

enum XSoundShaderFlags
{
	SSFX_3D				= BITF(0),
	SSFX_AMBIENT		= BITF(1),
	SSFX_FREQCONTROL	= BITF(2),
	SSFX_STREAMING		= BITF(3),
	SSFX_MUSIC			= BITF(4),
	SSFX_LOOPING		= BITF(5),
	SSFX_GLOBAL			= BITF(6),
	SSFX_CYCLE			= BITF(7),
	SSFX_DIRECTIONAL	= BITF(8),
	SSFX_NOERROR		= BITF(9),
	SSFX_FORCELOAD		= BITF(10),
};

enum SoundShaderTypes
{
	SoundShaderClassTypeID = 10,
};

const uint XSoundShaderTypeID = 8510;

//-----------------------------------------------------------------------------
class PUBLIC_API SoundShaderClass: public BaseClass
{

public:
	ENF_DECLARE_ITEM(SoundShaderClass, BaseClass);

	DEFINE_PARAM(TYPE);
	DEFINE_PARAM(STREAM);
	DEFINE_PARAM(VOLUME);
	DEFINE_PARAM(RANGE);
	DEFINE_PARAM(PRIORITY);
	DEFINE_PARAM(MINRANGE);
	DEFINE_PARAM(ROLLOFF);
	DEFINE_PARAM(DOPPLER);
	DEFINE_PARAM(BPS);
	DEFINE_PARAM(BEGINLOOP);
	DEFINE_PARAM(ENDLOOP);
	DEFINE_PARAM(FREQUENCY);
	DEFINE_PARAM(NOTEST);
	DEFINE_PARAM(CYCLE);
	DEFINE_PARAM(FREQCONTROL);

	DEFINE_PARAM(DIRECTIONAL);	
	DEFINE_PARAM(INNERCONEANGLE);
	DEFINE_PARAM(OUTERCONEANGLE);
	DEFINE_PARAM(INNERCONEVOLUME);
	DEFINE_PARAM(OUTERCONEVOLUME);
	DEFINE_PARAM(INNERCONELPF);
	DEFINE_PARAM(OUTERCONELPF);
	DEFINE_PARAM(INNERCONEREVERB);
	DEFINE_PARAM(OUTERCONEREVERB);

	SoundShaderClass();

	bool IsVariableUsed(uint index, const BaseSource* src) const;
};

//-----------------------------------------------------------------------------
class PUBLIC_API SoundShaderCollection: public BaseSourceCollection
{
public:
	struct SourceFile
	{
		ENF_DECLARE_SMALLOBJECT(SourceFile);
		DString	name;
		bool		changed;
		SourceFile(): changed(false) {}
	};

	void Save(bool forceSaveAll);
	uint FindSourceFile(CStr fileName) const;
	uint AddSourceFile(CStr fileName);
	void ChangeSource(BaseSource* src);
	bool AddSource(BaseSource* source, uint index);
	uint GetNumFiles();
	CStr GetFile(uint n);

	SoundShaderCollection(): BaseSourceCollection() {}
	~SoundShaderCollection();

protected:
	uint FindBestFile(CStr name);

	Array<SourceFile*>	m_SourceFiles;
	HashMap<CStr, uint, CStrHashFuncCS>	m_SourceHash;
};

//-----------------------------------------------------------------------------
class PUBLIC_API SoundShaderSource: public BaseSource
{
protected:
	Array<DString>	m_Samples;

public:
	uint				m_iSourceFile;
	ENF_INLINE uint GetFile(){return m_iSourceFile;}
	void SetFile(uint file)
	{
		m_iSourceFile = file;
	}
	Array<DString>& GetSamples()
	{
		return m_Samples;
	}
	const Array<DString>& GetSamples() const
	{
		return m_Samples;
	}
	SoundShaderSource(SoundShaderClass* sclass): BaseSource(sclass), m_iSourceFile(INDEX_NOT_FOUND) {}
	SoundShaderSource(SoundShaderClass* sclass, CStr name): BaseSource(sclass, name), m_iSourceFile(INDEX_NOT_FOUND) {}
	~SoundShaderSource() {}
	virtual void CopyData(SoundShaderSource& target) const;

};


//-----------------------------------------------------------------------------
class PUBLIC_API XSoundShader: public Item
{
protected:
	friend class ISoundSystem;

	virtual ~XSoundShader();
	ISoundSystem*	m_pOwner;
	uint				m_iFlags;
	float				m_fVolume;
	float				m_fRolloff;
	float				m_fDoppler;
	float				m_fRange;
	float				m_fMinRange;
	ushort			m_iFrequency;
	char				m_iPriority;
	char				m_pad0;
	uint				m_iBeginLoop;
	uint				m_iEndLoop;

	ushort			m_iLastSample;
	ushort			m_iNumSamples;
	union
	{
		XSoundSample**	m_pSamples;
		XSoundSample*	m_pSample;
	};

	void FreeSamples();
	void InitSamples(uint numSamples);

	XSoundSample* GetSampleByIndex(uint index) const
	{
		enf_assert(index < m_iNumSamples);
		return (m_iNumSamples == 1) ? m_pSample : m_pSamples[index];
		
	}

public:
	ENF_DECLARE_ABSTRACT_ITEM(XSoundShader, Item);
	ENF_DECLARE_SMALLOBJECT(XSoundShader);

	bool IsFlagSet(uint flags) const
	{
		return (m_iFlags & flags) != 0;
	}

	uint GetFlags() const
	{
		return m_iFlags;
	}

	XSoundShader(ISoundSystem* sys, CStr name);
	uint GetNumSamples() const
	{
		return m_iNumSamples;
	}

	CStr GetSampleName(uint index) const;
	/*!
	\param index either 0...GetNumSamples, or INDEX_NOT_FOUND for random/cyclic
	*/
	XSoundSample* GetSample(uint index = INDEX_NOT_FOUND, bool doLoad = true);
	float GetRange() const { return m_fRange; }
	float GetMinRange() const { return m_fMinRange; }
	float GetDoppler() const { return m_fDoppler; }
	uint GetRate() const { return m_iFrequency; }
	float GetVolume() const { return m_fVolume; }
	int GetPriority() const { return m_iPriority; }

	virtual bool InitFromSource(SoundShaderSource* source)=0;
};


//@}

}

#endif //ENF_XSOUNDSHADER_H