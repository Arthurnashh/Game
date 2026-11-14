#ifndef SIMUL_MATH_EXPORT_H
#define SIMUL_MATH_EXPORT_H

#if defined(_MSC_VER) && !defined(SIMUL_MATH_DLL)
	#ifdef _DEBUG
		#ifdef _DLL
			#pragma comment(lib,"SimulMath_MDd")
		#else
			#pragma comment(lib,"SimulMath_MTd")
		#endif
	#else
		#ifdef _DLL
			#pragma comment(lib,"SimulMath_MD")
		#else
			#pragma comment(lib,"SimulMath_MT")
		#endif
	#endif
#endif

#if defined(DOXYGEN) || defined(SN_TARGET_PS3) || defined(__GNUC__)
	#define SIMUL_MATH_EXPORT
#else
// In this lib:
	#if !defined(SIMUL_MATH_DLL) && defined(SIMUL_DLL)
	// If we're building dll libraries but not in this library IMPORT the classes
		#define SIMUL_MATH_EXPORT __declspec(dllimport)
	#else
	// In ALL OTHER CASES we EXPORT the classes!
		#define SIMUL_MATH_EXPORT __declspec(dllexport)
	#endif
#endif

#ifdef _MSC_VER
	#define SIMUL_MATH_EXPORT_FN		SIMUL_MATH_EXPORT __cdecl
#else
	#define SIMUL_MATH_EXPORT_FN		SIMUL_MATH_EXPORT
#endif
#define SIMUL_MATH_EXPORT_CLASS		class SIMUL_MATH_EXPORT
#define SIMUL_MATH_EXPORT_STRUCT	struct SIMUL_MATH_EXPORT

#endif
