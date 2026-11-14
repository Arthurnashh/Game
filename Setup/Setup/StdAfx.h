// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__5923B62B_0361_11D3_809E_0060083C95F5__INCLUDED_)
#define AFX_STDAFX_H__5923B62B_0361_11D3_809E_0060083C95F5__INCLUDED_

#pragma warning(disable : 4996)

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#define STRICT 1
#define WINVER 0x500 // required Win2000 (Other features need to be dynamically linked)

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC OLE automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxtempl.h>
#include <atlconv.h>

#include <El/elementpch.hpp>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__5923B62B_0361_11D3_809E_0060083C95F5__INCLUDED_)
