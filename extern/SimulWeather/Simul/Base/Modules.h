#ifndef SIMUL_BASE_LIBRARIES_H
#define SIMUL_BASE_LIBRARIES_H
#include <map>
#include <string>
#include "Simul/Base/Export.h"

#ifdef _MSC_VER
	#define HAS_RTTI _CPPRTTI
#endif

#ifdef __GNUC__
	#if GTEST_GCC_VER_ >= 40302
		#ifdef __GXX_RTTI
			#define HAS_RTTI 1
		#else
			#define HAS_RTTI 0
		#endif
	#else
		// For gcc versions smaller than 4.3.2,  assume RTTI is enabled.
		#define HAS_RTTI 1
	#endif
#endif


#ifndef HAS_RTTI
	#error RTTI Should be enabled if modules.h is included.
#endif

namespace simul
{
	namespace base
	{
		extern SIMUL_BASE_EXPORT void* GetModule(const char *name);
		extern SIMUL_BASE_EXPORT void AddModule(const char *name,void* hinstDLL);
		extern SIMUL_BASE_EXPORT size_t GetNumModules();
		extern SIMUL_BASE_EXPORT const char * GetModuleName(size_t i);
		extern SIMUL_BASE_EXPORT void UnloadModule(const char *name);
	}
}
#endif
