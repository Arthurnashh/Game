#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PERFLOG_HPP
#define _PERFLOG_HPP

#include <El/elementpch.hpp>

/// working set - using PSAPI
size_t GetMemoryUsedSize();
/// page file usage - using PSAPI
size_t GetMemoryCommitedSize();

#if _ENABLE_PERFLOG

#include <Es/Strings/bString.hpp>
#include <Es/Strings/rString.hpp>
#include "Es/Threads/multisync.hpp"

class LogFile
{
  FILE *_file;
  bool _attachedToDebugger;
  /// For automatic fflush().
  unsigned _counter;

  public:
  LogFile();
  LogFile( const char *name );
  void Open( const char *name );
  void AttachToDebugger();
  void Close();
  ~LogFile();

  bool IsOpen() const {return _file!=NULL || _attachedToDebugger;}

  void Append( const char *text );
  void PrintF( const char *text, ... );
  void Flush();
  LogFile &operator << ( const char *txt );
  LogFile &operator << ( int i );
  LogFile &operator << ( float f );
};

#define ENABLE_COREL_ANALIS 0

#if ENABLE_COREL_ANALIS
#include <El/CorelAnalys/corelAnalys.hpp>
#endif

/// one counter slot
/**
Used for performance counters and for performance profilers
*/
struct PerfCounterSlot
{
  enum {NLastValues = 16};

  BString<32> _name;
  BString<16> _nameCategory;
  /// value - total time (PerfProfiler) or total value (PerfCounter)
  int value;
  /// count is used for performance scopes to count scope occurrences
  int count;
  /// last value from a closed frame
  int savedValue;
  int savedCount;
  
  int scale; //!< print scale
  DWORD lastNonZero; //!< last GlobalTickCount when the counter was non-zero
  bool disabled; //!< slot printing disabled
  bool enabled; //!< slot printing forced
  //! Flag to determine the slot timing is enabled
  bool _timingEnabled;
  //@{ average for last N frames
  /// result - average
  int smoothValue;
  /// value from the slowest frame
  int slowValue;
  /// count from the slowest frame
  int slowCount;
  /// sum 
  float congregateValue;
  //@}

  /// values from last frames
  int lastValues[NLastValues];
  /// counts from last frames
  int lastCounts[NLastValues];

  #if ENABLE_COREL_ANALIS
  //@( linear regression
  float _regresResult;
  RegressionAnalyser<float> _regres;
  //@}
  #endif

  void Frame(float frameTime, int nCongregate, bool doCongregate, DWORD currentTime=0);
  void Reset();
  /// sum of lastValues
  int SumValue() const;
  /// sum of lastCounts
  int SumCount() const;
  /// get "historical" value (from lastValues)
  int LastValue(int frame) const;
  /// get "historical" count (from lastCounts)
  int LastCount(int frame) const;

  PerfCounterSlot();
};

TypeIsBinary(PerfCounterSlot);

struct PerfCounterSaved
{
  BString<32> _name;
  float duration;
  int count;
};

TypeIsBinaryZeroed(PerfCounterSaved);

class PerfCounter;

/// Frame capture - item of the buffer
struct CaptureBufferItem
{
  /// start of the scope (from the start of frame)
  int _start;
  /// duration of the scope
  int _duration;
  /// scope type
  int _slotIndex;
  /// thread
  int _threadId;
  /// optional additional information
  RString _moreInfo;
};
TypeIsMovableZeroed(CaptureBufferItem)

struct CounterSlotInfo;
#include <Es/Containers/staticArray.hpp>

/// array of all perf-counters / perf-profiles in the game
class PerfCounters : private NoCopy
{
  bool _constructed;
  bool _first,_line;
  int _lines;
  int _skip;

  bool _enabled;
  /// sometimes we want a special handling for profilers
  bool _profilers;
  
  /// set true when current avg. values have already been printed out
  bool _avgDiagnosed;
  /// counting until a congregate
  int _frameCount;
  /// how old is the slowest frame 
  int _slowFrameAge;
  /// duration of the oldest frame
  float _slowFrameTime;
  
