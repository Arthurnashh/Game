#ifndef SIMUL_SKY_EXPORT_H
#define SIMUL_SKY_EXPORT_H

#if defined(_MSC_VER) && !defined(SIMUL_SKY_DLL)
	#ifdef _DEBUG
		#ifdef _DLL
			#pragma comment(lib,"SimulSky_MDd")
		#else
			#pragma comment(lib,"SimulSky_MTd")
		#endif
	#else
		#ifdef _DLL
			#pragma comment(lib,"SimulSky_MD")
		#else
			#pragma comment(lib,"SimulSky_MT")
		#endif
	#endif
#endif

#if defined(DOXYGEN) || defined(SN_TARGET_PS3) || defined(__GNUC__)
	#define SIMUL_SKY_EXPORT
#else
// In this lib:
	#if !defined(SIMUL_SKY_DLL) && defined(SIMUL_DLL)
	// If we're building dll libraries but not in this library IMPORT the classes
		#define SIMUL_SKY_EXPORT __declspec(dllimport)
	#else
	// In ALL OTHER CASES we EXPORT the classes!
		#define SIMUL_SKY_EXPORT __declspec(dllexport)
	#endif
#endif
#endif

#ifdef _MSC_VER
	#define SIMUL_SKY_EXPORT_FN SIMUL_SKY_EXPORT __cdecl
#else
	#define SIMUL_SKY_EXPORT_FN SIMUL_SKY_EXPORT
#endif
#define SIMUL_SKY_EXPORT_CLASS class SIMUL_SKY_EXPORT
#define SIMUL_SKY_EXPORT_STRUCT struct SIMUL_SKY_EXPORT

