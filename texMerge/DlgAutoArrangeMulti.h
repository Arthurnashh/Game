#pragma once


// DlgAutoArrangeMulti dialog

class DlgAutoArrangeMulti : public CDialog
{
	DECLARE_DYNAMIC(DlgAutoArrangeMulti)

public:
	DlgAutoArrangeMulti(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgAutoArrangeMulti();

// Dialog Data
	enum { IDD = IDD_AAMULTI };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
  // count of sets to use
  int vNSets;
  // Reduces size of texture, when space remains
  BOOL vOptimize;
  BOOL vDontMoveSel;
};
