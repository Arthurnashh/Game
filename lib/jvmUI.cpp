#include "wpch.hpp"
#include "jvmUI.hpp"

#include <El/Evaluator/jvmImpl.hpp>

#include "textBank.hpp"

#include "dikCodes.h"
#include "keyInput.hpp"

class TextureSourceJVMUI : public ITextureSource
{
private:
  jclass _jvmClass;
  jmethodID _renderID;

  int _imgWidth;
  int _imgHeight;

public:
  TextureSourceJVMUI(jclass jvmClass, jmethodID renderID, int width, int height)
  {
    _jvmClass = jvmClass;
    _renderID = renderID;
    _imgWidth = width;
    _imgHeight = height;
  }
  ~TextureSourceJVMUI()
  {
    // activate JVM FPU settings
    SCOPE_FPU_JVM

    JNIEnv *env = GetJNIEnv();
    env->DeleteGlobalRef(_jvmClass);
  }

  // ITextureSource implementation

  // always create a new data
  virtual bool IgnoreLoadedLevels() const {return true;}

  virtual bool Init(PacLevelMem *mips, int maxMips)
  {
    if (maxMips < 1) return false;

    PacLevelMem &mip = mips[0];
    mip._w = _imgWidth;
    mip._h = _imgHeight;
    mip._pitch = 0;
    mip._sFormat = PacARGB8888;
    mip._dFormat = PacARGB8888;
    mip._start = -1;

    return true;
  }
  virtual int GetMipmapCount() const {return 1;}
  virtual PacFormat GetFormat() const {return PacARGB8888;}
  virtual TextureType GetTextureType() const {return TT_Diffuse;}
  virtual TexFilter GetMaxFilter() const {return TFAnizotropic16;}

  virtual bool IsAlpha() const {return true;}
  virtual bool IsAlphaNonOpaque() const {return false;}
  virtual bool IsTransparent() const {return false;}

  virtual bool IsLittleEndian() const {return IsPlatformLittleEndian();}
  virtual bool GetMipmapData(void *mem, const PacLevelMem &mip, int level, int texSize) const
  {
    if (!_jvmClass) return false;

    // activate JVM FPU settings
    SCOPE_FPU_JVM

    JNIEnv *env = GetJNIEnv();
    if (!env) return false;

    memset(mem, 0, mip._h * mip._pitch);

    jintArray array = (jintArray)env->CallStaticObjectMethod(_jvmClass, _renderID);
    if (env->ExceptionCheck())
    {
      env->ExceptionClear();
      return false;
    }
    if (array == NULL) return false;

    int srcSize = env->GetArrayLength(array);
    if (srcSize != _imgWidth * _imgHeight)
    {
      LogF("Unexpected data length: %d", srcSize);
    }

    char *ptr = (char *)mem;
    int srcOffset = 0;
    for (int y=0; y<_imgHeight; y++)
    {
      env->GetIntArrayRegion(array, srcOffset, _imgWidth, (jint *)ptr);

      ptr += mip._pitch;
      srcOffset += _imgWidth;
    }
    return true;
  }
  virtual bool RequestMipmapData(const PacLevelMem *mips, int levelMin, int levelMax, FileRequestPriority prior) const
  {
    return true; // data are always ready
  }

  virtual PackedColor GetAverageColor() const {return PackedBlack;}
  /// no dynamic range compression
  virtual PackedColor GetMaxColor() const {return PackedWhite;}

  virtual float GetMaxTexDetail(float maxTexDetail) const {return FLT_MAX;}
  virtual void ForceAlpha() {}

  virtual QFileTime GetTimestamp() const {return 0;}
  virtual void FlushHandles() {}

  // no clamping needed
  virtual int GetClamp() const {return 0;}
  virtual void SetClamp(int clampFlags) {}
};

