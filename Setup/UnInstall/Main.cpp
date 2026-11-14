#include <El/elementpch.hpp>
#include "stdafx.h"

#include <El/Stringtable/stringtable.hpp>
#include <Es\Files\filenames.hpp>

#include "uninstall.h"

HINSTANCE hInst; 
TCHAR  applName[256] = _T("Unknown Application");
TCHAR directory[_MAX_PATH];

TCHAR  sModuleFileName[_MAX_PATH];
TCHAR  sSpecFileName[_MAX_PATH];
TCHAR  sSetupLogFileName[_MAX_PATH];
TCHAR  sNewCmdLine[_MAX_PATH+_MAX_PATH];
TCHAR  sSetupCsvFileName[_MAX_PATH];


HINSTANCE AfxGetInstanceHandle()
{
  return hInst;
}

/////////////////////////////////////////////////////////////////////////////
// WinMain

int SkipUnicodePrefix(char * textData)
{
  int len = strlen(textData);
  if (len<3) return 0;
  unsigned char c1 = textData[0];
  unsigned char c2 = textData[1];

  if (c1 == 0xff && c2 == 0xfe) return 2;
  if (c1 == 0xfe && c2 == 0xff) return 2;
  if (c1 == 0xef && c2 == 0xbb)
  {
    if (len>=3 && (unsigned char)textData[2]==0xbf) return 3; // UTF-8 prefix
  }
  return 0;
}

#define BETA_PATCH_UNINSTALL 0

void GetMainDirectoryFromRegistry(DWORD len, LPTSTR directory)
{
  HKEY key;
  if (RegOpenKey (HKEY_LOCAL_MACHINE, _T("Software\\Bohemia Interactive Studio\\ArmA 2"), &key) == ERROR_SUCCESS)
  {
    if (RegQueryValueEx (key, _T("MAIN"), 0, NULL, (BYTE*) directory, &len) != ERROR_SUCCESS)
    {
      *directory = _T('\0');
      RegCloseKey (key);
      return;
    }

    TerminateBy(directory, _T('\\'));
    RegCloseKey (key);
  }
}

inline LPWSTR GetFilenameExt( LPWSTR w )
{	// short name with extension
  LPWSTR nam = wcsrchr(w,_T('\\'));
  if( nam ) return nam+1;
  if( w[0]!=0 && w[1]==':' ) return w+2;
  return w;
}

