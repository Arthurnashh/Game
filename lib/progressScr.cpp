#include "wpch.hpp"

#include "progress.hpp"
#include "engine.hpp"
#include "global.hpp"
#include <El/Debugging/debugTrap.hpp>
#include "paramFileExt.hpp"
#include "world.hpp"
#include "txtPreload.hpp"
#include "scripts.hpp"

#include "mbcs.hpp"

#if _ENABLE_REPORT
static DWORD LastAccessTime;
static bool ProblemDetected = false;
bool CheckProgressRefresh = false;
#define REFRESH_FREQUENCY 250
#ifdef _WIN32
#include <El/Debugging/imexhnd.h>
#endif
#endif

/// display progress of long operation
class Progress: public ProgressHandle
{
  // progress bar status
  float _progressTot; // should be in sec (estimated) if possible
  float _progressCur; // current state (starts at 0)
  float _progressDrawn; // state drawn (0..1)

  /// stack implemented using back pointers
  LLink<ProgressHandle> _prev;
  
  DWORD _progressStartTime; // GetTickCount() when started
  DWORD _progressEndTime; // GetTickCount() until must be displayed 
  RString _progressTitle,_progressFormat;
  Ref<Font> _progressFont;
  float _progressFontSize;
  int _progressShadow;

  IProgressDisplayRef _progressDisplay;

  DWORD _lastProgressRefresh;
  DWORD _progressRefreshBase;

  bool _clear;
  bool _initialized;
  /// if some rendering frame was already open, different handling  is needed
  bool _finishDrawNeeded;

  public:
  Progress(ProgressHandle *prev);
  ~Progress();

  private:
  Progress( const Progress &src );
  const Progress &operator = ( const Progress &src );

  public:
  void Reset();
  void Add( float ammount ); // calculate total estimation
  void Advance( float ammount ); // really advance

  void SetPassed( float value ); // set state 
  void SetRest( float value ); // set state 

  // execute in pairs - no nesting allowed
  void Start(RString title, RString format="%s", DWORD delay = 0, DWORD duration = 0);
  void Start(RString title, RString format, Ref<Font> font, float size=-1.0, int shadow = 0, DWORD delay = 0, DWORD duration = 0);
  void Start(IProgressDisplay *display, bool forced, DWORD delay = 0, DWORD duration = 0);
  void Finish();
  void Draw();

  IMemoryFreeOnDemand *GetFreeOnDemandInterface() const;
  
  void Frame(); // FinishDraw // Draw // InitDraw
  void Refresh(); // Frame (if neccessary)FinishDraw // ProgressDraw // InitDraw

  bool Active() const;

  void DisableClear();

protected:
  void FrameNoClearInit();
  void DrawFrame(); // FinishDraw // Draw // InitDraw
  void DoDraw();
};


// progress bar

const Color ProgressBackground = HBlack;

Ref<Script> ProgressScript;

Progress::Progress(ProgressHandle *parent)
{
  _lastProgressRefresh = 0;
  Reset();
  _prev = parent;
  _finishDrawNeeded = !parent && !GEngine->InitDrawDone();
}

Progress::~Progress()
{
  if (_finishDrawNeeded && GEngine->InitDrawDone())
  {
    Fail("Should not be reached");
    if (_initialized) Draw();
    // we were not in the middle of the frame, we should not leave the frame open
    GEngine->FinishDraw(1,true);
    GEngine->NextFrame(true);
    GEngine->FinishPresent();
    GEngine->FinishFinishPresent();
  }
  if (this==GProgress)
  {
    GProgress = _prev;
  }
}

void Progress::Reset()
{
  _progressTot=0;
  _progressCur=0;
  _progressDrawn=0;

  _clear = true;
  _initialized = false;
}

void Progress::Add( float ammount )
{
  // calculate total estimation
  _progressTot+=ammount;
}

void Progress::Advance( float ammount )
{
  // really advance - change display
  _progressCur+=ammount;  
}
void Progress::SetPassed( float value )
{
  // really advance - change display
  _progressCur=value; 
}
void Progress::SetRest( float value )
{
  // really advance - change display
  _progressCur=_progressTot-value;  
}

void Progress::DisableClear()
{
  _clear = false;
}



