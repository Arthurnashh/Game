#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FREETRACK_HPP
#define _FREETRACK_HPP

#include <Es/Common/win.h>
#include <Es/Types/pointers.hpp>
#include <Es/Strings/rString.hpp>

#include "freeTrackClient.hpp"

//! TrackIR support encapsulation
class FreeTrack
{
private:
  RString GetDllLocation();
  RString dllPath;
  bool freeTrackActive;
  bool enabled;
  FreeTrackClient freeTrackClient;

public:
  enum
  {
    NAxes=6, // pitch, yaw, roll, x, y, z
    FREETRACK_PITCH=0,
    FREETRACK_YAW,
    FREETRACK_ROLL,
    FREETRACK_X,
    FREETRACK_Y,
    FREETRACK_Z
  };
  float axisOld[NAxes];

  FreeTrack();
  void Init();
  void Enable(bool val);
  bool IsEnabled() const { return enabled; }
  bool IsActive() const { return freeTrackActive; }

  bool Get(float axis[NAxes]);
};

//! global FreeTrack instance
extern SRef<FreeTrack> FreeTrackDev;

#endif
