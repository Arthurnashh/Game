#include "common_Win.h"
#include ".\fiber.h"
#include "tls.h"

namespace MultiThread
{


typedef LPVOID (WINAPI *CreateFiberEx_type)(
                             SIZE_T dwStackCommitSize,
                             SIZE_T dwStackReserveSize,
                             DWORD dwFlags,
                             LPFIBER_START_ROUTINE lpStartAddress,
                             LPVOID lpParameter
                             );
typedef LPVOID (WINAPI *ConvertFiberToThread_type)();

static CreateFiberEx_type CreateFiberEx_fn=(CreateFiberEx_type)GetProcAddress(GetModuleHandle(_T("KERNEL32.DLL")),"CreateFiberEx");
static ConvertFiberToThread_type ConvertFiberToThread_fn=(ConvertFiberToThread_type)GetProcAddress(GetModuleHandle(_T("KERNEL32.DLL")),"ConvertFiberToThread");

static bool SFiberInfoBool;
static Tls<bool> SFiberAlreadyCreated;
static _inline void SetFiberData( LPVOID data )    {  *(PVOID *)::GetCurrentFiber() =data;}

MasterFiber::MasterFiber()
{
  if (SFiberAlreadyCreated!=&SFiberInfoBool)
  {
    ConvertThreadToFiber(0);
    SFiberAlreadyCreated=&SFiberInfoBool;
  }  
  _previous=GetCurrentFiber();
  SetFiberData(this);
  _fiberHandle=::GetCurrentFiber();
}

MasterFiber::~MasterFiber()
{
  SetFiberData(_previous);
  if (_previous==0 && ConvertFiberToThread_fn)
  {
    ConvertFiberToThread_fn();
    SFiberAlreadyCreated=0;
  }
  _fiberHandle=0;
}

Fiber::Fiber(size_t stackSize):_fiberHandle(0),_backPtr(0),_exitState(false),_maxStack(stackSize),_creator(0)
{
}

Fiber::~Fiber(void)
{
  if (IsActive() && !IsMySelf())
  {
    Stop();
  }
}

class Fiber_StartupClass
{
public:
  static VOID WINAPI FiberProc(PVOID lpParameter);
};

VOID CALLBACK Fiber_StartupClass::FiberProc(PVOID lpParameter)
{
  Fiber *self=reinterpret_cast<Fiber *>(lpParameter);
  self->_exitCode=self->Run();
  Fiber *creator=self->_creator;
  while (true)    //infinite loop - creator will stop the fibber
  {
    self->_exitState=true;
    creator->Resume();
  }
}


bool Fiber::Start(StartMode mode)
{
  if (IsActive()) return false;
  _creator=GetCurrentFiber();
  unsigned long stack=_maxStack?_maxStack:1024*1024;
  if (CreateFiberEx_fn)
  {
    _fiberHandle=CreateFiberEx_fn(1,stack,0,Fiber_StartupClass::FiberProc,this);
  }
  else
  {  
    _fiberHandle=CreateFiber(0,Fiber_StartupClass::FiberProc,this);
  }
  if (_fiberHandle)
  {
    if (mode==StartSuspended) return true;
    return Resume();
  }
  return _fiberHandle!=0;
}

bool Fiber::Resume()
{
  if (!IsActive()) return false;
  Fiber *current=GetCurrentFiber();
  if (current==this) return false;
  if (_backPtr==0) _backPtr=current;
  SwitchToFiber(_fiberHandle);
  if (_exitState) Stop();   
  return true;
}

Fiber *Fiber::GetCurrentFiber()
{
  return reinterpret_cast<Fiber *>(GetFiberData());
}

bool Fiber::Stop()
{
  if (!IsActive()) return false;
  if (IsMySelf())
  {
    _exitState=true;
    while (_creator) _creator->Resume();
  }
  else
  {
    Done();
    DeleteFiber(_fiberHandle);
   _fiberHandle=0;
  }
  return true;
}

bool Fiber::Yield()
{
  if (!IsActive()) return false;
  if (!IsMySelf()) return false;
  if (_backPtr) _backPtr->Resume();
  else if (_creator) _creator->Resume();
  else return false;
  _backPtr=0;
  return true;
}



};