bool Progress::Active() const
{
  return GEngine->TextBank() &&
    (_progressTitle.GetLength() > 0 || _progressDisplay);
}


void Progress::Draw()
{
  if (!_initialized)
  {
    if (GlobalTickCount() >= _progressStartTime)
    {
      // fill both back and front buffer with new content
      if (_clear)
        DrawFrame();
      else
        FrameNoClearInit();
      DrawFrame();
      DrawFrame();

      _initialized = true;
    }
  }
  if (_initialized) DoDraw();
}

void Progress::DoDraw()
{
  if (GEngine->IsAbleToDraw(false) == ATDUnable) return;

  static DWORD lastTime;
  DWORD time = ::GlobalTickCount();
  int deltaMs = time-lastTime;
  lastTime = time;

  if (deltaMs>300) deltaMs=300;
  float deltaT = deltaMs*0.001f;

  if (ProgressScript)
  {

    /*
    if (_progressTot>0)
    {
      float factor=_progressCur/_progressTot;
      GlobalShowMessage(100,"factor %.3f",factor);
    }
    */

    if (ProgressScript->Simulate(deltaT))
    {
      //ProgressScript.Free();
    }
    
    // draw title and cut effects (if any)
    TitleEffect *tit = GWorld->GetTitleEffect();
    if (tit)
    {
      tit->Simulate(deltaT);
      tit->Draw();
      if (tit->IsTerminated()) GWorld->SetTitleEffect(NULL);
    }

    for (int i=0; i<GWorld->GetCutEffects().Size(); i++)
    {
      TitleEffect *effect = GWorld->GetCutEffects()[i];
      if (!effect) continue;
      effect->Simulate(deltaT);
      effect->Draw();
      if (effect->IsTerminated()) GWorld->SetCutEffect(i, NULL);
    }

    if( GEngine->TextBank() && _progressTot>0 )
    {
      float factor=_progressCur/_progressTot;
      _progressDrawn=factor;

    }

    GDebugger.NextAliveExpected(15*60*1000); // no alive expected
  }
  else
  {
    // TODO: offline font loading
    // draw activity indicator

    if (_progressTot > 0 && ((_progressTitle.GetLength()>0 && GEngine->TextBank()) || _progressDisplay))
      _progressDrawn = _progressCur/_progressTot;
    
    if( _progressTitle.GetLength()>0 && GEngine->TextBank() )
    {
      DWORD time = ::GlobalTickCount()-_progressRefreshBase;
      float factor = fastFmod(time*(1.0f/3000),2);
      if (factor>0.001)
      {
        Draw2DPars pars;
        Texture *texture = GPreloadedTextures.New(TextureWhite);
        pars.mip=GEngine->TextBank()->UseMipmap(texture,0,0);
        pars.SetU(0,1);
        pars.SetV(0,1);
        pars.spec = NoZBuf|IsAlpha|IsAlphaFog|NoClamp;
        float h=GEngine->Height2D()*0.01;
        float w=GEngine->Width2D()*0.3;
        float ww=w*0.05;
        float x=GEngine->Width2D()*0.5-w*0.5;
        float y=GEngine->Height2D()*0.60-h*0.5;
        
        if (factor>1) factor=2-factor;
        static PackedColor color(Color(0.1,0.1,0.05));
        Rect2DPixel rectA(x+(w-ww)*factor,y,ww,h);
        pars.SetColor(color);
        GEngine->Draw2D(pars,rectA);
      }

      GDebugger.NextAliveExpected(15*60*1000); // no alive expected
      Assert( _progressFont );
      float textW=GEngine->GetTextWidth
      (
        _progressFontSize,_progressFont,_progressTitle
      );
      GEngine->DrawText
      (
        Point2DFloat(0.5-textW*0.5,0.5),
        _progressFontSize,_progressFont,PackedColor((unsigned)-1),_progressTitle, _progressShadow
      );
      Draw2DPars pars;
      if( GEngine->TextBank() && _progressTot>0 )
      {
        Texture *texture=GPreloadedTextures.New(TextureWhite);
        pars.mip=GEngine->TextBank()->UseMipmap(texture,0,0);
        pars.SetU(0,1);
        pars.SetV(0,1);
        pars.spec = NoZBuf|IsAlpha|IsAlphaFog|NoClamp;
        pars.spec|= (_progressShadow == 2)?UISHADOW : 0; 
        
        float h=GEngine->Height2D()*0.01;
        float w=GEngine->Width2D()*0.3;
        float x=GEngine->Width2D()*0.5-w*0.5;
        float y=GEngine->Height2D()*0.57-h*0.5;       

        Rect2DPixel rect(x,y,w,h);
        static PackedColor white(Color(1,1,1));
        static PackedColor gray(Color(0.1,0.1,0.1));
        pars.SetColor(gray);
        GEngine->Draw2D(pars,rect);

        Rect2DPixel rectA(x,y,w*_progressDrawn,h);
        pars.SetColor(white);
        GEngine->Draw2D(pars,rectA);

        //GEngine->TextBank()->ReleaseMipmap();
      }
    }

    if (_progressDisplay)
    {         
      _progressDisplay->SetProgress(::GlobalTickCount() - _progressRefreshBase, _progressCur, _progressTot);
      _progressDisplay->DrawProgress(1);
    }

  } 
}

