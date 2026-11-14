#include "wpch.hpp"
#include "engine.hpp"
#include "font.hpp"
#include "textbank.hpp"
#include "tlVertex.hpp"
#include "global.hpp"
#include "scene.hpp"
#include "keyInput.hpp"
#include "paramFileExt.hpp"
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>
#include <Es/Strings/bString.hpp>
#include "appFrameExt.hpp"
#include "dikCodes.h"
#include <Es/Algorithms/qsort.hpp>
#include "fileLocator.hpp"
#include <El/Evaluator/express.hpp>
#include "Es/Algorithms/splineNEqD.hpp"

#include <stdarg.h>

#include "mbcs.hpp"
#include <Es/Common/win.h>
#include "baseWorld/enf_shapes.h"

/*!
\patch_internal 1.82 Date 8/22/2002 by Ondra
- Improved: Performance counters that are zero disappear automatically on screen.
*/

#if _ENABLE_PERFLOG
extern size_t GetMemoryUsedSize();
extern size_t GetMemoryCommitedSize();
#endif

#define _ENABLE_PERFLOG_DRAW_PREFIX 0 

/**
We can use this as an opportunity for periodic memory cleanup.
*/
void Engine::FlushMemory(bool deep, bool allowAutoDeep)
{
  TextBank()->FlushTextures(deep,allowAutoDeep);
}

void Engine::RequestFlushMemory()
{
}


DEFINE_ENUM(ShowFps,Fps,SHOW_FPS_ENUM)

void Engine::ToggleFps( int state )
{
  // cycle
  const int maxFpsMode = NShowFps-1;
  if (state>maxFpsMode) state = 0;
  if (state<0) state = maxFpsMode;
  _showFps=state;
}

void Engine::StatsPage(int pages)
{
  #if _ENABLE_CHEATS
  _statPageStart += pages;
  if (_statPageStart<-1) _statPageStart = -1;
  if (_statPageStart>99) _statPageStart = 99;
  #endif
}

#if _ENABLE_PERFLOG && _ENABLE_CHEATS

static int CompareSlots(const int *s1, const int *s2, const PerfCounters *counters)
{
  const PerfCounterSlot *slot1 = counters->Slot(*s1);
  const PerfCounterSlot *slot2 = counters->Slot(*s2);
  int d = slot2->smoothValue-slot1->smoothValue;
  //int d = slot2->slowValue/slot2->scale-slot1->slowValue/slot1->scale;
  if (d) return d;
  d = *s1-*s2;
  return d;
}

static int CompareSlotsByName(const int *s1, const int *s2, const PerfCounters *counters)
{
  const PerfCounterSlot *slot1 = counters->Slot(*s1);
  const PerfCounterSlot *slot2 = counters->Slot(*s2);
  int d = strcmpi(slot1->_name,slot2->_name);
  return d;
}

#include <El/Evaluator/expressImpl.hpp>

static RString StatFilter;
static GameValue DebugFilterStat(const GameState *state, GameValuePar oper1)
{
  RString id = oper1;
  StatFilter = id;
  GlobalShowMessage(100,"Stat filter '%s'",cc_cast(StatFilter));
  return GameValue();
}

static GameValue DiagTiming(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  RString id = oper1;
  bool enable = oper2;
  GPerfProfilers.EnableCategory(id, enable);
  int enableNum = enable ? 1 : 0;
  GlobalShowMessage(100, "Diag timing '%s' set to %d", cc_cast(id), enableNum);
  return GameValue();
}

#include <El/Modules/modules.hpp>

static const GameOperator DbgBinary[]=
{
  //GameOperator(GameScalar,"diag_stat",function,DebugFlushMem,GameString,GameScalar),
  GameOperator(GameNothing, "diag_timing", function, DiagTiming, GameString, GameBool TODO_OPERATOR_DOCUMENTATION),
};

static const GameFunction DbgUnary[]=
{
  GameFunction(GameNothing,"diag_stat",DebugFilterStat,GameString TODO_FUNCTION_DOCUMENTATION),
};

INIT_MODULE(EngineDrawDbgObj, 3)
{
  GGameState.NewOperators(DbgBinary,lenof(DbgBinary));
  GGameState.NewFunctions(DbgUnary,lenof(DbgUnary));
};

#endif

#define DRAW_TEXT(xi,yi,attrText,format,...) \
  { \
    Point2DAbs pos; \
    GEngine->Convert(pos, Point2DFloat(xi, yi)); \
    const float yso=0.001,xso=0.001; \
    float xs = toInt(wScreen * xso); saturateMax(xs, 1); \
    float ys = toInt(hScreen * yso); saturateMax(ys, 1); \
    DrawTextF(Point2DAbs(pos.x+xs,pos.y+ys),ctx.attrShadow,format,__VA_ARGS__ ); \
    DrawTextF(pos,attrText,format,__VA_ARGS__ ); \
  }

#if _ENABLE_CHEATS && _ENABLE_PERFLOG
  #include <El/Statistics/statistics.hpp>
  extern StatisticsByName DPrimStats;
  extern StatisticsByName TrisStats;
  extern StatisticsByName ObjsStats;
  extern StatisticsByName PixelStats;
#endif

struct Engine::PerfDiagsContext
{
  float xName;
  float yName;
  float x1;
  float x2;
  float x3;
  #if _ENABLE_PERFLOG && _ENABLE_CHEATS
  float x4;
  float x5;
  #endif
  float size;
  TextDrawAttr attrShadow;
  TextDrawAttr attrText;
  TextDrawAttr attrHigh;
  
  PerfDiagsContext(TextDrawAttr s, TextDrawAttr t, TextDrawAttr h)
  :attrShadow(s),attrText(t),attrHigh(h){}
};

bool Engine::CheckCheatMouse(float &mouseX, float &mouseY, bool &cursor)
{
  // is the mouse cursor controlling the cheats?
  cursor = false;
  #if _ENABLE_CHEATS
  // when Multicore diags are active, we want to change the mouse cursor optionally so that we can control the cheats
  if (GInput.keys[DIK_RWIN])
  {
    ParamEntryVal cls = Pars >> "CfgWrapperUI" >> "Cursors" >> "Debug";
    Ref<Texture> texture= GlobPreloadTexture(GetPictureName(cls >> "texture"));
    int hsX = cls >> "hotspotX";
    int hsY = cls >> "hotspotY";
    //int mw = cls >> "width";
    //int mh = cls >> "height";
    // we want the debugging cursor to have a distinct shape and color
    PackedColor color = GetPackedColor(cls >> "color");

    float mouseScrX = GInput.cursorX * 0.5f + 0.5f;
    float mouseScrY = GInput.cursorY * 0.5f + 0.5f;
    if (GEngine->IsCursorSupported())
    {
      int wScreen = GEngine->Width2D();
      int hScreen = GEngine->Height2D();
      int mx = toInt(mouseScrX*wScreen);
      int my = toInt(mouseScrY*hScreen);
      GEngine->SetCursor(texture, mx, my, hsX, hsY, color, 0);
    }


    // prevent mouse inputs to be read by the game - we want them to be sent to cheat processing only
    bool button = GInput.GetInputToDo(INPUT_DEVICE_MOUSE,true,false,true,255);
    
    GInput.CheckInput(INPUT_DEVICE_MOUSE,false,true,255);

    // we are not actually interested in the axis information at all - we only want to "eat" it 
    for (int i=0; i<6; i++)
    {
      GInput.CheckInput(INPUT_DEVICE_MOUSE_AXIS+i,false,true,255);
    }

    mouseX = mouseScrX;
    mouseY = mouseScrY;
    cursor = true;
    return button;
  }
  #endif
  mouseX = mouseY = 0.5;
  return false;
}

#if _ENABLE_PERFLOG && _ENABLE_CHEATS

void Engine::DrawDiagsPerfScope( PerfDiagsContext &ctx, float &y )
{
  if (_statPageStart<0) _statPageStart = 1;
  if (_statPageStart>1) _statPageStart = 0;
  
  const float wScreen = GEngine->Width2D();
  const float hScreen = GEngine->Height2D();
  
  y+=ctx.size;          
  AUTO_STATIC_ARRAY(int,slots,256);
  int slotsCount = GPerfProfilers.N(); 
  DWORD nonZeroSince = GlobalTickCount()-3000;
  for (int c=0; c<slotsCount; c++)
  {
    const char *name=GPerfProfilers.Name(c);
    // if not matching the mask, ignore the counter
    if (strnicmp(name,StatFilter,StatFilter.GetLength())) continue;
    if (!GPerfProfilers.Show(c) || !GPerfProfilers.WasNonZero(c,nonZeroSince)) continue;
    slots.Add(c);
  }
  if (_statPageStart==0)
  {
    QSort(slots.Data(),slots.Size(),&GPerfProfilers,CompareSlots);
  }
  else
  {
    QSort(slots.Data(),slots.Size(),&GPerfProfilers,CompareSlotsByName);
  }
  for (int cc=0; cc<slots.Size(); cc++)
  {
    int c = slots[cc];
    const char *name=GPerfProfilers.Name(c);
    int value=GPerfProfilers.LastValue(c);
    int smoothValue=GPerfProfilers.SmoothValue(c);
    int slowValue=GPerfProfilers.SlowValue(c);
#if ENABLE_COREL_ANALIS
    float corelValue=GPerfProfilers.Correlation(c);
    const TextDrawAttr &attr = corelValue>0.7 ? ctx.attrHigh : ctx.attrText;
#else
    const TextDrawAttr &attr = ctx.attrText;
#endif
    DRAW_TEXT(ctx.x1,y,attr,"%s",name)
      DRAW_TEXT(ctx.x2,y,attr,"%6d",value)
#if ENABLE_COREL_ANALIS
      DRAW_TEXT(ctx.x3,y,attr,"%+.2f",corelValue)
#else
      DRAW_TEXT(ctx.x3,y,attr,"%6d",GPerfProfilers.LastCount(c))
#endif
      DRAW_TEXT(ctx.x4,y,attr,"%6d",smoothValue)
      DRAW_TEXT(ctx.x5,y,attr,"%6d",slowValue)
      y+=ctx.size;
    if (y>=1) break;
  }
}

