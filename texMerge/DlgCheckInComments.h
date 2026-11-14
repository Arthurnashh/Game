#pragma once


// DlgCheckInComments dialog

class DlgCheckInComments : public CDialog
{
	DECLARE_DYNAMIC(DlgCheckInComments)

public:
	DlgCheckInComments(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgCheckInComments();

// Dialog Data
	enum { IDD = IDD_CHECKINCOMMENTS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
  CString vComments;
  BOOL vKeepCheckOut;
};