void ProcessMessagesNoWait();

IMemoryFreeOnDemand *Progress::GetFreeOnDemandInterface() const
{
  if (GEngine && (_progressTitle.GetLength() > 0 || _progressDisplay || ProgressScript) && GEngine->TextBank())
  {
    return GEngine->TextBank()->GetProgressFreeOnDemandInterface(); // TexCache interface is the only one on this level
  }
  return NULL;  
}

void Progress::FrameNoClearInit()
{
  if (GSoundsys) GSoundsys->Commit();
  // close current frame
  if (GEngine->InitDrawDone())
  {
    DoDraw();
    GEngine->FinishDraw(1,true);
    GEngine->NextFrame(true);
    GEngine->FinishPresent();
    GEngine->FinishFinishPresent();
  }
  else
  {
    // copy it to all buffers
    GEngine->BackBufferToAllBuffers();
  }
  
  if (GEngine->IsAbleToDraw(false) != ATDUnable)
  {
    GEngine->InitDraw(_clear, ProgressBackground);
  }
  ProcessMessagesNoWait();
}

void Progress::DrawFrame()
{
  if (GEngine->InitDrawDone())
  {
    GEngine->NoPostprocess();
    DoDraw();
    GEngine->FinishDraw(1);
    GEngine->NextFrame(true);
    GEngine->FinishPresent();
    GEngine->FinishFinishPresent();
  }
  if (GEngine->IsAbleToDraw(false) != ATDUnable)
  {
    GEngine->InitDraw(_clear, ProgressBackground);
  }
}

void Progress::Frame()
{
  if (GSoundsys) GSoundsys->Commit();
  if (!_initialized)
  {
    if (GlobalTickCount() >= _progressStartTime)
    {
      // fill both back and front buffer with new content
      if (_clear)
        DrawFrame();
      else
        FrameNoClearInit();
      DrawFrame();
      DrawFrame();
  
      _initialized = true;
    }
  }
  if (_initialized) DrawFrame();
  ProcessMessagesNoWait();
}

TypeIsSimple(AtAliveFunction *);

class GlobalAliveImplementation : GlobalAliveInterface
{
  FindArray<AtAliveFunction *> _functions;
  
public:
  GlobalAliveImplementation() {Set(this);}
  virtual void Alive();
  
  void AtAlive(AtAliveFunction *function) {_functions.AddUnique(function);}
  void CancelAtAlive(AtAliveFunction *function) {_functions.Delete(function);}
};

static GlobalAliveImplementation GGlobalAliveImplementation;

LLink<ProgressHandle> GProgress;
LLink<ProgressHandle> GFirstProgress;

void GlobalAliveImplementation::Alive()
{
#if _ENABLE_DEDICATED_SERVER
  if ( IsDedicatedServer() )
  {
    static DWORD LastAliveTickCount = ::GetTickCount();
    const int GAInterval = 1000/50; //50Hz
    DWORD curTime = ::GetTickCount();
    if ( curTime - LastAliveTickCount < GAInterval )
      return; //process GlobalAlive in 50Hz tempo (Note: was called over 60000 times per second on Linux dedicated servers, causing extremely slow startup sometimes)
    LastAliveTickCount = curTime; //50Hz
  }
#endif
  for (int i=0; i<_functions.Size(); i++)
  {
    _functions[i]();
  }
  // report we are still active
  GDebugger.ProcessAlive();
  //if (GProgress) GProgress->Refresh();
}


