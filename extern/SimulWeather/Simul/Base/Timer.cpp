#include "precomp.h"

#include "Simul/Base/Timer.h"
#include <math.h>
#ifdef _MSC_VER
	#include <time.h>
	#include <windows.h>
	#include <mmsystem.h>
#else
	#define LARGE_INTEGER long
#endif
using namespace simul::base;
Timer::Timer()
{          
#ifdef _MSC_VER
	LARGE_INTEGER &PerfFreq=*(reinterpret_cast<LARGE_INTEGER*>(&iPerfFreq));
	if(!QueryPerformanceFrequency(&PerfFreq))
		return;
	// We can use hires timer, determine overhead
	OverheadTicks=600000;//200;
	for(int i=0;i<200;i++)
	{
		LARGE_INTEGER b,e;
		int Ticks;
		QueryPerformanceCounter(&b);
		EmptyFunction();
		QueryPerformanceCounter(&e);
		Ticks = e.LowPart - b.LowPart;
		if ( Ticks >= 0 && Ticks < OverheadTicks )
			OverheadTicks = Ticks;
	}
	// See if Freq fits in 32 bits; if not lose some precision
	PerfFreqAdjust=0;
	int High32=PerfFreq.HighPart;
	while(High32)
	{
		High32>>=1;
		PerfFreqAdjust++;
	}
	//Freq = PerfFreq;
	Oht = OverheadTicks;
	ReduceMag = 0;
	//SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
	//QueryPerformanceCounter(&tStart);

#endif
#ifdef PLAYSTATION_2
	Control  = SCE_PC0_CPU_CYCLE|SCE_PC_U0;
	Control |= (SCE_PC1_DCACHE_MISS | SCE_PC_U1);
	Control |= SCE_PC_CTE;
#endif
}

Timer::~Timer()
{          
}

void Timer::StartTime()
{
#ifdef _MSC_VER
//	Freq=PerfFreq;
	Oht=OverheadTicks;
	ReduceMag=0;
	LARGE_INTEGER &tStart=*(reinterpret_cast<LARGE_INTEGER*>(&iStart));
	QueryPerformanceCounter(&tStart);

	//unsigned int cycles_lo, cycles_hi;
/*	__asm
	{
		mov esi,[ebp+8]
		cpuid
		rdtsc
		mov [esi], eax;
		mov [esi+4],edx;
   }        */
#endif
#ifdef PLAYSTATION_2
	LastCounter=scePcGetCounter0();
	LastCacheMissCounter=scePcGetCounter1();
#endif
}
#ifdef PLAYSTATION_2
void AccurateTimer::PS2FrameStart()
{
	scePcStart(Control,0,0);
}
void AccurateTimer::PS2FrameFinish()
{
	scePcStop();
}
#endif
void Timer::FinishTime()
{
#ifdef _MSC_VER
	LARGE_INTEGER &tStop=*(reinterpret_cast<LARGE_INTEGER*>(&iStop));
	LARGE_INTEGER &tStart=*(reinterpret_cast<LARGE_INTEGER*>(&iStart));
	QueryPerformanceCounter(&tStop);
	unsigned int High32=tStop.HighPart-tStart.HighPart;
	while(High32)
	{
		High32>>=1;
		ReduceMag++;
	}
	if(PerfFreqAdjust||ReduceMag)
	{
		if(PerfFreqAdjust>ReduceMag)
			ReduceMag=PerfFreqAdjust;
		Oht>>=ReduceMag;
	}
    // Reduced numbers to 32 bits, now can do the maths
	LARGE_INTEGER &PerfFreq=*(reinterpret_cast<LARGE_INTEGER*>(&iPerfFreq));
	if(PerfFreq.LowPart==0)
		 Time=0.0;
	else
		 Time=1000*((float)(tStop.LowPart-tStart.LowPart-Oht))/PerfFreq.LowPart;
	TimeSum+=Time;
#endif
#ifdef PLAYSTATION_2
	Counter=scePcGetCounter0();
	if(Counter>0xffffff)
		scePcStop();
	Counter-=LastCounter;
	CacheMissCounter=scePcGetCounter1();
	CacheMissCounter-=LastCacheMissCounter;
	Time=(float)Counter/(float)(300000.f);
	CacheMissTime=(float)CacheMissCounter/(float)(300000.f);
	CacheMissTimeSum+=CacheMissTime;
	TimeSum+=Time;
#endif
}

