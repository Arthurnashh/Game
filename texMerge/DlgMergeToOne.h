#pragma once
#include <afxwin.h>


// DlgMergeToOne dialog

class DlgMergeToOne : public CDialog
{
	DECLARE_DYNAMIC(DlgMergeToOne)

public:
	DlgMergeToOne(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgMergeToOne();

// Dialog Data
	enum { IDD = IDD_MERGETOONE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  CListBox wList;

	DECLARE_MESSAGE_MAP()
public:

  Array<CString> vSetTitles;
  Array<int> vResult;

  virtual BOOL OnInitDialog();
protected:
  virtual void OnOK();
};