void Progress::Refresh()
{
#if _ENABLE_REPORT
  DWORD now = GlobalTickCount();
  if (CheckProgressRefresh && now >= LastAccessTime + REFRESH_FREQUENCY)
  {
    if (!ProblemDetected)
    {
      RString msg = Format("ProgressRefresh not called for %d ms", now - LastAccessTime);
      if (GDebugger.IsDebugger())
      {
        LogF(msg);
#ifdef _WIN32
        FailHook("break");
#endif
      }
      else
      {
#ifdef _WIN32
        DebugExceptionTrap::LogFFF(msg);
#endif
      }
    }
    ProblemDetected = !ProblemDetected;
  } else ProblemDetected = false;
  LastAccessTime = now;
#endif

#if _ENABLE_DEDICATED_SERVER
  if (IsDedicatedServer())
  {
    I_AM_ALIVE();
    return;
  }
#endif
  if (!GEngine || !GEngine->TextBank())
  {
    I_AM_ALIVE();
    return;
  }
  if (_progressTitle.GetLength()<=0 && !_progressDisplay && !ProgressScript)
  {
    I_AM_ALIVE();
    return;
  }
  // some progress information is displayed

  // do not update too often
  DWORD cTime=GlobalTickCount();
  DWORD minTime = 100; // max 10 per sec
  float changeRel = _progressTot>0 ? fabs(_progressDrawn*_progressTot-_progressCur) : 0;
  if( changeRel>0.01*_progressTot ) 
  {
    if (GEngine->CanVSyncTime())
    {
      // do not go faster than presentation interval allows
      minTime = 1000/GEngine->RefreshRate()*(GEngine->GetPresentationIntervalVSync()+1);
    }
    else
    {
      // do not go faster than 20 fps
      minTime = 50;
    }
  }
  Assert(cTime>=_lastProgressRefresh);
  if( cTime<_lastProgressRefresh+minTime )
  {
    I_AM_ALIVE();
    return;
  }
  _lastProgressRefresh=cTime;
  Frame();
#if _ENABLE_REPORT
  LastAccessTime = GlobalTickCount();
#endif
}

// execute in pairs - no nesting allowed
void Progress::Start(RString title, RString format, DWORD delay, DWORD duration)
{
//  if (GWorld) GWorld->DisableUserInput(true);

  Start(title, format, NULL, -1, delay, duration);
}

void Progress::Start( RString title, RString format, Ref<Font> font, float size, int shadow, DWORD delay, DWORD duration)
{
  GDebugger.NextAliveExpected(15*60*1000); // no alive expected
  _progressStartTime = GlobalTickCount() + delay;
  _progressEndTime = GlobalTickCount() + duration;

#if _ENABLE_DEDICATED_SERVER
  if (IsDedicatedServer())
  {
    DisableClear();
    return;
  }
#endif
//  if (GWorld) GWorld->DisableUserInput(true);

  //Assert( _progressTitle.GetLength()==0 );
  //Assert( _progressFormat.GetLength()==0 );
  _progressTitle=title;
  _progressFormat=format;
  _progressCur=0;
  if (font)
    _progressFont = font;
  else
  {
    ParamEntryVal fontPars = Pars >> "CfgInGameUI" >> "ProgressFont";
    _progressFont = GEngine->LoadFont(GetFontID(fontPars >> "font"));
  }
  if (size >= 0)
  {
    _progressFontSize = size;
    _progressShadow = shadow;
  }
  else
  {
    ParamEntryVal fontPars = Pars >> "CfgInGameUI" >> "ProgressFont";
    _progressFontSize = fontPars >> "size";
    _progressShadow = fontPars >> "shadow";
  }

  _progressRefreshBase=GlobalTickCount();
}