#if UNICODE
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#endif
{
	int  nRes, begin;
	TCHAR str[256];
	TCHAR title[100];
	char *beginChr;
	int  hFile;
	
	BOOL bUninstall;
	bUninstall = FALSE;

	switch (PRIMARYLANGID (GetUserDefaultLangID ()))
	{
	case LANG_FRENCH:
		GLanguage = "French";
		break;
	case LANG_SPANISH:
		GLanguage = "Spanish";
		break;
	case LANG_ITALIAN:
		GLanguage = "Italian";
		break;
	case LANG_GERMAN:
		GLanguage = "German";
		break;
	case LANG_POLISH:
		GLanguage = "Polish";
		break;
	case LANG_CZECH:
		GLanguage = "Czech";
		break;
	case LANG_RUSSIAN:
		GLanguage = "Russian";
		break;
	case LANG_HUNGARIAN:
		GLanguage = "Hungarian";
		break;

	default:
		GLanguage = "English";
		break;
	}

#if BETA_PATCH_UNINSTALL
  TCHAR curDir[512];
  GetMainDirectoryFromRegistry(MAX_PATH, curDir);
  if (!(*curDir)) return 1; //unable to find the root directory in the registry
  if (_wchdir(curDir)) return 2;  //unable to use this directory
  TCHAR newCmdLine[]=L"#@#uninstall.log";
  lpCmdLine = newCmdLine;
#endif

	// instance
	hInst = hInstance;
	GetModuleFileName(hInstance,sModuleFileName,_MAX_PATH);

	// cesta k setup.csv
	lstrcpy(sSetupCsvFileName,sModuleFileName);
#if BETA_PATCH_UNINSTALL
  TCHAR *fileName = GetFilenameExt(sSetupCsvFileName);
  *fileName = _T('\0');
  lstrcpy(sSetupCsvFileName, _T("uninstall.csv"));
#else
  nRes = lstrlen(sSetupCsvFileName);
	while(sSetupCsvFileName[nRes-1] != _T('.'))
		nRes--;
	lstrcpy(sSetupCsvFileName+nRes,_T("csv"));
#endif
	LoadStringtable ("global", LPCTSTR2RString(sSetupCsvFileName));

	if (lstrlen(lpCmdLine)>3)
	{
		if ((lpCmdLine[0]==_T('#'))&&(lpCmdLine[1]==_T('@'))&&(lpCmdLine[2]==_T('#')))
		{	
			bUninstall = TRUE;
		}
	}
	if (!bUninstall)	
	{
		// cesta k setup.log
		lstrcpy(sSetupLogFileName,sModuleFileName);
		nRes = lstrlen(sSetupLogFileName);
    while(sSetupLogFileName[nRes-1] != _T('.'))
			nRes--;
		lstrcpy(sSetupLogFileName+nRes,_T("log"));

		// File Exist ?
		if (_taccess(sSetupLogFileName,00)!=0)
		{
			TCHAR str[100];
			TCHAR str2[100+_MAX_PATH];
			TCHAR title[100];
			PosLoadString("UNINSTALL_FAIL_NOTFOUND",str,100);
			PosLoadString("UNINSTALL_APP_NAME",title,100);
			wsprintf(str2,str,sSetupLogFileName);
			MessageBox(NULL,str2,title,MB_OK|MB_ICONSTOP|MB_APPLMODAL);
			return 0;
		}

		// nactu soubor
		hFile = _topen( sSetupLogFileName, _O_RDONLY | _O_TEXT );
		if (hFile == -1)
		{
			return 0;
		}

		nRes = _filelength (hFile);
		char *buffer = (char*) malloc (nRes);
		if (buffer == NULL)
		{
			_close(hFile);
			return 0;
		}
		int read = 0;
		while (read < nRes)
		{
			int tl = _read(hFile,buffer + read,nRes - read);
			if (tl == -1)
			{
				_close(hFile);
				free (buffer);
				buffer = NULL;
				return 0;
			}
			else if (tl == 0)
			{
				break;
			}
			read += tl;
		}
		_close(hFile);
    buffer[read]=0; //zero terminate
    int skipIx = SkipUnicodePrefix(buffer);

		// nactu nazev aplikace
		beginChr = strchr (buffer, '\n');
		begin = 0;
		if (beginChr != NULL)
		{
			begin = beginChr - buffer;
#if UNICODE
      char appName[256];
			strncpy (appName, buffer+skipIx, begin-skipIx);
      appName[begin-skipIx]=0;
      MultiByteToWideChar(CP_UTF8, 0, appName, -1, applName, 256);
#else
      strncpy (applName, buffer+skipIx, begin-skipIx);
      applName[begin]=0;
#endif
			++begin;
		}

		// question
		PosLoadString("UNINSTALL_APP_NAME",title,100);
		PosLoadString("UNINSTALL_QUESTION",str,100);
		nRes = MessageBox(NULL,str,title,MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION|MB_APPLMODAL);
		if (nRes != IDYES)
		{
			free (buffer);
			buffer = NULL;
			return 0;	// storno
		}

		// kopíruji modul do temp souboru
		GetTempPath(_MAX_PATH,sNewCmdLine);
		GetTempFileName(sNewCmdLine,_T("STP"),0,sSpecFileName);
		if (!DoCopyFile(sModuleFileName,sSpecFileName,FALSE))
		{
			free (buffer);
			buffer = NULL;
			return 0;
		}

		TCHAR sTempCsv[_MAX_PATH];
		lstrcpy (sTempCsv, sSpecFileName);
		nRes = lstrlen(sTempCsv);
		while(sTempCsv[nRes-1] != '.')
			nRes--;
		lstrcpy(sTempCsv+nRes,_T("csv"));

		if (!DoCopyFile(sSetupCsvFileName,sTempCsv,FALSE))
		{
			free (buffer);
			buffer = NULL;
			return 0;
		}

		// spustim sam sebe
		lstrcpy(sNewCmdLine,sSpecFileName);
		lstrcat(sNewCmdLine,_T(" #@#"));
		lstrcat(sNewCmdLine, sSetupLogFileName);
		// Add original EXE file name
		/*
		lstrcat(sNewCmdLine, "|");
		lstrcat(sNewCmdLine, sModuleFileName);
		*/

		// run
		// WinExec(sNewCmdLine,SW_SHOW);
		
		STARTUPINFO stInfo;
		memset (&stInfo, 0, sizeof (stInfo));
		stInfo.cb = sizeof(stInfo);
		
		PROCESS_INFORMATION prInfo;
		memset (&prInfo, 0, sizeof (prInfo));

		if (CreateProcess (NULL, sNewCmdLine, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS,
			NULL, NULL, &stInfo, &prInfo))
		{
			::CloseHandle (prInfo.hThread);
			::CloseHandle (prInfo.hProcess);
		}

		free (buffer);
		buffer = NULL;
GetUserDirectoriesString();
	} else
	{
		int i;

		// Delete original EXE file
		/*
		char *ptr = strchr (lpCmdLine + 3, '|');
		if (ptr != NULL)
		{
			time_t t = time (NULL);
			while (!DoDeleteFile (ptr + 1))
			{
				if (time (NULL) - t > 3)
				{
					break;
				}
			}
			*ptr = '\0';
		}
		*/

    lstrcpy(sSetupLogFileName, lpCmdLine+3);
		// nactu soubor

		
    hFile = _topen( sSetupLogFileName, _O_RDONLY | _O_TEXT );
		if (hFile == -1)
		{
			return 0;
		}

		nRes = _filelength (hFile);
		char *buffer = (char*) malloc (nRes+1);
		if (buffer == NULL)
		{
			_close(hFile);
			return 0;
		}

		int read = 0;
		while (read < nRes)
		{
			int tl = _read(hFile,buffer + read,nRes - read);
			if (tl == -1)
			{
				_close(hFile);
				free (buffer);
				buffer = NULL;
				return 0;
			}
			else if (tl == 0)
			{
				break;
			}
			read += tl;
		}
		_close(hFile);
    buffer[read]=0; //zero terminate
    int skipIx = SkipUnicodePrefix(buffer);

		// adresar
		lstrcpy(directory, sSetupLogFileName);
		for (i=lstrlen(directory)-1; i>=0; i--)
		{
			if (directory[i] == _T('\\') || directory[i] == _T('/'))
			{
				directory[i] = 0;
				break;
			}
		}

#if !BETA_PATCH_UNINSTALL

#endif

		// nactu nazev aplikace
		beginChr = strchr (buffer, '\n');
		begin = 0;
		if (beginChr != NULL)
		{
			begin = beginChr - buffer;
#if UNICODE
      char appName[256];
      strncpy (appName, buffer+skipIx, begin-skipIx);
      appName[begin-skipIx]=0;
      MultiByteToWideChar(CP_UTF8, 0, appName, -1, applName, 256);
#else
      strncpy (applName, buffer+skipIx, begin-skipIx);
      applName[begin]=0;
#endif
      //strncpy (applName, buffer, begin);
			++begin;
		}

		// zpracuji soubor (po radcich od konce)
		AutoArray<RString> uninstLines;
    for(;;)
		{
			nRes = strlen(buffer);
			if (nRes <= begin)
				break;
			// konec rádku
			while((nRes > 0)&&(buffer[nRes-1] != '\n'))
				nRes--;
      
      char newCmdLn[1024];
      strcpy(newCmdLn,buffer+nRes);
      uninstLines.Add(newCmdLn);

			if (nRes > 0) nRes--;
			buffer[nRes] = 0;
		};
		// end

#if !BETA_PATCH_UNINSTALL
    //first pass ... init final uninstall settings (P commands)
    for (int i=0; i<uninstLines.Size(); i++)
    {
#if UNICODE
      MultiByteToWideChar(CP_UTF8, 0, uninstLines[i].Data(), -1, sNewCmdLine, 256);
#else
      strcpy(sNewCmdLine, uninstLines[i].Data());
#endif
      if (sNewCmdLine[0]=='P' || sNewCmdLine[0]=='p') DoUnInstallCmd(sNewCmdLine);
    }
#endif //BETA_PATCH_UNINSTALL

    //second pass ... uninstall files and other stuff
    int lastLine = uninstLines.Size()-1;
#if BETA_PATCH_UNINSTALL
    lastLine = -1; //no uninstalling when beta not found
    //the first line is the first one containing "\\beta\n"
    const char pattern[] = "\\beta";
    int patlen = strlen(pattern);
    for (int i=uninstLines.Size()-1; i>=0; i--)
    {
      int len = uninstLines[i].GetLength();
      if ( len>=patlen && stricmp(uninstLines[i].Substring(len-patlen,len), pattern)==0 )
      {
        lastLine = i;
        break;
      }
    }
#endif //BETA_PATCH_UNINSTALL
    for (int i=0; i<=lastLine; i++)
    {
#if UNICODE
      MultiByteToWideChar(CP_UTF8, 0, uninstLines[i].Data(), -1, sNewCmdLine, 256);
#else
      strcpy(sNewCmdLine, uninstLines[i].Data());
#endif
      if (sNewCmdLine[0]!='P' && sNewCmdLine[0]!='p') DoUnInstallCmd(sNewCmdLine);
    }

#if BETA_PATCH_UNINSTALL
    // there was bug in the OFPPatch.exe used in ArmA Patch 1.07, so wee need to uninstall Dta directory specifically
    TCHAR rmDir[512]; 
    wcscpy(rmDir, curDir); wcscat(rmDir,_T("beta\\dta"));
    _wrmdir(rmDir); //it removes only empty directories, so it is relatively safe
    wcscpy(rmDir, curDir); wcscat(rmDir,_T("beta"));
    _wrmdir(rmDir);
    // we need update the uninstall.log to contain only lines which were not processed during this uninstall
    PosLoadString("UNINSTALL_APP_NAME",title,100);
    if (lastLine>=0)
    {
      TCHAR unlogFile[512];
      wcscpy(unlogFile, curDir);
      wcscat(unlogFile, sSetupLogFileName);
      FILE *unlog = _tfopen(unlogFile, _T("wt"));
      if (unlog)
      {
        for (int i=uninstLines.Size()-1; i>lastLine; i--)
        {
          fprintf(unlog, "%s\n", cc_cast(uninstLines[i]));
        }
        fclose(unlog);
      }
      //and English message box saying everything worked great
      MessageBox(NULL,_T("ArmA2 Beta patch was successfully uninstalled."),_T("ARMA II Beta patch Uninstall"),MB_OK);
    }
    else
    {
      //and English message box saying everything worked great
      MessageBox(NULL,_T("ArmA2 Beta patch is not installed."),_T("ARMA II Beta patch Uninstall"),MB_OK);
    }
#else
    //Section: Uninstall user saves
    {
      // question
      PosLoadString("UNINSTALL_APP_NAME",title,100);
      PosLoadString("UNINSTALL_END",str,256);
      TCHAR strEndOk[256];
      PosLoadString("UNINSTALL_END_OK",strEndOk,256);

      RString directories = GetUserDirectoriesString();
#if UNICODE
      Temp<wchar_t> bufW;
      {
        int wSize = MultiByteToWideChar(CP_UTF8, 0, directories, -1, NULL, 0);
        bufW.Realloc(wSize);
        MultiByteToWideChar(CP_UTF8, 0, directories, -1, bufW.Data(), wSize);
      }
#else
      const char *bufW = cc_cast(directories);
#endif
      TCHAR userDirMsg[4096]; _stprintf(userDirMsg, str, bufW);    
      if (strEndOk[0]==_T('\0')) lstrcpy(strEndOk,userDirMsg);

      if (!directories.IsEmpty()) 
      {
        if (MessageBox(NULL,userDirMsg,title,MB_YESNO|MB_ICONWARNING|MB_APPLMODAL|MB_DEFBUTTON2)==IDYES)
        {
          DoUninstallFinal();
        }
      }
      else MessageBox(NULL,strEndOk,title,MB_OK|MB_ICONINFORMATION|MB_APPLMODAL);
    }
#endif //BETA_PATCH_UNINSTALL

		free (buffer);
		buffer = NULL;
	}
    return 0;      
}
