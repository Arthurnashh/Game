#ifndef SIMUL_BASE_EXPORT_H
#define SIMUL_BASE_EXPORT_H

#if defined(_MSC_VER) && !defined(SIMUL_BASE_DLL)
	#ifdef _DEBUG
		#ifdef _DLL
			#pragma comment(lib,"SimulBase_MDd")
		#else
			#pragma comment(lib,"SimulBase_MTd")
		#endif
	#else
		#ifdef _DLL
			#pragma comment(lib,"SimulBase_MD")
		#else
			#pragma comment(lib,"SimulBase_MT")
		#endif
	#endif
#endif

#if defined(DOXYGEN) || defined(SN_TARGET_PS3) || defined(__GNUC__)
	#define SIMUL_BASE_EXPORT
#else
// In this lib:
	#if !defined(SIMUL_BASE_DLL) && defined(SIMUL_DLL)
	// If we're building dll libraries but not in this library IMPORT the classes
		#define SIMUL_BASE_EXPORT __declspec(dllimport)
	#else
	// In ALL OTHER CASES we EXPORT the classes!
		#define SIMUL_BASE_EXPORT __declspec(dllexport)
	#endif
#endif

#ifdef _MSC_VER
	#define SIMUL_BASE_EXPORT_FN SIMUL_BASE_EXPORT __cdecl
#else
	#define SIMUL_BASE_EXPORT_FN SIMUL_BASE_EXPORT
#endif
#define SIMUL_BASE_EXPORT_CLASS class SIMUL_BASE_EXPORT
#define SIMUL_BASE_EXPORT_STRUCT struct SIMUL_BASE_EXPORT

#endif