JVMUI::JVMUI(int width, int height)
{
  _jvmClass = NULL;

  // activate JVM FPU settings
  SCOPE_FPU_JVM

  JNIEnv *env = GetJNIEnv();
  JavaLocalFrame frame(env, 10);

  // load the class
  jclass cls = env->FindClass("com/bistudio/JNIScripting/GUI");
  if (!cls) return;
  jmethodID renderID = env->GetStaticMethodID(cls, "render", "()[I");
  if (!renderID) return;
  jmethodID initID = env->GetStaticMethodID(cls, "init", "(II)V");
  if (!initID) return;
  env->CallStaticVoidMethod(cls, initID, width, height);
  if (env->ExceptionCheck())
  {
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  _addPanelID = env->GetStaticMethodID(cls, "addPanel", "(Ljavax/swing/JPanel;IIII)V");
  _removePanelID = env->GetStaticMethodID(cls, "removePanel", "()V");
  _movePanelID = env->GetStaticMethodID(cls, "movePanel", "(II)V");
  _noPanelID = env->GetStaticMethodID(cls, "noPanel", "()Z");

  _sendMouseID = env->GetStaticMethodID(cls, "sendMouseEvent", "(IIIII)V");
  _sendMouseWheelID = env->GetStaticMethodID(cls, "sendMouseWheelEvent", "(IIII)V");
  _sendKeyID = env->GetStaticMethodID(cls, "sendKeyEvent", "(IIIC)V");

  // keep global reference both here and in the texture source
  _jvmClass = (jclass)env->NewGlobalRef(cls);
  _texture = GlobLoadTexture(new TextureSourceJVMUI((jclass)env->NewGlobalRef(cls), renderID, width, height));

  // mouse cursor
  ParamEntryVal entry = Pars >> "CfgWrapperUI" >> "Cursors" >> "Arrow";
  RString GetPictureName(RString baseName);
  _cursorTexture = GlobPreloadTexture(GetPictureName(entry >> "texture"));
  _cursorHotspotX = entry >> "hotspotX";
  _cursorHotspotY = entry >> "hotspotY";
  _cursorColor = GetPackedColor(entry >> "color");
}

JVMUI::~JVMUI()
{
  // activate JVM FPU settings
  SCOPE_FPU_JVM

  JNIEnv *env = GetJNIEnv();
  if (env) env->DeleteGlobalRef(_jvmClass);
}

void JVMUI::AddPanel(RString name, int x, int y, int width, int height)
{
  if (_addPanelID == NULL) return;

  // activate JVM FPU settings
  SCOPE_FPU_JVM

  JNIEnv *env = GetJNIEnv();
  if (!env) return;
  JavaLocalFrame frame(env, 10);

  // load the class
  jclass cls = LoadJClass(env, name);
  if (!cls) return;
  jmethodID constructorID = env->GetMethodID(cls, "<init>", "()V");
  if (!constructorID) return;

  // create the instance
  jobject panel = env->NewObject(cls, constructorID);
  if (panel == NULL) return;

  env->CallStaticVoidMethod(_jvmClass, _addPanelID, panel, x, y, width, height);
  if (env->ExceptionCheck())
    env->ExceptionClear();
}

void JVMUI::RemovePanel()
{
  if (_removePanelID == NULL) return;

  // activate JVM FPU settings
  SCOPE_FPU_JVM

  JNIEnv *env = GetJNIEnv();
  if (!env) return;

  env->CallStaticVoidMethod(_jvmClass, _removePanelID);
  if (env->ExceptionCheck())
    env->ExceptionClear();
}

void JVMUI::MovePanel(int x, int y)
{
  if (_movePanelID == NULL) return;

  // activate JVM FPU settings
  SCOPE_FPU_JVM

  JNIEnv *env = GetJNIEnv();
  if (!env) return;

  env->CallStaticVoidMethod(_jvmClass, _movePanelID, x, y);
  if (env->ExceptionCheck())
    env->ExceptionClear();
}

bool JVMUI::IsVisible() const
{
  // activate JVM FPU settings
  SCOPE_FPU_JVM

  JNIEnv *env = GetJNIEnv();
  if (!env) return false;

  // check if some panel exist
  if (env->CallStaticBooleanMethod(_jvmClass, _noPanelID)) return false;
  if (env->ExceptionCheck())
  {
    env->ExceptionClear();
    return false;
  }
  
  return true;
}

void JVMUI::Draw(const Rect2DPixel &rect)
{
  if (!IsVisible()) return;

  MipInfo mip = GEngine->TextBank()->UseMipmap(_texture, 0, 0);
  if (mip.IsOK()) GEngine->Draw2D(mip, PackedWhite, rect);

  // cursor
  float mouseScrX = GInput.cursorX * 0.5f + 0.5f;
  float mouseScrY = GInput.cursorY * 0.5f + 0.5f;
  if (GEngine->IsCursorSupported())
  {
    int wScreen = GEngine->Width2D();
    int hScreen = GEngine->Height2D();
    int mx = toInt(mouseScrX * wScreen);
    int my = toInt(mouseScrY * hScreen);
    GEngine->SetCursor(_cursorTexture, mx, my, _cursorHotspotX, _cursorHotspotY, _cursorColor, 0);
  }
}

bool JVMUI::Simulate(float deltaT)
{
  if (_sendMouseID == NULL) return false;

  // activate JVM FPU settings
  SCOPE_FPU_JVM

  JNIEnv *env = GetJNIEnv();
  if (!env) return false;
  // check if some panel exist
  if (env->CallStaticBooleanMethod(_jvmClass, _noPanelID)) return false;
  if (env->ExceptionCheck())
  {
    env->ExceptionClear();
    return false;
  }

  // cursor position in screen coordinates
  int wScreen = GEngine->Width2D();
  int hScreen = GEngine->Height2D();
  int mouseX = toInt((0.5 + GInput.cursorX * 0.5) * wScreen);
  int mouseY = toInt((0.5 + GInput.cursorY * 0.5) * hScreen);

  int modifiers = 0;
  if (GInput.keys[DIK_LSHIFT] || GInput.keys[DIK_RSHIFT]) modifiers |= 1 << 0; // SHIFT_MASK
  if (GInput.keys[DIK_LCONTROL] || GInput.keys[DIK_RCONTROL]) modifiers |= 1 << 1; // CTRL_MASK
  if (GInput.keys[DIK_LMENU] || GInput.keys[DIK_RMENU]) modifiers |= 1 << 3; // ALT_MASK

  int firstButton = -1;
  bool buttonState[3];
  for (int button=2; button>=0; button--)
  {
    buttonState[button] = GInput.GetMouseButton(button, true, 2) > 0;
    if (buttonState[button]) firstButton = button;
  }

  // public void sendMouseEvent(int id, int modifiers, int x, int y, int button)
  env->CallStaticVoidMethod(_jvmClass, _sendMouseID, MouseMoved, modifiers, mouseX, mouseY, firstButton + 1);
  if (env->ExceptionCheck()) env->ExceptionClear();

  for (int button=0; button<3; button++)
  {
    // Go through the whole history
    unsigned int downToDo = GInput.mouseButtonsToDo[button];
    unsigned int upToDo = GInput.mouseButtonsReleased[button];
    unsigned int cycles;
    if (buttonState[button])
      cycles = downToDo - 1 < upToDo ? downToDo - 1 : upToDo; // downToDo need to be handled at last
    else
      cycles = downToDo < upToDo ? downToDo : upToDo;
    if (cycles < 0) cycles = 0;

    if (upToDo > cycles)
    {
      env->CallStaticVoidMethod(_jvmClass, _sendMouseID, MouseReleased, modifiers, mouseX, mouseY, button + 1);
      if (env->ExceptionCheck()) env->ExceptionClear();
      upToDo--;
    }
    for (unsigned int i=0; i<cycles; i++)
    {
      env->CallStaticVoidMethod(_jvmClass, _sendMouseID, MousePressed, modifiers, mouseX, mouseY, button + 1);
      if (env->ExceptionCheck()) env->ExceptionClear();
      downToDo--;
      env->CallStaticVoidMethod(_jvmClass, _sendMouseID, MouseReleased, modifiers, mouseX, mouseY, button + 1);
      if (env->ExceptionCheck()) env->ExceptionClear();
      upToDo--;
    }
    if (downToDo > 0)
    {
      env->CallStaticVoidMethod(_jvmClass, _sendMouseID, MousePressed, modifiers, mouseX, mouseY, button + 1);
      if (env->ExceptionCheck()) env->ExceptionClear();
      downToDo--;
    }
    Assert(downToDo == 0);
    Assert(upToDo == 0);
  }

  if (_sendMouseWheelID != NULL)
  {
    int wheelMoved = toInt(GInput.mouseWheelDown - GInput.mouseWheelUp);
    if (wheelMoved != 0)
    {
      // public void sendMouseWheelEvent(int rotation, int modifiers, int x, int y)
      env->CallVoidMethod(_jvmClass, _sendMouseWheelID, wheelMoved, modifiers, mouseX, mouseY);
      if (env->ExceptionCheck())
      {
        env->ExceptionDescribe();
        env->ExceptionClear();
      }
    }
  }

  return true;
}

bool JVMUI::DoKeyDown(int dikCode)
{
  if (_sendKeyID == NULL) return false;

  // activate JVM FPU settings
  SCOPE_FPU_JVM

  JNIEnv *env = GetJNIEnv();
  if (!env) return false;

  // check if some panel exist
  if (env->CallStaticBooleanMethod(_jvmClass, _noPanelID)) return false;
  if (env->ExceptionCheck())
  {
    env->ExceptionClear();
    return false;
  }

  int modifiers = GetModifiers();

  // map scan code to key code
  int keyCode = DIKCodeToKeyCode(dikCode);
  if (keyCode > 0)
  {
    // public void sendKeyEvent(int id, int modifiers, int keyCode, char keyChar)      
    env->CallStaticVoidMethod(_jvmClass, _sendKeyID, KeyPressed, modifiers, keyCode, 0xFFFF);
    if (env->ExceptionCheck())
    {
      env->ExceptionDescribe();
      env->ExceptionClear();
      return false;
    }
    return true;
  }
  return false;
}

bool JVMUI::DoKeyUp(int dikCode)
{
  if (_sendKeyID == NULL) return false;

  // activate JVM FPU settings
  SCOPE_FPU_JVM

  JNIEnv *env = GetJNIEnv();
  if (!env) return false;

  // check if some panel exist
  if (env->CallStaticBooleanMethod(_jvmClass, _noPanelID)) return false;
  if (env->ExceptionCheck())
  {
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }

  int modifiers = GetModifiers();

  // map scan code to key code
  int keyCode = DIKCodeToKeyCode(dikCode);
  if (keyCode > 0)
  {
    // public void sendKeyEvent(int id, int modifiers, int keyCode, char keyChar)      
    env->CallStaticVoidMethod(_jvmClass, _sendKeyID, KeyReleased, modifiers, keyCode, 0xFFFF);
    if (env->ExceptionCheck())
    {
      env->ExceptionDescribe();
      env->ExceptionClear();
      return false;
    }
    return true;
  }
  return false;
}

bool JVMUI::DoChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags)
{
  if (_sendKeyID == NULL) return false;

  // activate JVM FPU settings
  SCOPE_FPU_JVM

  JNIEnv *env = GetJNIEnv();
  if (!env) return false;

  // check if some panel exist
  if (env->CallStaticBooleanMethod(_jvmClass, _noPanelID)) return false;
  if (env->ExceptionCheck())
  {
    env->ExceptionClear();
    return false;
  }

  int modifiers = GetModifiers();

  // map scan code to key code
  if (nChar > 0)
  {
    // public void sendKeyEvent(int id, int modifiers, int keyCode, char keyChar)      
    env->CallStaticVoidMethod(_jvmClass, _sendKeyID, KeyTyped, modifiers, 0, nChar);
    if (env->ExceptionCheck())
    {
      env->ExceptionDescribe();
      env->ExceptionClear();
      return false;
    }
    return true;
  }
  return false;
}

