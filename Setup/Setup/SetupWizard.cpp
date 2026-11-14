#include "stdafx.h"
#include <WinCrypt.h>
#include "Winhttp.h"
#include "shlobj.h"
#include "SetupObject.h"
#include "Setup.h"

#include <El/stringtable/stringtable.hpp>
#undef Fail

#include <mmsystem.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define TIMER_COPY_ID	100
//#define TIMER_COPY_DEL	500
#define TIMER_COPY_DEL	100
#define TIMER_GET_CDKEY_FROM_WEB 100


extern CString UTF8ToCString(LPCSTR buff);

BOOL UserCancelSetup(CSetupObject *setupObj)
{
	if (setupObj->SetupMessageBox(CSetupObject::SetupLocalizeString (_T("SETUP_STR_CANCEL_SETUP_VERIFY")),MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION)==IDYES)
		return TRUE;
	return FALSE;
};

/////////////////////////////////////////////////////////////////////////////
// CWizardPageCDKEY dialog

class CWizardPageCDKEY : public CPropertyPage
{
	DECLARE_DYNCREATE(CWizardPageCDKEY)

  bool _readFromWeb;
  // { CONNECT TO THE SERVER
  HINTERNET session, connect, request;

// Construction
public:
	CWizardPageCDKEY();
	~CWizardPageCDKEY();

  void SetReadFromWeb() {_readFromWeb=true;}

// Dialog Data
	CSetupObject*	m_pSetup;

	//{{AFX_DATA(CWizardPageCDKEY)
	enum { IDD = IDD_WIZARD_PAGE_CDKEY };
	CString	m_cdkey1;
	CString	m_cdkey2;
	CString	m_cdkey3;
	CString	m_cdkey4;
	CString	m_cdkey5;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizardPageCDKEY)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizardPageCDKEY)
	afx_msg void OnChangeCdkey1();
	afx_msg void OnChangeCdkey2();
	afx_msg void OnChangeCdkey3();
	afx_msg void OnChangeCdkey4();
	afx_msg void OnChangeCdkey5();
  afx_msg void OnTimer(UINT nIDEvent);
  afx_msg void OnDestroy();
	//}}AFX_MSG
  void CheckAllCDKeysToAllowNextButton(); 


	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CWizardPageDir dialog

class CWizardPageDir : public CPropertyPage
{
// Construction
public:
	CWizardPageDir();
	~CWizardPageDir();

// Dialog Data
	CSetupObject*	m_pSetup;

	//{{AFX_DATA(CWizardPageDir)
	enum { IDD = IDD_WIZARD_PAGE_DIR };
//	CAnimateCtrl	m_cSetupAnim;
	CEdit	m_cEditDirCtrl;
	CString	m_sDirectory;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizardPageDir)
	public:
	virtual LRESULT OnWizardNext();
  virtual BOOL OnSetActive();
  
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizardPageDir)
	afx_msg void OnChangeDir();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  void UpdateSpaceAvailable();

  bool CheckTargetDirectory();

public:
  CString m_sDirSpaceAvailable;
};

CWizardPageDir::CWizardPageDir() : CPropertyPage(CWizardPageDir::IDD)
, m_sDirSpaceAvailable(_T(""))
{
	//{{AFX_DATA_INIT(CWizardPageDir)
	
	//}}AFX_DATA_INIT
}

CWizardPageDir::~CWizardPageDir()
{
}

static CString GetDirectoryDrive( CString dir )
{	// DriveName
  int ix = dir.Find(_T('\\'),0);
  if (ix<0) return dir;
  return dir.Mid(0,ix+1);
}

BOOL CWizardPageDir::OnSetActive()
{
  UpdateSpaceAvailable();
  return CPropertyPage::OnSetActive();
}

void CWizardPageDir::UpdateSpaceAvailable()
{
  RString availableSpaceFmt = LocalizeString ("SETUP_WIZARD_DIR_SPACE_AVAILABLE");
  ULARGE_INTEGER totalNumberOfBytes, totalNumberOfFreeBytes, freeBytesAvailable;
  float gigaByte = 1024.0f * 1024.0f * 1024.0f;
  CString driveName = GetDirectoryDrive(m_sDirectory);
  BOOL retval = GetDiskFreeSpaceEx(driveName, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes);
  if (!retval)
  {
    LPTSTR lpMsgBuf;
    DWORD dw = GetLastError(); 
    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &lpMsgBuf,
      0, NULL );
    m_sDirSpaceAvailable = lpMsgBuf;
  }
  else
  {
    float sizeNeed = (m_pSetup->m_TotalCopySize * 1.1 + 10 * 1024 * 1024) / gigaByte;
    float sizeAvailable = (double)freeBytesAvailable.QuadPart / gigaByte;
    RString dirSpace = Format(availableSpaceFmt, sizeAvailable, sizeNeed);
    m_sDirSpaceAvailable = UTF8ToCString(dirSpace);
  }
}

bool CWizardPageDir::CheckTargetDirectory()
{
  // empty string -> no check
  if (m_pSetup->m_sTargetFileCheck.GetLength()== 0)
    return true;

  // build path to the desired file
  CString path = m_sDirectory + "/" + m_pSetup->m_sTargetFileCheck;

  WIN32_FIND_DATA data;
  HANDLE handle;

  // start looking for the file
  handle = FindFirstFile(path, &data);

  // check if the handle is correct -> file found
  if (handle != INVALID_HANDLE_VALUE) 
  {
    FindClose(handle);
    return true;
  }

  // problem with file search, show message if search failed for different reason than "file not found"
  LPTSTR lpMsgBuf;
  DWORD dw = GetLastError(); 

  if (dw != ERROR_FILE_NOT_FOUND)
  {
    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &lpMsgBuf,
      0, NULL );
  }

  // anyway, search failed, return false (not found).
  return false;
}

