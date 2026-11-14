#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TRACKIR_HPP
#define _TRACKIR_HPP

#include <Es/Common/win.h>
#include <Es/Types/pointers.hpp>
#include <Es/Strings/rString.hpp>

//! TrackIR support encapsulation
const float TrackIrMaxValue = 16383.0f;
class TrackIR
{
private:
  RString GetDllLocation();
  unsigned long	npFrameSignature;
  unsigned long	npStaleFrames;
  RString gcsDLLPath;
  bool trackIRActive;
  bool enabled;
  int trackIRid, appKeyHigh, appKeyLow; //need for registration to proper application
  bool isWorking;

public:
  enum
  {
    NAxes=6, // pitch, yaw, roll, x, y, z
    TRACKIR_PITCH=0,
    TRACKIR_YAW,
    TRACKIR_ROLL,
    TRACKIR_X,
    TRACKIR_Y,
    TRACKIR_Z
  };
  float axisOld[NAxes];

 //static const int _defTholds[32][2];
  TrackIR(int trIRid, int keyHigh, int keyLow);
  ~TrackIR();
  void Init(HWND hwnd);
  void Enable(bool val);
  bool IsEnabled() const { return enabled; }
  bool IsActive() const { return trackIRActive; }
  bool IsWorking() const { return isWorking && IsEnabled(); }
  
  bool Get(float axis[NAxes]);
};

//! global TrackIR instance
extern SRef<TrackIR> TrackIRDev;

#endif
