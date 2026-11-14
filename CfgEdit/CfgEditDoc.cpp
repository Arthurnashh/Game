// CfgEditDoc.cpp : implementation of the CCfgEditDoc class
//

#include "stdafx.h"
#include "CfgEdit.h"

#include "CfgEditDoc.h"
#include "CfgEditView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCfgEditDoc

IMPLEMENT_DYNCREATE(CCfgEditDoc, CDocument)

BEGIN_MESSAGE_MAP(CCfgEditDoc, CDocument)
	//{{AFX_MSG_MAP(CCfgEditDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCfgEditDoc construction/destruction

CCfgEditDoc::CCfgEditDoc()
{
	_indexValid = true;
}

CCfgEditDoc::~CCfgEditDoc()
{
}

BOOL CCfgEditDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CCfgEditDoc serialization

void CCfgEditDoc::RebuildIndex()
{
	_buffer.Rewind();
	_index.Clear();
	_index.Parse(_buffer);
	UpdateAllViews(GetEditorView());
	_indexValid = true;
}

void CCfgEditDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		CEditorView *view = GetEditorView();
		if (view)
		{
			_buffer.SeekToBegin();
			CArchive arEditor(&_buffer, CArchive::store);
			view->Serialize(arEditor);
			int size = _buffer.GetLength();
			BYTE *buffer = new BYTE[size];
			if (buffer)
			{
				_buffer.SeekToBegin();
				_buffer.ReadHuge(buffer, size);
				ar.Write(buffer, size);
				delete [] buffer;
			}
		}
		if (!IsIndexValid()) RebuildIndex();
	}
	else
	{
		// get file size
		const CFile *file = ar.GetFile();
		int size = file->GetLength();
		BYTE *buffer = new BYTE[size];
		if (buffer)
		{
			ar.Read(buffer, size);
			_buffer.SetLength(0);
			_buffer.SeekToBegin();
			_buffer.WriteHuge(buffer, size);
			delete [] buffer;
		}

		_buffer.SeekToBegin();
		CArchive arEditor(&_buffer, CArchive::load);
		CEditorView *view = GetEditorView();
		if (view)
		{
			_indexValid = false;
			view->Serialize(arEditor);
		}
		if (!IsIndexValid()) RebuildIndex();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCfgEditDoc diagnostics

#ifdef _DEBUG
void CCfgEditDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CCfgEditDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CCfgEditDoc commands

BOOL CCfgEditDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	
	return TRUE;
}

BOOL CCfgEditDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	if (!CDocument::OnSaveDocument(lpszPathName))
		return FALSE;

	return TRUE;
}
