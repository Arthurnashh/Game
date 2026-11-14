//-----------------------------------------------------------------------------
// File: enf_animation.cpp
//
// Desc: Animation data class
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "engine/enf_engine.h"
#include "animations/enf_animationsystem.h"
#include "resource/enf_resourcemanager.h"
#include "animations/enf_animation.h"
#include "animations/enf_animationsource.h"
#include "common/enf_iff.h"
#include "common/enf_simdmathlib.h"

namespace enf
{
	/*
	void AnimTest()
	{
		AnimationSource as;
		AnimChannelSource* ch = as.AddChannel("layer1");
		
		ch->AddKeyframe(QKey(Quaternion(0,0,0,1), Vector3(0,0,0)), 0);
		ch->AddKeyframe(QKey(Quaternion(0,0,0,1), Vector3(10,0,0)), 10);

		Animation anm;
		anm.Build(&as);
		uint index = GetAnimationSystem()->GetBoneExternIndex("layer1");
		const Animation::Channel* chann = anm.GetChannel(index);
		QKey result;
		chann->Keyframe(result, 5, 6, 0.5f);
		result=result;
	}
*/
//-----------------------------------------------------------------------------
// Name: Animation::Animation()
// Desc: 
//-----------------------------------------------------------------------------
void Animation::Init()
{
	ObjectResource::Init();
	m_iNumFrames = 0;
	m_pChannels = NULL;
	m_iNumChannels = 0;
	m_iSize = 0;

	memset(m_iChannelIndex, 0xff, sizeof(m_iChannelIndex));
}

//-----------------------------------------------------------------------------
void Animation::Unload()
{
	ObjectResource::Unload();
	for(uint n = 0; n < m_iNumChannels; n++)
	{
		if(m_pChannels[n].m_TransChannel.m_iNumKeys > 1)
		{
			SAFE_DELETE_ARRAY(m_pChannels[n].m_TransChannel.m_pTime);
			SAFE_DELETE_ARRAY(m_pChannels[n].m_TransChannel.m_pData);
		}

		//if(m_pChannels[n].m_ScaleChannel.m_iNumKeys > 1)
		//{
		//	SAFE_DELETE_ARRAY(m_pChannels[n].m_ScaleChannel.m_pTime);
		//	SAFE_DELETE_ARRAY(m_pChannels[n].m_ScaleChannel.m_pData);
		//}
		if(m_pChannels[n].m_QuatChannel.m_iNumKeys > 1)
		{
			SAFE_DELETE_ARRAY(m_pChannels[n].m_QuatChannel.m_pTime);
			SAFE_DELETE_ARRAY(m_pChannels[n].m_QuatChannel.m_pData);
		}
	}

	SAFE_DELETE_ARRAY(m_pChannels);
	m_iNumChannels = 0;
	IL_EXCHANGE_ADD(MemStats::m_iAnims, -m_iSize);
}

//-----------------------------------------------------------------------------
const Animation::Channel* Animation::GetChannel(uint index) const
{
	enf_assert(!IsLoading());

	if(m_iChannelIndex[index] == 0xff)
		return NULL;

	return &m_pChannels[m_iChannelIndex[index]];
}

const float SCALE_KEY = 1.0f / 65535.0f;
//-----------------------------------------------------------------------------
size_t LoadKeys(IFFInput& iff, Animation::Keys& keys, uint elems, const bool bigEndian)
{
	size_t data = 0;
	if(keys.m_iNumKeys == 0)
	{
		//identity for both translation/rotation
		keys.m_vData[0] = 0.0f;
		keys.m_vData[1] = 0.0f;
		keys.m_vData[2] = 0.0f;
		keys.m_vData[3] = 1.0f;
	}
	if(keys.m_iNumKeys == 1)
	{
		ushort data[5];
		iff.Read(data, (elems + 1) * sizeof(ushort));
		float bias = keys.m_fBias;
		float scale = keys.m_fScale;

		for(uint n = 0; n < elems; n++)
		{
			ushort v = data[n + 1];
#if ENF_INTEGER_BIG_ENDIAN
			if(!bigEndian)
				v = LE_SHORT(v);
#endif
			keys.m_vData[n] = (float)v * scale + bias;
		}
	}
	else if(keys.m_iNumKeys)
	{
		keys.m_pTime = ENF_NEW ushort[keys.m_iNumKeys];
		iff.Read(keys.m_pTime, keys.m_iNumKeys * sizeof(ushort));
		data += (keys.m_iNumKeys * sizeof(ushort));
		keys.m_pData = ENF_NEW ushort[keys.m_iNumKeys * elems];
		iff.Read(keys.m_pData, keys.m_iNumKeys * sizeof(ushort) * elems);
		data += (keys.m_iNumKeys * sizeof(ushort) * elems);

#if ENF_INTEGER_BIG_ENDIAN
		if(!bigEndian)
		{
			for(uint k = 0; k < keys.m_iNumKeys; k++)
			{
				keys.m_pTime[k] = LE_SHORT(keys.m_pTime[k]);
				for(uint e = 0; e < elems; e++)
					keys.m_pData[k * elems + e] = LE_SHORT(keys.m_pData[k * elems + e]);
			}
		}
#endif
	}
	keys.InitTimeLimits();
	return data;
}

//-----------------------------------------------------------------------------
size_t SkipKeys(IFFInput& iff, Animation::Keys& keys, uint elems)
{
	if(keys.m_iNumKeys)
	{
		iff.Skip(keys.m_iNumKeys * sizeof(ushort) * (elems + 1));
		keys.m_iNumKeys = 0;
	}
	keys.InitTimeLimits();
	return 0;
}

struct DHead3
{
	char				Name[32];
	float				PosBias, PosMult;
	float				ScaleBias, ScaleMult;
	float				RotBias, RotMult;
	int				Time;
	unsigned short NumFrames;
	unsigned short NumPosKeys;
	unsigned short NumScaleKeys;
	unsigned short NumRotKeys;
};

struct DHead4
{
	char				Name[32];
	float				PosBias, PosMult;
	float				ScaleBias, ScaleMult;
	float				RotBias, RotMult;
	unsigned short NumFrames;
	unsigned short NumPosKeys;
	unsigned short NumScaleKeys;
	unsigned short NumRotKeys;
};

//-----------------------------------------------------------------------------
bool Animation::Load(CInputStream* instream)
{
	//AnimTest();
	Unload();

IFFInput		iff;
HunkID		id;
int			nbones = 0;
int			len = 0;
AnimationSystem* asys = GetAnimationSystem();

	m_iSize = 0;

	if(!iff.Open(instream, id))
		return false;

	if(id != "ANIM")
		return false;

	enf_assert2(GetAnimationSystem(), "AnimationSystem must be inserted into Engine");

	m_iNumFrames = 0;
	m_pChannels = NULL;
	m_iNumChannels = 0;

	Keys scaleKeys[MAX_CHANNELS];

	memset(m_iChannelIndex, 0xff, sizeof(m_iChannelIndex));
	uint numbones = 0;

	while((len = iff.BeginHunk(id)) != -1)
   {
		if(id == "SET3")
		{
			DHead3 heads[MAX_CHANNELS];
			while(true)
			{
				len = iff.BeginHunk(id);

				if(len < 0)
					break;

				if(id == "HEAD")
				{
					iff.Read(heads, len);
					numbones = len / sizeof(DHead3);

					m_pChannels = ENF_NEW Channel[numbones];
					ZeroMem(m_pChannels, sizeof(Channel) * numbones);

					m_iNumChannels = numbones;
					m_iSize += sizeof(Channel) * numbones;

					for(uint b = 0; b < numbones; b++)
					{
						uint internBoneIndex = GetAnimationSystem()->GetBoneIndex(heads[b].Name);
						uint boneindex = internBoneIndex == INDEX_NOT_FOUND ? INDEX_NOT_FOUND : GetAnimationSystem()->GetBoneExternIndex(internBoneIndex);

						uint time = LE_INT(heads[b].Time);
						uint frames = LE_SHORT(heads[b].NumFrames);
						m_pChannels[b].m_iBoneIndex = internBoneIndex;
						m_pChannels[b].m_TransChannel.m_fBias = LE_FLOAT(heads[b].PosBias);
						m_pChannels[b].m_TransChannel.m_fScale = LE_FLOAT(heads[b].PosMult) * SCALE_KEY;
						m_pChannels[b].m_TransChannel.m_iNumKeys = LE_SHORT(heads[b].NumPosKeys);
//						m_pChannels[b].m_ScaleChannel.m_fBias = LE_FLOAT(heads[b].ScaleBias);
//						m_pChannels[b].m_ScaleChannel.m_fScale = LE_FLOAT(heads[b].ScaleMult) * SCALE_KEY;
//						m_pChannels[b].m_ScaleChannel.m_iNumKeys = LE_SHORT(heads[b].NumScaleKeys);
						scaleKeys[b].m_fBias = LE_FLOAT(heads[b].ScaleBias);
						scaleKeys[b].m_fScale = LE_FLOAT(heads[b].ScaleMult) * SCALE_KEY;
						scaleKeys[b].m_iNumKeys = LE_SHORT(heads[b].NumScaleKeys);
//						enf_assert(m_pChannels[b].m_ScaleChannel.m_iNumKeys==0);
						m_pChannels[b].m_QuatChannel.m_fBias = LE_FLOAT(heads[b].RotBias);
						m_pChannels[b].m_QuatChannel.m_fScale = LE_FLOAT(heads[b].RotMult) * SCALE_KEY;
						m_pChannels[b].m_QuatChannel.m_iNumKeys = LE_SHORT(heads[b].NumRotKeys);


						if(boneindex == INDEX_NOT_FOUND)
						{
							enf::LogF(LL_ERROR, "Animation '%s' has undefined bone '%s'", m_strName.c_str(), heads[b].Name);
							continue;
						}
						m_iChannelIndex[boneindex] = b;
					}
					iff.EndHunk();
					continue;
				}

				if(id == "DATA")
				{
					size_t off = 0;

					for(uint b = 0; b < numbones; b++)
					{
						Channel& ch = m_pChannels[b];

						off += LoadKeys(iff, ch.m_TransChannel, 3, false);
						//off += LoadKeys(iff, ch.m_ScaleChannel, 3, false);
						//SkipKeys(iff, ch.m_ScaleChannel, 3);
						SkipKeys(iff, scaleKeys[b], 3);
						off += LoadKeys(iff, ch.m_QuatChannel, 4, false);
					}

					m_iSize += off;
					iff.EndHunk();
					continue;
				}

				enf_assert(!"Unknown hunk");
				return false;
			}
			IL_EXCHANGE_ADD(MemStats::m_iAnims, GetSize());

			uint maxtime = 0;
			for(uint k = 0; k < numbones; k++)
			{
				if(m_pChannels[k].m_TransChannel.m_iNumKeys > 1)
					maxtime = enf_max(maxtime, (uint)m_pChannels[k].m_TransChannel.m_pTime[m_pChannels[k].m_TransChannel.m_iNumKeys - 1]);
				if(m_pChannels[k].m_QuatChannel.m_iNumKeys > 1)
					maxtime = enf_max(maxtime, (uint)m_pChannels[k].m_QuatChannel.m_pTime[m_pChannels[k].m_QuatChannel.m_iNumKeys - 1]);
			}
			m_iNumFrames = maxtime + 1;
			return true;
		}
		else if(id == "SET4")
		{
			DHead4 heads[MAX_CHANNELS];
			uint numFrames = 0;
			while(true)
			{
				len = iff.BeginHunk(id);

				if(len == -1)
					break;

				if(id == "HEAD")
				{
					iff.Read(heads, len);
					numbones = len / sizeof(DHead4);

					m_pChannels = ENF_NEW Channel[numbones];
					ZeroMem(m_pChannels, sizeof(Channel) * numbones);

					m_iNumChannels = numbones;
					m_iSize += sizeof(Channel) * numbones;

					for(uint b = 0; b < numbones; b++)
					{
						uint internBoneIndex = asys ? asys->GetBoneIndex(heads[b].Name) : b;
						uint boneindex = internBoneIndex == INDEX_NOT_FOUND ? INDEX_NOT_FOUND : (asys ? asys->GetBoneExternIndex(internBoneIndex) : b);

						numFrames = enf_max(numFrames, (uint)heads[b].NumFrames);
						m_pChannels[b].m_iBoneIndex = internBoneIndex;
						m_pChannels[b].m_TransChannel.m_fBias = heads[b].PosBias;
						m_pChannels[b].m_TransChannel.m_fScale = heads[b].PosMult * SCALE_KEY;
						m_pChannels[b].m_TransChannel.m_iNumKeys = heads[b].NumPosKeys;
						//						m_pChannels[b].m_ScaleChannel.m_fBias = heads[b].ScaleBias;
						//						m_pChannels[b].m_ScaleChannel.m_fScale = heads[b].ScaleMult * SCALE_KEY;
						//						m_pChannels[b].m_ScaleChannel.m_iNumKeys = heads[b].NumScaleKeys;
						scaleKeys[b].m_fBias = heads[b].ScaleBias;
						scaleKeys[b].m_fScale = heads[b].ScaleMult * SCALE_KEY;
						scaleKeys[b].m_iNumKeys = heads[b].NumScaleKeys;
						//						enf_assert(m_pChannels[b].m_ScaleChannel.m_iNumKeys==0);
						m_pChannels[b].m_QuatChannel.m_fBias = heads[b].RotBias;
						m_pChannels[b].m_QuatChannel.m_fScale = heads[b].RotMult * SCALE_KEY;
						m_pChannels[b].m_QuatChannel.m_iNumKeys = heads[b].NumRotKeys;

						if(boneindex == INDEX_NOT_FOUND)
						{
							enf::LogF(LL_ERROR, "Animation '%s' has undefined bone '%s'", m_strName.c_str(), heads[b].Name);
							continue;
						}
						m_iChannelIndex[boneindex] = b;
					}
					iff.EndHunk();
					continue;
				}

				if(id == "DATA")
				{
					size_t off = 0;

					for(uint b = 0; b < numbones; b++)
					{
						Channel& ch = m_pChannels[b];

						off += LoadKeys(iff, ch.m_TransChannel, 3, true);
						SkipKeys(iff, scaleKeys[b], 3);
						off += LoadKeys(iff, ch.m_QuatChannel, 4, true);
					}

					m_iSize += off;
					iff.EndHunk();
					continue;
				}

				enf_assert(!"Unknown hunk");
				return false;
			}

			IL_EXCHANGE_ADD(MemStats::m_iAnims, GetSize());
			m_iNumFrames = numFrames;
			return true;
		}
		else if(id == "FILE")
		{
			iff.EndHunk(true);
			continue;
		}
		else
			iff.EndHunk(true);
	}
	return false;
}

//-----------------------------------------------------------------------------
class PUBLIC_API LoadAnimationResource: public LoadVObjectResource
{
protected:
	Animation m_Animation;

public:
	ENF_DECLARE_ITEM(LoadAnimationResource, LoadVObjectResource);
	ENF_DECLARE_SMALLOBJECT(LoadAnimationResource);

