// AutoRunDlg.h : header file
//

#if !defined(AFX_AUTORUNDLG_H__3FFC1331_1F47_11D3_95EA_00A0C9A57F0B__INCLUDED_)
#define AFX_AUTORUNDLG_H__3FFC1331_1F47_11D3_95EA_00A0C9A57F0B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAutoRunDlg dialog

class CAutoRunFile;

class CAutoRunDlg : public CDialog
{
// Construction
public:
	CAutoRunDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CAutoRunDlg ();

// Dialog Data
	BOOL	  m_bSetupIsRunning;
	PROCESS_INFORMATION m_prInfo;

	bool			m_bMainDirectorySet;
	CString			m_sMainDirectory;
	CString			m_sAutoRunDirectory;
	CString			m_sAutoRunFileName;
	HBITMAP			m_hBitmap;
	CPtrArray		m_arAutoRunFiles;
	CPtrArray		m_arButtons;
	CPtrArray		m_arTriggers;
	RECT			m_cQuitButtonRect;
	CString			m_sQuitButtonText;
	CString			m_sMainDirectoryRegKeyPath;
	CString			m_sMainDirectoryRegKeyName;
	int				m_iImageWidth;
	int				m_iImageHeight;
	int				m_iImageHorSpace;
	int				m_iImageVerSpace;
	static CString	m_sApplicationName;

	//{{AFX_DATA(CAutoRunDlg)
	enum { IDD = IDD_AUTORUN_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoRunDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CAutoRunDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
			CAutoRunFile *FindAutoRunFile (CString &name);
			BOOL LoadAutoRunDatFile (CStringArray &autorunDatLines);
			BOOL ProcessAutoRunDatLines (CStringArray &autorunDatLines);
			void UpdateTriggerStates ();
			void GetMainDirectoryFromRegistry ();
	static	int		SetupFindOneOf (CString &where, LPCTSTR charSet, int start);
	static	bool	ApplyApplicationNameAndStringTable (CString &text, bool applyApplName);
	static	void	ApplyApplicationNameAndStringTable (CWnd *item);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTORUNDLG_H__3FFC1331_1F47_11D3_95EA_00A0C9A57F0B__INCLUDED_)
