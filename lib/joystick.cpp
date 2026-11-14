/******************************************************************************
FILE: DXInput.cpp
This module contains all of the Direct Input specific code.  This includes
initialization and uninitializatoin functions as well as functions to get
joystick and keyboard information, and a function to cause force feedback
events.
*/

#define _WIN32_DCOM

#include "wpch.hpp"

#include <El/Common/perfProf.hpp>

#include <El/ParamFile/paramFile.hpp>
extern ParamFile Pars;
#include <El/Evaluator/expressImpl.hpp>

#include "joystick.hpp"
#include "trackIR.hpp"
#include "freeTrack.hpp"
#include "keyInput.hpp"
#include "global.hpp"
#include "diagModes.hpp"

#if _DISABLE_GUI
  #define ENABLE_DI_JOYSTICK 0
#else
  #define ENABLE_DI_JOYSTICK 1
#endif

#if _DISABLE_GUI
  #define ENABLE_JOYSTICK 0
#else
  #define ENABLE_JOYSTICK 1
#endif

static const EnumName JoystickModeNames[]=
{
  EnumName(JMCustom, "Custom"),
  EnumName(JMDisabled, "Disable"),
  EnumName()
};

template<>
const EnumName *GetEnumNames( JoystickMode dummy )
{
  return JoystickModeNames;
}

/// PC implementation of joystick
#if ENABLE_DI_JOYSTICK

class Joystick : public IJoystick
{
private:
  int _offset;  // offset to recognize this joysticks amongst others
  bool _enabled;

  DIDEVCAPS _capabilities;

  static const int _defTholds[32][2];

  ComRef<IDirectInputDevice8> _devJoystick;

  ComRef<IDirectInputEffect> _effCenter;
  ComRef<IDirectInputEffect> _effWeapon;
  ComRef<IDirectInputEffect> _effEngine;

  int _nCenterAxes;

  bool _effEngineStarted;
  bool _effWeaponStarted;

  bool _weaponByConstantForce;
  bool _buttonState[NButtons];
  bool _ffEnabled;
  bool _ffOn;

  float _stiffnessX,_stiffnessY;
  float _engineMag,_engineFreq;
  float _surfaceMag;

  JoyAxisInfo _axesInfos[NAxes];

public:
  Joystick();
  ~Joystick();

  void Init(LPCDIDEVICEINSTANCE lpddi, IDirectInputDevice8* devJoy, HWND hwnd, bool isXInput);

  // hat -- angle in degrees * 1000 (90000 , 180000 ...)
  bool Get(
    int axis[NAxes], unsigned short button[NButtons], bool buttonEdge[NButtons], bool buttonUpEdge[NButtons],
    bool buttonState[NButtons],
    int &hatAngle);

  void SetStiffness(float valueX, float valueY); // how much autocentering is applied
  void SetEngine( float mag, float freq ); // set constant sine
  void SetSurface( float mag ); // set constant sine

  // used to simulate weapons
  void PlayRamp(float begMag, float endMag, float duration); // play ramp

  void FFStart();
  void FFStop();

  void FFOn();
  void FFOff();

  void Enable(bool enable);
  bool IsEnabled() const { return _enabled; };

  int GetOffset() const {return _offset;}
  int GetAxesCount() const {return _capabilities.dwAxes;}

  const JoyAxisInfo* GetJoyAxesInfo() const {return _axesInfos;};
  JoystickMode GetMode();

  void SetDeadZone(const AutoArray<float> &deadZone);

protected:
  void InitForceFeedback();
  void InitEffects();

  // environment
  void DoSetStiffness(float valueX, float valueY); // how much autocentering is applied
  void DoSetEngine( float mag, float freq ); // set constant sine
  void DoSetSurface( float mag ); // set constant sine

  bool CheckStickActive();
  bool CheckStickActive(int joyIx);
};

JoystickDevices *CreateJoysticks()
{
#if _DISABLE_GUI
  return NULL;
#else

# if ENABLE_DI_JOYSTICK
  return new JoystickDevices;
# else
  return NULL;
# endif

#endif
}

Joystick *CreateJoystick()
{
#if _DISABLE_GUI
  return NULL;
#else

# if ENABLE_DI_JOYSTICK
  return new Joystick;
# else
  return NULL;
# endif

#endif
}

struct EnumContext
{
  JoystickDevices *system;
  HWND hwnd;
};

BOOL CALLBACK EnumJoyCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);

/******************************************************************************
FUNCTION: InitDirectInput
This is where the actual COM initialization occurs.  Also, the devices are
created here.

PARAMETERS:
None

RETURNS:
Success or Failure
******************************************************************************/

/*!
\patch_internal 1.01 Date 6/25/2001 by Ondra.
- Joystick support disabled.
*/

Joystick::Joystick()
{
  _engineMag = 0;
  _engineFreq = 0;
  _stiffnessX = 0;
  _stiffnessY = 0;
  _surfaceMag = 0;
  _ffEnabled = true;
  _offset = 0;
}

#define DEF_BUTTON_THOLD {64*128,48*128}
#define DEF_STICK_THOLD {128*128,96*128}
#define DEF_TRIGGER_THOLD {192*128,128*128}
const int Joystick::_defTholds[32][2]=
{
  // different button have different thresholds
  // when they are considered to be pressed
  // normal button thresholds
  DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD, // A,B,X,Y
  DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD, // DPad Up,Down,Left,Right
  DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD, // START, BACK, Black, White
  DEF_TRIGGER_THOLD,DEF_TRIGGER_THOLD, // triggers (L,R)
  DEF_BUTTON_THOLD,DEF_BUTTON_THOLD, // thumbstick clicks (L,R)
  // stick thresholds
  DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD, // L Right, L Up, R Right, R Up
  DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD, // 
  DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,
  DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,
};

BOOL WINAPI JoyAxisCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
  JoyAxisInfo *axesInfos = static_cast<JoyAxisInfo *>(pvRef);
  int ix = -1;
  if (lpddoi->guidType==GUID_XAxis) ix = 0; //X
  if (lpddoi->guidType==GUID_YAxis) ix = 1; //Y
  if (lpddoi->guidType==GUID_ZAxis) ix = 2; //Z
  if (lpddoi->guidType==GUID_RxAxis) ix = 3; //rX
  if (lpddoi->guidType==GUID_RyAxis) ix = 4; //rY
  if (lpddoi->guidType==GUID_RzAxis) ix = 5; //rZ
  if (lpddoi->guidType==GUID_Slider) 
  {
    if (axesInfos[6].active) ix = 7;
    else ix = 6;
  }
  if (ix>=0)
  {
    // Rrrola: properly convert system codepage to UTF-8
    wchar_t name_utf16[MAX_PATH*2];
    MultiByteToWideChar(CP_ACP, 0, lpddoi->tszName, -1, name_utf16, MAX_PATH*2);
    char name_utf8[MAX_PATH*6];
    WideCharToMultiByte(CP_UTF8, 0, name_utf16, -1, name_utf8, MAX_PATH*6, 0, 0);

    axesInfos[ix].name = name_utf8;
    axesInfos[ix].usagePage = lpddoi->wUsagePage;
    axesInfos[ix].usage = lpddoi->wUsage;
    axesInfos[ix].active = true;
  }
  else 
  {
    RptF("Invalid Joystick Axis!");
    RptF("  type=%x (%x), dwOfs=%d, name=%s", lpddoi->dwType, DIDFT_GETINSTANCE(lpddoi->dwType), lpddoi->dwOfs, lpddoi->tszName);
    RptF("  GUID = %x, %x, %x, ...", lpddoi->guidType.Data1, lpddoi->guidType.Data2, lpddoi->guidType.Data3);
    RptF("  wUsagePage = %x, wUsage = %x", lpddoi->wUsagePage, lpddoi->wUsage);
  }
  return DIENUM_CONTINUE;
}

// TODO: this would be better to configure in configs
static const char *PedalDeviceList[] = {"CH PRO PEDALS USB "}; // "logitech g940 pedals" <- does not work for it, as it has also reversed values +100-->-100
static bool IsJoystickPedal(const char *name)
{
  RString nameLC(name);
  nameLC.Lower();
  for (int i=0; i<sizeof(PedalDeviceList)/sizeof(*PedalDeviceList); i++)
  {
    if (stricmp(nameLC, PedalDeviceList[i])==0) return true; // it happens the pedals does not follow any standard (some has range -x..+x, others 0..+x)
  }
  return false;
}

