// InsertCdRomDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SetupObject.h"
#include "Setup.h"
#include "InsertCdRomDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInsertCdRomDlg dialog


CInsertCdRomDlg::CInsertCdRomDlg(CSetupObject *setupObj, CCdRom *cd, CString &drive, CWnd* pParent /*=NULL*/)
	: CDialog(CInsertCdRomDlg::IDD, pParent), _cd (cd), _drive (drive),
	_setupObj (setupObj)
{
	//{{AFX_DATA_INIT(CInsertCdRomDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CInsertCdRomDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInsertCdRomDlg)
	DDX_Control(pDX, IDC_INSERT_CDROM_MESSAGE, _message);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInsertCdRomDlg, CDialog)
	//{{AFX_MSG_MAP(CInsertCdRomDlg)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInsertCdRomDlg message handlers

int CInsertCdRomDlg::DoModal() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CDialog::DoModal();
}

void CInsertCdRomDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	
	CDialog::OnTimer(nIDEvent);

	if (CSetupObject::IsCdRomPresent (_cd, _drive))
	{
		EndDialog (IDOK);
	}
}

void CInsertCdRomDlg::PreSubclassWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CDialog::PreSubclassWindow();

	_timerId = SetTimer (1, 2000, NULL);
}

BOOL CInsertCdRomDlg::DestroyWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	KillTimer (_timerId);
	return CDialog::DestroyWindow();
}

BOOL CInsertCdRomDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	_setupObj->ApplyApplicationNameAndStringTable (this);

	CString curStr, newStr;
	_message.GetWindowText (curStr);
	newStr.Format (_T ("%s \"%s\"."), curStr, _cd->_label);
	//_message.SetWindowText (newStr);
  SetupSetWindowText(&_message, newStr);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
