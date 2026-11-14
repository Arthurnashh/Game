// AutoRunDlg.cpp : implementation file
//

#include "stdafx.h"
#include <El/elementpch.hpp>
#include <El/Stringtable/stringtable.hpp>

#include "io.h"
#include "AutoRun.h"
#include "AutoRunDlg.h"

#include <mmsystem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TEST_TIMER_ID		100
#define TEST_TIMER_TM		1000

#define TRIGGER_BASE_ID		30000
#define BUTTON_BASE_ID		30500

RString LPCTSTR2RString(LPCTSTR str, int codePage=CP_ACP);

/////////////////////////////////////////////////////////////////////////////
// CAutoRunFile

class CAutoRunFile
{
public:
	CString		m_sId;
	bool		m_bUseMainDirectory;
	CString		m_sFileName;
	bool		m_bUseDirectory;
	bool		m_bExitAutoRun;
	bool		m_bExecute;
	bool		m_bIsLink;
  bool    m_useCondition;
  CString m_sCondition;
	static CString m_sFileLanguageExtension;

  bool  m_bMainDirectorySet;
  CString  m_sMainDirectory;

public:
	CAutoRunFile ()
	{
	}

	bool Init (CString &line, bool mainDirectorySet, CString &mainDirectory)
	{
    m_bMainDirectorySet = mainDirectorySet;
    m_sMainDirectory = mainDirectory;

		int len = lstrlen (line);

		switch (line[0])
		{
		case _T ('R'):
		case _T ('r'):
			m_bExitAutoRun = true;
			m_bExecute = true;
			m_bUseDirectory = true;
			m_bIsLink = false;
      m_useCondition = false;
			break;

    case _T ('X'):
    case _T ('x'):
      m_bExitAutoRun = true;
      m_bExecute = true;
      m_bUseDirectory = true;
      m_bIsLink = false;
      m_useCondition = true;
      break;

		case _T ('E'):
		case _T ('e'):
			m_bExitAutoRun = false;
			m_bExecute = true;
			m_bUseDirectory = true;
			m_bIsLink = false;
      m_useCondition = false;
			break;

		case _T ('O'):
		case _T ('o'):
			m_bExitAutoRun = false;
			m_bExecute = false;
			m_bUseDirectory = true;
			m_bIsLink = false;
      m_useCondition = false;
			break;

		case _T ('L'):
		case _T ('l'):
			m_bExitAutoRun = false;
			m_bExecute = false;
			m_bUseDirectory = false;
			m_bIsLink = true;
      m_useCondition = false;
			break;

		default:
			return false;
		}

		int idx = line.Find (':', 2);
		if (idx < 0)
		{
			return false;
		}
		m_sId = line.Mid (2, idx - 2);
		++idx;

		if (m_bUseDirectory)
		{
			switch (line[idx])
			{
			case _T ('M'):
			case _T ('m'):
				m_bUseMainDirectory = true;
				m_bUseDirectory = true;
				break;
				
			case _T ('A'):
			case _T ('a'):
				m_bUseMainDirectory = false;
				m_bUseDirectory = true;
				break;
				
			case _T ('-'):
				m_bUseDirectory = false;
				break;
				
			default:
				return false;
			}
			idx += 2;
		}

		if (idx >= len)
		{
			return false;
		}

    if (m_useCondition)
    {
		  int ix = idx;
      idx = line.Find (':', idx);
      if (idx<0) return false;
      m_sCondition = line.Mid(ix, idx-ix);
      idx++;
    }
    m_sFileName = line.Mid (idx);

		return true;
	}