/*!
\patch 5164 Date 6/22/2007 by Ondra
- Fixed: Memory leak on dedicated server (several KB per a mission played).
*/
void Progress::Start(IProgressDisplay *display, bool forced, DWORD delay, DWORD duration)
{
  GDebugger.NextAliveExpected(15*60*1000); // no alive expected
  _progressStartTime = GlobalTickCount() + delay;
  _progressEndTime = GlobalTickCount() + duration;

#if _ENABLE_DEDICATED_SERVER
  if (IsDedicatedServer() && !forced)
  {
    // we need to take ownership to destroy the object
    IProgressDisplayRef destroy = display;    
    DisableClear();
    return;
  }
#endif
//  if (GWorld) GWorld->DisableUserInput(true);

  _progressDisplay = display;
  _progressCur=0;
  _progressTitle=RString();
  _progressFormat=RString();
//  GWorld->EnableDisplay(false);
}

/*!
  \patch 5149 Date 3/26/2007 by Flyman
  - Fixed: Progress bar was too bright at the end of the loading for a little while
*/
void Progress::Finish()
{
#if _ENABLE_REPORT
  DWORD now = GlobalTickCount();
  if (CheckProgressRefresh && now >= LastAccessTime + REFRESH_FREQUENCY)
  {
    if (!ProblemDetected)
    {
      RString msg = Format("ProgressRefresh not called for %d ms", now - LastAccessTime);
      if (GDebugger.IsDebugger())
      {
        LogF(msg);
#ifdef _WIN32
        FailHook("break");
#endif
      }
      else
      {
#ifdef _WIN32
        DebugExceptionTrap::LogFFF(msg);
#endif
      }
    }
    ProblemDetected = !ProblemDetected;
  } else ProblemDetected = false;
#endif

  while (::GlobalTickCount() < _progressEndTime)
  {
    ProgressRefresh();
  }

  if (_finishDrawNeeded && GEngine->InitDrawDone())
  {
    if (_initialized)
    {
      GEngine->NoPostprocess();
      DoDraw();
    }
    // we were not in the middle of the frame, we should not leave the frame open
    GEngine->FinishDraw(1,true);
    GEngine->NextFrame(true);
    GEngine->FinishPresent();
    GEngine->FinishFinishPresent();
  }
  
//  GWorld->EnableDisplay(true);
  _progressTitle=RString();
  _progressFormat=RString();
  _progressDisplay = NULL;

  _clear = true;

//  if (GWorld) GWorld->DisableUserInput(false);
}

IMemoryFreeOnDemand *ProgressGetFreeOnDemandInterface()
{
  if (GProgress.NotNull()) return GProgress->GetFreeOnDemandInterface();
  return NULL;
}

IProgressDisplay *CreateDisplayProgress(RString text);

void ProgressStartFinish(Progress *p, bool noClear)
{
  if (!GFirstProgress)
  {
    GFirstProgress = p;
    p->Frame();
#if _ENABLE_REPORT
    LastAccessTime = GlobalTickCount();
#endif
  }
}

Ref<ProgressHandle> ProgressStartExt(bool noClear, RString title, RString format, bool forced, DWORD delay, DWORD duration)
{
  // engine is needed to show the progress
  if (!GEngine) return NULL;

  Progress *p = new Progress(GProgress);
  if (noClear) p->DisableClear();
  GProgress = p;
  p->Start(CreateDisplayProgress(title), forced, delay, duration);
  ProgressStartFinish(p,noClear);
  return p;
}
Ref<ProgressHandle> ProgressStartExt(bool noClear, RString title, RString format, Ref<Font> font, float size, bool forced, DWORD delay, DWORD duration)
{
  // engine is needed to show the progress
  if (!GEngine) return NULL;

  Progress *p = new Progress(GProgress);
  if (noClear) p->DisableClear();
  GProgress = p;
  // p->Start(title,format,font,size);
  p->Start(CreateDisplayProgress(title), forced, delay, duration);
  ProgressStartFinish(p,noClear);
  return p;
}
Ref<ProgressHandle> ProgressStartExt(bool noClear, IProgressDisplay *display, bool forced, DWORD delay, DWORD duration)
{
  // engine is needed to show the progress
  if (!GEngine) return NULL;

  Progress *p = new Progress(GProgress);
  if (noClear) p->DisableClear();
  GProgress = p;
  p->Start(display, forced, delay, duration);
  ProgressStartFinish(p,noClear);
  return p;
}

void ProgressRefreshHack()
{
  if (GFirstProgress.NotNull()) GFirstProgress->Refresh();
}
