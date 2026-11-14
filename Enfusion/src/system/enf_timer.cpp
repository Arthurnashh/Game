//-----------------------------------------------------------------------------
// File: enf_timer.cpp
//
// Desc: High resolution timer
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "system/enf_timer.h"
#include "system/enf_time.h"
//#include "enf_main.h"

#ifdef ENF_XENON
	#include <xtl.h>
#elif defined(WIN32)
	#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
	#define NOMINMAX
#endif
	#include <windows.h>
#elif defined(ENF_PS3)
	#include <sys/sys_time.h>
	#include <sys/time_util.h>
#elif defined(__APPLE__)
	#include <sys/time.h>
#endif

namespace enf
{
TimeStamp Timer::m_iFrequency = 0;
TimeStamp Timer::m_iStartTime = 0;
ENF_DECL_THREAD TimeStamp Timer::m_iStack[];
ENF_DECL_THREAD int Timer::m_iStackPos = 0;

#if defined(WIN32) || defined(ENF_XENON)
//-----------------------------------------------------------------------------
void Timer::Init()
{
	//already initialized?
	if(m_iFrequency != 0)
		return;

	LONGLONG freq;

	if (QueryPerformanceFrequency((LARGE_INTEGER *) &freq))
		m_iFrequency = freq;

	QueryPerformanceCounter((LARGE_INTEGER *)&m_iStartTime);
}

//-----------------------------------------------------------------------------
double Timer::GetTime(int resolution)
{
	TimeStamp ctime;
	QueryPerformanceCounter((LARGE_INTEGER *) &ctime);
	ctime -= m_iStartTime;
	return Tick2Time(ctime, resolution);
}

//-----------------------------------------------------------------------------
TimeStamp Timer::GetTicks()
{
	TimeStamp ctime;
	QueryPerformanceCounter((LARGE_INTEGER *) &ctime);
	ctime -= m_iStartTime;
	return ctime;
}

#elif defined(ENF_PS3)
//-----------------------------------------------------------------------------
void Timer::Init()
{
	uint64_t newTime;
	SYS_TIMEBASE_GET( newTime );
	m_iStartTime = newTime;
	m_iFrequency = sys_time_get_timebase_frequency();
}

double Timer::GetTime(int resolution)
{
	uint64_t newTime;
	SYS_TIMEBASE_GET( newTime );
	return Tick2Time(newTime - m_iStartTime, resolution);
}

TimeStamp Timer::GetTicks()
{
	uint64_t newTime;
	SYS_TIMEBASE_GET( newTime );

	return newTime - m_iStartTime;
}

#elif defined(__APPLE__)

//-----------------------------------------------------------------------------
ENF_INLINE TimeStamp GetNow()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	TimeStamp now = (TimeStamp)tv.tv_sec * TimeStamp(1000000) + (TimeStamp)tv.tv_usec;
	return now;
}
void Timer::Init()
{
	m_iStartTime = GetNow();
	m_iFrequency = 1000000;
}

double Timer::GetTime(int resolution)
{
	return Tick2Time(GetNow() - m_iStartTime, resolution);
}

TimeStamp Timer::GetTicks()
{
	return GetNow() - m_iStartTime;
}

#endif

//-----------------------------------------------------------------------------
void Timer::PushTime()
{
	enf_assert(m_iStackPos < MAX_STACK);
	if (m_iStackPos >= MAX_STACK)
		return;

	m_iStack[m_iStackPos++] = GetTicks();
}

//-----------------------------------------------------------------------------
TimeStamp Timer::PopTick(bool subFromParent)
{
	enf_assert(m_iStackPos > 0);
	if (m_iStackPos == 0)
		return 0.0;

	TimeStamp acttime = GetTicks();
	TimeStamp diff = Sub(acttime, m_iStack[--m_iStackPos]);
	if(subFromParent && m_iStackPos > 0)
		m_iStack[m_iStackPos - 1] += diff;

	return diff;
}

//-----------------------------------------------------------------------------
double Timer::PopTime(int resolution, bool subFromParent)
{
	enf_assert(m_iStackPos > 0);
	if (m_iStackPos == 0)
		return 0.0;

	TimeStamp acttime = GetTicks();
	TimeStamp diff = Sub(acttime, m_iStack[--m_iStackPos]);
	if(subFromParent && m_iStackPos > 0)
		m_iStack[m_iStackPos - 1] += diff;

	return Tick2Time(diff, resolution);
}

//-----------------------------------------------------------------------------
double Timer::PopAndPushTime(int resolution)
{
	return Tick2Time(PopAndPushTick(), resolution);
}

//-----------------------------------------------------------------------------
TimeStamp Timer::PopAndPushTick()
{
	enf_assert(m_iStackPos > 0);
	TimeStamp acttime = GetTicks();
	TimeStamp diff = Sub(acttime, m_iStack[m_iStackPos - 1]);
	m_iStack[m_iStackPos] = acttime;
	return diff;
}

#if defined(ENF_DEVELOPER) && defined(ENF_CONSOLE)
	#define ENF_MEASURE_TIME
#endif

//-----------------------------------------------------------------------------
void BeginTimeMeasure()
{
#ifdef ENF_MEASURE_TIME
	enf::Timer::PushTime();
#endif
}
//-----------------------------------------------------------------------------
void EndTimeMeasure(const char* title)
{
#ifdef ENF_MEASURE_TIME
	uint depth = Timer::GetDepth();
	const char prefix[] = "**********";

	float elapsed = Timer::PopTime(enf::Timer::RESOLUTION_MILLISECONDS);
	LogF(enf::LL_PROFILE, "%sInit: %s: %f ms", &prefix[10 - depth], title, elapsed);
#endif
}

//-----------------------------------------------------------------------
const SDateTime& SDateTime::SetCurrent()
{
#ifdef WIN32
  SYSTEMTIME tim;

  ::GetLocalTime(&tim);

  Year = tim.wYear;
  Month = tim.wMonth;
  DayOfWeek = tim.wDayOfWeek;
  Day = tim.wDay;
  Hour = tim.wHour;
  Minute = tim.wMinute;
  Second = tim.wSecond;
  Milliseconds = tim.wMilliseconds;
#endif
  return *this;
}

//-----------------------------------------------------------------------
const SDateTime& SDateTime::SetCurrentUTC()
{
#ifdef WIN32
  SYSTEMTIME tim;

  ::GetSystemTime(&tim);

  Year = tim.wYear;
  Month = tim.wMonth;
  DayOfWeek = tim.wDayOfWeek;
  Day = tim.wDay;
  Hour = tim.wHour;
  Minute = tim.wMinute;
  Second = tim.wSecond;
  Milliseconds = tim.wMilliseconds;
#endif
  return *this;
}

}
