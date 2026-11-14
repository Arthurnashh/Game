#include "wpch.hpp"
#include "global.hpp"
#include "inputInterface.h"
#include "keyInput.hpp"
#include "world.hpp"
#include "dikCodes.h"
#include "El/enum/enumNames.hpp"
#include "El/Interfaces/iAppInfo.hpp"

extern bool GAppPaused;
extern int SkipKeysUntilFrame;
extern bool GAppActive;
extern bool UseWindow;

const EnumName& GetCheatName(int index);

RString GetKeyName(int dikCode);

#ifdef _WIN32
	extern INSTANCE_HANDLE  hInstApp;
#endif


#if _ENABLE_CHEATS

//-----------------------------------------------------------------------------
struct RawMouseHistoryItem
{
	float pos[2];
	float deltaT;
};

/// circular history buffer
static RawMouseHistoryItem RawMouseHistory[128];

/// time of the last item recored in RawMouseHistory
static DWORD RawMouseHistoryTime = ::GetTickCount();

/// index of last item recored in RawMouseHistory
static int CurMouseHistory;


//-----------------------------------------------------------------------------
class MouseAxisGraph: public IDiagGraph
{
	friend struct Input;

	int _axis;
	Input *_input;

	#pragma warning(disable:4351)
	float _ballistics[Input::MouseBufValSize];

public:
	MouseAxisGraph(Input *input, int axis):_input(input),_axis(axis),_ballistics(){}

	virtual RString GetName() const {return _axis ? "Mouse Y" : "Mouse X";}
	virtual PackedColor GetDataColor(int set) const
	{
		if (set==0) return PackedColor(Color(1,1,0.5,1));
		else if (set==1) return PackedColor(Color(1,0.5,0,1));
		return PackedColor(Color(0,1,0,1));
	}

	virtual int GetDataSetCount() const {return 3;}
	virtual int GetDataSetSize(int set) const
	{
		if (set==1) return lenof(RawMouseHistory);
		else return lenof(_input->_mouseBufVal);
	}
	float GetMaxTime() const {return 2500;}
	virtual void GetDataSetItem(int set, int i, float &dx, float &y) const
	{
		int index = (_input->_mouseBufValIx+Input::MouseBufValSize-i)%Input::MouseBufValSize;
		dx = _input->_mouseBufVal[index].deltaT*1000;
		if (set==0)
		{
			y = _input->_mouseBufVal[index].offset[_axis]/dx;
		}
		else if (set==1)
		{
			// display raw data as set 1
			// read 
			const RawMouseHistoryItem &item = RawMouseHistory[(CurMouseHistory+lenof(RawMouseHistory)-i)%lenof(RawMouseHistory)];
			y = item.pos[_axis];
			dx = item.deltaT;
		}
		else
		{
			y = _ballistics[index];
		}
	}

	bool ValidForFPSMode( int fpsMode ) const {return fpsMode==FpsMouse;}
};

#endif


#if ENABLE_KEYMOUSE

// this buffer will contain all keyboard changes during one frame
const int KeyboardBufferSize=256;

//make the buffer big enough as nowadays mice are really quick
const int MouseBufferSize=512;

//-----------------------------------------------------------------------------
struct RawKeyboardData
{
	RAWKEYBOARD _data;
	DWORD _time;
};
static RawKeyboardData KeyboardBuffer[KeyboardBufferSize];
int KeyboardBufferPos = 0;

//-----------------------------------------------------------------------------
struct RawMouseData
{
	RAWMOUSE _data;
	DWORD _time;
};
static CircularArray<RawMouseData,MouseBufferSize> MouseBuffer;


/// convert virtual key to DIK - caution, some VKs map to multiple DIKs
int VKey2DikCode[256];

/// convert DIK to virtual key
int Dik2VKeyCode[256];

//-----------------------------------------------------------------------------
/*
keyboard translation table
*/
//-----------------------------------------------------------------------------
static const int VKey2DikCodeTranslationTable[][2] =
{
	// dinput key, virtual key
	{ DIK_ESCAPE,			VK_ESCAPE},
	{ DIK_1,				'1'},
	{ DIK_2,				'2'},
	{ DIK_3,				'3'},
	{ DIK_4,				'4'},
	{ DIK_5,				'5'},
	{ DIK_6,				'6'},
	{ DIK_7,				'7'},
	{ DIK_8,				'8'},
	{ DIK_9,				'9'},
	{ DIK_0,				'0'},
	{ DIK_MINUS, 			VK_OEM_MINUS},
	{ DIK_EQUALS,		 	VK_OEM_PLUS},
	{ DIK_BACK, 			VK_BACK},
	{ DIK_TAB, 			VK_TAB},
	{ DIK_Q,				'Q'},
	{ DIK_W,				'W'},
	{ DIK_E,				'E'},
	{ DIK_R,				'R'},
	{ DIK_T,				'T'},
	{ DIK_Y,				'Y'},
	{ DIK_U,				'U'},
	{ DIK_I,				'I'},
	{ DIK_O,				'O'},
	{ DIK_P,				'P' },
	{ DIK_LBRACKET, 		VK_OEM_4},
	{ DIK_RBRACKET, 		VK_OEM_6},
	{ DIK_RETURN, 		VK_RETURN},
	{ DIK_LCONTROL, 		VK_LCONTROL},
	{ DIK_A,				'A'},
	{ DIK_S,				'S' },
	{ DIK_D,				'D'},
	{ DIK_F,				'F'},
	{ DIK_G,				'G'},
	{ DIK_H,				'H'},
	{ DIK_J,				'J'},
	{ DIK_K,				'K'},
	{ DIK_L,				'L'},
	{ DIK_SEMICOLON,		VK_OEM_1},
	{ DIK_APOSTROPHE,		VK_OEM_7},
	{ DIK_GRAVE, 			VK_OEM_3},
	{ DIK_LSHIFT, 		VK_LSHIFT},
	{ DIK_BACKSLASH, 		VK_OEM_5},
	{ DIK_Z,				'Z'},
	{DIK_X,				'X'},
	{DIK_C,				'C'},
	{DIK_V,				'V'},
	{DIK_B,				'B'},
	{DIK_N,				'N' },
	{DIK_M,				'M' },
	{DIK_COMMA,			VK_OEM_COMMA },
	{DIK_PERIOD, 		VK_OEM_PERIOD },
	{DIK_SLASH, 			VK_OEM_2 },
	{DIK_RSHIFT, 		VK_RSHIFT},
	{DIK_MULTIPLY, 		VK_MULTIPLY },
	{DIK_LMENU, 			VK_LMENU},
	{DIK_SPACE, 			VK_SPACE },
	{DIK_CAPITAL, 		VK_CAPITAL},
	{DIK_F1,				VK_F1 },
	{DIK_F2,				VK_F2},
	{DIK_F3,				VK_F3},
	{DIK_F4,				VK_F4},
	{DIK_F5,				VK_F5},
	{DIK_F6,				VK_F6},
	{DIK_F7,				VK_F7},
	{DIK_F8,				VK_F8},
	{DIK_F9,				VK_F9},
	{DIK_F10,			VK_F10},
	{DIK_NUMLOCK,		VK_NUMLOCK},
	{DIK_SCROLL,			VK_SCROLL},
	{DIK_NUMPAD7,		VK_NUMPAD7},
	{DIK_NUMPAD8,		VK_NUMPAD8},
	{DIK_NUMPAD9,		VK_NUMPAD9},
	{DIK_SUBTRACT,		VK_SUBTRACT},
	{DIK_NUMPAD4,		VK_NUMPAD4},
	{DIK_NUMPAD5,		VK_NUMPAD5},
	{DIK_NUMPAD6,		VK_NUMPAD6},
	{DIK_ADD,			VK_ADD},
	{DIK_NUMPAD1,		VK_NUMPAD1},
	{DIK_NUMPAD2,		VK_NUMPAD2},
	{DIK_NUMPAD3,		VK_NUMPAD3},
	{DIK_NUMPAD0,		VK_NUMPAD0},
	{DIK_DECIMAL,		VK_DECIMAL},
	{DIK_F11,			VK_F11},
	{DIK_F12,			VK_F12},
	{DIK_F13,			VK_F13},
	{DIK_F14,			VK_F14},
	{DIK_F15,			VK_F15},
	{DIK_NUMPADENTER,	VK_RETURN},
	{DIK_RCONTROL,		VK_RCONTROL},
	{DIK_DIVIDE,			VK_DIVIDE},
	{DIK_SYSRQ, 			0},
	{DIK_RMENU,			VK_RMENU},
	{DIK_HOME,			VK_HOME},
	{DIK_UP,				VK_UP},
	{DIK_PRIOR,			VK_PRIOR},
	{DIK_LEFT,			VK_LEFT},
	{DIK_RIGHT,			VK_RIGHT },
	{DIK_END,			VK_END},
	{DIK_DOWN,			VK_DOWN},
	{DIK_NEXT,			VK_NEXT},
	{DIK_INSERT,			VK_INSERT},
	{DIK_DELETE,			VK_DELETE},
	{DIK_LWIN,			VK_LWIN},
	{DIK_RWIN,			VK_RWIN},
	{DIK_APPS,			VK_APPS},
	{DIK_PAUSE,			VK_PAUSE},
	{0,					VK_CANCEL},
	{DIK_MUTE,			VK_VOLUME_MUTE},
	{DIK_VOLUMEDOWN,		VK_VOLUME_DOWN},
	{DIK_VOLUMEUP,		VK_VOLUME_UP},
	{DIK_WEBHOME,		VK_BROWSER_HOME},
	{DIK_WEBSEARCH,		VK_BROWSER_SEARCH},
	{DIK_WEBFAVORITES,	VK_BROWSER_FAVORITES},
	{DIK_WEBREFRESH,		VK_BROWSER_REFRESH },
	{DIK_WEBSTOP,		VK_BROWSER_STOP},
	{DIK_WEBFORWARD,		VK_BROWSER_FORWARD},
	{DIK_WEBBACK,		VK_BROWSER_BACK},
	{DIK_MAIL,			VK_LAUNCH_MAIL},
	{DIK_MEDIASELECT,	VK_LAUNCH_MEDIA_SELECT},
};

