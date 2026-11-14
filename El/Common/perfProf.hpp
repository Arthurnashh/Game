#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PERF_PROF_HPP
#define _PERF_PROF_HPP

#include <Es/Strings/rString.hpp>

#if defined(_WIN32) && _ENABLE_PERFLOG
// exact time measurement (for profiling purposes)

#include <El/Common/perfLog.hpp>
#include <Es/Framework/appFrame.hpp>
#include <Es/Common/win.h>

#ifndef _M_PPC
  #pragma warning(disable:4035)
  /// fast and simple version
  // TODO: implement timing for PowerPC
  inline __int64 GetCPUCycles()
  {
    __asm
    {
      rdtsc
    }
    // return value in edx:eax
  }

  /// "syncho" version - cpuid makes sure instructions are not reordered
  inline __int64 GetCPUCyclesSync()
  {
    __asm
    {
      //push ecx
      cpuid // changed eax,ebx,ecx,edx
      rdtsc
      //pop ecx
    }
    // return value in edx:eax
  }
  #pragma warning(default:4035)
#endif

  // QueryPerformanceCounter implementation
  /// current timer state
  
    extern bool UseRDTSC;
    long long GetQPC();
    
    static __forceinline __int64 ProfileTime()
    {
      if (UseRDTSC) return GetCPUCycles();
      else return GetQPC();
    }
  
  /// scale used for the counter
  int ProfileScale();
  /// should be called before PROFILE_SCOPE is used
  void InitProfileScale();

  /// need to be high enough to avoid overflow
  #define PROF_ACQ_SCALE_LOG 4
  /**
  with PROF_ACQ_SCALE_LOG 8 ProfileScale() is around 100 for 3 GHz computer.
  Max. time represented without overflow is ~ 3 minutes
  */
  #define PROF_COUNT_SCALE (ProfileScale())

#define START_PROFILE_EX(name,nameCategory) \
  DEF_COUNTER_P_EX(name,nameCategory,PROF_COUNT_SCALE); \
  __int64 Profile__##name=ProfileTime(); 

#define START_PROFILE(name) START_PROFILE_EX(name,"other")