BOOL CWizardPageDir::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// init adresáøe	
	CString sText; 
	CSetupObject::MakeShortDirStr(sText,m_pSetup->m_sSetupPoseidonDir);
	m_sDirectory = sText;
  // init dirSpaceAvailable
  UpdateSpaceAvailable();
  UpdateData(FALSE);

	// resize bitmap
	RECT rect;
	CWnd *ctrl = GetDlgItem(IDC_SETUP_TITLE_POS);
	ctrl->GetWindowRect(&rect);
	ScreenToClient(&rect);
  if (!m_pSetup->temporaryFolderActive) m_pSetup->PrepareResourceData();
  //if the image was in resource, it is located in temporary directory now
  CString tempDir = GetTempDirectory(m_pSetup->m_sApplicationName);
  CString bitmapName = tempDir + _T("\\setup.bmp");
  HBITMAP bitmap = (HBITMAP)LoadImage(
    NULL, bitmapName,
    IMAGE_BITMAP,
    rect.right - rect.left, rect.bottom - rect.top,
    LR_LOADFROMFILE
  );
  if (bitmap==NULL) //unsuccessful, so try to get the image from resource
  {
    bitmap = (HBITMAP) LoadImage(
      NULL, 
      (LPCTSTR) (m_pSetup->m_sSetupSourceDir + _T ("setup.bmp")), IMAGE_BITMAP, 
      rect.right - rect.left, rect.bottom - rect.top, LR_LOADFROMFILE
    );
  }
	if (bitmap != NULL)
	{
		CStatic *pict = (CStatic*) GetDlgItem (IDC_SETUP_TITLE);
		if (pict == NULL)
		{
			return FALSE;
		}
		pict->SetBitmap (bitmap);
		pict->MoveWindow(&rect);
	}

	// otevøe AVI
/*
	CString sFileAvi; m_pSetup->MakeLongDirStr(sFileAvi,m_pSetup->m_sSetupSourceDir);
	sFileAvi += AVI_NAME_SETUP;
	BOOL bRes = m_cSetupAnim.Open(sFileAvi);
  bRes = m_cSetupAnim.Play(0,-1,1);
*/	
	return TRUE;  
}

void CWizardPageDir::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CWizardPageDir)
  //	DDX_Control(pDX, IDC_ANIMATE_FIRST, m_cSetupAnim);
  DDX_Control(pDX, IDC_EDIT_DIRECTORY, m_cEditDirCtrl);
  //}}AFX_DATA_MAP
  if (!pDX->m_bSaveAndValidate)
  {
    DDX_Text(pDX, IDC_EDIT_DIRECTORY, m_sDirectory);
    DDV_MaxChars(pDX, m_sDirectory, _MAX_PATH);
  } else
  {
    DDX_Text(pDX, IDC_EDIT_DIRECTORY, m_sDirectory);
    DDV_MaxChars(pDX, m_sDirectory, _MAX_PATH);
    /*
    if (m_sDirectory.GetLength()==0)
    {
    SetupMessageBox(IDS_PLEASE_SELECT_DIRECTORY,MB_OK|MB_ICONEXCLAMATION);
    pDX->Fail();
    }
    m_pSetup->MakeLongDirStr(m_pSetup->m_sSetupPoseidonDir,m_sDirectory);

    __int64 need = m_pSetup->m_TotalCopySize * 1.1 + 10 * 1024 * 1024;
    DWORD cluster, sector, free, total;
    CString disc = m_pSetup->m_sSetupPoseidonDir.Left(3);
    GetDiskFreeSpace(disc, &cluster, &sector, &free, &total);
    __int64 avail = (__int64)cluster * (__int64)sector * (__int64)free;
    if (avail < need)
    {
    SetupMessageBox(IDS_NOT_ENOUGH_SPACE,MB_OK|MB_ICONEXCLAMATION);
    pDX->Fail();
    }
    */
  };

  DDX_Text(pDX, IDC_DISK_SPACE, m_sDirSpaceAvailable);
}

LRESULT CWizardPageDir::OnWizardNext() 
{
	LRESULT result = CPropertyPage::OnWizardNext();
	if (result == -1) return -1;

	if (!UpdateData()) return -1;

	if (m_sDirectory.GetLength()==0)
	{
		m_pSetup->SetupMessageBox(CSetupObject::SetupLocalizeString (_T("SETUP_STR_PLEASE_SELECT_DIRECTORY")),MB_OK|MB_ICONEXCLAMATION);
		return -1;
	}

  // check if there is required file in target directory
  if (!CheckTargetDirectory())
  {
    // do not proceed to next page if file is required
    if (m_pSetup->m_bTargetFileRequired)
    {
      m_pSetup->SetupMessageBox(CSetupObject::SetupLocalizeString (_T("SETUP_STR_INCORRECT_TARGET_DIRECTORY")),MB_OK|MB_ICONEXCLAMATION);;
      return -1;
    }
    else
    {
      // file in target directory is not essential, proceed according to user's decision
      int result = m_pSetup->SetupMessageBox(CSetupObject::SetupLocalizeString (_T("SETUP_STR_INCORRECT_TARGET_DIRECTORY")),MB_YESNO|MB_ICONEXCLAMATION);;
      if (result != IDYES)
        return -1;
    }
  }

	m_pSetup->MakeLongDirStr(m_pSetup->m_sSetupPoseidonDir,m_sDirectory);

	__int64 need = m_pSetup->m_TotalCopySize * 1.1 + 10 * 1024 * 1024;
	DWORD cluster, sector, free, total;
	CString disc = m_pSetup->m_sSetupPoseidonDir.Left(3);
	GetDiskFreeSpace(disc, &cluster, &sector, &free, &total);
	__int64 avail = (__int64)cluster * (__int64)sector * (__int64)free;
	if (avail < need)
	{
		m_pSetup->SetupMessageBox(CSetupObject::SetupLocalizeString (_T("SETUP_STR_NOT_ENOUGH_SPACE")),MB_OK|MB_ICONEXCLAMATION);
		return -1;
	}
	
	return 0;
}


BEGIN_MESSAGE_MAP(CWizardPageDir, CPropertyPage)
	//{{AFX_MSG_MAP(CWizardPageDir)
	ON_BN_CLICKED(IDC_CHANGE_DIR, OnChangeDir)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int WINAPI PosBrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
		::SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)((LPCTSTR)((CWizardPageDir*)lpData)->m_sDirectory));
		::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)((LPCTSTR)((CWizardPageDir*)lpData)->m_sDirectory));
	} else
	if (uMsg == BFFM_SELCHANGED)
	{
		TCHAR buff[_MAX_PATH];
		if (SHGetPathFromIDList((LPITEMIDLIST)lParam,buff))
		{
			CString sPath = buff;
			((CWizardPageDir*)lpData)->m_pSetup->AddInstallationSubdirectory(sPath);
			::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)((LPCTSTR)sPath));
		}
	}
	return 0;
};

