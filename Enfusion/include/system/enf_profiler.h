//-----------------------------------------------------------------------------
// File: enf_profiler.h
//
// Desc: System profiler
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_PROFILER_H
#define ENF_PROFILER_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_strings.h"

namespace enf
{
/** @addtogroup System
@{*/

enum ProfileFlags
{
	PF_EVENT		= BITF(1),
	PF_USED		= BITF(31)
};

#ifdef ENF_DEVELOPER

class Profiler
{
public:
	const static uint MAX_SLOTS = 256;

	Profiler();
	static void PUBLIC_API BeginFrame();
	static void PUBLIC_API EndFrame();
	static void PUBLIC_API Enable();
	static void PUBLIC_API Disable();
	static void PUBLIC_API Error();
	static bool IsInside()
	{
		return m_iStackPos != 0;
	}

	static bool IsRecent(uint slot, uint64 ctime);
	static void PUBLIC_API SetSlot(uint slot, const char *name, uint flags);
	static void PUBLIC_API AddTicks(uint slot, uint64 ticks);
	static void PUBLIC_API SetSlotName(uint slot, const char* name);

	static uint64 GetFrameTicks(uint slot);
	static uint GetFrameRatio(uint slot, uint scale);
	static uint64 GetLocalFrameTicks(uint slot);
	static uint GetLocalFrameRatio(uint slot, uint scale);

	static uint64 GetTotalFrameTicks();

	static int GetCount(uint slot)
	{
		enf_assert(slot < MAX_SLOTS);
		return Slots[slot].m_iRCount;
	}
	static const char* GetName(uint slot)
	{
		return Slots[slot].m_strName.cstr();
	}


	static void PUBLIC_API SetCounter(uint counter, const char *name);
	static int GetCounterTotalCount(uint counter)
	{
		enf_assert(counter < MAX_COUNTERS);
		return Counters[counter].m_iTotalCount;
	}
	static int GetCounterCount(uint counter)
	{
		enf_assert(counter < MAX_COUNTERS);
		return m_iCounterCounts[counter];
	}

	static const char* GetCounterName(uint counter)
	{
		return Counters[counter].m_strName.cstr();
	}

	ENF_INLINE static void AddCount(uint counter, int value)
	{
		m_iCounterCounts[counter] += value;
	}

	ENF_INLINE static bool IsEnabled()
	{
		return m_bEnabled;
	}

	static void test();

	ENF_INLINE static unsigned int Begin(uint slot)
	{
		if(m_bEnabled)
			return _Begin(slot);

		return 0;
	}
	ENF_INLINE static unsigned int End(int64* rticks = NULL)
	{
		if(m_bEnabled)
			return _End(rticks);

		return 0;
	}

private:

	const static uint MAX_STACK = 256;
	const static uint MAX_COUNTERS = 256;

	//separate it from def struct, to avoid heavy cache usage
	static uint64		m_iSlotFrameTicks[MAX_SLOTS][2];
	static uint			m_iSlotCounts[MAX_SLOTS];
	static int			m_iCounterCounts[MAX_COUNTERS];

	struct Counter
	{
		int			m_iTotalCount;
		SString<32>	m_strName;
	};

	struct Slot
	{
		int64			m_iRFrameTicks[2];
		uint			m_iRCount;
		uint			m_iFlags;
		int64			m_iLastUsed;
		SString<40>	m_strName;

		Slot():m_iRCount(0), m_iLastUsed(0), m_iFlags(0) { m_iRFrameTicks[0] = 0; m_iRFrameTicks[1] = 0;}
	};

	ENF_DECL_THREAD static uint	Stack[MAX_STACK];
	ENF_DECL_THREAD static uint	m_iStackPos;

	static Slot			Slots[MAX_SLOTS];
	static Counter		Counters[MAX_COUNTERS];

	static uint			m_iReportFrequency;
	static uint			m_iFrameCount;
	static PUBLIC_API bool m_bEnabled;
	static bool			m_bInFrame;

	static uint64		m_iBeginTicks;	//tickcount at the begin of frame
	static uint64		m_iFrameTicks;	//frame tickcount accumulator
	static uint64		m_iRFrameTicks;//reported frame tickcount

	static unsigned int PUBLIC_API _Begin(uint slot);
	static unsigned int PUBLIC_API _End(int64* rticks = NULL);
};

struct _ProfilerTag
{
	bool m_bWasEnabled;
	unsigned int m_iLastStack;

	ENF_INLINE _ProfilerTag(uint slot)
	{
		m_bWasEnabled = Profiler::IsEnabled();
		m_iLastStack = Profiler::Begin(slot);
	}

	ENF_INLINE ~_ProfilerTag()
	{
		enf_assert(m_bWasEnabled == Profiler::IsEnabled());
		unsigned int stack = Profiler::End();
		if(stack != m_iLastStack)
		{
			Profiler::Error();
			enf_assert(stack == m_iLastStack);
		}
	}
};

	#define ENF_PROFILER_IS_ENABLED() enf::Profiler::IsEnabled()
	#define ENF_PROFILE(a) enf::_ProfilerTag _profile_tag(a);
	#define ENF_PROFILER_SETSLOT(a, name, flags) enf::Profiler::SetSlot(a, name, flags)
	#define ENF_PROFILER_SETCOUNTER(a, name) enf::Profiler::SetCounter(a, name)
	#define ENF_BEGIN_PROFILE(a) enf::Profiler::Begin(a)
	#define ENF_END_PROFILE() enf::Profiler::End()
	#define ENF_END_PROFILE_RETURN(retValue) enf::Profiler::End(retValue)
	#define ENF_ADD_COUNTER(c, v) enf::Profiler::AddCount(c, v)

