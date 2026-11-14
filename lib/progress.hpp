#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PROGRESS_HPP
#define _PROGRESS_HPP

#include <El/Time/time.hpp>
#include <Es/Types/lLinks.hpp>
#include "font.hpp"

class IMemoryFreeOnDemand;

/// interface - display progress of long operation

class ProgressHandle: public RemoveLLinks
{
  public:
	virtual void Add( float ammount ) = 0; // calculate total estimation
	virtual void Advance( float ammount ) = 0; // really advance
	virtual void Frame() = 0; // FinishDraw // Draw // InitDraw
	virtual void Refresh() = 0; // Frame (if neccessary)FinishDraw // ProgressDraw // InitDraw

	virtual void Reset() = 0;
	virtual void Draw() = 0;
	virtual void Finish() = 0;
	
	virtual void SetPassed( float value ) = 0; // set state 
	virtual void SetRest( float value ) = 0; // set state 

	virtual bool Active() const = 0;
	virtual void DisableClear() = 0;
  virtual IMemoryFreeOnDemand *GetFreeOnDemandInterface() const = 0;
};

extern LLink<ProgressHandle> GProgress;
extern LLink<ProgressHandle> GFirstProgress;

/// interface to display passed to Progress
class IProgressDisplay
{
public:
  virtual ~IProgressDisplay() {}
  virtual void SetProgress(DWORD time, DWORD current, DWORD total) = 0;
  virtual void DrawProgress(float alpha) = 0;

  // reference counting
  virtual int DisplayAddRef() = 0;
  virtual int DisplayRelease() = 0;
};

struct IProgressDisplayRefTraits
{
  static __forceinline int AddRef(IProgressDisplay *ptr) {return ptr->DisplayAddRef();}
  static __forceinline int Release(IProgressDisplay *ptr) {return ptr->DisplayRelease();}
};

typedef RefR<IProgressDisplay, IProgressDisplay, IProgressDisplayRefTraits> IProgressDisplayRef;

#define PROGRESS_DEFAULT_DELAY 250

Ref<ProgressHandle> ProgressStartExt(bool noClear, RString title, RString format="%s", bool forced = false, DWORD delay = PROGRESS_DEFAULT_DELAY, DWORD duration = 0);
Ref<ProgressHandle> ProgressStartExt(bool noClear, RString title, RString format, Ref<Font> font, float size=-1.0, bool forced = false, DWORD delay = PROGRESS_DEFAULT_DELAY, DWORD duration = 0);
Ref<ProgressHandle> ProgressStartExt(bool noClear, IProgressDisplay *display, bool forced = false, DWORD delay = 0, DWORD duration = 0);

inline Ref<ProgressHandle> ProgressStart(RString title, RString format="%s", bool forced = false, DWORD delay = PROGRESS_DEFAULT_DELAY, DWORD duration = 0)
{
  return ProgressStartExt(false,title,format,forced,delay,duration);
}
inline Ref<ProgressHandle> ProgressStart(RString title, RString format, Ref<Font> font, float size=-1.0, bool forced = false, DWORD delay = PROGRESS_DEFAULT_DELAY, DWORD duration = 0)
{
  return ProgressStartExt(false,title,format,font,size,forced,delay,duration);
}
inline Ref<ProgressHandle> ProgressStart(IProgressDisplay *display, bool forced = false, DWORD delay = 0, DWORD duration = 0)
{
  return ProgressStartExt(false,display,forced,delay,duration);
}

// global progress access
inline void ProgressReset() {}

inline void ProgressAdd( float ammount ) {if (GProgress.NotNull()) GProgress->Add(ammount);}
inline void ProgressAdvance( float ammount ) {if (GFirstProgress.NotNull()) GFirstProgress->Advance(ammount);}

inline bool ProgressIsActive()
{
  return GFirstProgress.NotNull() && GFirstProgress->Active();
}
inline void ProgressFinish(Ref<ProgressHandle> &p)
{
  if (p.IsNull()) return;
  DoAssert(p==GProgress);
  p->Finish();
  p.Free();
}

inline void ProgressDraw() {if (GFirstProgress.NotNull()) GFirstProgress->Draw();}

inline void ProgressFrame() {if (GFirstProgress.NotNull()) GFirstProgress->Frame();}
inline void ProgressRefresh() {if (GFirstProgress.NotNull()) GFirstProgress->Refresh();}

/// Progress bar valid in the current scope
class ProgressScope
{
protected:
  Ref<ProgressHandle> _handle;

public:
  ProgressScope(bool noClear, RString title, RString format, bool forced = false, DWORD delay = 0, DWORD duration = 0)
  {
    _handle = ProgressStartExt(noClear, title, format, forced, delay, duration);
  }
  ~ProgressScope()
  {
    if (_handle) ProgressFinish(_handle);
  }
  void Refresh()
  {
    if (_handle) ProgressRefresh();
  }
  void Finish()
  {
    if (_handle)
    {
      ProgressFinish(_handle);
      _handle = NULL;
    }
  }
};

#endif
