// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//




#if !defined(AFX_STDAFX_H__6F94A436_55DE_11D4_90D5_00C0DFAE7D0A__INCLUDED_)
#define AFX_STDAFX_H__6F94A436_55DE_11D4_90D5_00C0DFAE7D0A__INCLUDED_

#ifndef _WIN32_WINNT
#define WINVER  0x0500      /* version 5.0 */
#define _WIN32_WINNT 0x0500
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

//#include <afxwin.h>         // MFC core and standard components
//#include <afxext.h>         // MFC extensions
//#include <afxdisp.h>        // MFC OLE automation classes
//#include <afxole.h>
//#ifndef _AFX_NO_AFXCMN_SUPPORT
//#include <afxcmn.h>			// MFC support for Windows Common Controls
//#include <keylib.h>
//#include "protection.h"
//#include "AutoArray.h"
//#endif // _AFX_NO_AFXCMN_SUPPORT

#include "optima2mfc.h"

//#include <g3dlib.h>
//#include <g3dpp\g3dpp.h>
//#define mxVector3(x,y,z) mxInline_SetVector4(x,y,z,1.0f)
//#undef Vector3 //G3D Matrix namespace colision. Remove Vector3 Definition
#include "ObjLOD.hpp" 
//#include "optima\misc.h"


//template Ref<AnimationPhase>;
/*
#define CDIALOGBAR_RESIZEABLE \
	virtual CSize CalcDynamicLayout(int nLength, DWORD nMode)\
	  {\
	  if (nMode & (LM_MRUWIDTH|LM_COMMIT))\
		  return m_sizeDefault;\
	  else if (nMode & LM_HORZ && !(nMode & (LM_HORZDOCK | LM_VERTDOCK)))\
		{\
		if (nMode & LM_LENGTHY) m_sizeDefault.cy=((nLength+8)/16)*16; else m_sizeDefault.cx=((nLength+8)/16)*16;\
		return m_sizeDefault;\
		}\
	  return CDialogBar::CalcDynamicLayout(nLength,nMode);\
	  }
*/
/*
void FSaveString(ostream& os,const char *str);
void FLoadString(istream& is, char *str, int size);
void MoveWindowRel(HWND hwnd, int xm, int ym, int xsm, int ysm,HDWP dwp);
void SaveBarSize(CDialogBar& bar);
bool LoadBarSize(CDialogBar& bar,int xdef=0,int ydef=0);

void SaveWindowSize(CWnd *wnd, const char *name);
void LoadWindowSize(CWnd *wnd, const char *name);

bool FileNewer(const char *src, const char *trg);

#define ArrSize(arr) (sizeof(arr)/sizeof(arr[0]))

#define MAXUINT (unsigned int)(-1)
#define MAXINT (int)(MAXUINT>>1)

#ifndef ListView_SetCheckState
#define ListView_SetCheckState(hwndLV, i, fCheck) \
      ListView_SetItemState(hwndLV, i, \
      INDEXTOSTATEIMAGEMASK((fCheck)+1), LVIS_STATEIMAGEMASK)
#endif


static inline int ToInt(float p)
  {
  int retr;
  __asm
    {    
    fld p
      fistp retr;    
    }
  return retr;
  }

#define MSG_UPDATESTATUS (WM_APP+1215)

*/




//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__6F94A436_55DE_11D4_90D5_00C0DFAE7D0A__INCLUDED_)
