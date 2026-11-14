//-----------------------------------------------------------------------------
// File: enf_timer.h
//
// Desc: High resolution timer
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_TIMER_H
#define ENF_TIMER_H

#include "enf_config.h"
#include "common/enf_inttypes.h"

namespace enf
{
/** @addtogroup System
@{*/

/*!
Precise timer, utilizes CPU performance counters
*/
class Timer
{
public:
	static const int RESOLUTION_MILLISECONDS = 1000;

	//!Must be called prior to any use!
	static void PUBLIC_API Init();

	/*!
	Returns time elapsed since creation
	in native frequency
	*/
	static TimeStamp PUBLIC_API GetTicks();

	/*!
	Converts tick value to time value
	in n-th of seconds
	\param ticks Number of ticks to be converted
	\param resolution n-th of second. I.e. resolution=1000 means milliseconds
	*/
	ENF_INLINE static double Tick2Time(TimeStamp ticks, int resolution)
	{
		return ((double)ticks / (double)m_iFrequency) * resolution;
	}

	ENF_INLINE static TimeStamp Time2Tick(double time, int resolution)
	{
		return (TimeStamp)((time / (double)resolution) * (double)m_iFrequency);
	}

	ENF_INLINE static TimeStamp Sub(TimeStamp t1, TimeStamp t2)
	{
		if(t1 < t2)
			return t1 + (ULONG64(0xffffffffffffffff) - t2);
		else
			return t1 - t2;
	}

	/*!
	Returns time elapsed since creation
	in n-th of seconds
	\param resolution
	n-th of second. I.e. resolution=1000 means milliseconds
	*/
	static double PUBLIC_API GetTime(int resolution = RESOLUTION_MILLISECONDS);

	//!push time into stack
	static void PUBLIC_API PushTime();

	static TimeStamp PUBLIC_API PopTick(bool subFromParent = false);

	//!return time from now till last push
	static double PUBLIC_API PopTime(int resolution = RESOLUTION_MILLISECONDS, bool subFromParent = false);

	//!returns time from now till last push and push again
	static double PUBLIC_API PopAndPushTime(int resolution = RESOLUTION_MILLISECONDS);

	//!returns ticks from now till last push and push again
	static TimeStamp PUBLIC_API PopAndPushTick();

	ENF_INLINE static TimeStamp GetTicksPerSecond()
	{
		return m_iFrequency;
	}
	ENF_INLINE static uint GetDepth()
	{
		return m_iStackPos;
	}

private:
	static const int MAX_STACK = 32;

	static TimeStamp	PUBLIC_API m_iFrequency; ///<Number of ticks per second
	static TimeStamp	m_iStartTime; ///<Time when Init was called.
	ENF_DECL_THREAD static int m_iStackPos;						///< position in stack
	ENF_DECL_THREAD static TimeStamp m_iStack [MAX_STACK];		///< stack for data saving
};

void PUBLIC_API EndTimeMeasure(const char* title);
void PUBLIC_API BeginTimeMeasure();

class PUBLIC_API TimeMeasureScope
{
protected:
	const char* Title;

public:
	TimeMeasureScope(const char* title): Title(title)
	{
		BeginTimeMeasure();
	}
	~TimeMeasureScope()
	{
		EndTimeMeasure(Title);
	}
};

//@}
}

#endif //ENF_TIMER_H