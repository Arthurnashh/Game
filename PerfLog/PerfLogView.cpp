// PerfLogView.cpp : implementation of the CPerfLogView class
//

#include "stdafx.h"
#include "PerfLog.h"

#include "PerfLogDoc.h"
#include "PerfLogView.h"
#include "MainFrm.h"
#include <Es/Algorithms/qsort.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPerfLogView

IMPLEMENT_DYNCREATE(CPerfLogView, CBaseView)

BEGIN_MESSAGE_MAP(CPerfLogView, CBaseView)
	//{{AFX_MSG_MAP(CPerfLogView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPerfLogView construction/destruction

CPerfLogView::CPerfLogView()
{
	#if RICHTEXT_VIEW
	m_nWordWrap = WrapNone;
	#endif
	_nColumns=0;
}

CPerfLogView::~CPerfLogView()
{
}

IMPLEMENT_DYNCREATE(CPerfLogSummaryView, CPerfLogView)

CPerfLogSummaryView::CPerfLogSummaryView()
{
}

CPerfLogSummaryView::~CPerfLogSummaryView()
{
}

void CPerfLogSummaryView::OnInitialUpdate()
{
	CPerfLogView::OnInitialUpdate();
}

void CPerfLogSummaryView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	UpdateSummaryView();
}

BOOL CPerfLogView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	return CBaseView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CPerfLogView drawing

void CPerfLogView::OnDraw(CDC* pDC)
{
	CPerfLogDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
	CBaseView::OnDraw(pDC);
}

/////////////////////////////////////////////////////////////////////////////
// CPerfLogView diagnostics

#ifdef _DEBUG
void CPerfLogView::AssertValid() const
{
	CBaseView::AssertValid();
}

void CPerfLogView::Dump(CDumpContext& dc) const
{
	CBaseView::Dump(dc);
}

CPerfLogDoc* CPerfLogView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPerfLogDoc)));
	return (CPerfLogDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPerfLogView message handlers

void CPerfLogView::OnInitialUpdate() 
{
	CPerfLogDoc *doc=GetDocument();

	_currentSection=doc->_sections.Size()-1;


	CBaseView::OnInitialUpdate();

	//SetScrollSizes( MM_TEXT, CSize(10,10) );

	/*
	DWORD mask = GetRichEditCtrl().GetEventMask();
	mask |= ENM_CHANGE;
	GetRichEditCtrl().SetEventMask(mask);
	*/

	#if RICHTEXT_VIEW
		CHARFORMAT cf;
		cf.cbSize = sizeof(cf); 
		cf.dwMask = CFM_FACE | CFM_SIZE | CFM_BOLD; 
		cf.dwEffects = 0;
		//cf.yHeight = 200; 
		cf.yHeight = 150; 
		cf.bCharSet = DEFAULT_CHARSET; 
		cf.bPitchAndFamily = FIXED_PITCH | FF_DONTCARE; 
		strncpy(cf.szFaceName, "Courier New", LF_FACESIZE); 
		//strncpy(cf.szFaceName, "Small Fonts", LF_FACESIZE); 
		GetRichEditCtrl().SetDefaultCharFormat(cf);

		PARAFORMAT pf;
		pf.cbSize = sizeof(pf); 
		pf.dwMask = PFM_TABSTOPS; 
		pf.cTabCount = MAX_TAB_STOPS;
		//for (int i=0; i<MAX_TAB_STOPS; i++) pf.rgxTabs[i] = 200 * i;
		for (int i=0; i<MAX_TAB_STOPS; i++) pf.rgxTabs[i] = 1000 * i;
		GetRichEditCtrl().SetParaFormat(pf);
	#endif

}

void CPerfLogView::Serialize(CArchive & ar, BOOL bSelection)
{
	/*
	Busy();
	EDITSTREAM es = {0, 0, EditStreamCallBack};
	EditorCookie cookie(ar);
	es.dwCookie = (DWORD)&cookie;
	int nFormat = SF_TEXT;
	if (bSelection)
		nFormat |= SFF_SELECTION;
	if (ar.IsStoring())
		GetRichEditCtrl().StreamOut(nFormat, es);
	else
	{
		GetRichEditCtrl().StreamIn(nFormat, es);
		Invalidate();
	}
	if (cookie.m_dwError != 0)
		AfxThrowFileException(cookie.m_dwError);
	Ready();
	*/
}

void CPerfLogView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	UpdateView();
}

