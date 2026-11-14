// CfgEditDoc.h : interface of the CCfgEditDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CFGEDITDOC_H__BA1937ED_85EB_11D3_94CD_C0140D8C4B14__INCLUDED_)
#define AFX_CFGEDITDOC_H__BA1937ED_85EB_11D3_94CD_C0140D8C4B14__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CCfgEditDoc : public CDocument
{
protected: // create from serialization only
	CCfgEditDoc();
	DECLARE_DYNCREATE(CCfgEditDoc)

	CParamFile _buffer;
	
	ParamClass _index;
	bool _indexValid;

// Attributes
public:
	const ParamClass &RootClass() const {return _index;}
	bool IsIndexValid() const {return _indexValid;}
	void SetIndexValid(bool valid) {_indexValid = valid;}

// Operations
public:
	void RebuildIndex();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCfgEditDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCfgEditDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CCfgEditDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CFGEDITDOC_H__BA1937ED_85EB_11D3_94CD_C0140D8C4B14__INCLUDED_)
