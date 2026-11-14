#include "wpch.hpp"
#include "timeManager.hpp"
#include "global.hpp"

/**
@class TimeManager
Time distribution works as follows:

When Frame function is called, part of the time is distributed between specific activities.
The rest is kept as an additional reserve.
Each activity may use the time it has reserved, and it can use also the time
from additional reserve when it has no more time of its own.
Any activity may report it no longer needs the time reserved
- it is then added into the additional reserve.
Activity may be limited in how much it can use an additional reserve.
*/


TimeManager::TimeManager()
{
  #if _ENABLE_REPORT
    _thread = 0;
  #endif
  Frame(0);
  _actScope = NULL;
  _startTime = GlobalTickCount();
}
TimeManager::~TimeManager()
{
}

const float TimeManager::_timePriority[NTimeClass]=
{
  TIME_CLASS(TC_ENUM_PRIOR1)
};

const float TimeManager::_addTimePriority[NTimeClass]=
{
  TIME_CLASS(TC_ENUM_PRIOR2)
};

void TimeManager::Frame(float totalTime)
{
  #if _ENABLE_REPORT
  if (totalTime>0) // totalTime==0 used during initialization, which is always done on the main thread
    AssertSameThread(_thread);
  #endif
  // how much time should be left as additional
  const float addTimePriority = 0.6f;
  // how much time we will distribute between specific activities
  #if _ENABLE_REPORT
    _addTimeLastFrame = _addTimeRest;
  #endif
  float addTimeRest = addTimePriority*totalTime;
  float divideTime = totalTime*(1-addTimePriority);
  for (int i=0; i<NTimeClass; i++)
  {
    #if _ENABLE_REPORT
      _timeSpentLastFrame[i] = _timeSpent[i];
      _timeReservedLastFrame[i] = _timeReserved[i];
    #endif
    _timeSpent[i] = 0;
    _timeReserved[i] = _timeRest[i] = divideTime*_timePriority[i];
    _addTimeLimit[i] = addTimeRest*_addTimePriority[i];
  }
  _addTimeRest = addTimeRest;
  _addTimeTotal = addTimeRest;
  _startTime = GlobalTickCount();
  _totalTime = totalTime;
}

/**
When time is overcharged, TimeLeft we may return a negative value
*/
float TimeManager::TimeLeft(TimeClass activity) const
{
  AssertSameThread(_thread);
  return _timeRest[activity]+floatMin(_addTimeLimit[activity],_addTimeRest);
}

float TimeManager::TimeLeft(const TimeManagerScope &scope) const
{
  AssertSameThread(_thread);
  float timeSpent = GetSectionTime(scope._section);
  TimeClass activity = scope._activity;
  return _timeRest[activity]+floatMin(_addTimeLimit[activity],_addTimeRest)-timeSpent;
}

void TimeManager::TimeSpent(TimeClass activity, float time)
{
  AssertSameThread(_thread);
  #if _ENABLE_REPORT
    _timeSpent[activity] += time;
  #endif

  if (time<_timeRest[activity])
  {
    _timeRest[activity] -= time;
  }
  else
  {
    // check how much time over our individual limit we used
    float overcharge = time-_timeRest[activity];
    _timeRest[activity] = 0;
    _addTimeRest -= overcharge;
    saturateMax(_addTimeRest,0);
    // FIX: add limits for this activity must be updated as well
    _addTimeLimit[activity] -= overcharge;
    saturateMax(_addTimeLimit[activity],0);
  }
}
void TimeManager::Finished(TimeClass activity, float rest)
{
  AssertSameThread(_thread);
  if (rest<_timeRest[activity])
  {
    float timeMore = _timeRest[activity]-rest;
    // we have more time than we need - move the time rest into the reserve
    // note: this will not increase the per-activity limits
    _addTimeRest += timeMore;
    // increase per-activity limits as well
    for (int i=0; i<NTimeClass; i++)
    {
      _addTimeLimit[i] += timeMore*_addTimePriority[activity];
    }
    _timeRest[activity] = rest;
  }
}

float TimeManager::GetFrameDuration() const
{
  return (GlobalTickCount()-_startTime)*0.001f;
}

float TimeManager::GetTimeSpentTotal() const
{
  float total = 0;
  for (int i=0; i<NTimeClass; i++)
  {
    total += _timeSpent[i];
  }
  return total;
}

#if _ENABLE_REPORT

float TimeManager::GetTimeSpentTotalLastFrame() const
{
  float total = 0;
  for (int i=0; i<NTimeClass; i++)
  {
    total += _timeSpentLastFrame[i];
  }
  return total;
}

#endif

TimeManager GTimeManager;

