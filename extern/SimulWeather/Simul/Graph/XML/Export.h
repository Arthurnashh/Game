#ifndef SIMUL_XML_EXPORT_H
#define SIMUL_XML_EXPORT_H
#if defined(_MSC_VER) && !defined(SIMUL_XML_DLL)
	#ifdef _DEBUG
		#ifdef _DLL
			#pragma comment(lib,"SimulXML_MDd")
		#else
			#pragma comment(lib,"SimulXML_MTd")
		#endif
	#else
		#ifdef _DLL
			#pragma comment(lib,"SimulXML_MD")
		#else
			#pragma comment(lib,"SimulXML_MT")
		#endif
	#endif
#endif


#ifdef SIMUL_XML_DLL
#define SIMUL_XML_EXPORT __declspec(dllexport)
#else
#ifdef SIMUL_DLL
#define SIMUL_XML_EXPORT __declspec(dllimport)
#else
#define SIMUL_XML_EXPORT
#endif
#endif
#define SIMUL_XML_EXPORT_CLASS class SIMUL_XML_EXPORT
#endif