#if RICHTEXT_VIEW

class LineCookie
{
public:
	const char *_read;
	LineCookie( const char *read ) : _read(read) {}
};

DWORD CALLBACK CPerfLogView::LineStreamInCallBack
(
	DWORD cookie, LPBYTE buff, LONG cb, LONG *pcb
)
{
	LineCookie *lCookie = (LineCookie *)cookie;
	const char *read = lCookie->_read;
	int len = strlen(read);
	if( cb>len ) cb = len;
	strcpy((char *)buff,read);
	*pcb=cb;
	lCookie->_read += cb;

	return 0;
}

void CPerfLogView::ListText( const char *line, bool clear )
{
	EDITSTREAM es = {0, 0, LineStreamInCallBack};
	LineCookie cookie(line);
	es.dwCookie = (DWORD)&cookie;
	int format = SF_TEXT;
	if ( !clear ) format |= SFF_SELECTION;
	GetRichEditCtrl().StreamIn(format, es);
}

void CPerfLogView::DeleteLastChar()
{
	CRichEditCtrl &ctrl=GetRichEditCtrl();
	long beg,end;
	ctrl.GetSel(beg,end);
	beg--;
	ctrl.SetSel(beg,end);
	ctrl.ReplaceSel("");
	ctrl.SetSel(0,0);
}

#define MAX_LINE ( 4*1024 )
#define colDelim "|"


#endif

void CPerfLogView::ListHeaders()
{
	#if RICHTEXT_VIEW
	CPerfLogDoc *doc=GetDocument();

	char text[MAX_LINE];
	char *write=text;

	for( int i=0; i<_showColumns.Size(); i++ )
	{
		int c=_showColumns[i];

		const char *buf=doc->_headings[c];
		while( *buf==' ' ) buf++;
		int lenBuf = strlen(buf);
		if( i==0 ) buf="msf";
		//if( !strcmp(buf,"fps") ) 
		while( lenBuf<5 ) *write++ = ' ',lenBuf++;
		strcpy( write, buf );
		strcat( write, colDelim );
		write = write + strlen(write);
	}

	strcat( write, "\n" );
	ListText(text);
	#endif
}


void CPerfLogView::UpdateHeaders()
{
	#if RICHTEXT_VIEW
	CPerfLogDoc *doc=GetDocument();


	ListText("",true);

	#else
	CListCtrl &list = GetListCtrl();
	CPerfLogDoc *doc=GetDocument();

	// scan headers and create correspoding listview columns
	
	int columns=0;
	for( int i=0; i<_showColumns.Size(); i++ )
	{
		int index=_showColumns[i];
		LV_COLUMN column;
		const char *text=doc->_headings[index];
		column.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_FMT; //|LVCF_SUBITEM; 
    column.fmt=LVCFMT_RIGHT; 
    column.cx=55;
    column.pszText=(char *)text;
    column.cchTextMax; 
    column.iSubItem=0; 
		if( i>=_nColumns )
		{
			list.InsertColumn(i,&column);
			_nColumns++;
		}
		else
		{
			list.SetColumn(i,&column);
		}
		columns++;
	}
	while( _nColumns>columns && list.DeleteColumn(columns) ) _nColumns--;

	//if( _currentSection<0 || _currentSection>=doc->_sections.Size() ) return;
	list.DeleteAllItems();
	_nLines=0;
	#endif
}


