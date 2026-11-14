/////////////////////////////////////////////////////////////////////////////
// Modifikace hlavního okna


#include "stdafx.h"
#include <process.h>
#include "shlobj.h"
#include "dsetup.h"
#include "io.h"
#include "direct.h"
#include "errno.h"
#include "SetupObject.h"
#include "Setup.h"
#include <El/stringtable/stringtable.hpp>
#include <El/WindowsFirewall/firewall.hpp>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Vykreslení pozadí okna


void CSetupObject::DrawRainbow(HDC hDC,RECT* pRect,COLORREF cFrom,COLORREF cTo)
{
	CDC dc; dc.Attach(hDC);

	int R1,G1,B1;
	R1 = GetRValue(cFrom);
	G1 = GetGValue(cFrom);
	B1 = GetBValue(cFrom);
	float RDiff = (float)(GetRValue(cTo) - R1);
	float GDiff = (float)(GetGValue(cTo) - G1);
	float BDiff = (float)(GetBValue(cTo) - B1);

	int		 rHg = pRect->bottom-pRect->top;
	float    Height = (float)(rHg) / (float)(256.);
	if (Height<2) Height = 2;
	float	 ActualPos = 0;
	CRect	 Rect = (*pRect);
	while( Rect.top < pRect->bottom )
	{
		ActualPos += Height;
		Rect.bottom = (int)(pRect->top + ActualPos);
		if (Rect.bottom+2>pRect->bottom)
		{
			Rect.bottom = pRect->bottom;
			ActualPos += Height;
		}
		float Diff = (rHg>Height)?(ActualPos-Height)/(((float)rHg)-Height):(1);
		if (Diff>1) Diff=1;
		if (dc.RectVisible(Rect))
		{
			CBrush br(RGB((int)(R1+Diff*RDiff),(int)(G1+Diff*GDiff),(int)(B1+Diff*BDiff)));		
			CBrush *pOld = dc.SelectObject(&br);
			dc.PatBlt(Rect.left,Rect.top,Rect.Width()+1,Rect.Height()+1,PATCOPY);
			dc.SelectObject(pOld);
		};
		// next rect
		Rect.top = Rect.bottom;
	}
	dc.Detach();
}

void CSetupObject::DoDrawWindowBackground(HDC hDC,CRect rArea)
{
	DrawRainbow(hDC,rArea,RGB(0,0,255),RGB(0,0,36));
};

// pomocná hlášení a operace
BOOL CSetupObject::VerifyFileRead(LPCTSTR pFileName,BOOL bReportErrors)
{
	if (_taccess(pFileName,00)!=0)
	{
		if (bReportErrors)
			ReportFileNotFound(pFileName,TRUE);
		return FALSE;
	}
	// soubor je OK
	return TRUE;
};

void CSetupObject::ReportFileNotFound(LPCTSTR pFileName,BOOL bSetInternalErr)
{
	CString sText;
	SetupFormatString1(sText,SetupLocalizeString (_T("SETUP_STR_ERR_FILE_NOT_FOUND")),pFileName);
	SetupMessageBox(sText,MB_OK|MB_ICONSTOP);


	if (bSetInternalErr)
		m_nSetupResult = stpInternalErr;
};


void CSetupObject::ReportFileErrorRead(LPCTSTR pFileName,BOOL bSetInternalErr)
{
	CString sText;
	SetupFormatString1(sText,SetupLocalizeString (_T("SETUP_STR_ERR_FILE_READ")),pFileName);
	SetupMessageBox(sText,MB_OK|MB_ICONSTOP);

	if (bSetInternalErr)
		m_nSetupResult = stpInternalErr;
};

/// Update 8-bit CRC value using polynomial X^8 + X^5 + X^4 + 1
static void UpdateCRC8(unsigned char &crc, unsigned char src)
{
  static const int polyVal = 0x8C;

  for (int i=0; i<8; i++)
  {
    if ((crc ^ src) & 1) crc = (crc >> 1 ) ^ polyVal;
    else crc >>= 1;
    src >>= 1;
  }
}

BOOL CSetupObject::ReadRegistryKey(CString path, unsigned char (&cdKey)[KEY_BYTES])
{
  HKEY root = HKEY_LOCAL_MACHINE;
  HKEY key;

  if (::RegOpenKeyEx(root, path, 0, KEY_READ, &key ) != ERROR_SUCCESS) 
  {
    RptF("Failed to get key \"%s\" from registry.", path);
    return false;
  }

  DWORD size = KEY_BYTES;
  DWORD type = REG_BINARY;

  HRESULT result = ::RegQueryValueEx(key, _T("KEY"), 0, &type, cdKey, &size);
  ::RegCloseKey(key);
  return result == ERROR_SUCCESS && size == KEY_BYTES;
}

BOOL CSetupObject::CheckCustomCDKey(int productID, unsigned char (&cdkey)[KEY_BYTES], unsigned char *publicKey, int checkOffset, const char *checkWith)
{
  if (productID >= 0)
  {
    unsigned char crc = 0;
    for (int i=0; i<KEY_BYTES-1; i++) UpdateCRC8(crc, cdkey[i]);
    return (crc ^ productID) == cdkey[KEY_BYTES - 1];
  }
  else
  {
    CDKey key;
    key.Init(cdkey, publicKey);
    return key.Check(checkOffset, checkWith);	
  }
}

BOOL CSetupObject::CheckCDKEY(int productID, CString &cdkey, unsigned char *publicKey, int checkOffset, const char *checkWith)
{
	CDKey key;
  RString rcdkey = CString2RString(cdkey);
	if (!key.DecodeMsg(m_cdkey, cc_cast(rcdkey))) return FALSE;
  if (productID >= 0)
  {
    unsigned char crc = 0;
    for (int i=0; i<KEY_BYTES-1; i++) UpdateCRC8(crc, m_cdkey[i]);
    return (crc ^ productID) == m_cdkey[KEY_BYTES - 1];
  }
  else
  {
	  key.Init(m_cdkey, publicKey);
	  return key.Check(checkOffset, checkWith);	
  }
}

BOOL CSetupObject::CheckCDKEYfromRegistry(int productID, unsigned char (&cdkey)[KEY_BYTES], unsigned char *publicKey, int checkOffset, const char *checkWith)
{
	CopyMemory(&m_cdkey[0], &cdkey[0], KEY_BYTES);
  if (productID >= 0)
  {
    unsigned char crc = 0;
    for (int i=0; i<KEY_BYTES-1; i++) UpdateCRC8(crc, m_cdkey[i]);
    return (crc ^ productID) == m_cdkey[KEY_BYTES - 1];
  }
  else
  {
    CDKey key;
    key.Init(m_cdkey, publicKey);
    return key.Check(checkOffset, checkWith);	
  }
}

LPCTSTR CSetupObject::MakeLongDirStr(CString& dest,LPCTSTR source)
{
	if (source!=(LPCTSTR)(dest.GetBuffer(0)))
		dest = source;
	int len;
	if ((len = dest.GetLength())!=0)
	{
		if ((dest[len-1]!=_T('\\'))&&(dest[len-1]!=_T('/')))
			dest += _T("\\");
	}
	return ((LPCTSTR)dest);
}