void Joystick::Init(LPCDIDEVICEINSTANCE lpddi, IDirectInputDevice8* devJoy, HWND hwnd, bool isXInput)
{
  //@{ HOTFIX: _devJoystick->GetCapabilities calls CoInitializeEx which fails, but the 
  //           corresponding CoUninitialize is called either (which should not).
  //           see: news:gh8c44$4e9$1@new-server.localdomain
  CoInitializeEx(NULL, COINIT_MULTITHREADED); //hotfix
  //@}

  // This cause crash, because devJoy has zero references after CreateDevice
  //   _dInput->CreateDevice(lpddi->guidInstance, &devJoystick, NULL)
  // and while destroying Joystick, Release on devJoy crash. <- fix coppied from Futura project rev.90449, news:jj7hr0$r75$1@new-server.localdomain
  // ^^^ _devJoystick << devJoy;
  _devJoystick += devJoy;

  for (int i=0; i<NButtons; i++) _buttonState[i] = false; 

  _enabled = (!isXInput || (isXInput && GInput.customScheme));

  // We are setting the data format for the device.  c_dfDIJoystick is a
  // predefined structure that should be used for joysticks.  It is defined
  // in dinput.lib
  _devJoystick->SetDataFormat(&c_dfDIJoystick);

  // We are setting the cooperative level to exclusive/foreground for this
  // object.  This means that as long as our app's window is in the
  // foreground, we have exclusive control of the device.  Consider using
  // exclusive background for debugging purposes.
  //_devJoystick->SetCooperativeLevel(hwnd,DISCL_EXCLUSIVE|DISCL_BACKGROUND);
  _devJoystick->SetCooperativeLevel(hwnd,DISCL_EXCLUSIVE|DISCL_FOREGROUND);

  // The SetProperty function of the IDirectInputDevice2 interface allows you
  // to set different settings for the device.  Here we are setting the min
  // and max ranges for the stick to -100 through 100 for the X and Y axes
  DIPROPRANGE          dipRange;
  dipRange.diph.dwSize       = sizeof(dipRange);
  dipRange.diph.dwHeaderSize = sizeof(dipRange.diph);
  dipRange.lMin              = -100;
  dipRange.lMax              = +100;
  dipRange.diph.dwHow        = DIPH_BYOFFSET;

  DIPROPRANGE          dipRangePedals;
  dipRangePedals.diph.dwSize       = sizeof(dipRangePedals);
  dipRangePedals.diph.dwHeaderSize = sizeof(dipRangePedals.diph);
  dipRangePedals.lMin              = 0;
  dipRangePedals.lMax              = 100;
  dipRangePedals.diph.dwHow        = DIPH_BYOFFSET;

  bool joyIsPedal = IsJoystickPedal(lpddi->tszProductName);

  if (joyIsPedal)
  {
    dipRangePedals.diph.dwObj        = DIJOFS_X;
    _devJoystick->SetProperty( DIPROP_RANGE, &dipRangePedals.diph);

    dipRangePedals.diph.dwObj        = DIJOFS_Y;
    _devJoystick->SetProperty( DIPROP_RANGE, &dipRangePedals.diph);
  }
  else
  {
    dipRange.diph.dwObj        = DIJOFS_X;
    _devJoystick->SetProperty( DIPROP_RANGE, &dipRange.diph);

    dipRange.diph.dwObj        = DIJOFS_Y;
    _devJoystick->SetProperty( DIPROP_RANGE, &dipRange.diph);
  }

  dipRange.diph.dwObj        = DIJOFS_Z;
  _devJoystick->SetProperty( DIPROP_RANGE, &dipRange.diph);

  dipRange.diph.dwObj        = DIJOFS_RX;
  _devJoystick->SetProperty( DIPROP_RANGE, &dipRange.diph);

  dipRange.diph.dwObj        = DIJOFS_RY;
  _devJoystick->SetProperty( DIPROP_RANGE, &dipRange.diph);

  dipRange.diph.dwObj        = DIJOFS_RZ;
  _devJoystick->SetProperty( DIPROP_RANGE, &dipRange.diph);

  dipRange.diph.dwObj        = DIJOFS_SLIDER(0);
  _devJoystick->SetProperty( DIPROP_RANGE, &dipRange.diph);

  dipRange.diph.dwObj        = DIJOFS_SLIDER(1);
  _devJoystick->SetProperty( DIPROP_RANGE, &dipRange.diph);

#define ENABLE_JOYSTICK_DEAD_ZONE 0
#if ENABLE_JOYSTICK_DEAD_ZONE
  // Here we are using SetProperty to set the deadzone to 7% of the joysticks range
  // range
  dipDWORD.diph.dwSize       = sizeof(dipDWORD);
  dipDWORD.diph.dwHeaderSize = sizeof(dipDWORD.diph);
  dipDWORD.diph.dwHow        = DIPH_BYOFFSET;
  dipDWORD.dwData            = 700;

  dipDWORD.diph.dwObj        = DIJOFS_X;
  _devJoystick->SetProperty( DIPROP_DEADZONE, &dipDWORD.diph);

  dipDWORD.diph.dwObj        = DIJOFS_Y;
  _devJoystick->SetProperty( DIPROP_DEADZONE, &dipDWORD.diph);

  dipDWORD.diph.dwObj        = DIJOFS_Z;
  _devJoystick->SetProperty( DIPROP_DEADZONE, &dipDWORD.diph);

  dipDWORD.diph.dwObj        = DIJOFS_RX;
  _devJoystick->SetProperty( DIPROP_DEADZONE, &dipDWORD.diph);

  dipDWORD.diph.dwObj        = DIJOFS_RY;
  _devJoystick->SetProperty( DIPROP_DEADZONE, &dipDWORD.diph);

  dipDWORD.diph.dwObj        = DIJOFS_RZ;
  _devJoystick->SetProperty( DIPROP_DEADZONE, &dipDWORD.diph);
#endif

  _devJoystick->EnumObjects(JoyAxisCallback, _axesInfos, DIDFT_AXIS);

#define REPORT_JOYSTICK_INFOS 1
#if REPORT_JOYSTICK_INFOS
  RptF("Detected Joystick: %s", lpddi->tszProductName);
  for (int i=0; i<N_JOYSTICK_AXES; ++i)
  {
    if (_axesInfos[i].active) RptF("    %d ... %s [%x,%x]",i,cc_cast(_axesInfos[i].name), _axesInfos[i].usagePage, _axesInfos[i].usage);
  }
#endif

  // If we were enumerating a force object then we made it... set the global
  // variable as such.
  _capabilities.dwSize = sizeof(_capabilities);
  if (_devJoystick->GetCapabilities(&_capabilities) != DI_OK)
  {
    _capabilities.dwAxes = _capabilities.dwButtons = _capabilities.dwPOVs = _capabilities.dwFlags = 0;
  }
  bool bEnumForce = (_capabilities.dwFlags & DIEDFL_FORCEFEEDBACK)!=0;
  LogF("Joystick added: %s %s", lpddi->tszProductName, bEnumForce ? "(Force feedback)" : "(no force feedback)");
  _ffEnabled = (_capabilities.dwFlags & DIEDFL_FORCEFEEDBACK)!=0;

#if 0
  // the axes,buttons,POVs names enumeration has no sense, as it gives only names like X Axis for all devices
  //   (i.e. there is no Wheel left or something)
  // we want to know the names of axes
  DIDEVICEOBJECTINSTANCE info; 
  info.dwSize=sizeof(DIDEVICEOBJECTINSTANCE);
  const DWORD axesOffset[N_JOYSTICK_AXES] = {DIJOFS_X,DIJOFS_Y,DIJOFS_Z,DIJOFS_RX,DIJOFS_RY,DIJOFS_RZ,DIJOFS_SLIDER(0),DIJOFS_SLIDER(1)};
  LogF(" Axes Count: %d", capabilities.dwAxes);
  for (int i=0; i<N_JOYSTICK_AXES; i++)
  {
    if (_devJoystick->GetObjectInfo(&info, axesOffset[i], DIPH_BYOFFSET)==DI_OK)
      LogF("   Axes name: %s", info.tszName);
  }
  LogF(" Buttons Count: %d", capabilities.dwButtons);
  for (int i=0; i<capabilities.dwButtons; i++)
  {
    if (_devJoystick->GetObjectInfo(&info, DIJOFS_BUTTON(i), DIPH_BYOFFSET)==DI_OK)
      LogF("   Button name: %s", info.tszName);
  }
  LogF(" POVs Count: %d", capabilities.dwPOVs);
  for (int i=0; i<capabilities.dwPOVs; i++)
  {
    if (_devJoystick->GetObjectInfo(&info, DIJOFS_POV(i), DIPH_BYOFFSET)==DI_OK)
      LogF("   Button name: %s", info.tszName);
  }
#endif

  // We need to determine the Joystick's offset
  //  1. stored one for known Joystick
  //  2. new one when joystick is connected first time
  _offset = GJoystickDevices->GetJoystickOffset(lpddi->guidInstance, lpddi->tszProductName, isXInput);

  // Force Feedback
  InitForceFeedback();

  DoSetStiffness(0.2,0.2);
  DoSetEngine(0,0);
  DoSetSurface(0);
}

void Joystick::SetDeadZone(const AutoArray<float> &deadZone)
{
  DIPROPDWORD          dipDWORD;
  // Here we are using SetProperty to set the deadzone to 7% of the joysticks range
  // range
  dipDWORD.diph.dwSize       = sizeof(dipDWORD);
  dipDWORD.diph.dwHeaderSize = sizeof(dipDWORD.diph);
  dipDWORD.diph.dwHow        = DIPH_BYOFFSET;

  static DWORD JoyAxisOffsets[] = {DIJOFS_X, DIJOFS_Y, DIJOFS_Z, DIJOFS_RX, DIJOFS_RY, DIJOFS_RZ, DIJOFS_SLIDER(0), DIJOFS_SLIDER(1)};
  int count = intMin(sizeof(JoyAxisOffsets)/sizeof(DWORD), deadZone.Size());
  for (int i=0; i<count; i++)
  {
    dipDWORD.dwData            = deadZone[i]*10000;
    dipDWORD.diph.dwObj        = JoyAxisOffsets[i];
    _devJoystick->SetProperty( DIPROP_DEADZONE, &dipDWORD.diph);
  }
}

/******************************************************************************
FUNCTION: AddJoy
This function is called by the EnumDevices function in the IDirectInput2
interface.  It is used to enumerate existing joysticks on the system.  This
function looks for a force feedback capable joystick, and then will settle for
any installed joystick.

PARAMETERS:
lpddi -- Device instance information
pvRef -- LPVOID passed into the call to EnumDevices... used here to indicate
whether we are trying to find force feedback joysticks.

RETURNS:
DIENUM_STOP -- If we find a joystick
DIENUM_CONTINUE -- If we haven't yet
******************************************************************************/

BOOL CALLBACK EnumJoyCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
  EnumContext *context=static_cast<EnumContext *>(pvRef);
  return context->system->AddJoy(lpddi,context->hwnd);
}

/*!
\patch_internal 1.01 Date 6/25/2001 by Ondra.
- Fixed: joystick support.
\patch 1.11 Date 7/31/2001 by Ondra.
- New: joystick support.
\patch 1.16 Date 8/9/2001 by Ondra.
- New: Steering wheel Force Feedback.
\patch 1.16 Date 8/10/2001 by Ondra.
- New: iFeel mouse force feedback support.
*/

//-----------------------------------------------------------------------------
// Enum each PNP device using WMI and check each device ID to see if it contains 
// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it's an XInput device
// Unfortunately this information can not be found by just using DirectInput 
//-----------------------------------------------------------------------------
#include <wbemidl.h>
#include <oleauto.h>
//#include <wmsstd.h> // not available, but we need only SAFE_RELEASE
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) if( NULL != p ) { ( p )->Release(); p = NULL; }
#endif
static bool IsXInputDevice( const GUID* pGuidProductFromDirectInput )
{
  IWbemLocator*           pIWbemLocator  = NULL;
  IEnumWbemClassObject*   pEnumDevices   = NULL;
  IWbemClassObject*       pDevices[20]   = {0};
  IWbemServices*          pIWbemServices = NULL;
  BSTR                    bstrNamespace  = NULL;
  BSTR                    bstrDeviceID   = NULL;
  BSTR                    bstrClassName  = NULL;
  DWORD                   uReturned      = 0;
  bool                    bIsXinputDevice= false;
  UINT                    iDevice        = 0;
  VARIANT                 var;
  HRESULT                 hr;

  // Create WMI
  hr = CoCreateInstance( __uuidof(WbemLocator),
    NULL,
    CLSCTX_INPROC_SERVER,
    __uuidof(IWbemLocator),
    (LPVOID*) &pIWbemLocator);
  if( FAILED(hr) || pIWbemLocator == NULL )
    goto LCleanup;

  bstrNamespace = SysAllocString( L"\\\\.\\root\\cimv2" );if( bstrNamespace == NULL ) goto LCleanup;        
  bstrClassName = SysAllocString( L"Win32_PNPEntity" );   if( bstrClassName == NULL ) goto LCleanup;        
  bstrDeviceID  = SysAllocString( L"DeviceID" );          if( bstrDeviceID == NULL )  goto LCleanup;        

  // Connect to WMI 
  hr = pIWbemLocator->ConnectServer( bstrNamespace, NULL, NULL, 0L, 
    0L, NULL, NULL, &pIWbemServices );
  if( FAILED(hr) || pIWbemServices == NULL )
    goto LCleanup;

  // Switch security level to IMPERSONATE. 
  CoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, 
    RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );                    

  hr = pIWbemServices->CreateInstanceEnum( bstrClassName, 0, NULL, &pEnumDevices ); 
  if( FAILED(hr) || pEnumDevices == NULL )
    goto LCleanup;

  // Loop over all devices
  for( ;; )
  {
    // Get 20 at a time
    hr = pEnumDevices->Next( 10000, 20, pDevices, &uReturned );
    if( FAILED(hr) )
      goto LCleanup;
    if( uReturned == 0 )
      break;

    for( iDevice=0; iDevice<uReturned; iDevice++ )
    {
      // For each device, get its device ID
      hr = pDevices[iDevice]->Get( bstrDeviceID, 0L, &var, NULL, NULL );
      if( SUCCEEDED( hr ) && var.vt == VT_BSTR && var.bstrVal != NULL )
      {
        // Check if the device ID contains "IG_".  If it does, then it's an XInput device
        // This information can not be found from DirectInput 
        if( wcsstr( var.bstrVal, L"IG_" ) )
        {
          // If it does, then get the VID/PID from var.bstrVal
          DWORD dwPid = 0, dwVid = 0;
          WCHAR* strVid = wcsstr( var.bstrVal, L"VID_" );
          if( strVid && swscanf( strVid, L"VID_%4X", &dwVid ) != 1 )
            dwVid = 0;
          WCHAR* strPid = wcsstr( var.bstrVal, L"PID_" );
          if( strPid && swscanf( strPid, L"PID_%4X", &dwPid ) != 1 )
            dwPid = 0;

          // Compare the VID/PID to the DInput device
          DWORD dwVidPid = MAKELONG( dwVid, dwPid );
          if( dwVidPid == pGuidProductFromDirectInput->Data1 )
          {
            bIsXinputDevice = true;
            goto LCleanup;
          }
        }
      }   
      SAFE_RELEASE( pDevices[iDevice] );
    }
  }

LCleanup:
  if(bstrNamespace)
    SysFreeString(bstrNamespace);
  if(bstrDeviceID)
    SysFreeString(bstrDeviceID);
  if(bstrClassName)
    SysFreeString(bstrClassName);
  for( iDevice=0; iDevice<20; iDevice++ )
    SAFE_RELEASE( pDevices[iDevice] );
  SAFE_RELEASE( pEnumDevices );
  SAFE_RELEASE( pIWbemLocator );
  SAFE_RELEASE( pIWbemServices );

  return bIsXinputDevice;
}