void CPerfLogView::ListLine( const AutoArray<double> &line, bool allFloats )
{
	#if RICHTEXT_VIEW

	char text[MAX_LINE];
	char *write=text;

	for( int i=0; i<_showColumns.Size(); i++ )
	{
		int c=_showColumns[i];
		float fValue=0;
		if( c<line.Size() ) fValue=line[c];
		char buf[80];

		if( c==0 )
		{
			sprintf(buf,"%5.1f",fValue);
		}
		else if( allFloats )
		{
			sprintf(buf,"%5.2f",fValue);
		}
		else
		{
			int iValue=toInt(fValue);
			sprintf(buf,"%d",iValue);
		}
		int lenBuf=strlen(buf);
		while( lenBuf<5 ) *write++ = ' ',lenBuf++;
		strcpy( write, buf );
		strcat( write, colDelim );
		write = write + strlen(write);
	}

	strcat( write, "\n" );

	ListText(text);


	#else
	CListCtrl &list = GetListCtrl();
	for( int i=0; i<_showColumns.Size(); i++ )
	{
		int c=_showColumns[i];
		float fValue=0;
		if( c<line.Size() ) fValue=line[c];
		char buf[80];

		if( c==0 || allFloats )
		{
			sprintf(buf,"%6.2f",fValue);
		}
		else
		{
			int iValue=toInt(fValue);
			sprintf(buf,"%d",iValue);
		}

		if( i==0 )
		{
			LV_ITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_STATE; // | LVIF_IMAGE | 
			lvi.iItem = _nLines;
			lvi.iSubItem = i;
			lvi.pszText = buf;
			lvi.iImage = -1;
			lvi.stateMask = 0; //LVIS_STATEIMAGEMASK;
			lvi.state = 0;

			list.InsertItem(&lvi);
		}
		else
		{
			list.SetItemText(_nLines,i,buf);
		}
	}
	_nLines++;
	#endif
}

void CPerfLogView::ListCount( const char *text, int count )
{
	#if RICHTEXT_VIEW
	char buf[MAX_LINE];
	sprintf(buf,"%s%5d\n",text,count);
	ListText(buf);
	#else
	#endif
}

void CPerfLogView::ListSectionSummary( int index )
{
	CPerfLogDoc *doc=GetDocument();

	// insert all items from current section
	// start with total section

	DataSection &section=doc->_sections[index];
	if( section.Size()<=0 ) return;
	
	AutoArray<double> avg,sum;
	
	sum.Resize(section.Size());
	avg.Resize(section.Size());
	for( int i=0; i<section.Size(); i++ )
	{
		sum[i]=0;
	}

	int lines=section[0].Size();
	for( int l=0; l<lines; l++ )
	{
		for( int c=0; c<section.Size(); c++ )
		{
			float fValue=section[c][l];
			sum[c]+=fValue;
		}
	}

	for( int i=0; i<section.Size(); i++ )
	{
		avg[i]=sum[i]/lines;
	}

	ListCount("Frames: ",lines);
	ListHeaders();
	ListLine(avg);
	//ListLine(sum);
}

void CPerfLogView::ListSection( int index )
{
	CPerfLogDoc *doc=GetDocument();

	// insert all items from current section
	// start with total section

	DataSection &section=doc->_sections[index];
	if( section.Size()<=0 ) return;

	ListHeaders();

	int lines=section[0].Size();
	AutoArray<double> line;
	line.Resize(section.Size());
	for( int l = 0; l < lines; l++)
	{
		for( int c=0; c<section.Size(); c++ )
		{
			line[c]=section[c][l];
		}
		ListLine(line);
	}
}

void CPerfLogView::ListDelimiter( const char *text )
{
	#if RICHTEXT_VIEW
		char delim=*text;
		char buf[MAX_LINE];
		const int len=_showColumns.Size()*6;
		memset(buf,delim,len);
		buf[len]='\n';
		buf[len+1]=0;
		ListText(buf);
	#else
	CListCtrl &list = GetListCtrl();
	for( int c=0; c<_nColumns; c++ )
	{
		if( c==0 )
		{
			LV_ITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_STATE;; // | LVIF_IMAGE | 
			lvi.iItem = _nLines;
			lvi.iSubItem = 0;
			lvi.pszText = (char *)text;
			lvi.iImage = -1;
			lvi.stateMask = 0; //LVIS_STATEIMAGEMASK;
			lvi.state = 0;

			list.InsertItem(&lvi);
		}
		else
		{
			list.SetItemText(_nLines,c,text);
		}
	}
	_nLines++;
	#endif
}

