/******************************************************************************
FILE: DXInput.h
******************************************************************************/
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _JOYSTICK_HPP
#define _JOYSTICK_HPP

#include <Es/Common/win.h>
#include <El/Enum/enumNames.hpp>
#include "keyInput.hpp"

#if defined(_WIN32) 
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
# include <XInput.h>
#endif

#define JOYSTICK_TYPE_ENUM(type, prefix, XX) \
  XX(type, prefix, Gamepad) \
  XX(type, prefix, Joystick) \
  XX(type, prefix, Wheel)

#ifndef DECL_ENUM_JOYSTICK_TYPE_ENUM
# define DECL_ENUM_JOYSTICK_TYPE_ENUM
DECL_ENUM(JoystickType)
#endif

DECLARE_ENUM(JoystickType, JT, JOYSTICK_TYPE_ENUM)

struct JoyAxisInfo
{
  bool    active;
  RString name;
  WORD usagePage, usage;
  JoyAxisInfo() : active(false) {};
};

/// Joystick interface - common for DirectInput and XInput implementations
class IJoystick : public RefCount
{
public:
  enum
  {
    NAxes=8, // 6 regular and two sliders
    // enable usage of Xbox controller on PC
    NButtons=32
  };

  IJoystick() {}
  virtual ~IJoystick() {}

#if defined (_WIN32)
  virtual void Init(HINSTANCE instance, HWND hwnd, IDirectInput8 *dInput = NULL) {};
#else
  virtual void Init() = 0;
#endif
  virtual void InitScheme(ParamEntryPar entry) {}

  // hat -- angle in degrees * 1000 (90000 , 180000 ...)
  virtual bool Get(
    int axis[NAxes], unsigned short button[NButtons], bool buttonEdge[NButtons], bool buttonUpEdge[NButtons],
    bool buttonState[NButtons],
    int &hatAngle) = 0;

  // Force feedback functions
  virtual void SetStiffness(float valueX, float valueY) = 0; // how much autocentering is applied
  virtual void SetEngine(float mag, float freq) = 0; // set constant sine
  virtual void SetSurface(float mag) = 0; // set constant sine

  // used to simulate weapons
  virtual void PlayRamp(float begMag, float endMag, float duration) = 0; // play ramp

  virtual void FFOn() = 0;
  virtual void FFOff() = 0;

  virtual JoystickType GetType() const {return JTJoystick;}
  /// Return mask of types of all connected controllers
  virtual int GetTypes() const {return 0;}
#ifdef _WIN32
  virtual XINPUT_GAMEPAD GetAvailable(RString type) const
  {
    XINPUT_GAMEPAD avail;
    memset(&avail, 0, sizeof(avail));
    return avail;
  }
#endif

  virtual int GetActiveController() const {return -1;}
  virtual int LockController() {return 0;}
  virtual int UnlockController() {return 0;}
  virtual bool IsControllerLocked() const {return false;}
  virtual int GetAxesCount() const {return NAxes;}

  virtual const JoyAxisInfo* GetJoyAxesInfo() const {return NULL;};

  virtual void Enable(bool enable) = 0;
  virtual bool IsEnabled() const = 0;

  virtual void SetDeadZone(const AutoArray<float> &deadZone) {}

  virtual int GetOffset() const {return 0;}
};

class JoystickDevices : public RefArray<IJoystick>
{
  typedef RefArray<IJoystick> base;
public:
  static float DeadZoneDefaults[N_JOYSTICK_AXES];

  struct RecognizedJoystick
  {
    RString guid;
    RString name;
    int  offset;
    AutoArray<float> sensitivity;
    AutoArray<float> deadZone;
    AutoArray<float> steepness;
    JoystickMode mode;
    bool isXInput;
    ClassIsMovableZeroed(RecognizedJoystick);
  };
private:
#if defined (_WIN32)
  ComRef<IDirectInput8> _dInput; //  allow external DirectInput object
#endif
  // Joysticks which the game already knows (their offset was assigned already)
  AutoArray<RecognizedJoystick> _recognizedJoysticks;

public:
  enum
  {
    OffsetStep=256
  };

  JoystickDevices();
  ~JoystickDevices();

#if defined (_WIN32)
  void Init(HINSTANCE instance, HWND hwnd, IDirectInput8 *dInput = NULL);
  // DI Callback
  BOOL AddJoy(LPCDIDEVICEINSTANCE lpddi, HWND hwnd);
#endif

  // ParamFile serialization
  void SaveJoysticks();
  void SaveJoysticks(ParamFile &userCfg);
  void LoadJoysticks(ParamFile &userCfg);

  int GetJoystickOffset(const GUID &guid, const char *productName, bool isXInput);
  Array<float> GetSensitivity(int offset) const;
  void SetSensitivity(int offset, const AutoArray<float> &sensitivity);
  bool IsXInput(int offset) const;
  bool XInputConnected() const;
  RecognizedJoystick* GetJoystickInfo(int offset);
  void SetDeadZone(int joyIx, const AutoArray<float>& deadZone);
  void ReloadDeadZone(int joyIx, const AutoArray<float>& deadZone);  //resets the DInput joystick DeadZones to the joystickState values
  AutoArray<float> GetDeadZone(int offset) const;
  void SetSteepness(int joyIx, const AutoArray<float>& steepness);
  AutoArray<float> GetSteepness(int offset) const;

  // Force feedback functions
  void SetStiffness(float valueX, float valueY); // how much autocentering is applied
  void SetEngine(float mag, float freq); // set constant sine
  void SetSurface(float mag); // set constant sine

  // used to simulate weapons
  void PlayRamp(float begMag, float endMag, float duration); // play ramp

  void FFOn();
  void FFOff();

private:
  bool _cleanupCOM;
};

/// global joysticks instance
extern SRef<JoystickDevices> GJoystickDevices;
/// global XInput instance
extern SRef<IJoystick> XInputDev;
/// DirectInput Joysticks devices creation
JoystickDevices *CreateJoysticks();
/// XInput device creation
IJoystick *CreateXInput();

int GetActiveController();
bool IsControllerLocked();
int LockController();
int UnlockController();

#endif //#ifndef _JOYSTICK_HPP
