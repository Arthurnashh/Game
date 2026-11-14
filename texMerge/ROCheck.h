#ifndef __READONLYCHECK_HEADER_
#define __READONLYCHECK_HEADER_

#include <El/Interfaces/IROCheck.hpp>
#include <windows.h>

ROCheckResult __declspec(deprecated) TestFileRO(HWND hWndOwner, const _TCHAR *szFilename, int flags=0,HICON hIcon=NULL, HINSTANCE hInst=NULL);
void __declspec(deprecated) TestFileRO_ActiveSSDB(SccFunctions *vss);

class WinROCheck:public IROCheck
{
public:
  HWND hWndOwner;
  HICON hIcon;
  HINSTANCE hInstance;
  virtual ROCheckResult TestFileRO(const _TCHAR *szFilename, int flags);

  WinROCheck():hWndOwner(NULL),hIcon(NULL),hInstance(NULL) {}
};

#endif