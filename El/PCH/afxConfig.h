// force using AFX new/delete operator

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

// #undef LoadString

// Configuration parameters for internal release / debug version
#define _ENABLE_PERFLOG						1
#define _ENABLE_PATCHING					1
#define _ENABLE_INJECTING					1