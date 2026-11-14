// texMerge.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "shlobj.h"
#include "texMerge.h"

#include "MainFrm.h"
#include "texMergeDoc.h"
#include "texMergeView.h"

#include <El/ParamFile/paramFile.hpp>//FLY#include "..\lib\paramFile.hpp"
//#include "..\posAccess\posAccess.hpp"
#include <Es/Files/filenames.hpp>

#include "TexMergeCmdLine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#include <class\rstring.cpp>


/////////////////////////////////////////////////////////////////////////////
// CTexMergeApp

BEGIN_MESSAGE_MAP(CTexMergeApp, CWinApp)
	//{{AFX_MSG_MAP(CTexMergeApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_PREF, OnFilePref)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTexMergeApp construction

CTexMergeApp::CTexMergeApp()
{
}

CTexMergeApp::~CTexMergeApp()
{
}

const char CfgFile[]="TexMerge.inf";

void CTexMergeApp::LoadConfig()
{
	ParamFile cfg;
	cfg.Parse(CfgFile);
	if (cfg.FindEntry("textures")) _texturePath=(const char *)RStringB(cfg>>"textures");
	if (cfg.FindEntry("merged")) _mergedPath=(const char *)RStringB(cfg>>"merged");
}

void CTexMergeApp::SaveConfig()
{
	ParamFile cfg;
	cfg.Parse(CfgFile);
	cfg.Add("textures",(const char *)_texturePath);
	cfg.Add("merged",(const char *)_mergedPath);
	cfg.Save(CfgFile);
}

#include "KeyCheck.h"
/////////////////////////////////////////////////////////////////////////////
// The one and only CTexMergeApp object

CTexMergeApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CTexMergeApp initialization


BOOL CTexMergeApp::InitInstance()
{

  KeyCheckClass cdkey;
  if (!cdkey.VerifyKey())
  {
    AfxMessageBox(IDS_INVALID_CD_KEY,MB_OK|MB_ICONSTOP);
    return FALSE;
  }
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.


	// Change the registry key under which our settings are stored.
	// You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Bohemia Interactive Studio"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	// load texture hits config
	RString retString;
	LoadPictureConfigRetVal ret = LoadPictureConfig(retString);
	if (ret!=LoadPictureConfigOK)
	{
		CString msg;
		UINT msgId = IDS_MSG_NOCONFIG;
		char mod[256];
		GetModuleFileName(NULL,mod,sizeof(mod));
		CString modname = GetFilenameExt(mod);
		switch (ret)
		{
			case LoadPictureConfigNoRegistry: msgId = IDS_MSG_NOREGISTRY; break;
			case LoadPictureConfigNoFile: msgId = IDS_MSG_NOCONFIG; break;
			case LoadPictureConfigBadFile: msgId = IDS_MSG_BADCONFIG; break;
			case LoadPictureConfigOldExe: msgId = IDS_MSG_CONFIGOLDAPP; break;
		}


		AfxFormatString2(msg,msgId,(const char *)retString,(const char *)modname);
		AfxMessageBox(msg);
		return FALSE;
	}

	LoadConfig();

	//FLYPosAccessInit();

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CTexMergeDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CTexMergeView));
	AddDocTemplate(pDocTemplate);

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CTexMergeCmdLine cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();
    if (cmdInfo.IsServerRequested())
      m_pMainWnd->PostMessage(TMM_OPENSERVER);
	return TRUE;
}

