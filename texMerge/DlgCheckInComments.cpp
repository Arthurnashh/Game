// DlgCheckInComments.cpp : implementation file
//

#include "stdafx.h"
#include "texMerge.h"
#include "DlgCheckInComments.h"


// DlgCheckInComments dialog

IMPLEMENT_DYNAMIC(DlgCheckInComments, CDialog)
DlgCheckInComments::DlgCheckInComments(CWnd* pParent /*=NULL*/)
: CDialog(DlgCheckInComments::IDD, pParent)
, vComments(_T(""))
, vKeepCheckOut(FALSE)
{
}

DlgCheckInComments::~DlgCheckInComments()
{
}

void DlgCheckInComments::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_COMMENTS, vComments);
  DDX_Check(pDX, IDC_KEEPCHIN, vKeepCheckOut);
}


BEGIN_MESSAGE_MAP(DlgCheckInComments, CDialog)
END_MESSAGE_MAP()


// DlgCheckInComments message handlers