CString CSetupObject::ClearCompressionExt(CString filename)
{
  CString dest = filename;
  TCHAR *buff = dest.GetBuffer();
  int i=dest.GetLength()-1;
  for (; (i>=0 && buff[i]!=_T('.')); i--) ;
  if (i>=0 && !_tcsicmp(buff+i, _T(".xz"))) dest.SetAt(i,0);
  else if (i>=0 && !_tcsicmp(buff+i, _T(".gz"))) dest.SetAt(i,0);
  dest.ReleaseBuffer();
  return dest;
}

LPCTSTR CSetupObject::MakeShortDirStr(CString& dest,LPCTSTR source)
{
	if (source!=(dest.GetBuffer(0)))
		dest = source;
	int len;
	if ((len = dest.GetLength())>3) // prevent root dir.
	{
		if ((dest[len-1]==_T('\\'))||(dest[len-1]==_T('/')))
		{
			(dest.GetBuffer(0))[len-1] = 0;
			dest.ReleaseBuffer();
		}
	}
	return ((LPCTSTR)dest);
}

BOOL CSetupObject::MakePath(LPCTSTR logFile, LPCTSTR pPath, BOOL bForFile)
{
	CString sPosDir;
	MakeShortDirStr(sPosDir,m_sSetupPoseidonDir);

    TCHAR abspath[_MAX_PATH];
    _tcscpy(abspath,pPath);
	  _tfullpath(abspath,pPath,_MAX_PATH);
	// oøíznutí jména souboru
	if (bForFile)
	{
		int nLen = lstrlen(abspath);
		for(int i=(nLen-1);i>0; i--)
		{
			if ((abspath[i]==_T('\\'))||(abspath[i]==_T('/')))
			{
				abspath[i] = 0;
				break;
			}
		};
	};
    TCHAR  temp[_MAX_PATH];
    memset(temp,0,sizeof(temp));
    // vytvoøení cesty
	BOOL bDisk = FALSE;
	unsigned int nPos = 0;
	while(nPos<_tcslen(abspath))
	{
		if ((abspath[nPos]==_T('\\'))||(abspath[nPos]==_T('/')))
		{
			if (!bDisk)
			{ // 1. '\\'
				temp[nPos] = abspath[nPos];
				nPos++;
				// sí -> 2x
				if ( (nPos<_tcslen(abspath)) &&
					((abspath[nPos]==_T('\\'))||
					 (abspath[nPos]==_T('/'))))
				{
					temp[nPos] = abspath[nPos];
					nPos++;
				}
				bDisk = TRUE;
			} else
			{ // vytvorim cestu a pokracuji
				if (_tchdir(temp)!=0)
				{
					if((_tmkdir(temp)==-1)&&(errno==ENOENT)) return FALSE;
					// adresar - uninstall
					AddSetupLogLine(logFile, 'D', temp);
				} else
				{	// existuje, ale dam ho do smazani
					//if (sPosDir.GetLength()<=lstrlen(temp))
					{
						//CString sPom = temp;
						//if (lstrcmp(sPom.Left(sPosDir.GetLength()),sPosDir)==0)
							AddSetupLogLine(logFile, 'D', temp);
					}
				};
				temp[nPos] = abspath[nPos];
				nPos++;
			}
		} else
		{ // kopíruji znak
			temp[nPos] = abspath[nPos];
			nPos++;
			if (nPos>=_tcslen(abspath))
			{ // poslední adresáø neukoncen '\\'
				if (_tchdir(temp)!=0)
				{
					if((_tmkdir(temp)==-1)&&(errno==ENOENT)) return FALSE;
					// adresáø - uninstall
					AddSetupLogLine(logFile, 'D', temp);
				} else
				{	// existuje, ale dám ho do smazání
					//if (sPosDir.GetLength()<=lstrlen(temp))
					{
						//CString sPom = temp;
						//if (lstrcmp(sPom.Left(sPosDir.GetLength()),sPosDir)==0)
							AddSetupLogLine(logFile, 'D', temp);
					}
				};
			}
		}
	};
	return TRUE;
}

void CSetupObject::SetFilenameToControl(HWND hWnd,LPCTSTR pText)
{
	CString sText = " ";
	sText += pText;
	//::SetWindowTextW(hWnd,sText);
  SetupSetWindowText(hWnd,sText);
};

void CSetupObject::AddInstallationSubdirectory(CString& sDir)
{
	MakeLongDirStr(sDir,sDir);
	sDir += m_sInstallationSubdirectory;
};

bool FileExists(const char *path)
{
  FILE *test = fopen(path,"rb");
  if (test)
  {
    fclose(test);
    return true;
  }
  return false;
}