void Engine::DrawDiagsCPU( PerfDiagsContext &ctx, float &y )
{
}

void Engine::StopDiagsCPU()
{
}


#endif


static float RoundUpNice(float x)
{
  if (x<=0) return 0;
  
  static const float nice[]={0.25f,0.5f};
  
  float roughLog = fastCeil(log10(x));
  float xNice = pow(10,roughLog);
  
  for (int i=0; i<lenof(nice); i++)
  {
    if (x<=xNice*nice[i]) return xNice*nice[i];
  }
  return xNice;
}

void Engine::ShowDiagGraph(IDiagGraph *graph)
{
  _diagGraphs.Add(graph);
}

void Engine::DrawDiagGraph(float x, float y, float w, float h, IDiagGraph *graph)
{
  // fill background
  MipInfo mip = TextBank()->UseMipmap(NULL, 0, 0);
  Rect2DFloat rect(x,y,w,h);
  Rect2DAbs rect2D;
  Convert(rect2D,rect);
  Draw2D(mip,PackedColorRGB(PackedBlack,128),rect2D);
  
  const float yso=0.001,xso=0.001;
  const float wScreen = GEngine->Width2D();
  const float hScreen = GEngine->Height2D();
  float xs = toInt(wScreen * xso), ys = toInt(hScreen * yso);
  saturateMax(xs, 1);
  saturateMax(ys, 1);
  float xsr = x/wScreen, ysr = y/hScreen;
  
  {
    RString text = graph->GetName();
    DrawText(Point2DFloat(x-xsr,y-_debugFontSize+ysr),_debugFontSize,Rect2DClipFloat,_debugFont,PackedBlack,text, 0);
    DrawText(Point2DFloat(x,y-_debugFontSize),_debugFontSize,Rect2DClipFloat,_debugFont,PackedWhite,text, 0);
  }
  
  
  float maxTime = graph->GetMaxTime();
  // first scan X/Y ranges for all data sets
  // always include zero
      
  float yMin = 0,yMax = 0, xMax = 0;
  int nSets = graph->GetDataSetCount();
  for (int set=0; set<nSets; set++)
  {
    int setSize = graph->GetDataSetSize(set);
    float xData = 0;
    for (int i=0; i<setSize; i++)
    {
      if (xData>maxTime) break; // ignore data which will be ignored
      
      float dx,y;
      graph->GetDataSetItem(set,i,dx,y);
      
      saturateMin(yMin,y);
      saturateMax(yMax,y);
      
      Assert(dx>=0);
      xData += dx;
      
    }
    
    saturateMax(xMax,xData);
  
  }
  
  if (xMax>maxTime) xMax = maxTime;
  
  // round up to a reasonable value
  float xRange = floatMin(maxTime,RoundUpNice(xMax));
  
  float xStart = xRange>0 ? 1-xMax/xRange : 0;

  float yRangePos = RoundUpNice(yMax);
  float yRangeNeg = -RoundUpNice(-yMin);

  { // draw axis legend
    float yOffset = 0;
    if (yRangePos>0)
    {
      BString<16> text;
      sprintf(text,"%5g",yRangePos);
      DrawText(Point2DFloat(x+w+xsr,y+yOffset+ysr),_debugFontSize,Rect2DClipFloat,_debugFont,PackedBlack,text, 0);
      DrawText(Point2DFloat(x+w,y+yOffset),_debugFontSize,Rect2DClipFloat,_debugFont,PackedWhite,text, 0);
    }
    if (yRangeNeg<0)
    {
      BString<16> text;
      sprintf(text,"%5g",yRangeNeg);
      DrawText(Point2DFloat(x+w+xsr,y+h-yOffset-_debugFontSize+ysr),_debugFontSize,Rect2DClipFloat,_debugFont,PackedBlack,text, 0);
      DrawText(Point2DFloat(x+w,y+h-yOffset-_debugFontSize),_debugFontSize,Rect2DClipFloat,_debugFont,PackedWhite,text, 0);
    }
    if (xRange>0)
    {
      BString<16> text;
      sprintf(text,"%5g",xRange);
      float tw = GetTextWidth(_debugFontSize,_debugFont,text);
      DrawText(Point2DFloat(x+w-tw+xsr,y+h+ysr),_debugFontSize,Rect2DClipFloat,_debugFont,PackedBlack,text, 0);
      DrawText(Point2DFloat(x+w-tw,y+h),_debugFontSize,Rect2DClipFloat,_debugFont,PackedWhite,text, 0);
    }
  }

  // draw all data sets
  float yText = y;
  for (int set=0; set<nSets; set++)
  {
    // for each set draw all data points
    
    int setSize = graph->GetDataSetSize(set);
    
    if (setSize>1 && xMax>0 && yRangePos>yRangeNeg)
    {
      PackedColor color = graph->GetDataColor(set);
      
      
      DrawLinePrepare();
      
      // draw right (most recent) to left (oldest)
      // always assume all data sets are synchronized at their end, not start
      float xPrev = 0;
      float dx,yPrev;
      graph->GetDataSetItem(set,0,dx,yPrev);
      
      for (int p=1; p<setSize; p++)
      {
        if (xPrev>xRange) break;
        
        float dx,yCur;
        graph->GetDataSetItem(set,p,dx,yCur);
        float xCur = xPrev+dx;
      
        Line2DFloat line;
        
        line.beg.x = x+w*InterpolativC(xPrev,0, xMax,1,xStart);
        line.end.x = x+w*InterpolativC(xCur ,0, xMax,1,xStart);
        
        line.beg.y = y+h*InterpolativC(yPrev, yRangeNeg, yRangePos,1,0);
        line.end.y = y+h*InterpolativC(yCur , yRangeNeg, yRangePos,1,0);
        
        DrawLineDo(line,color,color);
      
        xPrev = xCur;
        yPrev = yCur;
      }


      
      yText += _debugFontSize;
    }
    
  }
}

#if _ENABLE_CHEATS && _ENABLE_PERFLOG
int Engine::ShowStats( StatisticsByName &statsToShow, int scale, float &y, PerfDiagsContext &ctx )
{
  const float wScreen = GEngine->Width2D();
  const float hScreen = GEngine->Height2D();

  statsToShow.SortData(false);
  // stats can be influenced by stats drawing
  // perform a double loop - 1st scan, 2nd read
  AUTO_STATIC_ARRAY(int,stats,256);
  int subtotal = 0;
  int maxItems = toInt((0.95-y)/ctx.size);
  int items = statsToShow.Size();
  if (items>maxItems) items = maxItems;
  stats.Resize(items);
  int statTotal = 0;
  for (int cc=0; cc<items; cc++)
  {
    int value = statsToShow.Get(cc).count;
    stats[cc] = value;
    statTotal += value;
  }
  for (int cc=items; cc<statsToShow.Size(); cc++)
  {
    statTotal += statsToShow.Get(cc).count;
  }
  for (int cc=0; cc<items; cc++)
  {
    const StatisticsByName::Item &item = statsToShow.Get(cc);
    int value = stats[cc];
    subtotal += value;
    if (value<=scale/10 || y>=0.95f) break;
    const char *name=item.name;
    int len = strlen(name);
    const int maxLen = 32;
    if (len>maxLen) name = name+len-maxLen;
    BString<16> valueF;
    if (value<scale)
      sprintf(valueF,"%6.1f",float(value)/scale);
    else if (scale==1000)
      sprintf(valueF,"%5dk",(value+scale/2)/scale);
    else
      sprintf(valueF,"%6d",(value+scale/2)/scale);
      
    DRAW_TEXT(ctx.x1,y,ctx.attrText,"%s",(const char *)name);
    DRAW_TEXT(ctx.x3,y,ctx.attrText,"%6s",cc_cast(valueF));
    DRAW_TEXT(ctx.x4,y,ctx.attrText,"%6s",cc_cast(valueF));
    y+=ctx.size;
  }
  return statTotal;
}
#endif

/// draw fps history
class FpsGraph: public IDiagGraph
{
  DWORD (&_frameDuration)[Engine::NFrameDurations];
  
