// PerfLogDoc.cpp : implementation of the CPerfLogDoc class
//

#include "stdafx.h"
#include "PerfLog.h"
#include "mainFrm.h"

#include "PerfLogDoc.h"
#include "PerfLogView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPerfLogDoc

IMPLEMENT_DYNCREATE(CPerfLogDoc, CBaseDoc)

BEGIN_MESSAGE_MAP(CPerfLogDoc, CBaseDoc)
	//{{AFX_MSG_MAP(CPerfLogDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPerfLogDoc construction/destruction

CPerfLogDoc::CPerfLogDoc()
{
	// TODO: add one-time construction code here

}

CPerfLogDoc::~CPerfLogDoc()
{
}

// IsModified

BOOL CPerfLogDoc::OnNewDocument()
{
	if (!CBaseDoc::OnNewDocument())
		return FALSE;

	// no sections
	_headings.Clear();
	_sections.Clear();

	_currentSection=-1;
	_currentLine=0;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CPerfLogDoc serialization

void CPerfLogDoc::AddLine( const char *line )
{
	int len=strlen(line);
	if( len<=0 ) return;
	if( *line=='-' )
	{
		// TODO: begin section
		_currentSection=_sections.Size();
		_currentLine=0;
		return;
	}
	const int columnWidth=6;
	// check if line contains some numbers
	bool onlyNumbers=true;
	for( const char *scan=line; *scan; scan++ )
	{
		char c=*scan;
		if( isdigit(c) ) continue;
		if( isspace(c) ) continue;
		if( strchr("+-.",c) ) continue;
		onlyNumbers=false;
		break;
	}
	if( onlyNumbers )
	{
		if( _currentSection>=_sections.Size() || _currentLine>50)
		{
			_currentSection = _sections.Add();
			_currentLine = 0;
		}
		// this line is numbers only - add it to data
		int column=0;
		DataSection &section=_sections[_currentSection];

		while( len>0 )
		{
			char number[columnWidth+1];
			number[columnWidth]=0;
			strncpy(number,line,columnWidth);
			line+=columnWidth;
			len-=columnWidth;
			float value=atof(number);
			// select appropriate column
			if( column==0 )
			{
				// invert value (to get time in ms)
				// if ms time is too high
				if( value<1.1f) return;
				value=1000/value;
				// value is always < 1000
			}
			section.Access(column);
			_headings.Access(column);
			DataColumn &c=section[column];
			int oldLine=c.Size();
			c.Access(_currentLine);
			for( int l=oldLine; l<_currentLine; l++ ) c[l]=0;
			c[_currentLine]=value;
			column++;
		}
		_currentLine++;
	}
	else
	{
		// scan for headers
		int column=0;
		while( len>0 )
		{
			char number[columnWidth+1];
			number[columnWidth]=0;
			strncpy(number,line,columnWidth);
			line+=columnWidth;
			len-=columnWidth;

			// select appropriate column
			_headings.Access(column);
			CString &h=_headings[column];
			//if( h.IsEmpty() )
			h=number;
			column++;
		}
	}
}

void CPerfLogDoc::Load(CArchive& ar)
{
	CString line;

	_currentSection=0;
	_currentLine=0;
	_sections.Clear();

	while( ar.ReadString(line) )
	{
		// parse line
		AddLine(line);
	}
	// create a new section: total of all sections
	int tSection=_sections.Add();
	DataSection &total=_sections[tSection];
	for( int s=0; s<tSection; s++ )
	{
		DataSection &section=_sections[s];
		section.Normalize();
		for( int c=0; c<section.Size(); c++ )
		{
			total.Access(c);
			DataColumn &tc=total[c];
			const DataColumn &sc=section[c];
			int oldSize=tc.Size();
			int addSize=sc.Size();
			int newSize=oldSize+addSize;
			tc.Realloc(newSize);
			tc.Resize(newSize);
			for( int l=0; l<sc.Size(); l++ )
			{
				tc[oldSize+l]=sc[l];
			}
		}
	}
	total.Normalize();
}

void CPerfLogDoc::Serialize(CArchive& ar)
{
	CPerfLogView *view = GetPerfLogView();
	if (ar.IsStoring())
	{
		// save is not supported 
	}
	else
	{
		// new document 
		// TODO: add loading code here
		Load(ar);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPerfLogDoc diagnostics

#ifdef _DEBUG
void CPerfLogDoc::AssertValid() const
{
	CBaseDoc::AssertValid();
}

void CPerfLogDoc::Dump(CDumpContext& dc) const
{
	CBaseDoc::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPerfLogDoc commands


BOOL CPerfLogDoc::IsModified()
{
	return CDocument::IsModified();
}
