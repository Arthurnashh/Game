
#include "common_Win.h"
#include "UIMessageThread.h"

namespace MultiThread
{

  UIMessageTarget::UIMessageTarget(bool noInit)
  {
    memset(_instanceData,0,sizeof(_instanceData));
    if (!noInit) Install();
  }

  struct UIMessageTargetStruct
  {
    HWND hWnd;
    WNDPROC subclass;
  };

  UIMessageTarget::~UIMessageTarget(void)
  {
    Uninstall();
  }

#define WM_INCOMEMSG (WM_APP+1)

  static char *UIMessageTarget_GetInstanceData(UIMessageTarget *a)
  {
    return a->_instanceData;
  }

  static LRESULT CALLBACK ThreadMessagePump(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
  {
    UIMessageTarget *instance=(UIMessageTarget *)GetWindowLong(hWnd,GWL_USERDATA);
    if (message==WM_INCOMEMSG)
    {
      instance->PumpAllMessages();
      return 0;
    }
    else
    {
      UIMessageTargetStruct *nfo=reinterpret_cast<UIMessageTargetStruct *>(UIMessageTarget_GetInstanceData(instance));
      return CallWindowProc(nfo->subclass,hWnd,message,wParam,lParam);
    }
  }

  bool UIMessageTarget::Install()
  {
    UIMessageTargetStruct *nfo=reinterpret_cast<UIMessageTargetStruct *>(_instanceData);
    if (nfo->hWnd) return true;

    HWND hWnd=CreateWindowEx(0,_T("STATIC"),_T("UIMessageTarget"),0,0,0,0,0,HWND_MESSAGE,0,GetModuleHandle(NULL),0);
    if (hWnd==0) return false;
    nfo->subclass=(WNDPROC)GetWindowLong(hWnd,GWL_WNDPROC);
    SetWindowLong(hWnd,GWL_WNDPROC, (LPARAM)&ThreadMessagePump);
    SetWindowLong(hWnd,GWL_USERDATA, (LPARAM)this);
    nfo->hWnd=hWnd;
    return true;   
  }

  bool UIMessageTarget::IsInstalled() const
  {
    const UIMessageTargetStruct *nfo=reinterpret_cast<const UIMessageTargetStruct *>(_instanceData);
    return nfo->hWnd!=0;
  }

  void UIMessageTarget::NotifyNewMsg()
  {
    UIMessageTargetStruct *nfo=reinterpret_cast<UIMessageTargetStruct *>(_instanceData);
    PostMessage(nfo->hWnd,WM_INCOMEMSG,0,0);
  }

  bool UIMessageTarget::Uninstall()
  {
    UIMessageTargetStruct *nfo=reinterpret_cast<UIMessageTargetStruct *>(_instanceData);
    if (nfo->hWnd==0) return false;
    DestroyWindow(nfo->hWnd);
    nfo->hWnd=0;
    return true;
  }

}