bool FileExists(CString &path)
{
  FILE *test = _tfopen(path,_T("rb"));
  if (test)
  {
    fclose(test);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////////////////////////////////
// Direct X

typedef int (CALLBACK* DXSETUP)(HWND, char *, DWORD);

void CSetupObject::DoSetupDirectX()
{
	CString sRoot;
	MakeLongDirStr(sRoot,m_sSetupSourceDir);
	sRoot += DIRECTX_SUBDIRECTORY; 
	
  //try to load it from setup.exe resources (i.e. it is now in the temporary directory)
  PrepareResourceDirectX();

  CString tempDir = GetTempDirectory(m_sApplicationName);
  CString commandLine = tempDir + _T("\\") + DIRECTX_SUBDIRECTORY + _T("\\DXSETUP.exe");
  if (!FileExists(commandLine))
  { 
    commandLine =sRoot + _T("\\DXSETUP.exe");
  }

  commandLine += _T(" /silent");
	
  STARTUPINFO startupInfo;
  startupInfo.cb=sizeof(startupInfo); 
  startupInfo.lpReserved=0;
  startupInfo.lpDesktop=NULL;
  startupInfo.lpTitle=NULL;
  startupInfo.dwFlags=0;
  startupInfo.cbReserved2=0; 
  startupInfo.lpReserved2=NULL; 

  PROCESS_INFORMATION pi;
  BOOL ok = CreateProcess(
    NULL, commandLine.GetBuffer(),
    NULL,NULL, // security
    TRUE, // inheritance
    0, // creation flags 
    NULL, // env
    NULL, // pointer to current directory name 
    &startupInfo, &pi);
  if (!ok)
	{
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
		m_nSetupResult	= stpDirectXError;
	}
  commandLine.ReleaseBuffer();

/* Call the function explicitly
	int nRes = DirectXSetup(AfxGetMainWnd()->m_hWnd,sRoot.GetBuffer(0),DSETUP_DIRECTX);
*/
};


void CSetupObject::DoSetupOpenAL()
{
  if (!temporaryFolderActive) PrepareResourceData();
  CString tempDir = GetTempDirectory(m_sApplicationName);
  CString exeName = tempDir + _T("\\OpenALwEAX.exe");
  if (!FileExists(exeName))
  { //try to open it from setup.exe resources (currently in the temporary directory)
    CString dir;
    MakeLongDirStr(dir, m_sSetupSourceDir);
    exeName = dir + _T("OpenALwEAX.exe");
  }
  RString exeNameR = CString2RString(exeName);
/*
   OpenAL Installer Options on Windows
   /F or /f or -F or -f
     Bypasses any verification and version checking and forces the installer to install both the OpenAL32.dll and wrap_oal.dll components.
   /NU or /nu or -NU or -nu
     Prevents an uninstaller from being installed.
   /R or /r or -R or -r
     Use error codes to report OpenAL installation issues
     0 for no issues (default)
     1 for "reboot required"
     2 for "write access denied"
   /S or /s or -S or -s
     The installation runs in silent mode. Nothing will be displayed.
   /U or /u or -U or -u
     Uninstall OpenAL.   
*/
  int retVal = spawnl( _P_WAIT, exeNameR, exeNameR, "/s /r", NULL );
#define USE_OPENAL_RETVAL 1
#if USE_OPENAL_RETVAL
  if (retVal)
  { //not successful
    CString errText = SetupLocalizeString(_T("SETUP_STR_OPEN_AL_ERROR"));
    if (retVal>0) 
    {
      switch (retVal)
      {
      case 1:
        errText = SetupLocalizeString(_T("SETUP_STR_OPEN_AL_REBOOT"));
        break;
      case 2:
        errText = SetupLocalizeString(_T("SETUP_STR_OPEN_AL_ACCESS_DENIED"));
        break;
      default:
        errText.Format(_T("Unable to install OpenAL.\nReturned value was: %d"),retVal);
      }
      // there should be test, whether reinstall is necessary and setting m_nSetupResult	= stpOKRestart;
    }
    else
    {
      switch (errno)
      {
      case ENOENT: errText = SetupLocalizeString(_T("SETUP_STR_OPEN_AL_ERROR_PATH"));
        break;
      }
      //m_nSetupResult = stpSetupError;
    }
    SetupMessageBox(errText, MB_OK | MB_ICONEXCLAMATION);
  }
#endif
};

// shortcuts
BOOL  DoCreateLink(LPCTSTR pLinkFile,
									 LPCTSTR pFileObj, LPCTSTR pDescription, LPCTSTR pArguments, LPCTSTR pIcon, int pIcoIndex)
{
    HRESULT hres; 
    IShellLink* psl; 
 
    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, 
        CLSCTX_INPROC_SERVER, IID_IShellLink,(void**) &psl); 
    if (SUCCEEDED(hres)) { 
        IPersistFile* ppf; 
 
        // Set the path to the shortcut target, and add the 
        // description. 
        //psl->lpVtbl->SetPath(psl, pFileObj); 
				psl->SetPath(pFileObj); 
        //psl->lpVtbl->SetDescription(psl, pDescription); 
				psl->SetDescription(pDescription); 
        psl->SetArguments(pArguments);
        
        // set icon if pIcon is not empty
        if (_tcslen(pIcon) > 0)
          psl->SetIconLocation(pIcon, (pIcoIndex < 0 ? 0 : pIcoIndex ) );

				// current directory
				{
					TCHAR buff[_MAX_PATH]; _tcscpy(buff,pFileObj);
					int nLen = _tcslen(buff);
					for(int i=(nLen-1);i>0; i--)
					{
						if ((buff[i]==_T('\\'))||(buff[i]==_T('/')))
						{
							buff[i] = 0;
							break;
						}
					};
					psl->SetWorkingDirectory(buff); 
				}
 
       // Query IShellLink for the IPersistFile interface for saving the 
       // shortcut in persistent storage. 
    
				//hres = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, 
        //    &ppf); 
				hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 
 
        if (SUCCEEDED(hres)) { 
            wchar_t wsz[MAX_PATH]; 
 
#ifndef UNICODE
            // Ensure that the string is ANSI. 
            MultiByteToWideChar(CP_ACP, 0, pLinkFile, -1, 
                wsz, MAX_PATH); 
#else
            _tcscpy(wsz,pLinkFile);
#endif

            // Save the link by calling IPersistFile::Save. 
            //hres = ppf->lpVtbl->Save(ppf, wsz, TRUE); 
						hres = ppf->Save(wsz, TRUE); 
            //ppf->lpVtbl->Release(ppf); 
						ppf->Release(); 
        } 
        //psl->lpVtbl->Release(psl); 
				psl->Release(); 
    } 
    return SUCCEEDED(hres); 
};

bool PathExits(CString pathtocheck)
{
  int ret = _wchdir(pathtocheck);
  return (ret == 0);
}

void  CSetupObject::DoCreateShortcuts()
{
	CoInitialize(NULL);
	// directory pro shortcuty
	CString sMainDir = _T("D:\\");
  CString sMainMenu;
	TCHAR buff[_MAX_PATH];
	//ITEMIDLIST	pidl;
	LPITEMIDLIST	ppidl;
	CWnd *wnd=AfxGetMainWnd();
	HWND hWnd=wnd ? wnd->m_hWnd : NULL;

  // Start menu folder location
  int folder = m_bAllUsers ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS;
	HRESULT hres = SHGetSpecialFolderLocation(hWnd, folder, &ppidl);
	if (SUCCEEDED(hres))
  {
    SHGetPathFromIDList( ppidl, buff );
    MakeLongDirStr(sMainMenu,buff);

    // vytváøím shortcuts
    for(int i=0; i<m_arShortcuts.GetSize(); i++)
    {
      CSetupFileItem* pItem = (CSetupFileItem*)(m_arShortcuts[i]);
      if (pItem && TestCondition(pItem->m_sCondition,pItem->m_sDifferentDest))
      {
        CString sShFile;  
        CString fileNameWithoutColon = pItem->m_sSourceFileName;
        fileNameWithoutColon.Replace(_T(':'), _T(' '));
        sShFile= fileNameWithoutColon;
        sShFile+= ".lnk";

        sMainDir = sMainMenu + m_sStartMenuGroupDir;
        MakeLongDirStr(sMainDir,sMainDir);

        // vytvoøím skupinu
        for (int ii = 0; ii < pItem->m_arLogFileName.GetSize (); ++ii)
        {
          if (!MakePath(pItem->m_arLogFileName[ii], sMainDir+sShFile, TRUE))
            return;
        }

        CString sRunFile; MakeLongDirStr(sRunFile,m_sSetupPoseidonDir);
        CString sIcoFile;        
        if (_tcslen(pItem->m_sIconPath) > 0)
          sIcoFile = sRunFile + pItem->m_sIconPath;

        sRunFile+=pItem->m_sDestinationFileName;

        if (pItem->m_sDifferentDest.IsEmpty())
        {
          DoCreateLink(sMainDir+sShFile,sRunFile,pItem->m_sSourceFileName, pItem->m_sArguments, sIcoFile, pItem->m_iIconIndex);
          // vložím smazání pro uninstall
          for (int ii = 0; ii < pItem->m_arLogFileName.GetSize (); ++ii)
          {
            AddSetupLogLine(pItem->m_arLogFileName[ii], 'F', sMainDir+sShFile);
          }
        }
        else
        {
          sMainDir = sMainMenu + pItem->m_sDifferentDest;
          MakeLongDirStr(sMainDir,sMainDir);
          if (PathExits(sMainDir))
          {
            DoCreateLink(sMainDir+sShFile,sRunFile,pItem->m_sSourceFileName, pItem->m_sArguments, sIcoFile, pItem->m_iIconIndex);
            // vložím smazání pro uninstall
            for (int ii = 0; ii < pItem->m_arLogFileName.GetSize (); ++ii)
            {
              AddSetupLogLine(pItem->m_arLogFileName[ii], 'F', sMainDir+sShFile);
            }
          }
        }
      }
    }
  }

  CoUninitialize();
};

void  CSetupObject::CreateDesktopShortcuts()
{
  CoInitialize(NULL);
  // directory pro shortcuty
  CString sMainDir = _T("D:\\");
  TCHAR buff[_MAX_PATH];
  //ITEMIDLIST	pidl;
  LPITEMIDLIST	ppidl;
  CWnd *wnd=AfxGetMainWnd();
  HWND hWnd=wnd ? wnd->m_hWnd : NULL;
  // desktop shortcuts
  sMainDir = "D:\\";

  int folder = m_bAllUsers ? CSIDL_COMMON_DESKTOPDIRECTORY : CSIDL_DESKTOPDIRECTORY;
  HRESULT hres = SHGetSpecialFolderLocation(hWnd, folder, &ppidl);
  if (SUCCEEDED(hres))
  {
    SHGetPathFromIDList( ppidl, buff );
    MakeLongDirStr(sMainDir,buff);

    // vytváøím shortcuts
    for(int i=0; i<m_arShortcutsDesktop.GetSize(); i++)
    {
      CSetupFileItem* pItem = (CSetupFileItem*)(m_arShortcutsDesktop[i]);
      if (pItem && TestCondition(pItem->m_sCondition,pItem->m_sDifferentDest))
      {
        CString sShFile;  MakeLongDirStr(sShFile,sMainDir);
        CString fileName = pItem->m_sSourceFileName;
        fileName.Replace(_T(':'), _T(' ')); //there cannot be colons inside directory or file name
        sShFile+= fileName;
        sShFile+= ".lnk";
        // vytvoøím skupinu
        for (int ii = 0; ii < pItem->m_arLogFileName.GetSize (); ++ii)
        {
          if (!MakePath(pItem->m_arLogFileName[ii], sShFile, TRUE))
            return;
        }

        CString sRunFile; MakeLongDirStr(sRunFile,m_sSetupPoseidonDir);
        CString sIcoFile;        
        if (_tcslen(pItem->m_sIconPath) > 0)
          sIcoFile = sRunFile + pItem->m_sIconPath;

        sRunFile+=pItem->m_sDestinationFileName;

        DoCreateLink(sShFile,sRunFile,pItem->m_sSourceFileName,pItem->m_sArguments, sIcoFile, pItem->m_iIconIndex);
        // vložím smazání pro uninstall
        for (int ii = 0; ii < pItem->m_arLogFileName.GetSize (); ++ii)
        {
          AddSetupLogLine(pItem->m_arLogFileName[ii], 'F', sShFile);
        }
      }
    }
  }

  CoUninitialize();
};
void  CSetupObject::CreateSendToShortcuts()
{
  CoInitialize(NULL);
  // directory pro shortcuty
  CString sMainDir = _T("D:\\");
  TCHAR buff[_MAX_PATH];
  //ITEMIDLIST	pidl;
  LPITEMIDLIST	ppidl;
  CWnd *wnd=AfxGetMainWnd();
  HWND hWnd=wnd ? wnd->m_hWnd : NULL;
  // desktop shortcuts
  sMainDir = "D:\\";

  HRESULT hres = SHGetSpecialFolderLocation(hWnd, CSIDL_SENDTO, &ppidl);
  if (SUCCEEDED(hres))
  {
    SHGetPathFromIDList( ppidl, buff );
    MakeLongDirStr(sMainDir,buff);

    // vytváøím shortcuts
    for(int i=0; i<m_arShortcutsSendTo.GetSize(); i++)
    {
      CSetupFileItem* pItem = (CSetupFileItem*)(m_arShortcutsSendTo[i]);
      if (pItem)
      {
        CString sShFile;  MakeLongDirStr(sShFile,sMainDir);
        CString fileName = pItem->m_sSourceFileName;
        fileName.Replace(_T(':'), _T(' ')); //there cannot be colons inside directory or file name
        sShFile+= fileName;
        sShFile+= ".lnk";
        // vytvoøím skupinu
        for (int ii = 0; ii < pItem->m_arLogFileName.GetSize (); ++ii)
        {
          if (!MakePath(pItem->m_arLogFileName[ii], sShFile, TRUE))
            return;
        }

        CString sRunFile; MakeLongDirStr(sRunFile,m_sSetupPoseidonDir);
        CString sIcoFile;        
        if (_tcslen(pItem->m_sIconPath) > 0)
          sIcoFile = sRunFile + pItem->m_sIconPath;

        sRunFile+=pItem->m_sDestinationFileName;

        DoCreateLink(sShFile,sRunFile,pItem->m_sSourceFileName,pItem->m_sArguments, sIcoFile, pItem->m_iIconIndex);
        // vložím smazání pro uninstall
        for (int ii = 0; ii < pItem->m_arLogFileName.GetSize (); ++ii)
        {
          AddSetupLogLine(pItem->m_arLogFileName[ii], 'F', sShFile);
        }
      }
    }
  }

  CoUninitialize();
};

unsigned int __stdcall ThreadRunAppFunc(LPVOID pParam)
{
	CSetupObject* pSetupObj = (CSetupObject*)pParam;
	return pSetupObj->DoRunAplicationsInThread();
};

#include <sys/stat.h>
unsigned int  CSetupObject::DoRunAplicationsInThread()
{
    for (int i=0; i<m_runAppFiles.GetSize(); i++)
  {
    CSetupFileItem* pItem = (CSetupFileItem*)(m_runAppFiles[i]);

      CString appName = pItem->m_sDestinationFileName;
    CString dir; MakeLongDirStr (dir, m_sSetupPoseidonDir);
    CString appNameToRun = dir + appName + " " + pItem->m_sArguments;

    if (pItem && TestCondition(pItem->m_sCondition,pItem->m_sDifferentDest))
    {
    STARTUPINFO stInfo;
    memset (&stInfo, 0, sizeof (stInfo));
    stInfo.cb = sizeof(stInfo);
    PROCESS_INFORMATION prInfo;
    memset (&prInfo, 0, sizeof (prInfo));

    if (::CreateProcess (NULL,appNameToRun.GetBuffer(), NULL, NULL,
      FALSE, CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS|CREATE_NO_WINDOW,
      NULL, dir, &stInfo, &prInfo))
    {
      // wait until it finishes its work
      ::WaitForSingleObject(prInfo.hProcess, INFINITE);

      if (prInfo.hThread) ::CloseHandle(prInfo.hThread);
      if (prInfo.hProcess) ::CloseHandle(prInfo.hProcess);
      // delete the application
      chmod(CString2RString(appNameToRun),_S_IREAD | _S_IWRITE);
      }
      appNameToRun.ReleaseBuffer();
    }
      for (int i=0;i<10;i++)
      {
      if(remove(CString2RString(dir + appName))==0) break;
        Sleep(100);
      }
    }
    EnterCriticalSection(&m_CSection_RUNAPP); 
	  m_bRunningRUNAPP=false;
	  LeaveCriticalSection(&m_CSection_RUNAPP);
    return 0;
}


int  CSetupObject::DoRunAplications()
{
  EnterCriticalSection(&m_CSection_RUNAPP); 
	bool bResult = m_bRunningRUNAPP;
	LeaveCriticalSection(&m_CSection_RUNAPP);

  if (bResult) return 0;

  EnterCriticalSection(&m_CSection_RUNAPP); 
	m_bRunningRUNAPP=true;
	LeaveCriticalSection(&m_CSection_RUNAPP);

//  EnterCriticalSection(&m_CSection_RUNAPP);
//	LeaveCriticalSection(&m_CSection_RUNAPP);
  if (_RunAppThread==0) // start installing Q:runapp 
  {
 /*   CWnd *mainWnd = AfxGetMainWnd();
    if (mainWnd)
    {
      mainWnd->ShowWindow(SW_HIDE);
    }*/
	  _RunAppThread=(HANDLE)_beginthreadex(NULL,0, ThreadRunAppFunc,this,0,0);
	  if(!_RunAppThread)
	  {
		  SetupMessageBox(_T("Cannot create Thread."),MB_OK|MB_ICONSTOP);
	  }
    return 0;
  }
  else
  {
    ::CloseHandle(_RunAppThread);
		_RunAppThread=NULL;
   /* CWnd *mainWnd = AfxGetMainWnd();
    if (mainWnd)
    {
      mainWnd->ShowWindow(SW_SHOW);
    }*/
    return 1;
  }
}

int CSetupObject::DoRunSingleApplication(CString directory, CString fileName)
{
  CString dir; MakeLongDirStr (dir, directory);
  STARTUPINFO stInfo;
  memset (&stInfo, 0, sizeof (stInfo));
  stInfo.cb = sizeof(stInfo);
  PROCESS_INFORMATION prInfo;
  memset (&prInfo, 0, sizeof (prInfo));

  CString appNameToRun = dir + fileName;

  TCHAR  buffer[4096]=TEXT(""); 
  TCHAR** lppPart={NULL};
  GetFullPathName(appNameToRun, 4096, buffer, lppPart);

  RptF("Starting process: %s", buffer);
  if (::CreateProcess (NULL,buffer, NULL, NULL,
    FALSE, CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS|CREATE_NO_WINDOW,
    NULL, dir, &stInfo, &prInfo))
  {
    RptF("Process started: %s", buffer);
    if (prInfo.hThread) ::CloseHandle(prInfo.hThread);
    if (prInfo.hProcess) ::CloseHandle(prInfo.hProcess);
  }
  else
    RptF("Unable to start process: %s", buffer);

  return 1;
}

extern BOOL GetParamsSubstrings(LPCTSTR pLine,CString& sSource,CString& sDest);
class ParseRegistryFunc
{
private:
  RString installPath;
public:
  ParseRegistryFunc(CSetupObject *sobj)
  {
    installPath = CString2RString(sobj->m_sSetupPoseidonDir);
  }
  RString operator() (const char *&ptr)
  {
    if (*ptr == '(')
    {
      const char *start = ++ptr;
      while (*ptr && *ptr!=')') ptr++;
      if (*ptr==')')
      {
        int len = ptr-start;
        RString varName(start, len);
        ptr++; //point after the %(varname) construct
        if (stricmp(varName,"INSTALL_PATH")==0) 
          return RString(installPath,installPath.GetLength()-1); //no trailing "\\"
        else if (strnicmp(varName,"REG:",4)==0) 
          return ReadReg(varName.Mid(4));
      }
    }
    return RString("");
  }
  RString ReadReg(RString pLine) 
  {
    CString line = pLine;
    CString value, rest;
    HKEY root;
    if (!GetParamsSubstrings(line, value, rest)) return RString();
    if (value.CompareNoCase(_T("hklm"))==0) root = HKEY_LOCAL_MACHINE;
    else if (value.CompareNoCase(_T("hkcu"))==0) root = HKEY_CURRENT_USER;
    else if (value.CompareNoCase(_T("hkcr"))==0) root = HKEY_CLASSES_ROOT;
    else return RString(); //unknown root (or not implemented yet)

    line = rest;
    CString path,vName;
    if (!GetParamsSubstrings(line, path, vName)) return RString();

    HKEY key;
    RString ret;
    if( ::RegOpenKeyEx(root,path, 0, KEY_READ,&key) == ERROR_SUCCESS )
    {
	    DWORD type = REG_SZ;
	    TCHAR buffer[1024];
	    DWORD size = sizeof(buffer);
      long  hr = ::RegQueryValueEx(key, vName, 0, &type, (BYTE *)buffer, &size);  
	    if (hr == ERROR_SUCCESS)
	    {
#if UNICODE
        ret = LPCTSTR2RString(buffer);
#else
		    ret = buffer;
#endif
	    }
      ::RegCloseKey(key);
    }
    return ret;
  };
};


void CSetupObject::DoCreateRegistry()
{
	HKEY key;
	for (int i = 0; i < m_arSetupLogFiles.GetSize (); ++i)
	{
		CSetupLogFileItem *uninstall = (CSetupLogFileItem *) (m_arSetupLogFiles[i]);
		if (NULL != uninstall)
		{
			CString regKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + uninstall->m_sApplication;
			// uninstall
			if
				(
				::RegCreateKey(HKEY_LOCAL_MACHINE,regKey,&key)
				==ERROR_SUCCESS
				)
			{
        CString uninstallWithPath = uninstall->m_sUninstaller;
        if (uninstall->m_addDirectoryToUninstaller) uninstallWithPath =  m_sSetupPoseidonDir + uninstallWithPath;
				::RegSetValueEx(key,_T("UninstallString"),0,REG_SZ,(BYTE*)(LPCTSTR)uninstallWithPath,(uninstallWithPath.GetLength()+1)*sizeof(TCHAR));
        ::RegSetValueEx(key,_T("DisplayName"),0,REG_SZ,(BYTE*)(LPCTSTR)uninstall->m_sMessage,(uninstall->m_sMessage.GetLength()+1)*sizeof(TCHAR));
				::RegCloseKey(key);
				// záznam k odstranìní registru
				for (int ii = 0; ii < uninstall->m_arCurrentLogFileName.GetSize (); ++ii)
				{
					AddSetupLogLine(uninstall->m_arCurrentLogFileName[ii],'K',regKey);
				}
			}
		}
	}

	// poseidon directory
  for (int i=0;i<m_sRegistryRoot.GetCount();++i)
  {
    CString regKeyApp = m_sRegistryRoot[i];
	if
	(
		::RegCreateKey(HKEY_LOCAL_MACHINE,regKeyApp,&key)
		==ERROR_SUCCESS
	)
	{
		CString sDir; MakeShortDirStr(sDir,m_sSetupPoseidonDir);
    ::RegSetValueEx(key,_T("MAIN"),0,REG_SZ,(BYTE*)(LPCTSTR)sDir,(sDir.GetLength()+1)*sizeof(TCHAR));
		if (m_cdKeyOptions.action != CKeyOptions::skip)
		{
			::RegSetValueEx(key,_T("KEY"),0,REG_BINARY,m_cdkey,KEY_BYTES);
		}
		::RegCloseKey(key);
		for (int ii = 0; ii < m_arApplLogFileName.GetSize (); ++ii)
		{
			AddSetupLogLine(m_arApplLogFileName[ii],'K',regKeyApp);
		}
		// záznam k odstranìní registru
		//... je otazka, do ktereho logFilu toto dat:
		//AddSetupLogLine('K',regKeyApp);
	}
  }
	// cd key
	
/*
	if
	(
		::RegCreateKey(HKEY_LOCAL_MACHINE,"Software\\Suma\\Poseidon",&key)
		==ERROR_SUCCESS
	)
	{
		::RegSetValueEx(key,"USER",0,REG_SZ,(BYTE*)((const char*)m_sCDKey),m_sCDKey.GetLength()+1);
		::RegCloseKey(key);
	}
*/

  // File type associations
  for (int i=0; i<m_fileTypes.GetSize(); i++)
  {
    CString line = m_fileTypes[i];
    int to = line.Find(':');
    if (to < 0) continue;
    CString extension = line.Left(to);
    int from = to + 1;
    to = line.Find(':', from);
    CString regClass = line.Mid(from, to - from);
    from = to + 1;
    to = line.Find(':', from);
    CString exeName = line.Mid(from, to - from);
    from = to + 1;
    CString icon = line.Mid(from);

    CString dir;
    MakeLongDirStr(dir, m_sSetupPoseidonDir);
    CString exePath = dir + exeName;
    CString command = "\"" + exePath + "\" \"%1\"";
    CString defaultIcon = exePath + "," + icon;
    CString regKeyCommand = regClass + "\\shell\\open\\command";
    CString regKeyDefaultIcon = regClass + "\\DefaultIcon";

    if (::RegCreateKey(HKEY_CLASSES_ROOT, extension, &key) == ERROR_SUCCESS)
    {
      ::RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE *)(LPCTSTR)regClass, (regClass.GetLength() + 1)*sizeof(TCHAR));
      ::RegCloseKey(key);
    }
    if (::RegCreateKey(HKEY_CLASSES_ROOT, regKeyCommand, &key) == ERROR_SUCCESS)
    {
      ::RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE *)(LPCTSTR)command, (command.GetLength() + 1)*sizeof(TCHAR));
      ::RegCloseKey(key);
    }
    if (::RegCreateKey(HKEY_CLASSES_ROOT, regKeyDefaultIcon, &key) == ERROR_SUCCESS)
    {
      ::RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE *)(LPCTSTR)defaultIcon, (defaultIcon.GetLength()+1)*sizeof(TCHAR));
      ::RegCloseKey(key);
    }

    for (int j=0; j<m_arApplLogFileName.GetSize(); j++)
    {
      AddSetupLogLine(m_arApplLogFileName[j], 'C', extension);
      AddSetupLogLine(m_arApplLogFileName[j], 'C', regKeyCommand);
      AddSetupLogLine(m_arApplLogFileName[j], 'C', regKeyDefaultIcon);
    }
  }

  // Extended file type associations
  for (int i=0; i<m_fileTypesEx.GetSize(); i++)
  {
    // parse the command line:
    // Q:filetypeEx:<extension>:<file_type_name>:<exe_name>:<icon>:<product_name>:<file_version>
    CString line = m_fileTypesEx[i];
    int to = line.Find(':');
    if (to < 0) continue;
    CString extension = line.Left(to);
    int from = to + 1;
    to = line.Find(':', from);
    CString fileTypeName = line.Mid(from, to - from);
    from = to + 1;
    to = line.Find(':', from);
    CString exeName = line.Mid(from, to - from);
    from = to + 1;
    to = line.Find(':', from);
    CString icon = line.Mid(from, to - from);
    from = to + 1;
    to = line.Find(':', from);
    CString productName = line.Mid(from, to - from);

    CString sCondition;
    int fileVersion;
    from = to + 1;
    to = line.Find(':', from);
    if (to==-1)
    {
      fileVersion = _tstoi(line.Mid(from));
    }
    else
    {
      fileVersion =  _tstoi(line.Mid(from, to - from));
      from = to + 1;
      sCondition = line.Mid(from);
      if (!TestCondition(sCondition)) return; 
    }    


    // check the current association
    int oldFileVersion = fileVersion;
    bool ok = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, extension, 0, KEY_READ, &key) == ERROR_SUCCESS;
    if (ok)
    {
      // the key already exist
      TCHAR buffer[1024];
      // try to use the current file type
      DWORD size = sizeof(buffer);
      DWORD type = REG_SZ;
      LONG result = ::RegQueryValueEx(key, NULL, 0, &type, (BYTE *)buffer, &size);  
      if (result == ERROR_SUCCESS) fileTypeName = buffer;
      else ok = false;
      // read the current file version
      size = sizeof(buffer);
      type = REG_DWORD;
      result = ::RegQueryValueEx(key, _T("fileVersion"), 0, &type, (BYTE *)buffer, &size);  
      if (result == ERROR_SUCCESS) oldFileVersion = *(int *)buffer;

      ::RegCloseKey(key);
    }
    // fix the extension association if needed
    if (!ok || fileVersion > oldFileVersion)
    {
      if (::RegCreateKey(HKEY_CLASSES_ROOT, extension, &key) == ERROR_SUCCESS)
      {
        // new association to the file type is needed
        if (!ok) ::RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE *)(LPCTSTR)fileTypeName, (fileTypeName.GetLength() + 1) * sizeof(TCHAR));
        // current file version need to be updated
        ::RegSetValueEx(key, _T("fileVersion"), 0, REG_DWORD, (BYTE *)&fileVersion, sizeof(int));
        
        ::RegCloseKey(key);
      }
    }

    // the directory we are installing to
    CString dir;
    MakeLongDirStr(dir, m_sSetupPoseidonDir);
    // the full path to the associated exe
    CString exePath = dir + exeName;
    CString defaultIcon = exePath + "," + icon;

    // update the application table
    if (::RegCreateKey(HKEY_CLASSES_ROOT, extension + "\\AppsTable\\" + productName, &key) == ERROR_SUCCESS)
    {
      // store the associated exe and the file version
      ::RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE *)(LPCTSTR)exePath, (exePath.GetLength() + 1) * sizeof(TCHAR));
      ::RegSetValueEx(key, _T("fileVersion"), 0, REG_DWORD, (BYTE *)&fileVersion, sizeof(int));
      ::RegSetValueEx(key, _T("defaultIcon"), 0, REG_SZ, (BYTE *)(LPCTSTR)defaultIcon, (defaultIcon.GetLength() + 1) * sizeof(TCHAR));

      ::RegCloseKey(key);
    }

    // update the association of exe to the file type
    if (fileVersion >= oldFileVersion)
    {
      CString regKeyCommand = fileTypeName + "\\shell\\open\\command";
      if (::RegCreateKey(HKEY_CLASSES_ROOT, regKeyCommand, &key) == ERROR_SUCCESS)
      {
        CString command = "\"" + exePath + "\" \"%1\"";
        ::RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE *)(LPCTSTR)command, (command.GetLength() + 1)*sizeof(TCHAR));
        ::RegCloseKey(key);
      }
      CString regKeyDefaultIcon = fileTypeName + "\\DefaultIcon";
      if (::RegCreateKey(HKEY_CLASSES_ROOT, regKeyDefaultIcon, &key) == ERROR_SUCCESS)
      {
        ::RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE *)(LPCTSTR)defaultIcon, (defaultIcon.GetLength()+1)*sizeof(TCHAR));
        ::RegCloseKey(key);
      }
    }

    // uninstall info
    for (int j=0; j<m_arApplLogFileName.GetSize(); j++)
    {
      AddSetupLogLine(m_arApplLogFileName[j], 'A', extension + ":" + productName);
    }
  }

  // Other Registry Keys specified in setup.dat by Q:registry command
  for (int i=0; i<m_registryKeys.Size(); i++)
  {
    SetupRegistryKey &reg = m_registryKeys[i];
    if (reg.valueType==REG_SZ)
    {
      ParseRegistryFunc parseFunc(this);
      RString valueData(CString2RString(reg.valueData));
      valueData = valueData.ParseFormat(parseFunc); //convert all %(varname) into its values
      reg.valueData = valueData;
    }
    if( ::RegCreateKey(reg.root,reg.path,&key) == ERROR_SUCCESS )
    {
      ::RegSetValueEx(key,reg.valueName,0,reg.valueType,(BYTE*)(LPCTSTR)reg.valueData,(reg.valueData.GetLength()+1)*sizeof(TCHAR));
      ::RegCloseKey(key);
    }
    //insert entry for uninstall
    for (int i = 0; i < m_arSetupLogFiles.GetSize (); ++i)
    {
      CSetupLogFileItem *uninstall = (CSetupLogFileItem *) (m_arSetupLogFiles[i]);
      if (NULL != uninstall)
      {
        for (int ii = 0; ii < m_arApplLogFileName.GetSize (); ++ii)
        {
          if (reg.root== HKEY_LOCAL_MACHINE) AddSetupLogLine(uninstall->m_arCurrentLogFileName[ii],'K',reg.path);
          else if (reg.root== HKEY_CURRENT_USER) AddSetupLogLine(uninstall->m_arCurrentLogFileName[ii],'U',reg.path+":"+reg.valueName);
        }
      }
    }
  }
};

