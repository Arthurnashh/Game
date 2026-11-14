#include "../wpch.hpp"

#include "progressDisplay.hpp"
#include "resincl.hpp"
#include <El/Common/randomGen.hpp>

#include "../world.hpp"
#include "../Network/network.hpp"

#include "missionDirs.hpp"

void ProgressDisplay::SetProgress(DWORD time, DWORD current, DWORD total)
{
  IControl *ctrl = GetCtrl(IDC_PROGRESS_TIME);
  if (ctrl)
  {
    if (ctrl->GetType() == CT_PROGRESS)
    {
      float factor = fastFmod(time * (1.0f / 3000), 2);
      if (factor > 1) factor = 2 - factor;
      CProgressBar *bar = static_cast<CProgressBar *>(ctrl);
      bar->SetRange(0, 1);
      bar->SetPos(factor);
    }
    else if (ctrl->GetType() == CT_ANIMATED_TEXTURE)
    {
      float factor = fastFmod(time * (1.0f / 3000), 1);
      CAnimatedTexture *tex = static_cast<CAnimatedTexture *>(ctrl);
      tex->SetPhase(factor);
    }
  }

  ctrl = GetCtrl(IDC_PROGRESS_PROGRESS);
  if (ctrl && ctrl->GetType() == CT_PROGRESS)
  {
    if (total > 0)
    {
      ctrl->ShowCtrl(true);
      CProgressBar *bar = static_cast<CProgressBar *>(ctrl);
      bar->SetRange(0, total);
      bar->SetPos(current);
    }
    else
    {
      ctrl->ShowCtrl(false);
    }
  }
}

void ProgressDisplay::DrawProgress(float alpha)
{
  Display::DrawHUD(NULL, alpha);
}

class ProgressDisplayLoadIsland : public ProgressDisplay
{
protected:
  AutoArray<RString> _entries;
  AutoArray<int> _indices;
  DWORD _nextChange;

  RString _text;
  RString _date;

public:
  ProgressDisplayLoadIsland(ControlsContainer *parent, RString text, RString date);
  virtual void SetProgress(DWORD time, DWORD current, DWORD total);

protected:
  void NextDisplay();
};

ProgressDisplayLoadIsland::ProgressDisplayLoadIsland(ControlsContainer *parent, RString text, RString date)
: ProgressDisplay(parent)
{
  _text = text;
  _date = date;

  ParamEntryVal list = Pars >> "RscDisplayLoading" >> "Variants";
  int n = list.GetEntryCount();
  for (int i=0; i<n; i++)
    _entries.Add(list.GetEntry(i).GetName());

  NextDisplay();
}

void ProgressDisplayLoadIsland::SetProgress(DWORD time, DWORD current, DWORD total)
{
  if (GlobalTickCount() >= _nextChange) NextDisplay();
  ProgressDisplay::SetProgress(time, current, total);
}

void ProgressDisplayLoadIsland::NextDisplay()
{
  int n = _entries.Size();
  if (n > 0)
  {
    if (_indices.Size() == 0)
    {
      // all variants used, show again
      _indices.Realloc(n);
      _indices.Resize(n);
      for (int i=0; i<n; i++) _indices[i] = i;
    }

    int m = _indices.Size();
    int i = toIntFloor(m * GRandGen.RandomValue());
    saturate(i, 0, m - 1);
    ParamEntryVal entry = Pars >> "RscDisplayLoading" >> "Variants" >> _entries[_indices[i]];
    Init();
    LoadFromEntry(entry);

    SetCursor(NULL);
    CStatic *ctrl = static_cast<CStatic *>(GetCtrl(IDC_LOAD_MISSION_NAME));
    if (ctrl) ctrl->SetText(_text);
    ctrl = static_cast<CStatic *>(GetCtrl(IDC_LOAD_MISSION_DATE));
    if (ctrl) ctrl->SetText(_date);

    _indices.Delete(i);
  }

 
  RString name = GetMissionDirectory() + RString("description.ext");
  // mission parameters file
  GameDataNamespace globals(NULL, RString(), false); // TODO: parsing namespace if access to globals needed

  ParamFile missionParams;
  missionParams.Parse(name, NULL, NULL, &globals);

  CStatic *ctrl = static_cast<CStatic *>(GetCtrl(IDC_LOAD_MISSION_PICTURE));
  if (ctrl && missionParams.FindEntry("loadScreen"))  
  {
    RString texture = missionParams >> "loadScreen";
    ctrl->SetText(texture);
  }

  CHTMLContainer *html = GetHTMLContainer(GetCtrl(IDC_LOAD_MISSION_TEXT));
  if (html && missionParams.FindEntry("loadText")) 
  {
    RString text = missionParams >> "loadText";
    html->Load(GetMissionDirectory() + text);
  }
  
  _nextChange = GlobalTickCount() + 10000;
}

IProgressDisplay *CreateDisplayLoadIsland(RString text, RString date)
{
  return new ProgressDisplayLoadIsland(NULL, text, date);
}