  public:
  RString GetName() const {return "Frame Duration";}
  FpsGraph(DWORD (&frameDuration)[Engine::NFrameDurations])
  :_frameDuration(frameDuration)
  {
  }
  int GetDataSetCount() const {return 2;}
  PackedColor GetDataColor(int set) const {return set>0 ? PackedColor(255,255,0,255) : PackedWhite;}
  int GetDataSetSize(int set) const {return lenof(_frameDuration);}
  void GetDataSetItem(int set, int i, float &dx, float &y) const
  {
    dx = _frameDuration[Engine::NFrameDurations-1-i];
    if (set==0)
      y = _frameDuration[Engine::NFrameDurations-1-i];
    else
      y = Engine::GetMinFrameDurationSmooth(_frameDuration);
  }
  float GetMaxTime() const {return 4999;}
  
  bool ValidForFPSMode( int fpsMode ) const
  {
    #if _ENABLE_CHEATS
    return fpsMode==FpsBasic || fpsMode==FpsPerfScope || fpsMode==FpsMouse;
    #else
    return fpsMode==FpsBasic;
    #endif
  }

};

// TODO: find a more modular way how to get this information here
bool EnableLowFpsDetection();

static inline int SortByHandle(const TextInfo *t1, const TextInfo *t2)
{
  if (int d = t1->_handle-t2->_handle) return d;
  int d = t1->_id-t2->_id;
  return d;
}
void Engine::DrawFinishTexts()
{
  {
    PROFILE_SCOPE_GRF(dtTot,SCOPE_COLOR_GREEN);

    const float wScreen = GEngine->Width2D();
    const float hScreen = GEngine->Height2D();
      
    enum FpsMode {FMIgnore,FMMinimal,FMRelaxed,FMStrict,NFM};
#if _ENABLE_REPORT && _RELEASE
    static FpsMode fpsMode = FMIgnore;
#else
    static FpsMode fpsMode = FMIgnore;
#endif
    const char *fpsModeString[]={"None","Minimal","Relaxed","Strict"};
#if _ENABLE_CHEATS
    if (int dir = GInput.GetCheatXToDo(CXTLowFpsDetect)+GInput.GetCheat3ToDo(DIK_F))
#else
    if (int dir = GInput.GetCheatXToDo(CXTLowFpsDetect))
#endif
    {
      int newFpsMode = fpsMode+dir;
      while (newFpsMode<0) newFpsMode = NFM-1;
      while (newFpsMode>=NFM) newFpsMode = 0;
      fpsMode = FpsMode(newFpsMode);
      GlobalShowMessage(500,"Low fps detection %s",fpsModeString[fpsMode]);
    }
    Font *font=GetDebugFont();

    const float ctxSize = 0.02f;    
    PackedColor color(Color(1,0.8,0.5,0.8));
    PackedColor colorHigh(Color(1,0.7,0.2,0.8));
    TextDrawAttr attrShadow(ctxSize,font,PackedBlack,true);
    TextDrawAttr attrText(ctxSize,font,color,true);
    TextDrawAttr attrHigh(ctxSize,font,colorHigh,true);
    PerfDiagsContext ctx(attrShadow,attrText,attrHigh);
    const float y0 = 0.05f;
#if _ENABLE_PERFLOG && _ENABLE_CHEATS
    const float xName = 0.2f;
    const float yName = y0;
#endif
    ctx.x1 = 0.05f;
    ctx.x2 = 0.25f;
    ctx.x3 = 0.35f;
    ctx.size = ctxSize;
    float y=y0;
    if (ShowFpsMode()>0)
    {
      
      if (!_fpsGraph)
      {
        _fpsGraph = new FpsGraph(_frameDurations);
      }
      ShowDiagGraph(_fpsGraph);
      
    }

    #if _ENABLE_CHEATS
    { // draw all diagnostics graphs shown in this frame
      float x = 0.5f;
      float y = 0.22f;
      float w = 0.35f;
      float h =  0.2f;
      for (int i=0; i<_diagGraphs.Size(); i++)
      {
        if (_diagGraphs[i]->ValidForFPSMode(ShowFpsMode()))
        {
          DrawDiagGraph(x,y,w,h,_diagGraphs[i]);
          y += h+0.05f;
        }
      }
      _diagGraphs.Resize(0);
    }
    #endif

    if( ShowFpsMode()>0 )
    {
      
      // draw file queue status
      #if _ENABLE_PERFLOG
      float status = GFileServer->PreloadQueueStatus();
      int reqSize;
      int requests = GFileServer->PreloadQueueRequests(&reqSize);
      if (status>0 || requests>0)
      {
        Color fullColor(1,0,0,0.2);
        Color emptyColor(0,1,0,0.2);
        Color color = fullColor*status+emptyColor*(1-status);
        Rect2DFloat rect;
        rect.x = 0.5;
        rect.w = (0.95-rect.x)*status;
        rect.h = 0.05;
        rect.y = 0.15;
        Rect2DAbs rect2D;
        Convert(rect2D,rect);
        MipInfo mip = TextBank()->UseMipmap(NULL, 0, 0);
        Draw2D(mip,PackedColor(color),rect2D);

        Point2DAbs pos(rect2D.x,rect2D.y);
        BString<20> byteSize;
        FormatByteSize(byteSize,reqSize);
        DrawTextF(pos,ctx.attrText,"%d (%s)",requests,cc_cast(byteSize));
      }
      #endif
      

      float atps = 0;
      for( int i=0; i<NFrameDurations; i++ )
      {
        int dur = _frameDurations[i];
        float tps = dur>0 ? _frameTriangles[i]*1000/dur : _frameTriangles[i]*1000;
        atps+=tps;
      }
      atps *= 1.0/NFrameDurations;

      
      float fps=0;
      if (_lastFrameDuration>0) fps=1000.0/_lastFrameDuration;
      else fps = 1000;

      float afps = 1000.0f/GetAvgFrameDuration(NFrameDurations);
      #if _ENABLE_CHEATS
      float mfps = 1000.0f/GetMaxFrameDuration(NFrameDurations);
      #endif
      
      float afpsV, mfpsV;
      if (GEngine->CanVSyncTime())
      {
        afpsV = float(RefreshRate())/GetAvgFrameDurationVsync();
        mfpsV = float(RefreshRate())/GetMaxFrameDurationVsync();
      }
      else
      {
        afpsV = afps;
        mfpsV = 1.0f/GetSmoothFrameDurationMax();
      }
      
      // draw text information
      //ShowTextF(-1000,170,120,"FPS %5.2f",fps);
      
      DRAW_TEXT(ctx.x1,y,ctx.attrText,"iFPS %7.2f (%4.1f ms)",fps, 1000.0f/floatMax(0.001f, fps))
      y+=ctx.size;

      DRAW_TEXT(ctx.x1,y,ctx.attrText,"aFPS %7.2f (%4.1f ms)",afpsV, 1000.0f/floatMax(0.001f, afpsV))

      #if _ENABLE_CHEATS
        if (GEngine->CanVSyncTime())
        {
          DRAW_TEXT(ctx.x2,y,ctx.attrText,"(%7.2f, %4.1ms)",afps, 1000.0f/floatMax(0.001f, afps))
        }
      #endif
      
      y+=ctx.size;

      const TextDrawAttr &fAttr = mfpsV<16 ? ctx.attrHigh : ctx.attrText;
      
      DRAW_TEXT(ctx.x1,y,fAttr,"mFPS %7.2f (%4.1f ms)",mfpsV, 1000.0f/floatMax(0.001f, mfpsV))

      #if _ENABLE_CHEATS
        DRAW_TEXT(ctx.x2,y,ctx.attrText,"(%7.2f)",mfps)
      #endif
      y+=ctx.size;

      #if _ENABLE_PERFLOG
        y+=ctx.size;
        DRAW_TEXT(ctx.x1,y,ctx.attrText,"aTPS %7.0f",atps)
        y+=ctx.size;
        DRAW_TEXT(ctx.x1,y,ctx.attrText,"Density %.4f",GScene->GetDrawDensity())
        y+=ctx.size;
      #endif

      //DrawTextF(x1,y,attrText,"cLOD %7.2f",GScene->GetScaleDownCoef());
      //y+=0.02;

      #if _ENABLE_PERFLOG
        {
          int wset = GetMemoryUsedSize()/(1024*1024);
          int memPageFile = GetMemoryCommitedSize()/(1024*1024);
          DRAW_TEXT(ctx.x1,y,ctx.attrText,"wset %d",wset);
          y+=ctx.size;
          DRAW_TEXT(ctx.x1,y,ctx.attrText,"MemPF %d",memPageFile);
          y+=ctx.size;
            
          MEMORYSTATUSEX status;
          status.dwLength = sizeof(status);
          GlobalMemoryStatusEx(&status);
          DRAW_TEXT(ctx.x1,y,ctx.attrText,"Phys free %s",cc_cast(FormatByteSize(status.ullAvailPhys)));
          y+=ctx.size;
          DRAW_TEXT(ctx.x1,y,ctx.attrText,"Virt free %s",cc_cast(FormatByteSize(status.ullAvailVirtual)));
          y+=ctx.size;
        }
      #endif
      #if 0 //!_SUPER_RELEASE
        int memCommited = MemoryCommited()/(1024*1024);
        DRAW_TEXT(x1,y,attrText,"MemC %d",memCommited)
        y+=ctx.size;
      #endif

      #if _ENABLE_PERFLOG && _ENABLE_CHEATS
        ctx.x4 = 0.45f;
        ctx.x5 = 0.52f;
        if (_showFps>FpsBasic)
        {
          DRAW_TEXT(xName,yName,ctx.attrText,"%s",cc_cast(FindEnumName((ShowFps)_showFps)));
        }
        if (_showFps!=FpsCPU)
        {
          StopDiagsCPU();
        }
        switch (_showFps)
        {
          case FpsCPU:
            DrawDiagsCPU(ctx, y);
            break;
          case FpsPerfScope:
            DrawDiagsPerfScope(ctx, y);
            break;
          case FpsPerfCount:
            {
              y+=ctx.size;
              // no sort
              DWORD nonZeroSince = GlobalTickCount()-3000;
              for (int cc=0; cc<GPerfCounters.N(); cc++)
              {
                int c = cc;
                if (GPerfCounters.Show(c) && GPerfCounters.WasNonZero(c,nonZeroSince))
                {
                  const char *name=GPerfCounters.Name(c);
                  if (strnicmp(name,StatFilter,StatFilter.GetLength())) continue;
                  int value=GPerfCounters.LastValue(c);
                  int slowValue=GPerfCounters.SlowValue(c);
                  int smoothValue=GPerfCounters.SmoothValue(c);
                  #if ENABLE_COREL_ANALIS
                    float corelValue=GPerfCounters.Correlation(c);
                    const TextDrawAttr &attr = corelValue>0.5 ? ctx.attrHigh : ctx.attrText;
                  #else
                    const TextDrawAttr &attr = ctx.attrText;
                  #endif
                  DRAW_TEXT(ctx.x1,y,attr,"%s",(const char *)name)
                  DRAW_TEXT(ctx.x2,y,attr,"%6d",value)
                  #if ENABLE_COREL_ANALIS
                    DRAW_TEXT(ctx.x3,y,attr,"%+.2f",corelValue)
                  #endif
                  DRAW_TEXT(ctx.x4,y,attr,"%6d",smoothValue)
                  DRAW_TEXT(ctx.x5,y,attr,"%6d",slowValue)
                  y+=ctx.size;
                  if (y>=1) break;
                }
              }
              break;
            }
          case FpsDPrim:
            {
              DEF_COUNTER(dPrim,1);
              int total = COUNTER_VALUE(dPrim);
              // DPrim stats
              y+=ctx.size;       
              // no sort
              DPrimStats.SortData(false);
              // stats can be influenced by stats drawing
              // perform a double loop - 1st scan, 2nd read
              AUTO_STATIC_ARRAY(int,stats,256);
              int subtotal = 0;
              int maxItems = toInt((0.95-y)/ctx.size);
              const int perPage = 0.5f/ctx.size;
              // for pages here we do not want any cycling
              if (_statPageStart<0) _statPageStart = 0;
              int start = perPage*_statPageStart;
              int items = DPrimStats.Size();
              if (items>maxItems+start)
              {
                items = maxItems+start;
              }
              stats.Resize(items);
              int statTotal = 0;
              
              for (int cc=0; cc<items; cc++)
              {
                int value = DPrimStats.Get(cc).count;
                stats[cc] = value;
                statTotal += value;
              }
              for (int cc=items; cc<DPrimStats.Size(); cc++)
              {
                statTotal += DPrimStats.Get(cc).count;
              }
              for (int cc=0; cc<items; cc++)
              {
                const StatisticsByName::Item &item = DPrimStats.Get(cc);
                int value = stats[cc];
                subtotal += value;
                if (cc>=start)
                {
                  if (value<=0 || y>=0.95f) break;
                  const char *name=item.name;
                  int len = strlen(name);
                  const int maxLen = 32;
                  if (len>maxLen) name = name+len-maxLen;
                  DRAW_TEXT(ctx.x1,y,ctx.attrText,"%s",(const char *)name)
                  DRAW_TEXT(ctx.x3,y,ctx.attrText,"%6d",value)
                  DRAW_TEXT(ctx.x4,y,ctx.attrText,"%6d",subtotal)
                  y+=ctx.size;
                }
              }
              DRAW_TEXT(ctx.x3,y,ctx.attrHigh,"%6d",total)
              DRAW_TEXT(ctx.x4,y,ctx.attrHigh,"%6d",statTotal)
              break;
            }
          case FpsTris:
            {
              DEF_COUNTER(tris,1);
              int total = COUNTER_VALUE(tris);
              y+=ctx.size;       
              const int scale = 1000;
              int statTotal = ShowStats(TrisStats, scale, y, ctx);

              DRAW_TEXT(ctx.x3,y,ctx.attrHigh,"%6d",(total+scale/2)/scale)
              DRAW_TEXT(ctx.x4,y,ctx.attrHigh,"%6d",(statTotal+scale/2)/scale)
              break;
              }
          case FpsObjs:
              {
              DEF_COUNTER(obj,1);
              int total = COUNTER_VALUE(obj);
                y+=ctx.size;
              const int scale = 1;
              int statTotal = ShowStats(ObjsStats, scale, y, ctx);

              DRAW_TEXT(ctx.x3,y,ctx.attrHigh,"%6d",(total+scale/2)/scale)
              DRAW_TEXT(ctx.x4,y,ctx.attrHigh,"%6d",(statTotal+scale/2)/scale)
              break;
            }
          case FpsPixs:
            #if 0
            {
              //DEF_COUNTER(tris,1);
              //int total = COUNTER_VALUE(tris);
              // Tris stats
              y+=ctx.size;       
              // no sort
              PixelStats.SortData(false);
              // stats can be influenced by stats drawing
              // perform a double loop - 1st scan, 2nd read
              AUTO_STATIC_ARRAY(int,stats,256);
              int subtotal = 0;
              int maxItems = toInt((0.95-y)/ctx.size);
              int items = PixelStats.Size();
              if (items>maxItems) items = maxItems;
              stats.Resize(items);
              int statTotal = 0;
              for (int cc=0; cc<items; cc++)
              {
                int value = PixelStats.Get(cc).count;
                stats[cc] = value;
                statTotal += value;
              }
              for (int cc=items; cc<PixelStats.Size(); cc++)
              {
                statTotal += PixelStats.Get(cc).count;
              }
              const int scale = 1000;
              for (int cc=0; cc<items; cc++)
              {
                const StatisticsByName::Item &item = PixelStats.Get(cc);
                int value = stats[cc];
                subtotal += value;
                if (value<scale/2 || y>=0.95f) break;
                const char *name=item.name;
                int len = strlen(name);
                const int maxLen = 32;
                if (len>maxLen) name = name+len-maxLen;
                DRAW_TEXT(ctx.x1,y,ctx.attrText,"%s",(const char *)name)
                DRAW_TEXT(ctx.x3,y,ctx.attrText,"%6d",(value+scale/2)/scale)
                DRAW_TEXT(ctx.x4,y,ctx.attrText,"%6d",(subtotal+scale/2)/scale)
                y+=ctx.size;
              }
              //DRAW_TEXT(x3,y,ctx.attrHigh,"%6d",(total+scale/2)/scale)
              //DRAW_TEXT(x4,y,ctx.attrHigh,"%6d",(statTotal+scale/2)/scale)
            }
            #endif
            break;
          case FpsTexMem:
            y+=ctx.size;
            
            DRAW_TEXT(ctx.x1,y,ctx.attrText,"Resolution: 2D %d x %d, 3D %d x %d",WidthBB(),HeightBB(),Width(),Height());
            y+=ctx.size;
            y+=ctx.size;
            
            for (int c=0; c<100; c++)
            {
              RString memUsed,memUsed2;
              RString name = TextBank()->GetStat(c,memUsed,memUsed2);
              if (name.GetLength()==0) break;
              if (memUsed.GetLength()>0 || strcmp(name," "))
              {
                DRAW_TEXT(ctx.x1,y,ctx.attrText,"%s",(const char *)name)
                DRAW_TEXT(ctx.x2,y,ctx.attrText,"%6s",(const char *)memUsed)
                DRAW_TEXT(ctx.x3,y,ctx.attrText,"%6s",(const char *)memUsed2)
              }
              y+=ctx.size;
              if (y>=1) break;
            }
            y+=ctx.size;
            for (int c=0; c<100; c++)
            {
              RString memUsed,memUsed2;
              RString name = GetStat(c,memUsed,memUsed2);
              if (name.GetLength()==0) break;
              if (memUsed.GetLength()>0 || strcmp(name," "))
              {
                DRAW_TEXT(ctx.x1,y,ctx.attrText,"%s",(const char *)name)
                DRAW_TEXT(ctx.x2,y,ctx.attrText,"%6s",(const char *)memUsed)
                DRAW_TEXT(ctx.x3,y,ctx.attrText,"%6s",(const char *)memUsed2)
              }
              y+=ctx.size;
              if (y>=1) break;
            }
            break;
          case FpsMemStats:
            y+=ctx.size;
            for (int c=0; c<100; c++)
            {
              RString GetMemStat(mem_size_t statId, mem_size_t &statVal);
              
              mem_size_t value;
              RString name = GetMemStat(c,value);
              if (name.GetLength()==0) break;
              if (value==0 && strcmp(name," ")) continue;
              RString memUsed = value==0 ? "" : FormatByteSize(value);
              DRAW_TEXT(ctx.x1,y,ctx.attrText,"%s",(const char *)name)
              DRAW_TEXT(ctx.x2,y,ctx.attrText,"%6s",(const char *)memUsed)
              y+=ctx.size;
              if (y>=1) break;
            }
            break;
          case FpsDiscardableStats:
            {
              // display info from all MemoryOnDemand containers
              
              FreeOnDemandScopeLock lock;
              
              typedef IMemoryFreeOnDemand *GetFirstMemoryFreeOnDemandF();
              typedef IMemoryFreeOnDemand *GetNextMemoryFreeOnDemandF(IMemoryFreeOnDemand *cur);
              const static struct 
              {
                GetFirstMemoryFreeOnDemandF *first;
                GetNextMemoryFreeOnDemandF *next;
                const char *name;
              } onDemandFcs[]=
              {
                {lock.GetFirstFreeOnDemandMemory,lock.GetNextFreeOnDemandMemory,"Heap"},
                {GetFirstFreeOnDemandSystemMemory,GetNextFreeOnDemandSystemMemory,"Sys"},
                {lock.GetFirstFreeOnDemandLowLevelMemory,lock.GetNextFreeOnDemandLowLevelMemory,"LL"},
              };
                
              for (int i=0; i<sizeof(onDemandFcs)/sizeof(*onDemandFcs); i++)
              {
                y+=ctx.size;
                size_t total = 0;
                size_t totalRecent = 0;
                for
                (
                  IMemoryFreeOnDemand *mem = onDemandFcs[i].first();
                  mem;
                  mem = onDemandFcs[i].next(mem)
                )
                {
                  RString fullName = mem->GetDebugName();
                  if (fullName.GetLength()<=0) continue;
                  size_t memSize = mem->MemoryControlled();
                  size_t memSizeRecent = mem->MemoryControlledRecent();
                  total += memSize;
                  totalRecent += memSizeRecent;
                  RString memUsed = FormatByteSize(memSize);
                  RString memUsedRecent = FormatByteSize(memSizeRecent);
                  RString name = fullName.Substring(0,15);
                  float priority = mem->Priority();
                  float priorCheck = memSize/priority/100000;
                  DRAW_TEXT(ctx.x1,y,ctx.attrText,"%s",(const char *)name)
                  DRAW_TEXT(ctx.x2,y,ctx.attrText,"%6s",(const char *)memUsed)
                  DRAW_TEXT(ctx.x3,y,ctx.attrText,"%6s",(const char *)memUsedRecent)
                  DRAW_TEXT(ctx.x4,y,ctx.attrText,"%.2f",priority)
                  DRAW_TEXT(ctx.x5,y,ctx.attrText,"%.2f",priorCheck)
                  y+=ctx.size;
                }
                RString memUsed = FormatByteSize(total);
                RString memUsedRecent = FormatByteSize(totalRecent);
                DRAW_TEXT(ctx.x1,y,ctx.attrText,"%s - total",onDemandFcs[i].name)
                DRAW_TEXT(ctx.x2,y,ctx.attrText,"%6s",(const char *)memUsed)
                DRAW_TEXT(ctx.x3,y,ctx.attrText,"%6s",(const char *)memUsedRecent)
                y+=ctx.size;
              }
            }
            break;
        }
      #endif
    }
    else if (fpsMode>0 && EnableLowFpsDetection())
    {
    
      float afps,mfps;
      if (GEngine->CanVSyncTime())
      {
        mfps = float(RefreshRate())/GetMaxFrameDurationVsync();
        afps = float(RefreshRate())/GetAvgFrameDurationVsync();
      }
      else
      {
        mfps = 1000.0f/GetMaxFrameDuration(NFrameDurations);
        afps = 1000.0f/GetAvgFrameDuration(NFrameDurations);
      }

      #if _RELEASE
      static const float fpsLimitMin[]={0,14.0f,18.0f,19.5f,FLT_MAX};
      static const float fpsLimitAvg[]={0,0,0,0,FLT_MAX};
      #else
      // we have a little less strict requirement in SuperRelease
      // both avg. and min need to be below given limit for a condition to be satisfied
      static const float fpsLimitMin[]={0,14.0f,18.0f,19.5f,FLT_MAX};
      static const float fpsLimitAvg[]={0,25.0f,19.0f,19.0f,FLT_MAX};
      #endif
      
      int serious = 0;
      while (
        serious<fpsMode && fpsLimitMin[serious+1]<mfps && fpsLimitAvg[serious+1]<afps
      ) serious++;
      if (serious<fpsMode)
      {
        static PackedColor fpsColor[]=
        {
          PackedColor(0xff,0,0,0x80), // below 11.9
          PackedColor(0xff,0x80,0,0x60), // below 14.9
          PackedColor(0xff,0xff,0,0x40), // below 16.5
          PackedColor(0,0,0,0),
        };
        
        Rect2DFloat rect;
        rect.x = 0.05f;
        rect.y = 0.08f;
        rect.w = 0.04f;
        rect.h = 0.04f;
        Rect2DAbs rect2D;
        Convert(rect2D,rect);
        MipInfo mip = TextBank()->UseMipmap(NULL, 0, 0);
        Draw2D(mip,PackedColor(fpsColor[serious]),rect2D);
        
        // do not display fps in strict mode, as even displaying it affects fps
        if (fpsMode<=FMRelaxed)
        {
          DRAW_TEXT(ctx.x1,y,ctx.attrText,"aFPS %7.2f",afps)
          y+=ctx.size;
          DRAW_TEXT(ctx.x1,y,ctx.attrText,"mFPS %7.2f",mfps)
          y+=ctx.size;
        }
        #if _ENABLE_PERFLOG
          void ReportLowFps(float mfps, float afps);
          ReportLowFps(mfps,afps);

        #endif
      }
    }

    float maxWidth = 0; 
    TextDrawAttr stAttr(_showTextSize,_showTextFont,PackedWhite,true);
    TextDrawAttr stAttrShadow(_showTextSize,_showTextFont,PackedBlack,true);

    // draw all ShowText buffers
    QSort(_texts,SortByHandle);
    for( int i=0; i<_texts.Size(); i++)
    {
      TextInfo &info=_texts[i];
      float width = GetTextWidth(stAttr, info._text);
      saturateMax(maxWidth,width);
    }
    
    if (maxWidth>0)
    {
      float textX = 25*(1.0/800);
      float textY = 2*(1.0/800);
      PackedColor black(Color(0, 0, 0, 0.8));
      MipInfo mip = GLOB_ENGINE->TextBank()->UseMipmap(NULL, 0, 0);
      int w = Width2D(), h = Height2D();
      // draw fixed width
      GEngine->Draw2D
      (
        mip, black,
        Rect2DPixel(textX * w, textY * h, maxWidth * w, _showTextSize * h* _texts.Size())
      );
        
      for( int i=0; i<_texts.Size(); )
      {
        TextInfo &info=_texts[i];
        DRAW_TEXT(textX,textY,stAttr,"%s",(const char *)info._text)

        textY += _showTextSize;
        if( (int)(_frameTime-info._hideTime)>=0 ) _texts.Delete(i);
        else i++;
      }
    }

  }
  #if _ENABLE_CHEATS && _ENABLE_PERFLOG
    DPrimStats.Clear();
    TrisStats.Clear();
    PixelStats.Clear();
    ObjsStats.Clear();
  #endif
}