	size_t GetDataSize() const
	{
		return m_Animation.GetSize();
	}

	ENF_INLINE Animation* GetAnimation() const
	{
		return static_cast<Animation*>(LoadResource::m_pResource);
	}

	LoadAnimationResource(Animation* obj): LoadVObjectResource(obj)
	{
		obj->AddRef();
	}
	~LoadAnimationResource()
	{
		m_pResource->FinishLoading(this);
		GetAnimation()->Release();
		m_Animation.Unload();
	}
	bool DoLoad()
	{
		CFileInputStream instream(GetAnimation()->GetFileHandle());
		if(!instream.IsOpen())
			return false;

		return m_Animation.Load(&instream);
	}
	void Apply()
	{
		Animation* obj = GetAnimation();

		//Don't do this?
		//static IL_INT32			m_iCurrentFrame;
		//ResourceManager*			m_pManager;
		//mutable IL_INT32			m_iLastFrame;
		//mutable IL_INT32			m_iLevel;
		//volatile LoadResource*	m_pLoadHandle;
		//FileHandle					m_FileHandle;
		//int							m_iPriority;
		//obj->Item::Swap(m_Object);
		//obj->ObjectResource::Swap(m_Object);
		//obj->VObject::Swap(m_Object);

		obj->Animation::Swap(m_Animation);
	}

