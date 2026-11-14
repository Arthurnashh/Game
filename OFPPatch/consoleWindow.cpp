#include "stdafx.h"
#include "consoleWindow.hpp"
#include "resource.h"
#include "consoleBind.h"
#include <stdio.h>
#include <Es/Containers/array.hpp>
#include <Es/Strings/rString.hpp>

#if _VBS1
const char *AppName = "VBS1";
#else
const char *AppName = "Bohemia Interactive";
#endif
const char *PatchVersionTo = "9.99"; //not real version. It should be renamed

RString WCSTR2RString(LPCWSTR str, int codePage=CP_ACP)
{
  int size = WideCharToMultiByte(codePage, 0, str, -1, NULL, 0, NULL, NULL);
  RString buf;
  buf.CreateBuffer(size);
  WideCharToMultiByte(codePage, 0, str, -1, buf.MutableData(), size, NULL, NULL);
  return buf;
}

void GetStringFromStringTable(HINSTANCE hInstance, UINT uStringID, LPSTR lpBuffer, int nBufferMax, WORD wLanguage)
{
  wchar_t  *pwchMem, *pwchCur;
  UINT      idRsrcBlk = uStringID / 16 + 1;
  int       strIndex  = uStringID % 16;
  HINSTANCE hModule = NULL;
  HRSRC     hResource = NULL;

  hResource = FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE(idRsrcBlk), wLanguage);
  if( hResource != NULL )
  {
    pwchMem = (wchar_t *)LoadResource( hModule, hResource );

    if( pwchMem != NULL )
    {
      pwchCur = pwchMem;
      for(int i = 0; i<16; i++ )
      {
        if( *pwchCur )
        {
          int cchString = *pwchCur;  // String size in characters.
          pwchCur++;
          if( i == strIndex )
          {
            // The string has been found in the string table.
            wchar_t *pwchTemp = new wchar_t[ cchString+1 ];
            wcsncpy( pwchTemp, pwchCur, cchString );
            pwchTemp[ cchString ] = '\0';

            RString out = WCSTR2RString(pwchTemp);
            strncpy(lpBuffer, out, nBufferMax);
            lpBuffer[nBufferMax - 1] = 0;
            return;
          }
          pwchCur += cchString;
        }
        else
          pwchCur++;
      }
    }
  } else LoadString(hInstance, uStringID, lpBuffer, nBufferMax);
}

void LoadAppString(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int nBufferMax)
{
  const int MaskCount = 2;
  enum {IX_APPNAME, IX_VERSION};
  const char *mask[MaskCount] = { "#APPNAME", "#VERSION" };

  RString rsBuffer; rsBuffer.CreateBuffer(nBufferMax);
  char *buffer = rsBuffer.MutableData(); //temp buffer
  GetStringFromStringTable(hInstance, uID, buffer, nBufferMax, (WORD)GetUserDefaultLCID());
  for (int i=0; i<MaskCount; i++)
  {
    char *ptr = buffer;
    char *found = NULL;
    while (found = strstr(ptr, mask[i]))
    {
      int n = found - ptr;
      if (n >= nBufferMax) break;

      // copy up to mask
      strncpy(lpBuffer, ptr, n);
      lpBuffer += n;
      nBufferMax -= n;
      ptr = found + strlen(mask[i]);

      // AppName
      const char *varValue = NULL;
      switch (i)
      {
      case IX_VERSION:
        varValue = PatchVersionTo;
        break;
      default:
        varValue = AppName;
        break;
      }
      n = strlen(varValue);
      if (n > nBufferMax) n = nBufferMax;
      strncpy(lpBuffer, varValue, n);
      lpBuffer += n;
      nBufferMax -= n;
      if (nBufferMax == 0) break;
    }
    strncpy(lpBuffer, ptr, nBufferMax);
    lpBuffer[nBufferMax - 1] = 0;
    strcpy(buffer, lpBuffer);
  }
}

int CALLBACK ConsoleWindow::DlgProcCB(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message==WM_INITDIALOG)
	{
		SetWindowLong(hDlg,GWL_USERDATA,lParam);
		ConsoleWindow *cw = (ConsoleWindow *)lParam;
		return cw->DlgProc(hDlg,message,wParam,lParam);
	}
	else
	{
		ConsoleWindow *cw = (ConsoleWindow *)GetWindowLong(hDlg,GWL_USERDATA);
		if (!cw) return FALSE;
		return cw->DlgProc(hDlg,message,wParam,lParam);
	}
}