	bool GetFullPath (CString *fullPath, CString *dir, CAutoRunDlg *dlg, bool bMainDirectorySet, CString &sMainDirectory)
	{
		if (m_sFileName.Find (_T ("%L")) >= 0)
		{
			CString fp1;
			CString d1;
			CString tmpName = m_sFileName;
			tmpName.Replace (_T ("%L"), m_sFileLanguageExtension);
			if (GetFullPath (&fp1, &d1, dlg, m_bUseDirectory, m_bUseMainDirectory, tmpName ,bMainDirectorySet,sMainDirectory))
			{
				if (_taccess (fp1, 00) == 0)
				{
					if (fullPath != NULL)
					{
						*fullPath = fp1;
					}
					if (dir != NULL)
					{
						*dir = d1;
					}
					return true;
				}
				CString fp2;
				CString d2;
				tmpName = m_sFileName;
				tmpName.Replace (_T ("%L"), _T (""));
				if (GetFullPath (&fp2, &d2, dlg, m_bUseDirectory, m_bUseMainDirectory, tmpName ,bMainDirectorySet,sMainDirectory))
				{
					if (_taccess (fp2, 00) == 0)
					{
						if (fullPath != NULL)
						{
							*fullPath = fp2;
						}
						if (dir != NULL)
						{
							*dir = d2;
						}
						return true;
					}
				}
				if (fullPath != NULL)
				{
					*fullPath = fp1;
				}
				if (dir != NULL)
				{
					*dir = d1;
				}
				return true;
			}

			return false;
		}
		else
		{
			return GetFullPath (fullPath, dir, dlg, m_bUseDirectory, m_bUseMainDirectory,	m_sFileName, bMainDirectorySet,sMainDirectory);
		}
	}