class ProgressDisplayLoadMission : public ProgressDisplay
{
protected:
  AutoArray<RString> _sections;
  int _current;

  InitPtr<CHTMLContainer> _briefing;
  DWORD _nextChange;

public:
  ProgressDisplayLoadMission(ControlsContainer *parent, RString filename, RString text, RString date);
  virtual void SetProgress(DWORD time, DWORD current, DWORD total);
  bool IsValid() const {return !_briefing || _sections.Size() > 0;}

protected:
  void NextDisplay();
};

#if _VBS3
  #include "../hla/AAR.hpp"
#endif

ProgressDisplayLoadMission::ProgressDisplayLoadMission(ControlsContainer *parent, RString filename, RString text, RString date)
: ProgressDisplay(parent)
{
  Load("RscDisplayLoadMission");

  SetCursor(NULL);
  CStatic *ctrl = static_cast<CStatic *>(GetCtrl(IDC_LOAD_MISSION_NAME));
  if (ctrl) ctrl->SetText(text);
  ctrl = static_cast<CStatic *>(GetCtrl(IDC_LOAD_MISSION_DATE));
  if (ctrl) ctrl->SetText(date);

  IControl *control = GetCtrl(IDC_BRIEFING);
  if (control) control->EnableCtrl(false);
  _briefing = GetHTMLContainer(control);
  if (_briefing)
  {
    _briefing->Load(filename);
#if _VBS3 && _EXT_CTRL
    if(GAAR.IsReplayMode()) _briefing->Parse(GAAR.GetBriefingData());
#endif

    TargetSide side = Glob.header.playerSide;

    int section = _briefing->FindSection("Plan");
    if (section >= 0)
    {
      bool UpdatePlan(CHTMLContainer *briefing, TargetSide side, AIBrain *unit);
      UpdatePlan(_briefing, side, NULL);
      _sections.Add("__PLAN");
    }

    int FindSectionForUnit(HTMLContent &html, RString sectionName, TargetSide side, AIBrain *unit);
    section = FindSectionForUnit(*_briefing, "Main", side, NULL);
    if (section >= 0)
    {
      _briefing->AddName(section, "__BRIEFING");
      _sections.Add("__BRIEFING");
    }
  }

  _current = 0;
  if (IsValid()) NextDisplay();
}

void ProgressDisplayLoadMission::SetProgress(DWORD time, DWORD current, DWORD total)
{
  if (GlobalTickCount() >= _nextChange) NextDisplay();
  ProgressDisplay::SetProgress(time, current, total);
}

void ProgressDisplayLoadMission::NextDisplay()
{
  if (_briefing)
  {
    if (_current >= _sections.Size()) _current = 0;
    _briefing->SwitchSection(_sections[_current]);
    _current++;
  }
  _nextChange = GlobalTickCount() + 10000;

  RString name = GetMissionDirectory() + RString("description.ext");
  // mission parameters file
  GameDataNamespace globals(NULL, RString(), false); // TODO: parsing namespace if access to globals needed

  ParamFile missionParams;
  missionParams.Parse(name, NULL, NULL, &globals);

  CStatic *ctrl = static_cast<CStatic *>(GetCtrl(IDC_LOAD_MISSION_PICTURE));
  if (ctrl && missionParams.FindEntry("loadScreen"))  
  {
    RString texture = missionParams >> "loadScreen";
    ctrl->SetText(texture);
  }

  CHTMLContainer *html = GetHTMLContainer(GetCtrl(IDC_LOAD_MISSION_TEXT));
  if (html && missionParams.FindEntry("loadText")) 
  {
    RString text = missionParams >> "loadText";
    html->Load(GetMissionDirectory() + text);
  }
}

/*!
\patch 5153 Date 4/16/2007 by Jirka
- Fixed: Mission loading screen - missing text given in onLoadMission
*/

ProgressDisplay *CreateDisplayLoadMission(RString filename, RString text, RString date)
{
  ProgressDisplayLoadMission *disp = new ProgressDisplayLoadMission(NULL, filename, text, date);
  if (!disp->IsValid())
  {
    delete disp;
    return NULL;
  }
  return disp;
}

IProgressDisplay *CreateDisplayProgress(RString text, RString resource)
{
  ProgressDisplay *disp = new ProgressDisplay(NULL);
  
  // search for resource also in the description.ext of campaign and mission
  ConstParamEntryPtr FindResource(RString name);
  ConstParamEntryPtr cls = FindResource(resource);
  if (cls) disp->LoadFromEntry(*cls);
  
  disp->SetCursor(NULL);
  CStatic *ctrl = static_cast<CStatic *>(disp->GetCtrl(IDC_LOAD_MISSION_NAME));
  if (ctrl) ctrl->SetText(text);
  return disp;
}

IProgressDisplay *CreateDisplayProgress(RString text)
{
  return CreateDisplayProgress(text, "RscDisplayNotFreeze");
}