	CStr GetName() const
	{
		return GetAnimation()->GetName();
	}
};

//-----------------------------------------------------------------------------
void Animation::Swap(Animation& other)
{
	enf_swap(m_pChannels, other.m_pChannels);
	enf_swap(m_iNumChannels, other.m_iNumChannels);
	ubyte channelsIndex[MAX_CHANNELS];
	memcpy(channelsIndex, other.m_iChannelIndex, sizeof(channelsIndex));
	memcpy(other.m_iChannelIndex, m_iChannelIndex, sizeof(channelsIndex));
	memcpy(m_iChannelIndex, channelsIndex, sizeof(channelsIndex));
	enf_swap(m_iSize, other.m_iSize);
	enf_swap(m_iNumFrames, other.m_iNumFrames);
}
//-----------------------------------------------------------------------------
LoadResource* Animation::CreateResourceLoader()
{
	return ENF_NEW LoadAnimationResource(this);
}

//-----------------------------------------------------------------------------
void Animation::Keys::InitTimeLimits()
{
	if(m_iNumKeys > 1)
	{
		m_iMinTime = m_pTime[0];
		m_iMaxTime = m_pTime[m_iNumKeys - 1];
	}
	else
	{
		m_iMinTime = m_iMaxTime = 0;
	}
}

//-----------------------------------------------------------------------------
uint Animation::Keys::FindFrame(uint frame, uint low, uint high) const
{
	//the worst case - binary search
	while(true)
	{
		int pivot = (low + high) >> 1;

		if(frame < m_pTime[pivot])
		{
			enf_assert(pivot >= 0);
			high = pivot - 1;
			if(frame >= m_pTime[high])
				return high;
		}
		else
		{
			enf_assert(((uint)low + 1) < m_iNumKeys);
			if(frame < m_pTime[low + 1])
				return pivot;
			low = pivot;
		}
	}
}

//-----------------------------------------------------------------------------
/*#ifdef ENF_XENON
ENF_INLINE void DecompressVec3(Vector3* ENF_RESTRICT res, const Animation::Keys* keys, uint tkey)
{
	const ushort* ENF_RESTRICT data = &keys->m_pData[tkey * 3];

	SimdVector bs = SimdVector::LoadFloat2(keys->m_vBiasScale);
	SimdVector val = SimdVector::LoadUShort4(data);

	SimdVector xbias = bs.SplatX();
	SimdVector xscale = bs.SplatY();
	val = val.MultiplyAdd(xscale, xbias);

	SimdVector target = SimdVector::LoadFloat4A16(res->v);

	static const SimdVectorI32 mask = {0xffffffff, 0xffffffff, 0xffffffff, 0x00000000};
	target = target.Select(val, mask.v);
	target.StoreFloat4A16(res->v);
}

ENF_INLINE void DecompressLerpVec3(Vector3* ENF_RESTRICT res, const Animation::Keys* keys, uint key0, uint key1, SimdVector lerp)
{
	const ushort* ENF_RESTRICT d0 = &keys->m_pData[key0 * 3];
	const ushort* ENF_RESTRICT d1 = &keys->m_pData[key1 * 3];
	SimdVector bs = SimdVector::LoadFloat2(keys->m_vBiasScale);
	SimdVector xbias = bs.SplatX();
	SimdVector xscale = bs.SplatY();
	SimdVector v0 = SimdVector::LoadUShort4(d0);
	v0 = v0.MultiplyAdd(xscale, xbias);
	SimdVector v1 = SimdVector::LoadUShort4(d1);
	v1 = v1.MultiplyAdd(xscale, xbias);

	SimdVector val = v0.Lerp(v1, lerp);
	SimdVector target = SimdVector::LoadFloat4A16(res->v);

	static const SimdVectorI32 mask = {0xffffffff, 0xffffffff, 0xffffffff, 0x00000000};
	target = target.Select(val, mask.v);
	target.StoreFloat4A16(res->v);
}

ENF_INLINE void DecompressQuat(Quaternion* ENF_RESTRICT res, const Animation::Keys* keys, uint tkey)
{
	const ushort* ENF_RESTRICT data = &keys->m_pData[tkey * 4];
	SimdVector bs = SimdVector::LoadFloat2(keys->m_vBiasScale);
	SimdVector xbias = bs.SplatX();
	SimdVector xscale = bs.SplatY();
	SimdVector v0 = SimdVector::LoadUShort4(data);
	v0 = v0.MultiplyAdd(xscale, xbias);
	v0.StoreFloat4A16(res->v);
}

ENF_INLINE void DecompressLerpQuat(Quaternion* ENF_RESTRICT quat, const Animation::Keys* keys, uint key0, uint key1, SimdVector lerp)
{
	SimdVector bs = SimdVector::LoadFloat2(keys->m_vBiasScale);
	SimdVector xbias = bs.SplatX();
	SimdVector xscale = bs.SplatY();
	SimdVector v0 = SimdVector::LoadUShort4(&keys->m_pData[key0 * 4]);
	v0 = v0.MultiplyAdd(xscale, xbias);
	SimdVector v1 = SimdVector::LoadUShort4(&keys->m_pData[key1 * 4]);
	v1 = v1.MultiplyAdd(xscale, xbias);

	SimdVector qres = XMQuaternionSlerpV(v0, v1, lerp);
	qres.StoreFloat4A16(quat->v);
}

void Animation::Channel::Keyframe(QKey& result, int f1, int f2, const SimdVector& frac, UnpackInfo& info) const
{
	enf_assert(((intptr_t)(&result) & 15) == 0);

	const Animation::Keys& trans = GetTrans();
	const Animation::Keys& quat = GetQuat();

	uint tindex2;
	uint qindex2;

	uint tindex1 = trans.GetFrame(f1, info.TIndex);
	uint qindex1 = quat.GetFrame(f1, info.QIndex);

	if(f1 == f2)
	{
		tindex2 = tindex1;
		qindex2 = qindex1;
	}
	else
	{
		tindex2 = trans.GetFrame(f2, info.TIndex);
		qindex2 = quat.GetFrame(f2, info.QIndex);
	}

	if(tindex1 == tindex2)
	{
		if(tindex1 == INDEX_NOT_FOUND)
		{
			result.m_vTrans.Zero();
		}
		else
		{
			if(trans.m_iNumKeys == 1)
				result.m_vTrans = trans.m_vData;
			else
				DecompressVec3(&result.m_vTrans, &trans, tindex1);
		}
	}
	else
	{
		DecompressLerpVec3(&result.m_vTrans, &trans, tindex1, tindex2, frac);
	}

	if(qindex1 == qindex2)
	{
		if(qindex1 == INDEX_NOT_FOUND)
		{	
			result.m_qQuat.Identity();
		}
		else
		{
			if(quat.m_iNumKeys == 1)
				result.m_qQuat = quat.m_vData;
			else
				DecompressQuat(&result.m_qQuat, &quat, qindex1);
		}
	}
	else
	{
		DecompressLerpQuat(&result.m_qQuat, &quat, qindex1, qindex2, frac);
	}

	info.QIndex = qindex1;
	info.TIndex = tindex1;
}

#else*/

ENF_INLINE void DecompressVec3(Vector3* ENF_RESTRICT res, const Animation::Keys* keys, uint tkey)
{
	const ushort* ENF_RESTRICT data = &keys->m_pData[tkey * 3];

	float scale = keys->m_fScale;
	float bias = keys->m_fBias;

	res->x = (float)data[0] * scale + bias;
	res->y = (float)data[1] * scale + bias;
	res->z = (float)data[2] * scale + bias;
}
//-----------------------------------------------------------------------------
ENF_INLINE void DecompressLerpVec3(Vector3* ENF_RESTRICT res, const Animation::Keys* keys, uint key0, uint key1, float lerp)
{
	const ushort* ENF_RESTRICT d0 = &keys->m_pData[key0 * 3];
	const ushort* ENF_RESTRICT d1 = &keys->m_pData[key1 * 3];
	float ilerp = 1.0f - lerp;
	float scale = keys->m_fScale;
	float bias = keys->m_fBias;
	float iscale = scale * ilerp;
	float ibias = bias * ilerp;
	scale *= lerp;
	bias *= lerp;
	res->x = ((float)d0[0] * iscale + ibias) + ((float)d1[0] * scale + bias);
	res->y = ((float)d0[1] * iscale + ibias) + ((float)d1[1] * scale + bias);
	res->z = ((float)d0[2] * iscale + ibias) + ((float)d1[2] * scale + bias);
}

ENF_INLINE void DecompressQuat(Quaternion* ENF_RESTRICT res, const Animation::Keys* keys, uint tkey)
{
	float scale = keys->m_fScale;
	float bias = keys->m_fBias;
	const ushort* ENF_RESTRICT data = &keys->m_pData[tkey * 4];

	res->x = (float)data[0] * scale + bias;
	res->y = (float)data[1] * scale + bias;
	res->z = (float)data[2] * scale + bias;
	res->w = (float)data[3] * scale + bias;
}

ENF_INLINE void DecompressLerpQuat(Quaternion* ENF_RESTRICT quat, const Animation::Keys* keys, uint key0, uint key1, float lerp)
{
	float scale = keys->m_fScale;
	float bias = keys->m_fBias;

	const ushort* ENF_RESTRICT d0 = &keys->m_pData[key0 * 4];
	const ushort* ENF_RESTRICT d1 = &keys->m_pData[key1 * 4];

	*quat = Quaternion((float)d0[0] * scale + bias, (float)d0[1] * scale + bias, (float)d0[2] * scale + bias, (float)d0[3] * scale + bias).Lerp(
		Quaternion((float)d1[0] * scale + bias, (float)d1[1] * scale + bias, (float)d1[2] * scale + bias, (float)d1[3] * scale + bias), lerp);
}

//#endif


//-----------------------------------------------------------------------------
void Animation::Channel::Keyframe(QKey& result, int f1, int f2, float frac, UnpackInfo& info) const
{
	enf_assert(((intptr_t)(&result) & 15) == 0);
	const Animation::Keys& trans = GetTrans();
	const Animation::Keys& quat = GetQuat();
	uint qindex1 = 0, tindex1 = 0;
	
	if(trans.m_iNumKeys <= 1)
	{
		result.m_vTrans = trans.m_vData;
	}
	else
	{
		tindex1 = trans.GetFrame(f1, info.TIndex);
		if(f1 == f2)
		{
			DecompressVec3(&result.m_vTrans, &trans, tindex1);
		}
		else
		{
			uint index2 = trans.GetFrame(f2, info.TIndex);
			if(trans.m_pTime[index2] < f2)
				index2 = enf_min(index2 + 1, trans.m_iNumKeys - 1);
			if(tindex1 == index2)
			{
				DecompressVec3(&result.m_vTrans, &trans, tindex1);
			}
			else
			{
				float t0 = trans.m_pTime[tindex1];
				float t1 = trans.m_pTime[index2];
				float interp = (frac - t0) / (t1 - t0);

				DecompressLerpVec3(&result.m_vTrans, &trans, tindex1, index2, interp);
			}
		}
	}

	if(quat.m_iNumKeys <= 1)
	{
		result.m_qQuat = quat.m_vData;
	}
	else
	{
		qindex1 = quat.GetFrame(f1, info.QIndex);
		if(f1 == f2)
		{
			DecompressQuat(&result.m_qQuat, &quat, qindex1);
		}
		else
		{
			uint index2 = quat.GetFrame(f2, info.QIndex);
			if(quat.m_pTime[index2] < f2)
				index2 = enf_min(index2 + 1, quat.m_iNumKeys - 1);
			if(qindex1 == index2)
			{
				DecompressQuat(&result.m_qQuat, &quat, qindex1);
			}
			else
			{
				float t0 = quat.m_pTime[qindex1];
				float t1 = quat.m_pTime[index2];
				float interp = (frac - t0) / (t1 - t0);

				DecompressLerpQuat(&result.m_qQuat, &quat, qindex1, index2, interp);
			}
		}
	}
	info.QIndex = qindex1;
	info.TIndex = tindex1;
}

//-----------------------------------------------------------------------------
void Animation::Channel::Keyframe(QKey& result, int f1, int f2, float frac) const
{
	enf_assert(f2 >= f1);
	const Animation::Keys& trans = GetTrans();
	const Animation::Keys& quat = GetQuat();

	if(trans.m_iNumKeys <= 1)
	{
		result.m_vTrans = trans.m_vData;
	}
	else
	{
		uint index1 = trans.GetFrame(f1);
		if(f1 == f2)
		{
			DecompressVec3(&result.m_vTrans, &trans, index1);
		}
		else
		{
			uint index2 = trans.GetFrame(f2);
			if(trans.m_pTime[index2] < f2)
				index2 = enf_min(index2 + 1, trans.m_iNumKeys - 1);
			if(index1 == index2)
			{
				DecompressVec3(&result.m_vTrans, &trans, index1);
			}
			else
			{
				float t0 = trans.m_pTime[index1];
				float t1 = trans.m_pTime[index2];
				float interp = (frac - t0) / (t1 - t0);

				DecompressLerpVec3(&result.m_vTrans, &trans, index1, index2, interp);
			}
		}
	}

	if(quat.m_iNumKeys <= 1)
	{
		result.m_qQuat = quat.m_vData;
	}
	else
	{
		uint index1 = quat.GetFrame(f1);
		if(f1 == f2)
		{
			DecompressQuat(&result.m_qQuat, &quat, index1);
		}
		else
		{
			uint index2 = quat.GetFrame(f2);
			if(quat.m_pTime[index2] < f2)
				index2 = enf_min(index2 + 1, quat.m_iNumKeys - 1);
			if(index1 == index2)
			{
				DecompressQuat(&result.m_qQuat, &quat, index1);
			}
			else
			{
				float t0 = quat.m_pTime[index1];
				float t1 = quat.m_pTime[index2];
				float interp = (frac - t0) / (t1 - t0);

				DecompressLerpQuat(&result.m_qQuat, &quat, index1, index2, interp);
			}
		}
	}
}

//-----------------------------------------------------------------------------
void Animation::Channel::FirstKeyframe(QKey& result) const
{
	const Animation::Keys& trans = GetTrans();
	const Animation::Keys& quat = GetQuat();

	if(trans.m_iNumKeys <= 1)
		result.m_vTrans = trans.m_vData;
	else
		DecompressVec3(&result.m_vTrans, &trans, 0);

	if(quat.m_iNumKeys <= 1)
		result.m_qQuat = quat.m_vData;
	else
		DecompressQuat(&result.m_qQuat, &quat, 0);
}

//-----------------------------------------------------------------------------
void Animation::Channel::LastKeyframe(QKey& result) const
{
	const Animation::Keys& trans = GetTrans();
	const Animation::Keys& quat = GetQuat();

	if(trans.m_iNumKeys <= 1)
		result.m_vTrans = trans.m_vData;
	else
		DecompressVec3(&result.m_vTrans, &trans, trans.m_iNumKeys - 1);

	if(quat.m_iNumKeys <= 1)
		result.m_qQuat = quat.m_vData;
	else
		DecompressQuat(&result.m_qQuat, &quat, quat.m_iNumKeys - 1);
}

//-----------------------------------------------------------------------------
void Compress(ushort* dest, const float* source, int num, float min, float range)
{
	for(int n = 0; n < num; n++)
	{
		float fd = (source[n] - min) * range;
		fd = enf_limit(fd, 0.0f, 65535.0f);

		ushort d = (unsigned short)fd;
		dest[n] = d;
	}
}

struct KeyProps
{
	int	MinTime, MaxTime;
	float TransMin, TransMax;
	float QuatMin, QuatMax;
	float ScaleMin, ScaleMax;
	KeyProps():
	MinTime(0), MaxTime(0),
		TransMin(FLT_MAX), TransMax(-FLT_MAX),
		QuatMin(FLT_MAX), QuatMax(-FLT_MAX),
		ScaleMin(FLT_MAX), ScaleMax(-FLT_MAX)
	{}
};
//---------------------------------------------------------------------------------------------
bool Animation::Build(const AnimationSource* source)
{
	enf_assert2(GetAnimationSystem(), "AnimationSystem must be inserted into Engine");

	if(source->m_Channels.IsEmpty())
		return false;

	ubyte channelIndex[MAX_CHANNELS];
	memset(channelIndex, 0xff, sizeof(channelIndex));

	uint numChannels = 0;
	uint* channelMapping = ALLOCA(uint, source->m_Channels.GetCardinality());
	uint* channelBones = ALLOCA(uint, source->m_Channels.GetCardinality());
	for(uint c = 0; c < source->m_Channels.GetCardinality(); c++)
	{
		if(source->m_Channels[c]->m_Keys.IsEmpty())
			continue;

		uint internBoneIndex = GetAnimationSystem()->GetBoneIndex(source->m_Channels[c]->m_strName.cstr());
		if(internBoneIndex == INDEX_NOT_FOUND)
			continue;
		uint boneIndex = GetAnimationSystem()->GetBoneExternIndex(internBoneIndex);

		channelBones[c] = internBoneIndex;
		channelIndex[boneIndex] = numChannels;
		channelMapping[numChannels++] = c;
	}

	if(numChannels == 0)
		return false;

	int maxFrame = -1;
	uint keys = 0;

	KeyProps* props = ALLOCAINIT(KeyProps, numChannels);

	for(uint c = 0; c < numChannels; c++)
	{
		const AnimChannelSource* ch = source->m_Channels[channelMapping[c]];

		keys += ch->m_Keys.GetCardinality();
		maxFrame = enf_max(maxFrame, (int)ch->m_Indices.GetLast());

		for(uint k = 0; k < ch->m_Keys.GetCardinality(); k++)
		{
			props[c].TransMin = enf_min(props[c].TransMin, ch->m_Keys[k].m_vTrans.x);
			props[c].TransMin = enf_min(props[c].TransMin, ch->m_Keys[k].m_vTrans.y);
			props[c].TransMin = enf_min(props[c].TransMin, ch->m_Keys[k].m_vTrans.z);
			props[c].TransMax = enf_max(props[c].TransMax, ch->m_Keys[k].m_vTrans.x);
			props[c].TransMax = enf_max(props[c].TransMax, ch->m_Keys[k].m_vTrans.y);
			props[c].TransMax = enf_max(props[c].TransMax, ch->m_Keys[k].m_vTrans.z);

			props[c].QuatMin = enf_min(props[c].QuatMin, ch->m_Keys[k].m_qQuat.x);
			props[c].QuatMin = enf_min(props[c].QuatMin, ch->m_Keys[k].m_qQuat.y);
			props[c].QuatMin = enf_min(props[c].QuatMin, ch->m_Keys[k].m_qQuat.z);
			props[c].QuatMin = enf_min(props[c].QuatMin, ch->m_Keys[k].m_qQuat.w);
			props[c].QuatMax = enf_max(props[c].QuatMax, ch->m_Keys[k].m_qQuat.x);
			props[c].QuatMax = enf_max(props[c].QuatMax, ch->m_Keys[k].m_qQuat.y);
			props[c].QuatMax = enf_max(props[c].QuatMax, ch->m_Keys[k].m_qQuat.z);
			props[c].QuatMax = enf_max(props[c].QuatMax, ch->m_Keys[k].m_qQuat.w);
		}
		props[c].MinTime = ch->m_Indices[0];
		props[c].MaxTime = ch->m_Indices.GetLast();
	}

	//check ranges
	for(uint c = 0; c < numChannels; c++)
	{
		if(props[c].TransMin > props[c].TransMax)
			return false;
		if(props[c].QuatMin > props[c].QuatMax)
			return false;
	}
	
	memset(m_iChannelIndex, 0xff, sizeof(m_iChannelIndex));

	m_iSize = 0;
	m_iNumFrames = maxFrame;
	m_pChannels = ENF_NEW Channel[numChannels];
	ZeroMem(m_pChannels, sizeof(Channel) * numChannels);

	m_iNumChannels = numChannels;

	//initialize channel data
	for(uint c = 0; c < numChannels; c++)
	{
		Channel* dchannel = &m_pChannels[c];
		const AnimChannelSource* schannel = source->m_Channels[channelMapping[c]];
		uint channelKeys = schannel->m_Keys.GetCardinality();

		dchannel[c].m_iBoneIndex = channelBones[c];

		{
			dchannel->m_TransChannel.m_iNumKeys = channelKeys;
			if(channelKeys == 1)
			{
				dchannel->m_TransChannel.m_iMinTime = 0;
				dchannel->m_TransChannel.m_iMaxTime = 0;
				dchannel->m_TransChannel.m_vData[0] = schannel->m_Keys[0].m_vTrans.x;
				dchannel->m_TransChannel.m_vData[1] = schannel->m_Keys[0].m_vTrans.y;
				dchannel->m_TransChannel.m_vData[2] = schannel->m_Keys[0].m_vTrans.z;
			}
			else
			{
				float low = props[c].TransMin;
				float range = props[c].TransMax - props[c].TransMin;
				dchannel->m_TransChannel.m_fBias = low;
				dchannel->m_TransChannel.m_fScale = range * SCALE_KEY;
				dchannel->m_TransChannel.m_pData = ENF_NEW ushort[channelKeys * 3];
				dchannel->m_TransChannel.m_iMinTime = props[c].MinTime;
				dchannel->m_TransChannel.m_iMaxTime = props[c].MaxTime;
				for(uint k = 0; k < channelKeys; k++)
				{
					Compress(&dchannel->m_TransChannel.m_pData[k * 3], schannel->m_Keys[k].m_vTrans.v, 3, low, 65535.0 / range);
				}
				dchannel->m_TransChannel.m_pTime = ENF_NEW ushort[channelKeys];
				memcpy(dchannel->m_TransChannel.m_pTime, schannel->m_Indices.GetArray(), sizeof(ushort) * channelKeys);
			}
			m_iSize += channelKeys * 4 * sizeof(ushort);
		}
		{
			dchannel->m_QuatChannel.m_iNumKeys = channelKeys;
			if(channelKeys == 1)
			{
				dchannel->m_QuatChannel.m_iMinTime = 0;
				dchannel->m_QuatChannel.m_iMaxTime = 0;
				dchannel->m_QuatChannel.m_vData[0] = schannel->m_Keys[0].m_qQuat.x;
				dchannel->m_QuatChannel.m_vData[1] = schannel->m_Keys[0].m_qQuat.y;
				dchannel->m_QuatChannel.m_vData[2] = schannel->m_Keys[0].m_qQuat.z;
				dchannel->m_QuatChannel.m_vData[3] = schannel->m_Keys[0].m_qQuat.w;
			}
			else
			{
				float low = props[c].QuatMin;
				float range = props[c].QuatMax - props[c].QuatMin;
				dchannel->m_QuatChannel.m_fBias = low;
				dchannel->m_QuatChannel.m_fScale = range * SCALE_KEY;
				dchannel->m_QuatChannel.m_pData = ENF_NEW ushort[channelKeys * 4];
				dchannel->m_QuatChannel.m_iMinTime = props[c].MinTime;
				dchannel->m_QuatChannel.m_iMaxTime = props[c].MaxTime;

				for(uint k = 0; k < channelKeys; k++)
				{
					Compress(&dchannel->m_QuatChannel.m_pData[k * 4], schannel->m_Keys[k].m_qQuat.v, 4, low, 65535.0 / range);
				}
				dchannel->m_QuatChannel.m_pTime = ENF_NEW ushort[channelKeys];
				memcpy(dchannel->m_QuatChannel.m_pTime, schannel->m_Indices.GetArray(), sizeof(ushort) * channelKeys);
			}
			m_iSize += channelKeys * 5 * sizeof(ushort);
		}
	}
	memcpy(m_iChannelIndex, channelIndex, sizeof(m_iChannelIndex));
	return true;
}

//-----------------------------------------------------------------------------
AnimChannelSource* AnimationSource::AddChannel(CStr name)
{
	for(uint n = 0; n < m_Channels.GetCardinality(); n++)
	{
		if(m_Channels[n]->m_strName == name)
			return NULL;
	}
	AnimChannelSource* channel = ENF_NEW AnimChannelSource(name);
	m_Channels.Insert(channel);
	return channel;
}

//-----------------------------------------------------------------------------
AnimationSource::~AnimationSource()
{
	for(uint c = 0; c < m_Channels.GetCardinality(); c++)
	{
		delete m_Channels[c];
	}
}


}