void OFPFrameFunctions::ShowMessage(int timeMs, const char *msg)
{
  if (!GEngine) return;
  
  static int handle = -1;
  DiagMessage(handle,0,timeMs,msg);
}

void OFPFrameFunctions::DiagMessage(int &handle, int id, int timeMs, const char *msg, ...)
{
  if (!GEngine) return;

  // if given handle is valid, reuse it, otherwise create a new one
  BString<256> buf;
  va_list arglist;
  va_start(arglist, msg);
  vsprintf(buf,msg,arglist);
  va_end(arglist);
  handle = GEngine->CreateShowText(handle,id,timeMs, msg);
}

TextInfo::TextInfo(int handle, int id, DWORD hideTime, RString text)
:_hideTime(hideTime),
_handle(handle),_id(id),
_text(text)
{
}

void Engine::ShowFont( Font *font, PackedColor color, float size )
{
  _showTextFont=font;
  _showTextColor=color;
  _showTextSize=size;
}

void Engine::RemoveText( int handle )
{
  if( handle>=0 )
  {
    for( int i=0; i<_texts.Size(); i++ )
    {
      if( _texts[i]._handle==handle )
      {
        _texts.Delete(i);
        return;
      }
    }
    // note: text may be expired
    //Fail("Invalid text handle.");
  }
}

