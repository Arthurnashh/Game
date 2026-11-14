#ifndef SIMUL_CAMERA_EXPORT_H
#define SIMUL_CAMERA_EXPORT_H

#if defined(_MSC_VER) && !defined(SIMUL_CAMERA_DLL)
	#ifdef _DEBUG
		#ifdef _DLL
			#pragma comment(lib,"SimulCamera_MDd")
		#else
			#pragma comment(lib,"SimulCamera_MTd")
		#endif
	#else
		#ifdef _DLL
			#pragma comment(lib,"SimulCamera_MD")
		#else
			#pragma comment(lib,"SimulCamera_MT")
		#endif
	#endif
#endif

#if defined(DOXYGEN) || defined(SN_TARGET_PS3) || defined(__GNUC__)
	#define SIMUL_CAMERA_EXPORT
#else
// In this lib:
	#if !defined(SIMUL_CAMERA_DLL) && defined(SIMUL_DLL)
	// If we're building dll libraries but not in this library IMPORT the classes
		#define SIMUL_CAMERA_EXPORT __declspec(dllimport)
	#else
	// In ALL OTHER CASES we EXPORT the classes!
		#define SIMUL_CAMERA_EXPORT __declspec(dllexport)
	#endif
#endif

#define SIMUL_CAMERA_EXPORT_CLASS class SIMUL_CAMERA_EXPORT
#define SIMUL_CAMERA_EXPORT_STRUCT struct SIMUL_CAMERA_EXPORT

#endif
