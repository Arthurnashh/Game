#ifndef _TREE_ENGINE_DX_COMMON_H_
#define _TREE_ENGINE_DX_COMMON_H_
#include <Es/Common/win.h>
#include <Es/essencepch.hpp>
#include <d3dx8.h>
#include <Es/Types/Pointers.hpp>

#pragma once



class PDirect3DDevice;
class CDirect3DDevice: public RefCount
{  
  friend PDirect3DDevice;
  D3DPRESENT_PARAMETERS  _pparams;  
  IDirect3DDevice8 *_device;
  DWORD _creatorThread;
public:
  CDirect3DDevice(IDirect3DDevice8 *dev) {_device=dev;if (_device) _device->AddRef();}
  ~CDirect3DDevice() {_device->Release();}
};

class PDirect3DDevice: public Ref<CDirect3DDevice>
{
public:  
  void SetPP(D3DPRESENT_PARAMETERS &pp)
  {
    GetRef()->_pparams=pp;
  }
  const D3DPRESENT_PARAMETERS &GetPP() const
  {
    return GetRef()->_pparams;
  }

  IDirect3DDevice8 *operator->() {return GetRef()->_device;}
  operator IDirect3DDevice8 *(){return GetRef()->_device;}
  PDirect3DDevice(IDirect3DDevice8 *dev):Ref<CDirect3DDevice>(dev?new CDirect3DDevice(dev):0) {}
  PDirect3DDevice() {}
  HRESULT CreateDevice(IDirect3D8 *dx, UINT Adapter,D3DDEVTYPE DeviceType,HWND  hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS *pPresentationParameters)
  {
    IDirect3DDevice8 *ppReturnedDeviceInterface;
    HRESULT res=dx->CreateDevice(Adapter,DeviceType,hFocusWindow,BehaviorFlags,pPresentationParameters,&ppReturnedDeviceInterface);
    if (res==0)
    {
      (*this)=ppReturnedDeviceInterface;
      GetRef()->_creatorThread=GetCurrentThreadId();
      GetRef()->_pparams=*pPresentationParameters;
      ppReturnedDeviceInterface->Release();
    }
    return res;
  }

  bool Reset(bool force=false)
  {
    if (GetCurrentThreadId()!=GetRef()->_creatorThread) return false;
    IDirect3DDevice8 *dev=(*this);
    HRESULT res=0;
    while (force || FAILED(res=dev->TestCooperativeLevel()))
    {
      if (res!=D3DERR_DEVICELOST)
      {
        try
        {
        if (FAILED(GetRef()->_device->Reset(&(GetRef()->_pparams)))) return false;
        return true;
        }
        catch(...)
        {
          return false;
        }
      }
      Sleep(500);      
    }
    return true;
  }

  bool IsLost()
  {
    if (GetCurrentThreadId()!=GetRef()->_creatorThread) return false;
    IDirect3DDevice8 *dev=(*this);
    return FAILED(dev->TestCooperativeLevel());
  }


};

#define D3D_GUARD(x,what) do {HRESULT hr=x;\
  if (hr) {::LogF("%s(%d): error 0x%08X - Direct3D reports an error code %d\n\t%s",__FILE__,__LINE__,hr,hr,#x);what;}\
}while (false)

#endif