typedef HRESULT (CALLBACK *AddToGameExplorer)(const char *, const char *, int, GUID *);
typedef HRESULT (CALLBACK *CreateTask)(int, GUID *, BOOL, int, const char *, const char *, const char *);

void CSetupObject::DoRegisterInGameExplorer()
{
  if (m_geResource.GetLength() == 0) return;

  CString dir;
  MakeLongDirStr(dir, m_sSetupPoseidonDir);

  CString dllName = dir + "GameuxInstallHelper.dll";

  HINSTANCE dll = LoadLibrary(dllName);
  if (dll == NULL)
  {
    m_nSetupResult	= stpSetupError;
    return;
  }

  AddToGameExplorer addToGameExplorer = (AddToGameExplorer)GetProcAddress(dll, "AddToGameExplorerA");
  CreateTask createTask = (CreateTask)GetProcAddress(dll, "CreateTaskA");
  if (addToGameExplorer && createTask)
  {
    // enum GAME_INSTALL_SCOPE
    // {	
    //   GIS_NOT_INSTALLED = 1,
    //   GIS_CURRENT_USER = 2,
    //   GIS_ALL_USERS = 3
    // };
    int scope = m_bAllUsers ? 3 : 2;
    GUID guid = GUID_NULL;
    CString resourceName = dir + m_geResource;
    // register game
    HRESULT result = addToGameExplorer(CString2RString(resourceName), CString2RString(dir), scope, &guid);
    if (SUCCEEDED(result))
    {
      // add tasks
      for (int i=0; i<m_gePlayTasks.GetSize(); i++)
      {
        CString str = m_gePlayTasks[i];
        int pos1 = str.Find(':');
        if (pos1 < 0) continue;
        int pos2 = str.Find(':', pos1 + 1);
        if (pos2 < 0) pos2 = str.GetLength() - 1;
        CString name = str.Left(pos1);
        CString app = dir + str.Mid(pos1 + 1, pos2 - pos1);
        CString arg = str.Mid(pos2 + 1);
        createTask(scope, &guid, FALSE, i, CString2RString(name), CString2RString(app), CString2RString(arg));
      }
      for (int i=0; i<m_geSupportTasks.GetSize(); i++)
      {
        CString str = m_geSupportTasks[i];
        int pos1 = str.Find(':');
        if (pos1 < 0) continue;
        int pos2 = str.Find(':', pos1 + 1);
          if (pos2 < 0) pos2 = str.GetLength() - 1;
        CString name = str.Left(pos1);
        CString app = dir + str.Mid(pos1 + 1, pos2 - pos1);
        CString arg = str.Mid(pos2 + 1);
        createTask(scope, &guid, TRUE, i, CString2RString(name), CString2RString(app), CString2RString(arg));
      }

      // write to setup.log
      TCHAR str[256 + MAX_PATH];
      _stprintf(str, _T("{%8X-%4X-%4X-%2X%2X-%2X%2X%2X%2X%2X%2X}:%s"),
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7],
        dllName);
      for (int i=0; i< m_arApplLogFileName.GetSize(); i++)
      {
        AddSetupLogLine(m_arApplLogFileName[i], 'G', str);
      }
    }
    else
    {
      m_nSetupResult	= stpSetupError;
    }
  }
  else
  {
    m_nSetupResult	= stpSetupError;
  }
  FreeLibrary(dll);       
}

