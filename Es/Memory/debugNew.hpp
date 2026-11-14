//#ifdef _WIN32
#if ALLOC_DEBUGGER
	#ifdef MFC_NEW
		#define new DEBUG_NEW
	#else
		//#define new debugNew
	#endif
#endif
//#endif

