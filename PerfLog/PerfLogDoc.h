// PerfLogDoc.h : interface of the CPerfLogDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PERFLOGDOC_H__C8B8CC8E_B3D0_11D3_8373_00A0C9DF4D61__INCLUDED_)
#define AFX_PERFLOGDOC_H__C8B8CC8E_B3D0_11D3_8373_00A0C9DF4D61__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <El/CorelAnalys/corelAnalys.hpp>

//#define CBaseDoc CRichEditDoc
#define CBaseDoc CDocument

TypeIsGeneric(CString)

class CPerfLogDoc : public CBaseDoc
{
	friend class CPerfLogView;
	friend class CPerfLogApp;

	AutoArray<CString> _headings;
	AutoArray<DataSection> _sections;

	// load temporary
	int _currentSection;
	int _currentLine;

protected: // create from serialization only
	CPerfLogDoc();
	DECLARE_DYNCREATE(CPerfLogDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPerfLogDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL IsModified();
	virtual ~CPerfLogDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	
	void AddLine( const char *line );
	void Load( CArchive &ar );

// Generated message map functions
protected:
	//{{AFX_MSG(CPerfLogDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERFLOGDOC_H__C8B8CC8E_B3D0_11D3_8373_00A0C9DF4D61__INCLUDED_)