bool JVMUI::DoIMEChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags)
{
  return false;
}

bool JVMUI::DoIMEComposition(unsigned nChar, unsigned nFlags)
{
  return false;
}

int JVMUI::GetModifiers() const
{
  int modifiers = 0;
  if (GInput.keys[DIK_LSHIFT] || GInput.keys[DIK_RSHIFT]) modifiers |= 1 << 0; // SHIFT_MASK
  if (GInput.keys[DIK_LCONTROL] || GInput.keys[DIK_RCONTROL]) modifiers |= 1 << 1; // CTRL_MASK
  if (GInput.keys[DIK_LMENU] || GInput.keys[DIK_RMENU]) modifiers |= 1 << 3; // ALT_MASK
  return modifiers;
}

int JVMUI::DIKCodeToKeyCode(int dikCode)
{
  static int table[] =
  {
    // 0x00
    0x00,     // undefined
    0x1B,     // DIK_ESCAPE
    '1',       // DIK_1               
    '2',       // DIK_2               
    '3',       // DIK_3               
    '4',       // DIK_4               
    '5',       // DIK_5               
    '6',       // DIK_6               
    '7',       // DIK_7               
    '8',       // DIK_8               
    '9',       // DIK_9               
    '0',       // DIK_0               
    0x2D,       // DIK_MINUS           
    0x3D,       // DIK_EQUALS          
    '\b',       // DIK_BACK            
    '\t',       // DIK_TAB             
    // 0x10
    'Q',       // DIK_Q               
    'W',       // DIK_W               
    'E',       // DIK_E               
    'R',       // DIK_R               
    'T',       // DIK_T               
    'Y',       // DIK_Y               
    'U',       // DIK_U               
    'I',       // DIK_I               
    'O',       // DIK_O               
    'P',       // DIK_P               
    0x5B,       // DIK_LBRACKET        
    0x5D,       // DIK_RBRACKET        
    '\n',       // DIK_RETURN          
    0x11,       // DIK_LCONTROL        
    'A',       // DIK_A               
    'S',       // DIK_S               
    // 0x20
    'D',       // DIK_D               
    'F',       // DIK_F               
    'G',       // DIK_G               
    'H',       // DIK_H               
    'J',       // DIK_J               
    'K',       // DIK_K               
    'L',       // DIK_L               
    0x3B,       // DIK_SEMICOLON       
    0xDE,       // DIK_APOSTROPHE      
    0xC0,       // DIK_GRAVE           
    0x10,       // DIK_LSHIFT          
    0x5C,       // DIK_BACKSLASH       
    'Z',       // DIK_Z               
    'X',       // DIK_X               
    'C',       // DIK_C               
    'V',       // DIK_V               
    // 0x30
    'B',       // DIK_B               
    'N',       // DIK_N               
    'M',       // DIK_M               
    0x2C,       // DIK_COMMA           
    0x2E,       // DIK_PERIOD          
    0x2F,       // DIK_SLASH           
    0x10,       // DIK_RSHIFT          
    0x6A,       // DIK_MULTIPLY        
    0x12,       // DIK_LMENU           
    0x20,       // DIK_SPACE           
    0x14,       // DIK_CAPITAL         
    0x70,       // DIK_F1              
    0x71,       // DIK_F2              
    0x72,       // DIK_F3              
    0x73,       // DIK_F4              
    0x74,       // DIK_F5              
    // 0x40
    0x75,       // DIK_F6              
    0x76,       // DIK_F7              
    0x77,       // DIK_F8              
    0x78,       // DIK_F9              
    0x79,       // DIK_F10             
    0x90,       // DIK_NUMLOCK         
    0x91,       // DIK_SCROLL          
    0x67,       // DIK_NUMPAD7         
    0x68,       // DIK_NUMPAD8         
    0x69,       // DIK_NUMPAD9         
    0x6D,       // DIK_SUBTRACT        
    0x64,       // DIK_NUMPAD4         
    0x65,       // DIK_NUMPAD5         
    0x66,       // DIK_NUMPAD6         
    0x6B,       // DIK_ADD             
    0x61,       // DIK_NUMPAD1         
    // 0x50
    0x62,       // DIK_NUMPAD2         
    0x63,       // DIK_NUMPAD3         
    0x60,       // DIK_NUMPAD0         
    0x6E,       // DIK_DECIMAL         
    0,
    0,
    0x00,       // DIK_OEM_102         
    0x7A,       // DIK_F11             
    0x7B,       // DIK_F12             
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x60
    0,
    0,
    0,
    0,
    0xF000,       // DIK_F13             
    0xF001,       // DIK_F14             
    0xF002,       // DIK_F15             
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0x70
    0x15,       // DIK_KANA
    0,
    0,
    0x00,       // DIK_ABNT_C1         
    0,
    0,
    0,
    0,
    0,
    0x1C,       // DIK_CONVERT         
    0,
    0x1D,       // DIK_NOCONVERT       
    0,
    0x00,       // DIK_YEN             
    0x00,       // DIK_ABNT_C2         
    0,
    // 0x80
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0x3D,       // DIK_NUMPADEQUALS    
    0,
    0,
    // 0x90
    0x00,       // DIK_PREVTRACK       
    0x0200,       // DIK_AT              
    0x0201,       // DIK_COLON           
    0x020B,       // DIK_UNDERLINE       
    0x19,       // DIK_KANJI           
    0xFFC8,       // DIK_STOP            
    0x00,       // DIK_AX              
    0x00,       // DIK_UNLABELED       
    0,
    0x00,       // DIK_NEXTTRACK       
    0,
    0,
    '\n',       // DIK_NUMPADENTER     
    0x11,       // DIK_RCONTROL        
    0,
    0,
    // 0xA0
    0x00,       // DIK_MUTE            
    0x00,       // DIK_CALCULATOR      
    0x00,       // DIK_PLAYPAUSE       
    0,
    0x00,       // DIK_MEDIASTOP       
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0x00,       // DIK_VOLUMEDOWN      
    0,
    // 0xB0
    0x00,       // DIK_VOLUMEUP        
    0,
    0x00,       // DIK_WEBHOME         
    0x00,       // DIK_NUMPADCOMMA     
    0,
    0x6F,       // DIK_DIVIDE     
    0,
    0x00,       // DIK_SYSRQ           
    0x12,       // DIK_RMENU           
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    // 0xC0
    0,
    0,
    0,
    0,
    0,
    0x13,       // DIK_PAUSE           
    0,
    0x24,       // DIK_HOME            
    0x26,       // DIK_UP              
    0x21,       // DIK_PRIOR           
    0,
    0x25,       // DIK_LEFT            
    0,
    0x27,       // DIK_RIGHT           
    0,
    0x23,       // DIK_END             
    // 0xD0
    0x28,       // DIK_DOWN            
    0x22,       // DIK_NEXT            
    0x9B,       // DIK_INSERT          
    0x7F,       // DIK_DELETE          
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0x020C,       // DIK_LWIN            
    0x020C,       // DIK_RWIN            
    0x00,       // DIK_APPS            
    0x00,       // DIK_POWER           
    0x00,       // DIK_SLEEP           
    // 0xE0
    0,
    0,
    0,
    0x00,       // DIK_WAKE            
    0,
    0x00,       // DIK_WEBSEARCH       
    0x00,       // DIK_WEBFAVORITES    
    0x00,       // DIK_WEBREFRESH      
    0x00,       // DIK_WEBSTOP         
    0x00,       // DIK_WEBFORWARD      
    0x00,       // DIK_WEBBACK         
    0x00,       // DIK_MYCOMPUTER      
    0x00,       // DIK_MAIL            
    0x00,       // DIK_MEDIASELECT     
  };

  if (dikCode < 0 || dikCode >= lenof(table)) return 0;
  return table[dikCode];
}