int ConsoleWindow::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			//InitWindow(hInstApp,hDlg);
			//if( !AppWinCreate(hDlg) ) return FALSE; // some fatal error

			consoleEditHWnd = GetDlgItem(hDlg,IDC_CONSOLE_EDIT);
			progressTextHWnd= GetDlgItem(hDlg,IDC_CONSOLE_PROGRESS);
      progressBarHWnd = GetDlgItem(hDlg, IDC_CONSOLE_PROGRESS_BAR);			
      percentageHWnd = GetDlgItem(hDlg, IDC_CONSOLE_PERCENTAGE);			
      okButton = GetDlgItem(hDlg,IDOK);

			SetWindowLong(hDlg,GWL_USERDATA,lParam);

			return TRUE;
		}

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
        Enter();
				_hwndDlg = NULL;
        Leave();
				//AppWinDestroy(hDlg);
				//ValidateQuit=true;
				return TRUE;
			}
			break;

		case WM_SIZE:
			/*
			if (consoleEditHWnd)
			{
				RECT rect;
				GetClientRect(hDlg, &rect);
				MoveWindow
				(
					consoleEditHWnd, rect.left, rect.top,
					rect.right - rect.left, rect.bottom - rect.top, TRUE
				);
			}
			*/
			break;
	}
	return FALSE;
}

ConsoleWindow::ConsoleWindow()
{
	_hInstance = NULL;
	_hwndDlg = NULL;
  doWaitForClosing = false;
}

void ConsoleWindow::Create(HINSTANCE hInstance, bool show)
{
	_hInstance = hInstance; 
	_hwndDlg = CreateDialogParam
	(
		hInstance, MAKEINTRESOURCE(IDD_CONSOLE), NULL, ConsoleWindow::DlgProcCB, (LPARAM)this
	);
	SetProgress("");
  ShowWindow(show);
	UpdateWindow(GetHWnd());
}
ConsoleWindow::~ConsoleWindow()
{
}

void ConsoleWindow::Close()
{
	PostMessage(GetHWnd(),WM_CLOSE,0,0);
}

void ConsoleWindow::Destroy()
{
	DestroyWindow(GetHWnd());
  Enter();
	_hwndDlg = NULL;
  Leave();
}

void ConsoleWindow::SetTitle(UINT format, ...)
{
	char buf[512];
	char formatS[512];
		
	LoadAppString(_hInstance, format, formatS, sizeof(formatS));

	va_list arglist;
	va_start( arglist, format );
	_vsnprintf(buf,sizeof(buf),formatS,arglist);
	va_end( arglist );

  SetWindowText(GetHWnd(),buf);
}

void ConsoleWindow::SetProgress(UINT format, ...)
{
	char buf[512];
	char formatS[512];
		
	LoadAppString(_hInstance, format, formatS, sizeof(formatS));

	va_list arglist;
	va_start( arglist, format );
	_vsnprintf(buf,sizeof(buf),formatS,arglist);
	va_end( arglist );

	SetWindowText(progressTextHWnd,buf);
}

void ConsoleWindow::SetProgress(const char *format, ...)
{
	char buf[512];
		
	va_list arglist;
	va_start( arglist, format );
	_vsnprintf(buf,sizeof(buf),format,arglist);
	va_end( arglist );

	SetWindowText(progressTextHWnd,buf);
}

#include <CommCtrl.h>
void ConsoleWindow::InitProgressBar(__int64 total)
{ 
  progressBarInfo.totalVal = total; progressBarInfo.currentVal = 0i64; 
  if (progressBarHWnd)
  {
    SendMessage(progressBarHWnd, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); //we will use percentages
  }
}

