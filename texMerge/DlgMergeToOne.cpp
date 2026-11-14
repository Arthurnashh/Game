// DlgMergeToOne.cpp : implementation file
//

#include "stdafx.h"
#include "texMerge.h"
#include "DlgMergeToOne.h"
#include ".\dlgmergetoone.h"


// DlgMergeToOne dialog

IMPLEMENT_DYNAMIC(DlgMergeToOne, CDialog)
DlgMergeToOne::DlgMergeToOne(CWnd* pParent /*=NULL*/)
: CDialog(DlgMergeToOne::IDD, pParent),vResult(0),vSetTitles(0)
{
}

DlgMergeToOne::~DlgMergeToOne()
{
}

void DlgMergeToOne::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_LIST, wList);
}


BEGIN_MESSAGE_MAP(DlgMergeToOne, CDialog)
END_MESSAGE_MAP()


// DlgMergeToOne message handlers

BOOL DlgMergeToOne::OnInitDialog()
{
  CDialog::OnInitDialog();

  for (int i=0;i<vSetTitles.Size();i++)
  {
    wList.AddString(vSetTitles[i]);
  }

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}


void DlgMergeToOne::OnOK()
{
  Assert(vResult.Size()==vSetTitles.Size())
  int count=0;
  for (int i=0,lcnt=vSetTitles.Size();i<lcnt;i++)
  {
    if (wList.GetSel(i)) vResult[count++]=i;
  }
  vResult=Array<int>(vResult.Data(),count);

  CDialog::OnOK();
}