void CPerfLogView::ListSectionAnalysis( int index )
{
	CPerfLogDoc *doc=GetDocument();

	DataSection &section=doc->_sections[index];
	if( section.Size()<=0 ) return;

	// calculate and print analysis
	DataSectionAnalysis analysis;
	section.Analyze(analysis);

	ListDelimiter("======");

	ListLine(analysis._correlation,true);


}

void CPerfLogView::Busy()
{
	// CWinApp
	theApp.DoWaitCursor(+1);

	CMainFrame *frame = (CMainFrame *)theApp.GetMainWnd();
	frame->SetStatusText("Busy");
}

void CPerfLogView::Ready()
{
	theApp.DoWaitCursor(-1);

	CMainFrame *frame = (CMainFrame *)theApp.GetMainWnd();
	frame->SetStatusText("Ready");
}

static int SortByCorrelation( const int *i1, const int *i2, DataSectionAnalysis *analysis )
{
	float v1=fabs(analysis->_correlation[*i1]);
	float v2=fabs(analysis->_correlation[*i2]);
	if (v2>v1) return +1;
	if (v2<v1) return -1;
	return 0;
}


void CPerfLogView::UpdateView()
{
	CPerfLogDoc *doc=GetDocument();

	if( doc->_sections.Size()<=0 ) return;

	Busy();

	DataSection &total=doc->_sections[doc->_sections.Size()-1];
	DataSectionAnalysis analysis;
	total.Analyze(analysis);

	AutoArray<int> sort;
	sort.Resize(total.Size());
	
	// sort by correlation
	
	for( int i=0; i<total.Size(); i++ ) sort[i]=i;

	QSort(sort.Data(),sort.Size(),&analysis,SortByCorrelation);

	_showColumns.Clear();
	for( int i=0; i<total.Size(); i++ )
	{
		int index=sort[i];
		const float minCorr=0.05;
		if( fabs(analysis._correlation[index])>=minCorr )
		{
			_showColumns.Add(index);
		}
	}
	
	UpdateHeaders();


	ListCount("Sections: ",doc->_sections.Size()-1);
	for( int s=0; s<doc->_sections.Size()-1; s++ )
	{
		ListSection(s);
		ListDelimiter("++++++");
		ListSectionSummary(s);
		ListSectionAnalysis(s);
		ListDelimiter("------");
	}

	DeleteLastChar();

	Invalidate();

	Ready();
}

void CPerfLogView::UpdateSummaryView()
{
	CPerfLogDoc *doc=GetDocument();

	if( doc->_sections.Size()<=0 ) return;


	Busy();

	DataSection &total=doc->_sections[doc->_sections.Size()-1];
	DataSectionAnalysis analysis;
	total.Analyze(analysis);

	AutoArray<int> sort;
	sort.Resize(total.Size());
	
	// sort by correlation
	
	for( int i=0; i<total.Size(); i++ ) sort[i]=i;

	QSort(sort.Data(),sort.Size(),&analysis,SortByCorrelation);

	_showColumns.Clear();
	for( int i=0; i<total.Size(); i++ )
	{
		int index=sort[i];
		const float minCorr=0.05;
		if( fabs(analysis._correlation[index])>=minCorr )
		{
			_showColumns.Add(index);
		}
	}
	
	UpdateHeaders();

	ListSectionSummary(doc->_sections.Size()-1);
	ListSectionAnalysis(doc->_sections.Size()-1);
	DeleteLastChar();

	Invalidate();

	Ready();
}
