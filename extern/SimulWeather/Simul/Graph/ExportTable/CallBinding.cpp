// Portions of this code Copyright (C) Scott Bilas,2000

#include "Simul/Graph/ExportTable/CallBinding.h"
#ifdef WIN32
#include <windows.h>
#else
typedef unsigned int DWORD;
#endif
#include <iostream>
#include <vector>
#include "Callx64.h"

using namespace simul::graph::exporttable;


// Code from "Calling Conventions" Section
//
#ifdef WIN32
int_or_ptr simul::graph::exporttable::Call_cdecl(const void* args,size_t sz,void* func )
{
    unsigned long rc;		// here's our return value...
    __asm
    {
        mov   ecx,sz       // get size of buffer
        mov   esi,args     // get buffer
        sub   esp,ecx      // allocate stack space
        mov   edi,esp      // start of destination stack frame
        shr   ecx,2        // make it dwords
        rep   movsd        // copy params to real stack
        call  [func]       // call the function
        mov   rc, eax      // save the return value
        add   esp,sz       // restore the stack pointer
    }
    return ( rc );
}

int_or_ptr simul::graph::exporttable::Call_stdcall( void* args,size_t sz,void* func )
{
    unsigned long rc;		// here's our return value...
    __asm
    {
        mov   ecx,sz       // get size of buffer
        mov   esi,args     // get buffer
        sub   esp,ecx      // allocate stack space
        mov   edi,esp      // start of destination stack frame
        shr   ecx,2        // make it dwords
        rep   movsd         // copy it
        call  [func]        // call the function
        mov   rc, eax      // save the return value
    }
    return ( rc );
}
// RVK: thiscall is a C++ nonstatic member function call.
// It is exactly the same as stdcall except additionally store the 'this' pointer in ecx.
int_or_ptr simul::graph::exporttable::Call_thiscall(void *this_ptr,void* args,size_t sz,void *func )
{
    unsigned long rc;		// here's our return value...
    __asm
	{
        mov   ecx,sz       // get size of buffer
        mov   esi,args     // get buffer
        sub   esp,ecx      // allocate stack space
        mov   edi,esp      // start of destination stack frame
        shr   ecx,2        // make it dwords
        rep   movsd         // copy it
        mov   ecx,this_ptr // thiscall
        call  [func]        // call the function
        mov   rc, eax      // save the return value
    }
    return ( rc );
}

float simul::graph::exporttable::ReturnFloat_thiscall(void *this_ptr,void* args,size_t sz,void *func )
{
	float float_result=0.f;
	Call_thiscall(this_ptr,args,sz,func);
	_asm
	{
		fstp float_result
	}
	return float_result;
}
#endif

#if 0//def WIN64
float simul::graph::exporttable::ReturnFloat_win64_call(void* args,size_t sz,void *func,float *fargs,size_t num_floats)
{
	float float_result=0.f;
	Call_win64_call(args,sz,func,fargs,num_floats);
#ifdef WIN32
	_asm
	{
		fstp float_result
	}
#endif
#ifdef WIN64
	ReturnFloatx64(&float_result);
#endif
	return float_result;
}
#endif

#ifdef WIN32
float simul::graph::exporttable::ReturnFloat_stdcall(void* args,size_t sz,void *func)
{
	float float_result=0.f;
	Call_stdcall(args,sz,func);
	_asm
	{
		fstp float_result
	}
	return float_result;
}
float simul::graph::exporttable::ReturnFloat_cdecl(void* args,size_t sz,void *func)
{
	float float_result=0.f;
	Call_cdecl(args,sz,func);
	_asm
	{
		fstp float_result
	}
	return float_result;
}
#endif

#if 0//def WIN64
int_or_ptr simul::graph::exporttable::Call_win64_call(void* args,size_t sz,void* func,float *fargs,size_t num_floats)
{
	unsigned __int64 ar[]={0,0,0};
	unsigned __int64 *arg64=(unsigned __int64*)args;
	float fl[]={0,0,0};
	for(size_t i=0;i<sz;i++)
	{
		ar[i]=arg64[i];
	}
	for(size_t i=0;i<num_floats;i++)
	{
		fl[i]=fargs[i];
	}
	return Callx64(func,ar[0],ar[1],ar[2],fl[0],fl[1],fl[2]);
}
#endif

#if 0//def WIN64
int_or_ptr simul::graph::exporttable::Call_win64_thiscall(void *this_ptr,void* args,size_t sz,void* func,float *fargs,size_t num_floats)
{
	unsigned __int64 ar[]={0,0,0};
	unsigned __int64 *arg64=(unsigned __int64*)args;
	float fl[]={0,0,0};
	for(size_t i=0;i<sz;i++)
	{
		ar[i]=arg64[i];
	}
	for(size_t i=0;i<num_floats;i++)
	{
		fl[i]=fargs[i];
	}
	return Callx64(func,(unsigned __int64)this_ptr,ar[0],ar[1],0.f,fl[0],fl[1]);
}
#endif


#ifdef WIN32
__declspec ( naked )
#endif
DWORD GetST0( void )
{
#ifdef WIN32
    DWORD f;               // temp var
    __asm
    {
        fstp dword ptr [f]      // pop ST0 into f
        mov eax,dword ptr [f]  // copy into eax
        ret                     // done
    }
#endif
#ifdef WIN64
    DWORD f=0;               // temp var
	// inline asm is not supported: use compiler intrinsics instead:
	// func is the address of a cdecl function. sz is the number of arguments and args points to the argument list.
	return f;
#endif
}