	#define ENF_PROFILE_ADD_TICKS(a, ticks) enf::Profiler::AddTicks(a, ticks)
	#define ENF_PROFILE_SET_NAME(a, name) enf::Profiler::SetSlotName(a, name)

#else
	#define ENF_PROFILER_IS_ENABLED() (false)
	#define ENF_PROFILE(a) ((void)0)
	#define ENF_PROFILER_SETSLOT(a, name, flags) ((void)0)
	#define ENF_PROFILER_SETCOUNTER(a, name) ((void)0)
	#define ENF_BEGIN_PROFILE(a) ((void)0)
	#define ENF_END_PROFILE() ((void)0)
	#define ENF_ADD_COUNTER(c, v) ((void)0)
	#define ENF_PROFILE_ADD_TICKS(a, tick) ((void)0)
	#define ENF_PROFILE_SET_NAME(a, name) ((void)0)
#endif


enum {
	PROFILE_UPDATEENTITIES,
		PROFILE_UPDATEENTITY_TICK,
		PROFILE_UPDATEENTITY_BEFORE,
		PROFILE_SLOWENTITY,
		PROFILE_PHYSICS_ADVANCE,
		PROFILE_PHYSICS_MOVE,

		PROFILE_UPDATEENTITY_AFTER,
			PROFILE_ENTITY_UPDATE,
				PROFILE_API_CALL,

	PROFILE_WORLD_RENDER,
		PROFILE_SCENE_TRAVERSE,
		PROFILE_ENTITY_RENDER,
			PROFILE_DECAL_RENDER,
		PROFILE_SKY_RENDER,
		PROFILE_OCEAN_RENDER,
		PROFILE_BUILD_MESH,
		PROFILE_PARTICLE_RENDER,
		PROFILE_TERRAIN_PREPARE,
		PROFILE_TERRAIN_RENDER,
		PROFILE_PIPELINE_RENDER,
			PROFILE_COMMITSTATE,
			PROFILE_FILLCB,
			PROFILE_RENDERSHADOWS,
			PROFILE_RENDERLIGHTS,
			PROFILE_RENDERDEPTHPASS,
			PROFILE_BUILDSHADOWVOLUME,
			PROFILE_RENDERSHADOWVOLUME,
			PROFILE_RENDEROCCLUSIONQUERY,
			PROFILE_ONRENDER_IDLE,

	PROFILE_PART_BATCHES,
	PROFILE_PART_EMITTERS,
	PROFILE_PART_POINTS,
	PROFILE_PART_STREAKS,
	PROFILE_PART_SPRITES,

	PROFILE_SCRIPTEXECUTE,

	PROFILE_PROCESSTHREADS,
  PROFILE_SOUNDMANAGER_UPDATE,
  PROFILE_RESOURCEMANAGER_UPDATE,
	PROFILE_LOCK_VB,
	PROFILE_LOCK_IB,
	PROFILE_PATHFINDER_FINDPATH,
	PROFILE_UPDATEPARTICLES,
	PROFILE_OCEAN_CPUFFT,
	PROFILE_MESHOBJECT_GETSKINNEDMESH,
	PROFILE_MESHOBJECT_MORPH,
	PROFILE_MESHOBJECT_ANIM,
	PROFILE_WORLD_P2PVISIBILITY,
	PROFILE_WORLD_TRACELINE,
	PROFILE_WORLD_TRACEMOVE,
	PROFILE_WORLD_TRACEPOSITION,
	PROFILE_PRESENT,
	PROFILE_NETWORK,
	PROFILE_THREAD_ENQUEUE,
	PROFILE_THREAD_WAITFOR,
	PROFILE_THREAD_WAITFORIDLE,

	PROFILE_GRASS_TRIANGLE,
	PROFILE_GRASS_CLUSTER,
	PROFILE_GRASS_CLUSTERLIT,
	PROFILE_GRASS_MAT_CHANGE,

	PROFILE_TERRAIN_BLOCKS,
	PROFILE_TERRAIN_LIT_BLOCKS,
	PROFILE_TERRAIN_TILES,

	PROFILE_WIND_EMITTERS_ACT,
	PROFILE_WIND_EMITTERS,
	PROFILE_WIND_EMITTERS_CNT,
		
	PROFILE_DECAL_STAT_ACT,
	PROFILE_DECAL_STAT_REN,
	PROFILE_DECAL_DYN_ACT,
	PROFILE_DECAL_DYN_REN,
	PROFILE_DECAL_LAND_ACT,
	PROFILE_DECAL_LAND_REN,

	PROFILE_RAIN_QUADS,
	PROFILE_RAIN_DROPS,

	PROFILE_ACT_LIGHTS,

	#ifdef ENF_PS3
		PROFILE_CB_FRAME_SIZE,
		PROFILE_CB_FRAMES,
	#endif


//for quick tests
	PROFILE_USER_0,
	PROFILE_USER_1,
	PROFILE_USER_2,
	PROFILE_USER_3,
	PROFILE_USER_4,
	PROFILE_USER_5,
	PROFILE_USER_6,
	PROFILE_USER_7,
	PROFILE_USER_8,
	PROFILE_USER_9
};

enum
{
	ENF_COUNTER_USER = 128
};

//@}
}
#endif //ENF_PROFILER_H
