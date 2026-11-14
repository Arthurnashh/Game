#ifndef SIMUL_TERRAIN_EXPORT_H
#define SIMUL_TERRAIN_EXPORT_H

#if defined(_MSC_VER) && !defined(SIMUL_TERRAIN_DLL)
	#ifdef _DEBUG
		#ifdef _DLL
			#pragma comment(lib,"SimulTerrain_MDd")
		#else
			#pragma comment(lib,"SimulTerrain_MTd")
		#endif
	#else
		#ifdef _DLL
			#pragma comment(lib,"SimulTerrain_MD")
		#else
			#pragma comment(lib,"SimulTerrain_MT")
		#endif
	#endif
#endif

#if defined(DOXYGEN) || defined(SN_TARGET_PS3)
	#define SIMUL_TERRAIN_EXPORT
#else
	// In this lib:
	#if !defined(SIMUL_TERRAIN_DLL) && defined(SIMUL_DLL)
	// If we're building dll libraries but not in this library IMPORT the classes
		#define SIMUL_TERRAIN_EXPORT __declspec(dllimport)
	#else
	// In ALL OTHER CASES we EXPORT the classes!
		#define SIMUL_TERRAIN_EXPORT __declspec(dllexport)
	#endif
#endif
#define SIMUL_TERRAIN_EXPORT_CLASS class SIMUL_TERRAIN_EXPORT
#define SIMUL_TERRAIN_EXPORT_STRUCT struct SIMUL_TERRAIN_EXPORT
#endif