int Engine::FindTextSlot(int handle, int id) const
{
  for( int i=0; i<_texts.Size(); i++ )
  {
    if( _texts[i]._handle==handle && _texts[i]._id==id)
    {
      return i;
    }
  }
  return -1;
}

bool Engine::AllocateTextHandles(InitVal<int,-1> *handles, int count)
{
  for (int i = 0; i < count; ++i)
    if (handles[i] < 0)
    {
      handles[i] = _textHandle++;
    }
  return true;
}

int Engine::CreateShowText(int handle, int id, DWORD timeToLive, const char *text)
{
  // init the handle (a static variable) only once
  if (handle<0)
  {
    handle = _textHandle++;
  }
  // try to find a corresponding slot
  int slot = FindTextSlot(handle,id);
  DWORD hideTime = GlobalTickCount()+timeToLive;
  if (slot>=0)
  {
    // slot found, update
    _texts[slot]._text = text;
    _texts[slot]._hideTime = hideTime;
    return handle;
  }
  // not found: create a new one
  _texts.Append() = TextInfo(handle,id,hideTime,text);
  return handle;
}

int Engine::ShowText( DWORD timeToLive, const char *text )
{
  int handle = CreateShowText(-1,0,timeToLive,text);
  return handle;
}

int CCALL Engine::ShowTextF( DWORD timeToLive, const char *format, ... )
{
  BString<512> buf;
  va_list arglist;
  va_start( arglist, format );
  vsprintf( buf, format, arglist );
  va_end( arglist );
  return ShowText(timeToLive,buf);
}

