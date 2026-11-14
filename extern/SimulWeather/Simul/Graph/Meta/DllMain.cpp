#include "precomp.h"
#ifdef SIMUL_DLL
#include <windows.h>
#include "Simul/Base/Modules.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID )
{
if(fdwReason ==1) //The dll is attaching now.
	{
		// What is the filename of THIS module?
		char lpFilename[256];
		DWORD result=GetModuleFileName(hinstDLL,lpFilename,256);
		if(result!=0)// non-zero string
			simul::base::AddModule(lpFilename,hinstDLL);//modules[lpFilename]=hinstDLL;
	}
	return (TRUE);
}
#endif