void CWizardPageDir::OnChangeDir() 
{
	m_cEditDirCtrl.GetWindowText(m_sDirectory);

	LPITEMIDLIST pItemIDList = NULL;
	BROWSEINFO BrowseInfo; 
	memset(&BrowseInfo,0,sizeof(BrowseInfo));

	BrowseInfo.hwndOwner = m_hWnd;
  BrowseInfo.ulFlags	 = BIF_STATUSTEXT;
  BrowseInfo.lpfn		   = (BFFCALLBACK)(PosBrowseCallbackProc);
  BrowseInfo.lParam		 = (long)this;
/*
  BrowseInfo.pidlRoot
  BrowseInfo.pszDisplayName
  BrowseInfo.lpszTitle
  BrowseInfo.iImage
*/
	pItemIDList = SHBrowseForFolder(&BrowseInfo);
	if (pItemIDList != NULL)
	{
		TCHAR buff[_MAX_PATH];
		if (SHGetPathFromIDList(pItemIDList, buff))
		{
			m_sDirectory = buff;
			m_pSetup->AddInstallationSubdirectory(m_sDirectory);
		};
	}
  UpdateSpaceAvailable();
  UpdateData(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CWizardPageAllUsers dialog

class CWizardPageAllUsers : public CPropertyPage
{
  // Construction
public:
  CWizardPageAllUsers();
  ~CWizardPageAllUsers();

  // Dialog Data
  CSetupObject*	m_pSetup;

  //{{AFX_DATA(CWizardPageAllUsers)
  enum { IDD = IDD_WIZARD_PAGE_ALL_USERS };
  //}}AFX_DATA


  // Overrides
  // ClassWizard generate virtual function overrides
  //{{AFX_VIRTUAL(CWizardPageAllUsers)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

  // Implementation
protected:
  // Generated message map functions
  //{{AFX_MSG(CWizardPageAllUsers)
  virtual BOOL OnInitDialog();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

};

CWizardPageAllUsers::CWizardPageAllUsers() : CPropertyPage(CWizardPageAllUsers::IDD)
{
  //{{AFX_DATA_INIT(CWizardPageAllUsers)
  //}}AFX_DATA_INIT
}

CWizardPageAllUsers::~CWizardPageAllUsers()
{
}

void CWizardPageAllUsers::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CWizardPageDrX)
  //}}AFX_DATA_MAP
  DDX_Check(pDX, IDC_ALL_USERS, m_pSetup->m_bAllUsers);
}

BOOL CWizardPageAllUsers::OnInitDialog()
{
  CDialog::OnInitDialog();
  if (!m_pSetup->m_allUsers)
  { 
    CStatic *chbox = (CStatic*) GetDlgItem (IDC_ALL_USERS);
    if (chbox != NULL)chbox->ShowWindow(SW_HIDE); 
  }

  // TODO: Add extra initialization here
  // resize bitmap
  RECT rect;
  CWnd *ctrl = GetDlgItem(IDC_WELCOME_PICT_POS);
  ctrl->GetWindowRect(&rect);
  ScreenToClient(&rect);
  if (!m_pSetup->temporaryFolderActive) m_pSetup->PrepareResourceData();
  //if the image was in resource, it is located in temporary directory now
  CString tempDir = GetTempDirectory(m_pSetup->m_sApplicationName);
  CString bitmapName = tempDir + _T("\\data\\setupWelcome.bmp");
  HBITMAP bitmap = (HBITMAP)LoadImage(
    NULL, bitmapName,
    IMAGE_BITMAP,
    rect.right - rect.left, rect.bottom - rect.top,
    LR_LOADFROMFILE
  );
  if (bitmap==NULL) //unsuccessful, so try to get the image from resource
  {
    bitmap = (HBITMAP) LoadImage (NULL, 
      (LPCTSTR) (m_pSetup->m_sSetupSourceDir + _T ("data\\setupWelcome.bmp")), IMAGE_BITMAP, 
      rect.right - rect.left, rect.bottom - rect.top, LR_LOADFROMFILE);
  }
  if (bitmap != NULL)
  {
    CStatic *pict = (CStatic*) GetDlgItem (IDC_WELCOME_PICT);
    if (pict == NULL)
    {
      return FALSE;
    }
    pict->SetBitmap (bitmap);
    pict->MoveWindow(&rect);
  }

  return TRUE;   // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CWizardPageAllUsers, CPropertyPage)
  //{{AFX_MSG_MAP(CWizardPageAllUsers)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWizardPageInfo dialog
class CWizardPageInfo : public CPropertyPage
{
  // Construction
public:
  CWizardPageInfo();
  ~CWizardPageInfo();

  // Dialog Data
  CSetupObject*	m_pSetup;

  // Dialog Data
  enum { IDD = IDD_WIZARD_PAGE_INFO };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
  // Generated message map functions
  //{{AFX_MSG(CWizardPageAllUsers)
  virtual BOOL OnInitDialog();
  //}}AFX_MSG


public:
  CString m_InfoText;
};

CWizardPageInfo::CWizardPageInfo()
: CPropertyPage(CWizardPageInfo::IDD)
, m_InfoText(_T(""))
{

}

CWizardPageInfo::~CWizardPageInfo()
{
}

void CWizardPageInfo::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_INFO_TEXT, m_InfoText);
}

BOOL CWizardPageInfo::OnInitDialog()
{
  CString licenseFileName = CString(_T("data\\info#"))+ cc_cast(GLanguage);
  m_InfoText = UTF8ToCString(m_pSetup->LoadFile(licenseFileName, FALSE));
  if (m_InfoText.IsEmpty())
  {
    CString licenseFileName = _T("data\\info");
    m_InfoText = UTF8ToCString(m_pSetup->LoadFile(licenseFileName, FALSE));
  };
  
  UpdateData(FALSE);
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// CWizardPageLicence dialog
class CWizardPageLicence : public CPropertyPage
{
  // Construction
public:
  CWizardPageLicence();
  ~CWizardPageLicence();

  // Dialog Data
  CSetupObject*	m_pSetup;

  // Dialog Data
  enum { IDD = IDD_WIZARD_PAGE_LICENCE };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
  // Generated message map functions
  //{{AFX_MSG(CWizardPageAllUsers)
  virtual BOOL OnInitDialog();
  virtual void OnRSelection();
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

public:
  int m_licenceSelection;
  CString m_licenceText;
};

CWizardPageLicence::CWizardPageLicence()
: CPropertyPage(CWizardPageLicence::IDD)
, m_licenceSelection(1)
, m_licenceText(_T(""))
{

}

CWizardPageLicence::~CWizardPageLicence()
{
}

void CWizardPageLicence::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
  DDX_Radio(pDX, IDC_LICENCE, m_licenceSelection);
  DDX_Text(pDX, IDC_LICENCE_TEXT, m_licenceText);
}