void Engine::Draw2DWholeScreen(PackedColor color)
{
	Draw2DPars pars;
	pars.mip= GLOB_ENGINE->TextBank()->UseMipmap(NULL,0,0);
	pars.SetColor(color);
	pars.spec = NoZBuf|IsAlpha|NoClamp|IsAlphaFog;
	pars.SetU(0,1);
	pars.SetV(0,1);
	Rect2DAbs rect;
	rect.x=0,rect.y=0;
	rect.w=GEngine->WidthBB(),rect.h=GEngine->HeightBB();
	GEngine->Draw2D(pars,rect);
}

void Engine::Draw2DWholeScreen(PackedColor color, float alpha)
{
  int a8 = toInt(alpha*255);
  if (a8<=0) return;
  if (a8>255) a8 = 255;
  Draw2DWholeScreen(PackedColorRGB(color,a8));
}


void Engine::DrawFrame(Texture *corner, PackedColor color, const Rect2DPixel &frame, const Rect2DPixel &clip)
{
  const int screenW = GLOB_ENGINE->Width2D();
  const int screenH = GLOB_ENGINE->Height2D();

  MipInfo mip = GLOB_ENGINE->TextBank()->UseMipmap(corner, 0, 0);
  float invCornerH=1, invCornerW=1;
  if (corner)
  {
    invCornerH = 1200.0 * (1.0 / (screenH * corner->AHeight()));
    invCornerW = 1600.0 * (1.0 / (screenW * corner->AWidth()));
  }
  Draw2DPars pars;
  Rect2DPixel rect;
  rect.w = 0.5 * frame.w;
  rect.h = 0.5 * frame.h;
  pars.mip = mip;
  pars.SetColor(color);
  pars.spec = NoZBuf|IsAlpha|ClampU|ClampV|IsAlphaFog;
  float coefX = 0.5 * frame.w * invCornerW;
  float coefY = 0.5 * frame.h * invCornerH;
  
  rect.x = frame.x;
  rect.y = frame.y;
  pars.SetU(0,coefX);
  pars.SetV(0,coefY);
  GLOB_ENGINE->Draw2D(pars, rect, clip);

  rect.x = frame.x + rect.w;
  rect.y = frame.y;
  pars.SetU(coefX,0);
  pars.SetV(0,coefY);
  GLOB_ENGINE->Draw2D(pars, rect, clip);

  rect.x = frame.x;
  rect.y = frame.y + rect.h;
  pars.SetU(0,coefX);
  pars.SetV(coefY,0);
  GLOB_ENGINE->Draw2D(pars, rect, clip);

  rect.x = frame.x + rect.w;
  rect.y = frame.y + rect.h;
  pars.SetU(coefX,0);
  pars.SetV(coefY,0);
  GLOB_ENGINE->Draw2D(pars, rect, clip);

  //GLOB_ENGINE->TextBank()->ReleaseMipmap();
}

void Engine::DrawLineFrame(PackedColor color, const Rect2DPixel &frame)
{
  float x1 = frame.x;
  float x2 = frame.x+frame.w;
  float y1 = frame.y;
  float y2 = frame.y+frame.h;
  GEngine->DrawLine(Line2DPixel(x1, y1, x2, y1), color, color);
  GEngine->DrawLine(Line2DPixel(x2, y1, x2, y2), color, color);
  GEngine->DrawLine(Line2DPixel(x2, y2, x1, y2), color, color);
  GEngine->DrawLine(Line2DPixel(x1, y2, x1, y1), color, color);
}

void Engine::DrawLineFrame(PackedColor color, const Rect2DAbs &frame)
{
  Rect2DPixel rect;
  GEngine->Convert(rect,frame);
  DrawLineFrame(color,rect);
}

void Engine::DrawFrame(Texture *corner, PackedColor color, const Rect2DAbs &frame, const Rect2DAbs &clip)
{
  Rect2DPixel rect, clipRect;
  GEngine->Convert(rect, frame);
  GEngine->Convert(clipRect, clip);
  DrawFrame(corner, color, rect, clipRect);
}

void Engine::DrawMeshTLInstanced(
  int cb, const Shape &sMesh, const EngineShapeProperties &prop, int spec, const LightList &lights, int bias,
  const ObjectInstanceInfo *instances, int nInstances, float minDist2, const DrawParameters &dp
)
{
  // Go through the instances
  for (int j = 0; j < nInstances; j++)
  {
    //SCOPE_GRF("inst",0);
    PROFILE_SCOPE_GRF_EX(inst,drw,SCOPE_COLOR_RED);
    // check if shape is dynamic or not
    const ObjectInstanceInfo &info = instances[j];

    SetInstanceInfo(cb, info._color, info._shadowCoef);

    BeginInstanceTL(cb, info._origin, minDist2, spec, lights, dp);
    bool ok = BeginMeshTL(cb, NULL, sMesh, prop, spec, lights, bias);
    if (ok)
    {
      // check first face properties
      int secBeg = -1;
      int secEnd = -1;
      ShapeSectionInfo sProp;
      int minBoneIndex = -1;
      int bonesCount = -1;

      for (int i=0; i<sMesh.NSections(); i++)
      {
        const ShapeSection &sec = sMesh.GetSection(i);

        if (sec.Special()&(IsHidden|IsHiddenProxy)) continue;

        // Calculate the bias for section
        int secBias = ((sec.Special()&ZBiasMask)/ZBiasStep) * 5;
        int maxBias = intMax(bias, secBias);

        if (secBeg<0)
        {
          secBeg = i;
          secEnd = i+1;
          sProp = sec;
          minBoneIndex = sec._minBoneIndex;
          bonesCount = sec._bonesCount;
        }
        else if	(
          sec==sProp &&
          sec._minBoneIndex==minBoneIndex &&
          sec._bonesCount==bonesCount &&
          i==secEnd)
        {
          // extend section
          secEnd=i+1;
        }
        else
        {
          // flush section
          TLMaterial mat;

          // distribute by predefined materials
          CreateMaterial(mat, HWhite);

          sMesh.GetSection(secBeg).PrepareTL(cb,mat, spec, prop);
          DrawSectionTL(cb,sMesh, secBeg, secEnd, maxBias);
          // open another section
          secBeg = i;
          secEnd = i+1;
          sProp = sec;
          minBoneIndex = sec._minBoneIndex;
          bonesCount = sec._bonesCount;
        }
      }
      if (secEnd>secBeg)
      {
        TLMaterial mat;

        // distribute by predefined materials
        CreateMaterial(mat, HWhite);

        // flush section
        sMesh.GetSection(secBeg).PrepareTL(cb,mat, spec, prop);

        // Calculate the bias for section
        int secBias = ((sMesh.GetSection(secBeg).Special()&ZBiasMask)/ZBiasStep) * 5;
        int maxBias = intMax(bias, secBias);

        DrawSectionTL(cb,sMesh, secBeg, secEnd, maxBias);
      }

      EndMeshTL(cb,sMesh);
    }
  }

  SetInstanceInfo(cb,HWhite, 1);
}

void Engine::DrawShadowVolumeInstanced(int cb, LODShape *shape, int level, const ObjectInstanceInfo *instances, int instanceCount, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
                                       , int drawListId
#endif
                                       )
{
  for (int i = 0; i < instanceCount; i++)
  {
    SCOPE_GRF("inst-shadow",0);
#if _HELISIM_RENDER_TO_TEXTURE
    instances[i]._object->DrawShadowVolume(cb, shape, level, instances[i]._origin, oi, drawListId);
#else
    instances[i]._object->DrawShadowVolume(cb, shape, level, instances[i]._origin, oi);
#endif
  }
}

