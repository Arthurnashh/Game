#ifndef SIMUL_EXPORTTABLE_EXPORT_H
	#define SIMUL_EXPORTTABLE_EXPORT_H
	#ifndef DONT_AUTOLOAD_EXPORTTABLE
		#if defined(_MSC_VER) && !defined(EXPORTTABLE_DLL)
			#ifdef _DEBUG
				#ifdef _DLL
					#pragma comment(lib,"SimulExportTable_MDd")
				#else
					#pragma comment(lib,"SimulExportTable_MTd")
				#endif
			#else
				#ifdef _DLL
					#pragma comment(lib,"SimulExportTable_MD")
				#else
					#pragma comment(lib,"SimulExportTable_MT")
				#endif
			#endif
		#endif
	#endif

	#ifdef EXPORTTABLE_DLL
		#define EXPORTTABLE_EXPORT __declspec(dllexport)
	#else
		#ifdef SIMUL_DLL
			#define EXPORTTABLE_EXPORT __declspec(dllimport)
		#else
			#define EXPORTTABLE_EXPORT
		#endif
	#endif

#endif