BOOL CWizardPageLicence::OnInitDialog()
{
  // disable NEXT button
  CPropertyPage::OnInitDialog();
  m_licenceSelection = 1;
  if (m_licenceSelection) m_pSetup->m_licenceButtonDisabled=true;
  CPropertySheet *sheet = (CPropertySheet*)GetParent();
  m_pSetup->m_licencePageIndex = sheet->GetPageIndex(this); //sheet->->GetActiveIndex();
 // load LICENCE file


  // first search for "data\license.czech" and then "data\license#czech", some filesystems are not compatible with "#"
  CString licenseFileName = CString(_T("data\\license."))+ cc_cast(GLanguage);
  m_licenceText = UTF8ToCString(m_pSetup->LoadFile(licenseFileName, FALSE));
  if (m_licenceText.IsEmpty())
  {
    licenseFileName = CString(_T("data\\license#"))+ cc_cast(GLanguage);
    m_licenceText = UTF8ToCString(m_pSetup->LoadFile(licenseFileName, FALSE));
    if (m_licenceText.IsEmpty())
    {
      licenseFileName = _T("data\\license");
      m_licenceText = UTF8ToCString(m_pSetup->LoadFile(licenseFileName, FALSE));
  }
  }

  
  UpdateData(FALSE);
  return true;
}

void CWizardPageLicence::OnRSelection()
{
  // TODO: Add your control notification handler code here

  // Synchronize the data
  UpdateData(TRUE);
  DWORD dwFlags = PSWIZB_BACK;
  if (!m_licenceSelection) dwFlags |= PSWIZB_NEXT;
  m_pSetup->m_licenceButtonDisabled = m_licenceSelection!=0;
  ((CPropertySheet*)GetParent())->SetWizardButtons(dwFlags);
  // Repaint the second dialog
}

BEGIN_MESSAGE_MAP(CWizardPageLicence, CPropertyPage)
  ON_BN_CLICKED(IDC_LICENCE, OnRSelection)
  ON_BN_CLICKED(IDC_LICENCE2, OnRSelection)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWizardPageDrX dialog

class CWizardPageDrX : public CPropertyPage
{
// Construction
public:
	CWizardPageDrX();
	~CWizardPageDrX();

// Dialog Data
	CSetupObject*	m_pSetup;

	//{{AFX_DATA(CWizardPageDrX)
	enum { IDD = IDD_WIZARD_PAGE_DRX };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizardPageDrX)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizardPageDrX)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

CWizardPageDrX::CWizardPageDrX() : CPropertyPage(CWizardPageDrX::IDD)
{
	//{{AFX_DATA_INIT(CWizardPageDrX)
	//}}AFX_DATA_INIT
}

CWizardPageDrX::~CWizardPageDrX()
{
}

void CWizardPageDrX::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWizardPageDrX)
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_DIRECTX_SETUP, m_pSetup->m_bSetupDirectX);
}


BEGIN_MESSAGE_MAP(CWizardPageDrX, CPropertyPage)
	//{{AFX_MSG_MAP(CWizardPageDrX)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CWizardPageDrX::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// zobrazení verze DirectX
	CString sText;
	if ((m_pSetup->m_dwVersion == 0) && (m_pSetup->m_dwRevision == 0))
	{
		sText = CSetupObject::SetupLocalizeString (_T("SETUP_STR_DIRECTX_NOT_FOUND"));
	}
	else if (HIWORD(m_pSetup->m_dwVersion) < 4)
	{
		sText = "3";
	}
	else if (HIWORD(m_pSetup->m_dwVersion) == 4)
	{
		_stprintf(sText.GetBuffer(500),_T("%d.%d"), LOWORD(m_pSetup->m_dwVersion), HIWORD(m_pSetup->m_dwRevision));
		sText.ReleaseBuffer();
	}
	else
	{
		_stprintf(sText.GetBuffer(500),_T("%d.%d.%d.%d"),
			HIWORD(m_pSetup->m_dwVersion), LOWORD(m_pSetup->m_dwVersion),
            HIWORD(m_pSetup->m_dwRevision), LOWORD(m_pSetup->m_dwRevision));
		sText.ReleaseBuffer();
	}
	//::SetWindowText(::GetDlgItem(m_hWnd,IDC_DIRECTX_DETECT),sText);
  SetupSetWindowText(::GetDlgItem(m_hWnd,IDC_DIRECTX_DETECT),sText);

#if _MP_DEMO || _DEMO
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_DIRECTX_SETUP), FALSE);
	::ShowWindow(::GetDlgItem(m_hWnd, IDC_DIRECTX_SETUP), SW_HIDE);
#endif

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CWizardPageRes dialog

class CWizardPageRes : public CPropertyPage
{
// Construction
public:
	CWizardPageRes();
	~CWizardPageRes();

// Dialog Data
	CSetupObject*	m_pSetup;

	CString			m_sText;
	//{{AFX_DATA(CWizardPageRes)
	enum { IDD = IDD_WIZARD_PAGE_RES };
	CEdit	m_cShowText;
	//}}AFX_DATA

			void AddLineID(int nHeader,BOOL);
			void AddLine(LPCTSTR pValue,BOOL);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizardPageRes)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizardPageRes)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

CWizardPageRes::CWizardPageRes() : CPropertyPage(CWizardPageRes::IDD)
{
	//{{AFX_DATA_INIT(CWizardPageRes)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CWizardPageRes::~CWizardPageRes()
{
}

void CWizardPageRes::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWizardPageRes)
	DDX_Control(pDX, IDC_SETUP_TEXT, m_cShowText);
	//}}AFX_DATA_MAP
	if (!pDX->m_bSaveAndValidate)
	{
		// sestavení textu
		m_sText.Empty();
		CString sText;

		// FlashPoint
		AddLine(CSetupObject::SetupLocalizeString (_T("SETUP_STR_RESUME_DIRECOTRY")),FALSE);
		m_pSetup->MakeShortDirStr(sText,m_pSetup->m_sSetupPoseidonDir);
		AddLine(sText,TRUE);

		// Direct X
		if (m_pSetup->m_bSetupDirectX)
		{
			AddLine(_T(" "),FALSE);
			AddLine(CSetupObject::SetupLocalizeString (_T("SETUP_STR_RESUME_DIRECTX")),FALSE);
			AddLine(CSetupObject::SetupLocalizeString (_T("SETUP_STR_RESUME_DIRECTX_VER")),TRUE);
		};

		// zobrazení
		//m_cShowText.SetWindowText(m_sText);
    SetupSetWindowText(&m_cShowText, m_sText);
	}
}