#define END_PROFILE_EX(name,nameCategory) \
  __int64 Diff_##name=ProfileTime()-Profile__##name; \
  int DiffI_##name=int(Diff_##name>>PROF_ACQ_SCALE_LOG); \
  ADD_COUNTER_P_EX(name,nameCategory,DiffI_##name)

#define END_PROFILE(name) END_PROFILE_EX(name,"other")

#if SCOPED_FAST_CAP

extern void FastCapRoot();
// signatures for Xbox CAP functions - found by reverse engineering
//extern "C" __declspec(dllimport) void __stdcall _CAP_Start_Profiling(void *fnc1, void *fnc2);
//extern "C" __declspec(dllimport) void __stdcall _CAP_End_Profiling(void *fnc);

extern "C" __declspec(dllimport) void __stdcall __CAP_Start_Profiling(void *fnc1, void *fnc2);
extern "C" __declspec(dllimport) void __stdcall __CAP_End_Profiling(void *fnc);

// signatures for X360 CAP functions - found by reverse engineering

// signatures for Xbox CAP functions - found by reverse engineering
// callcap: __Cap_Enter_Function  __Cap_Exit_Function
// fastcap: __CAP_Start_Profiling __CAP_End_Profiling



struct FastCapScope
{
  FastCapScope *prev;
  void *addr;
};

extern FastCapScope *ActualFastCapScope;

/// use once for each counter name to define global variables for it
#define FAST_CAP_REGISTER(name) \
  void FastCap_##name() {LogF("Scope_" #name);} /* different string used to prevent COMDAT folding */

/**
We create a hierarchy of scopes. As an ID for profiling we use unique pointers. As XbPerfView is unable to resolve
address to string constants, we need to provide function names. We did not find any way to define global functions
locally, therefore FAST_CAP_REGISTER needs to be used for each scope once (and only once).
*/
#define FAST_CAP_SCOPE(name) \
  struct FastCap_##name : public FastCapScope \
  { \
    FastCap_##name() \
    { \
      prev = ActualFastCapScope; \
      ActualFastCapScope = this; \
      void *prevAddr = prev ? prev->addr : (void *)FastCapRoot; \
      void FastCap_##name(); /* refer to a scope ID function */\
      addr = (void *)FastCap_##name; \
      __CAP_Start_Profiling(prevAddr, addr); \
    } \
    ~FastCap_##name() \
    { \
      void *prevAddr = prev ? prev->addr : (void *)FastCapRoot; \
      __CAP_End_Profiling(prevAddr); \
      ActualFastCapScope = prev; \
    } \
  }; \
  FastCap_##name fastCap_##name;

#else

#define FAST_CAP_SCOPE(name)

#define FAST_CAP_REGISTER(name)


#endif

class ScopeProfiler: private NoCopy
{
  __int64 _start;
  PerfCounter &_counter;
  bool _timingEnabled;
  bool _profiling;
  /// optional additonal information
  RString _moreInfo;

  /// main thread ID, so that we can exclude work done in parallel from profiling
  static DWORD _mainThread;

public:
  // convert performance counter into a suitable 32b value
  static __forceinline int RelativeTime(__int64 time, __int64 base)
  {
    return int((time-base)>>PROF_ACQ_SCALE_LOG);
  }
  ScopeProfiler(PerfCounter &counter, bool timingEnabled)
    : _counter(counter), _timingEnabled(timingEnabled)
  {
    // once we capture the tail, we never open any scopes, only let them finish
    _profiling = !GPerfProfilers.IsCapturePending() && (GPerfProfilers.IsCapturing() || _timingEnabled && _mainThread == GetCurrentThreadId());
    if (_profiling)
      _start = ProfileTime();
    else
      _start = 0;
  }
  /// how much time was spent from the profiler start
  int TimeSpent() const {return RelativeTime(ProfileTime(),_start);}
  int TimeSpentNorm() const
  {
    if (!_profiling) return 0;
    return TimeSpent()/_counter.Slot()->scale;
  }
  /// check if we are capturing (used to determine if there is any sense providing more info, which can be costly)
  bool IsActive() const {return GPerfProfilers.IsCapturing();}
  bool IsEnabled() const {return _timingEnabled;}
  void AddMoreInfo(const RString &moreInfo){_moreInfo=moreInfo;}
  int GetScale() const {return _counter.Slot()->scale;}
  ~ScopeProfiler()
  {
    // when capturing, we want to capture all scopes ending in this frame, even when they have started before
    if (_profiling || GPerfProfilers.IsCapturing())
    {
      __int64 now = ProfileTime();
      GPerfProfilers.AddCapture(_start, now, _moreInfo, &_counter);
    }
  }
};

#define PROFILE_SCOPE_NAME(name) scopeProf__##name

/// scope for CPU time profiling - choose everything, including implementation class and the name of the object
#define PROFILE_SCOPE_EX_CLS(name,nameCategory,ScopeProfilerClass,objName) \
  DEF_COUNTER_P_EX(name,nameCategory,PROF_COUNT_SCALE); \
  ScopeProfilerClass objName(COUNTER_P_NAME(name),COUNTER_TIMINGENABLED_P(name)); \
  FAST_CAP_SCOPE(name)

/// scope for CPU time profiling - chose name and category
#define PROFILE_SCOPE_EX(name,nameCategory) PROFILE_SCOPE_EX_CLS(name,nameCategory,ScopeProfiler,PROFILE_SCOPE_NAME(name))
#define PROFILE_SCOPE_EX_HIER(name, nameCategory) PROFILE_SCOPE_EX(name, nameCategory)
#define PROFILE_SCOPE(name) PROFILE_SCOPE_EX(name,other)

// on Xbox use PIX
struct Scoped_D3DPERF_Event
{
  Scoped_D3DPERF_Event(const char *name, DWORD color)
  {
    CurrentAppFrameFunctions->ProfileBeginGraphScope(color,name);
  }
  ~Scoped_D3DPERF_Event()
  {
    CurrentAppFrameFunctions->ProfileEndGraphScope();
  }
};

// graphical scope profiling color constants
#define SCOPE_COLOR_GRAY   0xff808080
#define SCOPE_COLOR_RED    0xffff8080
#define SCOPE_COLOR_GREEN  0xff80ff80
#define SCOPE_COLOR_BLUE   0xff8080ff
#define SCOPE_COLOR_YELLOW 0xffffff00

#define PROFILE_SCOPE_GRF_EX(name,nameCategory,color) \
  PROFILE_SCOPE_EX(name,nameCategory); \
  Scoped_D3DPERF_Event _pix##name(#name,color)
#define PROFILE_SCOPE_GRF(name,color) PROFILE_SCOPE_GRF_EX(name,"other",color)

#define PROFILE_SCOPE_DETAIL_GRF_EX(name,nameCategory,color) \
  PROFILE_SCOPE_DETAIL_EX(name,nameCategory); \
  Scoped_D3DPERF_Event _pix##name(#name,color)
#define PROFILE_SCOPE_DETAIL_GRF(name,color) PROFILE_SCOPE_DETAIL_GRF_EX(name,"other",color)

#if (_PROFILE || _DEBUG)
  // Graphical (PIX) scope, no profiling
  #define SCOPE_GRF(name,color) \
    Scoped_D3DPERF_Event pixScopeGrf(name,color)
#else
  #define SCOPE_GRF(name,color)
#endif

#else // if (PerfLogging enabled)

#include <El/Common/perfLog.hpp>

#define START_PROFILE(name)
#define END_PROFILE(name)
#define PROFILE_SCOPE_EX_CLS(name,nameCategory,ScopeProfilerClass,objName)
#define PROFILE_SCOPE_EX(name,nameCategory)
#define PROFILE_SCOPE_EX_HIER(name,nameCategory)
#define PROFILE_SCOPE(name)
#define PROFILE_SCOPE_GRF_EX(name,nameCategory,color)
#define PROFILE_SCOPE_GRF(name,color)
#define PROFILE_SCOPE_DETAIL_GRF_EX(name,nameCategory,color)
#define PROFILE_SCOPE_DETAIL_GRF(name,color)
#define SCOPE_GRF(name,color)
#define FAST_CAP_REGISTER(name)

#define PROF_ACQ_SCALE_LOG 4

__int64 ProfileTime();

class PerfCounter;
/// pretend some profile scope exists, but always return zero


extern struct ScopeProfiler 
{
  ScopeProfiler(){}
  ScopeProfiler(PerfCounter &counter, bool enabled, bool hierarchical=true){}

  static int TimeSpentNorm() {return 0;}
  static void AddMoreInfo(const RString &moreInfo){}
  static bool IsActive() {return false;}

  static DWORD _mainThread;
} GFakeProfileScope;
#define PROFILE_SCOPE_NAME(name) GFakeProfileScope
#define PROF_COUNT_SCALE 1
#endif

#if _ENABLE_PERFLOG>1
  // some counter are only enabled when detailed perf-logging enabled
  #define PROFILE_SCOPE_DETAIL_EX(name,nameCategory) PROFILE_SCOPE_EX(name,nameCategory)
  #define PROFILE_SCOPE_DETAIL(name) PROFILE_SCOPE(name)
#else
  #define PROFILE_SCOPE_DETAIL_EX(name,nameCategory)
  #define PROFILE_SCOPE_DETAIL(name)
#endif

#endif