// open firewall for the application
void CSetupObject::DoSetupFirewall()
{
  for (int i=0; i<_firewall.GetSize(); i++)
  {
    CString appName;
    CString relativePath;
    if (!GetFileParamsSubstrings(_firewall[i], appName, relativePath)) continue;
    CString dir;
    MakeLongDirStr(dir, m_sSetupPoseidonDir);
    CString fullPath = dir + relativePath;
    if (WindowsFirewallAddApplication(fullPath, appName))
    {
      // add uninstall
      for (int j=0; j<m_arApplLogFileName.GetSize(); j++)
      {
        AddSetupLogLine(m_arApplLogFileName[j], 'W', fullPath);
      }
    }
  }
}

// vytvoøení setup.log
void CSetupObject::DoCreateSetupLog()
{
	CString sFileDir;
	MakeLongDirStr(sFileDir,m_sSetupPoseidonDir);
	for (int i = 0; i < m_arSetupLogFiles.GetSize (); ++i)
	{
		CSetupLogFileItem* pItem = (CSetupLogFileItem*)(m_arSetupLogFiles[i]);
		if (NULL != pItem)
		{
			// vložím smazání sebe sama
			for (int ii = 0; ii < pItem->m_arCurrentLogFileName.GetSize (); ++ii)
			{
				AddSetupLogLine(pItem->m_arCurrentLogFileName[ii],'F',sFileDir + pItem->m_sLogFileName);
			}
		}
	}

	for (int i = 0; i < m_arSetupLogFiles.GetSize (); ++i)
	{
		CSetupLogFileItem* pItem = (CSetupLogFileItem*)(m_arSetupLogFiles[i]);
		if (NULL != pItem)
		{
			CFile fileSap;
			TRY
			{
				if (!fileSap.Open(sFileDir + pItem->m_sLogFileName,CFile::modeCreate|CFile::modeWrite))
				{
					AfxThrowUserException();
				}

				static char* pNewLine = "\r\n";
				CString sLine = pItem->m_sApplication;

				RString dataR = CString2RString(sLine,CP_UTF8); //no conversion done for nonUNICODE
        fileSap.Write(cc_cast(dataR),dataR.GetLength());
        fileSap.Write(pNewLine,strlen(pNewLine));

				// uložení jednotlivých øádkù
				for(int i=0; i<pItem->m_arSetupLog.GetSize(); i++)
				{
					sLine = pItem->m_arSetupLog[i];
					if (sLine.GetLength()>0)
					{
            dataR = CString2RString(sLine,CP_UTF8);
            fileSap.Write(cc_cast(dataR),dataR.GetLength());
//          fileSap.Write((LPCTSTR)sLine,sLine.GetLength());
            fileSap.Write(pNewLine,strlen(pNewLine));
					}
				}
				fileSap.Close();
			}
			CATCH_ALL(e)
			{
				fileSap.Abort();
				return;
			}
			END_CATCH_ALL
		}
	}
};