BEGIN_MESSAGE_MAP(CWizardPageRes, CPropertyPage)
	//{{AFX_MSG_MAP(CWizardPageRes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CWizardPageRes::AddLineID(int nHeader,BOOL bValue)
{
	CString sHeader;
	VERIFY(sHeader.LoadString(nHeader));
	AddLine(sHeader,bValue);
};

void CWizardPageRes::AddLine(LPCTSTR pValue,BOOL bValue)
{
	if (bValue)
		m_sText += _T("       ");
	m_sText += pValue;
	m_sText += _T("\r\n");
};


/////////////////////////////////////////////////////////////////////////////
// CWizardPageSetup dialog

class CWizardPageSetup : public CPropertyPage
{
// Construction
public:
	CWizardPageSetup();
	~CWizardPageSetup();

// Dialog Data
	CSetupObject*	m_pSetup;

	BOOL			m_bInTimer;
	CString			m_sCopyFrom;
	CString			m_sCopyTo;
	int					m_nNextPicture;
	//{{AFX_DATA(CWizardPageSetup)
	enum { IDD = IDD_WIZARD_PAGE_STP };
//	CAnimateCtrl	m_cSetupAnim;
	CProgressCtrl	m_cPrograssCopy;
	CStatic				m_cSetupPicture;
	//}}AFX_DATA


			void DoEndSetup(int);


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizardPageSetup)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizardPageSetup)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

CWizardPageSetup::CWizardPageSetup() : CPropertyPage(CWizardPageSetup::IDD)
{
	m_bInTimer	= FALSE;
	//{{AFX_DATA_INIT(CWizardPageSetup)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CWizardPageSetup::~CWizardPageSetup()
{
}

void CWizardPageSetup::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWizardPageSetup)
//	DDX_Control(pDX, IDC_ANIMATE_COPY, m_cSetupAnim);
	DDX_Control(pDX, IDC_PROGRESS_COPY, m_cPrograssCopy);
	DDX_Control(pDX, IDC_PICTURE_COPY, m_cSetupPicture);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizardPageSetup, CPropertyPage)
	//{{AFX_MSG_MAP(CWizardPageSetup)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CWizardPageSetup::DoEndSetup(int nIDEnd)
{
	// zrušen timer
	KillTimer(TIMER_COPY_ID);
	// zrušen proces kopírování
	m_pSetup->CancelCopy();
  m_pSetup->CancelRunApp();
	// ukoncen Wizard	
	((CPropertySheet*)GetParent())->EndDialog(nIDEnd);
};

BOOL CWizardPageSetup::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// progress
	m_cPrograssCopy.SetRange(0,100);
	m_cPrograssCopy.SetPos(0);
  ::SetWindowText(::GetDlgItem(m_hWnd,IDC_COPY_FROM),_T(""));
  ::SetWindowText(::GetDlgItem(m_hWnd,IDC_COPY_TO),_T(""));
	// otevøe AVI
/*
	CString sFileAvi; m_pSetup->MakeLongDirStr(sFileAvi,m_pSetup->m_sSetupSourceDir);
	sFileAvi += AVI_NAME_COPY;
	BOOL bRes = m_cSetupAnim.Open(sFileAvi);
  bRes = m_cSetupAnim.Play(0,-1,-1);
*/
	CString sFileWav; m_pSetup->MakeLongDirStr(sFileWav,m_pSetup->m_sSetupSourceDir);
	sFileWav += WAV_NAME_COPY;

	// FIX: avoid searching music on bad CD
	// BOOL bRes = PlaySound(sFileWav, NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
  //check whether sound file exists (as it can be saved in temporary too
  if (!m_pSetup->temporaryFolderActive) m_pSetup->PrepareResourceData();
  //if the image was in resource, it is located in temporary directory now
  CString tempDir = GetTempDirectory(m_pSetup->m_sApplicationName);
  CString wavName = tempDir + _T("\\") + WAV_NAME_COPY;
  extern bool FileExists(CString &path);
  if (FileExists(wavName)) 
  {
    PlaySound(wavName, NULL, SND_FILENAME | SND_ASYNC);
  }
  else
  {
    FILE *testf = _tfopen(sFileWav,_T("rb"));
    if (testf)
    {
      fclose(testf);
      PlaySound(sFileWav, NULL, SND_FILENAME | SND_ASYNC);
    }
  }

	// run - SETUP_STATE_COPY_BGN
	ASSERT(m_pSetup->m_nCurrState == SETUP_STATE_STPWIZ);
	m_pSetup->m_nCurrState = SETUP_STATE_COPY_BGN;
	// next picture
	m_nNextPicture = 0;
	// timer

	SetTimer(TIMER_COPY_ID,TIMER_COPY_DEL,NULL);
	return TRUE;
}

void CWizardPageSetup::OnDestroy() 
{
	PlaySound(NULL, NULL, SND_NODEFAULT);
	KillTimer(TIMER_COPY_ID);
	CPropertyPage::OnDestroy();
}

