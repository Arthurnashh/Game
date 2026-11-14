#ifndef _PROGRESS_BAR_H_BREDY_INCLUDED_
#define _PROGRESS_BAR_H_BREDY_INCLUDED_
#pragma once

#include <stdarg.h>

/// ProgressBar base class
class ProgressBarBase
{
friend class ProgressBarFunctions;
protected:
  ProgressBarBase *_nextProgress;
  int _signal;

  ProgressBarBase() {_nextProgress=0;_signal=0;}
  void Append(ProgressBarBase *what)
  {    
    ProgressBarBase *l=this;;
    while (l->_nextProgress) l=l->_nextProgress;
    l->_nextProgress=what;
    what->_signal=l->_signal;
  }

  void Release(ProgressBarBase *what)
  {
    ProgressBarBase *l=this;
    while (l && l->_nextProgress!=what) l=l->_nextProgress;
    if (l) {l->_nextProgress=0;l->Detach();}
  }
  
  virtual void Detach()=0;  
  
  ProgressBarBase *GetLevel(int level)
  {
    if (level==0) return this;
    if (_nextProgress==0) return 0;
    return _nextProgress->GetLevel(level-1);    
  }

  void Signal(int signal)
  {
    ProgressBarBase *l=this;
    while (l ) 
    {
      l->_signal=signal;
      l=l->_nextProgress;
    }
  }
public:
  int GetSignal() {return _signal;}  
  virtual float ReadResult()=0;
};

class ProgressBarFunctions
{
ProgressBarBase *_base;

public:

  ProgressBarBase *GetTopProgressBar() {return _base;}
  void SetTopProgressBar(ProgressBarBase *p) {_base=p;}

  void Add(ProgressBarBase *p)
  {
    if (_base==0) _base=p;
    else _base->Append(p);      
  }

  void Remove(ProgressBarBase *p)
  {
    if (_base==0 || _base==p) 
        _base=0;
    else _base->Release(p);
  }

  ProgressBarFunctions() {_base=0;}
  virtual ~ProgressBarFunctions() {}

  ///Requests lock, during progress bar is created or destroyed
  virtual void Lock() {}
  ///Requests unlock, after progress bar is created or destroyed
  virtual void Unlock() {}

  ///Function returns true, when progress bar must be manually updated.
  /** Progress bar can be updated manually or automatically. Automatically update
  is done by another thread, that rendering progress bar in UI. Thread reading
  progress bar value and drawing result. Manually update is done by working thread
  by calling Update function everytime progress changes
  */
  virtual bool ManualUpdate() {return false;}

  ///Updates progress bar
  /** Function is implemented by UI. UI must read values of progress bar
  and visualise result
  */
  virtual void Update() {}

  ///Function reads result from progress
  /** Function returns value between 0 - 1. If result is negative, 
  progress bars value is unknown.
  */
  float ReadResult(int level=0) 
    {
      ProgressBarBase *p=_base;
      if (level && p) p=p->GetLevel(level);
      if (!p) return -1.0f;
      return p->ReadResult();
    }

  bool InProgress() {return _base!=0;}

  ///Function is called everytime state of progress changes    
  ///This function is called by ProgressBar::ReportState(...)
  virtual void ReportState(int messageId, va_list params) {}

  
  enum SignalStopEnum {enSignalStop=1};

  //Signals the process. Process can read signal using GetSignal
  void Signal(int signal)
  {
    Lock();
    _base->Signal(signal);
    Unlock();
  }

  void SignalStop() {Signal(enSignalStop);}

  static ProgressBarFunctions *SelectGlobalProgressBarHandler(ProgressBarFunctions *newHandler);

  static ProgressBarFunctions *GetGlobalProgressBarInstance();

  int Length()
  {
    int len=0;
    ProgressBarBase *l=_base;;
    while (l) {l=l->_nextProgress;len++;}
    return len;

  }
};


template<class T=int>
class ProgressBar:public ProgressBarBase
{
  T _curPos;
  T _nextStep;
  T _max;
  bool _update;
  ProgressBarFunctions *_pbFunc;

  void DoInit(ProgressBarFunctions *pbfunc)
  {
    _pbFunc=pbfunc;
    _pbFunc->Lock();    
    pbfunc->Add(this);
    _update=pbfunc->ManualUpdate();
    _pbFunc->Unlock();
  }
public:
  ProgressBar(T max,ProgressBarFunctions *pbfunc=0);


  ~ProgressBar()
  {
    _pbFunc->Lock();
    _pbFunc->Remove(this);
    _pbFunc->Unlock();
  }
  
  
  float ToFloat(T val);

  virtual float ReadResult()
  {
    float curPos=ToFloat(_curPos);
    float nextStep=ToFloat(_nextStep);
    float max=ToFloat(_max);
    if (curPos<nextStep && _nextProgress)
    { 
      float mid=_nextProgress->ReadResult();
      curPos+=mid*(nextStep-curPos);
    }
    if (curPos<0) curPos=0;
    else if (curPos>=max) curPos=1;
    else if (max==0) return 0;
      else curPos=curPos/max;
    return curPos;
  }
  virtual void Detach()
  {
    _curPos=_nextStep;
  }


  ///Changes maximum of progress bar
  void SetMax(T max) {_max=max;if (_update) _pbFunc->Update();}
  ///Sets position for next piece of work.
  /** It is recomended to using this function in recursive operations.
  If we know, how big part of progress will be processed during next step,
  use this function to set this position before progress processed.
  
  ProgressBar will use this value as hint to calculate progres of included
  progressbars.
  */
  void SetNextPos(T pos) {_curPos=_nextStep;_nextStep=pos;if (_update) _pbFunc->Update();}
  
  /// Functions sets current pos of progress
  /** It is better to use SetNextPos when we know how big part of progress will
  be processed. Especially in recursive alg.
  */
  void SetPos(T pos) {_curPos=_nextStep=pos;if (_update) _pbFunc->Update();}

  /// Advances for next operation
  /** Advances progressbar during next operation. Function
  doesn't advance progress now, but after operation is completted */
  void AdvanceNext(T pos) {SetNextPos(_nextStep+pos);if (_update) _pbFunc->Update();}
  /// Advances progress bar
  /** Advances progress bar now. This function cannot use included progress bars */
  void AdvanceCur(T pos) {SetPos(_curPos+pos);if (_update) _pbFunc->Update();}

  ///Function returns current progress position
  T GetPos() {return _curPos;}

  T GetMax() {return _max;}

  bool StopSignaled() {return GetSignal()==ProgressBarFunctions::enSignalStop;}

  void ReportState(int messageId, ...)
  {
    va_list args;
    va_start(args,messageId);
    ReportStateV(messageId,args);
  }

  void ReportStateV(int messageId, va_list args)
  {
    _pbFunc->ReportState(messageId,args);
  }

};

template <class T>
ProgressBar<T>::ProgressBar(T max,ProgressBarFunctions *pbfunc/* =0 */)
{
  _max=max;
  _nextStep=0;
  _curPos=0;
  DoInit(pbfunc==NULL?ProgressBarFunctions::GetGlobalProgressBarInstance():pbfunc);
  if (_update) _pbFunc->Update();
}


template <class T>
float ProgressBar<T>::ToFloat(T val)
{
  return (float)val;
}








#endif