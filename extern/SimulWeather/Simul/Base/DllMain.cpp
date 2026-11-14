#include "precomp.h"

#ifdef SIMUL_DLL
#include <windows.h>
//#include <vld.h>

BOOL WINAPI DllMain(HINSTANCE ,ULONG ,LPVOID )
{
	//Load this first, then load ExportTable!
	return (TRUE);
}
#endif