void CWizardPageSetup::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == TIMER_COPY_ID)
	{
		if (m_bInTimer) return;
		m_bInTimer = TRUE;
		// provedu krok instalace
		if (!m_pSetup->DoSetupStep())
		{	// ukoncení instalace
			DoEndSetup(ID_WIZFINISH);
		} else
		{	// update podle prùbìhu instalace
			if (m_pSetup->m_nCurrState == SETUP_STATE_FINAL)
			{	// ukoncení
				DoEndSetup(ID_WIZFINISH);
			} else
			if (m_pSetup->m_nCurrState == SETUP_STATE_COPY_RUN)
			{	// získání údajù
				CString	sCopyFrom,sCopyTo;
				EnterCriticalSection(&(m_pSetup->m_CSection_CopyFiles)); 
				__int64 nCopied = m_pSetup->m_nCurrCopySize;
				sCopyFrom	= m_pSetup->m_sCopySource;
				CString sCopyToTemp		= m_pSetup->m_sCopyDestination;
				LeaveCriticalSection(&(m_pSetup->m_CSection_CopyFiles));

				 // this will remove "../" from path 
				_tfullpath(sCopyTo.GetBufferSetLength(MAX_PATH),sCopyToTemp.GetBuffer(),MAX_PATH); 
				sCopyTo.ReleaseBuffer();
				sCopyToTemp.ReleaseBuffer();

				 
				// update kontrolù
				int nPos = (int)(100.*((double)nCopied)/((double)m_pSetup->m_TotalCopySize));
				m_cPrograssCopy.SetPos(nPos);
				if (lstrcmp(m_sCopyFrom,sCopyFrom)!=0)
				{
					CSetupObject::SetFilenameToControl(::GetDlgItem(m_hWnd,IDC_COPY_FROM),sCopyFrom);
					m_sCopyFrom = sCopyFrom;
				}
				if (lstrcmp(m_sCopyTo,sCopyTo)!=0)
				{
					CSetupObject::SetFilenameToControl(::GetDlgItem(m_hWnd,IDC_COPY_TO),sCopyTo);
					m_sCopyTo = sCopyTo;
				}
				if (m_nNextPicture < m_pSetup->m_arPictures.GetSize())
				{
					CPictureItem* pItem = (CPictureItem*)(m_pSetup->m_arPictures[m_nNextPicture]);
					if (nCopied >= pItem->m_nFrom)
					{
						if (pItem->m_hPicture)
						{
							RECT rect;
							CWnd *ctrl = GetDlgItem(IDC_PICTURE_COPY_POS);
							ctrl->GetWindowRect(&rect);
							ScreenToClient(&rect);
							// resize bitmap
							pItem->m_hPicture = (HBITMAP) CopyImage (pItem->m_hPicture, IMAGE_BITMAP, rect.right - rect.left, rect.bottom - rect.top, LR_COPYDELETEORG);
							m_cSetupPicture.SetBitmap(pItem->m_hPicture);
//							ctrl = GetDlgItem(IDC_PICTURE_COPY);
//							ctrl->MoveWindow(&rect);
							m_cSetupPicture.MoveWindow(&rect);
						}
						m_nNextPicture++;
					}
				}
			};
		};
		m_bInTimer = FALSE;
	} else
	CPropertyPage::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////
// CSetupWizardDlg

class CSetupWizardDlg : public CPropertySheet
{
// Construction
public:
	CSetupWizardDlg(CSetupObject *setupObj);

// Attributes
public:
	CSetupObject *m_pSetup;
	CWizardPageCDKEY	pgCDKEY; 
  CWizardPageAllUsers pgAllUsers; 
	CWizardPageLicence pgLicence;
  CWizardPageInfo   pgInfo;
  CWizardPageDir		pgDir; 
	CWizardPageDrX		pgDrX;
	CWizardPageRes		pgRes; 
	CWizardPageSetup	pgStp; 

// Operations
public:
			void	OnUpdateWizardButtons();


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetupWizardDlg)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSetupWizardDlg();
  void ForceNextPage() 
  {
    PressButton(PSBTN_NEXT);
    OnWizNext();
  }

	// Generated message map functions
protected:
	//{{AFX_MSG(CSetupWizardDlg)
	afx_msg void OnCancel();
	afx_msg void OnWizPrev();
	afx_msg void OnWizNext();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


CSetupWizardDlg::CSetupWizardDlg(CSetupObject *setupObj)
	:CPropertySheet(_T(""), AfxGetMainWnd()/*NULL*/, 0),
	m_pSetup (setupObj)
{
	// vložení stránek
	if (m_pSetup->m_cdKeyOptions.action == CKeyOptions::readFromDialog || (!m_pSetup->m_cdKeyOptions.m_cdKeyIsInRegistry && !m_pSetup->m_sParentAppRegistryRoot.IsEmpty()))
	{
		AddPage(&pgCDKEY);
	}
  else if (m_pSetup->m_cdKeyOptions.action == CKeyOptions::readFromWeb)
  {
    pgCDKEY.SetReadFromWeb();
    AddPage(&pgCDKEY);
  }
  else if (m_pSetup->m_cdKeyOptions.action == CKeyOptions::readFromOFPReg)
  {
    if (m_pSetup->m_cdKeyOptions.m_cdKeyIsInRegistry)
    {
      char buffer[32];
      CDKey::EncodeMsg(m_pSetup->m_cdkey,buffer);

      char *p = strtok(buffer, "-");
      if (p) 
      {
        pgCDKEY.m_cdkey1 = p;
        p = strtok(NULL, "-");
        pgCDKEY.m_cdkey2 = p;
        p = strtok(NULL, "-");
        pgCDKEY.m_cdkey3 = p;
        p = strtok(NULL, "-");
        pgCDKEY.m_cdkey4 = p;
        p = strtok(NULL, "-");
        pgCDKEY.m_cdkey5 = p;
      }
    }

    AddPage(&pgCDKEY);
  }

  if (setupObj->m_showWelcome)AddPage(&pgAllUsers);
  if (setupObj->m_processInfo) AddPage(&pgInfo);
  if (setupObj->m_processLicense) AddPage(&pgLicence);
	if (!setupObj->m_doNotAskToDestFolder)AddPage(&pgDir);
  if (setupObj->m_processDirectX==CSetupObject::DRX_DIALOG) AddPage(&pgDrX);
//	AddPage(&pgRes);
	AddPage(&pgStp);
}

CSetupWizardDlg::~CSetupWizardDlg()
{
}


BEGIN_MESSAGE_MAP(CSetupWizardDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CSetupWizardDlg)
	ON_COMMAND(ID_WIZBACK, OnWizPrev)
	ON_COMMAND(ID_WIZNEXT, OnWizNext)
	ON_WM_SYSCOMMAND()
	ON_COMMAND(IDCANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CSetupWizardDlg::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	OnUpdateWizardButtons();
	return bResult;
}

void CSetupWizardDlg::OnCancel() 
{
	int	  nCount  = GetPageCount();
	int	  nIndx	  = GetPageIndex(GetActivePage());
	if (nIndx == (nCount-1))
	{
		pgStp.m_bInTimer = TRUE;
		if (UserCancelSetup(m_pSetup))
		{
      m_pSetup->CancelCopy();
			Default();
		}
		pgStp.m_bInTimer = FALSE;
	} else
  {
    m_pSetup->CancelCopy();
		Default();
  }
}

void CSetupWizardDlg::OnWizPrev() 
{
	Default();
	// update buttons
	OnUpdateWizardButtons();
}
void CSetupWizardDlg::OnWizNext() 
{
	Default();
	// update buttons
	OnUpdateWizardButtons();
}

void CSetupWizardDlg::OnSysCommand(UINT nID, LPARAM lParam) 
{
	if (nID == SC_CLOSE)
	{
		pgStp.m_bInTimer = TRUE;
		if (UserCancelSetup(m_pSetup))
		{
			CPropertySheet::OnSysCommand(nID, lParam);
		}
		pgStp.m_bInTimer = FALSE;
	} else
		CPropertySheet::OnSysCommand(nID, lParam);
}