// new 3D drawing
void Engine::Draw3D(const PositionRender &space, const Draw2DParsExt &pars, const Rect2DFloat &rect, const Rect2DFloat &clip, const Pars3D *pars3D)
{
  Vertex2DFloat vf[4];
  vf[0].x = rect.x;
  vf[0].y = rect.y;
  vf[0].u = pars.uTL;
  vf[0].v = pars.vTL;
  vf[0].color = pars.colorTL;

  vf[1].x = rect.x + rect.w;
  vf[1].y = rect.y;
  vf[1].u = pars.uTR;
  vf[1].v = pars.vTR;
  vf[1].color = pars.colorTR;

  vf[2].x = rect.x + rect.w;
  vf[2].y = rect.y + rect.h;
  vf[2].u = pars.uBR;
  vf[2].v = pars.vBR;
  vf[2].color = pars.colorBR;

  vf[3].x = rect.x;
  vf[3].y = rect.y + rect.h;
  vf[3].u = pars.uBL;
  vf[3].v = pars.vBL;
  vf[3].color = pars.colorBL;

  DrawPoly3D(space, pars.mip, vf, 4, clip, pars.spec, TLMaterial::_default, pars3D);
}

static const float SizeFont3D = 48;

/*!
\patch 5143 Date 3/22/2007 by Jirka
- Fixed: Clipping of some briefings on the right edge

\patch 5160 Date 5/22/2007 by Ondra
- Optimized: Text rendering optimized, should make situations where a lot of UI text is rendered faster.

\patch 5161 Date 5/25/2007 by Jirka
- Fixed: Briefing was sometimes cut on the right edge.
*/

void Engine::DrawText3D(const PositionRender &space, const Point2DFloat &pos, const TextDrawAttr &attr, const char *text, const Rect2DFloat &clip, const Pars3D *pars3D)
{
  if (*text == 0) return;
  Font *font = attr._font;
  if (!font->IsLoaded()) return;

  AssertMainThread();
  
  DPRIM_STAT_SCOPE_SIMPLE(Tx3D);
  PROFILE_SCOPE(txt3D);
  SCOPE_GRF(text,0);

  // prepare polygon
  Vertex2DFloat vf[4];
  vf[0].color = attr._color;
  vf[1].color = attr._color;
  vf[2].color = attr._color;
  vf[3].color = attr._color;
  int spec = NoZBuf|IsAlpha|ClampU|ClampV|IsAlphaFog|FilterLinear|attr._specFlags;

  Point2DFloat posA = pos;

  TempWString<> buffer;
  // TODO: Korean support need to be reworked
/*
  if (font->GetLangID() == Korean)
  {
    buffer.Realloc(strlen(text) + 1);
    MultiByteStringToPseudoCodeString(text, buffer);
  }
  else
*/
  {
    // check the size of output buffer
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    buffer.Realloc(bufferSize);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, buffer.Data(), bufferSize);
  }

  bool mapOneToOne = false;
  float sizeH = 1, sizeW = 1;
  const FontWithSize *fontWithSize = SelectFontSize(attr,text,mapOneToOne,sizeH,sizeW);

  // sizeH / sizeW needs to be computed here as needed by 3D rendering
  if (mapOneToOne)
  {
    // we has the width computed by the layout engine - width is primary here
    // this is important when pixel aspect is not exactly 1:1
    sizeW = 1.0f / Width2D();
    sizeH = (4.0f/3) * sizeW;
  }
  else
  {
    sizeH = attr._size / fontWithSize->AbsHeight();
    sizeW = sizeH * 0.75f;
  }
  
  const CharInfo *baseChar = fontWithSize->GetInfo('o');
  int baseWidth = attr._fixedWidth || !baseChar ? fontWithSize->AbsWidth() : baseChar->wTex;

  float spaceWidth = baseWidth * font->GetSpaceWidth();
  float spacing = baseWidth * font->GetSpacing();
  if (mapOneToOne)
  {
    // make the values multiply of sizeW
    spaceWidth = toInt(spaceWidth);
    spacing = toInt(spacing);
  }
  spaceWidth *= sizeW;
  spacing *= sizeW;

  // use font to draw given text
  for (wchar_t *ptr=buffer.Data(); *ptr!=0; ptr++)
  {
    int c = *ptr;
    if (font->GetLangID() == Korean && c >= 0x80)
    {
      /*    TODO: implementation
      text = DrawComposedChar
      (
      posA,
      pars, clip,
      attr._size, attr._size * aspect, font,
      text - 1
      );
      */
      posA.x += spacing;
      continue;
    }

    const CharInfo *info = fontWithSize->GetInfo(c);
    // special handling for both space and non-breakable space here
    if (!info || c==0x0020 || c==0x00a0)
    {
      posA.x += spaceWidth + spacing; // add space width
      continue;
    }


    Ref<Texture> texture = info->tex.GetRef();
    Assert(texture);

    float tx = info->xTex;
    float ty = info->yTex;
    float ah = info->hTex;
    float aw = info->wTex;

    float sha = sizeH*ah;
    float swa = sizeW*aw;
    // texture may be resized - original texture size is _widthPow2
    // only part of texture is actually occupied by character - _width
    MipInfo mip = TextBank()->UseMipmap(texture, 0, 0);
    if (!mip.IsOK()) continue;
    // call wrapped function
    // draw tx,ty,th,tw region of texture
    int texW = 1, texH = 1;
    if (texture)
    {
      texW = texture->AWidth();
      texH = texture->AHeight();
    }

    // for single char texture: tw==texW, th==texH, tx==0, ty==0
    float invTW = 1.0 / texW;
    float invTH = 1.0 / texH;
    
    vf[0].x = posA.x;
    vf[0].y = posA.y;
    vf[0].u = tx * invTW;
    vf[0].v = ty * invTH;

    vf[1].x = posA.x + swa;
    vf[1].y = posA.y;
    vf[1].u = (tx + aw) * invTW;
    vf[1].v = ty * invTH;

    vf[2].x = posA.x + swa;
    vf[2].y = posA.y + sha;
    vf[2].u = (tx + aw) * invTW;
    vf[2].v = (ty + ah) * invTH;

    vf[3].x = posA.x;
    vf[3].y = posA.y + sha;
    vf[3].u = tx * invTW;
    vf[3].v = (ty + ah) * invTH;

    DrawPoly3D(space, mip, vf, 4, clip, spec, attr._mat, pars3D);

    int width = attr._fixedWidth ? fontWithSize->AbsWidth() : info->kw;
    float toAdd = width * sizeW;
    posA.x += toAdd + spacing;
  }
}

void Engine::DrawText3D(const PositionRender &space, const Point2DFloat &pos, const Point2DFloat &right, const Point2DFloat &down, const TextDrawAttr &attr, const char *text, const Rect2DFloat &clip, const Pars3D *pars3D)
{
  if (*text == 0) return;
  Font *font = attr._font;
  if (!font->IsLoaded()) return;

  DPRIM_STAT_SCOPE_SIMPLE(Tx3D);
  PROFILE_SCOPE(txt3D);
  SCOPE_GRF(text,0);

  // prepare polygon
  Vertex2DFloat vf[4];
  vf[0].color = attr._color;
  vf[1].color = attr._color;
  vf[2].color = attr._color;
  vf[3].color = attr._color;
  int spec = NoZBuf|IsAlpha|ClampU|ClampV|IsAlphaFog|FilterLinear|attr._specFlags;

  Point2DFloat posA = pos;

  TempWString<> buffer;
  // TODO: Korean support need to be reworked
/*
  if (font->GetLangID() == Korean)
  {
    buffer.Realloc(strlen(text) + 1);
    MultiByteStringToPseudoCodeString(text, buffer);
  }
  else
*/
  {
    // check the size of output buffer
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    buffer.Realloc(bufferSize);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, buffer.Data(), bufferSize);
  }

  // select fixed font size
  const FontWithSize *fontWithSize = font->Select(SizeFont3D);

  float invHeight = 1.0f / fontWithSize->AbsHeight();
  Point2DFloat sizeW = right * invHeight;
  Point2DFloat sizeH = down * invHeight;

  const CharInfo *baseChar = fontWithSize->GetInfo('o');
  int baseWidth = attr._fixedWidth || !baseChar ? fontWithSize->AbsWidth() : baseChar->wTex;

  Point2DFloat spaceWidth = sizeW * (baseWidth * font->GetSpaceWidth());
  Point2DFloat spacing = sizeW * (baseWidth * font->GetSpacing());

  // use font to draw given text
  for (wchar_t *ptr=buffer.Data(); *ptr!=0; ptr++)
  {
    int c = *ptr;
    if (font->GetLangID() == Korean && c >= 0x80)
    {
      /*    TODO: implementation
      text = DrawComposedChar
      (
      posA,
      pars, clip,
      attr._size, attr._size * aspect, font,
      text - 1
      );
      */
      posA = posA + spacing;
      continue;
    }

    const CharInfo *info = fontWithSize->GetInfo(c);
    // special handling for both space and non-breakable space here
    if (!info || c==0x0020 || c==0x00a0)
    {
      posA = posA + spaceWidth + spacing; // add space width
      continue;
    }

    Ref<Texture> texture = info->tex.GetRef();
    Assert(texture);

    float tx = info->xTex;
    float ty = info->yTex;
    float ah = info->hTex;
    float aw = info->wTex;

    Point2DFloat sha = sizeH * ah;
    Point2DFloat swa = sizeW * aw;

    // texture may be resized - original texture size is _widthPow2
    // only part of texture is actually occupied by character - _width
    MipInfo mip = TextBank()->UseMipmap(texture, 0, 0);
    if (!mip.IsOK()) continue;
    // call wrapped function
    // draw tx,ty,th,tw region of texture
    int texW = 1, texH = 1;
    if (texture)
    {
      texW = texture->AWidth();
      texH = texture->AHeight();
    }

    // for single char texture: tw==texW, th==texH, tx==0, ty==0
    float invTW = 1.0 / texW;
    float invTH = 1.0 / texH;

    vf[0].x = posA.x;
    vf[0].y = posA.y;
    vf[0].u = tx * invTW;
    vf[0].v = ty * invTH;

    vf[1].x = posA.x + swa.x;
    vf[1].y = posA.y + swa.y;
    vf[1].u = (tx + aw) * invTW;
    vf[1].v = ty * invTH;

    vf[2].x = posA.x + swa.x + sha.x;
    vf[2].y = posA.y + swa.y + sha.y;
    vf[2].u = (tx + aw) * invTW;
    vf[2].v = (ty + ah) * invTH;

    vf[3].x = posA.x + sha.x;
    vf[3].y = posA.y + sha.y;
    vf[3].u = tx * invTW;
    vf[3].v = (ty + ah) * invTH;

    DrawPoly3D(space, mip, vf, 4, clip, spec, attr._mat, pars3D);

    int width = attr._fixedWidth ? fontWithSize->AbsWidth() : info->kw;
    posA = posA + sizeW * width + spacing;
  }
}

