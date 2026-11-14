#ifndef SIMUL_CLOUDS_EXPORT_H
#define SIMUL_CLOUDS_EXPORT_H

#if defined(_MSC_VER) && !defined(SIMUL_CLOUDS_DLL)
	#ifdef _DEBUG
		#ifdef _DLL
			#pragma comment(lib,"SimulClouds_MDd")
		#else
			#pragma comment(lib,"SimulClouds_MTd")
		#endif
	#else
		#ifdef _DLL
			#pragma comment(lib,"SimulClouds_MD")
		#else
			#pragma comment(lib,"SimulClouds_MT")
		#endif
	#endif
#endif

#if defined(DOXYGEN) || defined(SN_TARGET_PS3) || defined(__GNUC__)
	#define SIMUL_CLOUDS_EXPORT
#else
	// In this lib:
	#if !defined(SIMUL_CLOUDS_DLL) && defined(SIMUL_DLL)
	// If we're building dll libraries but not in this library IMPORT the classes
		#define SIMUL_CLOUDS_EXPORT __declspec(dllimport)
	#else
	// In ALL OTHER CASES we EXPORT the classes!
		#define SIMUL_CLOUDS_EXPORT __declspec(dllexport)
	#endif
#endif
#endif

#define SIMUL_CLOUDS_EXPORT_CLASS class SIMUL_CLOUDS_EXPORT
#define SIMUL_CLOUDS_EXPORT_STRUCT struct SIMUL_CLOUDS_EXPORT
#ifdef _MSC_VER
	#define SIMUL_CLOUDS_EXPORT_FN SIMUL_CLOUDS_EXPORT __cdecl
#else
	#define SIMUL_CLOUDS_EXPORT_FN SIMUL_CLOUDS_EXPORT
#endif
