#pragma once

// calling arrangement - first parameter is always in integer - it is the function pointer.
enum 
{
	CALLX64___=0,
	CALLX64__F=4,
	CALLX64_F_=2,
	CALLX64F__=1,
	CALLX64III=CALLX64___,
	CALLX64IIF=CALLX64__F,
	CALLX64IFI=CALLX64_F_,
	CALLX64FII=CALLX64F__,
	CALLX64IFF=CALLX64_F_|CALLX64__F,
	CALLX64FIF=CALLX64F__|CALLX64__F,
	CALLX64FFI=CALLX64F__|CALLX64_F_,
	CALLX64FFF=CALLX64F__|CALLX64_F_|CALLX64__F,
};

extern "C"
{
	unsigned __int64 Callx64(void *func);
	unsigned __int64 Callx64iii(void *func,unsigned __int64 arg1,unsigned __int64 arg2	,unsigned __int64 arg3);
	unsigned __int64 Callx64iif(void *func,unsigned __int64 arg1,unsigned __int64 arg2	,float f3);
	unsigned __int64 Callx64ifi(void *func,unsigned __int64 arg1,float f2				,unsigned __int64 arg3);
	unsigned __int64 Callx64fii(void *func,float f1				,unsigned __int64 arg2	,unsigned __int64 arg3);
	unsigned __int64 Callx64iff(void *func,unsigned __int64 arg1,float f2				,float f3);
	unsigned __int64 Callx64fif(void *func,float f1				,unsigned __int64 arg2	,float f3);
	unsigned __int64 Callx64ffi(void *func,float f1				,float f2				,unsigned __int64 arg3);
	unsigned __int64 Callx64fff(void *func,float f1				,float f2				,float f3);
	void ReturnFloatx64(float *addr);
}