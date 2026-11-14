// implementation of performance monitor + logging

#include <El/elementpch.hpp>
#include <Es/Framework/appFrame.hpp>
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>

#if _MSC_VER && !defined INIT_SEG_COMPILER
  // we know no memory allocation is done here
  // we want to be constructed as early as possible - sooner than RStringB, which does allocation
  // otherwise allocation scopes are not working
  #pragma warning(disable:4074)
  #pragma init_seg(compiler)
  #define INIT_SEG_COMPILER
#endif

#ifndef _WIN32
#include <Es/Threads/pothread.hpp>
#endif

#include <Es/Common/win.h>

//#include "psapi.h"
// search for "psapi"
// Structure for GetProcessMemoryInfo()

typedef struct _PROCESS_MEMORY_COUNTERS {
    DWORD cb;
    DWORD PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
     SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;
typedef PROCESS_MEMORY_COUNTERS *PPROCESS_MEMORY_COUNTERS;

#ifdef _WIN32

typedef BOOL WINAPI GetProcessMemoryInfoF
(
  HANDLE Process,
  PPROCESS_MEMORY_COUNTERS ppsmemCounters,
  DWORD cb
);

static GetProcessMemoryInfoF *GetProcessMemoryInfo;

#endif

size_t GetMemoryUsedSize()
{
#ifdef _WIN32
  if (!GetProcessMemoryInfo) return 0;
  PROCESS_MEMORY_COUNTERS pc;
  pc.cb = sizeof(pc);
  if (GetProcessMemoryInfo(GetCurrentProcess(),&pc,sizeof(pc)))
  {
    return pc.WorkingSetSize;
  }
#endif
  return 0;
}

size_t GetMemoryCommitedSize()
{
#ifdef _WIN32
  if (!GetProcessMemoryInfo) return 0;
  PROCESS_MEMORY_COUNTERS pc;
  pc.cb = sizeof(pc);
  if (GetProcessMemoryInfo(GetCurrentProcess(),&pc,sizeof(pc)))
  {
    return pc.PagefileUsage;
  }
#endif
  return 0;
}

static struct InitPSAPI
{
  InitPSAPI()
  {
    static bool once = true;
    if (!once) return;
    once = false;
    #if defined(_WIN32) 
    HINSTANCE module = LoadLibraryA("psapi.dll");
    if (!module) return;
    void *pa = GetProcAddress(module,"GetProcessMemoryInfo");
    if (pa)
    {
      GetProcessMemoryInfo = (GetProcessMemoryInfoF *)pa;
    }
    #endif
  }
} SInitPSAPI; 


#if _ENABLE_PERFLOG

#include <Es/Strings/bString.hpp>
#include <Es/Common/fltopts.hpp>
#include <Es/Containers/staticArray.hpp>

void PerfCounters::Enable( bool value )
{
  if( !_enabled && value )
  {
    Reset();
    Reinit();
  }
  _enabled=value;
  if( !_enabled ) _first=true;
}


void PerfCounters::SavePCHeaders()
{
  _logFile <<"  wset";
  //_logFile <<"  mcom";

}

void PerfCounters::SavePCValues()
{
  _logFile << (int)(GetMemoryUsedSize()/(1024*1024));
  //_logFile << (int)(GetMemoryCommitedSize()/(1024*1024));
  // note: works only for Win9x
  // TODO: WinNT performance counters, see HKEY_PERFORMANCE_DATA
}

static int CompareCounters(const int *c1, const int *c2, const PerfCounters *context)
{
  const PerfCounterSlot *slot1=context->Slot(*c1);
  const PerfCounterSlot *slot2=context->Slot(*c2);
  return slot2->value-slot1->value;
}

struct CounterSlotInfo
{
  int index;
  int value;
  int count;
};
TypeIsSimple(CounterSlotInfo)

static int CompareCountersAvg(const CounterSlotInfo *c1, const CounterSlotInfo *c2)
{
  return c2->value - c1->value;
}

#include <Es/Algorithms/qsort.hpp>

/// format numeric value so that it fits in 5 characters
template <class String>
static void FormatValue(String &value, int slotValue, bool time)
{
  if (time)
  {
    if (slotValue<=99999)
    {
    // time in 1/100 ms
    sprintf(value,"%6d",slotValue);
  }
    else
  {
      // time in s
      sprintf(value,"%5.3gs",float(slotValue)*1e-5f);
  }
  }
  else
  {
    if (slotValue<99999)
    {
      // time in 1/100 ms
      sprintf(value,"%6d",slotValue);
  }
    else if (slotValue<999999)
    {
      sprintf(value,"%5.3gk",float(slotValue)*1e-3f);
}
    else
    {
      sprintf(value,"%5.3gM",float(slotValue)*1e-6f);
    }

  }
}

RString PerfCounters::Diagnose(bool time, int maxCounters)
{
  if( !_enabled ) return RString();

  // avoid increasing the size during the function
  int slotsCount = _countersFreeSlot;
  saturateMin(slotsCount, _maxSlots);

  // auto-enable counters
  BString<512> header;
  BString<512> line;
  BString<512> count;
  AUTO_STATIC_ARRAY(int,sorted,64);
  for( int i=0; i<slotsCount; i++ )
  {
    const PerfCounterSlot &slot=_counters[i];
    if( slot.disabled ) continue;
    if( slot.value==0 ) continue;
    sorted.Add(i);
  }
  QSort(sorted.Data(),sorted.Size(),this,CompareCounters);
  for( int i=0; i<sorted.Size() && maxCounters>0; i++ )
  {
    int slotI = sorted[i];
    const PerfCounterSlot &slot=_counters[slotI];
    if( slot.disabled ) continue;
    if( slot.value==0 || slot.scale==0) continue;
    BString<7> value;
    BString<7> name;
    sprintf(name,"%6s",cc_cast(slot._name));
    int slotValue = slot.value/slot.scale;
    FormatValue(value,slotValue,time);
    strcat(header,name);
    strcat(line,value);
    if (_profilers)
    {
      FormatValue(value,slot.count,time);
      strcat(count,value);
    }
    --maxCounters;
  }
  
  if (_profilers) return Format("%s\n%s\n%s", header.cstr(), line.cstr(), count.cstr());
  return Format("%s\n%s", header.cstr(), line.cstr());
}

void PerfCounters::WriteCounters(StaticArrayAuto<CounterSlotInfo> &sorted, int maxCounters, bool time)
{
  BString<512> header;
  BString<512> line;
  BString<512> count;
  for( int i=0; i<sorted.Size() && maxCounters>0; i++ )
  {
    int slotI = sorted[i].index;
    const PerfCounterSlot &slot=_counters[slotI];
    if( slot.disabled ) continue;
    int slotValue = sorted[i].value;
    if (slotValue == 0) continue;
    BString<7> value;
    BString<7> name;
    sprintf(name,"%6s",cc_cast(slot._name));
    FormatValue(value,slotValue,time);
    strcat(header,name);
    strcat(line,value);
    if (_profilers)
    {
      FormatValue(value,sorted[i].count,time);
      strcat(count,value);
    }
    --maxCounters;
  }
  LogF("%s",header.cstr());
  LogF("%s",line.cstr());
  LogF("%s",count.cstr());
}

/**
@param lastFrame Report last frame instead of the slowest one.
*/
void PerfCounters::DiagnoseAvg(bool time, int maxCounters, bool lastFrame)
{
  if( !_enabled && !lastFrame ) return;
  _avgDiagnosed = true;

  // avoid increasing the size during the function
  int slotsCount = _countersFreeSlot;
  saturateMin(slotsCount, _maxSlots);

  // select nonempty frame and frame with minimal fps
  int frameMaxIndex = -1;
  int frames = 0;
  int frameMaxValue = 0;
  for (int frame=0; frame<PerfCounterSlot::NLastValues; frame++)
  {
    // select max. value from all counters - this should closely correspond to lowest fps
    // there is some scope which holds whole frame
    int frameMax = 0;
    for( int i=0; i<slotsCount; i++ )
    {
      const PerfCounterSlot &slot = _counters[i];
      if (slot.disabled) continue;
      saturateMax(frameMax, slot.LastValue(frame));
    }
    if (frameMax > 0) frames++;
    if (frameMax >= frameMaxValue) // latest value is more interesting for us
    {
      frameMaxValue = frameMax;
      frameMaxIndex = frame;
    }
  }
  if (frames == 0) return;
  Assert(frameMaxIndex >= 0);
  if (lastFrame)
  {
    frameMaxIndex = PerfCounterSlot::NLastValues-1;
  }

  // auto-enable counters
  AUTO_STATIC_ARRAY(CounterSlotInfo,sorted,64);

  // min fps
  for( int i=0; i<slotsCount; i++ )
  {
    const PerfCounterSlot &slot = _counters[i];
    if (slot.disabled) continue;
    int value = toLargeInt((float)slot.LastValue(frameMaxIndex) / (float)slot.scale);
    if (value > 0)
    {
      int index = sorted.Add();
      sorted[index].index = i;
      sorted[index].value = value;
      sorted[index].count = slot.LastCount(frameMaxIndex);
    }
  }
  QSort(sorted.Data(),sorted.Size(),CompareCountersAvg);
  WriteCounters(sorted, maxCounters,time);

  sorted.Clear();

  // avg fps
  for( int i=0; i<slotsCount; i++ )
  {
    const PerfCounterSlot &slot = _counters[i];
    if (slot.disabled) continue;
    int value = toLargeInt((float)slot.SumValue() / (float)(slot.scale * frames));
    if (value > 0)
    {
      int index = sorted.Add();
      sorted[index].index = i;
      sorted[index].value = value;
      sorted[index].count = slot.SumCount()/frames;
    }
  }
  QSort(sorted.Data(),sorted.Size(),CompareCountersAvg);
  WriteCounters(sorted, maxCounters,time);
}

void PerfCounters::Save( float fps )
{
  _lastFrameStart = _frameStart;

  // avoid increasing the size during the function
  int slotsCount = _countersFreeSlot;
  saturateMin(slotsCount, _maxSlots);

  for (int i=0; i<slotsCount; i++)
  {
    PerfCounterSlot &slot=_counters[i];
    slot.savedValue = slot.value;
    slot.savedCount = slot.count;
  }
  // auto-enable counters
  for( int i=0; i<slotsCount; i++ )
  {
    PerfCounterSlot &slot=_counters[i];
    if( slot.disabled ) continue;
    if( !slot.enabled )
    {
      if( slot.value==0 ) continue;
      slot.enabled=true;
    }
  }
  if( !_enabled ) return;
  if( _skip>0 )
  {
    --_skip;
    return;
  }
  // check if save is enabled
  // repeat headers
  if( _lines++>=30 ) _first=true;
  if( _first )
  {
    _first=false;
    _lines=0;
    if (_logFile.IsOpen())
    {
      _logFile << "   fps";
      SavePCHeaders();
      _logFile << " Alloc";
      _logFile << " NFree";
      //_logFile << " NB100";
      for( int i=0; i<slotsCount; i++ )
      {
        const PerfCounterSlot &slot=_counters[i];
        if( slot.disabled || !slot.enabled ) continue;
        char name[256];
        sprintf(name,"%6s",cc_cast(slot._name));
        name[6]=0;
        _logFile<<name;
      }
      _logFile << "\n";
    }
  }
  if( _line )
  {
    _line=false;
    _logFile << "----------------------------------------------------";
    _logFile << "-----------------------------------------\n";
  }
  if (_logFile.IsOpen())
  {
    _logFile << floatMin(fps,999);
    SavePCValues();
    _logFile << (int)MemoryUsed()/(1024*1024);
    _logFile << (int)MemoryFreeBlocks();
    //_logFile << MemoryAllocatedBlocks()/100;
    for( int i=0; i<slotsCount; i++ )
    {
      const PerfCounterSlot &slot=_counters[i];
      if( slot.disabled || !slot.enabled || slot.scale==0) continue;
      _logFile<<(slot.value/slot.scale);
    }

    _logFile << "\n";
  }
}

static PerfCounterSlot GPerfCountersCounters[150] INIT_PRIORITY_URGENT;
PerfCounters GPerfCounters(false, lenof(GPerfCountersCounters), GPerfCountersCounters, 0, NULL) INIT_PRIORITY_URGENT;

static PerfCounterSlot GPerfProfilersCounters[640] INIT_PRIORITY_URGENT;
// should store all ScopeProfiler instances in a single (in-game) frame
static CaptureBufferItem GCaptureBuffer[100000];
PerfCounters GPerfProfilers(true,
  lenof(GPerfProfilersCounters), GPerfProfilersCounters,
  lenof(GCaptureBuffer), GCaptureBuffer) INIT_PRIORITY_URGENT;

void OpenPerfCounters()
{
  GPerfCounters.Open("events.spf");
  GPerfProfilers.Open("timing.spf");
}
void ClosePerfCounters()
{
  GPerfCounters.Close();
  GPerfProfilers.Close();
}

void PerfCounter::operator +=( int value )
{
  if (!_bank || _slotIndex<0) return;
  PerfCounterSlot *slot = Slot();
  if (slot)
  {
    slot->value+=value;
    slot->count++;
  }
}
int PerfCounter::GetValue() const
{
  if( _slotIndex>=0 && _bank)
  {
    const PerfCounterSlot *slot = Slot();
    if (slot) return slot->value;
  }
  return 0;
}

bool PerfCounter::TimingEnabled() const
{
  if (_slotIndex >= 0)
  {
    const PerfCounterSlot *slot = Slot();
    return slot && slot->_timingEnabled;
  }
  return false;
}

PerfCounters::PerfCounters(bool profilers,
  const int maxSlots, PerfCounterSlot *counters,
  const int captureBufferSize, CaptureBufferItem *captureBuffer
  )
  : _maxSlots(maxSlots), _counters(counters), _captureBufferSize(captureBufferSize), _captureBuffer(captureBuffer)
{
  _constructed=true;
  _enabled=false;
  _skip=5; // skip some frames - to reach stable environment
  _profilers = profilers;
  _countersFreeSlot = 0; // all slots unused now
  _frameStart = 0;
  _lastFrameStart = 0;

  _captureBufferIndex = 0;
  _captureSingleFrame = 0;
  _captureFrameThreshold = 0;
  _capturePermanent = false;
  _capture = false;
  _captureReady = DoNotCapture;

  Reinit();
  // some default setting
  if (!_profilers)
  {
    DEF_COUNTER(mSize, 1024);
    DEF_COUNTER(tHeap, 1024);
    DEF_COUNTER(tGRAM, 1024);
  }
}

void PerfCounters::Open(const char *name)
{
  _logFile.Open(name);
  _skip=3; // skip some frames - to reach stable environment
}

void PerfCounters::Close()
{
  _logFile.Close();
}

PerfCounters::~PerfCounters()
{
  // make sure all columns have corresponding header
  _first=true;
  Save(0);
  _constructed = false;
}

void PerfCounters::Reinit()
{
  _first=true;
  _line=true;
  _lines=0;
  _frameCount = 0;
  _avgDiagnosed = false;

  // avoid increasing the size during the function
  int slotsCount = _countersFreeSlot;
  saturateMin(slotsCount, _maxSlots);

  for( int i=0; i<slotsCount; i++ )
  {
    _counters[i].Reset();
  }
}

void PerfCounters::Reset()
{
  _frameCount = 0;
  _avgDiagnosed = false;

  // avoid increasing the size during the function
  int slotsCount = _countersFreeSlot;
  saturateMin(slotsCount, _maxSlots);

  for( int i=0; i<slotsCount; i++ )
  {
    _counters[i].Reset();
  }
}

void PerfCounters::AddCapture(__int64 start, __int64 end, const RString &moreInfo, PerfCounter *counter)
{
  // when item is closed which was never open, start is zero - saturate to frame
  if (start<_capturedFrameStart) start = _capturedFrameStart;

  start >>= PROF_ACQ_SCALE_LOG;
  end >>= PROF_ACQ_SCALE_LOG;
  
  int startRel = start - (_capturedFrameStart>>PROF_ACQ_SCALE_LOG);
  int duration = end-start;

  int slotIndex = counter->SlotNumber(GPerfProfilers);

  int halfSize = _captureBufferSize>>1;
  // the more data we have, the more we discard too short samples
  // half-empty: discard noting
  // full: discard everything but 0.5 of the frame
//   float frameRatio = float(duration)*2 / float(startRel+duration);
//   float inBuffer = float(_captureBufferIndex-_captureBufferSize/2) / float(_captureBufferSize/2);
//   float inBuffer = float(_captureBufferIndex-halfSize) / float(halfSize);
//   if (frameRatio<inBuffer)
//  if (float(duration)*2*float(halfSize) < float(_captureBufferIndex-halfSize)*float(startRel+duration))
  if (float(duration)*float(_captureBufferSize) < float(_captureBufferIndex-halfSize)*float(startRel+duration))
  {

    PerfCounterSlot *slot = Slot(slotIndex);
    #if _WIN32
      // atomic add with no overflow
      for(;;) 
      {
        unsigned originalValue = slot->value;
        unsigned newValue = originalValue+duration;
        if (newValue>unsigned(INT_MAX)) newValue = INT_MAX;
        if (_InterlockedCompareExchange((long *)&slot->value,newValue,originalValue)==originalValue) break;
      }
      _InterlockedIncrement((long *)&slot->count);
    #else
      unsigned newValue = slot->value + duration;
      if (newValue>unsigned(INT_MAX)) addNoOverflow = INT_MAX;
      slot->value = newValue;
      slot->count++;
    #endif
    
    return;
  }
  
    
  // new item need to be created
  LONG index = InterlockedIncrement(&_captureBufferIndex);
  if (index < _captureBufferSize)
  {
  // index is now first free slot -> fill last used slot
    CaptureBufferItem &item = _captureBuffer[index-1];

    item._start = startRel;
    item._duration = duration;
    item._threadId = GetCurrentThreadId();
    item._moreInfo = moreInfo;
    item._slotIndex = slotIndex;
  }
  else
  {
    // fight about the last entry - the longest duration should win
    CaptureBufferItem &item = _captureBuffer[_captureBufferSize-1];
  
    // optimization: fail often without taking the critical section
    if (duration>item._duration)
    {
      ScopeLockSection lock(_captureBufferOverflowLock);
      if (duration>item._duration)
      {
        item._start = startRel;
        item._duration = duration;
  item._threadId = GetCurrentThreadId();
  item._moreInfo = moreInfo;
        item._slotIndex = slotIndex;
}
    }
  }
}

void PerfCounters::LogSlowFrame(RString scope, float threshold)
{

}

void PerfCounters::Frame(float fps, float frameTime, int nCongregate)
{
  // handle the capture buffer
  bool captured = _capture;
  _capture = false; // stop capturing now

  int n = _captureBufferIndex; // ignore the next scopes to simplify analysis
  // the pointer may have overflown the buffer - read only the valid part
  if (n>_captureBufferSize)
  {
    LogF("Capture overflow: too many items (%d>%d)",n,_captureBufferSize);
    n = _captureBufferSize;
  }
  // values and counters are valid now, store them to backup values
  Save(fps);

  if (_captureBuffer)
  {
    
  // decide if deep capture is needed next frame
  if (_captureSingleFrame > 0)
  {
    if (--_captureSingleFrame == 0) // counter reached
    {
      _capture = true;
    }
  }
  else if (_capturePermanent)
  {
    if (_captureFrameScope.GetLength() > 0)
    {
      // check whether the last frame is the one we want to store
      int index = Find(_captureFrameScope);
      if (index >= 0)
      {
        int value = CurrentValue(index);
        if (0.00001f * value >= _captureFrameThreshold)
        {
          // the scope exceeds the limit
          _capturePermanent = false;
        }
      }
    }
    _capture = _capturePermanent;
  }
  __int64 now = ProfileTime();

  if (_captureReady==CaptureTail)
  { // tail open - process it
    // backup the buffer
    _capturedFrame.Copy(_captureBuffer, n);
      int captureSlots = _countersFreeSlot; // reduce risk of race condition - copy the value, _countersFreeSlot might be modified
      saturateMin(captureSlots, _maxSlots);

      _capturedCounters.Resize(0);
      for (int i=0; i<captureSlots; i++)
      {
        if (_counters[i].lastCounts[PerfCounterSlot::NLastValues-1]<=0) continue;
        PerfCounterSaved &save = _capturedCounters.Append();
        save._name = _counters[i]._name;
        float coef = 0.01f / (float)_counters[i].scale;
        save.duration = coef*_counters[i].lastValues[PerfCounterSlot::NLastValues-1];
        save.count = _counters[i].lastCounts[PerfCounterSlot::NLastValues-1];
      }
    // advise the data are ready
    _captureReady = CaptureReady;
  }
  else if (captured && !_capture)
  {
    _captureReady = CaptureTail;
    _capturedFrameStart = _frameStart;
    _capturedFrameEnd = now;
  }
  
  if (_captureReady!=CaptureTail)
  {
    // we need to call RString destructors here, to prevent other threads calling them
    // as we know no thread other than main should be using moreInfo, this is safe
    for (int i=0; i<n; i++)
    {
      _captureBuffer[i]._moreInfo = RString();
    }
      // reset so that fight about this item is predictable
      _captureBuffer[_captureBufferSize-1]._start = INT_MAX;
      _captureBuffer[_captureBufferSize-1]._duration = 0;
    
    // note: not 100 % thread safe. Some thread may be writing at this time
    // as a result nonsense results are sometimes possible
    _captureBufferIndex = 0; // erase the buffer
  }
  _frameStart = now;
  if (_captureReady==DoNotCapture) _capturedFrameStart = _frameStart;
  }


  _frameCount++;
  // if the slowest frame is too old or the current frame is slower, reset it
  _slowFrameAge++;

  // avoid increasing the size during the function
  int slotsCount = _countersFreeSlot;
  saturateMin(slotsCount, _maxSlots);

  /* The following code allows to catch pikes in oPas1 counter... . 
  int j = 0;
  for(;j < _counters.Size(); j++)
  {
    if (strcmp(_counters[j].name,"oPas1") == 0)
      break;
  }
  static float  frameTimeGlobal = 0;
  frameTimeGlobal += frameTime;
  //LogF("frameTimeGlobal %lf", frameTimeGlobal);
  if (frameTimeGlobal > 100 && j != _counters.Size() && _counters[j].slowValue < _counters[j].value)
  */
  if (_slowFrameTime<frameTime || _slowFrameAge>200)

  {
    for( int i=0; i<slotsCount; i++ )
    {
      _counters[i].slowValue=_counters[i].value;
      _counters[i].slowCount=_counters[i].count;
    }
    _slowFrameTime = frameTime;
    _slowFrameAge = 0;
  }
  bool doCongregate = _frameCount>=nCongregate;
  int currentTime = ::GlobalTickCount();
  for( int i=0; i<slotsCount; i++ )
  {
    PerfCounterSlot &slot = _counters[i];

    slot.Frame(frameTime, _frameCount, doCongregate,currentTime);
    slot.value = 0;
    slot.count = 0;
  }
  if (doCongregate)
  {
    _frameCount = 0;
    _avgDiagnosed = false;
  }
}

int PerfCounters::N() const
{
  return intMin(_countersFreeSlot, _maxSlots);
}

const char *PerfCounters::Name( int i ) const
{
  return _counters[i]._name;
}

void PerfCounters::FrameBegEnd(__int64 &beg, __int64 &end) const
{
  beg = _lastFrameStart;
  end = _frameStart;
}

int PerfCounters::LastValue( int i ) const
{
  if (_counters[i].scale==0) return 0;
  return _counters[i].savedValue/_counters[i].scale;
}

int PerfCounters::LastValueRaw( int i ) const
{
  return _counters[i].savedValue;
}

int PerfCounters::LastCount(int i) const
{
  return _counters[i].savedCount;
}


int PerfCounters::CurrentValue( int i ) const
{
  if (_counters[i].scale==0) return 0;
  return _counters[i].value/_counters[i].scale;
}

int PerfCounters::SlowValue( int i ) const
{
  if (_counters[i].scale==0) return 0;
  return _counters[i].slowValue/_counters[i].scale;
}

int PerfCounters::SlowCount( int i ) const
{
  return _counters[i].slowCount;
}

int PerfCounters::SmoothValue( int i ) const
{
  return _counters[i].smoothValue;
}

int PerfCounters::SumValue(int i) const
{
  return _counters[i].SumValue();
}

float PerfCounters::Correlation(int i) const
{
  #if ENABLE_COREL_ANALIS
  return _counters[i]._regresResult;
  #else
  return 1;
  #endif
}



bool PerfCounters::Show( int i ) const
{
  return !_counters[i].disabled && _counters[i].enabled;
}

bool PerfCounters::WasNonZero( int i, DWORD time ) const
{
  return _counters[i].lastNonZero>time;
}


int PerfCounters::Find(const char *name)
{
  // avoid increasing the size during the function
  int slotsCount = _countersFreeSlot;
  saturateMin(slotsCount, _maxSlots);

  for( int i=0; i<slotsCount; i++ )
  {
    if (!strcmp(_counters[i]._name, name)) return i;
  }
  return -1;
}

PerfCounterSlot::PerfCounterSlot()
{
  strcpy(_name,"");
  strcpy(_nameCategory,"");
  value=0;
  count=0;
  scale=1;
  enabled=true;
  disabled=false;

  _timingEnabled = false;

  congregateValue = 0;
  smoothValue = 0;
  for (int i=0; i<NLastValues; i++)
  {
    lastValues[i] = 0;
    lastCounts[i] = 0;
  }

  slowValue = 0;
  slowCount = 0;
  #if ENABLE_COREL_ANALIS
  _regresResult = 0;
  #endif
}

/**
@param frameTime frame time in seconds - needed for regression analysis
*/

void PerfCounterSlot::Frame(float frameTime, int nCongregate, bool doCongregate, DWORD currentTime)
{
  if (value) lastNonZero = currentTime;

  for (int i=0; i<NLastValues-1; i++)
  {
    lastValues[i] = lastValues[i+1];
    lastCounts[i] = lastCounts[i+1];
  }
  lastValues[NLastValues-1] = value;
  lastCounts[NLastValues-1] = count;

  if (scale!=0)
  {
    congregateValue += value/(float)scale;
  }
  #if ENABLE_COREL_ANALIS
    _regres.Sample(frameTime,value);
  #endif
  if (doCongregate)
  {
    smoothValue = toLargeInt(congregateValue / nCongregate);
    congregateValue = 0;        
    #if ENABLE_COREL_ANALIS
      _regresResult = _regres.Result();
      _regres.Reset();
    #endif
  }
}

int PerfCounterSlot::SumValue() const
{
  int sum = 0;
  for (int i=0; i<NLastValues; i++) sum += lastValues[i];
  return sum;
  //return toLargeInt(sum / (float)(NLastValues * scale));
}

int PerfCounterSlot::SumCount() const
{
  int sum = 0;
  for (int i=0; i<NLastValues; i++) sum += lastCounts[i];
  return sum;
}

int PerfCounterSlot::LastValue(int frame) const
{
  if (frame >= NLastValues) return 0;
  return lastValues[frame];
}

int PerfCounterSlot::LastCount(int frame) const
{
  if (frame >= NLastValues) return 0;
  return lastCounts[frame];
}

void PerfCounterSlot::Reset()
{
  lastNonZero = 0;
  value = 0;
  count = 0;
  smoothValue = 0;
  slowValue = 0;
  slowCount = 0;
  savedValue = 0;
  savedCount = 0;
  congregateValue = 0;
  for (int i=0; i<NLastValues; i++)
  {
    lastValues[i] = 0;
    lastCounts[i] = 0;
  }

  #if ENABLE_COREL_ANALIS
    _regres.Reset();
  #endif
}

int PerfCounters::New(const char *name, const char *nameCategory, int scale)
{
  if( !_constructed )
  {
    return -1;
  }
  int index = Find(name);
  if (index >= 0) return index;

  // new slot need to be created
  index = InterlockedIncrement(&_countersFreeSlot);
  if (index > _maxSlots)
  {
    RptF("No more counters enabled %d/%d, %s ignored", index, _maxSlots, name);
    return -1;
  }
  index--; // first free slot -> last used slot

  PerfCounterSlot &slot = _counters[index];
  slot._name =  name;
  slot._nameCategory = nameCategory;
  // QueryPerformanceCounter is quite slow on PC - avoid using it too often
  slot._timingEnabled = slot._nameCategory[0] == '*';
  slot.scale = scale;
  slot.lastNonZero = 0;
  //LogF("New perf slot %s",slot.name);
  return index;
}

void PerfCounters::Enable(const char *name, const char *nameCategory)
{
  int index = Find(name);
  if( index>=0 ) _counters[index].enabled=true;
}

void PerfCounters::SetScale(const char *name, const char *nameCategory, int scale)
{
  int index=Find(name);
  if( index>=0 ) _counters[index].scale=scale;
}

void PerfCounters::Disable(const char *name, const char *nameCategory)
{
  int index=Find(name);
  if( index>=0 ) _counters[index].disabled=true;
}

void PerfCounters::EnableCategory(const char *nameCategory, bool enable)
{
  // avoid increasing the size during the function
  int slotsCount = _countersFreeSlot;
  saturateMin(slotsCount, _maxSlots);

  for (int i = 0; i < slotsCount; i++)
  {
    // Skip in case the counter doesn't belong to the category
    // empty category means the counter is always enabled
    if (_counters[i]._nameCategory[0]!=0)
    {
      //if (strnicmp(_counters[i]._nameCategory, nameCategory, nameCategoryLength)) continue;
      // any substring match enables the counter
      // this is done to allow for multiple categories, like *sim
      if (!strstr(_counters[i]._nameCategory, nameCategory)) continue;
    }

    // Enable / disable timing
    _counters[i]._timingEnabled = enable;
  }
}

void PerfCounters::CaptureToCounters()
{
  // Analysis - aggregate frame capture to counters
  // note: this is not 100 % thread safe. Some thread may be still writing
  // as a result nonsense results are sometimes possible
  int n = _captureBufferIndex; // ignore the next scopes to simplify analysis
  // the pointer may have overflown the buffer - read only the valid part
  if (n>_captureBufferSize) n = _captureBufferSize;
  for (int i=0; i<n; i++)
  {
    const CaptureBufferItem &item = _captureBuffer[i];
    PerfCounterSlot *slot = Slot(item._slotIndex);
    
    unsigned newValue = slot->value + item._duration;
    if (newValue>unsigned(INT_MAX)) newValue = INT_MAX;
    slot->value = newValue;
    slot->count++;
  }
}



void LogFile::AttachToDebugger()
{
  _attachedToDebugger = true;
}

void LogFile::Open( const char *name )
{
  Close();
  if (!name || !*name) return; // empty name - no logging
  _file=fopen(name,"w");
  _counter = 0;
  if( !_file ) return;
  setvbuf(_file,NULL,_IOFBF,32*1024);
}
void LogFile::Close()
{
  if( !_file ) return;
  fclose(_file);
  _file=NULL;
  _attachedToDebugger = false;
}

LogFile::LogFile()
{
  _file=NULL;
  _attachedToDebugger = false;
}

LogFile::LogFile( const char *name )
{
  Open(name);
}

LogFile::~LogFile()
{
  Close();
}


void LogFile::Append( const char *text )
{
#ifdef _WIN32
  if (_attachedToDebugger)
  {
    OutputDebugStringA(text);
  }
#endif
  if( !_file ) return;
  fwrite(text,strlen(text),1,_file);
  if ( !(++_counter & 0xff) )
  fflush(_file);
}

void LogFile::Flush ()
{
  if ( !_file ) return;
  fflush(_file);
  _counter = 0;
}

void LogFile::PrintF( const char *text, ... )
{
  if (!IsOpen()) return;
  BString<1024> buf;
  va_list arglist;
  va_start( arglist, text );
  vsprintf( buf, text, arglist );
  va_end( arglist );
  Append(buf);
}

LogFile &LogFile::operator << ( int i )
{
  if (!IsOpen()) return *this;
  char buf[80];
  sprintf(buf,"%6d",i);
  Append(buf);
  return *this;
}


LogFile &LogFile::operator << ( float f )
{
  if (!IsOpen()) return *this;
  char buf[80];
  sprintf(buf,"%6.2f",f);
  Append(buf);
  return *this;
}

LogFile &LogFile::operator << ( const char *txt )
{
  Append(txt);
  return *this;
}


#if defined(_WIN32) 
  bool UseRDTSC = false;
  long long GetQPC()
  {
    LARGE_INTEGER count;
    ::QueryPerformanceCounter(&count);
    return count.QuadPart;
  }
#endif

inline __int64 GetCPUIDValue(int command)
{
  __asm
  {
    mov eax, command
    cpuid
  };
  // return value in edx:eax
}


#if _ENABLE_PERFLOG
static int ProfileScaleCalc()
{
  // TODOX360: ProfileScaleCalc may always return 1
#ifndef _WIN32
  return 1;
#else

  LARGE_INTEGER frequency;
  if (::QueryPerformanceFrequency(&frequency)==FALSE || frequency.QuadPart==0)
  {
    return 0;
  }

  volatile int qpcOuterStart = GetQPC();
  volatile int rdtscStart = GetCPUCycles();
  volatile int qpcInnerStart = GetQPC();
  __int64 freq = frequency.QuadPart;
  // verify the timer is monotonous accross all cores
  DWORD procAffinity,sysAffinity;
  GetProcessAffinityMask(GetCurrentProcess(),&procAffinity,&sysAffinity);
  const int repeat = 8;
  long long time[32*repeat];
  long long qpcTime[32*repeat];
  int times = 0;
  long long ms0p1 = to64bInt(freq*0.0001f);
  for (int r=0; r<repeat; r++)
  {
    for (int i=0; i<32; i++)
    {
      if (procAffinity&(1U<<i))
      {
        SetThreadAffinityMask(GetCurrentThread(),1U<<i);
        qpcTime[times] = GetQPC();
        time[times] = GetCPUCycles();
        times++;

        // wait for ~ 0.1 ms
        for (long long until = GetQPC()+ms0p1; GetQPC()<until; ){}
      }
    }
    if ((r&3)==0) Sleep(5); // make occasional Sleep to give speedstep a chance to slow down a CPU
  }
  SetThreadAffinityMask(GetCurrentThread(),procAffinity);
  volatile int qpcInnerEnd = GetQPC();
  volatile int rdtscEnd = GetCPUCycles();
  volatile int qpcOuterEnd = GetQPC();
  
  bool isGrowing = true;
  for (int i=1; i<times; i++)
  {
    if (time[i-1]-time[i]>0) // time[i-1]>time[i] not good, may overflow, which is OK
    {
      isGrowing = false;
    }
  }

  // estimate the RDTSC frequency and verify the rdtsc values are reasonably accurate
  long long qpcTimeWhole = qpcTime[times-1]-qpcTime[0];
  long long tscTimeWhole = time[times-1]-time[0];
  
  float tscFreq = float(tscTimeWhole)/qpcTimeWhole;
  float tscFreqMin = tscFreq;
  float tscFreqMax = tscFreq;
  for (int i=1; i<times; i++)
  {
    long long tscTimeI = time[i]-time[i-1];
    long long qpcTimeI = qpcTime[i]-qpcTime[i-1];
    float tscFreqI = float(tscTimeI)/qpcTimeI;
    saturateMin(tscFreqMin,tscFreqI);
    saturateMax(tscFreqMax,tscFreqI);
  }
  
  

  //LogF("QPC outer: %d, RDTSC: %d, QPC inner: %d",qpcOuterEnd-qpcOuterStart,rdtscEnd-rdtscStart,qpcInnerEnd-qpcInnerStart);
  
  // verify the frequency of PC matches rdtsc reasonably well
  if (qpcOuterEnd-qpcOuterStart>=(rdtscEnd-rdtscStart)/10*9 && (rdtscEnd-rdtscStart)/10*11>=qpcInnerEnd-qpcInnerStart)
  {
    UseRDTSC = isGrowing;
  }
  else if (isGrowing && tscFreqMin>=tscFreqMax*0.7f)
  {
    // verify the estimate using CPUID: Processor's support for invariant TSC is indicated by CPUID.80000007H:EDX[8].
    // see http://stackoverflow.com/questions/3388134/rdtsc-accuracy-across-cpu-cores/4145156#4145156
    // or http://stackoverflow.com/questions/4509727/how-to-detect-rdtsc-returns-a-costant-rate-counter-value
    long long val = GetCPUIDValue(0x80000007);
    if (val&(1ULL<<40) || tscFreqMin>=tscFreqMax*0.95f)
    {
      UseRDTSC = true;
      freq = to64bInt(frequency.QuadPart*tscFreq);
    }
  }
  
  __int64 scale = freq/100000/(1<<PROF_ACQ_SCALE_LOG);
  #if 0
    // check what time can we represent with given ACQ_SCALE_LOG
    __int64 acqFrequency = freq/(1<<PROF_ACQ_SCALE_LOG);
    // max. time we are able to represent safely is 31b
    float time = float(1U<<31)/acqFrequency;
    LogF("Max PROFILE_SCOPE time %.2f ms",time*1000);
  #endif
  // make sure we can represent 1 sec
  return int(scale);
#endif
}

/// profiler counter scale - should be calculated only once
/**
If any static variable initialization in init_seg(compiler)
needs to use profiling, it needs to make sure InitProfileScale is called first.
*/

static int ProfileScaleVal = ProfileScaleCalc();

void InitProfileScale()
{
  if (ProfileScaleVal!=0) return;
  ProfileScaleVal = ProfileScaleCalc();

}

int ProfileScale()
{
  return ProfileScaleVal;
}

DWORD ScopeProfiler::_mainThread = GetCurrentThreadId();

#endif

#ifndef _WIN32
#include <time.h>
__int64 ProfileTime()
{
  timespec time1;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
  return (__int64)(time1.tv_nsec) + (__int64)(1000000000)*(__int64)(time1.tv_sec);
}
#endif

#endif
