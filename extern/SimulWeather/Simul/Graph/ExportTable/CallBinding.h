#ifndef CALLBINDING_H
#define CALLBINDING_H

#include "Export.h"
namespace simul
{
	namespace graph
	{
		namespace exporttable
		{
#ifdef WIN64
			typedef unsigned  __int64 int_or_ptr;
#else
			typedef unsigned  long int_or_ptr;
#endif
#ifdef WIN32
			extern EXPORTTABLE_EXPORT float ReturnFloat_thiscall(void *this_ptr,void* args,size_t sz,void *func);
			extern EXPORTTABLE_EXPORT float ReturnFloat_cdecl(void* args,size_t sz,void *func);
			extern EXPORTTABLE_EXPORT float ReturnFloat_stdcall(void* args,size_t sz,void *func);
			extern EXPORTTABLE_EXPORT int_or_ptr Call_thiscall(void *this_ptr,void* args,size_t sz,void* func );
			extern EXPORTTABLE_EXPORT int_or_ptr Call_cdecl(const void* args,size_t sz,void* func );
			extern EXPORTTABLE_EXPORT int_or_ptr Call_stdcall( void* args,size_t sz,void* func );
#endif
#if 0//def WIN64
			extern EXPORTTABLE_EXPORT int_or_ptr Call_win64_call(void* args,size_t sz,void* func,float *fargs=0,size_t num_floats=0);
			extern EXPORTTABLE_EXPORT int_or_ptr Call_win64_thiscall(void *this_ptr,void* args,size_t sz,void* func,float *fargs=0,size_t num_floats=0);
			extern EXPORTTABLE_EXPORT float ReturnFloat_win64_call(void* args,size_t sz,void *func,float *fargs=0,size_t num_floats=0);
#endif
		}
	}
}
#endif
