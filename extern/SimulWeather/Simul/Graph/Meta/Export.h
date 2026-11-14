#ifndef SIMUL_META_EXPORT_H
#define SIMUL_META_EXPORT_H

#if defined(_MSC_VER) && !defined(SIMUL_META_DLL)
	#ifdef _DEBUG
		#ifdef _DLL
			#pragma comment(lib,"SimulMeta_MDd")
		#else
			#pragma comment(lib,"SimulMeta_MTd")
		#endif
	#else
		#ifdef _DLL
			#pragma comment(lib,"SimulMeta_MD")
		#else
			#pragma comment(lib,"SimulMeta_MT")
		#endif
	#endif
#endif

#if defined(DOXYGEN) || defined(SN_TARGET_PS3) || defined(__GNUC__)
	#define SIMUL_META_EXPORT
#else
// In this lib:
	#if !defined(SIMUL_META_DLL) && defined(SIMUL_DLL)
	// If we're building dll libraries but not in this library IMPORT the classes
		#define SIMUL_META_EXPORT __declspec(dllimport)
	#else
	// In ALL OTHER CASES we EXPORT the classes!
		#define SIMUL_META_EXPORT __declspec(dllexport)
	#endif
#endif

#define SIMUL_META_EXPORT_CLASS class SIMUL_META_EXPORT
#define SIMUL_META_EXPORT_STRUCT struct SIMUL_META_EXPORT

#endif
