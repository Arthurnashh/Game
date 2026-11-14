// PosSetup.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "SetupObject.h"
#include "Setup.h"
#include <El/Stringtable/stringtable.hpp>
#include <stdio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPosSetupApp

BEGIN_MESSAGE_MAP(CPosSetupApp, CWinApp)
	//{{AFX_MSG_MAP(CPosSetupApp)
	ON_UPDATE_COMMAND_UI(ID_HELP, OnUpdateHelp)
	//}}AFX_MSG
	// ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

// NO help
void CPosSetupApp::OnUpdateHelp(CCmdUI* pCmdUI)
{	pCmdUI->Enable(FALSE); };

/////////////////////////////////////////////////////////////////////////////
// CPosSetupApp construction

CPosSetupApp::CPosSetupApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_SetupObject.m_pPosSetupApp = this;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPosSetupApp object

CPosSetupApp theApp;

RString CString2RString(const CString &str, int codePage/*=CP_ACP*/)
{
  return LPCTSTR2RString(str,codePage);
}

RString LPCTSTR2RString(LPCTSTR str, int codePage/*=CP_ACP*/)
{
#ifdef UNICODE
  int size = WideCharToMultiByte(codePage, 0, str, -1, NULL, 0, NULL, NULL);
  RString buf;
  buf.CreateBuffer(size);
  WideCharToMultiByte(codePage, 0, str, -1, buf.MutableData(), size, NULL, NULL);
  return buf;
#else
  return RString(str);
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CPosSetupApp initialization

static void InitStringtable(HINSTANCE hInstance, CSetupObject &setupObject)
{
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

	CWaitCursor	WaitCursor;

	// String table
  FILE *testf = fopen("setup.csv","rb");
  if (testf!=NULL)
  {
    LoadStringtable ("global", ".\\setup.csv");
  }
  else
  {
    int ressize;
    void *setupCSV = FindAndLoadResource(ressize, &setupObject, "SETUP.CSV");
    if (setupCSV)
    {
      TCHAR buff[1];
      DWORD size=GetTempPath(1,buff);
      if (size!=0)
      {
        size++;
        Temp<TCHAR> out; out.Realloc(size);
        if (GetTempPath(size,out)!=0)
        {
          CString stringtableFileName = CString(out) + _T("__localizeSetup.csv");
          FILE *file = _tfopen(stringtableFileName,_T("wb"));
          if (file)
          {
            fwrite(setupCSV, sizeof(char), ressize, file);
            fclose(file); //file is prepared
            LoadStringtable("global", CString2RString(stringtableFileName));
            _tunlink(stringtableFileName); //and delete it
          }
        }
      }
    }
  }
}

BOOL CPosSetupApp::InitInstance()
{
  WNDCLASS wc;
  GetClassInfo(m_hInstance, _T("#32770"), &wc);
  wc.lpszClassName = _T("BIS Setup");
  RegisterClass(&wc);

	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

//	WORD lang = MAKELANGID(LANG_CZECH, SUBLANG_DEFAULT);
//	SetThreadLocale(MAKELCID(lang, SORT_DEFAULT));

  // possibly patching another setup (or debugging new setup using data from other setup)
  m_SetupObject.m_patchingSetup = "";  //default value
  m_SetupObject.m_updaterMode = false; // default is an updater mode
  bool doNotRunMSet = false;

  for (int i=1; i<__argc; i++)
  {
    CString arg = __targv[i];
    if (_tcsicmp(arg, _T("-debug"))==0)
    {
      m_SetupObject.m_debugMode = true;
    }
    else if (_tcsncicmp(arg, _T("-cdkey="), 7)==0) 
    {
      m_SetupObject.m_cdKeyOptions.m_cdKeyFromCommandLine = arg.Mid(7);
      m_SetupObject.m_cdKeyOptions.readFromCommandLine = true;
    }
    else if (_tcsicmp(arg, _T("-runas") )==0) 
    {
      // Setup was restarted with runas (admin rights)
      void SetupIsRunas();
      SetupIsRunas();
    }
    else if (_tcsicmp(arg, _T("-updater"))==0)
    {
      m_SetupObject.m_updaterMode = true;
    }
    else if (_tcsicmp(arg, _T("-doNotRunM"))==0)
    {
      m_SetupObject.m_runAfterUpdateFlag = false;
      doNotRunMSet = true;
    }
    else m_SetupObject.m_patchingSetup = arg;
  }

  if (!doNotRunMSet) m_SetupObject.m_runAfterUpdateFlag = m_SetupObject.m_updaterMode;

#if _CURRENT_DIR_IS_EXE_DIR
    // change working directory to the directory with setup.exe
    TCHAR buffer[_MAX_PATH];
    GetModuleFileName(NULL,buffer,_MAX_PATH);
    CString tmpDir = buffer;
    int index = tmpDir.ReverseFind('\\');
    if (index > 0)
    {
      // drop file name from the path
      tmpDir = tmpDir.Left(index);
      // change directory
      if (_tchdir(tmpDir) != 0)
        return FALSE;
    }
#endif

	// Stringtable must be initialized before first call of LocalizeString
	InitStringtable(m_hInstance, m_SetupObject);
	
	// jméno aplikace
  m_sAppName = (LPCTSTR) (CSetupObject::SetupLocalizeString (_T("SETUP_STR_APP_NAME")));
	m_pszOldAppName = m_pszAppName;
	m_pszAppName	= m_sAppName;

  // instalace
  while (m_SetupObject.DoSetupStep())
  {};

  // if we are in update mode, run after update app
  if (m_SetupObject.m_updaterMode && 
      m_SetupObject.m_runAfterUpdateFlag &&
      m_SetupObject.m_nSetupResult == CSetupObject::stpOK && 
      m_SetupObject.m_sRunAfterUpdate.GetLength() > 0)
  {
    // we are updater so start OA
    m_SetupObject.DoRunSingleApplication(m_SetupObject.m_sSetupPoseidonDir, m_SetupObject.m_sRunAfterUpdate);
  }

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int CPosSetupApp::ExitInstance() 
{
	// vracím pùvodní jméno aplikace
	m_pszAppName	= m_pszOldAppName;
	
	return CWinApp::ExitInstance();
}