  /// number of allocated slots
  const int _maxSlots;
  /// basic information (stored in the external static array to keep multi-core safe access)
  PerfCounterSlot *_counters;
  /// index of the first free slot for _counters, _countersEx
  LONG _countersFreeSlot;

  /// last ("overflow") item needs to be protected against concurrent access
  CriticalSection _captureBufferOverflowLock;
  /// external capture buffer
  CaptureBufferItem *_captureBuffer;
  /// size of the external capture buffer
  const int _captureBufferSize;
  /// number of valid items in _captureBuffer
  LONG _captureBufferIndex;
  /// backup of captured frame available to analysis
  AutoArray<CaptureBufferItem> _capturedFrame;
  AutoArray<PerfCounterSaved> _capturedCounters;

  //@{ total duration of the captured frame (as measured by GetCPUCycles)
  __int64 _capturedFrameStart;
  __int64 _capturedFrameEnd;
  //@}

  /// request to capture a single frame
  int _captureSingleFrame;
  /// request to capture a slow frame
  RString _captureFrameScope;
  /// request to capture a slow frame
  float _captureFrameThreshold;
  /// request to permanent capture
  bool _capturePermanent;
  /// capture enabled
  bool _capture;
  /// captured frame is ready to analysis - needs to be reset when frame is processed
  enum CaptureState
  {
    /// not capturing
    DoNotCapture,
    /// captured frame, we need to capture any trailing scopes
    CaptureTail,
    /// capture finished
    CaptureReady
  };
  CaptureState _captureReady;

  /// start of the current frame
  __int64 _frameStart;
  /// start of the last frame
  __int64 _lastFrameStart;

  /// logging file
  LogFile _logFile;

public:
  PerfCounters(bool profilers,
    const int maxSlots, PerfCounterSlot *counters,
    const int captureBufferSize, CaptureBufferItem *captureBuffer
    );
  ~PerfCounters();

  /// perform initialization after heap is initialized (not during init_seg(compiler))
  void Init();

  void Open( const char *name );
  void Close();
  /// log to the debug output
  RString Diagnose(bool time=false, int maxCounters=INT_MAX);
  /// log average values to the debug output
  void DiagnoseAvg(bool time=false, int maxCounters=INT_MAX, bool lastFrame=false);

  /// check if DiagnoseAvg has any sense
  bool AvgAlreadyDiagnosed() const {return _avgDiagnosed;}
  
  void Enable( bool value=true );
  void Disable(){Enable(false);}
  bool GetEnabled() const {return _enabled;}

  bool IsCapturing() const {return _capture;}
  bool IsCaptureReady() const {return _captureReady==CaptureReady;}
  bool IsCapturePending() const {return _captureReady==CaptureTail;}
  void ResetCaptureReady() {_captureReady=DoNotCapture;}

  
  void CaptureSingleFrame(int frameNo) {_captureSingleFrame = frameNo;}
  void CaptureSlowFrame(RString scope, float threshold)
  {
    _capturePermanent = true;
    _capture = true;
    _captureFrameScope = scope;
    _captureFrameThreshold = threshold;
  }
  void CapturePermanent(bool on) {_capturePermanent = on;} 
  const AutoArray<CaptureBufferItem> &GetCapturedFrame(__int64 &duration, AutoArray<PerfCounterSaved> &counters) const
  {
    counters = _capturedCounters;
    duration=_capturedFrameEnd-_capturedFrameStart;
    return _capturedFrame;
  }

  void AddCapture(__int64 start, __int64 end, const RString &moreInfo, PerfCounter *counter);
  __int64 GetFrameStart() const {return _frameStart;}

  void LogSlowFrame(RString scope, float threshold);

  void Reinit();
  void Reset();
  void Frame(float fps, float frameTime, int nCongregate);
  void CaptureToCounters();

  int N() const;
  const char *Name( int i ) const;
  //int Value( int i ) const;
  /// current value (incomplete)
  int CurrentValue( int i ) const;
  /// last complete value
  int LastValue( int i ) const;
  /// last complete value, without applying of scale
  int LastValueRaw( int i ) const;
  