void CSetupWizardDlg::OnUpdateWizardButtons()
{
  if (m_psh.dwFlags & PSH_WIZARD)
	{
		DWORD dwFlags = 0;
		int	  nCount  = GetPageCount();
		int	  nIndx	  = GetPageIndex(GetActivePage());
		
		if (nIndx > 0)
			dwFlags |= PSWIZB_BACK;
		if (nIndx < (nCount-1))
    {
      if ((!m_pSetup->m_licenceButtonDisabled || m_pSetup->m_licencePageIndex!=nIndx) &&
         (m_pSetup->m_CDKeyPageIndex!=nIndx || !m_pSetup->m_CDKeyButtonDisabled))
			  dwFlags |= PSWIZB_NEXT;
    }
		else
		{
			dwFlags &= ~PSWIZB_BACK;
			//dwFlags |= PSWIZB_FINISH;
			SetWizardButtons(dwFlags);
		}
		SetWizardButtons(dwFlags);
	}

	m_pSetup->ApplyApplicationNameAndStringTable (this);
};

/////////////////////////////////////////////////////////////////////////////
// DoSetupWizard

BOOL CSetupObject::DoSetupWizard()
{
	CSetupWizardDlg	dlg (this);
	dlg.pgCDKEY.m_pSetup = this;
  dlg.pgAllUsers.m_pSetup = this; 
	dlg.pgDir.m_pSetup = this;
  dlg.pgLicence.m_pSetup = this;
  dlg.pgInfo.m_pSetup = this;
	dlg.pgDrX.m_pSetup = this; 
	dlg.pgRes.m_pSetup = this;
	dlg.pgStp.m_pSetup = this;
	// realizace dialogu
	dlg.SetWizardMode();
	if (dlg.DoModal()==ID_WIZFINISH)
		return TRUE;
	// pøerušeno uživatelem
	return FALSE;
};

BOOL CWizardPageDir::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPage::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CWizardPageCDKEY property page

IMPLEMENT_DYNCREATE(CWizardPageCDKEY, CPropertyPage)

CWizardPageCDKEY::CWizardPageCDKEY() : CPropertyPage(CWizardPageCDKEY::IDD)
{
	//{{AFX_DATA_INIT(CWizardPageCDKEY)
	m_cdkey1 = _T("");
	m_cdkey2 = _T("");
	m_cdkey3 = _T("");
	m_cdkey4 = _T("");
	m_cdkey5 = _T("");
	//}}AFX_DATA_INIT
  _readFromWeb = false;
}

CWizardPageCDKEY::~CWizardPageCDKEY()
{
}

void CWizardPageCDKEY::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWizardPageCDKEY)
	DDX_Text(pDX, IDC_CDKEY1, m_cdkey1);
	DDV_MaxChars(pDX, m_cdkey1, 4);
	DDX_Text(pDX, IDC_CDKEY2, m_cdkey2);
	DDV_MaxChars(pDX, m_cdkey2, 5);
	DDX_Text(pDX, IDC_CDKEY3, m_cdkey3);
	DDV_MaxChars(pDX, m_cdkey3, 5);
	DDX_Text(pDX, IDC_CDKEY4, m_cdkey4);
	DDV_MaxChars(pDX, m_cdkey4, 5);
	DDX_Text(pDX, IDC_CDKEY5, m_cdkey5);
	DDV_MaxChars(pDX, m_cdkey5, 5);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
    int productID = m_pSetup->m_cdKeyOptions.m_newKeys ? m_pSetup->m_cdKeyOptions.m_productID : -1;

		if (!m_pSetup->CheckCDKEY(productID, m_cdkey1 + m_cdkey2 + m_cdkey3 + m_cdkey4 + m_cdkey5, 
			m_pSetup->m_cdKeyOptions.m_publicKey, m_pSetup->m_cdKeyOptions.m_checkOffset, 
			CString2RString(m_pSetup->m_cdKeyOptions.m_checkWith)))
		{
			m_pSetup->SetupMessageBox(CSetupObject::SetupLocalizeString (_T("SETUP_STR_BAD_CDKEY")),MB_OK|MB_ICONEXCLAMATION,0,GetSafeHwnd());
			pDX->Fail();
		}
	}
}

BEGIN_MESSAGE_MAP(CWizardPageCDKEY, CPropertyPage)
	//{{AFX_MSG_MAP(CWizardPageCDKEY)
	ON_EN_CHANGE(IDC_CDKEY1, OnChangeCdkey1)
	ON_EN_CHANGE(IDC_CDKEY2, OnChangeCdkey2)
	ON_EN_CHANGE(IDC_CDKEY3, OnChangeCdkey3)
	ON_EN_CHANGE(IDC_CDKEY4, OnChangeCdkey4)
	ON_EN_CHANGE(IDC_CDKEY5, OnChangeCdkey5)
  ON_WM_DESTROY()
  ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizardPageCDKEY message handlers