BOOL CTexMergeApp::ProcessMessageFilter( int code, LPMSG lpMsg )
{
	// MSG
	if( lpMsg->message==WM_USER )
	{
		// lParam is file name of dropped file
		const char *name=(const char *)lpMsg->lParam;

		// file is not document
		CMainFrame *main=(CMainFrame *)m_pMainWnd;
		//CTexMergeDoc *doc=(CTexMergeDoc *)main->GetActiveDocument();
		CTexMergeView *view=(CTexMergeView *)main->GetActiveView();

		// check file extension
		// file can be texture or model
		const char *ext=strrchr(name,'.');
		if( ext )
		{
			if( !strcmpi(ext,".p3d") )
			{
				// file is model
				view->AddModel(name);
			}
			else if( !strcmpi(ext,".pac") || !strcmpi(ext,".paa") || !strcmpi(ext,".tga"))
			{
				// file is texture
				view->AddItem(name);
			}
			else
			{
				Log("Unknown file %s dropped",name);
				// unknown file - ignore
			}
		}
		return TRUE;
	}
	return CWinApp::ProcessMessageFilter(code,lpMsg);
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CTexMergeApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

const char szAppName[]="TexMerge";

/////////////////////////////////////////////////////////////////////////////
// CTexMergeApp commands

BOOL CTexMergeApp::OnIdle(LONG lCount) 
{
	CMainFrame *main=(CMainFrame *)m_pMainWnd;
	CTexMergeDoc *doc=(CTexMergeDoc *)main->GetActiveDocument();
	CTexMergeView *view=(CTexMergeView *)main->GetActiveView();

	main->_setNumber=view->GetActSet();
	main->_setCount=doc->GetTextureSetCount();

	const TextureSet *set=doc->GetTextureSet(view->GetActSet());
	const TextureItem *item=doc->GetItem(view->GetActSet(),view->GetActItem());
	if( set )
	{
		main->_setW=set->_w;
		main->_setH=set->_h;
		main->_setName=set->BuildName(40,' ');
	}
	else
	{
		main->_setW=0;
		main->_setH=0;
		main->_setName="";
	}
	if( item )
	{
		main->_itemX=item->x;
		main->_itemY=item->y;
    main->_itemImportance=item->GetScaledImportance();
		main->_itemW=item->UsedW();
		main->_itemH=item->UsedH();
		main->_textureName=view->GetActTexture();
	}
	else
	{
		main->_itemX=0;
		main->_itemY=0;
		main->_itemW=0;
		main->_itemH=0;
    main->_itemImportance=0;
		main->_textureName="";
	}
	return CWinApp::OnIdle(lCount);
}


static int CALLBACK BrowseCallbackProc
( 
    HWND hwnd,  UINT uMsg,  LPARAM lParam,  LPARAM lpData
)
{
	BROWSEINFO *bInfo=(BROWSEINFO *)lpData;
	switch( uMsg )
	{
		case BFFM_SELCHANGED:
		{
			LPITEMIDLIST idList=(LPITEMIDLIST)lParam;
			char buffer[MAX_PATH];
			if( ::SHGetPathFromIDList(idList, buffer) )
			{
				LPARAM lParam=(LPARAM)buffer;
				::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,lParam);
			}
			else
			{
				LPARAM lParam=(LPARAM)"Please select folder";
				::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,lParam);
			}
		}
		break;
		case BFFM_INITIALIZED:
		{
				LPARAM lParam=(LPARAM)bInfo->pszDisplayName;
				::SendMessage(hwnd,BFFM_SETSELECTION,1,lParam);
		}
		break;
	}
	return 0;
}

bool BrowseForFolder( HWND parent, CString title, CString &path )
{
  BROWSEINFO bi; 
  HRESULT err;
  LPMALLOC shMalloc;
  err=::CoGetMalloc(1,&shMalloc);
  if( err!=S_OK ) return false;
  // Allocate a buffer to receive browse information. 
  LPSTR buffer = (LPSTR) shMalloc->Alloc(MAX_PATH);
  if( !buffer ) return false;

	bool ret=false;
  strcpy(buffer,path);
  // Fill in the BROWSEINFO structure. 
  bi.hwndOwner = NULL;
  bi.pidlRoot = NULL; 
  bi.pszDisplayName = buffer; 
  bi.lpszTitle = title;
  bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_STATUSTEXT; 
  bi.lpfn = BrowseCallbackProc; 
  bi.lParam = (LPARAM)&bi;

  // Browse for a folder and return its PIDL. 
  LPITEMIDLIST browse = ::SHBrowseForFolder(&bi);
  if( browse )
  { 
      // Show the display name, title, and file system path. 
      ::SHGetPathFromIDList(browse, buffer);
      path=buffer;
			ret=true;
      //path=AddFolder(path,"Poseidon");
      shMalloc->Free(browse); 
  } 
  // Clean up. 
  shMalloc->Free(buffer); 
  shMalloc->Release(); // release interface
	return ret;
}

void CTexMergeApp::OnFilePref()
{
	BrowseForFolder(NULL,"Texture Folder",_texturePath);
	SaveConfig();
}