//-----------------------------------------------------------------------------
static struct VKey2DikInit
{
	VKey2DikInit();
} SVKey2DikInit;

//-----------------------------------------------------------------------------
VKey2DikInit::VKey2DikInit()
{
	for (int i=0; i<sizeof(VKey2DikCodeTranslationTable)/sizeof(*VKey2DikCodeTranslationTable); i++)
	{
		Assert(Dik2VKeyCode[VKey2DikCodeTranslationTable[i][0]]==0);
		VKey2DikCode[VKey2DikCodeTranslationTable[i][1]]=VKey2DikCodeTranslationTable[i][0];
		Dik2VKeyCode[VKey2DikCodeTranslationTable[i][0]]=VKey2DikCodeTranslationTable[i][1];
	}
}


//-----------------------------------------------------------------------------
void ProcessMouseBuffer(const RAWMOUSE& data)
{
	// note: the mouse processing is typically not done here, but in a similar code in RawInputWndProc
	if (MouseBuffer.EmptySpace(0)>0)
	{
		RawMouseData mouseData = {data, ::GetTickCount()};
		MouseBuffer.Put(mouseData);
	}
}

//-----------------------------------------------------------------------------
void ProcessKeyboardBuffer(const RAWKEYBOARD& data)
{
	if (KeyboardBufferPos < KeyboardBufferSize)
	{
		//LogF(
		//  "$$ Keyboard raw data %x (%s+%x)",
		//  raw->data.keyboard.VKey,cc_cast(GetKeyName(raw->data.keyboard.MakeCode)),raw->data.keyboard.Flags
		//);
		KeyboardBuffer[KeyboardBufferPos]._data = data;
		KeyboardBuffer[KeyboardBufferPos]._time = ::GetTickCount();
		KeyboardBufferPos++;
	}
}

#endif