  int LastCount(int i) const;
  
  int SmoothValue(int i) const;    
  int SlowValue(int i) const;    
  int SlowCount(int i) const;    
  int SumValue(int i) const;
  int LastValue(int i, int frame) const;
  /// duration of the last frame
  void FrameBegEnd(__int64 &beg, __int64 &end) const;
  float Correlation(int i) const; 
  bool Show( int i ) const;
  
  bool WasNonZero( int i, DWORD time ) const;
  PerfCounterSlot *Slot(int i)
  {
    if(!_constructed) return NULL;
    return &_counters[i];
  }
  const PerfCounterSlot *Slot(int i) const
  {
    if(!_constructed) return NULL;
    return &_counters[i];
  }

  void SavePCHeaders();
  void SavePCValues();

  /// search for existing counter
  int Find(const char *name);
  /// create a new counter or use the existing one
  int New(const char *name, const char *nameCategory, int scale);

  void Enable(const char *name, const char *nameCategory);
  void Disable(const char *name, const char *nameCategory);
  void SetScale(const char *name, const char *nameCategory, int scale);

  //! Enable or disable perfcounters according to category name
  void EnableCategory(const char *nameCategory, bool enable = true);

protected:
  // implementation
  void WriteCounters(StaticArrayAuto<CounterSlotInfo> &sorted, int maxCounters, bool time);
  /// save to the offline analysis file
  void Save( float fps );
};

extern PerfCounters GPerfCounters;
extern PerfCounters GPerfProfilers;

/// basic counter
class PerfCounter
{
  friend class PerfCounters;
  
  PerfCounters *_bank;
  const char *_name;
  const char *_nameCategory;
  int _slotIndex;
  int _scale;

public:
  /// Create an uninitialized Counter
  PerfCounter() : _bank(NULL), _slotIndex(-1) {}
  /// Initialize the counter (Initialization splitted to avoid MT problems)
  void Init(const char *name, const char *nameCategory, PerfCounters *counters, int scale = 1)
  {
    _bank = counters;
    _name = name;
    _nameCategory = nameCategory;
    _scale = scale;
    _slotIndex = counters->New(name, nameCategory, scale);
  }

  void operator +=( int value );
  int GetValue() const;
  bool TimingEnabled() const;
  const char *GetName() const {return _name;}
  PerfCounterSlot *Slot()
  {
    Assert(_slotIndex>=0);
    return _bank->Slot(_slotIndex);
  }
  const PerfCounterSlot *Slot() const
  {
    Assert(_slotIndex>=0);
    return _bank->Slot(_slotIndex);
  }
  int SlotNumber(PerfCounters &bank) const
  {
    // verify the callers knows which bank we are in
    Assert(&bank==_bank);
    return _slotIndex;
  }
};


// performance counters helper macros

# define MEMORY_WRITE_BARRIER

#define COUNTER_NAME(name) Counter_##name
#define INIT_COUNTER_NAME(name) initCounter_##name

