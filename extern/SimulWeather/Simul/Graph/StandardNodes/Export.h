#ifndef SIMUL_STD_EXPORT_H
#define SIMUL_STD_EXPORT_H

#if defined(_MSC_VER) && !defined(SIMUL_STD_DLL)
	#ifdef _DEBUG
		#ifdef _DLL
			#pragma comment(lib,"SimulStandardNodes_MDd")
		#else
			#pragma comment(lib,"SimulStandardNodes_MTd")
		#endif
	#else
		#ifdef _DLL
			#pragma comment(lib,"SimulStandardNodes_MD")
		#else
			#pragma comment(lib,"SimulStandardNodes_MT")
		#endif
	#endif
#endif

#ifdef SIMUL_STD_DLL
	#define SIMUL_STD_EXPORT __declspec(dllexport)
#else
	#ifdef SIMUL_DLL
		#define SIMUL_STD_EXPORT __declspec(dllimport)
	#else
		#define SIMUL_STD_EXPORT
	#endif
#endif

#define SIMUL_STD_EXPORT_CLASS class SIMUL_STD_EXPORT
#define SIMUL_STD_EXPORT_STRUCT struct SIMUL_STD_EXPORT

#endif