//-----------------------------------------------------------------------------
void ProcessKeyboard(DWORD sysTime, DWORD timeDelta)
{
#if ENABLE_KEYMOUSE
	if (IsDedicatedClient() || IsDedicatedServer()) return;
	// check for the Steam overlay
	bool overlayRunning = false;
#if _ENABLE_STEAM
	if (UseSteam)
	{
		overlayRunning = GInput.steamOverlayActive;
	}
#endif

	if (GAppPaused || SkipKeysUntilFrame >= GInput.frameID || overlayRunning)
	{
		// ignore keyboard input
		GInput.ForgetKeys();
	}
	else
	{
		float invTimeDelta = timeDelta > 0 ? 1.0 / timeDelta : 1000;
		for (int k=0; k<256; k++)
		{
			GInput.keys[k] = 0;
			GInput.keysDoubleTapToDo[k] = GInput.keysToDo[k] = false; // avoid key accumulation
			GInput.keysUp[k] = false; // avoid key accumulation
			// switch level checking to the next frame
			GInput.keysMaxLevelLastFrame[k] = GInput.keysMaxLevelThisFrame[k];
			GInput.keysMaxLevelThisFrame[k] = 0;
		}

#if !_VBS3 //we want to allow ESC key
		if (GWorld->IsUserInputEnabled())
#endif
		{
			for (int i=0; i<KeyboardBufferPos; i++)
			{
				const RawKeyboardData &data = KeyboardBuffer[i];
				int key = data._data.MakeCode;
				bool down = (data._data.Flags & 0x01) == RI_KEY_MAKE;
				if (data._data.Flags & RI_KEY_E0) key += 0x80;
				if (data._data.Flags & RI_KEY_E1)
				{ // hack: RI_KEY_E1 is not documented well, so convert VKey into DIK_CODE, as we do not know details 
					//      (key DIK_PAUSE was behaving as DIK_LCONTROL without it)
					if (data._data.VKey>=0 && data._data.VKey<256)
						key = VKey2DikCode[data._data.VKey];
				}
#if _VBS3
				if(!GWorld->IsUserInputEnabled() && key != DIK_ESCAPE) continue;
#endif
				//DIAG_MESSAGE(100, Format("Key MakeCode = 0x%x, Flags = 0x%x, VKey = 0x%x, Message = 0x%x, ExtraInformation = 0x%x", (int)data._data.MakeCode, (int)data._data.Flags, (int)data._data.VKey, (int)data._data.Message, (int)data._data.ExtraInformation))
				//LogF("%d: Key MakeCode = 0x%x, Flags = 0x%x, VKey = 0x%x, Message = 0x%x, ExtraInformation = 0x%x", data._time, (int)data._data.MakeCode, (int)data._data.Flags, (int)data._data.VKey, (int)data._data.Message, (int)data._data.ExtraInformation);
				if (data._data.VKey==0xff)
				{ // FIX: additional 0xff keys caused crash after being assigned with double tap, see news:gnlsvu$v4h$1@new-server.localdomain
					// 0xFF key is generated for ArrowKeys and PgUp and other nearby keys
					//LogF("^^^ Key Code SKIPPED!");
					continue;
				}

				const int DoubleTapTimeDelta = 400; //ms
				if (down)
				{
					// key pressed
					if (GInput.keyPressed[key] > GInput.keyReleased[key])
					{
						// ignore autorepeat
						// LogF(" - ignored");
					}
					else
					{
						// test double tap before the keyPressed timestamp change
						// LogF(" ... %d - %d = %d", data._time, GInput.keyLastPressed[key], data._time - GInput.keyLastPressed[key]);
						if ((data._time - GInput.keyLastPressed[key]) < DoubleTapTimeDelta)
						{
							GInput.keysDoubleTapToDo[key] = true;
						}
						GInput.keyLastPressed[key] = GInput.keyPressed[key] = data._time;
						GInput.keyKeepAlive[key] = false;
						GInput.ProcessKeyPressed(key);
					}
				}
				else
				{
					if (GInput.keyPressed[key])
					{
						// key released
						GInput.UnblockButton(key+INPUT_DEVICE_KEYBOARD);
						DWORD start = GInput.keyPressed[key];
						GInput.keyPressed[key] = 0;
						GInput.keyAutorepeat[key] = 0;
						GInput.keysUp[key] = true;
						if (!GInput.keysProcessed[key])
						{
							if (start < sysTime - timeDelta) start = sysTime - timeDelta;
							DWORD howLong = data._time - start;
							GInput.keys[key] += howLong * invTimeDelta;
						}
						GInput.keysProcessed[key] = false;
						// if double click was read in the last frame, delay button release
						GInput.keyReleased[key] = data._time;
						GInput.keyDoubleClickAndHold[key] = false; //possible double click continuation should be stopped
						if (GInput.keyDoubleClickWanted[key] && ((data._time - GInput.keyLastPressed[key]) < DoubleTapTimeDelta))
							GInput.keyKeepAlive[key] = true;
					}
					else
					{
						LogF("Key released twice");
					}
				}
			}
			for (int k=0; k<256; k++)
			{
				//make released buttons still active, if the button is tested for double click 
				//and time quantum has not elapsed yet
				GInput.keyDoubleClickWanted[k]=false; //reset
				if (GInput.keyKeepAlive[k]) 
				{
					const int releaseDelay=200;
					int relTime = sysTime - GInput.keyReleased[k];
					if (relTime < releaseDelay) 
					{ 
						GInput.keys[k]=1;
						GInput.keyPressed[k]=GInput.keyReleased[k]; 
					}
					else {
						GInput.keys[k] = 0;
						GInput.keyPressed[k]=0;
						GInput.keyKeepAlive[k]=false;
					}
				}
				// check for any non-released keys
				if (!GInput.keysProcessed[k] && GInput.keyPressed[k])
				{
					DWORD howLong=sysTime-GInput.keyPressed[k];
					if (howLong<=0) howLong=1; //for double taps, we need key pressed activated immediately
					if( howLong>=timeDelta ) GInput.keys[k]=1;
					else GInput.keys[k]+=howLong*invTimeDelta;
				}
				// make holding keys with useDoubleTapAndHold set active
				if (GInput.keyDoubleClickAndHold[k] && GInput.keys[k]>0)
					GInput.keysDoubleTapToDo[k]=true;
			}
			GInput.combinationsWanted.Update();

			GInput.rControlActive = GInput.keys[DIK_RCONTROL]>0;
			if (GInput.keys[DIK_LSHIFT] && GInput.keys[DIK_NUMPADMINUS])
			{
				//GlobalShowMessage(1000,"Cheat started");
				GInput.awaitCheat = true;
				GInput.cheatInProgress = "";
			}
		}
	}
	KeyboardBufferPos = 0;
#endif
}





//-----------------------------------------------------------------------------
void Input::ReInitDIJoysticks()
{
#if ENABLE_DI_JOYSTICK
	if (GJoystickDevices) 
	{
#if 0
		static int count = 1;
		LogF("JoyReInit count=%d, hwndApp=0x%x, hInstApp=0x%x, dInput=0x%x", count++, hwndApp, hInstApp, dInput);
#endif
		GJoystickDevices->Init((HINSTANCE)hInstApp, (HWND)hwndApp, dInput);
	}
#endif
};


