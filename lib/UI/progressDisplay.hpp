#ifdef _MSC_VER
#pragma once
#endif

#include "../progress.hpp"
#include "uiControls.hpp"

#ifndef _PROGRESS_DISPLAY_HPP
#define _PROGRESS_DISPLAY_HPP

class ProgressDisplay : public Display, public IProgressDisplay
{
public:
  ProgressDisplay(ControlsContainer *parent) : Display(parent) {}
  virtual void SetProgress(DWORD time, DWORD current, DWORD total);
  virtual void DrawProgress(float alpha);

  // reference counting
  virtual int DisplayAddRef() {return AddRef();}
  virtual int DisplayRelease() {return Release();}
};

#endif