// our own flag handling initialization of PerfCounter is used to avoid using uninitialized PerfCounter from other threads
#define COUNTER_EX(name, nameCategory) \
  static bool INIT_COUNTER_NAME(name) = false; \
  static PerfCounter COUNTER_NAME(name); \
  if (!INIT_COUNTER_NAME(name)) \
  { \
    COUNTER_NAME(name).Init(#name, #nameCategory, &GPerfCounters); \
    MEMORY_WRITE_BARRIER; \
    INIT_COUNTER_NAME(name) = true; \
  }
#define COUNTER(name) COUNTER_EX(name,"other")
// our own flag handling initialization of PerfCounter is used to avoid using uninitialized PerfCounter from other threads
#define DEF_COUNTER_EX(name, nameCategory, scale) \
  static bool INIT_COUNTER_NAME(name) = false; \
  static PerfCounter COUNTER_NAME(name); \
  if (!INIT_COUNTER_NAME(name)) \
  { \
    COUNTER_NAME(name).Init(#name, #nameCategory, &GPerfCounters, scale); \
    MEMORY_WRITE_BARRIER; \
    INIT_COUNTER_NAME(name) = true; \
  }

#define DEF_COUNTER(name,scale) DEF_COUNTER_EX(name,"other",scale)

#define ADD_COUNTER_EX(name,nameCategory,value) {COUNTER_EX(name,nameCategory);COUNTER_NAME(name)+=(value);}
#define ADD_COUNTER(name,value) ADD_COUNTER_EX(name,"other",value)
#define COUNTER_VALUE(name) (COUNTER_NAME(name).GetValue())
#define COUNTER_TIMINGENABLED(name) (COUNTER_NAME(name).TimingEnabled())

/// scope handling for COUNTER_SCOPE
class AddCounterScope
{
	PerfCounter &_counter;
	PerfCounter &_src;
	int _startValue;
	
	public:
	AddCounterScope(PerfCounter &counter, PerfCounter &src)
	:_counter(counter),_src(src),_startValue(src.GetValue())
	{
	}
	~AddCounterScope()
	{
	  _counter += _src.GetValue()-_startValue;
	}
};

/// track counter change in a scope by another counter
#define ADD_COUNTER_SCOPE(scopeName,counterName) \
  COUNTER(scopeName); \
  COUNTER(counterName); \
  AddCounterScope addCounter__##scopeName##counterName(COUNTER_NAME(scopeName),COUNTER_NAME(counterName))

/// track counter change in a scope by another counter
#define ADD_COUNTER_SCOPE_MORE(scopeName,counterName) \
  COUNTER(counterName); \
  AddCounterScope addCounter__##scopeName##counterName(COUNTER_NAME(scopeName),COUNTER_NAME(counterName))
  
// performance profilers helper macros
#define COUNTER_P_NAME(name) CounterP_##name
#define INIT_COUNTER_P_NAME(name) initCounterP_##name

// #define COUNTER_P(name,nameCategory) static PerfCounter COUNTER_P_NAME(name)(#name,#nameCategory,&GPerfProfilers)
// our own flag handling initialization of PerfCounter is used to avoid using uninitialized PerfCounter from other threads
#define DEF_COUNTER_P_EX(name, nameCategory, scale) \
  static bool INIT_COUNTER_P_NAME(name) = false; \
  static PerfCounter COUNTER_P_NAME(name); \
  if (!INIT_COUNTER_P_NAME(name)) \
  { \
    COUNTER_P_NAME(name).Init(#name, #nameCategory, &GPerfProfilers, scale); \
    MEMORY_WRITE_BARRIER; \
    INIT_COUNTER_P_NAME(name) = true; \
  }
#define DEF_COUNTER_P(name,scale) DEF_COUNTER_P_EX(name,"other",scale)

#define ADD_COUNTER_P_EX(name,nameCategory,value) {COUNTER_P(name,nameCategory);COUNTER_P_NAME(name)+=(value);}
#define ADD_COUNTER_P(name,value) ADD_COUNTER_P_EX(name,"other",value)
#define COUNTER_VALUE_P(name) (COUNTER_P_NAME(name).GetValue())
#define COUNTER_TIMINGENABLED_P(name) (COUNTER_P_NAME(name).TimingEnabled())


#else
// not _ENABLE_PERFLOG

#define COUNTER_EX(name,nameCategory)
#define COUNTER(name)

#define ADD_COUNTER_EX(name,nameCategory,value)
#define ADD_COUNTER(name,value)
#define COUNTER_VALUE(name) 0
#define COUNTER_TIMINGENABLED(name) false

#define DEF_COUNTER_EX(name,nameCategory,scale)
#define DEF_COUNTER(name,scale)

#define ADD_COUNTER_SCOPE(scopeName,counterName)

#define COUNTER_P(name,nameCategory)

#define ADD_COUNTER_P_EX(name,nameCategory,value)
#define ADD_COUNTER_P(name,value)
#define COUNTER_VALUE_P(name) 0
#define COUNTER_TIMINGENABLED_P(name) false

#define DEF_COUNTER_P_EX(name,nameCategory,scale)
#define DEF_COUNTER_P(name,scale)

#endif

#endif