BOOL JoystickDevices::AddJoy(LPCDIDEVICEINSTANCE lpddi, HWND hwnd )
{
  IDirectInputDevice8 *devJoystick;

  bool isXInput = IsXInputDevice( &lpddi->guidProduct );
  // Are we enumerating force feedback joysticks?
  // Can we create the device that was passed in the lpddi pointer
  const GUID &guid = lpddi->guidInstance;
  LogF(" Joystick {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
    guid.Data1, guid.Data2, guid.Data3,
    guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
    guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

#if _SPACEMOUSE
  // that's a rather dirty way, since we wait for information from 3DConnexion
  if(!strcmp((const char*)lpddi->tszProductName, "SpaceTraveler USB"))
  {
    return DIENUM_CONTINUE;
  }
#endif
  // Are we enumerating force feedback joysticks?
  // Can we create the device that was passed in the lpddi pointer
  if
    (
    FAILED
    (
    _dInput->CreateDevice(lpddi->guidInstance, &devJoystick, NULL)
    )
    )
  {
    // keep trying
    return DIENUM_CONTINUE;
  }
  Joystick *joystick = CreateJoystick();
  if (joystick)
  {
    joystick->Init(lpddi, devJoystick, hwnd, isXInput);
    Add(joystick); //add into joystick devices
    int ix = GInput._joysticksState.Add();    //create space to store its input
    Input::JoystickState &state = GInput._joysticksState[ix];
    state.offset = joystick->GetOffset();
    state.mode = joystick->GetMode();
    state.isXInput = IsXInput(state.offset);
    state.sensitivity = GetSensitivity(state.offset);
    state.deadZone = GetDeadZone(state.offset);
    state.steepness = GetSteepness(state.offset);
    int initSize = state.deadZone.Size();
    state.deadZone.Realloc(N_JOYSTICK_AXES); state.deadZone.Resize(N_JOYSTICK_AXES);
    for (int i=initSize; i<N_JOYSTICK_AXES; i++)
    {
      state.deadZone[i] = DeadZoneDefaults[i];
    }
    joystick->SetDeadZone(state.deadZone);
  }
  return DIENUM_CONTINUE;
}

#include <Es/Strings/rString.hpp>

static RString GetParamNames(DWORD pars)
{
  RString ret = "";
  if (pars&DIEP_AXES) ret = ret + (RString)" Axes";
  if (pars&DIEP_DIRECTION) ret = ret + (RString)" Direction";
  if (pars&DIEP_DURATION) ret= ret + (RString)" Duration";
  if (pars&DIEP_ENVELOPE) ret= ret + (RString)" Envelope";
  if (pars&DIEP_GAIN) ret= ret + (RString)" Gain";
  if (pars&DIEP_SAMPLEPERIOD) ret= ret + (RString)" Sample period";
  if (pars&DIEP_STARTDELAY) ret= ret + (RString)" Startdelay"; 
  if (pars&DIEP_TRIGGERBUTTON) ret= ret + (RString)" Trigger button";
  if (pars&DIEP_TRIGGERREPEATINTERVAL) ret= ret + (RString)" Trigger repeat interval";
  if (pars&DIEP_TYPESPECIFICPARAMS) ret= ret + (RString)" Type specific params";
  return ret;
}

static BOOL CALLBACK DIEnumEffectsCallback(LPCDIEFFECTINFO ei, LPVOID pvRef)
{
  //HRESULT              hr;
  //LPDIRECTINPUTDEVICE8 lpdid = (LPDIRECTINPUTDEVICE8)pvRef;    
  Assert (ei->dwSize>=sizeof(DIEFFECTINFO));

  if (DIEFT_GETTYPE(ei->dwEffType) == DIEFT_CONSTANTFORCE)
  {
    LogF("Constant force %s",ei->tszName);
    LogF("  Static  %s",(const char *)GetParamNames(ei->dwStaticParams));
    LogF("  Dynamic %s",(const char *)GetParamNames(ei->dwDynamicParams));
  }
  if (DIEFT_GETTYPE(ei->dwEffType) == DIEFT_RAMPFORCE)
  {
    LogF("Ramp force %s",ei->tszName);
    LogF("  Static  %s",(const char *)GetParamNames(ei->dwStaticParams));
    LogF("  Dynamic %s",(const char *)GetParamNames(ei->dwDynamicParams));
  }
  if (DIEFT_GETTYPE(ei->dwEffType) == DIEFT_PERIODIC)
  {
    LogF("Periodic %s",ei->tszName);
    LogF("  Static  %s",(const char *)GetParamNames(ei->dwStaticParams));
    LogF("  Dynamic %s",(const char *)GetParamNames(ei->dwDynamicParams));
  }
  if (DIEFT_GETTYPE(ei->dwEffType) == DIEFT_CONDITION)
  {
    LogF("Condition %s",ei->tszName);
    LogF("  Static  %s",(const char *)GetParamNames(ei->dwStaticParams));
    LogF("  Dynamic %s",(const char *)GetParamNames(ei->dwDynamicParams));
  }


  return DIENUM_CONTINUE;
}  // End of callback

/******************************************************************************
FUNCTION: InitForceFeedback
This function is called by InitDirectInput if a force feedback joystick has
been detected.  This function turns off auto center, and then calls the
individual functions to create effects.

PARAMETERS:
None

RETURNS:
Success or Failure
******************************************************************************/
void Joystick::InitForceFeedback()
{
  if( !_devJoystick )
  {
    _ffEnabled=false;
    return;
  }

  _effWeaponStarted = false;
  _effEngineStarted = false;

  if( !_ffEnabled ) return;

  // enumerate effects
  //HRESULT hr =
#if _DEBUG
  _devJoystick->EnumEffects(&DIEnumEffectsCallback, _devJoystick, DIEFT_ALL);
#endif


  // SetProperty can be used to set various options for a device.  One of
  // them being DIPROP_AUTOCENTER, which we are disabling.
#if 1
  DIPROPDWORD diPropAutoCenter;
  diPropAutoCenter.diph.dwSize = sizeof(diPropAutoCenter);
  diPropAutoCenter.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  diPropAutoCenter.diph.dwObj = 0;
  diPropAutoCenter.diph.dwHow = DIPH_DEVICE;
  diPropAutoCenter.dwData = DIPROPAUTOCENTER_OFF;
  _devJoystick->SetProperty(DIPROP_AUTOCENTER,&diPropAutoCenter.diph);
#endif

  if( FAILED(_devJoystick->Acquire()) ) _ffEnabled=false;
  InitEffects();

  FFOn(); // TODO: FFOn/Off from game UI
}

bool Joystick::CheckStickActive(int joyIx)
{
  // find corresponding JoystickState
  if (joyIx<0) return false;
  const Input::JoystickState &joy = GInput._joysticksState[joyIx];
  if (!joy.IsEnabled()) return false;

  for (int i=0; i<JoystickNAxes; i++)
  {
    if (joy.jAxisLastActive[i]>Glob.uiTime-15)
    {
#if 0
      UITime maxTime = UITIME_MIN;
      for (int i=0; i<JoystickNAxes; i++)
      {
        if (maxTime<joy.jAxisLastActive[i]) maxTime = joy.jAxisLastActive[i];
      }
      GlobalShowMessage(100,"%d: Active %.3f",i,Glob.uiTime-maxTime);
      LogF
        (
        "%.3f - %.3f, %d: Active %.3f",
        Glob.uiTime.toFloat(),maxTime.toFloat(),
        i,Glob.uiTime-maxTime
        );
#endif
      return true;
    }
  }
  return false;
}

bool Joystick::CheckStickActive()
{
  int joyIx = -1;
  int offset = GetOffset();
  for (int i=0; i<GInput._joysticksState.Size(); i++)
  {
    if (GInput._joysticksState[i].offset == offset) { joyIx = i; break; }
  }
  return CheckStickActive(joyIx);
}

/******************************************************************************
FUNCTION: InitEffects
This function is called by InitForceFeedback to create all effects

******************************************************************************/
/*!
*/
void Joystick::InitEffects()
{
  // What axes are we mapping for this effect to
  static DWORD dwAxes[1] = {DIJOFS_Y };
  static LONG lDirection[1] = {1};

  // parameters common to all effects
  DIEFFECT diEffect;
  memset(&diEffect,0,sizeof(diEffect));

  diEffect.dwSize = sizeof(DIEFFECT);
  diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
  diEffect.dwSamplePeriod = 0;
  diEffect.dwGain = DI_FFNOMINALMAX;

  // both axes
  diEffect.cAxes = 1;
  diEffect.rgdwAxes = dwAxes;
  diEffect.rglDirection = lDirection;
  diEffect.lpEnvelope = NULL;

  diEffect.dwTriggerButton = DIEB_NOTRIGGER;
  diEffect.dwTriggerRepeatInterval = 0;

  // default infinite duration
  diEffect.dwDuration = INFINITE;

  // No envelope
  diEffect.lpEnvelope = NULL;

  DIPERIODIC  diPeriodic;

  // Set up the "Wavy" effect.
  // This effect will be periodic, so we will use the DIPERIODIC

  // Set the magnitude to half of max.
  diPeriodic.dwMagnitude = 0;

  // No offset or phase defined... we want it cycling around center
  // and we don't much care where it starts in the wave
  diPeriodic.lOffset = 0;
  diPeriodic.dwPhase = 0;
  diPeriodic.dwPeriod = DI_SECONDS;


  diEffect.cbTypeSpecificParams = sizeof(diPeriodic);
  diEffect.lpvTypeSpecificParams = &diPeriodic;

  // Create the effect and get an interface back in our pointer
#if 0
  HRESULT res = _devJoystick->CreateEffect(GUID_Sine, &diEffect,_effEngine.Init(), NULL);
  switch(res)
  {
    case DIERR_DEVICEFULL: LogF("DIERR_DEVICEFULL"); break;
    case DIERR_DEVICENOTREG: LogF("DIERR_DEVICENOTREG"); break;
    case DIERR_INVALIDPARAM: LogF("DIERR_INVALIDPARAM"); break;
    case DIERR_NOTINITIALIZED: LogF("DIERR_NOTINITIALIZED"); break;
    case S_OK:  LogF("S_OK"); break;
    default: LogF("value %x", res);
  }
#else
  _devJoystick->CreateEffect(GUID_Sine, &diEffect,_effEngine.Init(), NULL);
#endif
  if (!_effEngine)
  {
    // if effect creation failed, chance is device has no Y axis
    dwAxes[0] = DIJOFS_X;
#if 0
    HRESULT res = _devJoystick->CreateEffect(GUID_Sine, &diEffect,_effEngine.Init(), NULL);
    switch(res)
    {
      case DIERR_DEVICEFULL: LogF("DIERR_DEVICEFULL"); break;
      case DIERR_DEVICENOTREG: LogF("DIERR_DEVICENOTREG"); break;
      case DIERR_INVALIDPARAM: LogF("DIERR_INVALIDPARAM"); break;
      case DIERR_NOTINITIALIZED: LogF("DIERR_NOTINITIALIZED"); break;
      case S_OK:  LogF("S_OK"); break;
      default: LogF("value %x", res);
    }
#else
    _devJoystick->CreateEffect(GUID_Sine, &diEffect,_effEngine.Init(), NULL);
#endif
  }

  DICONSTANTFORCE  cf;
  cf.lMagnitude = 0;

  DIENVELOPE envelope;
  envelope.dwSize = sizeof(envelope);
  envelope.dwAttackTime = 0;
  envelope.dwFadeTime = INFINITE;
  envelope.dwFadeLevel = 0;
  envelope.dwAttackLevel = 0;

  diEffect.lpEnvelope = &envelope;
  diEffect.cbTypeSpecificParams = sizeof(cf);
  diEffect.lpvTypeSpecificParams = &cf;
  diEffect.dwDuration = INFINITE;

#if 1
  _devJoystick->CreateEffect
    (
    GUID_ConstantForce, &diEffect,_effWeapon.Init(), NULL
    );
#endif
  if (_effWeapon)
  {
    _weaponByConstantForce = true;
  }
  else
  {
    _weaponByConstantForce = false;

    DIPERIODIC per;
    per.dwMagnitude = 0;
    per.dwPhase = 0;
    per.dwPeriod = 20;
    per.lOffset = 0;

    diEffect.cbTypeSpecificParams = sizeof(per);
    diEffect.lpvTypeSpecificParams = &per;
    _devJoystick->CreateEffect
      (
      GUID_Square, &diEffect,_effWeapon.Init(), NULL
      );
  }

  DICONDITION cond[2];
  cond[0].dwNegativeSaturation = DI_FFNOMINALMAX;
  cond[0].dwPositiveSaturation = DI_FFNOMINALMAX;
  cond[0].lDeadBand = 0;
  cond[0].lOffset = 0;
  cond[0].lNegativeCoefficient = 0;
  cond[0].lPositiveCoefficient = 0;

  cond[1].dwNegativeSaturation = DI_FFNOMINALMAX;
  cond[1].dwPositiveSaturation = DI_FFNOMINALMAX;
  cond[1].lDeadBand = 0;
  cond[1].lOffset = 0;
  cond[1].lNegativeCoefficient = 0;
  cond[1].lPositiveCoefficient = 0;

  static DWORD condAxes[2] = { DIJOFS_X, DIJOFS_Y };
  static LONG condDirection[2] = {0,1};

  diEffect.cAxes = 2;
  diEffect.rgdwAxes = condAxes;
  diEffect.rglDirection = condDirection;
  diEffect.dwDuration = INFINITE;
  diEffect.dwGain = DI_FFNOMINALMAX;

  diEffect.cbTypeSpecificParams = sizeof(cond);
  diEffect.lpvTypeSpecificParams = cond;

  _devJoystick->CreateEffect(GUID_Spring, &diEffect,_effCenter.Init(), NULL);
  if (!_effCenter)
  {
    // failed: try one axis only
    diEffect.cAxes = 1;
    diEffect.cbTypeSpecificParams = sizeof(cond[0]);
    _devJoystick->CreateEffect(GUID_Spring, &diEffect,_effCenter.Init(), NULL);
  }
  _nCenterAxes = diEffect.cAxes;
}

JoystickMode Joystick::GetMode()
{
  if (GJoystickDevices)
  {
    const JoystickDevices::RecognizedJoystick *joy = GJoystickDevices->GetJoystickInfo(_offset);
    if (joy) return joy->mode;
  }
  return JMCustom;
}

/*!
\param axis -1000..+1000 analog value
\param button 8b analog value, 0xff = fully pressed, 0x80 = half pressed
\param buttonEdge boolean value if button is pressed since last poll
\param buttonUpEdge boolean value if button is released since last poll
*/

bool Joystick::Get
(
 int axis[NAxes], unsigned short button[NButtons], bool buttonEdge[NButtons], bool buttonUpEdge[NButtons],
 bool buttonState[NButtons], int &hat
 )
{
  PROFILE_SCOPE(joyGt);

  HRESULT     hr;
  DIJOYSTATE  diJoyState;

  // No device object?  Give up.
  if(!_devJoystick) return false;

  int joyIx = -1;
  int offset = GetOffset();
  for (int i=0; i<GInput._joysticksState.Size(); i++)
  {
    if (GInput._joysticksState[i].offset == offset) { joyIx = i; break; }
  }
  CheckStickActive(joyIx);

  // Starting from scratch
  memset(axis,0,NAxes*sizeof(*axis));
  memset(button,0,NButtons*sizeof(*button));
  memset(buttonEdge,0,NButtons*sizeof(*buttonEdge));
  memset(buttonUpEdge,0,NButtons*sizeof(*buttonUpEdge));
  memset(buttonState,0,NButtons*sizeof(*buttonState));

  if (joyIx<0 || !GInput._joysticksState[joyIx].IsEnabled())
    return false;
  
  // Poll, or retrieve information from the device
  hr = _devJoystick->Poll();
  if(hr== DIERR_INPUTLOST || hr== DIERR_NOTACQUIRED )
  {
    // If the poll failed, maybe we should acquire
    _devJoystick->Acquire();
    // When we lose control of the device we may need to re-download
    // the effects that are mapped to buttons, the other effects will
    if( _ffOn ) FFStart();
    // autodownload
    //if( _effect[Wave] ) _effect[Wave]->Download();
    //if( _effect[Ramp] ) _effect[Ramp]->Download();

    hr = _devJoystick->Poll();
  }
  if( FAILED(hr) ) return false;

  // Get the device state, and populate the DIJOYSTATE structure
  hr=_devJoystick->GetDeviceState(sizeof(DIJOYSTATE),&diJoyState);
  if( FAILED(hr) ) return false;

  // Map information here to our return variables.  It would be a trivial
  // addition to retrieve information such as rudder, hat, and throttle from
  // this structure.
  axis[0] = diJoyState.lX;
  axis[1] = diJoyState.lY;
  axis[2] = diJoyState.lZ;
  axis[3] = diJoyState.lRx;
  axis[4] = diJoyState.lRy;
  axis[5] = diJoyState.lRz;
  axis[6] = diJoyState.rglSlider[0];
  axis[7] = diJoyState.rglSlider[1];

  /*
  GlobalShowMessage
  (
  100,"%4d %4d %4d, %4d %4d %4d",
  diJoyState.lX,diJoyState.lY,diJoyState.lZ,
  diJoyState.lRx,diJoyState.lRy,diJoyState.lRz
  );
  */

  //{ mapping of Xbox controller
  for (int i=0; i<16; i++)
  {
    int state = (int)diJoyState.rgbButtons[i] * 256;
    saturateMin(state, 0x7fff);
    button[i] = state;
  }
  // map axis to buttons for usage of Xbox Controller on PC
#if ENABLE_ANALOG_JOYSTICK_BUTTONS
  button[16] = diJoyState.lX > 0 ? diJoyState.lX * 256 : 0;
  button[17] = diJoyState.lX < 0 ? -diJoyState.lX * 256 : 0;
  button[18] = diJoyState.lY > 0 ? diJoyState.lY * 256 : 0;
  button[19] = diJoyState.lY < 0 ? -diJoyState.lY * 256 : 0;
  button[20] = diJoyState.lZ > 0 ? diJoyState.lZ * 256 : 0;
  button[21] = diJoyState.lZ < 0 ? -diJoyState.lZ * 256 : 0;
  button[22] = diJoyState.lRx > 0 ? diJoyState.lRx * 256 : 0;
  button[23] = diJoyState.lRx < 0 ? -diJoyState.lRx * 256 : 0;
  button[24] = diJoyState.lRy > 0 ? diJoyState.lRy * 256 : 0;
  button[25] = diJoyState.lRy < 0 ? -diJoyState.lRy * 256 : 0;
  button[26] = diJoyState.lRz > 0 ? diJoyState.lRz * 256 : 0;
  button[27] = diJoyState.lRz < 0 ? -diJoyState.lRz * 256 : 0;
#endif

  for( int i=0; i<NButtons; i++ )
  {
    // if button is pressed, request lower threshold to consider it released again
    int thold = _defTholds[i][_buttonState[i]];
    bool nbi = button[i]>thold;

    buttonEdge[i]=( nbi && !_buttonState[i] );
    buttonUpEdge[i]=( !nbi && _buttonState[i] );
    _buttonState[i] = nbi; 
    buttonState[i] = nbi;
  }
  //}

  hat = diJoyState.rgdwPOV[0];
  if( (hat&0xfffff)==0xffff ) hat=-1;
  if (hat!=-1)
  {
    hat /= DI_DEGREES;
  }
  return true;
}

void Joystick::FFStart()
{
  //if( _effForce ) _effForce->Start(1,0);
  if( _effEngine && _effEngineStarted) _effEngine->Start(1,0);
  if( _effWeapon && _effWeaponStarted) _effWeapon->Start(1,0);
  if( _effCenter ) _effCenter->Start(1,0);
}

void Joystick::FFStop()
{
  if (_effEngine) _effEngine->Stop();
  if (_effWeapon) _effWeapon->Stop();
  //if( _effForce ) _effForce->Stop();
}

void Joystick::FFOn()
{
  if (_ffOn) return;
  _ffOn = true;
  FFStart();
}

void Joystick::Enable(bool enable)
{
  _enabled=enable;
  JoystickMode mode = enable ? JMCustom : JMDisabled;
  int offset = GetOffset();
  if (GJoystickDevices) 
  {
    for (int i=0; i<GInput._joysticksState.Size(); i++)
    {
      if (GInput._joysticksState[i].offset == offset) 
      { 
        GInput._joysticksState[i].mode = mode;
        break; 
      }
    }
    JoystickDevices::RecognizedJoystick *joy = GJoystickDevices->GetJoystickInfo(offset);
    if (joy)
    {
      joy->mode = mode;
    }
  }
}

void Joystick::FFOff()
{
  if (_ffOn) FFStop();
  _ffOn = false;
}

#ifdef _WIN32
void DIError( const char *op, HRESULT err )
{
  const char *name="???";
  char buf[256];
  switch( err )
  {
  case DI_OK: return;
  case S_FALSE: return;
#define CODE(s) case s: name=#s; break;
    CODE(DI_EFFECTRESTARTED);
    CODE(DI_DOWNLOADSKIPPED);
    CODE(DI_TRUNCATED);
    CODE(DI_TRUNCATEDANDRESTARTED);
    CODE(DIERR_NOTINITIALIZED);
    CODE(DIERR_INCOMPLETEEFFECT);
    CODE(DIERR_INPUTLOST);
    CODE(DIERR_INVALIDPARAM);
    CODE(DIERR_EFFECTPLAYING);
  default:
    {
      FormatMessage
        (
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, // source
        err, // requested message identifier 
        0, // languague
        buf,sizeof(buf),
        NULL
        );
      char *eol=strrchr(buf,'\n');
      if( eol==buf+strlen(buf)-1 ) *eol=0;
      name=buf;
    }
    break;
  }
  if( name ) LogF("DInput %s: %x - %s",op,err,name);
}
#endif

void Joystick::PlayRamp(float begMag, float endMag, float duration)
{
  // play ramp
  if (!_effWeapon) return;

  // check if some axis has been active recently
  bool active = CheckStickActive();

  // Constant force, full strength
  float mag2=Square(begMag)+Square(endMag);
  if( mag2<1e-6 || !active)
  {
#if 0
    DWORD status;
    _effWeapon->GetEffectStatus(&status);
    if(status&DIEGES_PLAYING)
    {
      LogF("%.2f: Cancel constant force",Glob.time.toFloat());
    }
#endif
    _effWeapon->Stop();
    _effWeaponStarted = false;
    return;
  }

  int iBegMag=toIntFloor(begMag*DI_FFNOMINALMAX);
  int iEndMag=toIntFloor(endMag*DI_FFNOMINALMAX);
  int iDuration = toLargeInt(DI_SECONDS*duration);

  saturate(iBegMag,1,DI_FFNOMINALMAX);
  saturate(iEndMag,1,DI_FFNOMINALMAX);

  //DICONSTANTFORCE  diPars = { iBegMag, iEndMag };
  static LONG  lDirection[1] = {1};
  static DWORD dwAxes[1] = { DIJOFS_Y };

  DICONSTANTFORCE  cf;
  DIPERIODIC per;
  DIEFFECT diEffect;
  memset(&diEffect,0,sizeof(diEffect));
  diEffect.dwSize = sizeof(diEffect);

  if (_weaponByConstantForce)
  {

    // The beginning and the end values for our ramp force
    cf.lMagnitude = endMag;
    diEffect.cbTypeSpecificParams = sizeof(cf);
    diEffect.lpvTypeSpecificParams = &cf;
  }
  else
  {
    per.dwMagnitude = endMag;
    per.lOffset = 0;
    per.dwPhase = 0;
    per.dwPeriod = 20;
    diEffect.cbTypeSpecificParams = sizeof(per);
    diEffect.lpvTypeSpecificParams = &per;
  }


  DIENVELOPE envelope;
  envelope.dwSize = sizeof(envelope);
  envelope.dwAttackTime = iDuration;
  envelope.dwFadeTime = INFINITE;
  envelope.dwFadeLevel = iEndMag;
  envelope.dwAttackLevel = iBegMag;



  diEffect.cAxes = 1;
  diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
  diEffect.dwTriggerButton = -1;
  diEffect.dwGain = DI_FFNOMINALMAX;
  diEffect.lpEnvelope = &envelope;

  diEffect.rgdwAxes = dwAxes;
  diEffect.rglDirection = lDirection;
  diEffect.dwDuration = INFINITE; // after attack stay on ending level

#if 0
  LogF
    (
    "%.2f: Set constant envelope: %d..%d, time %d",
    Glob.time.toFloat(),iBegMag,iEndMag,iDuration
    );
#endif
  // Get an interface to our new effect
  HRESULT err=_effWeapon->SetParameters
    (
    &diEffect,
    //DIEP_DIRECTION|
    DIEP_DURATION|DIEP_ENVELOPE|
    DIEP_TYPESPECIFICPARAMS|DIEP_START
    );
  if (err<DI_OK)
  {
    DIError("Force SetParameters",err);
  }
  _effWeaponStarted = true;
}

// weapon effects

void Joystick::SetStiffness(float valueX, float valueY)
{
  float thold = 0.05f;
  if (fabs(valueX-_stiffnessX)>thold || fabs(valueY-_stiffnessY)>=thold)
  {
    DoSetStiffness(valueX,valueY);
  }
}

void Joystick::DoSetStiffness(float valueX, float valueY)
{
  _stiffnessX = valueX;
  _stiffnessY = valueY;
  if (!_effCenter) return;

  DIEFFECT diEffect;
  memset(&diEffect,0,sizeof(diEffect));
  diEffect.dwSize = sizeof(DIEFFECT);
  diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;

  // how much autocentering is applied
  int iValX = toInt(DI_FFNOMINALMAX*valueX);
  int iValY = toInt(DI_FFNOMINALMAX*valueY);

  saturate(iValX,0,DI_FFNOMINALMAX);
  saturate(iValY,0,DI_FFNOMINALMAX);

  //GlobalShowMessage(2000,"X %d Y %d",iValX,iValY);

  DICONDITION cond[2];
  cond[0].dwNegativeSaturation = DI_FFNOMINALMAX;
  cond[0].dwPositiveSaturation = DI_FFNOMINALMAX;
  cond[0].lDeadBand = 0;
  cond[0].lOffset = 0;
  cond[0].lNegativeCoefficient = iValX;
  cond[0].lPositiveCoefficient = iValX;

  cond[1].dwNegativeSaturation = DI_FFNOMINALMAX;
  cond[1].dwPositiveSaturation = DI_FFNOMINALMAX;
  cond[1].lDeadBand = 0;
  cond[1].lOffset = 0;
  cond[1].lNegativeCoefficient = iValY;
  cond[1].lPositiveCoefficient = iValY;

  static DWORD condAxes[2] = { DIJOFS_X, DIJOFS_Y };
  static LONG condDirection[2] = {0,1};

  diEffect.cAxes = _nCenterAxes;
  diEffect.rgdwAxes = condAxes;
  diEffect.rglDirection = condDirection;

  diEffect.cbTypeSpecificParams = sizeof(*cond)*_nCenterAxes;
  diEffect.lpvTypeSpecificParams = cond;

  HRESULT err = _effCenter->SetParameters(&diEffect,DIEP_TYPESPECIFICPARAMS|DIEP_START);
  if (err<DI_OK)
  {
    DIError("_effCenter SetParameters",err);
  }
}

void Joystick::SetEngine( float mag, float freq )
{
  if (fabs(mag-_engineMag)>0.02 || fabs(freq-_engineFreq)>5)
  {
    DoSetEngine(mag,freq);
  }
}

void Joystick::SetSurface( float mag )
{
  if (fabs(mag-_surfaceMag) > 0.02)
  {
    DoSetSurface(mag);
  }
}

void Joystick::DoSetEngine( float mag, float freq )
{
  _engineMag = mag;
  _engineFreq = freq;

  if( !_effEngine ) return;

  // Constant force, full strength

  bool active = CheckStickActive();
  if( mag<1e-3 || !active)
  {
    _effEngine->Stop();
    _effEngineStarted = false;
    return;
  }

  int iMag=toIntFloor(mag*DI_FFNOMINALMAX);
  saturateMin(iMag,DI_FFNOMINALMAX);

  DIPERIODIC  diSpec;
  diSpec.dwMagnitude = iMag;
  diSpec.lOffset = 0;
  diSpec.dwPhase = 0;
  diSpec.dwPeriod = toLargeInt(DI_SECONDS/freq);

  //LogF("Engine %5d,%5d : %5d",iDirX,iDirY,iMag);

  LONG             lDirection[1] = { 1 };

  DIEFFECT         diEffect;
  memset(&diEffect,0,sizeof(diEffect));

  diEffect.dwSize = sizeof(DIEFFECT);
  diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
  //diEffect.dwSamplePeriod = 0;
  //diEffect.dwGain = DI_FFNOMINALMAX;

  diEffect.cAxes = 1;
  diEffect.rglDirection = lDirection;

  diEffect.cbTypeSpecificParams = sizeof(diSpec);
  diEffect.lpvTypeSpecificParams = &diSpec;

  // Get an interface to our new effect
  HRESULT err=_effEngine->SetParameters
    (
    &diEffect,
    //DIEP_DIRECTION|
    DIEP_TYPESPECIFICPARAMS
    );
  DIError("Engine SetParameters",err);
  DWORD status;
  _effEngine->GetEffectStatus(&status);
  if( (status&DIEGES_PLAYING)==0 )
  {
    err=_effEngine->Start(1,0);

    DIError("Engine Start",err);
  }
  _effEngineStarted = true;
}

void Joystick::DoSetSurface( float mag )
{
  _surfaceMag = mag;
}

Joystick::~Joystick()
{
  //Basic rule of thumb:  If it exists... release it
  if( _devJoystick )
  {
    _devJoystick->Acquire();

    FFOff();

    if( _effEngine ) _effEngine->Unload(),_effEngine.Free();
    //if( _effGun ) _effGun->Unload(),_effGun.Free();
    //if( _effForce ) _effForce->Unload(),_effForce.Free();
    if( _effWeapon ) _effWeapon->Unload(),_effWeapon.Free();
    if( _effCenter ) _effCenter->Unload(),_effCenter.Free();

    _devJoystick->Unacquire();

    DIPROPDWORD diPropAutoCenter;
    diPropAutoCenter.diph.dwSize = sizeof(diPropAutoCenter);
    diPropAutoCenter.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diPropAutoCenter.diph.dwObj = 0;
    diPropAutoCenter.diph.dwHow = DIPH_DEVICE;
    diPropAutoCenter.dwData = DIPROPAUTOCENTER_ON;
    _devJoystick->SetProperty(DIPROP_AUTOCENTER,&diPropAutoCenter.diph);

    _devJoystick.Free();
  }
}

#endif

/// Xbox controller

DEFINE_ENUM(JoystickType, JT, JOYSTICK_TYPE_ENUM)
COMPILETIME_COMPARE(NJoystickType, N_JOYSTICK_TYPES)

#if !_DISABLE_GUI

# pragma comment(lib, "XInput")

struct JoystickThresholds
{
  /// button thresholds
  int _tholds[32][2];
  int _tholdStickRX,_tholdStickRY;
  int _tholdStickLX,_tholdStickLY;
};

//! Joystick support encapsulation
class XController : public IJoystick
{
private:
  bool _rumbleEnabled;

  bool _rumbleEngineStarted;
  bool _rumbleSurfaceStarted;
  float _rumbleEngineMag;
  float _rumbleSurfaceMag;

  float _rumbleWeaponBegMag;
  float _rumbleWeaponEndMag;
  DWORD _rumbleWeaponStart;
  DWORD _rumbleWeaponEnd;
  float _rumbleWeaponInvDuration;

  XINPUT_VIBRATION _rumble;

  // coeficients to translate values to motor speed
  float _coefEngineToLeft;
  float _coefEngineToRight;
  float _coefSurfaceToLeft;
  float _coefSurfaceToRight;
  float _coefWeaponToLeft;
  float _coefWeaponToRight;

  JoystickThresholds _thresholds[NJoystickType];

  int _controllerLocked;

  static const int _defTholds[32][2];

#define XUSER_MAX_COUNT 4

#   define XGetPortCount() (XUSER_MAX_COUNT)
  /// check for active controllers still desirable - performance hit on PC for using inactive controller
  bool CheckInputValid(int index) const {return _usedControllers[index];}
  static int GetInputHandle(int index) {return index;}
  typedef int InputHandle;

  JoystickType _inputTypes[XGetPortCount()]; //!< types of connected controllers
  DWORD _times[XGetPortCount()]; //!< times of last activity on controllers
  DWORD _devices; //!< map of detected controllers
  int _lastIndex; //!< index of last used controller
  //JoystickType _type;
  XINPUT_GAMEPAD _lastStates[XGetPortCount()];
  XINPUT_GAMEPAD _available[XGetPortCount()];

  //@{
  //! XInputGetState should be called only for connected controllers 
  //! others are checked only each ConDevCheckTime ms
  static const DWORD ConDevCheckTime = 5000; // 5 seconds
  DWORD _lastConnDevCheckTime;
  bool  _usedControllers[XGetPortCount()];
  //@}

  int _nCenterAxes;

  bool _effEngineStarted;
  bool _effWeaponStarted;

  bool _weaponByConstantForce;
  bool _buttonState[NButtons];
  bool _ffEnabled;
  bool _ffOn;

  float _stiffnessX,_stiffnessY;
  float _engineMag,_engineFreq;
  float _surfaceMag;

public:
  XController();
  ~XController();

#if defined (_WIN32)
  void Init(HINSTANCE instance, HWND hwnd, IDirectInput8 *dInput = NULL);
#else
  void Init();
#endif

  // hat -- angle in degrees * 1000 (90000 , 180000 ...)
  bool Get(
    int axis[NAxes], unsigned short button[NButtons], bool buttonEdge[NButtons], bool buttonUpEdge[NButtons],
    bool buttonState[NButtons],
    int &hatAngle);

  int GetActiveController() const {return _lastIndex;}
  JoystickType GetType() const {return _inputTypes[_lastIndex];}
  /// Return mask of types of all connected controllers
  int GetTypes() const;
  XINPUT_GAMEPAD GetAvailable(RString type) const;

  int LockController() {return ++_controllerLocked;}
  int UnlockController() {return --_controllerLocked;}
  bool IsControllerLocked() const {return _controllerLocked > 0;}

  void SetStiffness(float valueX, float valueY); // how much autocentering is applied
  void SetEngine( float mag, float freq ); // set constant sine
  void SetSurface( float mag ); // set constant sine

  // used to simulate weapons
  void PlayRamp(float begMag, float endMag, float duration); // play ramp

  void FFStart();
  void FFStop();

  void FFOn();
  void FFOff();

  void Enable(bool enable) {}
  bool IsEnabled() const { return false; }

  /// load various device dependent settings from a scheme
  void InitScheme(ParamEntryPar entry);

protected:
  void XboxCloseInput(int index);
  bool XboxOpenInput(int index);

  // we cannot start another rumble until this one completed
  bool CheckIfRumbleCompleted() const;
  void WaitForRumbleCompleted();
  void ProcessRumble();
  void ClearRumble(InputHandle handle);
};

IJoystick *CreateXInput()
{
#if _DISABLE_GUI
  return NULL;
#else

# if ENABLE_JOYSTICK
  return new XController;
# else
  return NULL;
# endif

#endif
}

XController::XController()
{
  for (int t=0; t<NJoystickType; t++)
  {
    JoystickThresholds &th = _thresholds[t];
    for (int i=0; i<lenof(th._tholds); i++)
    {
      th._tholds[i][0] = _defTholds[i][0];
      th._tholds[i][1] = _defTholds[i][1];
    }
    th._tholdStickLX = toInt(0.21f*32767);
    th._tholdStickLY = toInt(0.23f*32767);
    th._tholdStickRX = toInt(0.21f*32767);
    th._tholdStickRY = toInt(0.23f*32767);
  }

  _controllerLocked = 0;

  _engineMag = 0;
  _engineFreq = 0;
  _stiffnessX = 0;
  _stiffnessY = 0;
  _surfaceMag = 0;
}

#define DEF_BUTTON_THOLD {64*128,48*128}
#define DEF_STICK_THOLD {128*128,96*128}
#define DEF_TRIGGER_THOLD {192*128,128*128}
const int XController::_defTholds[32][2]=
{
  // different button have different thresholds
  // when they are considered to be pressed
  // normal button thresholds
  DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD, // A,B,X,Y
  DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD, // DPad Up,Down,Left,Right
  DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD,DEF_BUTTON_THOLD, // START, BACK, Black, White
  DEF_TRIGGER_THOLD,DEF_TRIGGER_THOLD, // triggers (L,R)
  DEF_BUTTON_THOLD,DEF_BUTTON_THOLD, // thumbstick clicks (L,R)
  // stick thresholds
  DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD, // L Right, L Up, R Right, R Up
  DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD, // 
  DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,
  DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,DEF_STICK_THOLD,
};

#if defined(_WIN32)
void XController::Init(HINSTANCE instance, HWND hwnd, IDirectInput8 *dInput)
#else
void XController::Init()
#endif
{
  for (int i=0; i<XGetPortCount(); i++)
  {
    _inputTypes[i] = JTGamepad;
    _times[i] = 0;
    _usedControllers[i] = true;
  }
  _lastConnDevCheckTime = GlobalTickCount();
  _devices = 0;
  _lastIndex = -1;

  _rumbleEnabled = true;
  _ffOn = true;

  _rumbleEngineStarted = false;
  _rumbleEngineMag = 0;

  _rumbleSurfaceStarted = false;
  _rumbleSurfaceMag = 0;

  _rumbleWeaponBegMag = 0;
  _rumbleWeaponEndMag = 0;
  _rumbleWeaponStart = 0;
  _rumbleWeaponEnd = 0;
  _rumbleWeaponInvDuration = 0;

  ZeroMemory(&_rumble, sizeof(_rumble));

  ParamEntryVal cls = Pars >> "CfgRumble";
  _coefEngineToLeft = cls >> "coefEngineToLeft";
  _coefEngineToRight = cls >> "coefEngineToRight";
  _coefSurfaceToLeft = cls >> "coefSurfaceToLeft";
  _coefSurfaceToRight = cls >> "coefSurfaceToRight";
  _coefWeaponToLeft = cls >> "coefWeaponToLeft";
  _coefWeaponToRight = cls >> "coefWeaponToRight";

  for (int i=0; i<NButtons; i++ ) _buttonState[i] = false; 
}

/*
XBox controller mapping:

button[0] = A
button[1] = B
button[2] = X
button[3] = Y
button[4] = DPAD_UP
button[5] = DPAD_DOWN
button[6] = DPAD_LEFT
button[7] = DPAD_RIGHT

button[8] = START
button[9] = BACK

button[10] = BLACK
button[11] = WHITE

button[12] = LEFT_TRIGGER;
button[13] = RIGHT_TRIGGER;

button[14] = LEFT_THUMB;
button[15] = RIGHT_THUMB;

button[16] = ThumbLX>0 (Right)
button[17] = ThumbLY>0 (Up)
button[18] = ThumbRX>0 (Right)
button[19] = ThumbRY>0 (Up)
button[20] = ThumbLX<0 (Left)
button[21] = ThumbLY<0 (Down)
button[22] = ThumbRX<0 (Left)
button[23] = ThumbRY<0 (Down)

*/

static JoystickType GetControllerSubtype(int handle, XINPUT_GAMEPAD &available)
{
  XINPUT_CAPABILITIES cap;
  DWORD result = XInputGetCapabilities(handle, XINPUT_FLAG_GAMEPAD, &cap);
  if (result != ERROR_SUCCESS) return JTGamepad;

  available = cap.Gamepad;
  switch (cap.SubType)
  {
  case XINPUT_DEVSUBTYPE_ARCADE_STICK:
    return JTJoystick;
  case XINPUT_DEVSUBTYPE_WHEEL:
    return JTWheel;
  case XINPUT_DEVSUBTYPE_GAMEPAD:
  default:
    return JTGamepad;
  }
}

void XController::XboxCloseInput(int index)
{
}

bool XController::XboxOpenInput(int index)
{
  _inputTypes[index] = GetControllerSubtype(index, _available[index]);
  return true;
}

//! thumb normalization
/*!
thumb moves in something which resembles circle
we want to map this circle to square
*/
      
static void MapCircleToSquare(int &thumbLX, int &thumbLY)
{
  /*
  float angle = atan2(float(thumbLX),float(thumbLY));
  float size = sqrt(Square(thumbLX)+Square(thumbLY));
  if (size>=1) size = 1;
  // convert circle to square
  */
  float x = thumbLX*(1.0/32767);
  float y = thumbLY*(1.0/32767);
  float x2 = Square(x);
  float y2 = Square(y);
  float d = 1;
  if (x2<y2)
  {
    d = sqrt(1+x2/y2);
  }
  else if (x2>0)
  {
    d = sqrt(1+y2/x2);
  }
  thumbLX = toInt(thumbLX*d);
  thumbLY = toInt(thumbLY*d);
  saturate(thumbLX,-32767,+32767);
  saturate(thumbLY,-32767,+32767);
}

static bool CompareStates(const XINPUT_GAMEPAD &pad1, const XINPUT_GAMEPAD &pad2)
{
  const int allButtons = (
    XINPUT_GAMEPAD_DPAD_UP|XINPUT_GAMEPAD_DPAD_DOWN|
    XINPUT_GAMEPAD_DPAD_LEFT|XINPUT_GAMEPAD_DPAD_RIGHT|
    XINPUT_GAMEPAD_START|XINPUT_GAMEPAD_BACK|
    XINPUT_GAMEPAD_LEFT_THUMB|XINPUT_GAMEPAD_RIGHT_THUMB
      |XINPUT_GAMEPAD_LEFT_SHOULDER|XINPUT_GAMEPAD_RIGHT_SHOULDER
      |XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_B|XINPUT_GAMEPAD_X|XINPUT_GAMEPAD_Y
  );
  if ((pad1.wButtons&allButtons) != (pad2.wButtons&allButtons)) return true;
    {
      int change = pad1.bLeftTrigger-pad2.bLeftTrigger;
      if (abs(change)>10) return true;
      change = pad1.bRightTrigger-pad2.bRightTrigger;
      if (abs(change)>10) return true;
    }

	 int change = pad1.sThumbLX - pad2.sThumbLX;
  if (abs(change) >= 1000) return true;
  change = pad1.sThumbLY - pad2.sThumbLY;
  if (abs(change) >= 1000) return true;
  change = pad1.sThumbRX - pad2.sThumbRX;
  if (abs(change) >= 1000) return true;
  change = pad1.sThumbRY - pad2.sThumbRY;
  if (abs(change) >= 1000) return true;

  return false;
}

static int GetThumbstickDeadzoneAdjusted(int val, int deadzone)
{
  if (val>deadzone)
  {
    return toInt((val-deadzone)*(32767.0f/(32767-deadzone)));
  }
  else if (val<-deadzone)
  {
    return toInt((val+deadzone)*(32767.0f/(32767-deadzone)));
  }
  return 0;
}


/**
Buttons are now digital, button is not index, but rather a mask
*/
static int GetButtonCrossTalkAdjusted(const XINPUT_GAMEPAD &pad, int button)
{
  return (pad.wButtons&button) ? 0x7fff : 0;
}

#define ALL_XBOX_BUTTONS(XX) \
  XX(A) \
  XX(B) \
  XX(X) \
  XX(Y) \
   \
  XX(Up) \
  XX(Down) \
  XX(Left) \
  XX(Right) \
   \
  XX(Start) \
  XX(Back) \
 \
  XX(LeftBumper) \
  XX(RightBumper) \
 \
  XX(LeftTrigger) \
  XX(RightTrigger) \
 \
  XX(LeftThumb) \
  XX(RightThumb)
  
void XController::InitScheme(ParamEntryPar cfg)
{
  for (int t=0; t<NJoystickType; t++)
  {
    JoystickThresholds &th = _thresholds[t];
    ParamEntryVal cls = cfg >> FindEnumName((JoystickType)t);

    ParamEntryVal deadzones = cls >> "ButtonDeadZones";
    #define GET_DEADZONE(name) \
      { \
        ParamEntryVal entry = deadzones>>("deadZone_" #name); \
        th._tholds[XBOX_##name][0] = toInt(float(entry[0])*128); \
        th._tholds[XBOX_##name][1] = toInt(float(entry[1])*128); \
      }
    
    ALL_XBOX_BUTTONS(GET_DEADZONE)

    th._tholdStickLX = toInt(float(cls>>"ThumbStickDeadZones">>"LeftX")*32767);
    th._tholdStickLY = toInt(float(cls>>"ThumbStickDeadZones">>"LeftY")*32767);
    th._tholdStickRX = toInt(float(cls>>"ThumbStickDeadZones">>"RightX")*32767);
    th._tholdStickRY = toInt(float(cls>>"ThumbStickDeadZones">>"RightY")*32767);
  }
}

/*!
\param axis -1000..+1000 analog value
\param button 8b analog value, 0xff = fully pressed, 0x80 = half pressed
\param buttonEdge boolean value if button is pressed since last poll
\param buttonUpEdge boolean value if button is released since last poll
*/

bool XController::Get(
  int axis[NAxes], unsigned short button[NButtons], bool buttonEdge[NButtons], bool buttonUpEdge[NButtons],
  bool buttonState[NButtons], int &hat)
{
  PROFILE_SCOPE(joyXS);

  memset(axis,0,NAxes*sizeof(*axis));
  // note: button must be permanent 
  memset(button,0,NButtons*sizeof(*button));
  memset(buttonEdge,0,NButtons*sizeof(*buttonEdge));
  memset(buttonUpEdge,0,NButtons*sizeof(*buttonUpEdge));
  memset(buttonState,0,NButtons*sizeof(*buttonState));
  hat = -1;

  // TODOX360: check type of the device - XInputGetCapabilities
  
  // store time of activity for all controllers
  XINPUT_STATE states[XGetPortCount()];
  {
    //PROFILE_SCOPE(joyXS);
    DWORD time = GlobalTickCount();
    if (time-_lastConnDevCheckTime > ConDevCheckTime)
    {
      _lastConnDevCheckTime=time;
      // reset state
      for (int i=0; i<XGetPortCount(); i++) _usedControllers[i]=true;
    }
    for (int i=0; i<XGetPortCount(); i++)
    {
      if (_usedControllers[i])
      {
        DWORD errCode = XInputGetState(i, &(states[i]));
        if (errCode==ERROR_SUCCESS)
        {
          const XINPUT_GAMEPAD &pad = states[i].Gamepad;
          if (CompareStates(pad, _lastStates[i]))
          {
            //LogF("State of controller %d changed in time %d", i, time);
            _times[i] = time;
          }

          _lastStates[i] = pad;
        }
        else
        {
          if (errCode == ERROR_DEVICE_NOT_CONNECTED) _usedControllers[i]=false;
          XINPUT_GAMEPAD &pad = states[i].Gamepad;
          ZeroMemory(&pad, sizeof(pad));
        }
      }
      else
      {
        XINPUT_GAMEPAD &pad = states[i].Gamepad;
        ZeroMemory(&pad, sizeof(pad));
      }
    }
  }
//DIAG_MESSAGE(100, Format("Controller 0: last change %d", _times[0]));
//DIAG_MESSAGE(100, Format("Controller 1: last change %d", _times[1]));
//DIAG_MESSAGE(100, Format("Controller 2: last change %d", _times[2]));
//DIAG_MESSAGE(100, Format("Controller 3: last change %d", _times[3]));

  if (!IsControllerLocked())
  {
    // select the best controller
    int best = -1;
    DWORD maxTime = 0;
    for (int i=0; i<XGetPortCount(); i++)
    {
      if (CheckInputValid(i) && (_times[i] > maxTime || best < 0))
      {
        best = i;
        maxTime = _times[i];
      }
    }

    // switch controller to selected?
    if (best < 0)
    {
      _lastIndex = -1;
    }
    else if (best != _lastIndex)
    {
      //LogF("Controller %d is best in time %d", best, time);

      if (_lastIndex < 0 || !CheckInputValid(_lastIndex) || _times[best] > _times[_lastIndex] + 1000)
      {
        if (_lastIndex >= 0)
        {
          if (CheckInputValid(_lastIndex))
          {
            ClearRumble(GetInputHandle(_lastIndex));
          }
          //LogF("Clear rumble %d", _lastIndex);
        }
        _lastIndex = best;
        //LogF("Controller %d selected in time %d", best, time);

        // init rumble when switched to other controller
        WaitForRumbleCompleted();
        ZeroMemory(&_rumble, sizeof(_rumble));
      }
    }
    //DIAG_MESSAGE(100, Format("Best %d, selected %d: time %d", best, _lastIndex, time));
  }

  if (_lastIndex >= 0)
  {
    // check for device type
    JoystickType type = _inputTypes[_lastIndex];
    // DIAG_MESSAGE(100, Format("Controller type: %s", (const char *)FindEnumName(_type)));

    // convert XBox controls to standard joystick controls
    // left hand and right hand is converted to four axes
    const XINPUT_GAMEPAD &pad = states[_lastIndex].Gamepad;
    const JoystickThresholds &th = _thresholds[type];

    int thumbLX = GetThumbstickDeadzoneAdjusted(pad.sThumbLX,th._tholdStickLX);
    int thumbLY = GetThumbstickDeadzoneAdjusted(pad.sThumbLY,th._tholdStickLY);
    int thumbRX = GetThumbstickDeadzoneAdjusted(pad.sThumbRX,th._tholdStickRX);
    int thumbRY = GetThumbstickDeadzoneAdjusted(pad.sThumbRY,th._tholdStickRY);
    
    // thumb normalization
    // thumb moves in something which resembles circle
    // we want to map this circle to square
    
    MapCircleToSquare(thumbLX,thumbLY);
    MapCircleToSquare(thumbRX,thumbRY);

    button[XBOX_A] = GetButtonCrossTalkAdjusted(pad,XINPUT_GAMEPAD_A);
    button[XBOX_B] = GetButtonCrossTalkAdjusted(pad,XINPUT_GAMEPAD_B);
    button[XBOX_X] = GetButtonCrossTalkAdjusted(pad,XINPUT_GAMEPAD_X);
    button[XBOX_Y] = GetButtonCrossTalkAdjusted(pad,XINPUT_GAMEPAD_Y);
    
    button[XBOX_LeftBumper] = (pad.wButtons&XINPUT_GAMEPAD_LEFT_SHOULDER)!=0 ? 0x7fff : 0;
    button[XBOX_RightBumper] = (pad.wButtons&XINPUT_GAMEPAD_RIGHT_SHOULDER)!=0 ? 0x7fff : 0;

    // note: multiplication by (0x7fff/0xff) actually never gives 0x7fff, only 0x7f80
    button[XBOX_LeftTrigger] = pad.bLeftTrigger*(0x7fff/0xff);
    button[XBOX_RightTrigger] = pad.bRightTrigger*(0x7fff/0xff);

    button[XBOX_Up] = (pad.wButtons&XINPUT_GAMEPAD_DPAD_UP)!=0 ? 0x7fff : 0;
    button[XBOX_Down] = (pad.wButtons&XINPUT_GAMEPAD_DPAD_DOWN)!=0 ? 0x7fff : 0;
    button[XBOX_Left] = (pad.wButtons&XINPUT_GAMEPAD_DPAD_LEFT)!=0 ? 0x7fff : 0;
    button[XBOX_Right] = (pad.wButtons&XINPUT_GAMEPAD_DPAD_RIGHT)!=0 ? 0x7fff : 0;

    button[XBOX_Start] = (pad.wButtons&XINPUT_GAMEPAD_START)!=0 ? 0x7fff : 0;
    button[XBOX_Back] = (pad.wButtons&XINPUT_GAMEPAD_BACK)!=0 ? 0x7fff : 0;
    
    button[XBOX_LeftThumb] = (pad.wButtons&XINPUT_GAMEPAD_LEFT_THUMB)!=0 ? 0x7fff : 0;
    button[XBOX_RightThumb] = (pad.wButtons&XINPUT_GAMEPAD_RIGHT_THUMB)!=0 ? 0x7fff : 0;

    button[XBOX_LeftThumbXRight] = thumbLX>0 ? thumbLX : 0;
    button[XBOX_LeftThumbYUp] = thumbLY>0 ? thumbLY : 0;
    button[XBOX_RightThumbXRight] = thumbRX>0 ? thumbRX : 0;
    button[XBOX_RightThumbYUp] = thumbRY>0 ? thumbRY : 0;
    button[XBOX_LeftThumbXLeft] = thumbLX<0 ? -thumbLX : 0;
    button[XBOX_LeftThumbYDown] = thumbLY<0 ? -thumbLY : 0;
    button[XBOX_RightThumbXLeft] = thumbRX<0 ? -thumbRX : 0;
    button[XBOX_RightThumbYDown] = thumbRY<0 ? -thumbRY : 0;

    hat = -1;

    // rumble
    ProcessRumble();

    for( int i=0; i<NButtons; i++ )
    {
      // if button is pressed, request lower treshold to consider it released again
      int thold = th._tholds[i][_buttonState[i]];
      bool nbi = button[i]>thold;

      buttonEdge[i]=( nbi && !_buttonState[i] );
      buttonUpEdge[i]=( !nbi && _buttonState[i] );
      _buttonState[i] = nbi; 
      buttonState[i] = nbi;
    }
  }

  return true;
}

int XController::GetTypes() const
{
  int mask = 0;
  for (int i=0; i<XGetPortCount(); i++)
  {
    if (CheckInputValid(i))
    {
      int type = _inputTypes[i];
      mask |= 1 << type;
    }
  }
  return mask;
}

XINPUT_GAMEPAD XController::GetAvailable(RString type) const
{
  XINPUT_GAMEPAD avail;
  memset(&avail, 0, sizeof(avail));

  JoystickType jt = GetEnumValue<JoystickType>((const char *)type);
  if (jt < 0) return avail;

  for (int i=0; i<XGetPortCount(); i++)
  {
    if (CheckInputValid(i) && _inputTypes[i] == jt)
    {
      avail.wButtons |= _available[i].wButtons;
      avail.bLeftTrigger |= _available[i].bLeftTrigger;
      avail.bRightTrigger |= _available[i].bRightTrigger;
      avail.sThumbLX |= _available[i].sThumbLX;
      avail.sThumbLY |= _available[i].sThumbLY;
      avail.sThumbRX |= _available[i].sThumbRX;
      avail.sThumbRY |= _available[i].sThumbRY;
    }      
  }
  return avail;
}

void XController::ProcessRumble()
{
  // Check to see if we are still transferring current motor value
  float engineMag = 0;
  float surfaceMag = 0;
  float weaponMag = 0;
  if (_rumbleEnabled)
  {
    if (_rumbleEngineStarted) engineMag = _rumbleEngineMag;
    if (_rumbleSurfaceStarted) surfaceMag = _rumbleSurfaceMag;

    DWORD now = GlobalTickCount();
    Assert(now >= _rumbleWeaponStart);
    if (now < _rumbleWeaponEnd)
    {
      float pos = (now - _rumbleWeaponStart) * _rumbleWeaponInvDuration;
      float mag = (1.0f - pos) * _rumbleWeaponBegMag + pos * _rumbleWeaponEndMag;
      weaponMag = fabs(mag);
    }
  }

  const float maxWord = 65535.0f;
  float leftMotor = maxWord * floatMax(_coefEngineToLeft * engineMag, _coefSurfaceToLeft * surfaceMag, _coefWeaponToLeft * weaponMag);
  float rightMotor = maxWord * floatMax(_coefEngineToRight * engineMag, _coefSurfaceToRight * surfaceMag, _coefWeaponToRight * weaponMag);

  if (CHECK_DIAG(DERumble))
  {
    if (engineMag > 0 || surfaceMag > 0 || weaponMag > 0)
    {
      LogF
        (
        " - engine %.3f, surface %.3f, weapon %.3f, left %.3f, right %.3f",
        engineMag, surfaceMag, weaponMag,
        leftMotor / maxWord, rightMotor / maxWord
        );
    }
    DIAG_MESSAGE(100, Format("FF input: engine %.3f, surface %.3f, weapon %.3f", engineMag, surfaceMag, weaponMag));
    DIAG_MESSAGE(
      100, Format(
        "FF output: left %.0f %%, right %.0f %%",
        100 * leftMotor / maxWord, 100 * rightMotor / maxWord
      )
    );
  }

  // values under certain limits are not reliable
  // the behavior depends on previous motor history (was already turning, or not?)  
  const float minLeftMotor = maxWord*0.30f;
  const float minRightMotor = maxWord*0.25f;

  // we ignore values below "ignore" limits, and make saturate to 
  const float minLeftMotorIgnore = maxWord*0.20f;
  const float minRightMotorIgnore = maxWord*0.10f;
  
  if (leftMotor<minLeftMotorIgnore) leftMotor = 0;
  else if (leftMotor<minLeftMotor) leftMotor = minLeftMotor;
  if (rightMotor<minRightMotorIgnore) rightMotor = 0;
  else if (rightMotor<minRightMotor) leftMotor = minRightMotor;
  
  saturate(leftMotor, 0, maxWord);
  saturate(rightMotor, 0, maxWord);

  if (CheckIfRumbleCompleted())
  {
    _rumble.wLeftMotorSpeed = (WORD)toInt(leftMotor);
    _rumble.wRightMotorSpeed = (WORD)toInt(rightMotor);
    if (_lastIndex >= 0 && CheckInputValid(_lastIndex))
    {
      XInputSetState(_lastIndex, &_rumble);
    }
  }
  
//(void)result;
// LogF("Process rumble %d", _lastIndex);
}

bool XController::CheckIfRumbleCompleted() const
{
  return true;
}

void XController::WaitForRumbleCompleted()
{
  // vibration pending is handled by the library on X360
}

void XController::ClearRumble(InputHandle handle)
{
  WaitForRumbleCompleted();
  _rumble.wLeftMotorSpeed = 0;
  _rumble.wRightMotorSpeed = 0;
  // we want to stop vibrating - we need to be sure the device has accepted the instruction
  // TODOX360: alternative could be to implement full shadowing for vibration of all controllers and handle it there
  for(;;)
  {
    DWORD result = XInputSetState(handle, &_rumble);
    if (result!=ERROR_BUSY) break;
  }
}

void XController::FFStart()
{
  _rumbleEnabled = true;
}

void XController::FFStop()
{
  if (_rumbleEnabled)
  {
    _rumbleEnabled = false;
    PlayRamp(0,0,0);
  }
}

void XController::FFOn()
{
  if (_ffOn) return;
  _ffOn = true;
  FFStart();
}

void XController::FFOff()
{
  if (_ffOn) FFStop();
  _ffOn = false;
}

void XController::PlayRamp(float begMag, float endMag, float duration)
{
/*
  LogF("Play ramp %.3f -> %.3f, duration %.3f", begMag, endMag, duration);
*/
  if (duration <= 0) endMag = 0;
  _rumbleWeaponBegMag = begMag;
  _rumbleWeaponEndMag = endMag;
  _rumbleWeaponStart = GlobalTickCount();
  _rumbleWeaponEnd = _rumbleWeaponStart + toInt(1000 * duration);
  _rumbleWeaponInvDuration = duration > 0 ? 1e-3f / duration : 0;
  ProcessRumble();
}


// weapon effects

void XController::SetStiffness(float valueX, float valueY)
{
}

void XController::SetEngine( float mag, float freq )
{
  _rumbleEngineStarted = true;
  _rumbleEngineMag = fabs(mag);
}

void XController::SetSurface( float mag )
{
  _rumbleSurfaceStarted = true;
  _rumbleSurfaceMag = fabs(mag);
}

XController::~XController()
{
  //Basic rule of thumb:  If it exists... release it
}

#endif // #if !_DISABLE_GUI

/*!
\patch 5156 Date 5/11/2007 by Jirka
- Fixed: XInput handling of input devices disabled because of missing configuration options
*/

int GetActiveController()
{
#ifdef _WIN32
  if (GInput.IsXInputPresent()) return XInputDev->GetActiveController();
#endif
  return -1;
}

bool IsControllerLocked()
{
#ifdef _WIN32
  return GInput.IsXInputPresent() ? XInputDev->IsControllerLocked() : false;
#else
  return false;
#endif
}

int LockController()
{
#ifdef _WIN32
  return GInput.IsXInputPresent() ? XInputDev->LockController() : 0;
#else
  return 0;
#endif
}

int UnlockController()
{
#ifdef _WIN32
  return GInput.IsXInputPresent() ? XInputDev->UnlockController() : 0;
#else
  return 0;
#endif
}

bool ParseUserParams(ParamFile &cfg, GameDataNamespace *globals);
void SaveUserParams(ParamFile &cfg);

float JoystickDevices::DeadZoneDefaults[N_JOYSTICK_AXES] = {0.07f, 0.07f, 0, 0.07f, 0.07f, 0.07f, 0, 0};

JoystickDevices::JoystickDevices()
{
#if ENABLE_DI_JOYSTICK
  // CoInit if needed
  HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  _cleanupCOM = SUCCEEDED(hr);
  if ( FAILED(hr) ) RptF("Error: JoystickDevices - CoInitilizeEx return %x", hr);

  // load recognized joysticks from user cfg
  GameDataNamespace globals(NULL, RString(), false); // save - no global namespace
  ParamFile cfg;
  if (ParseUserParams(cfg, &globals))
  {
    LoadJoysticks(cfg);
  }
#endif
}

JoystickDevices::~JoystickDevices()
{
  Resize(0);
#if ENABLE_DI_JOYSTICK
  _dInput.Free();
  if( _cleanupCOM )
    CoUninitialize();
#endif
}

static bool GNewXInputJoystick=false;
#if ENABLE_DI_JOYSTICK
void JoystickDevices::Init(HINSTANCE instance, HWND hwnd, IDirectInput8 *dInput)
{
  if (!dInput)
  {
    HRESULT err = DirectInput8Create(
      instance, DIRECTINPUT_VERSION, IID_IDirectInput8 ,(void **)&dInput, NULL);
    //    HRESULT err = DirectInputCreateEx(instance, DIRECTINPUT_VERSION,IID_IDirectInput7,_dInput.Init(),NULL);
    if (err != DI_OK) return;
  }
  Resize(0); // when called only to reenumerate joysticks, we need to destroy current joysticks
  GInput._joysticksState.Resize(0);
  _dInput += dInput; // make a ComRef, including incrementing RefCount

  // We need to enumerate joysticks.
  // This requires a callback function (EnumJoyCallback) which is where we call the Joystick constructor
  // and do the actual CreateDevice call for the joystick.

#if 1 //_ENABLE_CHEATS
  int joystickCount = _recognizedJoysticks.Size();
  EnumContext context;
  context.system=this;
  context.hwnd=hwnd;
  
  GNewXInputJoystick = false;
  _dInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoyCallback,&context,DIEDFL_ATTACHEDONLY);
  if (GNewXInputJoystick) GInput.SetCustomScheme(false); //make XInput device active in XInput (scheme) mode
  if (joystickCount < _recognizedJoysticks.Size()) SaveJoysticks();
#endif

  SetStiffness(0.2,0.2);
  SetEngine(0,0);
  SetSurface(0);
}

#endif

void JoystickDevices::SaveJoysticks()
{
  { // Save new state
    GameDataNamespace globals(NULL, RString(), false); // save - no global namespace
    ParamFile cfg;
    if (ParseUserParams(cfg, &globals))
    {
      SaveJoysticks(cfg);
#if defined(_WIN32) 
      cfg.Add("customScheme", GInput.customScheme);
      cfg.Add("trackIR", TrackIRDev && TrackIRDev->IsEnabled());
      cfg.Add("freeTrack", FreeTrackDev && FreeTrackDev->IsEnabled());
#endif
      SaveUserParams(cfg);
    }
  }
}

// Force feedback functions
void JoystickDevices::SetStiffness(float valueX, float valueY)
{
  for (int i=0; i<Size(); i++)
  { // set for all joysticks available
    const Ref<IJoystick> joy = Get(i);
    if (joy->IsEnabled())
      joy->SetStiffness(valueX, valueY);
  }
}
void JoystickDevices::SetEngine(float mag, float freq)
{
  for (int i=0; i<Size(); i++)
  { // set for all joysticks available
    const Ref<IJoystick> joy = Get(i);
    if (joy->IsEnabled())
      joy->SetEngine(mag, freq);
  }
}
void JoystickDevices::SetSurface(float mag)
{
  for (int i=0; i<Size(); i++)
  { // set for all joysticks available
    const Ref<IJoystick> joy = Get(i);
    if (joy->IsEnabled())
      joy->SetSurface(mag);
  }
}
// used to simulate weapons
void JoystickDevices::PlayRamp(float begMag, float endMag, float duration)
{
  for (int i=0; i<Size(); i++)
  { // set for all joysticks available
    const Ref<IJoystick> joy = Get(i);
    if (joy->IsEnabled())
      joy->PlayRamp(begMag, endMag, duration);
  }
}
void JoystickDevices::FFOn()
{
  for (int i=0; i<Size(); i++)
  { // set for all joysticks available
    const Ref<IJoystick> joy = Get(i);
    if (joy->IsEnabled())
      joy->FFOn();
  }
}
void JoystickDevices::FFOff()
{
  for (int i=0; i<Size(); i++)
  { // set for all joysticks available
    const Ref<IJoystick> joy = Get(i);
    if (joy->IsEnabled())
      joy->FFOff();
  }
}

int JoystickDevices::GetJoystickOffset(const GUID &guid, const char *productName, bool isXInput)
{
  // try to find whether given GUID is already known
  RString guidStr = Format("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
    guid.Data1, guid.Data2, guid.Data3,
    guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
    guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]
  );
  int maxOffset = 0;
  for (int i=0; i<_recognizedJoysticks.Size(); i++)
  {
    if (guidStr == _recognizedJoysticks[i].guid)
    { // known
      return _recognizedJoysticks[i].offset;
    }
    if (_recognizedJoysticks[i].offset > maxOffset) 
      maxOffset = _recognizedJoysticks[i].offset;
  }
  // assign next available offset
  int ix = _recognizedJoysticks.Add();
  RecognizedJoystick &joy = _recognizedJoysticks[ix];
  joy.offset = maxOffset + OffsetStep;
  joy.guid = guidStr;
  joy.name = productName;
  joy.mode = (!isXInput || (isXInput && GInput.customScheme)) ? JMCustom : JMDisabled;
  joy.isXInput = isXInput;
  // load the default Joystick keys for the new joystick!
  GInput.AssignDefaultJoystickKeys(joy.offset);
  if (isXInput) GNewXInputJoystick=true;  // when new XInput device is plugged in, configure it as XInput scheme device
  return joy.offset;
}

Array<float> JoystickDevices::GetSensitivity(int offset) const
{
  for (int i=0; i<_recognizedJoysticks.Size(); i++)
  {
    if (_recognizedJoysticks[i].offset == offset)
    { // known
      if (_recognizedJoysticks[i].sensitivity.Size())
        return _recognizedJoysticks[i].sensitivity;
      break; // use default joystick sensitivity
    }
  }
  return Array<float>(GInput.joystickSensitivity, N_JOYSTICK_AXES*2);
}

void JoystickDevices::SetSensitivity(int joyIx, const AutoArray<float> &sensitivity)
{
  int offset = Get(joyIx)->GetOffset();
  Assert(sensitivity.Size()==N_JOYSTICK_AXES*2);
  for (int i=0; i<_recognizedJoysticks.Size(); i++)
  {
    if (_recognizedJoysticks[i].offset == offset)
    { // known
      _recognizedJoysticks[i].sensitivity = sensitivity;
      break;
    }
  }
  for (int i=0; GInput._joysticksState.Size(); i++)
  {
    if (GInput._joysticksState[i].offset == offset)
    { // known
      GInput._joysticksState[i].sensitivity = sensitivity;
      break;
    }
  }
}

AutoArray<float> JoystickDevices::GetDeadZone(int offset) const
{
  for (int i=0; i<_recognizedJoysticks.Size(); i++)
  {
    if (_recognizedJoysticks[i].offset == offset)
    { // known
      return _recognizedJoysticks[i].deadZone;
    }
  }
  return AutoArray<float>(); //no deadZone
}

void JoystickDevices::SetDeadZone(int joyIx, const AutoArray<float>& deadZone)
{
  const Ref<IJoystick> joy = Get(joyIx);
  int offset = joy->GetOffset();
  joy->SetDeadZone(deadZone);
  for (int i=0; i<_recognizedJoysticks.Size(); i++)
  {
    if (_recognizedJoysticks[i].offset == offset)
    { // known
      _recognizedJoysticks[i].deadZone = deadZone;
      break;
    }
  }
  for (int i=0; GInput._joysticksState.Size(); i++)
  {
    if (GInput._joysticksState[i].offset == offset)
    { // known
      GInput._joysticksState[i].deadZone = deadZone;
      break;
    }
  }
}

void JoystickDevices::ReloadDeadZone(int joyIx, const AutoArray<float>& deadZone)
{
  const Ref<IJoystick> joy = Get(joyIx);
  joy->SetDeadZone(deadZone);
}

AutoArray<float> JoystickDevices::GetSteepness(int offset) const
{
  for (int i=0; i<_recognizedJoysticks.Size(); i++)
  {
    if (_recognizedJoysticks[i].offset == offset)
    { // known
      return _recognizedJoysticks[i].steepness;
    }
  }
  return AutoArray<float>(); //no steepness
}

void JoystickDevices::SetSteepness(int joyIx, const AutoArray<float>& steepness)
{
  const Ref<IJoystick> joy = Get(joyIx);
  int offset = joy->GetOffset();
  for (int i=0; i<_recognizedJoysticks.Size(); i++)
  {
    if (_recognizedJoysticks[i].offset == offset)
    { // known
      _recognizedJoysticks[i].steepness = steepness;
      break;
    }
  }
  for (int i=0; GInput._joysticksState.Size(); i++)
  {
    if (GInput._joysticksState[i].offset == offset)
    { // known
      GInput._joysticksState[i].steepness = steepness;
      break;
    }
  }
}

bool JoystickDevices::IsXInput(int offset) const
{
  for (int i=0; i<_recognizedJoysticks.Size(); i++)
  {
    if (_recognizedJoysticks[i].offset == offset)
    { // known
      return _recognizedJoysticks[i].isXInput;
    }
  }
  return false;
}

void JoystickDevices::SaveJoysticks(ParamFile &userCfg)
{
  userCfg.Delete("JoysticksList");
  ParamClassPtr joysticks = userCfg.AddClass("JoysticksList");
  for (int i=0; i<_recognizedJoysticks.Size(); i++)
  {
    RString joyLabel = Format("Joystick%d", i+1); //start from 1
    ParamClassPtr joy = joysticks->AddClass(joyLabel);
    joy->Add("guid", _recognizedJoysticks[i].guid);
    joy->Add("name", _recognizedJoysticks[i].name);
    joy->Add("offset", _recognizedJoysticks[i].offset);
    joy->Add("isXInput", _recognizedJoysticks[i].isXInput);
    RecognizedJoystick &recJoy = _recognizedJoysticks[i];
    if (recJoy.sensitivity.Size())
    {
      ParamEntryPtr sensitivity = joy->AddArray("sensitivity");
      for (int j=0; j<recJoy.sensitivity.Size(); j++)
      {
        sensitivity->AddValue(recJoy.sensitivity[j]);
      }
    }
    if (recJoy.deadZone.Size())
    {
      ParamEntryPtr deadZone = joy->AddArray("deadZone");
      for (int j=0; j<intMin(N_JOYSTICK_AXES, recJoy.deadZone.Size()); j++)
      {
        deadZone->AddValue(recJoy.deadZone[j]);
      }
    }
    if (recJoy.steepness.Size())
    {
      ParamEntryPtr steepness = joy->AddArray("steepness");
      for (int j=0; j<intMin(N_JOYSTICK_AXES, recJoy.steepness.Size()); j++)
      {
        steepness->AddValue(recJoy.steepness[j]);
      }
    }
    const EnumName *names = GetEnumNames(recJoy.mode);
    joy->Add("mode", GetEnumName(names, recJoy.mode));
  }
}

void JoystickDevices::LoadJoysticks(ParamFile &userCfg)
{
  ParamEntryPtr joysticks = userCfg.FindEntry("JoysticksList");
  if (!joysticks || !joysticks->IsClass()) return;
  for (int i=0; i<joysticks->GetEntryCount(); i++)
  {
    RString joyLabel = Format("Joystick%d", i+1); //start from 1
    ParamEntryPtr joy = joysticks->FindEntry(joyLabel);
    if (!joy || !joy->IsClass()) continue;
    int ix = _recognizedJoysticks.Add();
    RecognizedJoystick &newJoy = _recognizedJoysticks[ix];
    newJoy.guid = *joy >> "guid";
    newJoy.name = *joy >> "name";
    newJoy.offset = *joy >> "offset";
    newJoy.isXInput = bool(*joy >> "isXInput");
    ParamEntryPtr sensitivity = joy->FindEntry("sensitivity");
    if (sensitivity && sensitivity->IsArray()) 
    {
      int count = sensitivity->GetSize();
      newJoy.sensitivity.Realloc(2*N_JOYSTICK_AXES);
      newJoy.sensitivity.Resize(2*N_JOYSTICK_AXES);
      for (int j=0; j<2*N_JOYSTICK_AXES; j++) newJoy.sensitivity[j]=0;
      for (int j=0; j<count; j++)
      {
        float val = float((*sensitivity)[j]);
        saturate(val, -1.0f, 1.0f);
        newJoy.sensitivity[j]=val;
        if (count<N_JOYSTICK_AXES) newJoy.sensitivity[N_JOYSTICK_AXES+j]=val;
      }
    }
    ParamEntryPtr deadZone = joy->FindEntry("deadZone");
    if (deadZone && deadZone->IsArray()) 
    {
      int count = intMin(deadZone->GetSize(),N_JOYSTICK_AXES);
      newJoy.deadZone.Realloc(N_JOYSTICK_AXES);
      newJoy.deadZone.Resize(N_JOYSTICK_AXES);
      for (int j=0; j<N_JOYSTICK_AXES; j++)
      {
        if (j<count) newJoy.deadZone[j]=float((*deadZone)[j]);
        else newJoy.deadZone[j]=DeadZoneDefaults[j];
      }
    }
    ParamEntryPtr steepness = joy->FindEntry("steepness");
    if (steepness && steepness->IsArray()) 
    {
      int count = intMin(steepness->GetSize(),N_JOYSTICK_AXES);
      newJoy.steepness.Realloc(N_JOYSTICK_AXES);
      newJoy.steepness.Resize(N_JOYSTICK_AXES);
      for (int j=0; j<N_JOYSTICK_AXES; j++)
      {
        if (j<count)newJoy.steepness[j]=float((*steepness)[j]);
        else newJoy.steepness[j]=1.0f;
      }
    }
    RString modeStr = *joy >> "mode";
    newJoy.mode = GetEnumValue<JoystickMode>(cc_cast(modeStr));
    // mode should be stored also in GInput
    for (int j=0; j<GInput._joysticksState.Size(); j++)
    {
      if (GInput._joysticksState[j].offset==newJoy.offset)
      {
        GInput._joysticksState[j].mode = newJoy.mode; 
        break;
      }
    }
  }
}

JoystickDevices::RecognizedJoystick* JoystickDevices::GetJoystickInfo(int offset)
{
  for (int i=0; i<_recognizedJoysticks.Size(); i++)
  {
    if (_recognizedJoysticks[i].offset==offset)
    {
      return &_recognizedJoysticks[i];
    }
  }
  return NULL;
}