void CSetupObject::DoCreateCheckFileLog()
{
  CFile fileSap;
  TRY
  {
    if (!fileSap.Open(m_sSetupSourceDir + m_sCheckFileName, CFile::modeCreate|CFile::modeWrite))
    {
      AfxThrowUserException();
    }

    static char* pNewLine = "\r\n";

    // uložení jednotlivých øádkù
    for(int i=0; i<m_arCheckFiles.GetSize(); i++)
    {
      CString sLine = m_arCheckFiles[i];
      if (sLine.GetLength()>0)
      {
        RString dataR = CString2RString(sLine,CP_UTF8);
        fileSap.Write(cc_cast(dataR),dataR.GetLength());
        fileSap.Write(pNewLine,strlen(pNewLine));
      }
    }
    fileSap.Close();
  }
  CATCH_ALL(e)
  {
    fileSap.Abort();
    return;
  }
  END_CATCH_ALL
}

void CSetupObject::AddCheckFileListItem(LPCTSTR file, __int64 fileSize)
{
  char sFileSize[32];
  _i64toa(fileSize, sFileSize, 10);
  CString sLine = file;
  sLine += "?";
  sLine += sFileSize;
  m_arCheckFiles.Add(sLine);
}

void CSetupObject::AddSetupLogLine(LPCTSTR logFile,char nID,LPCTSTR pLine)
{
	bool logFound = false;
	CSetupLogFileItem *pItem = NULL;

	for (int i = 0; i < m_arSetupLogFiles.GetSize (); ++i)
	{
		pItem = (CSetupLogFileItem*) m_arSetupLogFiles[i];
		if (NULL != pItem)
		{
			if (pItem->m_sLogFileName == logFile)
			{
				logFound = true;
				break;
			}
		}
	}

	if (!logFound || !pItem)
	{
		return;
	}

	char buff[3] = { 'F', ':', 0 }; buff[0] = (char)nID;
	CString sLine = buff;
	sLine += pLine;
	BOOL bExist = FALSE;
	for (int i = 0; i < pItem->m_arSetupLog.GetSize(); i++)
	{
		if(lstrcmp(sLine,pItem->m_arSetupLog[i])==0)
		{
			bExist = TRUE;
			break;
		}
	}

	if (!bExist)
	{
		pItem->m_arSetupLog.Add(sLine);
	}
};

