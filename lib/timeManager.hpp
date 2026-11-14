#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TIME_MANAGER_HPP
#define _TIME_MANAGER_HPP

/**
@file
Global time manager object
*/

#include <El/HiResTime/hiResTime.hpp>

/// factory defining all properties of all activities
//                       priority
#define TIME_CLASS(XX) \
  XX(CreateVertexBuffer,  0.25,  0.80,  Red) \
  XX(LoadTexture,         0.25,  0.70,  Green) \
  XX(PrepareShadow,       0.10,  0.50,  Blue) \
  XX(LoadObjects,         0.40,  0.90,  Yellow) \
  XX(LoadShape,           0.30,  0.90,  Pink) \
  XX(VisUpdate,           0.10,  0.80,  Red) \
  XX(AICenterMap,         0.10,  0.25,  Cyan) \
  XX(LoadMapObjects,      0.40,  0.90,  Yellow)

#define TC_ENUM_DEF(name,prior1,prior2,color) TC##name,
#define TC_ENUM_PRIOR1(name,prior1,prior2,color) prior1,
#define TC_ENUM_PRIOR2(name,prior1,prior2,color) prior2,

/// list of possible time classes
enum TimeClass
{
  TIME_CLASS(TC_ENUM_DEF)
  NTimeClass,
  TCDisabled = NTimeClass,
  TCUninitialized
};

class TimeManagerScope;

/// distribute time between various modules
class TimeManager
{
  #if _ENABLE_REPORT
    mutable int _thread;
    //@{ values from the last frame for debugging
    float _addTimeLastFrame;
    float _timeReservedLastFrame[NTimeClass];
    float _timeSpentLastFrame[NTimeClass];
    //@}
  #endif
  /// time already spent by an activity
  float _timeSpent[NTimeClass];
  /// time left for given activity
  float _timeRest[NTimeClass];
  /// time originally reserved for given activity
  float _timeReserved[NTimeClass];
  /// time which can be used by any activity (within certain limits)
  float _addTimeRest;
  /// time which can be used by any activity (value for whole frame)
  float _addTimeTotal;
  /// limit activities using additional time
  float _addTimeLimit[NTimeClass];
  /// start of the frame (in ms)
  int _startTime;
  /// what was the total time we are controlling
  float _totalTime;
  /// priority for _timeLimit - sum should be one
  static const float _timePriority[NTimeClass];
  /// priority for _addTimeLimit - max should be one or less
  static const float _addTimePriority[NTimeClass];

  TimeManagerScope *_actScope;
  
  public:
  TimeManager();
  ~TimeManager();
  
  /// start a new frame
  void Frame(float totalTime);
  
  float GetTotalTime() const {return _totalTime;}
  /// check how many time is left for an activity
  float TimeLeft(TimeClass activity) const;
  /// check how many time is left for an activity, used with an open scope
  float TimeLeft(const TimeManagerScope &scope) const;
  
  /// activity reports the time it spent
  void TimeSpent(TimeClass activity, float time);
  // activity reports it is finished or it is able to predict how many time it will need
  void Finished(TimeClass activity, float rest=0);

  /// check how many time was spent in this frame
  float GetTimeSpent(TimeClass activity) const {return _timeSpent[activity];}

  /// how much time was spent in all activities in this frame so far
  float GetTimeSpentTotal() const;

  /// get total frame duration so far
  float GetFrameDuration() const;

  #if _ENABLE_REPORT
    //@{ values from the last frame for debugging
    float GetAddTimeLastFrame() const {return _addTimeLastFrame;}
    float GetTimeReservedLastFrame(TimeClass a) const {return _timeReservedLastFrame[a];}
    float GetTimeSpentLastFrame(TimeClass a) const {return _timeSpentLastFrame[a];}

    /// how much time was spent in all activities in last frame
    float GetTimeSpentTotalLastFrame() const;
  #endif

  #if _ENABLE_REPORT || _ENABLE_PERFLOG
    float GetAddTimeThisFrame() const {return _addTimeTotal;}
    float GetAddTimeRestThisFrame() const {return _addTimeRest;}
    //@}
  #endif


  TimeManagerScope *NewScope(TimeManagerScope *scope)
  {
    AssertSameThread(_thread);
    TimeManagerScope *ret = _actScope;
    _actScope = scope;
    return ret;
  }

  bool EndScope(TimeManagerScope *scope, TimeManagerScope *prev)
  {
    AssertSameThread(_thread);
    Assert(_actScope==scope);
    _actScope = prev;
    return prev==NULL;
  }
};


extern TimeManager GTimeManager;

/// measure how much time was spent in given scope are report it to global TimeManager
class TimeManagerScope : private NoCopy
{
  friend class TimeManager;
  
  SectionTimeHandle _section;
  TimeClass _activity;
  /// scope chain
  TimeManagerScope *_prevScope;
  TimeManager *_manager;
  
  public:
  /// empty scope for later initialization by Init
  TimeManagerScope()
  :_activity(TCUninitialized)
  {
    _prevScope = NULL;
    _manager = NULL;
  }
  /// start already existing timing object
  void Start(TimeClass activity, TimeManager &manager=GTimeManager)
  {
    Assert(_activity == TCUninitialized);
    Assert(_prevScope == NULL);
    _section = StartSectionTime();
    _activity = activity;
    _manager = &manager;
    _prevScope = _manager->NewScope(this);
  }
  TimeManagerScope(TimeClass activity, TimeManager &manager=GTimeManager):
  _activity(activity),_section(StartSectionTime())
  {
    AssertMainThread();
    Assert(_activity < TCUninitialized);
    _manager = &manager;
    _prevScope = _manager->NewScope(this);
  }
  ~TimeManagerScope()
  {
    // only topmost scope is tracked
    // FIX: for TCDisabled, GTimeManager.EndScope need to be called
    if (_activity < TCUninitialized && _manager->EndScope(this, _prevScope) && _activity != TCDisabled)
    {
      _manager->TimeSpent(_activity, GetSectionTime(_section));
    }
  }
};

/// disable time manager in given scope
class TimeManagerDisabled: public TimeManagerScope
{
  public:
  TimeManagerDisabled(TimeManager &manager=GTimeManager):TimeManagerScope(TCDisabled,manager){}
};


#endif
