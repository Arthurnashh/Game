//-----------------------------------------------------------------------------
// File: enf_isoundsystem.cpp
//
// Desc: Sound system implementation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#include "common/enf_quicksort.h"
#include "common/enf_parser.h"
#include "common/enf_streams.h"
#include "sound/enf_isoundsystem.h"
#include "sound/enf_xsoundset.h"
#include "sound/enf_xsoundshader.h"
#include "resource/enf_resourcemanager.h"
#include "system/enf_vfilesystem.h"
#include "enf_main.h"

#include <ogg/ogg.h>

namespace enf
{
XSoundSystem* g_pXSoundSystem;

DECLARE_PARAM(SoundShaderClass, TYPE);
DECLARE_PARAM(SoundShaderClass, STREAM);
DECLARE_PARAM(SoundShaderClass, VOLUME);
DECLARE_PARAM(SoundShaderClass, PRIORITY);
DECLARE_PARAM(SoundShaderClass, RANGE);
DECLARE_PARAM(SoundShaderClass, ROLLOFF);
DECLARE_PARAM(SoundShaderClass, MINRANGE);
DECLARE_PARAM(SoundShaderClass, DOPPLER);
DECLARE_PARAM(SoundShaderClass, BPS);
DECLARE_PARAM(SoundShaderClass, BEGINLOOP);
DECLARE_PARAM(SoundShaderClass, ENDLOOP);
DECLARE_PARAM(SoundShaderClass, FREQUENCY);
DECLARE_PARAM(SoundShaderClass, NOTEST);
DECLARE_PARAM(SoundShaderClass, CYCLE);
DECLARE_PARAM(SoundShaderClass, FREQCONTROL);

DECLARE_PARAM(SoundShaderClass, DIRECTIONAL);
DECLARE_PARAM(SoundShaderClass, INNERCONEANGLE);
DECLARE_PARAM(SoundShaderClass, OUTERCONEANGLE);
DECLARE_PARAM(SoundShaderClass, INNERCONEVOLUME);
DECLARE_PARAM(SoundShaderClass, OUTERCONEVOLUME);
DECLARE_PARAM(SoundShaderClass, INNERCONELPF);
DECLARE_PARAM(SoundShaderClass, OUTERCONELPF);
DECLARE_PARAM(SoundShaderClass, INNERCONEREVERB);
DECLARE_PARAM(SoundShaderClass, OUTERCONEREVERB);

//-----------------------------------------------------------------------
SoundShaderClass::SoundShaderClass(): BaseClass()
{
	static const ParmEnumScalar rangeEnumsX[] = {
		ParmEnumScalar("quart", MAX_SOUND_RANGE / 4),
		ParmEnumScalar("half", MAX_SOUND_RANGE / 2),
		ParmEnumScalar("high", MAX_SOUND_RANGE)
	};
	static const ParmEnum* rangeEnums[] = {
		&rangeEnumsX[0], &rangeEnumsX[1], &rangeEnumsX[2], NULL
	};

	static const ParmEnumInteger bpsEnumsX[] = {
		ParmEnumInteger("16", 16),
		ParmEnumInteger("8", 8)
	};
	static const ParmEnum* bpsEnums[] = {
		&bpsEnumsX[0], &bpsEnumsX[1], NULL
	};

	static const ParmEnumInteger typeEnumsX[] = {
		ParmEnumInteger("undefined", 0),
		ParmEnumInteger("ambient", 1),
		ParmEnumInteger("spatial", 2),
		ParmEnumInteger("music", 3),
	};
	static const ParmEnum* typeEnums[] = {
		&typeEnumsX[0], &typeEnumsX[1], &typeEnumsX[2], &typeEnumsX[3], NULL
	};

	DECLARE_INTEGER(TYPE, "type", "spatial", "combobox", NULL, "Type of sound", typeEnums);
	DECLARE_BOOLEAN(STREAM, "stream", "false", "Is streamed");
	DECLARE_SCALAR(VOLUME, "volume", "1.0", "slider", "0.0 1.0 0.01", "Volume", NULL);
	DECLARE_SCALAR(RANGE, "range", "4096.0", "editbox", "0.0 65536.0 0", "Max distance", rangeEnums);
	DECLARE_INTEGER(PRIORITY, "priority", "0", "slider", "-128 127 1", "Priority (higher overrides lower)", NULL);
	DECLARE_SCALAR(MINRANGE, "minrange", "0.0", "editbox", "0.0 65536.0 0", "Minimum distance", rangeEnums);
	DECLARE_SCALAR(ROLLOFF, "rolloff", "1.0", "slider", "0.1 2.0 0.1", "RolloffFactor", NULL);
	DECLARE_SCALAR(DOPPLER, "doppler", "1.0", "slider", "0.1 10 0.1", "Minimum distance", NULL);
	DECLARE_INTEGER(BPS, "bps", "16", "combobox", NULL, "Bits Per Sample", bpsEnums);
	DECLARE_INTEGER(BEGINLOOP, "beginloop", "0", "editbox", NULL, "Begin of loop in samples", NULL);
	DECLARE_INTEGER(ENDLOOP, "endloop", "0", "editbox", NULL, "End of loop in samples", NULL);
	DECLARE_INTEGER(FREQUENCY, "frequency", "44100", "editbox", "4800 48000 1", "Samples per second", NULL);
	DECLARE_BOOLEAN(NOTEST, "notest", "false", "Is testing hearability");
	DECLARE_BOOLEAN(CYCLE, "cycle", "false", "Cycling samples, instead of random");
	DECLARE_BOOLEAN(FREQCONTROL, "freqcontrol", "false", "Allows frequency change");

	DECLARE_BOOLEAN(DIRECTIONAL, "directional", "false", "Allows cone attenuation");
	DECLARE_SCALAR(INNERCONEANGLE, "innerconeangle", "45.0", "slider", "1 360 1", "Inner cone angle", NULL);
	DECLARE_SCALAR(OUTERCONEANGLE, "outerconeangle", "90.0", "slider", "1 360 1", "Outer cone angle", NULL);
	DECLARE_SCALAR(INNERCONEVOLUME, "innerconevolume", "1.0", "slider", "0 1 0.01", "Inner cone volume", NULL);
	DECLARE_SCALAR(OUTERCONEVOLUME, "outerconevolume", "0.708", "slider", "0 1 0.01", "Outer cone volume", NULL);
	DECLARE_SCALAR(INNERCONELPF, "innerconelpf", "0.0", "slider", "0 1 0.01", "Inner cone LPF", NULL);
	DECLARE_SCALAR(OUTERCONELPF, "outerconelpf", "0.25", "slider", "0 1 0.01", "Outer cone LPF", NULL);
	DECLARE_SCALAR(INNERCONEREVERB, "innerconereverb", "0.708", "slider", "0 1 0.01", "Inner cone reverb", NULL);
	DECLARE_SCALAR(OUTERCONEREVERB, "outerconereverb", "1.0", "slider", "0 1 0.01", "Outer cone reverb", NULL);
}

//-----------------------------------------------------------------------
bool SoundShaderClass::IsVariableUsed(uint index, const BaseSource* src) const
{
	if(index >= INNERCONEANGLE && index <= OUTERCONEREVERB && !src->GetBooleanDef(DIRECTIONAL, false))
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
void* enf_ogg_malloc(int _Size, const char* file, unsigned int line)
{
#ifdef _DEBUG
	return MemoryManager::Alloc(_Size, file, line);
#else
	return MemoryManager::Alloc(_Size, __FILE__, __LINE__);
#endif
}

void* enf_ogg_calloc(int _Count, int _Size, const char* file, unsigned int line)
{
#ifdef _DEBUG
	void* ptr = MemoryManager::Alloc(_Size * _Count, file, line);
#else
	void* ptr = MemoryManager::Alloc(_Size * _Count, __FILE__, __LINE__);
#endif
	if(ptr)
		ZeroMem(ptr, _Count * _Size);
	return ptr;
}

void* enf_ogg_realloc(void* _Memory, int _NewSize, const char* file, unsigned int line)
{
	return REALLOC(_Memory, _NewSize);
}

void enf_ogg_free(void* ptr)
{
	if(ptr)
		MemoryManager::Free(ptr);
}

//-----------------------------------------------------------------------
ISoundSystem::ISoundSystem(): m_iAllocatedBytes(0)
{
	//ogg_set_allocator(&enf_ogg_malloc, &enf_ogg_calloc, &enf_ogg_realloc, &enf_ogg_free);

	ZeroMem(m_pSoundScenes, sizeof(m_pSoundScenes));
	
	for(uint n = 0; n < MAX_SOUNDS; n++)
	{
		m_FreeSources.Insert(&m_SourceRefs[MAX_SOUNDS - 1 - n]);
	}
}

//-----------------------------------------------------------------------
void ISoundSystem::Cleanup()
{
	ResourceManager* res = GetResources();
	for(uint n = 0; n < m_Samples.GetCardinality(); n++)
	{
		res->CancelLoading(m_Samples[n]);
	}

	ForEach<ReleaseItem>(m_Shaders);
	ForEach<DeleteItem>(m_SoundsetDefs);

	if(!m_Shaders.IsEmpty())
	{
		enf::LogF(LL_ERROR, "--- Sound shaders leaks: ---");
		for(uint n = 0; n < m_Shaders.GetCardinality(); n++)
			enf::LogF(LL_ERROR, m_Shaders[n]->GetName().cstr());
		enf::LogF(LL_ERROR, "--- end ---");
	}
	m_Shaders.Clear();
	m_ShadersHash.Clear();

	if(!m_SourcesSet.IsEmpty())
	{
		enf::LogF(LL_ERROR, "--- SoundSet leaks: ---");
		for(uint n = 0; n < m_Soundsets.GetCardinality(); n++)
			enf::LogF(LL_ERROR, m_Soundsets[n]->GetName().cstr());
		enf::LogF(LL_ERROR, "--- end ---");
	}

	ForEach<SafeReleaseItem>(m_Environments);
	m_Environments.Clear();
	m_EnvironmentsHash.Clear();
	ForEach<DeleteItem>(m_ShaderClasses);
}

//-----------------------------------------------------------------------
ISoundSystem::~ISoundSystem()
{
}

//-----------------------------------------------------------------------
SoundShaderClass* ISoundSystem::FindShaderClass(CStr name) const
{
	uint index;

	if(m_ShaderClassesHash.Find(name, index))
		return m_ShaderClasses[index];

	return NULL;
}

//-----------------------------------------------------------------------
uint ISoundSystem::GetNumShaderClasses() const
{
	return m_ShaderClasses.GetCardinality();
}

//-----------------------------------------------------------------------
SoundShaderClass* ISoundSystem::GetShaderClass(uint index) const
{
	return m_ShaderClasses[index];
}

//-----------------------------------------------------------------------
void ISoundSystem::AddShaderClass(SoundShaderClass* sclass)
{
	if(!m_ShaderClassesHash.Contains(sclass->GetTypeName()))
	{
		uint index;
		m_ShaderClasses.Insert(sclass, &index);
		m_ShaderClassesHash.Insert(sclass->GetTypeName().cstr(), index);
	}
}

//-----------------------------------------------------------------------
void SoundShaderSource::CopyData(SoundShaderSource& target) const
{
	BaseSource::CopyData(target);
	target.m_Samples = m_Samples;
}
//-----------------------------------------------------------------------
bool ISoundSystem::LoadShaderSources(SoundShaderCollection* coll, CStr name)
{
	CParser tp;
	DString tname;

	SoundShaderClass* sclass = FindShaderClass("SoundShaderClass");
	if(sclass == NULL)
		return false;

	if(tp.Parse(name.cstr()) != PE_OK)
		return false;

	SoundShaderSource* shader = NULL;
	bool res = true;
	bool first = true;

	Array<DString> samples;
	TModule* lastmodule = NULL;
	uint fileindex = 0;

	for(uint l = 0; l < tp.GetNumLines() && res; l++)
	{
		if(!first)
		{
			if(!tp.NextLine(false))
				break;
		}
		else
			first = false;

		//HACK: solve it in CParser
		if(tp.CLine->m_iNumTokens == 0)
			continue;

		ISTOKEN(tp, "sound")
		{
			if(shader)
			{
				tp.LError(CParser::SequenceError);
				res = false;
				break;
			}

			if(!tp.NextToken(true))
			{
				res = false;
				break;
			}

			if(tp.AssertType(TT_QUOTE))
			{
				tname = tp.CToken->Text;
				tname.StripExt();

				shader = ENF_NEW SoundShaderSource(sclass, tname);
				if(tp.GetModule(tp.CLine->m_iModule) != lastmodule)
				{
					lastmodule = tp.GetModule(tp.CLine->m_iModule);
					fileindex = coll->FindSourceFile(lastmodule->GetName());

					if(fileindex == INDEX_NOT_FOUND)
					{
						fileindex = coll->AddSourceFile(lastmodule->GetName());
					}
				}

				if(!coll->AddSource(shader, fileindex))
				{
					tp.LError("Sound shader '%s' already exists", tname.cstr());
					res = false;
					SAFE_DELETE(shader);
					continue;
				}
				continue;
			}
			res = false;
			continue;
		}

		if(!shader)
		{
			tp.LError(CParser::SequenceError);
			res = false;
			break;
		}

		if(TTDefEqual(tp.CToken, CParser::TLCurly))
			continue;

		if(TTDefEqual(tp.CToken, CParser::TRCurly))
		{
			shader->GetSamples() = samples;
			samples.Clear();
			coll->AddSource(shader, fileindex);
			shader = NULL;
			continue;
		}

		if(tp.CToken->Type == TT_QUOTE)
		{
			tname = tp.CToken->Text;
			tname.StripExt();

			if(tname.IsEmpty())
			{
				tp.LError("Empty sample name");
				res = false;
				continue;
			}
			samples.Insert(DString(tname));
			continue;
		}

		const BaseVariableDef* vdef = sclass->GetVariableDef(tp.CToken->Text);
		if(vdef == NULL)
		{
			tp.LError("Unknown keyword %s", tp.CToken->Text);
			res = false;
			continue;
		}
		if(!tp.NextToken(true))
		{
			res = false;
			continue;
		}
		/*
		if(vdef->m_Type == DVT_INTEGER || vdef->m_Type == DVT_SCALAR)
		{
			if(tp.CToken->Type != TT_NUMBER)
			{
				enf::LogF(LL_ERROR, "CParser::ParmParseError);
				res = false;
				continue;
			}
		}*/
		if(!shader->SetVariable(vdef->m_iIndex, tp.CToken->Text))
		{
			res = false;
			continue;
		}
	}

	if(!res)
	{
		delete coll;
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
uint ISoundSystem::GetNumEnvironment() const
{
	return m_Environments.GetCardinality();
}

//-----------------------------------------------------------------------------
CStr ISoundSystem::GetEnvironment(uint index) const
{
	return m_Environments[index]->GetName();
}

//-----------------------------------------------------------------------
bool ISoundSystem::AddSourceRef(XSoundSource* source, SoundHandle handle)
{
	XSoundSourceRef* newRef;
	if(source->GetNumSoundRefs() >= MAX_SOUNDS_PER_SOURCE)
		return false;

	if(source->GetNumSoundRefs() > 0)
	{
		uint index = m_SourcesSet.GetIndexOf(MakePair(source, uint(0)));
		enf_assert(index != INDEX_NOT_FOUND);

		newRef = &m_SourceRefs[m_SourcesSet[index].Data];
	}
	else
	{
		enf_assert(!m_SourcesSet.Contains(source));

		//no free unique sources
		newRef = m_FreeSources.GetFirstNode();
		if(newRef == NULL)
			return false;
		m_FreeSources.Remove(newRef);
		m_ActiveSources.Insert(newRef);

		uint index;
		m_SourcesSet.Insert(MakePair(source, uint(0)), &index);
		m_SourcesSet[index].Data = newRef - m_SourceRefs;
	}

	enf_assert(!newRef->m_Handles.Contains(handle));
	newRef->m_Handles.Insert(handle);
	source->AddSoundRef();
	return true;
}

//-----------------------------------------------------------------------
void ISoundSystem::RemoveSourceRef(XSoundSource* source, SoundHandle handle)
{
	enf_assert(source->GetNumSoundRefs() != 0);

	uint index = m_SourcesSet.GetIndexOf(MakePair(source, uint(0)));
	enf_assert(index != INDEX_NOT_FOUND);
	XSoundSourceRef* newRef = &m_SourceRefs[m_SourcesSet[index].Data];
	enf_assert(newRef->m_Handles.Contains(handle));
	newRef->m_Handles.Remove(handle);
	if(source->RemoveSoundRef() == 0)
	{
		m_ActiveSources.Remove(newRef);
		m_FreeSources.Insert(newRef);
		m_SourcesSet.RemoveElement(index);
	}
}
//-----------------------------------------------------------------------
void XSoundRef::InvalidateHandle()
{
	uint spinCount = (uint)((m_iSpinCount + 1) & MAX_SPINCOUNT_MASK);
	m_iSpinCount = (ushort)enf_max(spinCount, 1U);
}

//-----------------------------------------------------------------------
struct SampleAge
{
	uint	age;
	XSoundSample* sample;
};

class CompareSamples
{
public:
	int operator()(const SampleAge& s0, const SampleAge& s1) const
	{
		if(s0.age > s1.age)
			return -1;
		if(s0.age < s1.age)
			return 1;
		return 0;
	}
};

//-----------------------------------------------------------------------
void ISoundSystem::LRUInsert(XSoundSample* sample)
{
	size_t size = sample->GetSize();
	enf_assert(size < m_iMaxAllocated);

	m_iAllocatedBytes += size;
	if(m_iAllocatedBytes >= m_iMaxAllocated)
	{
		//free something
		Set<XSoundSample*>& samples = m_LoadedSamples;
		SampleAge* tempSamples = ALLOCA(SampleAge, samples.GetCardinality());
		uint numOldSamples = 0;
		for(uint n = 0; n < samples.GetCardinality(); n++)
		{
			if(samples[n]->IsLoading())
				continue;
			if(samples[n]->IsPlaying())
				continue;

			uint age = samples[n]->GetAge();
			if(age > 1)
			{
				tempSamples[numOldSamples].age = age;
				tempSamples[numOldSamples].sample = samples[n];
				numOldSamples++;
			}
		}
		QuickSortCmp<SampleAge, CompareSamples>(numOldSamples, tempSamples);

		uint index = 0;
		while(m_iAllocatedBytes >= m_iMaxAllocated && index < numOldSamples)
		{
			tempSamples[index++].sample->Unload();
		}
	}
	m_LoadedSamples.Insert(sample);
}

//-----------------------------------------------------------------------
void ISoundSystem::LRURemove(XSoundSample* sample)
{
	//sample must not be in LRU yet (after Load, before Apply)
	if(m_LoadedSamples.Remove(sample))
	{
		enf_assert(sample->GetSize() <= m_iAllocatedBytes);
		m_iAllocatedBytes -= sample->GetSize();
	}
}

//-----------------------------------------------------------------------
void ISoundSystem::InitSounds()
{
	m_FreeSounds.Clear();
	for(int n = 0; n < MAX_SOUND_SCENES; n++)
	{
		m_pSoundScenes[n]->m_PlayingSounds.Clear();
		m_pSoundScenes[n]->m_StoppedSounds.Clear();
	}

	for(int n = 0; n < MAX_SOUNDS; n++)
	{
		m_FreeSounds.Insert(&m_Sounds[n]);
	}
}

//-----------------------------------------------------------------------
SoundHandle XSoundSystem::Play(uint sceneIndex, XSoundset* ss, CStr soundId, uint flags, XSoundSource* source, float position)
{
	enf_assert(ss != NULL);
	const XSoundSetRef* sref = ss->GetSound(soundId);
	if(sref)
	{
		return Play(sceneIndex, sref->GetShader(), flags, source, position);
	}

	return 0;
}
//-----------------------------------------------------------------------
SoundHandle XSoundSystem::Play(uint sceneIndex, CStr shaderName, uint flags, XSoundSource* source, float position)
{
	ComPtr<XSoundShader> shader = GetShader(shaderName, 0);
	if(shader.IsNull())
		return SOUND_NO_SHADER;

	return Play(sceneIndex, shader, flags, source, position);
}

//-----------------------------------------------------------------------
SoundHandle XSoundSystem::Play(uint sceneIndex, XSoundset* ss, uint soundIndex, uint flags, XSoundSource* source, float position)
{
	enf_assert(ss != NULL);
	const XSoundSetRef* sref = ss->GetSound(soundIndex);
	if(sref)
	{
		return Play(sceneIndex, sref->GetShader(), flags, source, position);
	}

	return 0;
}

//-----------------------------------------------------------------------
bool ISoundSystem::ExistsShader(CStr shaderName) const
{
	return m_ShadersHash.Contains(shaderName.cstr());
}

//-----------------------------------------------------------------------
XSoundShader* ISoundSystem::GetShader(CStr strName, uint flags)
{
	XSoundShader* ss;

	flags &= SSFX_NOERROR|SSFX_FORCELOAD;

	if(m_ShadersHash.Find(strName.cstr(), ss))
	{
		ss->AddRef();
		if((flags & SSFX_FORCELOAD) == SSFX_FORCELOAD)
		{
			Load(ss, flags);
		}
		return ss;
	}

	if(!(flags & SSFX_NOERROR))
		enf::LogF(LL_ERROR, "Missing sound shader %s", strName.cstr());

	return NULL;
}

//-----------------------------------------------------------------------
bool ISoundSystem::Load(XSoundShader* shader, uint shaderFlags)
{
	//streaming ones are loaded later
	if(shader->IsFlagSet(SSFX_STREAMING))
		return true;

	shaderFlags |= shader->GetFlags();
	for(uint n = 0; n < shader->m_iNumSamples; n++)
	{
		XSoundSample* sample = shader->GetSampleByIndex(n);
		if(!sample->IsLoaded())
			Load(sample, shaderFlags);
	}

	return true;
}

//-----------------------------------------------------------------------
bool ISoundSystem::LoadShaders(SoundShaderCollection* coll)
{
	enf_assert(coll != NULL);

	for(uint n = 0; n < coll->GetNumSources(); n++)
	{
		SoundShaderSource* source = static_cast<SoundShaderSource*>(coll->GetSource(n));
		XSoundShader* shader = CreateShader(source->GetName());
		shader->InitFromSource(source);
	}

	return true;
}

//-----------------------------------------------------------------------
CStr XSoundShader::GetSampleName(uint index) const
{
	return GetSampleByIndex(index)->GetName().cstr();
}

//-----------------------------------------------------------------------
XSoundSample* XSoundShader::GetSample(uint index, bool doLoad)
{
	if(m_iNumSamples == 0)
	{
		enf_assert(index == INDEX_NOT_FOUND);
		return NULL;
	}

	if(m_iNumSamples <= 1)
	{
		enf_assert(index == INDEX_NOT_FOUND || index == 0);
		return m_pSample;
	}

	uint num;

	if(index == INDEX_NOT_FOUND)
	{
		if(m_iFlags & SSFX_CYCLE)
		{
			num = m_iLastSample++;
			if(m_iLastSample >= m_iNumSamples)
				m_iLastSample = 0;
		}
		else
		{
			//ensure that we never get two the same in row
			num = rand() * (m_iNumSamples - 1) / RAND_MAX;
			if(num >= m_iLastSample)
				num++;

			m_iLastSample = num = enf_min(num, (uint)(m_iNumSamples - 1));
		}
	}
	else
	{
		enf_assert(index < m_iNumSamples);
		num = index;
	}
	return m_pSamples[num];
}

//-----------------------------------------------------------------------
void XSoundShader::FreeSamples()
{
	if(m_iNumSamples <= 1)
	{
		SAFE_RELEASE(m_pSample);
	}
	else
	{
		for(uint n = 0; n < m_iNumSamples; n++)
			SAFE_RELEASE(m_pSamples[n]);
		SAFE_DELETE_ARRAY(m_pSamples);
		MemStats::m_iSoundSystem -= sizeof(XSoundSample*) * m_iNumSamples;
	}
}

//-----------------------------------------------------------------------
void XSoundShader::InitSamples(uint numSamples)
{
	m_iNumSamples = numSamples;
	if(numSamples != 0)
	{
		if(numSamples == 1)
			m_pSample = NULL;
		else
		{
			m_pSamples = ENF_NEW XSoundSample*[numSamples];
			ZeroMem(m_pSamples, numSamples * sizeof(XSoundSample*));
			MemStats::m_iSoundSystem += numSamples * sizeof(XSoundSample*);
		}
	}
}

//-----------------------------------------------------------------------
XSoundShader::~XSoundShader()
{
	FreeSamples();
	if(m_pOwner)
	{
		m_pOwner->m_Shaders.Remove(this);
		m_pOwner->m_ShadersHash.Remove(GetName());
	}
}

//-----------------------------------------------------------------------
XSoundSample::~XSoundSample()
{
	if(m_pOwner)
	{
		m_pOwner->m_Samples.Remove(this);
		m_pOwner->m_SampleHash.Remove(GetName());
	}
}

//-----------------------------------------------------------------------
SoundShaderCollection::~SoundShaderCollection()
{
	ForEach<DeleteItem>(m_SourceFiles);
}

//-----------------------------------------------------------------------
void SoundShaderCollection::ChangeSource(BaseSource* src)
{
	SoundShaderSource* ss = static_cast<SoundShaderSource*>(src);
	if(ss->m_iSourceFile != INDEX_NOT_FOUND)
		m_SourceFiles[ss->m_iSourceFile]->changed = true;
}

//-----------------------------------------------------------------------
uint SoundShaderCollection::FindSourceFile(CStr fileName) const
{
	uint index;

	if(m_SourceHash.Find(fileName, index))
		return index;
	return INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------
uint SoundShaderCollection::AddSourceFile(CStr fileName)
{
	SourceFile* sf = ENF_NEW SourceFile();
	sf->name = fileName;
	uint index = m_SourceFiles.Insert(sf);
	m_SourceHash.Insert(sf->name.cstr(), index);
	return index;
}

//-----------------------------------------------------------------------
bool SoundShaderCollection::AddSource(BaseSource* source, uint index)
{
	if(BaseSourceCollection::AddSource(source))
	{
		SoundShaderSource* ss = static_cast<SoundShaderSource*>(source);
		ss->m_iSourceFile = index;

		if(ss->m_iSourceFile == INDEX_NOT_FOUND)
		{
			ss->m_iSourceFile = FindBestFile(source->GetName());
			if(ss->m_iSourceFile == INDEX_NOT_FOUND)
				ss->m_iSourceFile = 0;
		}
		ChangeSource(source);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------
uint SoundShaderCollection::FindBestFile(CStr name)
{
	SString<> temp;
	temp = name.cstr();
	temp.lower();

	for(uint f = 0; f < m_SourceFiles.GetCardinality(); f++)
	{
		SString<> filename;
		filename = m_SourceFiles[f]->name.cstr();
		filename.StripExt();
		int slash = filename.FindR('/');
		if(slash >= 0)
			filename = filename.cstr() + slash + 1;

		if(CStr(temp.cstr()).strstr(filename.cstr()))
			return f;
	}

	return 0;
}

//-----------------------------------------------------------------------
uint SoundShaderCollection::GetNumFiles()
{
	return m_SourceFiles.GetCardinality();
}

//-----------------------------------------------------------------------
CStr SoundShaderCollection::GetFile(uint n)
{
	return m_SourceFiles[n]->name.cstr();
}

//-----------------------------------------------------------------------
void SoundShaderCollection::Save(bool forceSaveAll)
{
	Set<Pair<CStr, BaseSource*> > sortedSources;
	Ptr<VFile>* files = ALLOCAINIT(Ptr<VFile>, m_SourceFiles.GetCardinality());
	SString<256> lerror;
	SString<> temp;

	bool res = true;

	for(uint n = 0; n < m_Sources.GetCardinality(); n++)
		sortedSources.Insert(MakePair(m_Sources[n]->GetName(), m_Sources[n]));

	for(uint n = 0; n < sortedSources.GetCardinality(); n++)
	{
		const SoundShaderSource* ss = static_cast<const SoundShaderSource*>(sortedSources[n].Data);
		if(!forceSaveAll && !m_SourceFiles[ss->m_iSourceFile]->changed)
			continue;

		VFile* vf = files[ss->m_iSourceFile];

		//do we have opened file?
		if(vf == NULL)
		{
			vf = GetFileSystem()->Open(m_SourceFiles[ss->m_iSourceFile]->name, FILEMODE_WRITE);

			if(vf == NULL)
			{
				lerror.format("File '%s' cannot be opened.\n", m_SourceFiles[ss->m_iSourceFile]->name.cstr());
/*				size_t len = lerror.Length();
				if((len + pos + 1) < maxsize)
				{
					memcpy(errors + pos, lerror.cstr(), len);
					pos += len;
					errors[pos] = 0;
				}
*/
				res = false;
				vf = (VFile*)0xffffffff;
			}

			files[ss->m_iSourceFile] = vf;
		}

		if(vf == (VFile*)0xffffffff)
		{
			//couldn't open this file
			continue;
		}

		CFileOutputStream ostr(vf, true);

		ostr << "sound \"";
		ostr << ss->GetName().cstr();
		ostr << "\"\n{\n";

		const Array<DString>& samples = ss->GetSamples();
		for(uint n = 0; n < samples.GetCardinality(); n++)
		{
			ostr << "\t\"";
			ostr << samples[n];
			ostr << "\"\n";
		}

		const BaseClass* sc = ss->GetClass();

		for(uint n = 0; n < ss->GetNumVariables(); n++)
		{
			if(!ss->IsVariableSet(n))
				continue;

			const BaseVariableDef& vd = sc->GetVariableDef(n);

			ostr << '\t';
			ostr << vd.m_strName.cstr();
			ostr << ' ';

			switch(vd.m_Type)
			{
			case DVT_STRING:
				ostr << '\"';
				ostr << ss->GetString(n);
				ostr << '\"';
				break;

			case DVT_BOOLEAN:
				ostr << (ss->GetBoolean(n) ? "true" : "false");
				break;

			case DVT_INTEGER:
				{
				const char* strEnum = vd.FindEnumInteger(ss->GetInteger(n));
				if(strEnum)
					ostr << strEnum;
				else
					ostr << temp.format("%d", ss->GetInteger(n));
				}
				break;
			case DVT_SCALAR:
				{
				const char* strEnum = vd.FindEnumScalar(ss->GetScalar(n));
				if(strEnum)
					ostr << strEnum;
				else
					ostr << temp.format("%f", ss->GetScalar(n));
				}
				break;
			default:
				enf_assert("!Unsupported");
				break;
			}

			ostr << '\n';
		}
		ostr << "}\n\n";
	}

	for(uint n = 0; n < m_SourceFiles.GetCardinality(); n++)
	{
		VFile* file = files[n];

		if(file && file != (VFile*)0xffffffff)
			file->Close();
	}
}

//-----------------------------------------------------------------------
void XEnvironmentVolume::AddChild(XEnvironmentVolume* child)
{
	enf_assert(child->m_pParent != this);
	if(child->m_pParent)
		child->m_pParent->RemoveChild(child);

	XEnvironmentVolume** prev = &m_pChildren;
	while(*prev) prev = &(*prev)->m_pSibling;
	*prev = child;
	child->m_pSibling = NULL;
	child->m_pParent = this;
}

//-----------------------------------------------------------------------
void XEnvironmentVolume::RemoveChild(XEnvironmentVolume* child)
{
	enf_assert(child->m_pParent == this);
	XEnvironmentVolume** prev = &m_pChildren;
	while(*prev)
	{
		if(*prev == child)
		{
			*prev = child->m_pSibling;
			child->m_pSibling = NULL;
			child->m_pParent = NULL;
			return;
		}
		prev = &(*prev)->m_pSibling;
	}
}

//-----------------------------------------------------------------------
XSoundShader::XSoundShader(ISoundSystem* sys, CStr name):
	m_pOwner(sys),
	m_iFlags(0),
	m_fVolume(1.0f),
	m_fRolloff(1.0f),
	m_fDoppler(1.0f),
	m_fRange(MAX_SOUND_RANGE),
	m_fMinRange(16),
	m_iFrequency(0),
	m_iPriority(0),
	m_iBeginLoop(0),
	m_iEndLoop(0),
	m_iLastSample(0),
	m_iNumSamples(0),
	m_pSamples(NULL)
{
	m_strName = name.cstr();
}

}