void CSetupObject::ApplyApplicationNameAndStringTable (CWnd *item)
{
	CString text;
	item->GetWindowText (text);

	if (ApplyApplicationNameAndStringTable (text, true))
	{
		//item->SetWindowText (text);
    SetupSetWindowText(item, text);
	}

	CWnd *subItem = item->GetTopWindow ();
	while (subItem != NULL)
	{
		ApplyApplicationNameAndStringTable (subItem);
		subItem = subItem->GetNextWindow (GW_HWNDNEXT);
	}
}

bool CSetupObject::ApplyApplicationNameAndStringTable (CString &text, bool applyApplName)
{
	bool r = false;
	int f = text.FindOneOf (_T ("%$"));
	int max = text.GetLength () - 1;
	if (f >= 0 && f < max)
	{
		do {
			switch (text[f])
			{
			case _T ('$'):
				{
					int a = f + 1;
					while ((a <= max) && 
						(text[a] >= _T ('A') && text[a] <= _T ('Z') ||
						text[a] >= _T ('0') && text[a] <= _T ('9') ||
						text[a] >= _T ('a') && text[a] <= _T ('z') ||
						text[a] == _T ('_')))
					{
						++a;
					}
					text = text.Left (f) + 
						SetupLocalizeString (text.Mid (f + 1, a - f - 1)) + 
						text.Mid (a);
					max = text.GetLength () - 1;
					f = SetupFindOneOf (text, _T ("%$"), f);
					r = true;
				}
				break;

			case _T ('%'):
				switch (text[f + 1])
				{
				case _T ('0'):
					if (applyApplName)
					{
						text = text.Left (f) + m_sApplicationName + text.Mid (f + 2);
						max = text.GetLength () - 1;
						f = SetupFindOneOf (text, _T ("%$"), f);
						r = true;
					}
					else
					{
						f = SetupFindOneOf (text, _T ("%$"), f + 1);
					}
					break;

				default:
					f = SetupFindOneOf (text, _T ("%$"), f + 1);
					break;
				}
				break;

			default:
				f = SetupFindOneOf (text, _T ("%$"), f);
				break;
			}
		} while (f >= 0 && f < max);
	}

	return r;
}

