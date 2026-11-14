#include <El/elementpch.hpp>
#include "hiResTime.hpp"
#include <Es/Common/win.h>
#include <Es/Common/fltopts.hpp>

#ifdef _WIN32

#ifndef _XBOX
#include <mmsystem.h>
#endif

class HiResTicks
{
  bool _counterPresent;
  LARGE_INTEGER _frequency;
  float _invFrequency;

  public:
  HiResTicks();
  __int64 GetTicks() const; // get ticks in ms
  __int64 GetFreq() const
  {
    return _frequency.QuadPart;
  }
  float TimeFrom(__int64 from) const
  {
    return (GetTicks()-from)*_invFrequency;
  }
};

static HiResTicks HiResCounter;

HiResTicks::HiResTicks()
{
  _counterPresent=::QueryPerformanceFrequency(&_frequency)!=FALSE;
  if( !_counterPresent )
  {
    _frequency.QuadPart = 1000;
    _invFrequency = 0.001;
    return;
  }
  _invFrequency=1.0f/_frequency.QuadPart;
}

__int64 HiResTicks::GetTicks() const
{
  if( !_counterPresent )
  {
    #ifdef _XBOX
      return GetTickCount();
    #else
      return timeGetTime();
    #endif
  }
  // get ticks in ms
  LARGE_INTEGER count;
  ::QueryPerformanceCounter(&count);
  return count.QuadPart;
}

#endif

#ifndef _WIN32
typedef unsigned long long unsigned64;
extern unsigned64 getSystemTime ();
#endif

SectionTimeHandle StartSectionTime()
{
#ifdef _WIN32
  return HiResCounter.GetTicks();
#else
  return getSystemTime();
#endif
}

__int64 GetSectionResolution()
{
#ifdef _WIN32
  return HiResCounter.GetFreq();
#else
  return 1000000;
#endif
};

float GetSectionTime(SectionTimeHandle section)
{
#ifdef _WIN32
  return HiResCounter.TimeFrom(section);
#else
  return( 1.e-6f * (getSystemTime() - section) );
#endif
}

bool CompareSectionTimeGE(SectionTimeHandle section, float time)
{
#ifdef _WIN32
  return HiResCounter.GetTicks()>=section+to64bInt(time*HiResCounter.GetFreq());
#else
  return( 1.e-6f * (getSystemTime() - section) >= time );
#endif
}
