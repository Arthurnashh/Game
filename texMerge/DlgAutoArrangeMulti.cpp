// DlgAutoArrangeMulti.cpp : implementation file
//

#include "stdafx.h"
#include "texMerge.h"
#include "DlgAutoArrangeMulti.h"


// DlgAutoArrangeMulti dialog

IMPLEMENT_DYNAMIC(DlgAutoArrangeMulti, CDialog)
DlgAutoArrangeMulti::DlgAutoArrangeMulti(CWnd* pParent /*=NULL*/)
	: CDialog(DlgAutoArrangeMulti::IDD, pParent)
  , vNSets(0)
  , vOptimize(TRUE)
  , vDontMoveSel(FALSE)
{
}

DlgAutoArrangeMulti::~DlgAutoArrangeMulti()
{
}

void DlgAutoArrangeMulti::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_NUMSETS, vNSets);  
  DDV_MinMaxInt(pDX,vNSets,1,99);
  DDX_Check(pDX, IDC_OPTIMIZE, vOptimize);
  DDX_Check(pDX, IDC_DONTMOVESEL, vDontMoveSel);
}


BEGIN_MESSAGE_MAP(DlgAutoArrangeMulti, CDialog)
END_MESSAGE_MAP()


// DlgAutoArrangeMulti message handlers