	static bool GetFullPath (CString *fullPath, CString *dir, CAutoRunDlg *dlg, 
		bool useDirectory, bool useMainDirectory, CString &fileName, bool bMainDirectorySet, CString &sMainDirectory)
	{
		if (useDirectory)
		{
			if (useMainDirectory)
			{
				if (bMainDirectorySet)
				{
					if (dir != NULL)
					{
						*dir = sMainDirectory;
					}
					if (fullPath != NULL)
					{
						*fullPath = sMainDirectory + fileName;
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				if (dir != NULL)
				{
					*dir = dlg->m_sAutoRunDirectory;
				}
				if (fullPath != NULL)
				{
					*fullPath = dlg->m_sAutoRunDirectory + fileName;
				}
			}
		}
		else
		{
			if (dir != NULL)
			{
				*dir = dlg->m_sAutoRunDirectory;
			}
			if (fullPath != NULL)
			{
				*fullPath = fileName;
			}
		}

		return true;
	}

	void Run (CAutoRunDlg *dlg)
	{
		CString wholeName;
    CString dir;

    CString fileNameBackup = m_sFileName;
    CString runArgs = _T("");
    if (m_useCondition)
    {
      int ix = m_sFileName.Find(_T(' '));
      if (ix>0) 
      {
        runArgs = m_sFileName.Mid(ix+1);
        m_sFileName = m_sFileName.Mid(0,ix);
      }
    }
    if (m_bUseDirectory)
		{
			CString fullPath;
			if (!GetFullPath (&wholeName, &dir, dlg, m_bMainDirectorySet, m_sMainDirectory))
			{
				return;
			}
      // execute exe in his own folder
      int pos = m_sFileName.ReverseFind(_T('\\'));
      if (pos)
      {
        dir = dir + m_sFileName.Mid(0,pos+1);
      }
			SetCurrentDirectory (dir);
		}
		else
		{
			wholeName = m_sFileName;
		}
    m_sFileName = fileNameBackup;

		if (m_bExecute)
		{
      STARTUPINFO stInfo;
      memset (&stInfo, 0, sizeof (stInfo));
      stInfo.cb = sizeof(stInfo);
      PROCESS_INFORMATION prInfo;
      memset (&prInfo, 0, sizeof (prInfo));

      CString useCommandLine = CString(_T("\"")) + wholeName + CString(_T("\" ")) + runArgs;
      ::CreateProcess (wholeName, useCommandLine.GetBuffer(), NULL, NULL,
        FALSE, CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS,
        NULL, dir, &stInfo, &prInfo);
			//WinExec (LPCTSTR2RString(wholeName), SW_SHOW);
		}
		else
		{
			ShellExecute (NULL, _T("open"), wholeName, _T(""), NULL, SW_SHOWNORMAL);
		}

		if (m_bExitAutoRun)
		{
			dlg->EndDialog (IDCANCEL);
		}
	}

	virtual bool Exists (CAutoRunDlg *dlg)
	{
		CString fullPath;

		if (m_bIsLink)
		{
			return true;
		}
    else if (m_useCondition)
    {
      if (GetFullPath (&fullPath, NULL, dlg, m_bUseDirectory, m_bUseMainDirectory, m_sCondition, m_bMainDirectorySet, m_sMainDirectory))
      {
        if (_taccess (fullPath, 00) == 0)
        {
          return true;
        }
      }
    }
    else if (GetFullPath (&fullPath, NULL, dlg, m_bMainDirectorySet, m_sMainDirectory))
		{
			if (_taccess (fullPath, 00) == 0)
			{
				return true;
			}
		}

		return false;
	}
};

CString CAutoRunFile::m_sFileLanguageExtension;

/////////////////////////////////////////////////////////////////////////////
// CAutoRunButton

class CAutoRunButton
{
public:
	RECT			m_cRect;
	CString			m_sText;
	CAutoRunFile	*m_pFile;
	CButton			*m_pButton;
	CAutoRunDlg		*m_pDialog;

public:
	CAutoRunButton ()
		: m_pButton (NULL), m_pDialog (NULL)
	{
	}

	~CAutoRunButton ()
	{
		if (m_pButton != NULL)
		{
			delete m_pButton;
			m_pButton = NULL;
		}
	}

	virtual bool Init (CString &line, CAutoRunDlg *dlg, int butId)
	{
		m_pDialog = dlg;

		int findLeft = line.Find (':', 2);
		int findTop = line.Find (':', findLeft + 1);
		int findRight = line.Find (':', findTop + 1);
		int findBottom = line.Find (':', findRight + 1);
		int findText = line.Find (':', findBottom + 1);

// 1 more condition for REINFORECEMENTS Setup: show this button only if this reg key exist
    int findRegPath = line.Find (':', findText + 1);
    int findRegName = line.Find (':', findRegPath + 1);

    if (findRegPath > 0 && findRegName > 0)
    {
      CString regPath = line.Mid (findRegPath + 1,findRegName - (findRegPath + 1));
      CString regName = line.Mid (findRegName + 1);

      HKEY key;
      bool ok = (RegOpenKey (HKEY_LOCAL_MACHINE, regPath, &key) == ERROR_SUCCESS);
      if (!ok) 
      {
        RegCloseKey (key);
        return true;
      }

      ok = (RegQueryValueEx (key, regName, 0, NULL, NULL, NULL) == ERROR_SUCCESS);
      RegCloseKey (key);

      if (!ok) return true;
    }
    

		if (findLeft <= 0 || findTop <= 0 || findRight <= 0 || 
			findBottom <= 0 || findText <= 0)
		{
			return false;
		}

		m_cRect.left = _tcstol (line.Mid (2, findLeft - 2), NULL, 0);
		m_cRect.top = _tcstol (line.Mid (findLeft + 1, findTop - (findLeft + 1)), NULL, 0);
		m_cRect.right = _tcstol (line.Mid (findTop + 1, findRight - (findTop + 1)), NULL, 0);
		m_cRect.bottom = _tcstol (line.Mid (findRight + 1, findBottom - (findRight + 1)), NULL, 0);
		m_sText = line.Mid (findBottom + 1, findText - (findBottom + 1));

    CString id;
    if (findRegPath==-1)
      id= line.Mid (findText + 1);
    else
      id= line.Mid (findText + 1,findRegPath - (findText + 1));

		m_pFile = m_pDialog->FindAutoRunFile (id);
		if (m_pFile == NULL)
		{
			return false;
		}

		m_pButton = new CButton ();
		if (m_pButton == NULL)
		{
			return false;
		}
		if (!m_pButton->Create (m_sText, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP, m_cRect, m_pDialog, butId))
		{
			return false;
		}

		m_pButton->SetFont (dlg->GetFont ());

		UpdateState ();

		return true;
	}

	virtual void UpdateState ()
	{
		m_pButton->EnableWindow (m_pFile->Exists (m_pDialog) ? TRUE : FALSE);
	}

	virtual void Push ()
	{
		m_pFile->Run (m_pDialog);
	}
};

class CAutoRunTrigger : public CAutoRunButton
{
public:
	enum
	{
		trueFileExists,
		falseFileExists,
		noneFileExists,
	};

	CString			m_sFalseText;
	CAutoRunFile	*m_pTestFile;
	CAutoRunFile	*m_pFalseFile;
	int				m_iState;

public:
	virtual bool Init (CString &line, CAutoRunDlg *dlg, int butId)
	{
		m_pDialog = dlg;

		int findLeft = line.Find (':', 2);
		int findTop = line.Find (':', findLeft + 1);
		int findRight = line.Find (':', findTop + 1);
		int findBottom = line.Find (':', findRight + 1);
		int findTestId = line.Find (':', findBottom + 1);
		int findTrueText = line.Find (':', findTestId + 1);
		int findTrueId = line.Find (':', findTrueText + 1);
		int findFalseText = line.Find (':', findTrueId + 1);
		
		if (findLeft <= 0 || findTop <= 0 || findRight <= 0 || 
			findBottom <= 0 || findTrueText <= 0 || findTrueId <= 0 ||
			findFalseText <= 0)
		{
			return false;
		}

		m_cRect.left = _tcstol (line.Mid (2, findLeft - 2), NULL, 0);
		m_cRect.top = _tcstol (line.Mid (findLeft + 1, findTop - (findLeft + 1)), NULL, 0);
		m_cRect.right = _tcstol (line.Mid (findTop + 1, findRight - (findTop + 1)), NULL, 0);
		m_cRect.bottom = _tcstol (line.Mid (findRight + 1, findBottom - (findRight + 1)), NULL, 0);
		CString id = line.Mid (findBottom + 1, findTestId - (findBottom + 1));
		m_pTestFile = dlg->FindAutoRunFile (id);
		if (m_pTestFile == NULL)
		{
			return false;
		}
		m_sText = line.Mid (findTestId + 1, findTrueText - (findTestId + 1));
		id = line.Mid (findTrueText + 1, findTrueId - (findTrueText + 1));
		m_pFile = dlg->FindAutoRunFile (id);
		if (m_pFile == NULL)
		{
			return false;
		}
		m_sFalseText = line.Mid (findTrueId + 1, findFalseText - (findTrueId + 1));
		id = line.Mid (findFalseText + 1);
		m_pFalseFile = dlg->FindAutoRunFile (id);
		if (m_pFalseFile == NULL)
		{
			return false;
		}

		m_pButton = new CButton ();
		if (m_pButton == NULL)
		{
			return false;
		}
		if (!m_pButton->Create (m_sText, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP, m_cRect, dlg, butId))
		{
			return false;
		}

		m_pButton->SetFont (dlg->GetFont ());

		UpdateState ();

		return true;
	}

	virtual void UpdateState ()
	{
		if (m_pTestFile->Exists (m_pDialog) && m_pFile->Exists (m_pDialog))
		{
			m_pButton->SetWindowText (m_sText);
			CAutoRunDlg::ApplyApplicationNameAndStringTable (m_pButton);
			m_pButton->EnableWindow (TRUE);
			m_iState = trueFileExists;
			return;
		}

		if (m_pFalseFile->Exists (m_pDialog))
		{
			m_pButton->SetWindowText (m_sFalseText);
			CAutoRunDlg::ApplyApplicationNameAndStringTable (m_pButton);
			m_pButton->EnableWindow (TRUE);
			m_iState = falseFileExists;
			return;
		}

		m_pButton->SetWindowText (m_sFalseText);
		CAutoRunDlg::ApplyApplicationNameAndStringTable (m_pButton);
		m_pButton->EnableWindow (FALSE);
		m_iState = noneFileExists;
	}

	virtual void Push ()
	{
		switch (m_iState)
		{
		case trueFileExists:
			m_pFile->Run (m_pDialog);
			break;

		case falseFileExists:
			m_pFalseFile->Run (m_pDialog);
			break;

		case noneFileExists:
			break;
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
// CAutoRunDlg dialog


CString CAutoRunDlg::m_sApplicationName;


CAutoRunDlg::CAutoRunDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAutoRunDlg::IDD, pParent), m_bMainDirectorySet (false), m_hBitmap (NULL)
{
	//{{AFX_DATA_INIT(CAutoRunDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bSetupIsRunning = FALSE;
}

CAutoRunDlg::~CAutoRunDlg ()
{
	for (int i = 0; i < m_arAutoRunFiles.GetSize (); ++i)
	{
		CAutoRunFile *f = (CAutoRunFile*) m_arAutoRunFiles[i];
		delete f;
	}
	m_arAutoRunFiles.RemoveAll ();

	for (int i = 0; i < m_arButtons.GetSize (); ++i)
	{
		CAutoRunButton *f = (CAutoRunButton*) m_arButtons[i];
		delete f;
	}
	m_arButtons.RemoveAll ();

	for (int i = 0; i < m_arTriggers.GetSize (); ++i)
	{
		CAutoRunTrigger *f = (CAutoRunTrigger*) m_arTriggers[i];
		delete f;
	}
	m_arTriggers.RemoveAll ();
}

void CAutoRunDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutoRunDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAutoRunDlg, CDialog)
	//{{AFX_MSG_MAP(CAutoRunDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CAutoRunDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	KillTimer(TEST_TIMER_ID);
}

/////////////////////////////////////////////////////////////////////////////
// CAutoRunDlg message handlers

BOOL CAutoRunDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	switch (PRIMARYLANGID (GetUserDefaultLangID ()))
	{
	case LANG_FRENCH:
		CAutoRunFile::m_sFileLanguageExtension = ".french";
		GLanguage = "French";
		break;
	case LANG_SPANISH:
		CAutoRunFile::m_sFileLanguageExtension = ".spanish";
		GLanguage = "Spanish";
		break;
	case LANG_ITALIAN:
		CAutoRunFile::m_sFileLanguageExtension = ".italian";
		GLanguage = "Italian";
		break;
	case LANG_GERMAN:
		CAutoRunFile::m_sFileLanguageExtension = ".german";
		GLanguage = "German";
		break;
	case LANG_POLISH:
		CAutoRunFile::m_sFileLanguageExtension = ".polish";
		GLanguage = "Polish";
		break;
	case LANG_CZECH:
		CAutoRunFile::m_sFileLanguageExtension = ".czech";
		GLanguage = "Czech";
		break;
	case LANG_RUSSIAN:
		CAutoRunFile::m_sFileLanguageExtension = ".russian";
		GLanguage = "Russian";
		break;
	case LANG_HUNGARIAN:
		CAutoRunFile::m_sFileLanguageExtension = ".hungarian";
		GLanguage = "Hungarian";
		break;
	default:
		CAutoRunFile::m_sFileLanguageExtension = "";
		GLanguage = "English";
		break;
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Set autorun file path and file name
	LPTSTR str= m_sAutoRunDirectory.GetBuffer (_MAX_PATH);
	DWORD len = GetModuleFileName (AfxGetApp()->m_hInstance, str, _MAX_PATH);
	m_sAutoRunDirectory.ReleaseBuffer ();
	m_sAutoRunFileName = m_sAutoRunDirectory;
	m_sAutoRunDirectory.MakeReverse ();
	int i = m_sAutoRunDirectory.FindOneOf (_T ("\\/"));
	if (i < 0)
	{
		return FALSE;
	}
	m_sAutoRunDirectory.MakeReverse ();
	i = len - i;
	m_sAutoRunDirectory = m_sAutoRunDirectory.Left (i);
	i = m_sAutoRunFileName.ReverseFind ('.');
	if (i >= 0)
	{
		m_sAutoRunFileName = m_sAutoRunFileName.Left (i);
	}

	// Load string table
	LoadStringtable ("global", LPCTSTR2RString(m_sAutoRunFileName + _T (".csv")));

	// Load *.dat file
	CStringArray autorunDatLines;
	LoadAutoRunDatFile (autorunDatLines);
	ProcessAutoRunDatLines (autorunDatLines);

	// Update trigger states
	UpdateTriggerStates ();

	// BMP file
	CString sFileBmp = m_sAutoRunFileName + _T (".bmp");
	m_hBitmap = (HBITMAP) LoadImage (NULL, sFileBmp, IMAGE_BITMAP, m_iImageWidth, m_iImageHeight, LR_LOADFROMFILE);

	// resize bitmap
	RECT wRect;
	RECT cRect;
	if (m_hBitmap != NULL)
	{
		CStatic *pict = (CStatic*) GetDlgItem (IDC_AUTORUN_PICTURE);
		if (pict == NULL)
		{
			return FALSE;
		}
		wRect.left = m_iImageHorSpace;
		wRect.top = m_iImageVerSpace;
		wRect.right = m_iImageHorSpace + m_iImageWidth;
		wRect.bottom = m_iImageVerSpace + m_iImageHeight;
		pict->SetBitmap (m_hBitmap);
		pict->MoveWindow(&wRect);
	}

	// resize quit button
	CWnd *ctrl = GetDlgItem (IDCANCEL);
	if (ctrl == NULL)
	{
		return FALSE;
	}
	ctrl->MoveWindow (&m_cQuitButtonRect);
	ctrl->SetWindowText (m_sQuitButtonText);

	// resize window
	this->GetWindowRect (&wRect);
	this->GetClientRect (&cRect);
	ClientToScreen (&cRect);
	cRect.right = cRect.left + m_iImageWidth + 2 * m_iImageHorSpace;
	cRect.bottom = cRect.top + m_iImageHeight + 2 * m_iImageVerSpace;
	this->CalcWindowRect (&cRect);
	cRect.left += wRect.left;
	cRect.right += wRect.left;
	cRect.top += wRect.top;
	cRect.bottom += wRect.top;
	this->MoveWindow (&cRect);

	ApplyApplicationNameAndStringTable (this);

	// WAV file
	CString sFileWav = m_sAutoRunFileName + _T (".wav");
	PlaySound (sFileWav, NULL, SND_FILENAME | SND_ASYNC);

	// timer pro testování
	SetTimer(TEST_TIMER_ID,TEST_TIMER_TM,NULL);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAutoRunDlg::GetMainDirectoryFromRegistry ()
{
	HKEY key;
	m_bMainDirectorySet = false;
	if (RegOpenKey (HKEY_LOCAL_MACHINE, m_sMainDirectoryRegKeyPath, &key) == ERROR_SUCCESS)
	{
		LPTSTR str = m_sMainDirectory.GetBuffer (_MAX_PATH);
		DWORD len = _MAX_PATH;
		if (RegQueryValueEx (key, m_sMainDirectoryRegKeyName, 0, NULL, (BYTE*) str, &len) != ERROR_SUCCESS)
		{
			m_sMainDirectory.ReleaseBuffer (0);
			RegCloseKey (key);
			return;
		}

		m_sMainDirectory.ReleaseBuffer ();
		len = (DWORD) m_sMainDirectory.GetLength ();
		if (m_sMainDirectory[(int) len - 1] != _T ('\\') &&
			m_sMainDirectory[(int) len - 1] != _T ('/'))
		{
			m_sMainDirectory += '\\';
		}
		m_bMainDirectorySet = true;
		RegCloseKey (key);
	}
}

BOOL CAutoRunDlg::ProcessAutoRunDatLines (CStringArray &autorunDatLines)
{
	for (int i = 0; i < autorunDatLines.GetSize (); ++i)
	{
		ApplyApplicationNameAndStringTable (autorunDatLines[i], false);
	}

	for (int i = 0; i < autorunDatLines.GetSize (); ++i)
	{
		CString line = autorunDatLines[i];
		switch (line[0])
		{
    case _T ('/'): //this line is comment (but it is recommended to use double //
    case _T (';'): //this line is comment
      break;
		case _T ('A'):
		case _T ('a'):
			m_sApplicationName = line.Mid (2);
			break;

		case _T ('D'):
		case _T ('d'):
			{
				int f = line.Find (':', 2);
				if (f < 0)
				{
					return FALSE;
				}

				m_sMainDirectoryRegKeyPath = line.Mid (2, f - 2);
				m_sMainDirectoryRegKeyName = line.Mid (f + 1);
				GetMainDirectoryFromRegistry ();
			}
			break;

		case _T ('I'):
		case _T ('i'):
			{
				int fWidth = line.Find (':', 2);
				int fHeight = line.Find (':', fWidth + 1);
				int fHSpace = line.Find (':', fHeight + 1);

				if (fWidth < 0 || fHeight < 0 || fHSpace < 0)
				{
					return FALSE;
				}

				m_iImageWidth = _tcstol (line.Mid (2, fWidth - 2), NULL, 0);
				m_iImageHeight = _tcstol (line.Mid (fWidth + 1, fHeight - (fWidth + 1)), NULL, 0);
				m_iImageHorSpace = _tcstol (line.Mid (fHeight + 1, fHSpace - (fHeight + 1)), NULL, 0);
				m_iImageVerSpace = _tcstol (line.Mid (fHSpace + 1), NULL, 0);
			}
			break;

    case _T ('R'):
    case _T ('r'):
    case _T ('X'):
    case _T ('x'):
		case _T ('E'):
		case _T ('e'):
		case _T ('O'):
		case _T ('o'):
		case _T ('L'):
		case _T ('l'):
			{
				CAutoRunFile *f = new CAutoRunFile ();
				if (f == NULL)
				{
					return FALSE;
				}

				if (!f->Init (line,m_bMainDirectorySet,m_sMainDirectory))
				{
					delete f;
					return FALSE;
				}

				m_arAutoRunFiles.Add (f);
			}
			break;

		case _T ('B'):
		case _T ('b'):
			{
				CAutoRunButton *but = new CAutoRunButton ();
				if (but == NULL)
				{
					return FALSE;
				}

				if (!but->Init (line, this, BUTTON_BASE_ID + m_arButtons.GetSize ()))
				{
					delete but;
					return FALSE;
				}

				m_arButtons.Add (but);
			}
			break;

		case _T ('T'):
		case _T ('t'):
			{
				CAutoRunTrigger *but = new CAutoRunTrigger ();
				if (but == NULL)
				{
					return FALSE;
				}

				if (!but->Init (line, this, TRIGGER_BASE_ID + m_arTriggers.GetSize ()))
				{
					delete but;
					return FALSE;
				}

				m_arTriggers.Add (but);
			}
			break;

		case _T ('Q'):
		case _T ('q'):
			{
				int fLeft = line.Find (':', 2);
				int fTop = line.Find (':', fLeft + 1);
				int fRight = line.Find (':', fTop + 1);
				int fBottom = line.Find (':', fRight + 1);

				if (fLeft < 0 || fTop < 0 || fRight < 0 || fBottom < 0)
				{
					return FALSE;
				}

				m_cQuitButtonRect.left = _tcstol (line.Mid (2, fLeft - 2), NULL, 0);
				m_cQuitButtonRect.top = _tcstol (line.Mid (fLeft + 1, fTop - (fLeft + 1)), NULL, 0);
				m_cQuitButtonRect.right = _tcstol (line.Mid (fTop + 1, fRight - (fTop + 1)), NULL, 0);
				m_cQuitButtonRect.bottom = _tcstol (line.Mid (fRight + 1, fBottom - (fRight + 1)), NULL, 0);
				m_sQuitButtonText = line.Mid (fBottom + 1);
			}
			break;
		}
	}

	return TRUE;
}

CString UTF8ToCString(LPCSTR buff)
{
#if UNICODE
  Temp<wchar_t> bufW;
  {
    int wSize = MultiByteToWideChar(CP_UTF8, 0, buff, -1, NULL, 0);
    bufW.Realloc(wSize);
    MultiByteToWideChar(CP_UTF8, 0, buff, -1, bufW.Data(), wSize);
  }
  return CString(bufW);
#else
  return CString(buff);
#endif
}

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

#include "El/QStream/qStream.hpp"
BOOL CAutoRunDlg::LoadAutoRunDatFile (CStringArray &autorunDatLines)
{
	CFile file;
	CString fname = m_sAutoRunFileName + _T (".dat");
  int size = QIFileFunctions::GetFileSize(LPCTSTR2RString(fname));
  if (!file.Open (fname, CFile::modeRead, NULL))
	{
		return FALSE;
	}
  Temp<char> datFile; 
  datFile.Realloc(size);
  file.Read(datFile,size);
  file.Close();
  int ix=SkipUnicodePrefix(datFile);
  int  nIndx = 0;
  char buff[1024];
  while(ix<size)
  {
    buff[nIndx]=datFile[ix++];
    if ((buff[nIndx] == '\n')&&(buff[nIndx-1] == '\r'))
    {
      buff[nIndx-1] = 0;
      nIndx = 0;
      // uchovám rádku
      if (strlen(buff) > 0)
        autorunDatLines.Add(UTF8ToCString(buff));
    } else
      nIndx++;
  }
  if (nIndx>0) //we should add the last line too
  {
    buff[nIndx]=0;
    if (strlen(buff) > 0)
      autorunDatLines.Add(UTF8ToCString(buff));
  }

	return TRUE;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAutoRunDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAutoRunDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

/////////////////////////////////////////////////////////
// testování stavu

static BOOL bInTimer = FALSE;
void CAutoRunDlg::OnTimer(UINT nIDEvent) 
{

	if (nIDEvent == TEST_TIMER_ID)
	{
		if (bInTimer)
			return;
		bInTimer = TRUE;
		// testuji konec aplikace
		if (m_bSetupIsRunning)
		{
			 DWORD dwExitCode;
			if (GetExitCodeProcess(m_prInfo.hProcess,&dwExitCode))
			{
				if (dwExitCode == STILL_ACTIVE)
				{
					bInTimer = FALSE;
					return;
				}
			};
			m_bSetupIsRunning = FALSE;
			::ShowWindow(AfxGetMainWnd()->m_hWnd,SW_SHOW);
		}

		// testuji INSTALL
		UpdateTriggerStates ();

		bInTimer = FALSE;
	} else
	CDialog::OnTimer(nIDEvent);
}


/////////////////////////////////////////////////////////
// realizace akcí

CAutoRunFile *CAutoRunDlg::FindAutoRunFile (CString &name)
{
	for (int i = 0; i < m_arAutoRunFiles.GetSize (); ++i)
	{
		CAutoRunFile *f = (CAutoRunFile*) m_arAutoRunFiles[i];
		if (name != f->m_sId)
		{
			continue;
		}

		return f;
	}

	return NULL;
}

void CAutoRunDlg::UpdateTriggerStates ()
{
	for (int i = 0; i < m_arTriggers.GetSize (); ++i)
	{
		((CAutoRunTrigger*) m_arTriggers[i])->UpdateState ();
	}
}

BOOL CAutoRunDlg::OnCommand (WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	if ((wParam >> 16) == BN_CLICKED)
	{
		int id = wParam & 0xffff;
		if ((id >= BUTTON_BASE_ID) && (id < BUTTON_BASE_ID + m_arButtons.GetSize ()))
		{
			((CAutoRunButton*) m_arButtons[id - BUTTON_BASE_ID])->Push ();
			return TRUE;
		}
		else if ((id >= TRIGGER_BASE_ID) && (id < TRIGGER_BASE_ID + m_arTriggers.GetSize ()))
		{
			((CAutoRunTrigger*) m_arTriggers[id - TRIGGER_BASE_ID])->Push ();
			return TRUE;
		}
	}

	return CDialog::OnCommand(wParam, lParam);
}

void CAutoRunDlg::ApplyApplicationNameAndStringTable (CWnd *item)
{
	CString text;
	item->GetWindowText (text);

	if (ApplyApplicationNameAndStringTable (text, true))
	{
		item->SetWindowText (text);
	}

	CWnd *subItem = item->GetTopWindow ();
	while (subItem != NULL)
	{
		ApplyApplicationNameAndStringTable (subItem);
		subItem = subItem->GetNextWindow (GW_HWNDNEXT);
	}
}

CString AutoRunLocalizeString(RString str)
{
  RString locStrUtf8 = LocalizeString(str);
#if UNICODE
  Temp<TCHAR> tbuf;
  int tsize = MultiByteToWideChar(CP_UTF8, 0, locStrUtf8, -1, NULL, 0);
  tbuf.Realloc(tsize);
  MultiByteToWideChar(CP_UTF8, 0, locStrUtf8, -1, tbuf, tsize);
  return CString(tbuf);
#else
  return CString(locStrUtf8); //in this case it is CP_ACP
#endif
}

bool CAutoRunDlg::ApplyApplicationNameAndStringTable (CString &text, bool applyApplName)
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
						CString(AutoRunLocalizeString (LPCTSTR2RString(text.Mid (f + 1, a - f - 1)))) + 
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

int CAutoRunDlg::SetupFindOneOf (CString &where, LPCTSTR charSet, int start)
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
