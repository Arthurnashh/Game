#ifndef SIMUL_GEOMETRY_EXPORT_H
#define SIMUL_GEOMETRY_EXPORT_H

#if defined(_MSC_VER) && !defined(SIMUL_GEOMETRY_DLL)
	#ifdef _DEBUG
		#ifdef _DLL
			#pragma comment(lib,"SimulGeometry_MDd")
		#else
			#pragma comment(lib,"SimulGeometry_MTd")
		#endif
	#else
		#ifdef _DLL
			#pragma comment(lib,"SimulGeometry_MD")
		#else
			#pragma comment(lib,"SimulGeometry_MT")
		#endif
	#endif
#endif

#if defined(DOXYGEN) || defined(__GNUC__)
	#define SIMUL_GEOMETRY_EXPORT
#else
// In this lib:
	#if !defined(SIMUL_GEOMETRY_DLL) && defined(SIMUL_DLL)
	// If we're building dll libraries but not in this library IMPORT the classes
		#define SIMUL_GEOMETRY_EXPORT __declspec(dllimport)
	#else
	// In ALL OTHER CASES we EXPORT the classes!
		#define SIMUL_GEOMETRY_EXPORT __declspec(dllexport)
	#endif
#endif

#define SIMUL_GEOMETRY_EXPORT_CLASS class SIMUL_GEOMETRY_EXPORT
#define SIMUL_GEOMETRY_EXPORT_STRUCT struct SIMUL_GEOMETRY_EXPORT
#define SIMUL_GEOMETRY_EXPORT_FN SIMUL_GEOMETRY_EXPORT __cdecl

#endif