int CSetupObject::SetupFindOneOf (CString &where, LPCTSTR charSet, int start)
{
	int len = where.GetLength ();
	if (start < 0 || start >= len)
	{
		return -1;
	}

	int charSetLen = _tcslen (charSet);
	while (start < len)
	{
		TCHAR c = where[start];
		for (int i = 0; i < charSetLen; ++i)
		{
			if (c != charSet[i])
			{
				continue;
			}

			return start;
		}
		++start;
	}
	return -1;
}

int CSetupObject::SetupMessageBox (CString &text, UINT nType /*= MB_OK*/, UINT nIDHelp /*= 0*/ , HWND hwnd /*= NULL*/)
{
	text.Replace(_T("%0"), m_sApplicationName);
#ifdef UNICODE
  return ::MessageBoxW(hwnd, text, m_pPosSetupApp->m_pszAppName, nType);
#else
  Temp<wchar_t> bufW;
  {
    int wSize = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    bufW.Realloc(wSize);
    MultiByteToWideChar(CP_ACP, 0, text, -1, bufW.Data(), wSize);
  }
  //return AfxMessageBox (text, nType, nIDHelp);
  Temp<wchar_t> captionW;
  {
    int wSize = MultiByteToWideChar(CP_ACP, 0, m_pPosSetupApp->m_pszAppName, -1, NULL, 0);
    captionW.Realloc(wSize);
    MultiByteToWideChar(CP_ACP, 0, m_pPosSetupApp->m_pszAppName, -1, captionW.Data(), wSize);
  }
  return ::MessageBoxW(NULL, bufW.Data(), captionW.Data(), nType);
#endif
}

int CSetupObject::SetupMessageBox (LPCTSTR lpszText, UINT nType /*= MB_OK*/, UINT nIDHelp /*= 0*/, HWND hwnd /*= NULL*/)
{
  CString text(lpszText);
	return SetupMessageBox (text, nType, nIDHelp, hwnd);
}

int CSetupObject::SetupMessageBox (UINT nIDPrompt, UINT nType /*= MB_OK*/, UINT nIDHelp /*= (UINT) -1*/, HWND hwnd /*= NULL*/)
{
	CString sText;
	VERIFY (sText.LoadString (nIDPrompt));
	return SetupMessageBox (sText, nType, nIDHelp, hwnd);
}

void SetupSetWindowText(CWnd *wnd, CString text)
{
  SetupSetWindowText(wnd->m_hWnd, text);
}

BOOL SetupSetWindowText(HWND hWnd, CString text)
{
#ifdef UNICODE
  return ::SetWindowText(hWnd, text);
#else
  Temp<wchar_t> bufW;
  {
    int wSize = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    bufW.Realloc(wSize);
    MultiByteToWideChar(CP_ACP, 0, text, -1, bufW.Data(), wSize);
  }
  return ::SetWindowTextW(hWnd, bufW);
#endif
}