// { HELPER FUNCTIONS
BOOL GenerateRandomBytes(BYTE *buffer, int count) 
{
  BOOL ok = false;

  HCRYPTPROV hProvider = 0;
  if (::CryptAcquireContext(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
  {
    ok = ::CryptGenRandom(hProvider, count, buffer);
    ::CryptReleaseContext(hProvider, 0);
  }
  return ok;
}

inline TCHAR ToChar(int digit)
{
  if (digit < 10) return _T('0') + digit;
  return _T('a') + digit - 10;
}
// }

BOOL CWizardPageCDKEY::OnInitDialog() 
{
  CPropertyPage::OnInitDialog();

  CPropertySheet *sheet = (CPropertySheet*)GetParent();
  m_pSetup->m_CDKeyPageIndex = sheet->GetPageIndex(this);

  UpdateData(FALSE);

  if (_readFromWeb)
  {
    m_pSetup->SetupMessageBox(CSetupObject::SetupLocalizeString(_T("SETUP_WIZARD_CDKEY_FROM_WEB")),MB_OK);

    // { OPEN THE BROWSER
    const bool hostSecure = false; // https protocol
    const TCHAR *hostName = _T("free.arma2.com");
    const int hostPort = INTERNET_DEFAULT_HTTP_PORT;
    const TCHAR *hostRoot = _T("activation");

    // create a nonce
    BYTE random[32];
    TCHAR nonce[2 * sizeof(random) + 1];
    BOOL nonceOK = GenerateRandomBytes(random, sizeof(random));
    if (nonceOK)
    {
      TCHAR *ptr = nonce;
      for (int i=0; i<sizeof(random); i++)
      {
        *ptr = ToChar((random[i] >> 4) & 0x0f); ptr++;
        *ptr = ToChar(random[i] & 0x0f); ptr++;
      }
      *ptr = 0;
    }

    // open the browser
    TCHAR cmd[1024];
    size_t cmdSize = sizeof(cmd) / sizeof(TCHAR);
    _stprintf_s(cmd, cmdSize, _T("url.dll,FileProtocolHandler %s://%s:%d/%s/index.jsp"),
      hostSecure ? _T("https") : _T("http"), hostName, hostPort, hostRoot);
    if (nonceOK) // otherwise user need to copy Product Key manually
    {
      _tcscat_s(cmd, cmdSize, _T("?nonce="));
      _tcscat_s(cmd, cmdSize, nonce);
    }
    ShellExecute(NULL, _T("open"), _T("rundll32.exe"), cmd, NULL, SW_SHOWNORMAL);
    // } OPEN THE BROWSER

    // { CONNECT TO THE SERVER
    session = connect = request = NULL;
    if (nonceOK)
    {
      TCHAR query[1024];
      size_t querySize = sizeof(query) / sizeof(TCHAR);
      _stprintf_s(query, querySize, _T("/%s/Result?nonce=%s"), hostRoot, nonce);

      session = WinHttpOpen(NULL, 
        WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
        0);
      if (!session) return FALSE;
      else
      {
        connect = WinHttpConnect(session, hostName, hostPort, 0);
        if (!connect) return FALSE;
        else
        {
          request = WinHttpOpenRequest(connect, _T("GET"), query,
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, hostSecure ? WINHTTP_FLAG_SECURE : 0); 
          if (!request) return FALSE;
        }
      }
    }
    // } CONNECT TO THE SERVER

    SetTimer(TIMER_GET_CDKEY_FROM_WEB,2000,NULL);
  }
  CheckAllCDKeysToAllowNextButton();
  return TRUE;
}

void CWizardPageCDKEY::OnDestroy() 
{
  if (_readFromWeb)
  {
    KillTimer(TIMER_GET_CDKEY_FROM_WEB);
  }
  CPropertyPage::OnDestroy();
}

static char _webCDKeyResult[100] = "";

void CWizardPageCDKEY::OnTimer(UINT nIDEvent) 
{
  if (nIDEvent == TIMER_GET_CDKEY_FROM_WEB)
  {
    // { CHECK THE SERVER
    if (request)
    {
      if (!WinHttpSendRequest(request, 
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 
        0, 0)) return; //OnError();
      else
      {
        if (!WinHttpReceiveResponse(request, NULL)) return;
        else
        {
          DWORD maxSize = sizeof(_webCDKeyResult);
          DWORD size;
          if (WinHttpQueryDataAvailable(request, &size))
          {
            if (size > 0 && size < maxSize)
            {
              if (!WinHttpReadData(request, _webCDKeyResult, size, &size))
              {
                //OnError();
                _webCDKeyResult[0] = 0;
              }
              else
              {
                KillTimer(TIMER_GET_CDKEY_FROM_WEB);
                _webCDKeyResult[size] = 0;
                // fill it in
                char *p = strtok(_webCDKeyResult, "-");
                if (!p) { return; }
                m_cdkey1 = p;
                p = strtok(NULL, "-"); if (!p) { UpdateData(FALSE); return; }
                m_cdkey2 = p;
                p = strtok(NULL, "-"); if (!p) { UpdateData(FALSE); return; }
                m_cdkey3 = p;
                p = strtok(NULL, "-"); if (!p) { UpdateData(FALSE); return; }
                m_cdkey4 = p;
                p = strtok(NULL, "-"); if (!p) { UpdateData(FALSE); return; }
                m_cdkey5 = p;
                UpdateData(FALSE);
                CheckAllCDKeysToAllowNextButton();
                ((CSetupWizardDlg*)GetParent())->ForceNextPage();
              }
            }
          }
        }
      }
    }
    // } CHECK THE SERVER
  } else
    CPropertyPage::OnTimer(nIDEvent);
}

void CWizardPageCDKEY::OnChangeCdkey1() 
{
	CString text;
	GetDlgItem(IDC_CDKEY1)->GetWindowText(text);
	if (text.GetLength() >= 4)
		GetDlgItem(IDC_CDKEY2)->SetFocus();

  CheckAllCDKeysToAllowNextButton();
}

void CWizardPageCDKEY::OnChangeCdkey2() 
{
	CString text;
	GetDlgItem(IDC_CDKEY2)->GetWindowText(text);
	if (text.GetLength() >= 5)
		GetDlgItem(IDC_CDKEY3)->SetFocus();

  CheckAllCDKeysToAllowNextButton();
}

void CWizardPageCDKEY::OnChangeCdkey3() 
{
	CString text;
	GetDlgItem(IDC_CDKEY3)->GetWindowText(text);
	if (text.GetLength() >= 5)
		GetDlgItem(IDC_CDKEY4)->SetFocus();

  CheckAllCDKeysToAllowNextButton();
}

void CWizardPageCDKEY::OnChangeCdkey4() 
{
	CString text;
	GetDlgItem(IDC_CDKEY4)->GetWindowText(text);
	if (text.GetLength() >= 5)
		GetDlgItem(IDC_CDKEY5)->SetFocus();

  CheckAllCDKeysToAllowNextButton();
}

void CWizardPageCDKEY::OnChangeCdkey5() 
{
/*
	CString text;
	GetDlgItem(IDC_CDKEY5)->GetWindowText(text);
	if (text.GetLength() >= 5)
		GetDlgItem(IDC_CDKEY1)->SetFocus();
*/
  CheckAllCDKeysToAllowNextButton();
}
void CWizardPageCDKEY::CheckAllCDKeysToAllowNextButton()
{
  m_pSetup->m_CDKeyButtonDisabled = (GetDlgItem(IDC_CDKEY1)->GetWindowTextLength()==0 || GetDlgItem(IDC_CDKEY2)->GetWindowTextLength()==0 || GetDlgItem(IDC_CDKEY3)->GetWindowTextLength()==0 || GetDlgItem(IDC_CDKEY4)->GetWindowTextLength()==0 || GetDlgItem(IDC_CDKEY5)->GetWindowTextLength()==0 );
  ((CSetupWizardDlg*)GetParent())->OnUpdateWizardButtons();
};