void Engine::DrawFrame3D(const PositionRender &space, Texture *corner, PackedColor color, const Rect2DFloat &frame, const Rect2DFloat &clip, const Pars3D *pars3D)
{
  MipInfo mip = TextBank()->UseMipmap(corner, 0, 0);
  float invCornerH = 1, invCornerW = 1;
  if (corner)
  {
    invCornerH = 1200.0 / corner->AHeight();
    invCornerW = 1600.0 / corner->AWidth();
  }
  Draw2DParsExt pars;
  Rect2DFloat rect;
  rect.w = 0.5 * frame.w;
  rect.h = 0.5 * frame.h;
  pars.mip = mip;
  pars.SetColor(color);
  pars.spec = NoZBuf|IsAlpha|ClampU|ClampV|IsAlphaFog;
  float coefX = 0.5 * frame.w * invCornerW;
  float coefY = 0.5 * frame.h * invCornerH;

  rect.x = frame.x;
  rect.y = frame.y;
  pars.SetU(0,coefX);
  pars.SetV(0,coefY);
  Draw3D(space, pars, rect, clip, pars3D);

  rect.x = frame.x + rect.w;
  rect.y = frame.y;
  pars.SetU(coefX,0);
  pars.SetV(0,coefY);
  Draw3D(space, pars, rect, clip, pars3D);

  rect.x = frame.x;
  rect.y = frame.y + rect.h;
  pars.SetU(0,coefX);
  pars.SetV(coefY,0);
  Draw3D(space, pars, rect, clip, pars3D);

  rect.x = frame.x + rect.w;
  rect.y = frame.y + rect.h;
  pars.SetU(coefX,0);
  pars.SetV(coefY,0);
  Draw3D(space, pars, rect, clip, pars3D);
}

#include "txtPreload.hpp"

void Engine::DrawLine3D(
  const PositionRender &space, const Line2DFloat &rect, PackedColor c0, PackedColor c1, const Rect2DFloat &clip, float width, int spec, const TLMaterial &mat,
  const Pars3D *pars3D
)
{
  float lineWidth;
  if (width>0) lineWidth = width / Width2D();
  else lineWidth = -width;

  float dx = rect.end.x - rect.beg.x;
  float dy = rect.end.y - rect.beg.y;
  float sq = Square(dx) + Square(dy);
  float pdx;
  float pdy;
  if (sq <= 0.0f)
  {
    pdx = lineWidth;
    pdy = -lineWidth;
  }
  else
  {
    float invSize = InvSqrt(sq);
    pdx = dy * invSize * lineWidth;
    pdy = -dx * invSize * lineWidth;
  }
  Vertex2DFloat vf[4];
  vf[0].x = rect.beg.x - pdx;
  vf[0].y = rect.beg.y - pdy;
  vf[0].u = 0;
  vf[0].v = 0;
  vf[0].color = c0;

  vf[1].x = rect.beg.x + pdx;
  vf[1].y = rect.beg.y + pdy;
  vf[1].u = 0;
  vf[1].v = 1;
  vf[1].color = c0;

  vf[2].x = rect.end.x + pdx;
  vf[2].y = rect.end.y + pdy;
  vf[2].u = 1;
  vf[2].v = 1;
  vf[2].color = c1;

  vf[3].x = rect.end.x - pdx;
  vf[3].y = rect.end.y - pdy;
  vf[3].u = 1;
  vf[3].v = 0;
  vf[3].color = c1;

  Texture *tex = GPreloadedTextures.New(TextureLine3D);
  MipInfo mip = TextBank()->UseMipmap(tex, 1, 1);
  int specFlags = NoZWrite|IsAlpha|ClampU|ClampV|IsAlphaFog|spec;

  DrawPoly3D(space, mip, vf, 4, clip, specFlags, mat, pars3D);
}

void Engine::DrawLines3D(const PositionRender &space, const Line2DFloatInfo *lines, int nLines, const Rect2DFloat &clip, float width, int spec, const TLMaterial &mat, const Pars3D *pars3D)
{
  float lineWidth;
  if (width>0) lineWidth = width / Width2D();
  else lineWidth = -width;

  Texture *tex = GPreloadedTextures.New(TextureLine3D);
  MipInfo mip = TextBank()->UseMipmap(tex, 1, 1);
  int specFlags = NoZWrite|IsAlpha|ClampU|ClampV|IsAlphaFog|spec;

  for (int i=0; i<nLines; i++)
  {
    const Line2DFloatInfo &line = lines[i];

    float dx = line.end.x - line.beg.x;
    float dy = line.end.y - line.beg.y;
    float invSize = InvSqrt(Square(dx) + Square(dy));
    float pdx = dy * invSize * lineWidth;
    float pdy = -dx * invSize * lineWidth;

    Vertex2DFloat vf[4];

    vf[0].x = line.beg.x - pdx;
    vf[0].y = line.beg.y - pdy;
    vf[0].u = 0;
    vf[0].v = 0;
    vf[0].color = line.c0;

    vf[1].x = line.beg.x + pdx;
    vf[1].y = line.beg.y + pdy;
    vf[1].u = 0;
    vf[1].v = 1;
    vf[1].color = line.c0;

    vf[2].x = line.end.x + pdx;
    vf[2].y = line.end.y + pdy;
    vf[2].u = 1;
    vf[2].v = 1;
    vf[2].color = line.c1;

    vf[3].x = line.end.x - pdx;
    vf[3].y = line.end.y - pdy;
    vf[3].u = 1;
    vf[3].v = 0;
    vf[3].color = line.c1;

    DrawPoly3D(space, mip, vf, 4, clip, specFlags, mat, pars3D);
  }
}

void Engine::DrawPoly3D(const PositionRender &space, const MipInfo &mip, const Vertex2DFloat *vertices, int nVertices, const Rect2DFloat &clip, int specFlags, const TLMaterial &mat, const Pars3D *pars3D)
{
}

void Engine::DrawHorizontalArrow(Vector3Par connection, Vector3Par end,PackedColor colorLinkLine, float width, int spec, float arrowSize)
{
                
  Vector3 dir = (end-connection).Normalized();
  Vector3 aside = dir.CrossProduct(VUp).Normalized();
  float sizeD = 0.3f*arrowSize;
  float sizeA = 0.2f*arrowSize;
  Vector3 arrow1 = end-dir*sizeD-aside*sizeA;
  Vector3 arrow2 = end-dir*sizeD+aside*sizeA;
  Vector3 arr[5];
  arr[0] = connection;
  arr[1] = end;
  arr[2] = arrow1;
  arr[3] = arrow2;
  arr[4] = end;
  enf::Shape::CreateLines(arr, 5, colorLinkLine, enf::SS_ONCE|enf::SS_TRANSP);
}

void Engine::DrawVerticalArrow(Vector3Par connection, Vector3Par end,PackedColor colorLinkLine, float width, int spec, float arrowSize)
{
  Vector3 dir = (end-connection).Normalized();
  Vector3 aside = dir.CrossProduct(VUp).Normalized();
  Vector3 asideUp = aside.CrossProduct(dir).Normalized();
  float sizeD = 0.3f*arrowSize;
  float sizeA = 0.2f*arrowSize;
  Vector3 arrow1 = end-dir*sizeD-asideUp*sizeA;
  Vector3 arrow2 = end-dir*sizeD+asideUp*sizeA;
  Vector3 arr[5];
  arr[0] = connection;
  arr[1] = end;
  arr[2] = arrow1;
  arr[3] = arrow2;
  arr[4] = end;
  enf::Shape::CreateLines(arr, 5, colorLinkLine, enf::SS_ONCE|enf::SS_TRANSP);
}