void ConsoleWindow::SetProgressBar(int processed)
{ 
  const int MinPercetageVal = 3;
  progressBarInfo.currentVal += processed;
  if (progressBarInfo.totalVal)
  {
    int percentage = (int)(100*(((double)progressBarInfo.currentVal) / ((double)progressBarInfo.totalVal)));
    if (percentage>100) percentage = 100;
    if (percentage<MinPercetageVal) percentage = MinPercetageVal;
    if (progressBarHWnd)
    {
      SendMessage(progressBarHWnd, PBM_SETPOS, percentage, 0); //we will use percentages
    }
    if (percentageHWnd)
    {
      RString percentageStr = Format("%d %%", percentage);
      //SetWindowText(percentageHWnd, percentageStr);
      SendMessage(percentageHWnd, WM_SETTEXT, 0, (LPARAM)cc_cast(percentageStr));
    }
  }
  else
  {
    int percentage = MinPercetageVal; //4% shown as minimal value
    if (progressBarHWnd)
    {
      SendMessage(progressBarHWnd, PBM_SETPOS, percentage, 0); //we will use percentages
    }
    if (percentageHWnd)
    {
      RString percentageStr = Format("%d %%", percentage);
      //SetWindowText(percentageHWnd, percentageStr);
      SendMessage(percentageHWnd, WM_SETTEXT, 0, (LPARAM)cc_cast(percentageStr));
    }
  }
}

void ConsoleWindow::EnableOK(bool enable)
{
	EnableWindow(okButton,enable);
	//if (enable) SetWindowText(okButton,"OK");
	//else SetWindowText(okButton,"Cancel");
}

void ConsoleWindow::DebugF(const char *format, ...)
{
  char buf[4096];

  va_list arglist;
  va_start( arglist, format );
  _vsnprintf(buf,sizeof(buf),format,arglist);
  va_end( arglist );

  strcat(buf,"\r\n");

  // output to dialog window
  // TODO: replace any \n with \r\n
  /*
  char buf2[4096];

  const char *eol
  for(;;)
  {
  const char *eol = strchr(buf,'\n');
  if (eol
  */

  Temp<char> temp;
  int count = GetWindowTextLength(consoleEditHWnd);

  temp.Realloc(count+strlen(buf)+1);
  temp[0] = 0;
  GetWindowText(consoleEditHWnd,temp,count+1);

  size_t limit = 30*1024;
  while (strlen(temp)>limit)
  {
    const char *firstLine = strchr(temp,'\n');
    if (!firstLine) {*temp=0;break;}
    memmove(temp,firstLine+1,strlen(firstLine));
  }
  strcat(temp,buf);
  SetWindowText(consoleEditHWnd,temp);
  PostMessage(consoleEditHWnd,EM_SETSEL,strlen(temp),strlen(temp));
  PostMessage(consoleEditHWnd,EM_SCROLLCARET,0,0);
}

void ConsoleWindow::PrintF(UINT format, ...)
{
	char formatS[1024];
		
	LoadAppString(_hInstance, format, formatS, sizeof(formatS));
	char buf[4096];
		
	va_list arglist;
	va_start( arglist, format );
	_vsnprintf(buf,sizeof(buf),formatS,arglist);
	va_end( arglist );

	strcat(buf,"\r\n");

	// output to dialog window
	// TODO: replace any \n with \r\n
	/*
	char buf2[4096];
	
	const char *eol
	for(;;)
	{
		const char *eol = strchr(buf,'\n');
		if (eol
	*/

	Temp<char> temp;
	int count = GetWindowTextLength(consoleEditHWnd);

	temp.Realloc(count+strlen(buf)+1);
	temp[0] = 0;
	GetWindowText(consoleEditHWnd,temp,count+1);

	size_t limit = 30*1024;
	while (strlen(temp)>limit)
	{
		const char *firstLine = strchr(temp,'\n');
		if (!firstLine) {*temp=0;break;}
		memmove(temp,firstLine+1,strlen(firstLine));
	}
	strcat(temp,buf);
	SetWindowText(consoleEditHWnd,temp);
	PostMessage(consoleEditHWnd,EM_SETSEL,strlen(temp),strlen(temp));
	PostMessage(consoleEditHWnd,EM_SCROLLCARET,0,0);
}

void ConsoleWindow::ProcessMessages()
{
	if (!GetHWnd()) return;
	MSG msg;
	while( PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void ConsoleWindow::WaitForClosing(bool enableOK)
{
	if (enableOK) EnableOK();
	while (GetHWnd())
	{
		MSG msg;
		if( GetMessageA(&msg, 0, 0, 0) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void ConsoleWindow::SetWaitForClosing()
{
  Enter();
  doWaitForClosing = true;
  Leave();
}

bool ConsoleWindow::ShouldWaitForClosing()
{
  return doWaitForClosing;
}
