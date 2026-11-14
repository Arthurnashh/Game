//-----------------------------------------------------------------------------
// File: enf_profiler.cpp
//
// Desc: System profiler
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "system/enf_profiler.h"
#include "system/enf_timer.h"
#include "system/enf_time.h"
#include "common/enf_diag.h"

//#define CHECKPROFILER
namespace enf
{
#ifdef ENF_DEVELOPER
//-----------------------------------------------------------------------
uint		Profiler::m_iFrameCount = 0;
uint		Profiler::m_iReportFrequency = 1;
bool		Profiler::m_bEnabled = false;
bool		Profiler::m_bInFrame = false;
uint64	Profiler::m_iBeginTicks = 0;
uint64	Profiler::m_iFrameTicks = 0;
uint64	Profiler::m_iRFrameTicks = 0;

Profiler::Slot	Profiler::Slots[];
ENF_DECL_THREAD uint	Profiler::Stack[];
ENF_DECL_THREAD uint	Profiler::m_iStackPos = 0;

uint64				Profiler::m_iSlotFrameTicks[][2];
uint					Profiler::m_iSlotCounts[];
int					Profiler::m_iCounterCounts[];
Profiler::Counter	Profiler::Counters[];

//-----------------------------------------------------------------------
void Profiler::Error()
{
	_OutputDebugString("Profiler stack damaged\n");
	for(uint n = 0; n < m_iStackPos; n++)
	{
		char buff[256];
		memset(buff, ' ', n);
		buff[n] = '>';
		strcpy(buff + 1 + n, Slots[Stack[n]].m_strName.cstr());
		_OutputDebugString(buff);
		_OutputDebugString("\n");
	}
	enf_assert(!"deadend");
}

//-----------------------------------------------------------------------
void Profiler::Enable()
{
	if(!m_bEnabled)
	{
		m_bEnabled = true;
		m_bInFrame = false;
	}
}

//-----------------------------------------------------------------------
void Profiler::Disable()
{
	enf_assert(m_iStackPos == 0);

	if(m_bEnabled)
	{
		m_bEnabled = false;
		m_bInFrame = false;
	}
}
//-----------------------------------------------------------------------
uint64 Profiler::GetTotalFrameTicks()
{
	return m_iRFrameTicks;
}

//-----------------------------------------------------------------------
uint64 Profiler::GetFrameTicks(uint slot)
{
	return Slots[slot].m_iRFrameTicks[0];
}

//-----------------------------------------------------------------------
uint64 Profiler::GetLocalFrameTicks(uint slot)
{
	return Slots[slot].m_iRFrameTicks[1];
}

//-----------------------------------------------------------------------
uint Profiler::GetFrameRatio(uint slot, uint scale)
{
	if(m_iRFrameTicks == 0)
		return 0;

	uint64 t = Slots[slot].m_iRFrameTicks[0] * (uint64)scale;
	t = t / m_iRFrameTicks;
	return (uint)t;
}

//-----------------------------------------------------------------------
uint Profiler::GetLocalFrameRatio(uint slot, uint scale)
{
	if(m_iRFrameTicks == 0)
		return 0;

	uint64 t = Slots[slot].m_iRFrameTicks[1] * (uint64)scale;
	t = t / m_iRFrameTicks;
	return (uint)t;
}

DWORD lastthread = NULL;
//-----------------------------------------------------------------------
unsigned int Profiler::_Begin(uint slot)
{
	//DWORD th = ::GetCurrentThreadId();
	//if(lastthread == NULL) lastthread = th;
	//enf_assert(th == lastthread);

	enf_assert(m_iStackPos < MAX_STACK);
#ifdef CHECKPROFILER
	char buff[256];
	memset(buff, ' ', m_iStackPos);
	buff[m_iStackPos] = '>';
	strcpy(buff + 1 + m_iStackPos, Slots[slot].m_strName.cstr());
	_OutputDebugString(buff);
	_OutputDebugString("\n");
#endif

	Stack[m_iStackPos] = slot;
	if(Slots[slot].m_iFlags & PF_EVENT)
	{
		ENF_BEGIN_EVENT_NC(EC_BLACK, Slots[slot].m_strName.cstr());
	}
	m_iSlotCounts[slot]++;
	m_iStackPos++;

	Timer::PushTime();
	return m_iStackPos;
}

//-----------------------------------------------------------------------
unsigned int Profiler::_End(int64* rticks)
{
	//DWORD th = ::GetCurrentThreadId();
	//enf_assert(th == lastthread);

	enf_assert(m_iStackPos);

	uint prevStackPos = m_iStackPos;
	uint stackpos = prevStackPos - 1;

	uint slot = Stack[stackpos];

#ifdef CHECKPROFILER
	char buff[256];
	memset(buff, ' ', stackpos);
	buff[stackpos] = '<';
	strcpy(buff + 1 + stackpos, Slots[slot].m_strName.cstr());
	_OutputDebugString(buff);
	_OutputDebugString("\n");
#endif

	uint64 ticks = Timer::PopTick();
	if(stackpos > 0)
	{
		m_iSlotFrameTicks[Stack[stackpos - 1]][1] -= ticks;
	}
	m_iSlotFrameTicks[slot][0] += ticks;
	m_iSlotFrameTicks[slot][1] += ticks;
	if(Slots[slot].m_iFlags & PF_EVENT)
	{
		ENF_END_EVENT();
	}
	if(rticks)
		*rticks = ticks;

	m_iStackPos = stackpos;
	return prevStackPos;
}

//-----------------------------------------------------------------------
void Profiler::SetSlot(uint slot, const char *name, uint flags)
{
	enf_assert(slot < MAX_SLOTS);
	Slots[slot].m_strName = name;
	Slots[slot].m_iFlags = (flags & 0x00ffffff) | PF_USED;
	m_iSlotFrameTicks[slot][0] = 0;
	m_iSlotFrameTicks[slot][1] = 0;
	m_iSlotCounts[slot] = 0;
}

//-----------------------------------------------------------------------
void Profiler::SetSlotName(uint slot, const char *name)
{
	enf_assert(slot < MAX_SLOTS);
	Slots[slot].m_strName = name;
}

//-----------------------------------------------------------------------
void Profiler::AddTicks(uint slot, uint64 ticks)
{
	enf_assert(slot < MAX_SLOTS);
	m_iSlotFrameTicks[slot][0] += ticks;
	m_iSlotFrameTicks[slot][1] += ticks;
	m_iSlotCounts[slot]++;
}

//-----------------------------------------------------------------------
void Profiler::SetCounter(uint counter, const char *name)
{
	enf_assert(counter < MAX_COUNTERS);
	Counters[counter].m_iTotalCount = 0;
	Counters[counter].m_strName = name;
	m_iCounterCounts[counter] = 0;
}

//-----------------------------------------------------------------------
bool Profiler::IsRecent(uint slot, uint64 ctime)
{
	return (Timer::Tick2Time(ctime - Slots[slot].m_iLastUsed, Timer::RESOLUTION_MILLISECONDS) < 1500);
}

//-----------------------------------------------------------------------
void Profiler::BeginFrame()
{
	for(uint n = 0; n < MAX_COUNTERS; n++)
	{
		Counters[n].m_iTotalCount += m_iCounterCounts[n];
		m_iCounterCounts[n] = 0;
	}

	if(!m_bEnabled)
		return;

	if(m_bInFrame)
	{
		EndFrame();
	}

	enf_assert(!m_bInFrame);
	m_bInFrame = true;

	TimeStamp cticks = Timer::GetTicks();

	if(++m_iFrameCount == m_iReportFrequency)
	{
		m_iRFrameTicks = m_iFrameTicks / m_iReportFrequency;

		m_iFrameTicks = 0;
		m_iFrameCount = 0;

		for(uint n = 0; n < MAX_SLOTS; n++)
		{
			if(!(Slots[n].m_iFlags & PF_USED))
				continue;

			Slots[n].m_iRFrameTicks[0] = m_iSlotFrameTicks[n][0] / m_iReportFrequency;
			Slots[n].m_iRFrameTicks[1] = m_iSlotFrameTicks[n][1] / m_iReportFrequency;

			if(m_iSlotCounts[n])
				Slots[n].m_iLastUsed = cticks;

			//count at least one if used
			if(m_iSlotCounts[n])
				Slots[n].m_iRCount = (m_iSlotCounts[n] < m_iReportFrequency) ? 1 : (m_iSlotCounts[n] / m_iReportFrequency);
			else
				Slots[n].m_iRCount = 0;

			m_iSlotCounts[n] = 0;
			m_iSlotFrameTicks[n][0] = 0;
			m_iSlotFrameTicks[n][1] = 0;
		}
	}

	m_iBeginTicks = cticks;
	m_iStackPos = 0;
}

//TLog slowframe("slowframes.log", LOGF_APPEND);

//-----------------------------------------------------------------------
void Profiler::EndFrame()
{
	if(!m_bEnabled || !m_bInFrame)
		return;

	m_bInFrame = false;
	enf_assert(m_iStackPos == 0);
	TimeStamp frameticks = Timer::Sub(Timer::GetTicks(), m_iBeginTicks);
	m_iFrameTicks += frameticks;

	const int val = ENF_DIAG_VALUE( DM_STATS_SLOWFRAMES );
	if( val > 0 )
	{
		static const float thresholds[] = {0, 20, 40, 60, 80, 100, 150};
		float treshold = thresholds[val];

		if(Timer::Tick2Time(frameticks, Timer::RESOLUTION_MILLISECONDS) > treshold)
		{
			TimeStamp ctime = Timer::GetTicks();

			SDateTime tim;

			tim.SetCurrent();

			LogF(LL_NORMAL, "\nSLOW FRAME, time %02d:%02d:%02d:%04d, took %lf milliseconds\n",
				(int)tim.Hour,
				(int)tim.Minute,
				(int)tim.Second,
				(int)tim.Milliseconds,
				Timer::Tick2Time(frameticks, Timer::RESOLUTION_MILLISECONDS));

			char tmp[256];

			for(int n = 0; n < 256; n++)
			{
				const char* name = Profiler::GetName(n);

				if(name[0] == 0 || !Profiler::IsRecent(n, ctime))
					continue;

				FillMem(tmp, 255, ' ');
				tmp[255] = 0;
				CopyMem(tmp, name, strlen(name));

				float ftime = (float)Timer::Tick2Time(Profiler::GetFrameTicks(n), Timer::RESOLUTION_MILLISECONDS * 100) * 0.01f;
				float lftime = (float)Timer::Tick2Time(Profiler::GetLocalFrameTicks(n), Timer::RESOLUTION_MILLISECONDS * 100) * 0.01f;

				sprintf(tmp + 20, "| %4d | %6.2fms (%02.2f%%) | %6.2fms (%02.2f%%)\n", Profiler::GetCount(n),
					ftime, (float)Profiler::GetFrameRatio(n, 10000) / 100.0f,
					lftime, (float)Profiler::GetLocalFrameRatio(n, 10000) / 100.0f
					);

				LogF(LL_NORMAL, tmp);
			}
			LogF(LL_NORMAL, "SLOW FRAME END\n");
		}
	}
}
#endif
}