//-----------------------------------------------------------------------------
void Input::RemoveInaccessableKeys()
{
#if ENABLE_DI_JOYSTICK
	for (int i=0; i<UAN; i++)
	{
		KeyList &keys = userKeys[i];
		backupJoysticksKeys[i].Resize(0);
		for (int j=0; j<keys.Size(); j++)
		{
			switch (keys[j] & INPUT_DEVICE_MASK)
			{
			case INPUT_DEVICE_STICK:
			case INPUT_DEVICE_STICK_AXIS:
			case INPUT_DEVICE_STICK_POV:
				int offset = ((keys[j] - (keys[j] & INPUT_DEVICE_MASK)) / JoystickDevices::OffsetStep) * JoystickDevices::OffsetStep;
				int joyIx = -1;
				for (int ix=0; ix<_joysticksState.Size(); ix++)
				{ // for default joystick mappings (offset==0) it holds too
					if (_joysticksState[ix].offset == offset) { joyIx = ix; break; }
				}
				if (joyIx<0/* || _joysticksState[joyIx].mode==JMDisabled*/) // Disabled Joysticks values should be shown in "grey" color
				{ // joystick with given offset is not connected now - remove key
					backupJoysticksKeys[i].Add(keys[j]); //backup these joysticks keys
					keys.Delete(j);
					j--; // there is different key under this index
				}
				break;
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
void Input::GetAverage(float &speedX, float &speedY, int offsetX, int offsetY, int timeDelta)
{
  #if _ENABLE_CHEATS
  const int fastStepLimit = 50;
  const int fastStep = 10;
  if( GInput.GetCheat1ToDo(DIK_UP) )
  {
    if (_filterMouse<fastStepLimit) _filterMouse++;
    else _filterMouse += fastStep;
    if (_filterMouse>200) _filterMouse = 200;
    DIAG_MESSAGE(500,"Mouse filtering %d",_filterMouse);
  }
  if( GInput.GetCheat1ToDo(DIK_DOWN) )
  {
    if (_filterMouse<=fastStepLimit) _filterMouse--;
    else _filterMouse -= fastStep;
    if (_filterMouse<0) _filterMouse = 0;
    DIAG_MESSAGE(500,"Mouse filtering %d",_filterMouse);
  }
  #endif

  // filter slow values over longer time
  // the primary purpose of the filter is to handle slow motion
  // when mouse is moving slow, it only sends a sample once per a few frames
  // with high fps this would cause high speed motion in one frame and then zero speed in several frames
  _mouseBufValIx = (_mouseBufValIx+1)%MouseBufValSize;
  
  float deltaT = timeDelta/1000.0f;
  _mouseBufVal[_mouseBufValIx]= MouseVal(deltaT,offsetX, offsetY);
  
  if (_filterMouse)
  {
    speedX = FilterMouseAxis(0,offsetX,deltaT);
    speedY = FilterMouseAxis(1,offsetY,deltaT);
  }
  else
  {
    speedX = offsetX/deltaT;
    speedY = offsetY/deltaT;
  }
}

//-----------------------------------------------------------------------------
void Input::OnMouseButton(int button, bool down, DWORD time)
{
#if RMB_HOLD_CLICK_DBL
  if (button == 1) // RMB
  {
    /* 
    Button activity in time:  ____|''p1''|____r____|''p2''|____
    i.e. p1, p2 how long was the button pressed, r release time between
    Detection:
    Click ... (p1<ClickTime && (!DblClickInLastFrameWanted || p1+r>=DblClickTime)) ||
    (ButtonPressed && !DblClickInLastFrameWanted && !HoldInLastFrameWanted)
    Hold  ... p1>=ClickTime || (ButtonPressed && !ClickInLastFrameWanted && !DblClickInLastFrameWanted)
    Double click ... p1+r<DblClickTime && p1<ClickTime
    */
    if (down)
    {
      // RMB pressed
      mouseButtons[button] = 1; //mouseButtons is duplicated in mouseButtonsHold, which works little bit different mostly in RMB handling (distinguish between click and hold)
      mouseButtonsToDo[button]++;
      if ((time - mouseButtonsPressed[button] < mouseDoubleClickTime) &&
        (mouseButtonsReleasedTime[button] - mouseButtonsPressed[button] < mouseClickTime))
      {
        mouseButtonsDblClickDetected[button] = true;
        mouseButtonsDoubleToDo[button]++; //dbl click
        if (!mouseButtonDoubleClickWanted[button] && !mouseButtonHoldWanted[button])
        { //double click should be considered as single click too
          mouseButtonsClickToDo[button]=true;
          mouseButtonsClickToDoAlreadySet[button]=true; //do not set it again
        }
      }
      else if (!mouseButtonDoubleClickWanted[button] && !mouseButtonHoldWanted[button])
      {
        mouseButtonsClickToDo[button]=true; //click
        mouseButtonsClickToDoAlreadySet[button]=true; //do not set it again
        mouseButtonsHold[button]=1;         //this is possibly used in map editor
      }
      else if (/*!mouseButtonDoubleClickWanted[btn] && */!mouseButtonClickWanted[button])
      {
        mouseButtonsHold[button]=1; //hold
      }            
      mouseButtonsPressed[button] = time;
      mouseButtonsKeepAlive[button]=false;
    }
    else
    {
      // RMB released
      UnblockButton(INPUT_DEVICE_MOUSE+button);
      mouseButtonsReleased[button]++;
      mouseButtons[button] = mouseButtonsHold[button] = 0;
      if ((time - mouseButtonsPressed[button] < mouseClickTime) && 
        !mouseButtonDoubleClickWanted[button] &&
        mouseButtonsReleasedTime[button]!=mouseButtonsPressed[button] && 
        /*second click of doubleClick is click only if last frame there was no DoubleClick wanted*/
        (!mouseButtonsDblClickDetected[button] || !mouseButtonDoubleClickWanted[button]) 
      )
      {
        if (!mouseButtonsClickToDoAlreadySet[button])
          mouseButtonsClickToDo[button]=true; //click
        mouseButtonsReleasedTime[button] = mouseButtonsPressed[button] = time; //click cannot be recognized again (really needed, as mouseButtonDoubleClickWanted[btn] changes sometimes)
      }
      else if (mouseButtonsReleasedTime[button]==mouseButtonsPressed[button])
      { //second click of doubleClick is NOT click
        mouseButtonsReleasedTime[button] = mouseButtonsPressed[button] = time-mouseDoubleClickTime;
      }
      else mouseButtonsReleasedTime[button] = time;
      
      mouseButtonsClickToDoAlreadySet[button]=false;
      
      //click cannot be recognized again (really needed, as mouseButtonDoubleClickWanted[btn] changes sometimes)
      if (mouseButtonsDblClickDetected[button]) 
        mouseButtonsReleasedTime[button] = mouseButtonsPressed[button] = time-mouseDoubleClickTime; 
      // if double click was read in the last frame, delay button release
      if (mouseButtonDoubleClickWanted[button] && !mouseButtonsDblClickDetected[button] && 
        (time - mouseButtonsPressed[button]) < mouseDoubleClickTime)
        mouseButtonsKeepAlive[button]=true;
      mouseButtonsDblClickDetected[button] = false;
    }
  }
  else
#endif
  {
    if (down)
    {
      // button pressed
      mouseButtons[button] = mouseButtonsHold[button] = 1;
      mouseButtonsToDo[button]++;
      if ((time - mouseButtonsPressed[button]) < mouseDoubleClickTime)
      {
        mouseButtonsDoubleToDo[button]++;
        mouseButtonsDblClickDetected[button] = true;
        mouseButtonsPressed[button] = time-mouseDoubleClickTime;
      }
      else mouseButtonsPressed[button] = time;
      mouseButtonsKeepAlive[button]=false;
      //LogF("Mouse btn %d pressed, time=%ud, dblClickDetected=%d", btn, mouseButtonsPressed[btn], mouseButtonsKeepAlive[btn], mouseButtonsDblClickDetected[btn]);
    }
    else
    {
      // button released
      UnblockButton(INPUT_DEVICE_MOUSE+button);
      mouseButtonsReleased[button]++;
      mouseButtons[button] = mouseButtonsHold[button] = 0;
      // if double click was read in the last frame, delay button release
      mouseButtonsReleasedTime[button] = time;
      if (mouseButtonDoubleClickWanted[button] && !mouseButtonsDblClickDetected[button]) 
        mouseButtonsKeepAlive[button]=true;
      mouseButtonsDblClickDetected[button] = false;
      //LogF("Mouse btn %d released, time=%ud, keepAlive=%d", btn, mouseButtonsReleasedTime[btn], mouseButtonsKeepAlive[btn]);
    }
  }
}

//-----------------------------------------------------------------------------
/*!
based on http://www.microsoft.com/whdc/device/input/pointer-bal.mspx

\patch 5245 Date 3/22/2008 by Ondra
- Improved: Mouse ballistics allowing for more precise aiming.
*/
//-----------------------------------------------------------------------------
static float Accelerate(float mag)
{
  // values used in WinXP for "Low" acceleration (from registry SmoothMouseXCurve/SmoothMouseYCurve)
  //static const float xCurve[]={0x0, 0x6e15,0x14000, 0x3dc29, 0x280000};
  //static const float yCurve[]={0x0,0x15eb8,0x54ccd,0x184ccd,0x2380000};
  // attempt to provide a better values
  const float sc = 0x10000;
  static const float xCurve[]={0x0, 0x6e15/sc,0x14000/sc, 0x3dc29/sc, 0x280000/sc};
  // slower motion with slow motion
  static const float yCurve[]={0x0, 0x37ae/sc,0x13333/sc, 0x5f333/sc, 0x8c0000/sc};
  // XP curve / 4
  //static const float yCurve[]={0x0, 0x57ae/sc,0x15333/sc, 0x61333/sc, 0x8e0000/sc};
  static bool ballistics = true;
  if (!ballistics) return 1.0f;
  
  // find the x-point
  float x = mag;
  for (int i=1; i<lenof(xCurve); i++)
  {
    if (xCurve[i]>x)
    {
      // we have found the point - perform the interpolation
      return Interpolativ(x,xCurve[i-1],xCurve[i],yCurve[i-1],yCurve[i]);
    }
  }
  int i = lenof(xCurve)-1;
  // now we know the mouse is moving very fast
  // extrapolate the last segment (taken from Interpolativ function)
  float cMin = xCurve[i-1], cMax = xCurve[i];
  float vMin = yCurve[i-1], vMax = yCurve[i];
  return ((x-cMin)/(cMax-cMin)*(vMax-vMin)+vMin);
}

//-----------------------------------------------------------------------------
static void MouseBallistics( float &mouseSpeedX, float &mouseSpeedY)
{
  float mag = sqrt(Square(mouseSpeedX)+Square(mouseSpeedY));
  float acc = mag>0 ? Accelerate(mag)/mag : 1.0f;
  mouseSpeedX *= acc;
  mouseSpeedY *= acc;
}

//-----------------------------------------------------------------------------
void Input::MouseClearWhenCannotProcess()
{
  // clear unblocked buttons
  for (int i=0; i<_blockedInput.Size(); i++)
  {
    if (!_blockedInput[i]._blocked)
    {
      _blockedInput.DeleteAt(i);
      i--; //process again
    }
  }
#ifdef _WIN32
  mouseLToDo=false;
  mouseRToDo=false;
  mouseMToDo=false;

  for (int i=0; i<N_MOUSE_BUTTONS; i++)
  {
    mouseButtonsDoubleToDo[i] = 0;
    mouseButtonsToDo[i] = 0;
    mouseButtonsClickToDo[i]=false;
    mouseButtonsReleased[i]=0;
    // switch level checking to the next frame
    mouseMaxLevelLastFrame[i] = mouseMaxLevelThisFrame[i];
    mouseMaxLevelThisFrame[i] = 0;
  }
  for (int i=0; i<N_MOUSE_AXES_INC_Z; i++)
  {
    // switch level checking to the next frame
    mouseAxisMaxLevelLastFrame[i] = mouseAxisMaxLevelThisFrame[i];
    mouseAxisMaxLevelThisFrame[i] = 0;
  }
#endif
}


//-----------------------------------------------------------------------------
/*!
\patch 5117 Date 1/15/2007 by Bebul
- Fixed: Mouse button triple click was misinterpreted as 2x double click
*/
//-----------------------------------------------------------------------------
void Input::ProcessMouse(DWORD timeDelta)  // used from outside too
{
  // Clear mouse variables (are to be set now]
  MouseClearWhenCannotProcess();
#ifdef _WIN32
  int deltaT = timeDelta;saturateMax(deltaT, 1);

  float moveX = 0;
  float moveY = 0;

  mouseZ=0;

#if ENABLE_KEYMOUSE
  // mouse input - not used yet
  // every time you read input try to acquire mouse
  // TODO: remove mouseXToDo

  if (IsDedicatedClient() || IsDedicatedServer()) return;

  /*
  if (!UseWindow || !IsAppPaused())
  {
    SetForegroundWindow((HWND)hwndApp);
  }
  */

  if (!GWorld->IsUserInputEnabled())
  {
    _lastProcessMouse = ::GetTickCount();
    return;
  }

  if (GAppPaused || SkipKeysUntilFrame >= GInput.frameID)
  {
    // ignore mouse input
  }
  else
  {
    int mouseX = 0, mouseY = 0;
    
    bool historyRecorded = false;
    
    while (MouseBuffer.CountPending(0)>0)
    {
      const RawMouseData &data = MouseBuffer.Get(0);
      // x axis
      if (data._data.lLastX != 0) mouseX += data._data.lLastX;
      // y axis
      if (data._data.lLastY != 0) mouseY += data._data.lLastY;
      
      #if _ENABLE_CHEATS
      if (data._data.lLastX!=0 || data._data.lLastY != 0)
      {
        float deltaT = data._time-RawMouseHistoryTime;
        if (deltaT>0)
        {
          RawMouseHistoryItem &item = RawMouseHistory[CurMouseHistory%lenof(RawMouseHistory)];
          item.pos[0] = data._data.lLastX/deltaT;
          item.pos[1] = data._data.lLastY/deltaT;
          item.deltaT = deltaT;
          CurMouseHistory++;
          historyRecorded = true;
        }
        RawMouseHistoryTime = data._time;
      }
      #endif
      // mouse wheel
      if (data._data.usButtonFlags & RI_MOUSE_WHEEL) mouseZ += (short)data._data.usButtonData;
      // mouse buttons
      if (data._data.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) OnMouseButton(mouseButtonsReversed ? 1 : 0, true, data._time);
      if (data._data.usButtonFlags & RI_MOUSE_BUTTON_1_UP) OnMouseButton(mouseButtonsReversed ? 1 : 0, false, data._time);
      if (data._data.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) OnMouseButton(mouseButtonsReversed ? 0 : 1, true, data._time);
      if (data._data.usButtonFlags & RI_MOUSE_BUTTON_2_UP) OnMouseButton(mouseButtonsReversed ? 0 : 1, false, data._time);
      if (data._data.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) OnMouseButton(2, true, data._time);
      if (data._data.usButtonFlags & RI_MOUSE_BUTTON_3_UP) OnMouseButton(2, false, data._time);
      if (data._data.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) OnMouseButton(3, true, data._time);
      if (data._data.usButtonFlags & RI_MOUSE_BUTTON_4_UP) OnMouseButton(3, false, data._time);
      if (data._data.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) OnMouseButton(4, true, data._time);
      if (data._data.usButtonFlags & RI_MOUSE_BUTTON_5_UP) OnMouseButton(4, false, data._time);
      MouseBuffer.Advance(0);
    }
    if (!historyRecorded)
    {
      #if _ENABLE_CHEATS
      DWORD now = ::GetTickCount();
      if (deltaT>0)
      {
        RawMouseHistoryItem &item = RawMouseHistory[CurMouseHistory%lenof(RawMouseHistory)];
        item.pos[0] = 0;
        item.pos[1] = 0;
        item.deltaT = now-RawMouseHistoryTime;
        CurMouseHistory++;
      }
      RawMouseHistoryTime = now;
      #endif
    }
    
    for (int i=0; i<N_MOUSE_BUTTONS; i++)
    { //make released buttons still active, iff the button is tested for double click 
      //and time quantum has not elapsed yet
#if RMB_HOLD_CLICK_DBL
      if (i==1) //RMB
      {
        // What remains to detect:
        // Click ... (p1<ClickTime && p1+r>=DblClickTime) && DblClickInLastFrameWanted
        // Hold  ... p1>=ClickTime
        if ((mouseButtonsReleasedTime[i] > mouseButtonsPressed[i]) && mouseButtonDoubleClickWanted[i])
        {
          unsigned int p1 = mouseButtonsReleasedTime[i] - mouseButtonsPressed[i];
          if (p1 < mouseClickTime)
          {
            DWORD sysTime=::GetTickCount();
            unsigned int r = sysTime - mouseButtonsReleasedTime[i];
            if (p1+r >= mouseDoubleClickTime)
            { //we must also disable detecting this click in next ProcessMouse call
              mouseButtonsReleasedTime[i] = mouseButtonsPressed[i];
              mouseButtonsClickToDo[i]=true;
              mouseButtonsClickToDoAlreadySet[i]=true;
            }
          }
        }
        else if (mouseButtonsReleasedTime[i] < mouseButtonsPressed[i]) //if equal, then click already detected
        {
          DWORD sysTime=::GetTickCount();
          unsigned int p1 = sysTime - mouseButtonsPressed[i];
          if (p1 >= mouseClickTime) mouseButtonsHold[i]=1; //hold was possibly delayed due to mouseButtonClickWanted[btn]
        }
      }
#endif
      if (mouseButtonsKeepAlive[i]) 
      {
        int releaseDelay=200;
        DWORD sysTime=::GetTickCount();
        int relTime = sysTime - mouseButtonsReleasedTime[i];
        if (relTime < releaseDelay) mouseButtonsHold[i]=1;
        else {
          mouseButtons[i]=mouseButtonsHold[i]=0;
          mouseButtonsKeepAlive[i]=false;
        }
        //LogF("Mouse btn %d keptAlive!, time=%ud, mouseButtons=%.1f", i, sysTime, mouseButtons[i]);
      }
      mouseButtonDoubleClickWanted[i]=false; //reset
      mouseButtonClickWanted[i]=false; //reset
      mouseButtonHoldWanted[i]=false; //reset
    }
    
    #if _ENABLE_CHEATS
      static int mouseStep = 30;
      if (GInput.GetCheat3ToDo(DIK_NUMPAD4)) mouseX -= mouseStep;
      if (GInput.GetCheat3ToDo(DIK_NUMPAD6)) mouseX += mouseStep;
      if (GInput.GetCheat3ToDo(DIK_NUMPAD8)) mouseY -= mouseStep;
      if (GInput.GetCheat3ToDo(DIK_NUMPAD2)) mouseY += mouseStep;
    #endif

    float mouseFactorX =  mouseSensitivityX * 1.5f;
    float mouseFactorY =  mouseSensitivityY * 1.5f;
    
    moveX = mouseX*(1.0f/200)*mouseFactorX;
    moveY = mouseY*(1.0f/150)*mouseFactorY;
    
    float mouseSpeedX,mouseSpeedY;
    for (int i=0; i<N_MOUSE_AXES; i++) mouseAxis[i]=0.0f;
    GetAverage(mouseSpeedX, mouseSpeedY, mouseX, mouseY, deltaT);
    
    mouseSpeedX *= mouseFactorX*0.001f;
    mouseSpeedY *= mouseFactorY*0.001f;
    
    // this is mouse acceleration
/*    float preBalX = mouseSpeedX, preBalY = mouseSpeedY;
    MouseBallistics(mouseSpeedX, mouseSpeedY);
    (void)preBalX,(void)preBalY;*/

    #if _ENABLE_CHEATS
    if (GEngine && GEngine->ShowFpsMode()>0)
    {
      static Ref<MouseAxisGraph> xAxisGraph = new MouseAxisGraph(this,0);
      static Ref<MouseAxisGraph> yAxisGraph = new MouseAxisGraph(this,1);
      GEngine->ShowDiagGraph(xAxisGraph);
      GEngine->ShowDiagGraph(yAxisGraph);
      
      xAxisGraph->_ballistics[_mouseBufValIx] = mouseSpeedX;
      yAxisGraph->_ballistics[_mouseBufValIx] = mouseSpeedY;
    }
    #endif

		#if 0
    DIAG_MESSAGE(500,Format(
      "Prebal %+6.3f,%+6.3f, Balist %+6.3f,%+6.3f, acc %.2f",mouseSpeedX, mouseSpeedY, preBalX, preBalY,
      preBalX!=0 ? mouseSpeedX/preBalX : 1
      
    ));
		#endif
    mouseSpeedX *= GetMouseSensitivityMultX();
    mouseSpeedY *= GetMouseSensitivityMultY();
    if (mouseSpeedX < 0.0f) mouseAxis[0] = -mouseSpeedX; //mouseMovesLeft
    else mouseAxis[1] = mouseSpeedX; //mouseMovesRight
    if (mouseSpeedY < 0.0f) mouseAxis[2] = -mouseSpeedY; //mouseMovesBackward
    else mouseAxis[3] = mouseSpeedY; //mouseMovesForward
    for (int i=0; i<N_MOUSE_AXES; i++) 
    {
      if (mouseAxis[i]<MouseAxisThresholdLow) mouseAxisCleared[i] = true;
    }
  }
  
  // scan buttons (old version)
  mouseL = mouseButtons[0]>0;
  mouseR = mouseButtons[1]>0;
  mouseM = mouseButtons[2]>0;
  mouseLToDo = mouseButtonsToDo[0]!=0;
  mouseRToDo = mouseButtonsToDo[1]!=0;
  mouseMToDo = mouseButtonsToDo[2]!=0;
  _lastProcessMouse = ::GetTickCount();

  // limit movement - helps in low framerates
  const float limitX=0.6;
  const float limitY=0.4;
  saturate(moveX,-limitX,+limitX);
  saturate(moveY,-limitY,+limitY);

// Cheat to revert mouse axis (temporarily) removed, see news:gsplqr$6kl$1@new-server.localdomain
#if _ENABLE_CHEATS && 0
  if (GetCheat1ToDo(DIK_SLASH))
    revMouse=!revMouse;
#endif

#endif

  if (fabs(moveX)>0 || fabs(moveY)>0 || mouseL || mouseR || mouseM)
  {
    mouseLastActive = Glob.uiTime;
  }
  // record change
  // update UI mouse
  // used also for map
  //cursorX+=moveX;
  //cursorY+=moveY;

  cursorMovedZ += 0.01 * mouseZ;
  mouseWheelDown = mouseWheelUp = 0.0f;
  if (cursorMovedZ<0) mouseWheelDown = -cursorMovedZ;
  else mouseWheelUp = cursorMovedZ;

  // record coordinate
  // saturation for whole screen, not only for 2D region
  AspectSettings as;
  GEngine->GetAspectSettings(as);

  float screenTopX = as.uiTopLeftX/(as.uiTopLeftX-as.uiBottomRightX);
  float screenTopY = as.uiTopLeftY/(as.uiTopLeftY-as.uiBottomRightY);

  //float screenBotX * (as.uiBottomRightX-as.uiTopLeftX) + as.uiTopLeftX = 1;
  float screenBotX = (1-as.uiTopLeftX)/(as.uiBottomRightX-as.uiTopLeftX);
  float screenBotY = (1-as.uiTopLeftY)/(as.uiBottomRightY-as.uiTopLeftY);

  // convert from 0..1 to -1 .. +1
  saturate(cursorX,screenTopX*2-1,screenBotX*2-1);
  saturate(cursorY,screenTopY*2-1,screenBotY*2-1);
  //saturate(cursorY,-1,1);
#endif
}


#if _ENABLE_STEAM

//-----------------------------------------------------------------------------
void Input::RegisterCallbacks()
{
	if (UseSteam) _callbackOverlayActivated.Register(this, &Input::OnOverlayActivated);
}

extern bool SteamOverlayClosed;

//-----------------------------------------------------------------------------
void Input::OnOverlayActivated(GameOverlayActivated_t *param)
{
	steamOverlayActive = param->m_bActive != 0;
	if (!steamOverlayActive) 
	{
		SteamOverlayClosed = true;
	}
}

#include "UI/resincl.hpp"

//-----------------------------------------------------------------------------
void Input::OnServerChangeRequested(GameServerChangeRequested_t *param)
{
	LogF("STEAM ServerChangeRequested %s [%s]", param->m_rgchServer, param->m_rgchPassword);
	// check we are inside Main or MultiplayerDisplay dialogs
	if (GWorld && GWorld->Options())
	{
		int idd = GWorld->Options()->GetTopID();
		if (idd==IDD_MULTIPLAYER || idd==IDD_MAIN)
		{
			changeServer = param->m_rgchServer;
			changeServerPassword = param->m_rgchPassword; 
		}
	}
}

//-----------------------------------------------------------------------------
void Input::OnPersonaStateChanged(PersonaStateChange_t *param)
{
#if 0
	// NOTE: we want player name to be configurable inside user profile
	if (param->m_ulSteamID==SteamUser()->GetSteamID().ConvertToUint64())
	{ //it is me
		if (param->m_nChangeFlags & k_EPersonaChangeName)
		{
			RString steamPersonalName;
			if (SteamFriends()) 
			{
				steamPersonalName = SteamFriends()->GetPersonaName();
				if (!steamPersonalName.IsEmpty()) Glob.header.SetPlayerName(steamPersonalName);
			}
		}
	}
#endif
}
#endif


//-----------------------------------------------------------------------------
float Input::FilterMouseAxis(int axis, int offset, float duration)
{
	const float maxDeltaT = 0.2f;
	int ix;

	for(ix=1; duration<maxDeltaT && ix<MouseBufValSize; ix++)
	{
		// + MouseBufValSize needed to keep number positive
		int newix = (_mouseBufValIx-ix+MouseBufValSize)%MouseBufValSize;
		// never go over the values which are already blocked by the barrier
		if (newix==_mouseBarrierIx[axis]) break;
		// any fast movement means we do not to perform filtering, as we get precise data
		// filtering is needed only when mouse is moving that slow mouse sensors trigger only very seldom
		if (abs(_mouseBufVal[newix].offset[axis])>_filterMouse)
			break;
		duration += _mouseBufVal[newix].deltaT;
		offset += _mouseBufVal[newix].offset[axis];
	}
	// the sample determined by ix was already rejected
	_mouseBarrierIx[axis] = (_mouseBufValIx-ix+MouseBufValSize)%MouseBufValSize;
	return offset/duration;
}

//-----------------------------------------------------------------------------
/*!
\patch 5117 Date 1/15/2007 by Bebul
-Changed: Right Ctrl used for key combination instead of left one. (Right Ctrl no longer mapped to any actions.)
*/
//-----------------------------------------------------------------------------
void Input::ProcessKeyPressed(int dik)
{
#ifdef _WIN32
#if 1
	if (keyPressed[DIK_LALT] || keyPressed[DIK_RALT])
	{
		if (dik==DIK_NUMPADENTER || dik==DIK_RETURN)
		{
			if (GAppActive && GEngine && GEngine->IsCursorShown() && !GEngine->IsInOptionsVideo())
			{
				GEngine->SwitchWindowed(!UseWindow);
				GInput.keyPressed[dik] = 0; // pretend the key was never pressed
				return;
			}
		}
	}
#endif
	//LogF("$$ ProcessKeyPressed %x (%s)",dik,cc_cast(GetKeyName(dik)));
	// detect release-mode cheats
	if (!awaitCheat)
	{
		keysToDo[dik]=true; //UI and buldozer tests DIK_LCONTROL itself
		return;
	}
	// check if we match any cheat
	RString key = GetKeyName(dik);
	// check if we match some complete or incomplete cheat name
	cheatInProgress = cheatInProgress + key;
	//LogF("Cheat in progress: '%s' ('%s')",(const char *)key,(const char *)CheatInProgress);

	int len = cheatInProgress.GetLength();
	bool matchPossible = false;
	for (int i=0; ; i++)
	{
		const EnumName &chName = GetCheatName(i); //CheatCodeNames[i];
		if (!chName.IsValid()) break;
		if (!strcmpi(chName.name,cheatInProgress))
		{
			awaitCheat = false;
			cheatActive = (CheatCode)chName.value;
			// offer the cheat to the mission event handler (restricted to CheatUnlockArmory for now)
			if (cheatActive == CheatUnlockArmory)
			{
				RString handler;
				ConstParamEntryPtr entry = ExtParsMission.FindEntry("onCheat");
				if (entry) handler = *entry;
				if (handler.GetLength() > 0)
				{
					GameVarSpace local(false);
					GameState *gstate = GWorld->GetGameState();
					int gshandle = gstate->BeginContext(&local);
					gstate->VarSetLocal("_this", GameValue(chName.name), true);
					if (gstate->EvaluateBool(handler, GameState::EvalContext::_default, GWorld->GetMissionNamespace())) CheatServed(); // mission namespace
					gstate->EndContext(gshandle);
				}
			}
			GlobalShowMessage(1000,"Activated %s",(const char *)cheatInProgress);
			return;
		}
		if (!strnicmp(chName.name,cheatInProgress,len))
		{
			matchPossible = true;
#if PROTECTION_ENABLED

			__asm
			{
				jmp Skip
					// Integrity check
					// here emit enough 0 to be on 4B boundary
					align 4
					// value here should be replaced with external program
					_emit SUM_ADJUST_MAGIC&0xff
					_emit (SUM_ADJUST_MAGIC>>8)&0xff
					_emit (SUM_ADJUST_MAGIC>>16)&0xff
					_emit (SUM_ADJUST_MAGIC>>24)&0xff
Skip:
			}
#endif
		}
	}
	if (!matchPossible)
	{
		//GlobalShowMessage(1000,"Cheat canceled");
		awaitCheat = false;
	}
	else
	{
		//GlobalShowMessage(1000,"Progress %s",(const char *)CheatInProgress);
	}
#endif
}


#if ENABLE_KEYMOUSE
#pragma comment(lib,"dinput8")

static HWND rawInputWindow;
static HANDLE rawInputThread;

//-----------------------------------------------------------------------------
static LRESULT CALLBACK RawInputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PROFILE_SCOPE_EX(rawIP,*);
	if (hwnd==rawInputWindow) switch (msg)
	{
	case WM_DESTROY:
		rawInputWindow = NULL;
		break;
	case WM_INPUT:
		{
			// check the needed size
			unsigned int size = 0;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
			// allocate the buffer
			AutoArray<char, MemAllocLocal<char,256> > buffer;
			buffer.Resize(size);
			// read the data
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer.Data(), &size, sizeof(RAWINPUTHEADER));

			RAWINPUT* raw = (RAWINPUT*)buffer.Data();
			// DIAG_MESSAGE(500, Format("Input type: %d (wParam = %d)", raw->header.dwType, wParam));
			switch (raw->header.dwType)
			{
			case RIM_TYPEMOUSE:
				if (MouseBuffer.EmptySpace(0)>0)
				{
					RawMouseData data = {raw->data.mouse,::GetTickCount()};
					MouseBuffer.Put(data);
				}
				break;
			case RIM_TYPEKEYBOARD:
#if 0 // ENABLE_KEYMOUSE
				if (KeyboardBufferPos < KeyboardBufferSize)
				{
					KeyboardBuffer[KeyboardBufferPos]._data = raw->data.keyboard;
					KeyboardBuffer[KeyboardBufferPos]._time = ::GetTickCount();
					KeyboardBufferPos++;
				}
#endif
				break;
			case RIM_TYPEHID:
				break;
			}
		}
		break;
	}
	if (IsWindowUnicode(hwnd))
		return DefWindowProcW(hwnd,msg,wParam,lParam);
	else
		return DefWindowProc(hwnd,msg,wParam,lParam);
}


//-----------------------------------------------------------------------------
struct RawInputThreadContext
{
	bool exclusive;
};

//! Event guarding the rawInputWindow is created before the DestroyMouseDevice posts the WM_CLOSE message to it
Event RawInputThreadEvent(true);

//-----------------------------------------------------------------------------
static DWORD CALLBACK RawInputThread(LPVOID parameter)
{
	const RawInputThreadContext *ctx = (RawInputThreadContext *)parameter;

	// create a window
	rawInputWindow = CreateWindowExW(  
		WS_EX_NOPARENTNOTIFY,
		L"Invisible Message Window",     // Class name
		L"Invisible Message Window",     // Caption
		WS_OVERLAPPED /*WS_CAPTION|WS_MINIMIZEBOX|WS_SYSMENU|WS_SIZEBOX*/,
		0, 0,   // Position
		10,10,             // Size
		HWND_MESSAGE,                   // Parent window (message only)
		NULL,                   // use class menu
		(HINSTANCE)hInstApp,               // handle to window instance
		NULL                    // no params to pass on
		);

	RAWINPUTDEVICE devices[1];
	devices[0].usUsagePage = 0x01; // generic page 
	devices[0].usUsage = 0x02; // generic mouse 
	devices[0].dwFlags = ctx->exclusive ? RIDEV_NOLEGACY|RIDEV_CAPTUREMOUSE : 0;
	devices[0].hwndTarget = rawInputWindow;
	if (!RegisterRawInputDevices(devices, lenof(devices), sizeof(*devices)))
	{
		RptF("RegisterRawInputDevices failed with 0x%x", GetLastError());
		return false;
	}

	RawInputThreadEvent.Set();
	MSG msg;
	// now pump the message loop for the window, until we are requested to terminate
	//while( PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
	// TODO: to be useful for joysticks we need to use PeekMessage + joystick polling
	while (rawInputWindow && GetMessageA(&msg, 0, 0, 0))
	{
		PROFILE_SCOPE_EX(inMsg,input);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	delete ctx;
	return 0;

}

//-----------------------------------------------------------------------------
void DestroyMouseDevice()
{
	if (rawInputThread)
	{
		PROFILE_SCOPE_EX(desMD,*);
		RawInputThreadEvent.Wait();
		PostMessage(rawInputWindow,WM_CLOSE,0,0);

		WaitForSingleObject(rawInputThread,INFINITE);
		CloseHandle(rawInputThread);
		rawInputThread = NULL;
	}
}

//-----------------------------------------------------------------------------
bool CreateMouseDevice(HWND hwnd, bool exclusive)
{
	PROFILE_SCOPE_EX(crMD,*);
	DestroyMouseDevice();

	GInput.InitMouseVariables();

	RAWINPUTDEVICE devices[1];
	devices[0].usUsagePage = 0x01; // generic page 
	devices[0].usUsage = 0x02; // generic mouse 
	devices[0].dwFlags = exclusive ? RIDEV_NOLEGACY|RIDEV_CAPTUREMOUSE : 0;
	devices[0].hwndTarget = hwnd;
	if (!RegisterRawInputDevices(devices, lenof(devices), sizeof(*devices)))
	{
		RptF("RegisterRawInputDevices failed with 0x%x", GetLastError());
		return false;
	}
	static bool once = true;
	if (once)
	{
		once = false;

		// create a window class
		WNDCLASSW cls;
		cls.hCursor        = NULL;
		cls.hIcon          = NULL;
		cls.lpszMenuName   = NULL;
		cls.lpszClassName  = L"Invisible Message Window";
		cls.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
		cls.hInstance      = (HINSTANCE)hInstApp;
		cls.style          = 0; // CS_VREDRAW | CS_HREDRAW;
		cls.lpfnWndProc    = (WNDPROC)RawInputWndProc;
		cls.cbClsExtra     = 0;
		cls.cbWndExtra     = 0;

		if( !RegisterClassW(&cls) ) return NULL;
	}


#if 1
	{
		PROFILE_SCOPE_EX(crMDT,*);
		RawInputThreadContext *ctx = new RawInputThreadContext;
		ctx->exclusive = exclusive;
		DWORD rawInputThreadId;
		DoAssert(rawInputThread==NULL);
		rawInputThread = CreateThread(NULL,64*1024,&RawInputThread,ctx,CREATE_SUSPENDED,&rawInputThreadId);

		RegisterThread(GetCurrentThreadId(), 0, "Main Thread");

		SetThreadPriority(rawInputThread,THREAD_PRIORITY_HIGHEST);
		RawInputThreadEvent.Reset();
		ResumeThread(rawInputThread);
	}
#endif

	return true;
}

//-----------------------------------------------------------------------------
bool CreateKeyboard(HWND hwnd, bool firstTime)
{
	RAWINPUTDEVICE devices[1];
	devices[0].usUsagePage = 0x01; // generic page 
	devices[0].usUsage = 0x06; // generic keyboard
	// we use input sink so that we can track key status even when not in foreground
	// without this we had problem of tracking Alt key state when switching between windowed/fullscreen
	devices[0].dwFlags = RIDEV_NOHOTKEYS|RIDEV_INPUTSINK;
	devices[0].hwndTarget = hwnd;
	if (!RegisterRawInputDevices(devices, lenof(devices), sizeof(*devices)) && firstTime)
	{
		RptF("RegisterRawInputDevices failed with 0x%x", GetLastError());
		return false;
	}

	return true;
}

#endif //ENABLE_KEYMOUSE
