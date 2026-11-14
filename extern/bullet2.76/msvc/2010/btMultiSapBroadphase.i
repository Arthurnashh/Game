/*
 * SNC Preprocessed Output:
 * "e:\EnforceNG\bullet-2.76\src\BulletCollision\BroadphaseCollision\btMultiSapBroadphase.cpp"
 * "SN Systems ps3ppusnc - 'ps3ppucf.exe' 440.1.11914.0 (rel,TR,3.3 @184299) WIN32"
 * "-XYc=cp+gnu_ext+rtti-exceptions+bool+wchar_t+array_nd+tmplname -XYg -DSN_TARGET_PS3 -D_DEBUG -D__SNC__ -DREMOTE_PATH="/app_home/" -D_DEBUG -D_IRR_STATIC_LIB_ -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS -DCMAKE_INTDIR="Debug" -D__PS3__ -Xdepmode=1 -Xmserrors --diag_suppress=1700 --diag_suppress=68 --diag_suppress=1011 --diag_suppress=178 --diag_suppress=613 -O0 -Xfastmath=1 -Xpreprocess --display_error_number --alternative_tokens -D_BIG_ENDIAN=1 -D__BIG_ENDIAN__ -D__ALTIVEC__ -JC:\PS3SDK/host-win32/sn/ppu/include -JC:\PS3SDK/host-win32/sn/ppu/include/sn -JC:\PS3SDK/host-win32/sn/common/include -JC:\PS3SDK/host-win32/sn/common/include/sn -JC:\PS3SDK/target/ppu/include -JC:\PS3SDK/target/common/include -JC:\Program Files (x86)\SN Systems\PS3/ppu/include -JC:\Program Files (x86)\SN Systems\PS3/ppu/include/sn -JC:\Program Files (x86)\SN Systems\PS3/common/include -JC:\Program Files (x86)\SN Systems\PS3/common/include/sn -M -MO -E -f"
 *
 */

# 1 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp"













 

# 1 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.h"













 



# 1 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btBroadphaseInterface.h"













 






struct btDispatcherInfo;
class btDispatcher;
# 1 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btBroadphaseProxy.h"













 




# 1 "..\\..\\src\\LinearMath/btScalar.h"












 
 











# 1 "C:/PS3SDK\\target\\ppu\\include/math.h"




 
 
# 1 "C:/PS3SDK\\target\\ppu\\include/ymath.h"




 
 
# 1 "C:/PS3SDK\\target\\ppu\\include/yvals.h"




 
 
# 1 "C:/PS3SDK\\target\\ppu\\include/sys/types.h"
 
 
 
 




# 1 "C:/PS3SDK\\target\\ppu\\include/sys/syscall.h"
 
 
 
 




# 248 "C:/PS3SDK\\target\\ppu\\include/sys/syscall.h"



 
# 262 "C:/PS3SDK\\target\\ppu\\include/sys/syscall.h"

# 1 "C:/PS3SDK\\target\\ppu\\include/sys/types.h"
 
 
 
 

# 264 "C:/PS3SDK\\target\\ppu\\include/sys/syscall.h"




 










































 






 
# 326 "C:/PS3SDK\\target\\ppu\\include/sys/syscall.h"

# 525 "C:/PS3SDK\\target\\ppu\\include/sys/syscall.h"



 







# 10 "C:/PS3SDK\\target\\ppu\\include/sys/types.h"
# 1 "C:/PS3SDK\\target\\common\\include/sys/sys_types.h"
 
 
 
 




# 1 "C:/PS3SDK\\target\\ppu\\include/sys/cdefs.h"
 
 
 
 




# 20 "C:/PS3SDK\\target\\ppu\\include/sys/cdefs.h"

# 32 "C:/PS3SDK\\target\\ppu\\include/sys/cdefs.h"






# 10 "C:/PS3SDK\\target\\common\\include/sys/sys_types.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/sys/integertypes.h"
 
 
 
 






namespace std { typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;


typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef signed int intptr_t;
typedef unsigned int uintptr_t;
# 29 "C:/PS3SDK\\target\\ppu\\include/sys/integertypes.h"

}

	using ::std:: int8_t;
using ::std:: uint8_t;
using ::std:: int16_t;
using ::std:: uint16_t;
using ::std:: int32_t;
using ::std:: uint32_t;
using ::std:: int64_t;
using ::std:: uint64_t;
using ::std:: intptr_t;
using ::std:: uintptr_t;



# 11 "C:/PS3SDK\\target\\common\\include/sys/sys_types.h"







 
typedef ::std:: uint64_t callout_t;
typedef ::std:: uint64_t timer_t;



 
namespace std {
# 35 "C:/PS3SDK\\target\\common\\include/sys/sys_types.h"
typedef ::std:: uint32_t size_t;
typedef ::std:: int32_t off_t;




typedef ::std:: int64_t off64_t;
typedef ::std:: int64_t time_t;




}


 
 
typedef ::std:: uint32_t sys_pid_t;

 
typedef ::std:: uint32_t sys_program_segment_t;

 
typedef ::std:: uint32_t sys_overlay_t;




 
typedef ::std:: uint64_t sys_ppu_thread_t;

 
typedef ::std:: uint32_t sys_interrupt_tag_t;



 
typedef ::std:: uint32_t sys_mutex_t;
typedef ::std:: uint32_t sys_cond_t;
typedef ::std:: uint32_t sys_shared_mutex_t;
typedef ::std:: uint32_t sys_sync_point_t;
typedef ::std:: uint32_t sys_rwlock_t;

typedef ::std:: uint32_t sys_protocol_t;
typedef ::std:: uint32_t sys_recursive_t;
typedef ::std:: uint32_t sys_process_shared_t;
typedef ::std:: uint32_t sys_adaptive_t;

typedef ::std:: uint32_t _sys_sleep_queue_t;
typedef ::std:: uint32_t _sys_lwcond_queue_t;
typedef ::std:: uint32_t sys_semaphore_t;
typedef ::std:: uint32_t sys_event_flag_t;




 
typedef ::std:: uint32_t sys_event_queue_t;
typedef ::std:: uint32_t sys_event_port_t;
typedef ::std:: uint32_t sys_event_type_t;



 
typedef ::std:: uint32_t sys_spu_thread_group_t;
typedef ::std:: uint32_t sys_spu_thread_t;



 
typedef ::std:: uint32_t sys_spu_system_module_t;




 







 
typedef ::std:: uint32_t sys_timer_t;




 
typedef ::std:: int64_t system_time_t;
typedef ::std:: uint64_t usecond_t;
typedef ::std:: uint32_t second_t;

 
typedef ::std:: uint64_t sys_ipc_key_t;



 
typedef ::std:: uintptr_t lparaddr_t;
typedef ::std:: uintptr_t sys_addr_t;
typedef ::std:: uint32_t sys_memory_t;
typedef ::std:: uint32_t sys_memory_container_t;




 
typedef ::std:: uint32_t sys_trace_t;
typedef ::std:: uint32_t sys_trace_buffer_t;





 
namespace std { typedef struct sys_ppu_guid_info {
	const char magic[5];		 
	const ::std:: uint8_t id_revision;	 
	const ::std:: uint16_t reserved1;	 
	const ::std:: uint32_t ppu_guid[5];	 
	const ::std:: uint32_t reserved2;	 
} sys_ppu_guid_info_t;

}


 
	namespace std {


typedef int32_t ssize_t;



typedef int mode_t;
typedef long int useconds_t;
typedef long key_t;
typedef uint32_t pid_t;
typedef unsigned long dev_t;
typedef unsigned long ino_t;
typedef unsigned long nlink_t;
typedef long blkcnt_t;
typedef long blksize_t;


}

	using ::std:: size_t;

using ::std:: off_t;
using ::std:: off64_t;
using ::std:: ssize_t;
using ::std:: mode_t;
using ::std:: useconds_t;
using ::std:: key_t;
using ::std:: dev_t;
using ::std:: ino_t;
using ::std:: nlink_t;
using ::std:: blkcnt_t;
using ::std:: blksize_t;
using ::std:: time_t;

using ::std:: sys_ppu_guid_info_t;




# 11 "C:/PS3SDK\\target\\ppu\\include/sys/types.h"

 

# 10 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

# 24 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

# 1 "C:/PS3SDK\\target\\ppu\\include/stdarg.h"




 





typedef char *   va_list;


  extern  "C"
  {

    extern char  *__va_funcx(int, va_list, ...) ;  

  }


# 28 "C:/PS3SDK\\target\\ppu\\include/stdarg.h"

# 39 "C:/PS3SDK\\target\\ppu\\include/stdarg.h"

# 26 "C:/PS3SDK\\target\\ppu\\include/yvals.h"












































 

 

 



 
 




















		 

# 109 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

# 119 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

# 127 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

		 


typedef int _Int32t;
typedef unsigned int _Uint32t;







typedef int _Ptrdifft;






typedef unsigned int _Sizet;









		 
# 165 "C:/PS3SDK\\target\\ppu\\include/yvals.h"



		 
 
 



















		 












 


 

		 
# 216 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

# 223 "C:/PS3SDK\\target\\ppu\\include/yvals.h"







# 236 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

		 




namespace std {}





# 278 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

# 287 "C:/PS3SDK\\target\\ppu\\include/yvals.h"






# 309 "C:/PS3SDK\\target\\ppu\\include/yvals.h"












# 327 "C:/PS3SDK\\target\\ppu\\include/yvals.h"





# 350 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

# 369 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

 




     
 
# 387 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

# 410 "C:/PS3SDK\\target\\ppu\\include/yvals.h"


namespace std {
typedef bool _Bool;
}


		 



# 427 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

namespace std {
		 






		 
















typedef long long _Longlong;
typedef unsigned long long _ULonglong;

# 462 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

		 



typedef wchar_t _Wchart;


# 482 "C:/PS3SDK\\target\\ppu\\include/yvals.h"
  




  




typedef int _Wintt;

# 506 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

		 


		 




		 
typedef va_list _Va_list;



# 526 "C:/PS3SDK\\target\\ppu\\include/yvals.h"



		 


extern "C" {
void _Atexit(void (*)(void));
}

		 









typedef char _Sysch_t;

		 


		 
 


}

extern "C" {
# 566 "C:/PS3SDK\\target\\ppu\\include/yvals.h"
typedef void *_Rmtx;

}

		 


extern "C" {
void _Locksyslock(int);
void _Unlocksyslock(int);
}






		 
# 594 "C:/PS3SDK\\target\\ppu\\include/yvals.h"










namespace std {
extern "C++" {	
		
class _Lockit
	{	
public:

# 628 "C:/PS3SDK\\target\\ppu\\include/yvals.h"

	explicit _Lockit()
		: _Locktype(1)
		{	
		
			_Locksyslock(_Locktype);
		}

	explicit _Lockit(int _Type)
		: _Locktype(_Type)
		{	
		
			_Locksyslock(_Locktype);
		}

	~_Lockit()
		{	
		
			_Unlocksyslock(_Locktype);
		}

private:
	int _Locktype;
public:
	_Lockit(const _Lockit&);			
	_Lockit& operator=(const _Lockit&);	


	};

class _Mutex
	{	
public:

# 672 "C:/PS3SDK\\target\\ppu\\include/yvals.h"
	_Mutex();
	~_Mutex();
	void _Lock();
	void _Unlock();

private:
	_Mutex(const _Mutex&);				
	_Mutex& operator=(const _Mutex&);	
	_Rmtx *_Mtx;


	};
}	
}


		 


















namespace std {
using ::va_list;
}





# 723 "C:/PS3SDK\\target\\ppu\\include/yvals.h"






 
# 10 "C:/PS3SDK\\target\\ppu\\include/ymath.h"
namespace std {
extern "C" {

		 
# 27 "C:/PS3SDK\\target\\ppu\\include/ymath.h"

		 





		 







		 
typedef union
	{	 
	unsigned short _Word[8];
	float _Float;
	double _Double;
	long double _Long_double;
	} _Dconst;

		 
void _Feraise(int);

		 
double _Cosh(double, double);
short _Dtest(double *);
short _Exp(double *, double, short);
double _Log(double, int);
double _Sin(double, unsigned int);
double _Sinh(double, double);
extern   _Dconst _Denorm, _Hugeval, _Inf,
	_Nan, _Snan;

		 
float _FCosh(float, float);
short _FDtest(float *);
short _FExp(float *, float, short);
float _FLog(float, int);
float _FSin(float, unsigned int);
float _FSinh(float, float);
extern   _Dconst _FDenorm, _FInf, _FNan, _FSnan;

		 
long double _LCosh(long double, long double);
short _LDtest(long double *);
short _LExp(long double *, long double, short);
long double _LLog(long double, int);
long double _LSin(long double, unsigned int);
long double _LSinh(long double, long double);
extern   _Dconst _LDenorm, _LInf, _LNan, _LSnan;

}
}





 
# 11 "C:/PS3SDK\\target\\ppu\\include/math.h"


namespace std {

		 




		 











		 

# 43 "C:/PS3SDK\\target\\ppu\\include/math.h"
typedef float float_t;
typedef double double_t;


		 

 
# 68 "C:/PS3SDK\\target\\ppu\\include/math.h"

# 75 "C:/PS3SDK\\target\\ppu\\include/math.h"




















extern "C" {
int _FFpcomp(float, float);
int _Fpcomp(double, double);
int _LFpcomp(long double, long double);

int _FDclass(float);
int _Dclass(double);
int _LDclass(long double);

int _FDsign(float);
int _Dsign(double);
int _LDsign(long double);
}









		
template<class _Ty>
	struct _Rc_type
	{	
	typedef float _Type;	
	};

		
template<class _Ty, class _T2>
	struct _Rc_widened
	{	
	typedef float _Type;	
	};

		

# 163 "C:/PS3SDK\\target\\ppu\\include/math.h"
template<class _Ty>
	struct _Real_type
	{	
	typedef double _Type;	
	};


template<> struct _Real_type<float>
	{	
	typedef float _Type;
	};

template<> struct _Real_type<long double>
	{	
	typedef long double _Type;
	};

		
template<class _Ty, class _T2>
	struct _Real_widened
	{	
	typedef long double _Type;	
	};

template<> struct _Real_widened<float, float>
	{	
	typedef float _Type;
	};

template<> struct _Real_widened<float, double>
	{	
	typedef double _Type;
	};

template<> struct _Real_widened<double, float>
	{	
	typedef double _Type;
	};

template<> struct _Real_widened<double, double>
	{	
	typedef double _Type;
	};

		
template<class _Trc, class _Tre>
	struct _Combined_type
	{	
	typedef float _Type;	
	};

template<> struct _Combined_type<float, double>
	{	
	typedef double _Type;
	};

template<> struct _Combined_type<float, long double>
	{	
	typedef long double _Type;
	};

		
inline int _FPCOMP(float _Left, float _Right)
	{	
	return (_FFpcomp(_Left, _Right));
	}

inline int _FPCOMP(double _Left, double _Right)
	{	
	return (_Fpcomp(_Left, _Right));
	}

inline int _FPCOMP(long double _Left, long double _Right)
	{	
	return (_LFpcomp(_Left, _Right));
	}

template<class _T1, class _T2> inline
	int _FPCOMP(_T1 _Left, _T2 _Right)
	{	
	typedef typename _Combined_type<float,
		typename _Real_widened<
			typename _Real_type<_T1>::_Type,
			typename _Real_type<_T2>::_Type>::_Type>::_Type _Tw;
	return (_FPCOMP((_Tw)_Left, (_Tw)_Right));
	}

		
inline int fpclassify(float _Left)
	{	
	return (_FDtest(&_Left));
	}

inline int fpclassify(double _Left)
	{	
	return (_Dtest(&_Left));
	}

inline int fpclassify(long double _Left)
	{	
	return (_LDtest(&_Left));
	}

		
inline bool signbit(float _Left)
	{	
	return (_FDsign(_Left) != 0);
	}

inline bool signbit(double _Left)
	{	
	return (_Dsign(_Left) != 0);
	}

inline bool signbit(long double _Left)
	{	
	return (_LDsign(_Left) != 0);
	}

template<class _Ty> inline
	bool isfinite(_Ty _Left)
	{	
	return (fpclassify(_Left) <= 0);
	}

template<class _Ty> inline
	bool isinf(_Ty _Left)
	{	
	return (fpclassify(_Left) == 1);
	}

template<class _Ty> inline
	bool isnan(_Ty _Left)
	{	
	return (fpclassify(_Left) == 2);
	}

template<class _Ty> inline
	bool isnormal(_Ty _Left)
	{	
	return (fpclassify(_Left) == (-1));
	}

template<class _Ty1, class _Ty2> inline
	bool isgreater(_Ty1 _Left, _Ty2 _Right)
	{	
	return ((_FPCOMP(_Left, _Right) & 4) != 0);
	}

template<class _Ty1, class _Ty2> inline
	bool isgreaterequal(_Ty1 _Left, _Ty2 _Right)
	{	
	return ((_FPCOMP(_Left, _Right) & (2 | 4)) != 0);
	}

template<class _Ty1, class _Ty2> inline
	bool isless(_Ty1 _Left, _Ty2 _Right)
	{	
	return ((_FPCOMP(_Left, _Right) & 1) != 0);
	}

template<class _Ty1, class _Ty2> inline
	bool islessequal(_Ty1 _Left, _Ty2 _Right)
	{	
	return ((_FPCOMP(_Left, _Right) & (1 | 2)) != 0);
	}

template<class _Ty1, class _Ty2> inline
	bool islessgreater(_Ty1 _Left, _Ty2 _Right)
	{	
	return ((_FPCOMP(_Left, _Right) & (1 | 4)) != 0);
	}

template<class _Ty1, class _Ty2> inline
	bool isunordered(_Ty1 _Left, _Ty2 _Right)
	{	
	return (_FPCOMP(_Left, _Right) == 0);
	}

# 354 "C:/PS3SDK\\target\\ppu\\include/math.h"

# 427 "C:/PS3SDK\\target\\ppu\\include/math.h"






extern "C" {
		 
double acos(double);
double asin(double);
double atan(double);
double atan2(double, double);
double ceil(double);
double exp(double);
double fabs(double);
double floor(double);
double fmod(double, double);
double frexp(double, int *);
double ldexp(double, int);
double modf(double, double *);
double pow(double, double);
double sqrt(double);
double tan(double);
double tanh(double);


double acosh(double);
double asinh(double);
double atanh(double);
double cbrt(double);
double copysign(double, double);
double erf(double);
double erfc(double);
double exp2(double);
double expm1(double);
double fdim(double, double);
double fma(double, double, double);
double fmax(double, double);
double fmin(double, double);
double hypot(double, double);
int ilogb(double);
double lgamma(double);
_Longlong llrint(double);
_Longlong llround(double);
double log1p(double);
double logb(double);
long lrint(double);
long lround(double);
double nan(const char *);
double nearbyint(double);
double nextafter(double, double);
double nexttoward(double, long double);
double remainder(double, double);
double remquo(double, double, int *);
double rint(double);
double round(double);
double scalbn(double, int);
double scalbln(double, long);
double tgamma(double);
double trunc(double);


		 
float acosf(float);
float asinf(float);
float atanf(float);
float atan2f(float, float);
float ceilf(float);
float expf(float);
float fabsf(float);
float floorf(float);
float fmodf(float, float);
float frexpf(float, int *);
float ldexpf(float, int);
float modff(float, float *);
float powf(float, float);
float sqrtf(float);
float tanf(float);
float tanhf(float);


float acoshf(float);
float asinhf(float);
float atanhf(float);
float cbrtf(float);
float copysignf(float, float);
float erff(float);
float erfcf(float);
float expm1f(float);
float exp2f(float);
float fdimf(float, float);
float fmaf(float, float, float);
float fmaxf(float, float);
float fminf(float, float);
float hypotf(float, float);
int ilogbf(float);
float lgammaf(float);
_Longlong llrintf(float);
_Longlong llroundf(float);
float log1pf(float);
float logbf(float);
long lrintf(float);
long lroundf(float);
float nanf(const char *);
float nearbyintf(float);
float nextafterf(float, float);
float nexttowardf(float, long double);
float remainderf(float, float);
float remquof(float, float, int *);
float rintf(float);
float roundf(float);
float scalbnf(float, int);
float scalblnf(float, long);
float tgammaf(float);
float truncf(float);


		 
long double acosl(long double);
long double asinl(long double);
long double atanl(long double);
long double atan2l(long double, long double);
long double ceill(long double);
long double expl(long double);
long double fabsl(long double);
long double floorl(long double);
long double fmodl(long double, long double);
long double frexpl(long double, int *);
long double ldexpl(long double, int);
long double modfl(long double, long double *);
long double powl(long double, long double);
long double sqrtl(long double);
long double tanl(long double);
long double tanhl(long double);


long double acoshl(long double);
long double asinhl(long double);
long double atanhl(long double);
long double cbrtl(long double);
long double copysignl(long double, long double);
long double erfl(long double);
long double erfcl(long double);
long double exp2l(long double);
long double expm1l(long double);
long double fdiml(long double, long double);
long double fmal(long double, long double, long double);
long double fmaxl(long double, long double);
long double fminl(long double, long double);
long double hypotl(long double, long double);
int ilogbl(long double);
long double lgammal(long double);
_Longlong llrintl(long double);
_Longlong llroundl(long double);
long double log1pl(long double);
long double logbl(long double);
long lrintl(long double);
long lroundl(long double);
long double nanl(const char *);
long double nearbyintl(long double);
long double nextafterl(long double, long double);
long double nexttowardl(long double, long double);
long double remainderl(long double, long double);
long double remquol(long double, long double, int *);
long double rintl(long double);
long double roundl(long double);
long double scalbnl(long double, int);
long double scalblnl(long double, long);
long double tgammal(long double);
long double truncl(long double);


}






		
template<class _Ty> inline
	_Ty _Pow_int(_Ty _Left, int _Right)
	{	
	unsigned int _Num = _Right;
	if (_Right < 0)
		_Num = 0 - _Num;

	for (_Ty _Ans = 1; ; _Left *= _Left)
		{	
		if ((_Num & 1) != 0)
			_Ans *= _Left;
		if ((_Num >>= 1) == 0)
			return (0 <= _Right ? _Ans
				: _Ans == _Ty(0) ? ((double)((float)(1e+30F * 1e+30F))) : _Ty(1) / _Ans);
		}
	}


		
extern "C" {
inline double cos(double _Left)
	{	
	return (_Sin(_Left, 1));
	}

inline double cosh(double _Left)
	{	
	return (_Cosh(_Left, 1));
	}

inline double log(double _Left)
	{	
	return (_Log(_Left, 0));
	}

inline double log10(double _Left)
	{	
	return (_Log(_Left, 1));
	}

inline double sin(double _Left)
	{	
	return (_Sin(_Left, 0));
	}

inline double sinh(double _Left)
	{	
	return (_Sinh(_Left, 1));
	}


inline double log2(double _Left)
	{	
	return (_Log(_Left, -1));
	}


}

inline double abs(double _Left)	
	{	
	return (__builtin_fabs(_Left));
	}

# 687 "C:/PS3SDK\\target\\ppu\\include/math.h"
inline double pow(double _Left, int _Right)
	{	
	return (_Pow_int(_Left, _Right));
	}


		
extern "C" {
inline float cosf(float _Left)
	{	
	return (_FSin(_Left, 1));
	}

inline float coshf(float _Left)
	{	
	return (_FCosh(_Left, 1));
	}

inline float logf(float _Left)
	{	
	return (_FLog(_Left, 0));
	}

inline float log10f(float _Left)
	{	
	return (_FLog(_Left, 1));
	}

inline float sinf(float _Left)
	{	
	return (_FSin(_Left, 0));
	}

inline float sinhf(float _Left)
	{	
	return (_FSinh(_Left, 1));
	}


inline float log2f(float _Left)
	{	
	return (_FLog(_Left, -1));
	}


}

inline float abs(float _Left)	
	{	
	return (__builtin_fabsf(_Left));
	}

inline float acos(float _Left)
	{	
	return (acosf(_Left));
	}

inline float asin(float _Left)
	{	
	return (asinf(_Left));
	}

inline float atan(float _Left)
	{	
	return (atanf(_Left));
	}

inline float atan2(float _Left, float _Right)
	{	
	return (atan2f(_Left, _Right));
	}

inline float ceil(float _Left)
	{	
	return (ceilf(_Left));
	}

inline float cos(float _Left)
	{	
	return (_FSin(_Left, 1));
	}

inline float cosh(float _Left)
	{	
	return (_FCosh(_Left, 1));
	}

inline float exp(float _Left)
	{	
	return (expf(_Left));
	}

inline float fabs(float _Left)
	{	
	return (__builtin_fabsf(_Left));
	}

inline float floor(float _Left)
	{	
	return (floorf(_Left));
	}

inline float fmod(float _Left, float _Right)
	{	
	return (fmodf(_Left, _Right));
	}

inline float frexp(float _Left, int *_Right)
	{	
	return (frexpf(_Left, _Right));
	}

inline float ldexp(float _Left, int _Right)
	{	
	return (ldexpf(_Left, _Right));
	}

inline float log(float _Left)
	{	
	return (_FLog(_Left, 0));
	}

inline float log10(float _Left)
	{	
	return (_FLog(_Left, 1));
	}

inline float modf(float _Left, float *_Right)
	{	
	return (modff(_Left, _Right));
	}

inline float pow(float _Left, float _Right)
	{	
	return (powf(_Left, _Right));
	}

# 840 "C:/PS3SDK\\target\\ppu\\include/math.h"
inline float pow(float _Left, int _Right)
	{	
	return (_Pow_int(_Left, _Right));
	}


inline float sin(float _Left)
	{	
	return (_FSin(_Left, 0));
	}

inline float sinh(float _Left)
	{	
	return (_FSinh(_Left, 1));
	}

inline float sqrt(float _Left)
	{	

	return (sqrtf(_Left));



	}

inline float tan(float _Left)
	{	
	return (tanf(_Left));
	}

inline float tanh(float _Left)
	{	
	return (tanhf(_Left));
	}


inline float acosh(float _Left)
	{	
	return (acoshf(_Left));
	}

inline float asinh(float _Left)
	{	
	return (asinhf(_Left));
	}

inline float atanh(float _Left)
	{	
	return (atanhf(_Left));
	}

inline float cbrt(float _Left)
	{	
	return (cbrtf(_Left));
	}

inline float copysign(float _Left, float _Right)
	{	
	return (copysignf(_Left, _Right));
	}

inline float erf(float _Left)
	{	
 	return (erff(_Left)); }

inline float erfc(float _Left)
	{	
	return (erfcf(_Left));
	}

inline float exp2(float _Left)
	{	
	return (exp2f(_Left));
	}

inline float expm1(float _Left)
	{	
	return (expm1f(_Left));
	}

inline float fdim(float _Left, float _Right)
	{	
	return (fdimf(_Left, _Right));
	}

inline float fma(float _Left, float _Right, float _Addend)
	{	
	return (fmaf(_Left, _Right, _Addend));
	}

inline float fmax(float _Left, float _Right)
	{	
	return (fmaxf(_Left, _Right));
	}

inline float fmin(float _Left, float _Right)
	{	
	return (fminf(_Left, _Right));
	}

inline float hypot(float _Left, float _Right)
	{	
	return (hypotf(_Left, _Right));
	}

inline int ilogb(float _Left)
	{	
	return (ilogbf(_Left));
	}

inline float lgamma(float _Left)
	{	
	return (lgammaf(_Left));
	}

inline _Longlong llrint(float _Left)
	{	
	return (llrintf(_Left));
	}

inline _Longlong llround(float _Left)
	{	
	return (llroundf(_Left));
	}

inline float log1p(float _Left)
	{	
	return (log1pf(_Left));
	}

inline float log2(float _Left)
	{	
	return (_FLog(_Left, -1));
	}

inline float logb(float _Left)
	{	
	return (logbf(_Left));
	}

inline long lrint(float _Left)
	{	
	return (lrintf(_Left));
	}

inline long lround(float _Left)
	{	
	return (lroundf(_Left));
	}

inline float nearbyint(float _Left)
	{	
	return (nearbyintf(_Left));
	}

inline float nextafter(float _Left, float _Right)
	{	
	return (nextafterf(_Left, _Right));
	}

inline float nexttoward(float _Left, long double _Right)
	{	
	return (nexttowardf(_Left, _Right));
	}

inline float remainder(float _Left, float _Right)
	{	
	return (remainderf(_Left, _Right));
	}

inline float remquo(float _Left, float _Right, int *_Pval)
	{	
	return (remquof(_Left, _Right, _Pval));
	}

inline float rint(float _Left)
	{	
	return (rintf(_Left));
	}

inline float round(float _Left)
	{	
	return (roundf(_Left));
	}

inline float scalbn(float _Left, int _Right)
	{	
	return (scalbnf(_Left, _Right));
	}

inline float scalbln(float _Left, long _Right)
	{	
	return (scalblnf(_Left, _Right));
	}

inline float tgamma(float _Left)
	{	
	return (tgammaf(_Left));
	}

inline float trunc(float _Left)
	{	
	return (truncf(_Left));
	}


		
extern "C" {
inline long double cosl(long double _Left)
	{	
	return (_LSin(_Left, 1));
	}

inline long double coshl(long double _Left)
	{	
	return (_LCosh(_Left, 1));
	}

inline long double logl(long double _Left)
	{	
	return (_LLog(_Left, 0));
	}

inline long double log10l(long double _Left)
	{	
	return (_LLog(_Left, 1));
	}

inline long double sinl(long double _Left)
	{	
	return (_LSin(_Left, 0));
	}

inline long double sinhl(long double _Left)
	{	
	return (_LSinh(_Left, 1));
	}


inline long double log2l(long double _Left)
	{	
	return (_LLog(_Left, -1));
	}


}

inline long double abs(long double _Left)	
	{	
	return (fabsl(_Left));
	}

inline long double acos(long double _Left)
	{	
	return (acosl(_Left));
	}

inline long double asin(long double _Left)
	{	
	return (asinl(_Left));
	}

inline long double atan(long double _Left)
	{	
	return (atanl(_Left));
	}

inline long double atan2(long double _Left, long double _Right)
	{	
	return (atan2l(_Left, _Right));
	}

inline long double ceil(long double _Left)
	{	
	return (ceill(_Left));
	}

inline long double cos(long double _Left)
	{	
	return (_LSin(_Left, 1));
	}

inline long double cosh(long double _Left)
	{	
	return (_LCosh(_Left, 1));
	}

inline long double exp(long double _Left)
	{	
	return (expl(_Left));
	}

inline long double fabs(long double _Left)
	{	
	return (fabsl(_Left));
	}

inline long double floor(long double _Left)
	{	
	return (floorl(_Left));
	}

inline long double fmod(long double _Left, long double _Right)
	{	
	return (fmodl(_Left, _Right));
	}

inline long double frexp(long double _Left, int *_Right)
	{	
	return (frexpl(_Left, _Right));
	}

inline long double ldexp(long double _Left, int _Right)
	{	
	return (ldexpl(_Left, _Right));
	}

inline long double log(long double _Left)
	{	
	return (_LLog(_Left, 0));
	}

inline long double log10(long double _Left)
	{	
	return (_LLog(_Left, 1));
	}

inline long double modf(long double _Left, long double *_Right)
	{	
	return (modfl(_Left, _Right));
	}

inline long double pow(long double _Left, long double _Right)
	{	
	return (powl(_Left, _Right));
	}

# 1193 "C:/PS3SDK\\target\\ppu\\include/math.h"
inline long double pow(long double _Left, int _Right)
	{	
	return (_Pow_int(_Left, _Right));
	}


inline long double sin(long double _Left)
	{	
	return (_LSin(_Left, 0));
	}

inline long double sinh(long double _Left)
	{	
	return (_LSinh(_Left, 1));
	}

inline long double sqrt(long double _Left)
	{	
	return (sqrtl(_Left));
	}

inline long double tan(long double _Left)
	{	
	return (tanl(_Left));
	}

inline long double tanh(long double _Left)
	{	
	return (tanhl(_Left));
	}


inline long double acosh(long double _Left)
	{	
	return (acoshl(_Left));
	}

inline long double asinh(long double _Left)
	{	
	return (asinhl(_Left));
	}

inline long double atanh(long double _Left)
	{	
	return (atanhl(_Left));
	}

inline long double cbrt(long double _Left)
	{	
	return (cbrtl(_Left));
	}

inline long double copysign(long double _Left, long double _Right)
	{	
	return (copysignl(_Left, _Right));
	}

inline long double erf(long double _Left)
	{	
	return (erfl(_Left));
	}

inline long double erfc(long double _Left)
	{	
	return (erfcl(_Left));
	}

inline long double exp2(long double _Left)
	{	
	return (exp2l(_Left));
	}

inline long double expm1(long double _Left)
	{	
	return (expm1l(_Left));
	}

inline long double fdim(long double _Left, long double _Right)
	{	
	return (fdiml(_Left, _Right));
	}

inline long double fma(long double _Left, long double _Right,
	long double _Addend)
	{	
	return (fmal(_Left, _Right, _Addend));
	}

inline long double fmax(long double _Left, long double _Right)
	{	
	return (fmaxl(_Left, _Right));
	}

inline long double fmin(long double _Left, long double _Right)
	{	
	return (fminl(_Left, _Right));
	}

inline long double hypot(long double _Left, long double _Right)
	{	
	return (hypotl(_Left, _Right));
	}

inline int ilogb(long double _Left)
	{	
	return (ilogbl(_Left));
	}

inline long double lgamma(long double _Left)
	{	
	return (lgammal(_Left));
	}

inline _Longlong llrint(long double _Left)
	{	
	return (llrintl(_Left));
	}

inline _Longlong llround(long double _Left)
	{	
	return (llroundl(_Left));
	}

inline long double log1p(long double _Left)
	{	
	return (log1pl(_Left));
	}

inline long double log2(long double _Left)
	{	
	return (_LLog(_Left, -1));
	}

inline long double logb(long double _Left)
	{	
	return (logbl(_Left));
	}

inline long lrint(long double _Left)
	{	
	return (lrintl(_Left));
	}

inline long lround(long double _Left)
	{	
	return (lroundl(_Left));
	}

inline long double nearbyint(long double _Left)
	{	
	return (nearbyintl(_Left));
	}

inline long double nextafter(long double _Left, long double _Right)
	{	
	return (nextafterl(_Left, _Right));
	}

inline long double nexttoward(long double _Left, long double _Right)
	{	
	return (nexttowardl(_Left, _Right));
	}

inline long double remainder(long double _Left, long double _Right)
	{	
	return (remainderl(_Left, _Right));
	}

inline long double remquo(long double _Left, long double _Right,
	int *_Pval)
	{	
	return (remquol(_Left, _Right, _Pval));
	}

inline long double rint(long double _Left)
	{	
	return (rintl(_Left));
	}

inline long double round(long double _Left)
	{	
	return (roundl(_Left));
	}

inline long double scalbn(long double _Left, int _Right)
	{	
	return (scalbnl(_Left, _Right));
	}

inline long double scalbln(long double _Left, long _Right)
	{	
	return (scalblnl(_Left, _Right));
	}

inline long double tgamma(long double _Left)
	{	
	return (tgammal(_Left));
	}

inline long double trunc(long double _Left)
	{	
	return (truncl(_Left));
	}


# 1465 "C:/PS3SDK\\target\\ppu\\include/math.h"

 
 






}


 
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"




 



 
# 1 "C:/PS3SDK\\host-win32\\sn\\ppu\\include/altivec.h"






 












# 61 "C:/PS3SDK\\host-win32\\sn\\ppu\\include/altivec.h"

# 12 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"

# 1 "C:/PS3SDK\\target\\ppu\\include/vec_types.h"





 



 
# 1 "C:/PS3SDK\\target\\ppu\\include/vec_types_snc.h"
 
 
 
 




typedef vector unsigned char  qword;

typedef vector float          vec_float4;

typedef vector pixel          vec_pixel8;

typedef vector bool char      vec_bchar16;
typedef vector bool short     vec_bshort8;
typedef vector bool int       vec_bint4;
typedef vector bool int       vec_double2;

typedef vector signed char    vec_char16;
typedef vector signed short   vec_short8;
typedef vector signed int     vec_int4;
typedef vector bool short     vec_llong2;

typedef vector unsigned char  vec_uchar16;
typedef vector unsigned short vec_ushort8;
typedef vector unsigned int   vec_uint4;
typedef vector bool char      vec_ullong2;

# 13 "C:/PS3SDK\\target\\ppu\\include/vec_types.h"






# 14 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"

namespace std {

 
extern "C" {



 

extern inline vec_float4 acosf4fast(vec_float4 x);
extern inline vec_float4 asinf4fast(vec_float4 x);
extern inline vec_float4 atan2f4fast(vec_float4 y, vec_float4 x);
extern inline vec_float4 atanf4fast(vec_float4 x);
extern inline vec_float4 cbrtf4fast(vec_float4 x);
extern inline vec_float4 cosf4fast(vec_float4 x);
extern inline vec_float4 divf4(vec_float4 x, vec_float4 y);
extern inline vec_float4 divf4fast(vec_float4 x, vec_float4 y);
extern inline vec_float4 exp2f4fast(vec_float4 x);
extern inline vec_float4 expf4fast(vec_float4 x);
extern inline vec_float4 expm1f4fast(vec_float4 x);
extern inline vec_float4 log10f4fast(vec_float4 x);
extern inline vec_float4 log1pf4fast(vec_float4 x);
extern inline vec_float4 log2f4fast(vec_float4 x);
extern inline vec_float4 logf4fast(vec_float4 x);
extern inline vec_float4 negatef4(vec_float4 x);
extern inline vec_float4 powf4fast(vec_float4 x, vec_float4 y);
extern inline vec_float4 recipf4(vec_float4 x);
extern inline vec_float4 recipf4fast(vec_float4 x);
extern inline vec_float4 rsqrtf4(vec_float4 x);
extern inline vec_float4 rsqrtf4fast(vec_float4 x);
extern inline void       sincosf4fast(vec_float4 x, vec_float4* s, vec_float4* c);
extern inline vec_float4 sinf4fast(vec_float4 x);
extern inline vec_float4 sqrtf4fast(vec_float4 x);
extern inline vec_float4 tanf4fast(vec_float4 x);


 

extern inline vec_float4 acosf4(vec_float4);
extern inline vec_float4 asinf4(vec_float4);
extern inline vec_float4 atanf4(vec_float4);
extern inline vec_float4 atan2f4(vec_float4, vec_float4);
extern inline vec_float4 ceilf4(vec_float4);
extern inline vec_float4 copysignf4(vec_float4, vec_float4);
extern inline vec_float4 cosf4(vec_float4 x);
extern inline vec_float4 expf4(vec_float4);
extern inline vec_float4 fabsf4(vec_float4);
extern inline vec_float4 floorf4(vec_float4);
extern inline vec_float4 fmaxf4(vec_float4, vec_float4);
extern inline vec_float4 fminf4(vec_float4, vec_float4);
extern inline vec_float4 fmodf4(vec_float4, vec_float4);
extern inline vec_float4 frexpf4(vec_float4, vec_int4 *);
extern inline vec_float4 ldexpf4(vec_float4, vec_int4);
extern inline vec_float4 logf4(vec_float4);
extern inline vec_float4 log10f4(vec_float4);
extern inline vec_float4 modff4(vec_float4, vec_float4*);
extern inline vec_float4 remainderf4(vec_float4, vec_float4);
extern inline void       sincosf4(vec_float4 x, vec_float4* s, vec_float4* c);
extern inline vec_float4 powf4(vec_float4, vec_float4);
extern inline vec_float4 sinf4(vec_float4 x);
extern inline vec_float4 sqrtf4(vec_float4 x);
extern inline vec_float4 tanf4(vec_float4 x);
extern inline vec_float4 truncf4(vec_float4);

extern inline vec_float4 cbrtf4(vec_float4);
extern inline vec_float4 expm1f4(vec_float4);
extern inline vec_float4 exp2f4(vec_float4);
extern inline vec_float4 fdimf4(vec_float4, vec_float4);
extern inline vec_float4 fmaf4(vec_float4, vec_float4, vec_float4);
extern inline vec_float4 hypotf4(vec_float4, vec_float4);
extern inline vec_int4 ilogbf4(vec_float4);


extern inline vec_float4 log1pf4(vec_float4);
extern inline vec_float4 log2f4(vec_float4);
extern inline vec_float4 logbf4(vec_float4);

}


 
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/spec.h"




 








# 98 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"

 
 
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/truncf4.h"




 









extern inline vec_float4 truncf4 ( vec_float4 x )
{
   return vec_trunc( x );
}

# 102 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/ceilf4.h"




 








extern inline vec_float4 ceilf4 ( vec_float4 x )
{
   return vec_ceil( x );
}

# 103 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/copysignf4.h"




 








extern inline vec_float4 copysignf4 ( vec_float4 x, vec_float4 y )
{
   vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
   smask = vec_vslw(smask, smask);
   return vec_sel(x, y, smask);
}

# 104 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/divf4.h"




 








extern inline vec_float4 divf4 ( vec_float4 numer, vec_float4 denom )
{  
   
   vec_uint4 mask = (vec_uint4)(vec_int4)(-1);
   vec_float4 vnzero = (vec_float4)vec_vslw(mask, mask);
   vec_float4 y0, y0numer;

   y0 = vec_re(denom);
   y0numer = vec_madd(numer, y0, vnzero);
   y0 = vec_madd(vec_nmsub(denom, y0, (vec_float4)(1.0f) ), y0numer, y0numer);

   return vec_sel(y0numer, y0, vec_cmpeq(y0, y0));
}

# 105 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/divf4fast.h"




 









extern inline vec_float4 divf4fast ( vec_float4 numer, vec_float4 denom )
{
   vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
   return vec_madd( numer, vec_re( denom ), vzero);
}

# 106 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/fabsf4.h"




 








extern inline vec_float4 fabsf4 ( vec_float4 x )
{
   return vec_abs( x );
}

# 107 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/floorf4.h"




 








extern inline vec_float4 floorf4 ( vec_float4 x )
{
   return vec_floor( x );
}

# 108 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/fmaxf4.h"




 

                    


      



extern inline vec_float4 fmaxf4 ( vec_float4 x, vec_float4 y )
{
   return vec_max( x, y );
}

# 109 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/fminf4.h"




 

         






extern inline vec_float4 fminf4 ( vec_float4 x, vec_float4 y )
{
   return vec_min( x, y );
}

# 110 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/fmodf4.h"




 












extern inline vec_float4 fmodf4 ( vec_float4 x, vec_float4 y )
{
   vec_float4 q, xabs, yabs, qabs;
   vec_int4   qi0, qi1, qi2;
   vec_float4 i0, i1, i2, r1, r2, i;
   vec_uint4  inrange;
   vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
   vec_uint4 iszero = (vec_uint4)vec_cmpeq(x, vzero);
 
   
 
   
   
                                                                                
   q = divf4( x, y );
   xabs = fabsf4( x );
   yabs = fabsf4( y );
   qabs = fabsf4( q );
                                                                                
   inrange = (vec_uint4)vec_cmpgt( (vec_float4)((vec_uint4)(0x49800000)), qabs );
                                                                                
   qi1 = vec_cts( qabs, 0 );
   qi0 = vec_add( qi1, (vec_int4)(-1) );
   qi2 = vec_add( qi1, (vec_int4)(1) );
                                                                                                                                                          
   i0 = vec_ctf( qi0, 0 );
   i1 = vec_ctf( qi1, 0 );
   i2 = vec_ctf( qi2, 0 );
                                                                                                                                                          
   
                                                                                                                                                          
   r1 = vec_nmsub( i1, yabs, xabs );
   r2 = vec_nmsub( i2, yabs, xabs );
                                                                                                                                                          
   i = i0;
   i = vec_sel( i1, i, (vec_uint4)vec_cmpgt( vzero, r1 ) );
   i = vec_sel( i2, i, (vec_uint4)vec_cmpgt( vzero, r2 ) );
                                                                                                                                                          
   i = copysignf4( i, q );
                                                                                                                                                          
   return vec_sel(vec_sel((vec_float4)vzero, vec_nmsub( i, y, x ), inrange),
		  x, iszero);
}

# 111 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/modff4.h"




 









extern inline vec_float4 modff4 ( vec_float4 x, vec_float4 *iptr )
{
   vec_float4 integral, fraction;
 
   integral = truncf4( x );
   fraction = vec_sub( x, integral );
 
   *iptr = integral;
   return fraction;
}

# 112 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/negatef4.h"




 








extern inline vec_float4 negatef4 ( vec_float4 x )
{
   vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
   smask = vec_vslw(smask, smask);
   return (vec_float4)vec_xor( (vec_uint4)x, smask );
}

# 113 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/recipf4.h"




 

                     


                                             



extern inline vec_float4 recipf4 ( vec_float4 x )
{ 
   
 
   vec_float4 y0 = vec_re( x );
   vec_float4 res = vec_madd( vec_nmsub( x, y0, (vec_float4)(1.0f) ), y0, y0 );
   return vec_sel(y0, res, vec_cmpeq(res, res));
}

# 114 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/recipf4fast.h"




 










extern inline vec_float4 recipf4fast ( vec_float4 x )
{
   return vec_re( x );
}

# 115 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/remainderf4.h"




 













extern inline vec_float4 remainderf4 ( vec_float4 x, vec_float4 y )
{
   vec_float4 q, xabs, yabs, qabs, yabshalf;
   vec_int4   qi0, qi1, qi2;
   vec_float4 i0, i1, i2, i, rem;
   vec_uint4  inrange, odd0, odd1, odd2, cmp1, cmp2, odd;
   vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
   vec_uint4 iszero = (vec_uint4)vec_cmpeq(x, vzero);
 
   
 
   
   
                                                                                
   q = divf4( x, y );
   xabs = fabsf4( x );
   yabs = fabsf4( y );
   qabs = fabsf4( q );
                                                                                
   inrange = (vec_uint4)vec_cmpgt( (vec_float4)((vec_uint4)(0x49800000)), qabs );
                                                                                
   qi1 = vec_cts( qabs, 0 );
   qi0 = vec_add( qi1, (vec_int4)(-1) );
   qi2 = vec_add( qi1, (vec_int4)(1) );
                                                                                                                                                          
   odd1 = (vec_uint4)vec_cmpeq( vec_and( qi1, (vec_int4)(1) ), (vec_int4)(1) );
   odd0 = odd2 = vec_nor( odd1, odd1 );

   i0 = vec_ctf( qi0, 0 );
   i1 = vec_ctf( qi1, 0 );
   i2 = vec_ctf( qi2, 0 );
                                                                                                                                                          
   
                                                                                                                                                          
   cmp1 = (vec_uint4)vec_cmpgt( (vec_int4)vzero, (vec_int4)vec_nmsub( i1, yabs, xabs ) );
   cmp2 = (vec_uint4)vec_cmpgt( (vec_int4)vzero, (vec_int4)vec_nmsub( i2, yabs, xabs ) );
                                                                                                                    
   i = i0;
   i = vec_sel( i1, i, cmp1 );
   i = vec_sel( i2, i, cmp2 );
                                                                                                                    
   odd = odd0;
   odd = vec_sel( odd1, odd, cmp1 );
   odd = vec_sel( odd2, odd, cmp2 );
                                                                                                                    
   rem = vec_nmsub( i, yabs, xabs );
                                                                                                                    
   
   
   
   
   
   
   

   yabshalf = vec_madd( yabs, (vec_float4)(0.5f), (vec_float4)vzero );
   cmp1 = (vec_uint4)vec_cmpgt( rem, yabshalf );
   cmp2 = vec_and( (vec_uint4)vec_cmpeq( rem, yabshalf ), odd );
                                                                                                                    
   i = vec_sel( i, vec_add( i, (vec_float4)(1.0f) ), vec_or( cmp1, cmp2 ) );
   i = copysignf4( i, q );
                                                                                                                                                          
   return vec_sel(vec_sel((vec_float4)vzero, vec_nmsub( i, y, x ), inrange ),
		 x, iszero);
}

# 116 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/rsqrtf4.h"




 






      



extern inline vec_float4 rsqrtf4 ( vec_float4 x )
{ 
   
 
   vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
   vec_float4 half = (vec_float4)(0.5f);
   vec_float4 one = (vec_float4)(1.0f);
   vec_float4 y0, y0x, y0half;

   y0 = vec_rsqrte( x );
   y0x = vec_madd( y0, x, vzero );
   y0half = vec_madd( y0, half, vzero );
   y0x = vec_madd( vec_nmsub( y0, y0x, one ), y0half, y0 );
   return vec_sel(y0, y0x, vec_cmpeq(y0x, y0x));
}

# 117 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/rsqrtf4fast.h"




 






      



extern inline vec_float4 rsqrtf4fast ( vec_float4 x )
{
   return vec_rsqrte( x );
}

# 118 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/sqrtf4.h"




 






      



extern inline vec_float4 sqrtf4 ( vec_float4 x )
{
   
 
   vec_float4 zero = (vec_float4)(0.0f);
   vec_float4 half = (vec_float4)(0.5f);
   vec_float4 one = (vec_float4)(1.0f);
   vec_float4 y0, y0x, y0xhalf;
   vec_uint4 cmp_zero;

   y0 = vec_rsqrte( x );
   cmp_zero = (vec_uint4)vec_cmpeq( x, zero );
   cmp_zero = vec_or(cmp_zero, (vec_uint4)vec_cmpeq(y0, zero));
   y0x = vec_madd( y0, x, zero );
   y0xhalf = vec_madd( y0x, half, zero );
   return vec_sel(vec_madd(vec_nmsub(y0, y0x, one), y0xhalf, y0x), x, cmp_zero);
}

# 119 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/sqrtf4fast.h"




 


      






extern inline vec_float4 sqrtf4fast ( vec_float4 x )
{
   vec_float4 zero = (vec_float4)(0.0f);
   return vec_sel( vec_madd( x, vec_rsqrte( x ), zero ), zero, vec_cmpeq( x, zero ) );
}

# 120 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"

 
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/sinf4.h"




 




# 1 "C:/PS3SDK\\target\\ppu\\include/bits/sincos_c.h"




 






# 18 "C:/PS3SDK\\target\\ppu\\include/bits/sincos_c.h"

# 25 "C:/PS3SDK\\target\\ppu\\include/bits/sincos_c.h"








# 39 "C:/PS3SDK\\target\\ppu\\include/bits/sincos_c.h"
































 




# 11 "C:/PS3SDK\\target\\ppu\\include/bits/sinf4.h"





extern inline vec_float4 sinf4 (vec_float4 x)
{
    vec_float4 xl,xl2,xl3,res;
    vec_int4   q;
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
    smask = vec_vslw(smask, smask);

    
    
    xl = vec_madd(x, (vec_float4)(0.63661977236f),(vec_float4)vzero);

    
    
    
    xl = vec_add(xl,vec_sel((vec_float4)(0.5f),xl,smask));
    q = vec_cts(xl,0);

     
    
    
    vec_int4 offset = vec_and(q,(vec_int4)((int)0x3));

    
    
    vec_float4 qf = vec_ctf(q,0);
    vec_float4 p1 = vec_nmsub(qf,(vec_float4)(1.57079625129f),x);
    xl  = vec_nmsub(qf,(vec_float4)(7.54978995489e-8f),p1);
    
    
    
    xl2 = vec_madd(xl,xl,(vec_float4)vzero);
    xl3 = vec_madd(xl2,xl,(vec_float4)vzero);
    

    
    
    
    
    
    vec_float4 ct1 = vec_madd((vec_float4)(-0.0013602249f),xl2,(vec_float4)(0.0416566950f));
    vec_float4 st1 = vec_madd((vec_float4)(-0.0001950727f),xl2,(vec_float4)(0.0083320758f));

    vec_float4 ct2 = vec_madd(ct1,xl2,(vec_float4)(-0.4999990225f));
    vec_float4 st2 = vec_madd(st1,xl2,(vec_float4)(-0.1666665247f));
    
    vec_float4 cx = vec_madd(ct2,xl2,(vec_float4)(1.0f));
    vec_float4 sx = vec_madd(st2,xl3,xl);

    
    
    
    vec_uint4 mask1 = (vec_uint4)vec_cmpeq(vec_and(offset,
                                                   (vec_int4)(0x1)),
                                           (vec_int4)vzero);
    res = vec_sel(cx,sx,mask1);

    
    
    vec_uint4 mask2 = (vec_uint4)vec_cmpeq(vec_and(offset,(vec_int4)(0x2)),(vec_int4)vzero);
    res = vec_sel((vec_float4)vec_xor(smask,(vec_uint4)res),res,mask2);

    return res;

}

# 123 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/cosf4.h"




 









extern inline vec_float4 cosf4 (vec_float4 x)
{
    vec_float4 xl,xl2,xl3,res;
    vec_int4   q;
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
    smask = vec_vslw(smask, smask);

    
    
    xl = vec_madd(x, (vec_float4)(0.63661977236f), vzero);

    
    
    
    xl = vec_add(xl,vec_sel((vec_float4)(0.5f),xl, smask));
    q = vec_cts(xl,0);

     
    
    
    vec_int4 offset = vec_add((vec_int4)(1),vec_and(q,(vec_int4)((int)0x3)));

    
    
    vec_float4 qf = vec_ctf(q,0);
    vec_float4 p1 = vec_nmsub(qf,(vec_float4)(1.57079625129f),x);
    xl  = vec_nmsub(qf,(vec_float4)(7.54978995489e-8f),p1);
    
    
    
    xl2 = vec_madd(xl,xl,vzero);
    xl3 = vec_madd(xl2,xl,vzero);
    

    
    
    
    
    
    vec_float4 ct1 = vec_madd((vec_float4)(-0.0013602249f),xl2,(vec_float4)(0.0416566950f));
    vec_float4 st1 = vec_madd((vec_float4)(-0.0001950727f),xl2,(vec_float4)(0.0083320758f));

    vec_float4 ct2 = vec_madd(ct1,xl2,(vec_float4)(-0.4999990225f));
    vec_float4 st2 = vec_madd(st1,xl2,(vec_float4)(-0.1666665247f));
    
    vec_float4 cx = vec_madd(ct2,xl2,(vec_float4)(1.0f));
    vec_float4 sx = vec_madd(st2,xl3,xl);

    
    
    
    vec_uint4 mask1 = (vec_uint4)vec_cmpeq(vec_and(offset,
                                                   (vec_int4)(0x1)),
                                           (vec_int4)vzero);
    res = vec_sel(cx,sx,mask1);

    
    
    vec_uint4 mask2 = (vec_uint4)vec_cmpeq(vec_and(offset,(vec_int4)(0x2)),(vec_int4)(vzero));
    res = vec_sel((vec_float4)vec_xor(smask,(vec_uint4)res),res,mask2);

    return res;

}

# 124 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/sincosf4.h"




 










extern inline void sincosf4 (vec_float4 x, vec_float4* s, vec_float4* c)
{
    vec_float4 xl,xl2,xl3;
    vec_int4   q;
    vec_int4   offsetSin, offsetCos;
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
    smask = vec_vslw(smask, smask);

    
    
    xl = vec_madd(x, (vec_float4)(0.63661977236f),(vec_float4)vzero);

    
    
    
    xl = vec_add(xl,vec_sel((vec_float4)(0.5f),xl,smask));
    q = vec_cts(xl,0);

     
    
    
    
    offsetSin = vec_and(q,(vec_int4)((int)0x3));
    offsetCos = vec_add((vec_int4)(1),offsetSin);

    
    
    vec_float4 qf = vec_ctf(q,0);
    vec_float4 p1 = vec_nmsub(qf,(vec_float4)(1.57079625129f),x);
    xl  = vec_nmsub(qf,(vec_float4)(7.54978995489e-8f),p1);
    
    
    
    xl2 = vec_madd(xl,xl,(vec_float4)vzero);
    xl3 = vec_madd(xl2,xl,(vec_float4)vzero);
    

    
    
    
    
    
    vec_float4 ct1 = vec_madd((vec_float4)(-0.0013602249f),xl2,(vec_float4)(0.0416566950f));
    vec_float4 st1 = vec_madd((vec_float4)(-0.0001950727f),xl2,(vec_float4)(0.0083320758f));

    vec_float4 ct2 = vec_madd(ct1,xl2,(vec_float4)(-0.4999990225f));
    vec_float4 st2 = vec_madd(st1,xl2,(vec_float4)(-0.1666665247f));
    
    vec_float4 cx = vec_madd(ct2,xl2,(vec_float4)(1.0f));
    vec_float4 sx = vec_madd(st2,xl3,xl);

    
    
    
    vec_uint4 sinMask = (vec_uint4)vec_cmpeq(vec_and(offsetSin,(vec_int4)(0x1)),(vec_int4)vzero);
    vec_uint4 cosMask = (vec_uint4)vec_cmpeq(vec_and(offsetCos,(vec_int4)(0x1)),(vec_int4)vzero);    
    *s = vec_sel(cx,sx,sinMask);
    *c = vec_sel(cx,sx,cosMask);

    
    
    sinMask = (vec_uint4)vec_cmpeq(vec_and(offsetSin,(vec_int4)(0x2)),(vec_int4)vzero);
    cosMask = (vec_uint4)vec_cmpeq(vec_and(offsetCos,(vec_int4)(0x2)),(vec_int4)vzero);
    
    *s = vec_sel((vec_float4)vec_xor(smask,(vec_uint4)*s),*s,sinMask);
    *c = vec_sel((vec_float4)vec_xor(smask,(vec_uint4)*c),*c,cosMask);
    
}

# 125 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/tanf4.h"




 










extern inline vec_float4 tanf4 (vec_float4 x)
{
    vec_float4 xl,xl2,xl3,res;
    vec_int4   q;
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
    smask = vec_vslw(smask, smask);

    
    
    xl = vec_madd(x, (vec_float4)(0.63661977236f),(vec_float4)vzero);

    
    
    
    xl = vec_add(xl,vec_sel((vec_float4)(0.5f),xl,smask));
    q = vec_cts(xl,0);

     
    
    
    vec_float4 qf = vec_ctf(q,0);
    vec_float4 p1 = vec_nmsub(qf,(vec_float4)(1.57079625129f),x);
    xl  = vec_nmsub(qf,(vec_float4)(7.54978995489e-8f),p1);
    
    
    
    xl2 = vec_madd(xl,xl,(vec_float4)vzero);
    xl3 = vec_madd(xl2,xl,(vec_float4)vzero);
 
    
    
    
    
    
    vec_float4 ct2 = vec_madd((vec_float4)( 0.0097099364f),xl2,(vec_float4)(-0.4291161787f));
    
    vec_float4 cx = vec_madd(ct2,xl2,(vec_float4)(1.0f));
    vec_float4 sx = vec_madd((vec_float4)(-0.0957822992f),xl3,xl);

    
    
    
    vec_float4 cxosx = divf4(cx,sx);
    vec_float4 sxocx = divf4(sx,cx);

    vec_float4 ncxosx = (vec_float4)vec_xor(smask,(vec_uint4)cxosx);

    
    
    
    vec_uint4 mask = (vec_uint4)vec_cmpeq(vec_and(q,(vec_int4)(0x1)),(vec_int4)vzero);
    res = vec_sel(ncxosx,sxocx,mask);

    return res;
}

# 126 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/asinf4.h"




 






extern inline vec_float4 asinf4 (vec_float4 x)
{
    vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
    smask = vec_vslw(smask, smask);
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    
    
    vec_uint4 positive = (vec_uint4)vec_cmpgt(x, vzero);

    
    
    x = vec_abs(x);

    
    
    vec_uint4 gtHalf = (vec_uint4)vec_cmpgt(x,(vec_float4)(0.5f));    


    
    
    
    
    
    
    vec_float4 g = vec_sel(vec_madd(x,x, vzero),vec_madd((vec_float4)(-0.5f),x,(vec_float4)(0.5f)),gtHalf);
    
    x = vec_sel(x,vec_madd((vec_float4)(-2.0f),sqrtf4(g), vzero),gtHalf);

    
    
    
    
    vec_float4 denom = vec_add(g,(vec_float4)(-5.54846723f));
    vec_float4 num = vec_madd((vec_float4)(-0.504400557f),g,(vec_float4)(0.933933258f));
    denom = vec_madd(denom,g,(vec_float4)(5.603603363f));
    num = vec_madd(vec_madd(x,g, vzero),num, vzero);

    
    
    
    x = vec_add(x,divf4(num,denom));

    
    
    
    x = vec_sel(x,vec_add(x,(vec_float4)(1.57079632679489661923f)),gtHalf);

    
    
    
    x = vec_sel((vec_float4)vec_xor((vec_int4)smask, (vec_int4)x),x,positive);

    return x;
}

# 127 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/acosf4.h"




 
 
 
 
 
 
 
 
 
 
 
 






extern inline vec_float4 acosf4 (vec_float4 x)
{
    vec_float4 result, xabs;
    vec_float4 t1;
    vec_float4 xabs2, xabs4;
    vec_float4 hi, lo;
    vec_float4 neg, pos;
    vec_uint4 select;
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    
    xabs = vec_abs(x);
    select = (vec_uint4)(vec_sra((vec_int4)(x), (vec_uint4)(31)));
    
    t1 = sqrtf4(vec_sub((vec_float4)(1.0), xabs));
    
    



 
    xabs2 = vec_madd(xabs,  xabs, vzero);
    xabs4 = vec_madd(xabs2, xabs2, vzero);
    hi = vec_madd((vec_float4)(-0.0012624911), xabs, (vec_float4)(0.0066700901));
    hi = vec_madd(hi, xabs, (vec_float4)(-0.0170881256));
    hi = vec_madd(hi, xabs, (vec_float4)( 0.0308918810));
    lo = vec_madd((vec_float4)(-0.0501743046), xabs, (vec_float4)(0.0889789874));
    lo = vec_madd(lo, xabs, (vec_float4)(-0.2145988016));
    lo = vec_madd(lo, xabs, (vec_float4)( 1.5707963050));
    
    result = vec_madd(hi, xabs4, lo);
    
    
 
    neg = vec_nmsub(t1, result, (vec_float4)(3.1415926535898f));
    pos = vec_madd(t1, result, vzero);
    
    result = vec_sel(pos, neg, select);
    
    return result;
}

# 128 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/atanf4.h"




 
 
 
 
 
 
 
 
 
 
 
 
 
 






extern inline vec_float4 atanf4 (vec_float4 x)
{
    vec_float4 bias;
    vec_float4 x2, x3, x4, x8, x9;
    vec_float4 hi, lo;
    vec_float4 result;
    vec_float4 inv_x;
    vec_uint4 sign;
    vec_uint4 select;
    vec_float4 xabs;
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
    smask = vec_vslw(smask, smask);
    vec_float4 vnzero = vec_or(vzero, (vec_float4)smask);
    
    sign = vec_and((vec_uint4)x, smask);
    xabs = (vec_float4)vec_andc((vec_uint4)x, smask);
    inv_x = recipf4(x);
    inv_x = (vec_float4)vec_xor((vec_uint4)inv_x, smask);
    select = (vec_uint4)vec_cmpgt(xabs, (vec_float4)(1.0f));
    bias = (vec_float4)(1.57079632679489661923f);
    bias = (vec_float4)vec_and((vec_uint4)bias, select);
    bias = (vec_float4)vec_or((vec_uint4)sign, (vec_uint4)bias);
    
    x = vec_sel(x, inv_x, select);
    
    



 
    bias = vec_add(bias, x);
    x2 = vec_madd(x,  x,  vzero);
    x3 = vec_madd(x2, x,  vzero);
    x4 = vec_madd(x2, x2, vzero);
    x8 = vec_madd(x4, x4, vzero);
    x9 = vec_madd(x8, x,  vnzero);
    hi = vec_madd((vec_float4)(0.0028662257f), x2, (vec_float4)(-0.0161657367f));
    hi = vec_madd(hi, x2, (vec_float4)(0.0429096138f));
    hi = vec_madd(hi, x2, (vec_float4)(-0.0752896400f));
    hi = vec_madd(hi, x2, (vec_float4)(0.1065626393f));
    lo = vec_madd((vec_float4)(-0.1420889944f), x2, (vec_float4)(0.1999355085f));
    lo = vec_madd(lo, x2, (vec_float4)(-0.3333314528f));
    lo = vec_madd(lo, x3, bias);
    
    result = vec_madd(hi, x9, lo);    
    return result;
}

# 129 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/atan2f4.h"




 







extern inline vec_float4 atan2f4 (vec_float4 y, vec_float4 x)
{
    vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
    smask = vec_vslw(smask, smask);
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    vec_float4 xx = vec_sel(x, (vec_float4)(1.0f), vec_cmpeq(x, y));
    vec_float4 yy = vec_sel(y, (vec_float4)(1.0f), vec_cmpeq(x, y));
    vec_float4 res = atanf4(divf4(yy,xx));

    
    
    
    
    
    
    
    vec_uint4 signy = (vec_uint4)vec_and((vec_uint4)y, smask);
    vec_uint4 isnan = vec_sel((vec_uint4)x, signy, vec_cmpeq(x, x));
    isnan = vec_sel((vec_uint4)y, isnan, vec_cmpeq(y, y));
    vec_uint4 yNeg = (vec_uint4)vec_cmpgt(vzero,y);
    vec_uint4 xNeg = (vec_uint4)vec_cmpgt(vzero,x);
    vec_uint4 szero = (vec_uint4)vec_cmpeq((vec_uint4)x, (vec_uint4)smask);

    vec_float4 bias = vec_sel((vec_float4)(3.14159265358979323846f),(vec_float4)(-3.14159265358979323846f),yNeg);

    vec_float4 newRes = vec_add(bias, res);

    res = vec_sel(vec_sel(res,newRes,xNeg),newRes,szero);

    return (vec_float4)vec_or((vec_uint4)res, isnan);
}

# 130 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/sinf4fast.h"




 











extern inline vec_float4 sinf4fast (vec_float4 x)
{
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
    smask = vec_vslw(smask, smask);
    vec_float4 g = vec_madd(x,x,(vec_float4)vzero);
    vec_float4 f = (vec_float4)vec_andc((vec_uint4)x,smask);

    vec_float4 t1 = vec_madd(g,g,(vec_float4)vzero);
    vec_float4 t2 = vec_madd((vec_float4)(-0.1980741872e-3f),g,(vec_float4)(0.8333025139e-2f));
    vec_float4 t3 = vec_madd((vec_float4)(-0.1666665668f),g,(vec_float4)vzero);
    vec_float4 t4 = vec_madd(t1,t1,(vec_float4)vzero);
    vec_float4 t5 = vec_madd(t1,t2,t3);
    vec_float4 r  = vec_madd((vec_float4)(0.2601903036e-5f),t4,t5);

    vec_float4 res = vec_madd(f,r,f);

    res = vec_sel(res, x, smask);

    return res;
}

# 131 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/cosf4fast.h"




 










extern inline vec_float4 cosf4fast (vec_float4 x)
{
    
    
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
    smask = vec_vslw(smask, smask);
    x = vec_sub((vec_float4)(1.57079625129f),x);
    x = vec_add((vec_float4)(7.54978995489e-8f),x);

    vec_float4 g = vec_madd(x,x,vzero);
    vec_float4 f = (vec_float4)vec_andc((vec_uint4)x,smask);

    vec_float4 t1 = vec_madd(g,g,vzero);
    vec_float4 t2 = vec_madd((vec_float4)(-0.1980741872e-3f),g,(vec_float4)(0.8333025139e-2f));
    vec_float4 t3 = vec_madd((vec_float4)(-0.1666665668f),g,vzero);
    vec_float4 t4 = vec_madd(t1,t1,vzero);
    vec_float4 t5 = vec_madd(t1,t2,t3);
    vec_float4 r  = vec_madd((vec_float4)(0.2601903036e-5f),t4,t5);

    vec_float4 res = vec_madd(f,r,f);

    res = vec_sel(res, x, smask);

    return res;
}

# 132 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/sincosf4fast.h"




 











extern inline void sincosf4fast (vec_float4 x, vec_float4* s, vec_float4* c)
{
    vec_float4 xl2,xl3;
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
 
    
    
    xl2 = vec_madd(x,x,(vec_float4)vzero);
    xl3 = vec_madd(xl2,x,(vec_float4)vzero);
    

    
    
    
    
    
    vec_float4 ct1 = vec_madd((vec_float4)(-0.0013602249f),xl2,(vec_float4)(0.0416566950f));
    vec_float4 st1 = vec_madd((vec_float4)(-0.0001950727f),xl2,(vec_float4)(0.0083320758f));

    vec_float4 ct2 = vec_madd(ct1,xl2,(vec_float4)(-0.4999990225f));
    vec_float4 st2 = vec_madd(st1,xl2,(vec_float4)(-0.1666665247f));
    
    *c = vec_madd(ct2,xl2,(vec_float4)(1.0f));
    *s = vec_madd(st2,xl3,x);

}

# 133 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/tanf4fast.h"




 








extern inline vec_float4 tanf4fast (vec_float4 x)
{
    vec_float4 x2,x3;
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));

    
    
    x2 = vec_madd(x,x,(vec_float4)vzero);
    x3 = vec_madd(x2,x,(vec_float4)vzero);
    

    
    
    
    
    
    vec_float4 ct2 = vec_madd((vec_float4)( 0.0097099364f),x2,(vec_float4)(-0.4291161787f));
    
    vec_float4 cx = vec_madd(ct2,x2,(vec_float4)(1.0f));
    vec_float4 sx = vec_madd((vec_float4)(-0.0957822992f),x3,x);

    vec_float4 res = divf4(sx,cx);
    return res;
}

# 134 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/acosf4fast.h"




 
 
 
 
 
 
 
 
 
 
 
 








extern inline vec_float4 acosf4fast (vec_float4 x)
{
    vec_float4 result, xabs;
    vec_float4 t1;
    vec_float4 xabs2, xabs4;
    vec_float4 hi, lo;
    vec_float4 neg, pos;
    vec_uint4 select;
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    
    xabs = vec_abs(x);
    select = (vec_uint4)(vec_sra((vec_int4)(x), (vec_uint4)(31)));
    
    t1 = sqrtf4fast(vec_sub((vec_float4)(1.0), xabs));
    
    



 
    xabs2 = vec_madd(xabs,  xabs, vzero);
    xabs4 = vec_madd(xabs2, xabs2, vzero);
    hi = vec_madd((vec_float4)(-0.0012624911), xabs, (vec_float4)(0.0066700901));
    hi = vec_madd(hi, xabs, (vec_float4)(-0.0170881256));
    hi = vec_madd(hi, xabs, (vec_float4)( 0.0308918810));
    lo = vec_madd((vec_float4)(-0.0501743046), xabs, (vec_float4)(0.0889789874));
    lo = vec_madd(lo, xabs, (vec_float4)(-0.2145988016));
    lo = vec_madd(lo, xabs, (vec_float4)( 1.5707963050));
    
    result = vec_madd(hi, xabs4, lo);
    
    
 
    neg = vec_nmsub(t1, result, (vec_float4)(3.1415926535898f));
    pos = vec_madd(t1, result, vzero);
    
    result = vec_sel(pos, neg, select);
    
    return result;
}

# 135 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/asinf4fast.h"




 
 
 
 
 
 
 
 
 
 
 
 
 









extern inline vec_float4 asinf4fast (vec_float4 x)
{
    vec_uint4 mask = (vec_uint4)(vec_int4)(-1);
    mask = vec_vslw(mask, mask);
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    vec_uint4 sign;
    vec_float4 result, xabs;
    vec_float4 t1;
    vec_float4 xabs2, xabs4;
    vec_float4 hi, lo;
    
    
 
    sign = vec_and((vec_uint4)(x), mask);
    xabs = vec_abs(x);
    
    t1 = sqrtf4(vec_sub((vec_float4)(1.0f), xabs));
    
    



 
    xabs2 = vec_madd(xabs,  xabs, vzero);
    xabs4 = vec_madd(xabs2, xabs2, vzero);
    hi = vec_madd((vec_float4)(-0.0012624911), xabs, (vec_float4)(0.0066700901));
    hi = vec_madd(hi, xabs, (vec_float4)(-0.0170881256));
    hi = vec_madd(hi, xabs, (vec_float4)( 0.0308918810));
    lo = vec_madd((vec_float4)(-0.0501743046), xabs, (vec_float4)(0.0889789874));
    lo = vec_madd(lo, xabs, (vec_float4)(-0.2145988016));
    lo = vec_madd(lo, xabs, (vec_float4)( 1.5707963050));
    
    result = vec_madd(hi, xabs4, lo);
    result = vec_nmsub(t1, result, (vec_float4)(1.5707963267949f));
    
    
 
    result = (vec_float4)(vec_or((vec_uint4)(result), sign));
    
    return result;
}

# 136 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/atanf4fast.h"




 
 
 
 
 
 
 
 
 
 
 
 
 
 








extern inline vec_float4 atanf4fast (vec_float4 x)
{
    vec_float4 bias;
    vec_float4 x2, x3, x4, x8, x9;
    vec_float4 hi, lo;
    vec_float4 result;
    vec_float4 inv_x;
    vec_uint4 sign;
    vec_uint4 select;
    vec_float4 xabs;
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
    smask = vec_vslw(smask, smask);
    
    sign = vec_and((vec_uint4)x, smask);
    xabs = (vec_float4)vec_andc((vec_uint4)x, smask);
    inv_x = recipf4fast(x);
    inv_x = (vec_float4)vec_xor((vec_uint4)inv_x, smask);
    select = (vec_uint4)vec_cmpgt(xabs, (vec_float4)(1.0));
    bias = (vec_float4)vec_or(sign, (vec_uint4)((vec_float4)(1.57079632679489661923f)));
    bias = (vec_float4)vec_and((vec_uint4)bias, select);
    
    x = vec_sel(x, inv_x, select);
    
    



 
    bias = vec_add(bias, x);
    x2 = vec_madd(x,  x,  vzero);
    x3 = vec_madd(x2, x,  vzero);
    x4 = vec_madd(x2, x2, vzero);
    x8 = vec_madd(x4, x4, vzero);
    x9 = vec_madd(x8, x,  vzero);
    hi = vec_madd((vec_float4)(0.0028662257), x2, (vec_float4)(-0.0161657367));
    hi = vec_madd(hi, x2, (vec_float4)(0.0429096138));
    hi = vec_madd(hi, x2, (vec_float4)(-0.0752896400));
    hi = vec_madd(hi, x2, (vec_float4)(0.1065626393));
    lo = vec_madd((vec_float4)(-0.1420889944), x2, (vec_float4)(0.1999355085));
    lo = vec_madd(lo, x2, (vec_float4)(-0.3333314528));
    lo = vec_madd(lo, x3, bias);
    
    result = vec_madd(hi, x9, lo);
    
    return result;
}

# 137 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/atan2f4fast.h"




 








extern inline vec_float4 atan2f4fast (vec_float4 y, vec_float4 x)
{
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    vec_float4 res = atanf4fast(divf4fast(y,x));

    
    
    
    
    
    
    
    vec_uint4 yNeg = (vec_uint4)vec_cmpgt(vzero,y);
    vec_uint4 xNeg = (vec_uint4)vec_cmpgt(vzero,x);

    vec_float4 bias = vec_sel((vec_float4)(3.14159265358979323846f),(vec_float4)(-3.14159265358979323846f),yNeg);

    vec_float4 newRes = vec_add(bias, res);

    res = vec_sel(res,newRes,xNeg);

    return res;
}

# 138 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"

 
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/fmaf4.h"




 






extern inline vec_float4 fmaf4 (vec_float4 x, vec_float4 y, vec_float4 z)
{
  return vec_madd(x,y,z);
}

# 141 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/fdimf4.h"




 






extern inline vec_float4 fdimf4 (vec_float4 x, vec_float4 y)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_float4 diff = vec_sub(x,y);
  return vec_sel(vzero, diff, vec_cmpgt(x,y));
}

# 142 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/hypotf4.h"




 






extern inline vec_float4 hypotf4 (vec_float4 x, vec_float4 y)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_float4 sum = vec_madd(x,x,vzero);
  sum = vec_madd(y,y,sum);
  return sqrtf4(sum);
}

# 143 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"

 
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/frexpf4.h"




 




extern inline vec_float4 frexpf4 (vec_float4 x, vec_int4 *exp)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_uint4 zeromask = (vec_uint4)vec_cmpeq(x, (vec_float4)vzero);
 
  vec_int4 expmask = (vec_int4)(0x7F800000);
  vec_int4 e1 = vec_and ( (vec_int4)x, expmask);
  vec_int4 e2 = vec_sub(vec_sr(e1, (vec_uint4)(23) ), (vec_int4)(126) );
  *exp = vec_sel(e2, (vec_int4)vzero, zeromask);
 
  vec_float4 m2 = vec_sel(x, (vec_float4)((vec_int4)(0x3F000000)), (vec_uint4)expmask);

  return vec_sel(m2, (vec_float4)vzero, zeromask);
}







 

# 146 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/ldexpf4.h"




 




extern inline vec_float4 ldexpf4 (vec_float4 x, vec_int4 exp) 
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
  smask = vec_vslw(smask, smask);

  vec_int4 expmask = (vec_int4)(0x7F800000);
  vec_int4 e1 = vec_and((vec_int4)x, expmask);
  vec_int4 e2 = vec_sr(e1,(vec_uint4)(23));

  vec_uint4 maxmask = (vec_uint4)vec_cmpgt(exp, (vec_int4)(255));
  vec_uint4 minmask = (vec_uint4)vec_cmpgt((vec_int4)(-255), exp);
  minmask = vec_or (minmask, (vec_uint4)vec_cmpeq(x, (vec_float4)vzero));

  vec_int4 esum = vec_add(e2, exp);

  maxmask = vec_or (maxmask, (vec_uint4)vec_cmpgt(esum, (vec_int4)(255)));
  maxmask = vec_andc(maxmask, smask);
  minmask = vec_or (minmask, (vec_uint4)vec_cmpgt((vec_int4)vzero, esum));

  x = vec_sel(x, (vec_float4)vec_sl(esum,(vec_uint4)(23)), (vec_uint4)expmask);
  x = vec_sel(x, (vec_float4)vzero, minmask);
  x = vec_sel(x, (vec_float4)maxmask, maxmask);
  return x;
}

# 147 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/log2f4.h"




 












extern inline vec_float4 log2f4 (vec_float4 x)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
  smask = vec_vslw(smask, smask);
  vec_float4 ones = (vec_float4)(1.0f);
  vec_uint4 iszero = (vec_uint4)vec_cmpeq(x, vzero);
  vec_uint4 isnan = (vec_uint4)vec_cmpeq(x, x);
  vec_int4 expmask = (vec_int4)(0x7F800000);
  vec_uint4 isinf = (vec_uint4)vec_cmpeq((vec_uint4)x, (vec_uint4)expmask);

  vec_int4 xexp = vec_add( vec_sr(vec_and((vec_int4)x, expmask), (vec_uint4)(23)), (vec_int4)(-126) );
  vec_float4 xx = vec_sel(x, (vec_float4)((vec_int4)(0x3F000000)), (vec_uint4)expmask);

  vec_uint4  mask = (vec_uint4)vec_cmpgt( (vec_float4)((float)0.7071067811865f), xx);
  xx    = vec_sel(xx   , vec_add(xx, xx)                , mask);
  xexp = vec_sel(xexp, vec_sub(xexp, (vec_int4)(1) ), mask);
  
  vec_float4 x1 = vec_sub(xx , ones);
  vec_float4 z  = divf4(x1, vec_add(xx, ones));
  vec_float4 w  = vec_madd(z , z, vzero);
  vec_float4 polywneg;
  polywneg = vec_madd((vec_float4)(((float)(-0.2988439998f))), w, (vec_float4)(((float)(-0.3997655209f))));
  polywneg = vec_madd(polywneg                 , w, (vec_float4)(((float)(-0.6666679125f))));
  
  vec_float4 y    = vec_madd(z, vec_madd(polywneg, w, x1), vzero);
  vec_float4 zz1  = vec_madd((vec_float4)(((float) 1.4426950216293f)), x1, vec_ctf(xexp,0));
  vec_float4 zz2  = vec_madd((vec_float4)(((float) 1.9259629911e-8f)), x1,
			     vec_madd((vec_float4)(((float)-1.4426950408890f)), y, vzero)
			     );
  
  vec_float4 res =  vec_sel(vec_add(zz1,zz2), (vec_float4)vec_or((vec_uint4)expmask, smask), iszero);
  res = vec_sel(res, x, isinf);
  return vec_sel(x, res, isnan);
}

# 148 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/log2f4fast.h"




 




extern inline vec_float4 log2f4fast (vec_float4 x)
{
  return vec_loge(x);
}

# 149 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/logf4.h"




 











extern inline vec_float4 logf4 (vec_float4 x) 
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
  smask = vec_vslw(smask, smask);
  vec_float4 ones = (vec_float4)(1.0f);
  vec_int4 expmask = (vec_int4)(0x7F800000);
  vec_uint4 iszero = (vec_uint4)vec_cmpeq(x, vzero);
  vec_uint4 isnan = (vec_uint4)vec_cmpeq(x, x);
  vec_uint4 isinf = (vec_uint4)vec_cmpeq((vec_uint4)x, (vec_uint4)expmask);

  vec_int4 xexp = vec_add( vec_sr(vec_and((vec_int4)x, expmask), (vec_uint4)(23)), (vec_int4)(-126) );
  vec_float4 xx = vec_sel(x, (vec_float4)((vec_int4)(0x3F000000)), (vec_uint4)expmask);

  vec_uint4  mask = (vec_uint4)vec_cmpgt((vec_float4)((float)0.7071067811865f), xx);
  xx = vec_sel(xx, vec_add(xx, xx), mask);
  xexp = vec_sel(xexp, vec_sub(xexp,(vec_int4)(1)), mask);

  vec_float4 x1 = vec_sub(xx, ones);
  vec_float4 z  = divf4  (x1, vec_add(xx, ones));
  vec_float4 w  = vec_madd(z, z, vzero);
  vec_float4 polywneg;
  polywneg = vec_madd((vec_float4)(((float)(-0.2988439998f))), w, (vec_float4)(((float)(-0.3997655209f))));
  polywneg = vec_madd(polywneg                , w, (vec_float4)(((float)(-0.6666679125f))));

  vec_float4 y    = vec_madd(z, vec_madd(polywneg, w, x1), vzero);
  vec_float4 wnew = vec_ctf(xexp,0);
  vec_float4 zz1     = vec_madd((vec_float4)(((float)(0.6931470632553f)))   , wnew, x1);
  vec_float4 zz2neg  = vec_madd((vec_float4)(((float)(-1.1730463525e-7f))), wnew, y );

  
  vec_float4 res = vec_sel(vec_sub(zz1,zz2neg), (vec_float4)vec_or((vec_uint4)expmask, smask), iszero);
  res = vec_sel(res, x, isinf);
  return vec_sel(x, res, isnan);
}

# 150 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/logf4fast.h"




 
 
 
 
 
 
 
 
 
 
 
 
 













 

extern inline vec_float4 logf4fast (vec_float4 x)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  return vec_madd(log2f4fast(x), (vec_float4)(0.69314718055995f), vzero);
}

# 151 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/log10f4.h"




 














extern inline vec_float4 log10f4 (vec_float4 x) 
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
  smask = vec_vslw(smask, smask);
  vec_float4 ones = (vec_float4)(1.0f);
  vec_int4 expmask = (vec_int4)(0x7F800000);
  vec_uint4 iszero = (vec_uint4)vec_cmpeq(x, vzero);
  vec_uint4 isnan = (vec_uint4)vec_cmpeq(x, x);
  vec_uint4 isinf = (vec_uint4)vec_cmpeq((vec_uint4)x, (vec_uint4)expmask);

  vec_int4 xexp = vec_add(vec_sr(vec_and((vec_int4)x, expmask), (vec_uint4)(23)), (vec_int4)(-126));
  vec_float4 xx = vec_sel(x, (vec_float4)((vec_int4)(0x3F000000)), (vec_uint4)expmask);

  vec_uint4 mask = (vec_uint4)vec_cmpgt((vec_float4)((float)0.7071067811865f), xx);
  xx = vec_sel(xx, vec_add(xx, xx), mask);
  xexp = vec_sel(xexp, vec_sub(xexp, (vec_int4)(1)), mask);
  
  vec_float4 x1 = vec_sub(xx, ones);
  vec_float4 z = divf4(x1, vec_add(xx, ones));
  vec_float4 w = vec_madd(z, z, vzero);
  vec_float4 polywneg;
  polywneg = vec_madd((vec_float4)(((float)(-0.2988439998f))), w, (vec_float4)(((float)(-0.3997655209f))));
  polywneg = vec_madd(polywneg                  , w, (vec_float4)(((float)(-0.6666679125f))));
  
  vec_float4 y = vec_madd(z, vec_madd(polywneg, w, x1), vzero);
  vec_float4 wnew = vec_ctf(xexp, 0);
  
  vec_float4 zz1 = vec_madd((vec_float4)(((float) 0.4342944622040f)), x1, 
			    vec_madd((vec_float4)(((float) 0.3010299205780f)), wnew, vzero));
  vec_float4 zz2 = vec_madd((vec_float4)(((float) 1.9699272335e-8f)), x1,
			    vec_madd((vec_float4)(((float) 7.5085978266e-8f)), wnew, 
				     vec_madd((vec_float4)(((float)-0.4342944819033f)), y, vzero)));
  
  vec_float4 res = vec_sel(vec_add(zz1, zz2), (vec_float4)vec_or((vec_uint4)expmask, smask), iszero);
  res = vec_sel(res, x, isinf);
  return vec_sel(x, res, isnan);
}

# 152 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/log10f4fast.h"




 
 
 
 
 
 
 
 
 
 
 
 
 













 
extern inline vec_float4 log10f4fast (vec_float4 x) 
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  return vec_madd(log2f4fast(x), (vec_float4)(0.30102999566398f), (vec_float4)vzero);
}

# 153 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/log1pf4.h"




 




extern inline vec_float4 log1pf4 (vec_float4 x)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_uint4 iszero = (vec_uint4)vec_cmpeq(x, vzero);
  vec_uint4 isnan = (vec_uint4)vec_cmpeq(x, x);
  vec_uint4 nearzeromask = (vec_uint4)vec_and(vec_cmpgt(x, (vec_float4)(-0.5f)),
					      vec_cmpgt((vec_float4)(0.5f), x));
  vec_float4 x2 = vec_madd(x,x,vzero);
  vec_float4 d0, d1, n0, n1;

  d0 = vec_madd(x , (vec_float4)(1.5934420741f), (vec_float4)(0.8952856868f));
  d1 = vec_madd(x , (vec_float4)(0.1198195734f), (vec_float4)(0.8377145063f));
  d1 = vec_madd(x2, d1, d0);
  
  n0 = vec_madd(x , (vec_float4)(1.1457993413f), (vec_float4)(0.8952856678f));
  n1 = vec_madd(x , (vec_float4)(0.0082862580f), (vec_float4)(0.3394238808f));
  n1 = vec_madd(x2, n1, n0);
 
  vec_float4 res = vec_sel(logf4(vec_add(x, (vec_float4)(1.0f))),
                 vec_madd(x, divf4(n1, d1), vzero), nearzeromask);
  res = vec_sel(res, x, iszero);
  res = vec_sel(x, res, isnan);
  return res;
}

# 154 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/log1pf4fast.h"




 




extern inline vec_float4 log1pf4fast (vec_float4 x)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_float4 x2 = vec_madd(x,x,vzero);
  vec_float4 d0, d1, n0, n1;

  d0 = vec_madd(x , (vec_float4)(1.5934420741f), (vec_float4)(0.8952856868f));
  d1 = vec_madd(x , (vec_float4)(0.1198195734f), (vec_float4)(0.8377145063f));
  d1 = vec_madd(x2, d1, d0);
  
  n0 = vec_madd(x , (vec_float4)(1.1457993413f), (vec_float4)(0.8952856678f));
  n1 = vec_madd(x , (vec_float4)(0.0082862580f), (vec_float4)(0.3394238808f));
  n1 = vec_madd(x2, n1, n0);

  return vec_madd(x, divf4(n1, d1), vzero);
}

# 155 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/logbf4.h"




 




extern inline vec_float4 logbf4 (vec_float4 x)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_int4 expmask = (vec_int4)(0x7F800000);
  vec_uint4 isnan = (vec_uint4)vec_cmpeq(x, x);
  vec_int4 e1 = vec_and((vec_int4)x, expmask);
  vec_uint4 isinf = (vec_uint4)vec_cmpeq(e1, expmask);
  vec_uint4 zeromask = (vec_uint4)vec_cmpeq(e1, (vec_int4)vzero);
  e1 = vec_sub(e1, (vec_int4)(0x3F800000));
  vec_float4 res = vec_sel(vec_ctf(e1,23), __extension__ ((vec_float4){-((float)((float)(1e+30F * 1e+30F))), -((float)((float)(1e+30F * 1e+30F))), -((float)((float)(1e+30F * 1e+30F))), -((float)((float)(1e+30F * 1e+30F)))}), zeromask);
  res = vec_sel(res, (vec_float4)expmask, isinf);
  return vec_sel(x, res, isnan);
}

# 156 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/ilogbf4.h"




 




extern inline vec_int4 ilogbf4 (vec_float4 x)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_int4 minus127 = (vec_int4)(-127);

  vec_int4 e1 = vec_and((vec_int4)x, (vec_int4)(0x7F800000));
  vec_uint4 zeromask = (vec_uint4)vec_cmpeq(e1, (vec_int4)vzero);
  vec_int4 e2 = vec_add(vec_sr(e1,(vec_uint4)(23)), minus127);
  
  return vec_sel(e2, (vec_int4)((-0x7fffffff - 1)), zeromask);

}

# 157 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/exp2f4.h"




 
 
 
 
 
 
 
 
 
 
 
 
 









































 


extern inline vec_float4 exp2f4 (vec_float4 x)
{
  vec_int4 ix;
  vec_uint4 overflow;
  vec_uint4 underflow;
  vec_float4 frac, frac2, frac4;
  vec_float4 exp_int, exp_frac;
  vec_float4 result;
  vec_float4 hi, lo;
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_float4 bias;
  
 

  bias = (vec_float4)(vec_sra((vec_int4)(x), (vec_uint4)(31) ));
  bias = (vec_float4)(vec_andc((vec_uint4)(0x3F7FFFFF), (vec_uint4)bias));
  ix = vec_cts(vec_add(x, bias), 0);  




  frac = vec_sub(vec_ctf(ix, 0), x);
  frac = vec_madd(frac, (vec_float4)(0.69314718055995f), vzero);

  
  
  overflow  = (vec_uint4)vec_cmpgt(x, (vec_float4)((vec_int4)(0x4300FFFF))); 
  underflow = (vec_uint4)vec_cmpgt((vec_float4)(-126.0f), x);

  
                                                          
  exp_int = (vec_float4)(vec_sl(vec_add(ix, (vec_int4)(126)), (vec_uint4)(23)));   

  



 
  frac2 = vec_madd(frac, frac, vzero);
  frac4 = vec_madd(frac2, frac2, vzero);

  hi = vec_madd(frac, (vec_float4)(-0.0001413161), (vec_float4)(0.0013298820));
  hi = vec_madd(frac, hi, (vec_float4)(-0.0083013598));
  hi = vec_madd(frac, hi, (vec_float4)(0.0416573475));
  lo = vec_madd(frac, (vec_float4)(-0.1666653019), (vec_float4)(0.4999999206));
  lo = vec_madd(frac, lo, (vec_float4)(-0.9999999995));
  lo = vec_madd(frac, lo, (vec_float4)(1.0));

  exp_frac = vec_madd(frac4, hi, lo);
  
  result = vec_madd(exp_frac, exp_int, vzero);
  result = vec_madd(exp_frac, exp_int, result); 

   
  result = vec_sel(result, __extension__ ((vec_float4){((float)((float)(1e+30F * 1e+30F))), ((float)((float)(1e+30F * 1e+30F))), ((float)((float)(1e+30F * 1e+30F))), ((float)((float)(1e+30F * 1e+30F)))}), overflow); 
  result = vec_sel(result, vzero, underflow);
  

  return (result);
}

# 158 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/exp2f4fast.h"




 




extern inline vec_float4 exp2f4fast (vec_float4 x)
{
  return vec_expte(x);
}







 

# 159 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/expf4.h"




 








extern inline vec_float4 expf4 (vec_float4 x) 
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_uint4 xnegmask = (vec_uint4)vec_cmpgt(vzero, x);
  vec_float4 goffset  = vec_sel((vec_float4)( 0.5f),(vec_float4)(-0.5f),xnegmask);
  vec_float4 g  = vec_madd(x, (vec_float4)(((float)1.4426950408889634f)), vzero);  
  vec_int4 xexp = vec_cts(vec_add(g, goffset),0);
  
  g = vec_ctf(xexp, 0);
  g = vec_madd(g, (vec_float4)(((float)-1.1730463525082e-7f)), vec_madd(g, (vec_float4)(((float)-0.6931470632553101f)), x));
  vec_float4 z  = vec_madd(g, g, vzero);
  vec_float4 a = vec_madd(z, (vec_float4)(0.0999748594f), vzero);
  vec_float4 b = vec_madd(g, 
			  vec_madd(z, 
				   (vec_float4)(0.0083208258f), 
				   (vec_float4)(0.4999999992f)
				   ),
			  vzero);
  
  vec_float4 foo  = divf4(vec_add((vec_float4)(1.0f), vec_add(a, b)),
			  vec_add((vec_float4)(1.0f), vec_sub(a, b)));

  return ldexpf4(foo, xexp);
  
}

# 160 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/expf4fast.h"




 
 
 
 
 
 
 
 
 
 
 
 
 
 




 







 
 
extern inline vec_float4 expf4fast (vec_float4 x)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  return exp2f4fast(vec_madd( (vec_float4)(((float)1.442695040889f)), x, vzero));
}

# 161 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/expm1f4.h"




 







extern inline vec_float4 expm1f4 (vec_float4 x) 
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_uint4 nearzeromask = (vec_uint4)vec_and(vec_cmpgt(x, (vec_float4)(((float)-0.6931471805599f))),
					      vec_cmpgt((vec_float4)(((float) 0.4054651081082f)), x));
  vec_float4 x2 = vec_madd(x,x,vzero);
  vec_float4 d0, d1, n0, n1;

  d0 = vec_madd(x , (vec_float4)(-0.3203561199f), (vec_float4)(0.9483177697f));
  d1 = vec_madd(x2, (vec_float4)( 0.0326527809f), d0);
  
  n0 = vec_madd(x , (vec_float4)(0.1538026623f), (vec_float4)(0.9483177732f));
  n1 = vec_madd(x , (vec_float4)(0.0024490478f), (vec_float4)(0.0305274668f));
  n1 = vec_madd(x2, n1, n0);
 
  return vec_sel(vec_sub(expf4(x), (vec_float4)(1.0f)),
                 vec_madd(x, divf4(n1, d1), vzero),
                 nearzeromask);
}

# 162 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/expm1f4fast.h"




 




extern inline vec_float4 expm1f4fast (vec_float4 x)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_float4 x2 = vec_madd(x,x,vzero);
  vec_float4 d0, d1, n0, n1;
  
  d0 = vec_madd(x , (vec_float4)(-0.3203561199f), (vec_float4)(0.9483177697f));
  d1 = vec_madd(x2, (vec_float4)( 0.0326527809f), d0);
  
  n0 = vec_madd(x , (vec_float4)(0.1538026623f), (vec_float4)(0.9483177732f));
  n1 = vec_madd(x , (vec_float4)(0.0024490478f), (vec_float4)(0.0305274668f));
  n1 = vec_madd(x2, n1, n0);
 
  return vec_madd(x, divf4(n1, d1), vzero);

}

# 163 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/powf4.h"




 




extern inline vec_float4 powf4 (vec_float4 x, vec_float4 y)
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0),(vec_uint4)(0));
  vec_uint4 yzero = (vec_uint4)vec_cmpeq((vec_float4)vzero, y);
  vec_uint4 negx  = (vec_uint4)vec_cmpgt((vec_int4)vzero, (vec_int4)x);

  vec_uint4 sbit = (vec_uint4)(vec_int4)(-1);
  sbit = vec_vslw(sbit, sbit);
  vec_float4 absx = vec_andc(x, (vec_float4)sbit);
  vec_float4 absy = vec_andc(y, (vec_float4)sbit);
  vec_uint4 oddy = vec_and(vec_ctu(absy, 0), (vec_uint4)(0x00000001));
  negx = vec_and(negx, (vec_uint4)vec_cmpgt(oddy, (vec_uint4)vzero));

  vec_float4 res = log2f4(absx);
  res = vec_sel(res, y, yzero);
  vec_float4 res1 = vec_madd(y, res, (vec_float4)vzero);
  res1 = exp2f4(vec_sel(res1, res, vec_and(vec_cmpeq(res, vzero),
                        (vec_uint4)vec_cmpgt((vec_int4)x,(vec_int4)vzero))));
  res1 = vec_sel(res1, vec_or((vec_float4)sbit, res1), negx);
  res1 = vec_sel(x, res1, vec_cmpeq(res, res));
  return res1;
}

# 164 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/powf4fast.h"




 




extern inline vec_float4 powf4fast (vec_float4 x, vec_float4 y)
{
  vec_int4 zeros = (vec_int4)(0);
  vec_uint4 zeromask = (vec_uint4)vec_cmpeq((vec_float4)zeros, x);

  vec_uint4 negmask  = (vec_uint4)vec_cmpgt((vec_float4)zeros, x);
  
  vec_float4 sbit = (vec_float4)((vec_int4)(0x80000000));
  vec_float4 absx = vec_andc(x, sbit);
  vec_float4 absy = vec_andc(y, sbit);
  vec_uint4 oddy = vec_and(vec_ctu(absy, 0), (vec_uint4)(0x00000001));
  negmask = vec_and(negmask, (vec_uint4)vec_cmpgt(oddy, (vec_uint4)zeros));

  vec_float4 res = exp2f4fast(vec_madd(y, log2f4(absx), (vec_float4)zeros));
  res = vec_sel(res, vec_or(sbit, res), negmask);


  return vec_sel(res, (vec_float4)zeros, zeromask);
}

# 165 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/cbrtf4.h"




 



# 31 "C:/PS3SDK\\target\\ppu\\include/bits/cbrtf4.h"

extern inline vec_float4 cbrtf4 (vec_float4 x) 
{
  vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
  vec_int4 xexp, n;
  vec_uint4 sgnmask = (vec_uint4)(vec_int4)(-1);
  sgnmask = vec_vslw(sgnmask, sgnmask);
  vec_uint4 iszero = (vec_uint4)vec_cmpeq(x, vzero);
  vec_uint4 isnan = (vec_uint4)vec_cmpeq(x, x);
  vec_uint4 negmask = (vec_uint4)vec_cmpgt(vzero, x);
  vec_float4 xx = vec_andc(x, (vec_float4)sgnmask);

  xx = frexpf4(xx, &xexp);
  vec_float4 p = vec_madd(
			  vec_madd(xx, (vec_float4)(-0.191502161678719066f), (vec_float4)(0.697570460207922770f)),
			  xx,
			  (vec_float4)(0.492659620528969547f)
			  );
  vec_float4 p3 = vec_madd(p, vec_madd(p, p, vzero), vzero);

  vec_int4 quot;
  n = xexp; vec_uint4 negxexpmask = (vec_uint4)vec_cmpgt((vec_int4)(vzero), n); n = vec_sel(n, vec_add(n,(vec_int4)(2)), negxexpmask); quot = vec_add(vec_sra(n,(vec_uint4)(2)), vec_sra(n,(vec_uint4)(4))); quot = vec_add(quot, vec_sra(quot, (vec_uint4)(4))); quot = vec_add(quot, vec_sra(quot, (vec_uint4)(8))); quot = vec_add(quot, vec_sra(quot,(vec_uint4)(16))); vec_int4 r = vec_sub(vec_sub(n,quot), vec_sl(quot,(vec_uint4)(1))); quot = vec_add( quot, vec_sra( vec_add( vec_add(r,(vec_int4)(5)), vec_sl (r,(vec_uint4)(2)) ), (vec_uint4)(4) ) );;
  vec_int4 modval = vec_sub(vec_sub(xexp,quot), vec_sl(quot,(vec_uint4)(1))); 
  vec_float4 factor =  (vec_float4)(1.0/1.5874010519681994748);
  factor = vec_sel(factor, (vec_float4)(1.0/1.2599210498948731648), vec_cmpeq(modval,(vec_int4)(-1)));
  factor = vec_sel(factor, (vec_float4)(               1.0), vec_cmpeq(modval,(vec_int4)(vzero)));
  factor = vec_sel(factor, (vec_float4)(    1.2599210498948731648), vec_cmpeq(modval,(vec_int4)( 1)));
  factor = vec_sel(factor, (vec_float4)(1.5874010519681994748), vec_cmpeq(modval,(vec_int4)( 2)));

  vec_float4 pre  = vec_madd(p, factor, vzero);
  vec_float4 numr = vec_madd(xx , (vec_float4)(2.0f), p3);
  vec_float4 denr = vec_madd(p3, (vec_float4)(2.0f), xx );
  vec_float4 res = vec_madd(pre, divf4(numr, denr), vzero);
  res = ldexpf4(res, quot);

  res = vec_sel(res, vec_or(res,(vec_float4)sgnmask), negmask);
  res = vec_sel(res, x, iszero);
  return vec_sel(x, res, isnan);
}











 

# 166 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/cbrtf4fast.h"




 



extern inline vec_float4 cbrtf4fast (vec_float4 x)
{
  return cbrtf4(x);
}











 

# 167 "C:/PS3SDK\\target\\ppu\\include/bits/sce_math.h"
                                                                                                                                                                                  


}






using ::std:: acosf4fast;
using ::std:: asinf4fast;
using ::std:: atan2f4fast;
using ::std:: atanf4fast;
using ::std:: cbrtf4fast;
using ::std:: cosf4fast;
using ::std:: divf4;
using ::std:: divf4fast;
using ::std:: exp2f4fast;
using ::std:: expf4fast;
using ::std:: expm1f4fast;
using ::std:: log10f4fast;
using ::std:: log1pf4fast;
using ::std:: log2f4fast;
using ::std:: logf4fast;
using ::std:: negatef4;
using ::std:: powf4fast;
using ::std:: recipf4;
using ::std:: recipf4fast;
using ::std:: rsqrtf4;
using ::std:: rsqrtf4fast;
using ::std:: sincosf4fast;
using ::std:: sinf4fast;
using ::std:: sqrtf4fast;
using ::std:: tanf4fast;

using ::std:: acosf4;
using ::std:: asinf4;
using ::std:: atanf4;
using ::std:: atan2f4;
using ::std:: ceilf4;
using ::std:: copysignf4;
using ::std:: cosf4;
using ::std:: expf4;
using ::std:: fabsf4;
using ::std:: floorf4;
using ::std:: fmaxf4;
using ::std:: fminf4;
using ::std:: fmodf4;
using ::std:: frexpf4;
using ::std:: ldexpf4;
using ::std:: logf4;
using ::std:: log10f4;
using ::std:: modff4;
using ::std:: remainderf4;
using ::std:: sincosf4;
using ::std:: powf4;
using ::std:: sinf4;
using ::std:: sqrtf4;
using ::std:: tanf4;
using ::std:: truncf4;

using ::std:: cbrtf4;
using ::std:: expm1f4;
using ::std:: exp2f4;
using ::std:: fdimf4;
using ::std:: fmaf4;
using ::std:: hypotf4;
using ::std:: ilogbf4;


using ::std:: log1pf4;
using ::std:: log2f4;
using ::std:: logbf4;
# 1480 "C:/PS3SDK\\target\\ppu\\include/math.h"






using ::std:: abs;


using ::std:: acos; using ::std:: asin;
using ::std:: atan; using ::std:: atan2; using ::std:: ceil;
using ::std:: cos; using ::std:: cosh; using ::std:: exp;
using ::std:: fabs; using ::std:: floor; using ::std:: fmod;
using ::std:: frexp; using ::std:: ldexp; using ::std:: log;
using ::std:: log10; using ::std:: modf; using ::std:: pow;
using ::std:: sin; using ::std:: sinh; using ::std:: sqrt;
using ::std:: tan; using ::std:: tanh;

using ::std:: acosf; using ::std:: asinf;
using ::std:: atanf; using ::std:: atan2f; using ::std:: ceilf;
using ::std:: cosf; using ::std:: coshf; using ::std:: expf;
using ::std:: fabsf; using ::std:: floorf; using ::std:: fmodf;
using ::std:: frexpf; using ::std:: ldexpf; using ::std:: logf;
using ::std:: log10f; using ::std:: modff; using ::std:: powf;
using ::std:: sinf; using ::std:: sinhf; using ::std:: sqrtf;
using ::std:: tanf; using ::std:: tanhf;

using ::std:: acosl; using ::std:: asinl;
using ::std:: atanl; using ::std:: atan2l; using ::std:: ceill;
using ::std:: cosl; using ::std:: coshl; using ::std:: expl;
using ::std:: fabsl; using ::std:: floorl; using ::std:: fmodl;
using ::std:: frexpl; using ::std:: ldexpl; using ::std:: logl;
using ::std:: log10l; using ::std:: modfl; using ::std:: powl;
using ::std:: sinl; using ::std:: sinhl; using ::std:: sqrtl;
using ::std:: tanl; using ::std:: tanhl;




using ::std:: _Rc_type; using ::std:: _Rc_widened;
using ::std:: _Real_type; using ::std:: _Real_widened;
using ::std:: _Combined_type;

using ::std:: _FPCOMP; using ::std:: fpclassify;
using ::std:: signbit; using ::std:: isfinite; using ::std:: isinf;
using ::std:: isnan; using ::std:: isnormal;
using ::std:: isgreater; using ::std:: isgreaterequal;
using ::std:: isless; using ::std:: islessequal; using ::std:: islessgreater;
using ::std:: isunordered;


using ::std:: float_t; using ::std:: double_t;

using ::std:: acosh; using ::std:: asinh; using ::std:: atanh;
using ::std:: cbrt; using ::std:: erf; using ::std:: erfc;
using ::std:: expm1; using ::std:: exp2;
using ::std:: hypot; using ::std:: ilogb; using ::std:: lgamma;
using ::std:: log1p; using ::std:: log2; using ::std:: logb;
using ::std:: llrint; using ::std:: lrint; using ::std:: nearbyint;
using ::std:: rint; using ::std:: llround; using ::std:: lround;
using ::std:: fdim; using ::std:: fma; using ::std:: fmax; using ::std:: fmin;
using ::std:: round; using ::std:: trunc;
using ::std:: remainder; using ::std:: remquo;
using ::std:: copysign; using ::std:: nan; using ::std:: nextafter;
using ::std:: scalbn; using ::std:: scalbln;
using ::std:: nexttoward; using ::std:: tgamma;

using ::std:: acoshf; using ::std:: asinhf; using ::std:: atanhf;
using ::std:: cbrtf; using ::std:: erff; using ::std:: erfcf;
using ::std:: expm1f; using ::std:: exp2f;
using ::std:: hypotf; using ::std:: ilogbf; using ::std:: lgammaf;
using ::std:: log1pf; using ::std:: log2f; using ::std:: logbf;
using ::std:: llrintf; using ::std:: lrintf; using ::std:: nearbyintf;
using ::std:: rintf; using ::std:: llroundf; using ::std:: lroundf;
using ::std:: fdimf; using ::std:: fmaf; using ::std:: fmaxf; using ::std:: fminf;
using ::std:: roundf; using ::std:: truncf;
using ::std:: remainderf; using ::std:: remquof;
using ::std:: copysignf; using ::std:: nanf;
using ::std:: nextafterf; using ::std:: scalbnf; using ::std:: scalblnf;
using ::std:: nexttowardf; using ::std:: tgammaf;

using ::std:: acoshl; using ::std:: asinhl; using ::std:: atanhl;
using ::std:: cbrtl; using ::std:: erfl; using ::std:: erfcl;
using ::std:: expm1l; using ::std:: exp2l;
using ::std:: hypotl; using ::std:: ilogbl; using ::std:: lgammal;
using ::std:: log1pl; using ::std:: log2l; using ::std:: logbl;
using ::std:: llrintl; using ::std:: lrintl; using ::std:: nearbyintl;
using ::std:: rintl; using ::std:: llroundl; using ::std:: lroundl;
using ::std:: fdiml; using ::std:: fmal; using ::std:: fmaxl; using ::std:: fminl;
using ::std:: roundl; using ::std:: truncl;
using ::std:: remainderl; using ::std:: remquol;
using ::std:: copysignl; using ::std:: nanl;
using ::std:: nextafterl; using ::std:: scalbnl; using ::std:: scalblnl;
using ::std:: nexttowardl; using ::std:: tgammal;







 
# 27 "..\\..\\src\\LinearMath/btScalar.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/stdlib.h"




 
 









namespace std {

		 








extern "C" {
extern char _Mbcurmax;
}

















		 






typedef struct
	{	 
	int quot;
	int rem;
	} div_t;

typedef struct
	{	 
	long quot;
	long rem;
	} ldiv_t;


typedef struct
	{	 
	_Longlong quot;
	_Longlong rem;
	} _Lldiv_t;



typedef _Lldiv_t lldiv_t;


		 
extern "C" {  
void exit(int) __attribute__((__noreturn__));
void _Exit(int) __attribute__((__noreturn__));	 
void exitspawn(const char *, char *const [], char *const [], sys_addr_t, size_t, int, uint64_t) __attribute__((__noreturn__));  
void _Exitspawn(const char *, char *const [], char *const [], sys_addr_t, size_t, int, uint64_t) __attribute__((__noreturn__));  
}

extern "C" {
int abs(int);
long labs(long);
void abort(void) __attribute__((__noreturn__));
void *calloc(size_t, size_t);
div_t div(int, int);
void free(void *);
ldiv_t ldiv(long, long);
void *malloc(size_t);
void *(memalign)(size_t boundary, size_t size_arg);   
int mblen(const char *, size_t);
size_t mbstowcs(wchar_t *,
	const char *, size_t);
int mbtowc(wchar_t *, const char *, size_t);
int rand(void);
void srand(unsigned int);
void *realloc(void *, size_t);


void *reallocalign(void *, size_t size_arg, size_t boundary);
size_t malloc_usable_size(void*);

struct malloc_managed_size {
  size_t max_system_size;
  size_t current_system_size;
  size_t current_inuse_size;
};

int malloc_stats(struct malloc_managed_size *);
int malloc_stats_fast(struct malloc_managed_size *);


long strtol(const char *, char **, int);
size_t wcstombs(char *,
	const wchar_t *, size_t);
int wctomb(char *, wchar_t);

unsigned long _Stoul(const char *, char **, int);
float _Stof(const char *, char **, long);
double _Stod(const char *, char **, long);
long double _Stold(const char *, char **, long);
_Longlong _Stoll(const char *, char **, int);
_ULonglong _Stoull(const char *, char **, int);

size_t _Getmbcurmax(void);


_Longlong llabs(_Longlong);
lldiv_t lldiv(_Longlong, _Longlong);


}


extern "C" {
typedef void _Atexfun(void);
typedef int _Cmpfun(const void *, const void *);

int atexit(void (*)(void));
void *bsearch(const void *, const void *,
	size_t, size_t, _Cmpfun *);
void qsort(void *, size_t, size_t, _Cmpfun *);
}

# 170 "C:/PS3SDK\\target\\ppu\\include/stdlib.h"

		
extern "C" {
inline double atof(const char *_Str)
	{	
	return (_Stod(_Str, 0, 0));
	}

inline float atoff(const char *_Str)
	{	
	return (_Stof(_Str, 0, 0));
	}

inline int atoi(const char *_Str)
	{	
	return ((int)_Stoul(_Str, 0, 10));
	}

inline long atol(const char *_Str)
	{	
	return ((long)_Stoul(_Str, 0, 10));
	}

# 200 "C:/PS3SDK\\target\\ppu\\include/stdlib.h"
inline double strtod(const char * _Str,
	char ** _Endptr)
	{	
	return (_Stod(_Str, _Endptr, 0));
	}

inline unsigned long strtoul(const char * _Str,
	char ** _Endptr, int _Base)
	{	
	return (_Stoul(_Str, _Endptr, _Base));
	}



inline _Longlong atoll(const char *_Str)
	{	
	return ((_Longlong)_Stoull(_Str, 0, 10));
	}

inline float strtof(const char * _Str,
	char ** _Endptr)
	{	
	return (_Stof(_Str, _Endptr, 0));
	}

inline long double strtold(const char * _Str,
	char ** _Endptr)
	{	
	return (_Stold(_Str, _Endptr, 0));
	}

inline _Longlong strtoll(const char * _Str,
	char ** _Endptr, int _Base)
	{	
	return (_Stoll(_Str, _Endptr, _Base));
	}

inline _ULonglong strtoull(const char * _Str,
	char ** _Endptr, int _Base)
	{	
	return (_Stoull(_Str, _Endptr, _Base));
	}


}

		


inline long abs(long _Left)
	{	
	return (labs(_Left));
	}

inline ldiv_t div(long _Left, long _Right)
	{	
	return (ldiv(_Left, _Right));
	}





inline _Longlong abs(_Longlong _Left)
	{	
	return (llabs(_Left));
	}

inline _Lldiv_t div(_Longlong _Left, _Longlong _Right)
	{	
	return (lldiv(_Left, _Right));
	}




# 321 "C:/PS3SDK\\target\\ppu\\include/stdlib.h"





extern "C" {
extern inline int get_state_TT800(void **state);
extern inline void init_TT800(unsigned int s);
extern inline void init_by_array_TT800(unsigned int init_key[], int key_length);
extern inline unsigned int rand_int32_TT800(void);
extern inline unsigned int rand_int31_TT800(void);
extern inline float rand_real1_TT800(void);
extern inline float rand_real2_TT800(void);
extern inline float rand_real3_TT800(void);
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/tt800.h"




 



 
 
 
 
 





# 1 "C:/PS3SDK\\target\\ppu\\include/bits/tt800_globals.h"




 












typedef struct 
{
  
  unsigned int State[25]; 
  
  int Next_Number;
} __TT800_State_t;

extern "C" {
extern __TT800_State_t __TT800;
}

# 20 "C:/PS3SDK\\target\\ppu\\include/bits/tt800.h"





















extern inline int get_state_TT800 (void * *state)
{
  *state = (void *)&__TT800;
  return(sizeof(__TT800));
}

 

extern inline void init_TT800 (unsigned int s)
{
  int i;
  unsigned int *p = (unsigned int *)__TT800.State;
  for(i=0; i < 25; i++)
  {
    *(p++) ^= s;
  }
  __TT800.Next_Number = 0;
}

 

extern inline void init_by_array_TT800 (unsigned int init_key[], int key_length)
{
  int i,j;
  unsigned int *p = (unsigned int *)__TT800.State;
  for(i=0,j=0; i < 25; i++, j++)
  {
    if(j > key_length) j=0;
    *(p++) ^= init_key[j];
  }
  __TT800.Next_Number = 0;
}


extern inline unsigned int rand_int32_TT800 (void)
{
  unsigned int y;
  static const unsigned int mag01[2]={0x0, 0x8ebfd028 } ; 

  
  if (__TT800.Next_Number==25)
  { 
    
    int kk;
    for (kk=0;kk<25-7;kk++)
    {
      __TT800.State[kk] = __TT800.State[kk+7] ^ (__TT800.State[kk] >> 1) ^ mag01[__TT800.State[kk] % 2];
    }
    for (; kk<25;kk++)
    {
      __TT800.State[kk] = __TT800.State[kk+(7-25)] ^ (__TT800.State[kk] >> 1) ^ mag01[__TT800.State[kk] % 2];
    }
    __TT800.Next_Number=0;
  }
  
  y = __TT800.State[__TT800.Next_Number];
  
  y ^= (y << 7) & 0x2b5b2500; 
  y ^= (y << 15) & 0xdb8b0000;
  y &= 0xffffffff; 
  y ^= (y >> 16); 
  __TT800.Next_Number++;
  return( y );
}
 
extern inline unsigned int rand_int31_TT800 (void)
{
    return (long)(rand_int32_TT800()>>1);
}

 
extern inline float rand_real1_TT800 (void)
{
    return rand_int32_TT800()*(1.0f/4294967295.0f); 
      
}

 
extern inline float rand_real2_TT800 (void)
{
    return rand_int32_TT800()*(1.0f/4294967296.0f); 
     
}

 
extern inline float rand_real3_TT800 (void)
{
  unsigned int dr = rand_int32_TT800();
    return (((float)dr) + 0.5f)*(1.0f/4294967296.0f); 
     
}

# 338 "C:/PS3SDK\\target\\ppu\\include/stdlib.h"

}

}


# 1 "C:/PS3SDK\\target\\ppu\\include/bits/sce_stdlib.h"




 



 



                       
typedef struct
{
   vec_int4 quot; 
   vec_int4 rem;  
} divi4_t;
                     
 


namespace std {


 
extern "C" {
vec_int4 absi4(vec_int4 x);
divi4_t divi4(vec_int4 numer, vec_int4 denom);
}



 



# 1 "C:/PS3SDK\\target\\ppu\\include/bits/absi4.h"




 







extern inline vec_int4 absi4 ( vec_int4 x )
{
   return vec_abs( x );
}


# 39 "C:/PS3SDK\\target\\ppu\\include/bits/sce_stdlib.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/bits/divi4.h"




 










extern inline divi4_t divi4 ( vec_int4 numer, vec_int4 denom )
{
   vec_int4 minusone = (vec_int4)(-1);
   vec_uint4 zero = (vec_uint4)(0);
   vec_uint4 one = (vec_uint4)(1);
   vec_uint4 k158 = (vec_uint4)(158);
   vec_uint4 k23 = (vec_uint4)(23);
   
   divi4_t res;
   vec_uint4 numerPos, denomPos, quotNeg;
   vec_uint4 numerAbs, denomAbs;
   vec_uint4 denomZeros, numerZeros, shift, denomShifted, oneShifted;
   vec_uint4 quot, newQuot, skip, newNum, cont;
   int       anyCont;
       
   
 
   numerPos = (vec_uint4)vec_cmpgt( numer, minusone );
   denomPos = (vec_uint4)vec_cmpgt( denom, minusone );
   quotNeg = vec_xor( numerPos, denomPos );
   
   
 
   numerAbs = (vec_uint4)vec_sel( vec_sub( (vec_int4)zero, numer ), numer, numerPos );
   denomAbs = (vec_uint4)vec_sel( vec_sub( (vec_int4)zero, denom ), denom, denomPos );
 
   

   denomZeros = vec_sub( k158, vec_sr( (vec_uint4)vec_ctf( denomAbs, 0 ), k23 ) );
   numerZeros = vec_sub( k158, vec_sr( (vec_uint4)vec_ctf( numerAbs, 0 ), k23 ) );
      
   shift = vec_sub( denomZeros, numerZeros );
   denomShifted = vec_sl( denomAbs, shift );
   oneShifted = vec_sl( one, shift );
   oneShifted = vec_sel( oneShifted, zero, vec_or( vec_cmpeq( denomAbs, zero ), 
                                                   vec_cmpgt( denomAbs, numerAbs ) ) );
   
   

   quot = zero;
   
   do
   {
      cont = (vec_uint4)vec_cmpgt( oneShifted, zero );
      anyCont = vec_any_gt( oneShifted, zero );
      skip = (vec_uint4)vec_cmpgt( denomShifted, numerAbs );
      
      newQuot = vec_or( quot, oneShifted );
      newNum = vec_sub( numerAbs, denomShifted );
      
      oneShifted = vec_sr( oneShifted, one );
      denomShifted = vec_sr( denomShifted, one );
      
      quot = vec_sel( newQuot, quot, skip );
      numerAbs = vec_sel( numerAbs, newNum, vec_andc( cont, skip ) );
   }
   while ( anyCont );

   res.quot = (vec_int4)vec_sel( quot, vec_sub( zero, quot ), quotNeg );
   res.rem = (vec_int4)vec_sel( (vec_uint4)vec_sub( (vec_int4)zero, (vec_int4)numerAbs ), numerAbs, numerPos );
   return res;
}

# 40 "C:/PS3SDK\\target\\ppu\\include/bits/sce_stdlib.h"


}






using ::std:: absi4;
using ::std:: divi4;
# 346 "C:/PS3SDK\\target\\ppu\\include/stdlib.h"





using ::std:: div_t; using ::std:: ldiv_t;

using ::std:: abort; using ::std:: abs; using ::std:: atexit;
using ::std:: atof; using ::std:: atoi; using ::std:: atol;
using ::std:: bsearch; using ::std:: calloc; using ::std:: div;
using ::std:: exit; using ::std:: free; 
using ::std:: labs; using ::std:: ldiv; using ::std:: malloc;
using ::std:: mblen; using ::std:: mbstowcs; using ::std:: mbtowc;
using ::std:: qsort; using ::std:: rand; 
using ::std:: realloc;
using ::std:: srand; using ::std:: strtod; using ::std:: strtol;
using ::std:: strtoul; 
using ::std:: wcstombs; using ::std:: wctomb;
using ::std:: memalign;   
using ::std:: atoff;


using  ::std:: reallocalign;
using  ::std:: malloc_usable_size;
using  ::std:: malloc_stats;
using  ::std:: malloc_stats_fast;
using  ::std:: malloc_managed_size;



using ::std:: lldiv_t;

using ::std:: atoll; using ::std:: llabs; using ::std:: lldiv;
using ::std:: strtof; using ::std:: strtold;
using ::std:: strtoll; using ::std:: strtoull;
using ::std:: _Exit;


using ::std:: exitspawn; using ::std:: _Exitspawn;

using ::std:: get_state_TT800; using ::std:: init_TT800; using ::std:: init_by_array_TT800;
using ::std:: rand_int32_TT800; using ::std:: rand_int31_TT800;
using ::std:: rand_real1_TT800; using ::std:: rand_real2_TT800; using ::std:: rand_real3_TT800;






 
# 28 "..\\..\\src\\LinearMath/btScalar.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/cstdlib"




 





# 1 "C:/PS3SDK\\target\\ppu\\include/stdlib.h"




 
 
# 343 "C:/PS3SDK\\target\\ppu\\include/stdlib.h"

# 1 "C:/PS3SDK\\target\\ppu\\include/bits/sce_stdlib.h"




 
# 45 "C:/PS3SDK\\target\\ppu\\include/bits/sce_stdlib.h"

# 346 "C:/PS3SDK\\target\\ppu\\include/stdlib.h"


# 391 "C:/PS3SDK\\target\\ppu\\include/stdlib.h"




 
# 14 "C:/PS3SDK\\target\\ppu\\include/cstdlib"


# 30 "C:/PS3SDK\\target\\ppu\\include/cstdlib"

# 96 "C:/PS3SDK\\target\\ppu\\include/cstdlib"




 
# 29 "..\\..\\src\\LinearMath/btScalar.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/cfloat"





# 1 "C:/PS3SDK\\target\\ppu\\include/float.h"




 
 






namespace std {

		 


typedef struct
	{	 
	int _Ddig, _Dmdig, _Dmax10e, _Dmaxe, _Dmin10e, _Dmine;
	union
		{	 
		unsigned short _Us[8];
		float _Float;
		double _Double;
		long double _Long_double;
		} _Deps, _Dmax, _Dmin;
	} _Dvals;


		 
extern "C" {
extern   _Dvals _Dbl, _Flt, _Ldbl;
}

		 





















# 64 "C:/PS3SDK\\target\\ppu\\include/float.h"



		 

   

   

   
# 86 "C:/PS3SDK\\target\\ppu\\include/float.h"

		 

   

   

   
# 106 "C:/PS3SDK\\target\\ppu\\include/float.h"

		 


    

    

    
# 128 "C:/PS3SDK\\target\\ppu\\include/float.h"

}





 
# 7 "C:/PS3SDK\\target\\ppu\\include/cfloat"

# 40 "C:/PS3SDK\\target\\ppu\\include/cfloat"






 
# 30 "..\\..\\src\\LinearMath/btScalar.h"


 


inline int	btGetVersion()
{
	return 277;
}





# 51 "..\\..\\src\\LinearMath/btScalar.h"








# 94 "..\\..\\src\\LinearMath/btScalar.h"
	
# 1 "C:/PS3SDK\\target\\ppu\\include/assert.h"




 
 




namespace std {



# 21 "C:/PS3SDK\\target\\ppu\\include/assert.h"

# 34 "C:/PS3SDK\\target\\ppu\\include/assert.h"

# 44 "C:/PS3SDK\\target\\ppu\\include/assert.h"

# 52 "C:/PS3SDK\\target\\ppu\\include/assert.h"





extern "C" {
void _Assert(const char *, const char *);
void _SCE_Assert(const char *, const char *);
}



}




 
# 102 "..\\..\\src\\LinearMath/btScalar.h"
# 1 "C:/PS3SDK\\target\\ppu\\include/alloca.h"




 







extern "C" {


extern void* alloca (unsigned int size);


}








# 104 "..\\..\\src\\LinearMath/btScalar.h"

# 113 "..\\..\\src\\LinearMath/btScalar.h"
	



		





# 204 "..\\..\\src\\LinearMath/btScalar.h"





 

















 
typedef float btScalar;









# 247 "..\\..\\src\\LinearMath/btScalar.h"

# 258 "..\\..\\src\\LinearMath/btScalar.h"


# 277 "..\\..\\src\\LinearMath/btScalar.h"
		
inline __attribute__((always_inline)) btScalar btSqrt(btScalar y) 
{ 
# 295 "..\\..\\src\\LinearMath/btScalar.h"
	return sqrtf(y); 

}
inline __attribute__((always_inline)) btScalar btFabs(btScalar x) { return fabsf(x); }
inline __attribute__((always_inline)) btScalar btCos(btScalar x) { return cosf(x); }
inline __attribute__((always_inline)) btScalar btSin(btScalar x) { return sinf(x); }
inline __attribute__((always_inline)) btScalar btTan(btScalar x) { return tanf(x); }
inline __attribute__((always_inline)) btScalar btAcos(btScalar x) { 
	if (x<btScalar(-1))	
		x=btScalar(-1); 
	if (x>btScalar(1))	
		x=btScalar(1);
	return acosf(x); 
}
inline __attribute__((always_inline)) btScalar btAsin(btScalar x) { 
	if (x<btScalar(-1))	
		x=btScalar(-1); 
	if (x>btScalar(1))	
		x=btScalar(1);
	return asinf(x); 
}
inline __attribute__((always_inline)) btScalar btAtan(btScalar x) { return atanf(x); }
inline __attribute__((always_inline)) btScalar btAtan2(btScalar x, btScalar y) { return atan2f(x, y); }
inline __attribute__((always_inline)) btScalar btExp(btScalar x) { return expf(x); }
inline __attribute__((always_inline)) btScalar btLog(btScalar x) { return logf(x); }
inline __attribute__((always_inline)) btScalar btPow(btScalar x,btScalar y) { return powf(x,y); }
inline __attribute__((always_inline)) btScalar btFmod(btScalar x,btScalar y) { return fmodf(x,y); }
	


# 331 "..\\..\\src\\LinearMath/btScalar.h"




# 342 "..\\..\\src\\LinearMath/btScalar.h"

inline __attribute__((always_inline)) btScalar btAtan2Fast(btScalar y, btScalar x) 
{
	btScalar coeff_1 = (btScalar(6.283185307179586232) * btScalar(0.5)) / 4.0f;
	btScalar coeff_2 = 3.0f * coeff_1;
	btScalar abs_y = btFabs(y);
	btScalar angle;
	if (x >= 0.0f) {
		btScalar r = (x - abs_y) / (x + abs_y);
		angle = coeff_1 - coeff_1 * r;
	} else {
		btScalar r = (x + abs_y) / (abs_y - x);
		angle = coeff_2 - coeff_1 * r;
	}
	return (y < 0.0f) ? -angle : angle;
}

inline __attribute__((always_inline)) bool      btFuzzyZero(btScalar x) { return btFabs(x) < 1.1920928955078125e-07f; }

inline __attribute__((always_inline)) bool	btEqual(btScalar a, btScalar eps) {
	return (((a) <= eps) && !((a) < -eps));
}
inline __attribute__((always_inline)) bool	btGreaterEqual (btScalar a, btScalar eps) {
	return (!((a) <= eps));
}


inline __attribute__((always_inline)) int       btIsNegative(btScalar x) {
    return x < btScalar(0.0) ? 1 : 0;
}

inline __attribute__((always_inline)) btScalar btRadians(btScalar x) { return x * (btScalar(6.283185307179586232) / btScalar(360.0)); }
inline __attribute__((always_inline)) btScalar btDegrees(btScalar x) { return x * (btScalar(360.0) / btScalar(6.283185307179586232)); }




inline __attribute__((always_inline)) btScalar btFsel(btScalar a, btScalar b, btScalar c)
{
	return a >= btScalar(0.f) ? b : c;
}




inline __attribute__((always_inline)) bool btMachineIsLittleEndian()
{
   long int i = 1;
   const char *p = (const char *) &i;
   if (p[0] == 1)  
	   return true;
   else
	   return false;
}





inline __attribute__((always_inline)) unsigned btSelect(unsigned condition, unsigned valueIfConditionNonZero, unsigned valueIfConditionZero) 
{
    
    
    
    
    unsigned testNz = (unsigned)(((int)condition | -(int)condition) >> 31);
    unsigned testEqz = ~testNz;
    return ((valueIfConditionNonZero & testNz) | (valueIfConditionZero & testEqz)); 
}
inline __attribute__((always_inline)) int btSelect(unsigned condition, int valueIfConditionNonZero, int valueIfConditionZero)
{
    unsigned testNz = (unsigned)(((int)condition | -(int)condition) >> 31);
    unsigned testEqz = ~testNz; 
    return static_cast<int>((valueIfConditionNonZero & testNz) | (valueIfConditionZero & testEqz));
}
inline __attribute__((always_inline)) float btSelect(unsigned condition, float valueIfConditionNonZero, float valueIfConditionZero)
{



    return (condition != 0) ? valueIfConditionNonZero : valueIfConditionZero; 

}

template<typename T> inline __attribute__((always_inline)) void btSwap(T& a, T& b)
{
	T tmp = a;
	a = b;
	b = tmp;
}



inline __attribute__((always_inline)) unsigned btSwapEndian(unsigned val)
{
	return (((val & 0xff000000) >> 24) | ((val & 0x00ff0000) >> 8) | ((val & 0x0000ff00) << 8)  | ((val & 0x000000ff) << 24));
}

inline __attribute__((always_inline)) unsigned short btSwapEndian(unsigned short val)
{
	return static_cast<unsigned short>(((val & 0xff00) >> 8) | ((val & 0x00ff) << 8));
}

inline __attribute__((always_inline)) unsigned btSwapEndian(int val)
{
	return btSwapEndian((unsigned)val);
}

inline __attribute__((always_inline)) unsigned short btSwapEndian(short val)
{
	return btSwapEndian((unsigned short) val);
}







inline __attribute__((always_inline)) unsigned int  btSwapEndianFloat(float d)
{
    unsigned int a = 0;
    unsigned char *dst = (unsigned char *)&a;
    unsigned char *src = (unsigned char *)&d;

    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];
    return a;
}


inline __attribute__((always_inline)) float btUnswapEndianFloat(unsigned int a) 
{
    float d = 0.0f;
    unsigned char *src = (unsigned char *)&a;
    unsigned char *dst = (unsigned char *)&d;

    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];

    return d;
}



inline __attribute__((always_inline)) void  btSwapEndianDouble(double d, unsigned char* dst)
{
    unsigned char *src = (unsigned char *)&d;

    dst[0] = src[7];
    dst[1] = src[6];
    dst[2] = src[5];
    dst[3] = src[4];
    dst[4] = src[3];
    dst[5] = src[2];
    dst[6] = src[1];
    dst[7] = src[0];

}


inline __attribute__((always_inline)) double btUnswapEndianDouble(const unsigned char *src) 
{
    double d = 0.0;
    unsigned char *dst = (unsigned char *)&d;

    dst[0] = src[7];
    dst[1] = src[6];
    dst[2] = src[5];
    dst[3] = src[4];
    dst[4] = src[3];
    dst[5] = src[2];
    dst[6] = src[1];
    dst[7] = src[0];

	return d;
}


inline __attribute__((always_inline)) btScalar btNormalizeAngle(btScalar angleInRadians) 
{
	angleInRadians = btFmod(angleInRadians, btScalar(6.283185307179586232));
	if(angleInRadians < -(btScalar(6.283185307179586232) * btScalar(0.5)))
	{
		return angleInRadians + btScalar(6.283185307179586232);
	}
	else if(angleInRadians > (btScalar(6.283185307179586232) * btScalar(0.5)))
	{
		return angleInRadians - btScalar(6.283185307179586232);
	}
	else
	{
		return angleInRadians;
	}
}


struct btTypedObject
{
	btTypedObject(int objectType)
		:m_objectType(objectType)
	{
	}
	int	m_objectType;
	inline int getObjectType() const
	{
		return m_objectType;
	}
};
# 20 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btBroadphaseProxy.h"
# 1 "..\\..\\src\\LinearMath/btVector3.h"












 



# 1 "..\\..\\src\\LinearMath/btMinMax.h"












 








template <class T>
inline __attribute__((always_inline)) const T& btMin(const T& a, const T& b) 
{
  return a < b ? a : b ;
}

template <class T>
inline __attribute__((always_inline)) const T& btMax(const T& a, const T& b) 
{
  return  a > b ? a : b;
}

template <class T>
inline __attribute__((always_inline)) const T& btClamped(const T& a, const T& lb, const T& ub) 
{
	return a < lb ? lb : (ub < a ? ub : a); 
}

template <class T>
inline __attribute__((always_inline)) void btSetMin(T& a, const T& b) 
{
    if (b < a) 
	{
		a = b;
	}
}

template <class T>
inline __attribute__((always_inline)) void btSetMax(T& a, const T& b) 
{
    if (a < b) 
	{
		a = b;
	}
}

template <class T>
inline __attribute__((always_inline)) void btClamp(T& a, const T& lb, const T& ub) 
{
	if (a < lb) 
	{
		a = lb; 
	}
	else if (ub < a) 
	{
		a = ub;
	}
}

# 19 "..\\..\\src\\LinearMath/btVector3.h"

# 27 "..\\..\\src\\LinearMath/btVector3.h"






const vec_int4 btElementMask = {1, 2, 4, 8};

const vec_float4 btZero = {0.f, 0.f, 0.f, 0.f};
const vec_float4 btOne = {1.f, 1.f, 1.f, 1.f};

const vec_float4 btIdentVec0 = {1.f, 0.f, 0.f, 0.f};
const vec_float4 btIdentVec1 = {0.f, 1.f, 0.f, 0.f};
const vec_float4 btIdentVec2 = {0.f, 0.f, 1.f, 0.f};


const vec_uint4 btSelVec0 = {0xffffffff, 0, 0, 0};
const vec_uint4 btSelVec1 = {0, 0xffffffff, 0, 0};
const vec_uint4 btSelVec2 = {0, 0, 0xffffffff, 0};
const vec_uint4 btSelVec3 = {0, 0, 0, 0xffffffff};
const vec_int4 btIntZero = {0, 0, 0, 0};

const vec_float4 btLargeFloatVec = {1e18f, 1e18f, 1e18f, 1e18f};

static inline __attribute__((always_inline)) const vec_float4 btZeroVec()
{
	return btZero;
}





 
class __attribute__ ((aligned (16))) btVector3
{
public:
	static void btHitPrint(unsigned int id);


	union
	{
		vec_float4 mVec;
		float m_floats[4];
	};
	
	inline __attribute__((always_inline)) const vec_float4*	get128ptr() const
	{
		return &mVec;
	}

	inline __attribute__((always_inline)) const vec_float4&	get128() const
	{
		return mVec;
	}

	inline __attribute__((always_inline))	void set128(vec_float4 v)
	{
		mVec = v;
	}

	inline __attribute__((always_inline)) btVector3(const vec_float4 vec):mVec(vec)
	{
	}

	inline __attribute__((always_inline)) btVector3(const btScalar vec)
	{
		mVec = vec_float4(vec);
	}
	
# 115 "..\\..\\src\\LinearMath/btVector3.h"
	


   
	inline __attribute__((always_inline)) btVector3():mVec(btZero)  
	{
	}

	



 
	inline __attribute__((always_inline)) btVector3(const btScalar& x, const btScalar& y, const btScalar& z)
	{
		m_floats[0] = x;
		m_floats[1] = y;
		m_floats[2] = z;
	}

	
 
	inline __attribute__((always_inline)) btVector3& operator+=(const btVector3& v)
	{
		mVec = vec_add(v.mVec, mVec);
		
		return *this;
	}

  
 
	inline __attribute__((always_inline)) btVector3& operator-=(const btVector3& v) 
	{
		mVec = vec_sub(mVec, v.mVec);
		
		return *this;
	}

  
 
	inline __attribute__((always_inline)) btVector3& operator*=(const btScalar& s)
	{
		
		mVec = vec_madd(mVec, vec_float4(s), btZeroVec());
		return *this;
	}

	
 
	inline __attribute__((always_inline)) btScalar dot(const btVector3& v) const
	{
		
		
		
		vec_float4 tmp = vec_madd(mVec, v.mVec, btZeroVec());
		vec_float4 result = vec_add(tmp, vec_sld(tmp, tmp, 4));
		return btScalar(vec_extract(vec_add(result, vec_sld(tmp, tmp, 8)), 0));
	}

	inline __attribute__((always_inline)) btVector3& safeNormalize() 
	{
		if(vec_any_ne(vec_sel(mVec, btZero, btSelVec3), btZero))
		{
			return *this /= length();
		}
		
		mVec = btIdentVec0;
		return *this;
	}

  
 
	inline __attribute__((always_inline)) btVector3& normalize() 
	{
		return *this /= length();
	}

   
	inline __attribute__((always_inline)) btVector3 absolute() const 
	{
		return vec_abs(mVec);
	}
  
 
	inline __attribute__((always_inline)) btVector3 cross(const btVector3& v) const
	{
		vec_float4 vec1021 = vec_vmrghw(vec_sld(mVec, mVec, 4), mVec);
		vec_float4 vec1201a = vec_vmrghw(vec1021, vec_sld(vec1021, vec1021, 8));

		vec1021 = vec_vmrghw(vec_sld(v.mVec, v.mVec, 4), v.mVec);
		vec_float4 vec1201b = vec_vmrghw(vec1021, vec_sld(vec1021, vec1021, 8));

		return vec_sub(vec_madd(vec1201a, vec_sld(vec1201b, vec1201b, 4), btZeroVec()), vec_madd(vec1201b, vec_sld(vec1201a, vec1201a, 4), btZeroVec()));
	}

	inline __attribute__((always_inline)) btScalar triple(const btVector3& v1, const btVector3& v2) const
	{
		return dot(v1.cross(v2));
	}

	inline __attribute__((always_inline)) void setInterpolate3(const btVector3& v0, const btVector3& v1, btScalar rt) 
	{
		




 
		vec_float4 s = vec_float4(1.0f - rt);
		mVec = vec_madd(s, v0.mVec, vec_madd(vec_float4(rt), v1.mVec, btZeroVec()));
	}

  

 
	inline __attribute__((always_inline)) btVector3 lerp(const btVector3& v, const btScalar& t) const 
	{
		



 
		return btVector3(vec_madd(vec_float4(t), vec_sub(v.mVec, mVec), mVec));
	}

	inline __attribute__((always_inline)) vec_float4 re() const
	{
		vec_float4 vre = vec_re( mVec );
		return vec_madd(vec_nmsub( mVec, vre, btOne ), vre, vre);
	}

  
 
	inline __attribute__((always_inline)) btVector3& operator*=(const btVector3& v)
	{
		mVec = vec_madd(mVec, v.mVec, btZeroVec());
		
		return *this;
	}
	
 
	inline __attribute__((always_inline)) btVector3& operator/=(const btScalar& s) 
	{
		;
		return *this *= btScalar(1.0) / s;
	}

	 
	inline __attribute__((always_inline)) btScalar length2() const
	{
		return dot(*this);
	}

	 
	inline __attribute__((always_inline)) btScalar length() const
	{
		return btSqrt(length2());
	}
	
	
 
	inline __attribute__((always_inline)) btScalar angle(const btVector3& v) const 
	{
		btScalar s = btSqrt(length2() * v.length2());
		;
		return btAcos(dot(v) / s);
	}
  
	
 
	inline __attribute__((always_inline)) int minAxis() const
	{
		return m_floats[0] < m_floats[1] ? (m_floats[0] <m_floats[2] ? 0 : 2) : (m_floats[1] <m_floats[2] ? 1 : 2);
		







 
	}

  
  
	inline __attribute__((always_inline)) int maxAxis() const 
	{
		return m_floats[0] < m_floats[1] ? (m_floats[1] <m_floats[2] ? 2 : 1) : (m_floats[0] <m_floats[2] ? 2 : 0);
		





 
	}

	inline __attribute__((always_inline)) int furthestAxis() const
	{
		return absolute().minAxis();
	}

	inline __attribute__((always_inline)) int closestAxis() const 
	{
		return absolute().maxAxis();
	}

	
 
	inline __attribute__((always_inline)) btScalar distance2(const btVector3& v) const;

	
 
	inline __attribute__((always_inline)) btScalar distance(const btVector3& v) const;

	 
	inline __attribute__((always_inline)) btVector3 normalized() const;

	

 
	inline __attribute__((always_inline)) btVector3 rotate( const btVector3& wAxis, const btScalar angle ) const;

	
	inline __attribute__((always_inline)) btScalar getX() const { return (btScalar)m_floats[0];}
  	inline __attribute__((always_inline)) btScalar getY() const { return (btScalar)m_floats[1];}
  	inline __attribute__((always_inline)) btScalar getZ() const { return (btScalar)m_floats[2];}
	inline __attribute__((always_inline)) btScalar getW() const { return (btScalar)m_floats[3];}
	inline __attribute__((always_inline)) btScalar get(unsigned int i) const {((i < 4) ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btVector3.h" ":" "344" " " "i < 4", 0)); return (btScalar)m_floats[i];}
  	inline __attribute__((always_inline)) void	setX(btScalar x) { m_floats[0] = x;}
  	inline __attribute__((always_inline)) void	setY(btScalar y) { m_floats[1] = y;}
  	inline __attribute__((always_inline)) void	setZ(btScalar z) { m_floats[2] = z;}
  	inline __attribute__((always_inline)) void	setW(btScalar w) { m_floats[3] = w;}
	inline __attribute__((always_inline)) void	set(unsigned int i, btScalar v) {((i < 4) ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btVector3.h" ":" "349" " " "i < 4", 0));   m_floats[i] = v;}
  
	inline __attribute__((always_inline))	operator       btScalar *()       { return (btScalar*)m_floats;}
	inline __attribute__((always_inline))	operator const btScalar *() const { return (btScalar*)m_floats;}

	inline __attribute__((always_inline))	bool	operator==(const btVector3& other) const
	{
		return vec_all_eq(vec_sel(mVec, other.mVec, btSelVec3), other.mVec);
	}

	inline __attribute__((always_inline))	bool	operator!=(const btVector3& other) const
	{
		return !(*this == other);
	}

	 

 
	inline __attribute__((always_inline)) void	setMax(const btVector3& other)
	{
		mVec = vec_max(mVec, other.mVec);
	}

	

 
	inline __attribute__((always_inline)) void	setMin(const btVector3& other)
	{
		mVec = vec_min(mVec, other.mVec);
	}

	void	setZero()
	{
		mVec = btZeroVec();
	}

  

		inline __attribute__((always_inline)) void 	setValue(const btScalar& x, const btScalar& y, const btScalar& z)
		{
			m_floats[0]=x;
			m_floats[1]=y;
			m_floats[2]=z;
		}

		void	getSkewSymmetricMatrix(btVector3* v0,btVector3* v1,btVector3* v2) const
		{
			v0->setValue(0.		,-getZ()		,getY());
			v1->setValue(getZ()	,0.			,-getX());
			v2->setValue(-getY()	,getX()	,0.);
		}

		inline __attribute__((always_inline)) bool isZero() const 
		{
			return vec_all_eq(vec_sel(mVec, btZero, btSelVec3), btZero);
		}

		inline __attribute__((always_inline)) bool fuzzyZero() const 
		{
			return length2() < 1.1920928955078125e-07f;
		}

		inline __attribute__((always_inline))	void	serialize(struct	btVector3FloatData& dataOut) const;

		inline __attribute__((always_inline))	void	deSerialize(const struct	btVector3FloatData& dataIn);

		inline __attribute__((always_inline))	void	serializeFloat(struct	btVector3FloatData& dataOut) const;

		inline __attribute__((always_inline))	void	deSerializeFloat(const struct	btVector3FloatData& dataIn);

		inline __attribute__((always_inline))	void	serializeDouble(struct	btVector3DoubleData& dataOut) const;

		inline __attribute__((always_inline))	void	deSerializeDouble(const struct	btVector3DoubleData& dataIn);
};

 
inline __attribute__((always_inline)) btVector3 
operator+(const btVector3& v1, const btVector3& v2) 
{
	
	return btVector3(vec_add(v1.mVec, v2.mVec));
}

 
inline __attribute__((always_inline)) btVector3 
operator*(const btVector3& v1, const btVector3& v2) 
{
	
	return btVector3(vec_madd(v1.mVec, v2.mVec, btZeroVec()));
}

 
inline __attribute__((always_inline)) btVector3 
operator-(const btVector3& v1, const btVector3& v2)
{
	
	return btVector3(vec_sub(v1.mVec, v2.mVec));
}

 
inline __attribute__((always_inline)) btVector3 
operator-(const btVector3& v)
{
	
	return btVector3(vec_sub(btZeroVec(), v.mVec));
}

 
inline __attribute__((always_inline)) btVector3
operator/(const btVector3& v1, const btVector3& v2)
{
	
	
	
	vec_float4 vre = vec_re( v2.mVec );
	vec_float4 r = vec_madd(vec_nmsub( v2.mVec, vre, btOne ), vre, vre);
	return btVector3(vec_madd(v1.mVec, r, btZero));
	
}

 
inline __attribute__((always_inline)) btVector3 
operator*(const btVector3& v, const btScalar& s)
{
	
	return btVector3(vec_madd(v.mVec, vec_float4(s), btZeroVec())); 
}

 
inline __attribute__((always_inline)) btVector3 
operator*(const btScalar& s, const btVector3& v)
{ 
	return v * s; 
}

 
inline __attribute__((always_inline)) btVector3
operator/(const btVector3& v, const btScalar& s)
{
	;
	return v * (btScalar(1.0) / s);
}

 
inline __attribute__((always_inline)) btScalar 
btDot(const btVector3& v1, const btVector3& v2) 
{ 
	return v1.dot(v2); 
}


 
inline __attribute__((always_inline)) btScalar
btDistance2(const btVector3& v1, const btVector3& v2) 
{ 
	return v1.distance2(v2); 
}


 
inline __attribute__((always_inline)) btScalar
btDistance(const btVector3& v1, const btVector3& v2) 
{ 
	return v1.distance(v2); 
}

 
inline __attribute__((always_inline)) btScalar
btAngle(const btVector3& v1, const btVector3& v2) 
{ 
	return v1.angle(v2); 
}

 
inline __attribute__((always_inline)) btVector3 
btCross(const btVector3& v1, const btVector3& v2) 
{ 
	return v1.cross(v2); 
}

inline __attribute__((always_inline)) btScalar
btTriple(const btVector3& v1, const btVector3& v2, const btVector3& v3)
{
	return v1.triple(v2, v3);
}




 
inline __attribute__((always_inline)) btVector3 
lerp(const btVector3& v1, const btVector3& v2, const btScalar& t)
{
	return v1.lerp(v2, t);
}



inline __attribute__((always_inline)) btScalar btVector3::distance2(const btVector3& v) const
{
	return (v - *this).length2();
}

inline __attribute__((always_inline)) btScalar btVector3::distance(const btVector3& v) const
{
	return (v - *this).length();
}

inline __attribute__((always_inline)) btVector3 btVector3::normalized() const
{
	return *this / length();
} 

inline __attribute__((always_inline)) btVector3 btVector3::rotate( const btVector3& wAxis, const btScalar angle ) const
{
	


	
	vec_float4 tmp = vec_madd(wAxis.mVec, mVec, btZero);
	tmp = vec_splat(vec_add(vec_add(tmp, vec_sld(tmp, tmp, 4)), vec_sld(tmp, tmp, 8)), 0);
	btVector3 o = vec_madd(wAxis.mVec, tmp, btZeroVec());



	btVector3 x = *this - o;
	btVector3 y;

	y = wAxis.cross( *this );

	return ( o + x * btCos( angle ) + y * btSin( angle ) );
}

class btVector4 : public btVector3
{
public:

	inline __attribute__((always_inline)) btVector4() {}

	inline __attribute__((always_inline)) btVector4(const btScalar& x, const btScalar& y, const btScalar& z,const btScalar& w) 
		: btVector3(x,y,z)
	{
		
		m_floats[3] = w;
	}

	inline __attribute__((always_inline)) btVector4(vec_float4 vec)
	{
		mVec = vec;
	}

	inline __attribute__((always_inline)) btVector4 absolute4() const 
	{
		return btVector4(vec_abs(mVec));
	}

	inline __attribute__((always_inline)) int maxAxis4() const
	{
		vec_float4 twins = vec_max(mVec, vec_sld(mVec, mVec, 4));	
		twins = vec_max(twins, vec_sld(twins, twins, 8));	
		vec_int4 ret = vec_sums((vec_int4)btZeroVec(), vec_and(btElementMask, (vec_int4)vec_cmpeq(twins, mVec)));
		int out = vec_extract(ret, 3);
		return (out > 2) ? ((out & 4) ? 2 : 3) : ((out & 1) ? 0 : 1);
	}


	inline __attribute__((always_inline)) int minAxis4() const
	{
		
		vec_float4 twins = vec_min(mVec, vec_sld(mVec, mVec, 4));	
		twins = vec_min(twins, vec_sld(twins, twins, 8));	
		vec_int4 ret = vec_sums((vec_int4)btZeroVec(), vec_and(btElementMask, (vec_int4)vec_cmpeq(twins, mVec)));
		int out = vec_extract(ret, 3);
		return (out > 2) ? ((out & 4) ? 2 : 3) : ((out & 1) ? 0 : 1);
	}


	inline __attribute__((always_inline)) int closestAxis4() const 
	{
		return absolute4().maxAxis4();
	}

	




 
		inline __attribute__((always_inline)) void	setValue(const btScalar& x, const btScalar& y, const btScalar& z,const btScalar& w)
		{
			m_floats[0]=x;
			m_floats[1]=y;
			m_floats[2]=z;
			m_floats[3]=w;
		}
};




inline __attribute__((always_inline)) void	btSwapVector3Endian(const btVector3& sourceVec, btVector3& destVec)
{
	
	vec_uint4 src = (vec_uint4)sourceVec.mVec;	

	vec_ushort8  dest = (vec_ushort8)vec_rl(src, vec_splats(16u));			
	dest = vec_rl(dest, vec_ushort8(8));	
	destVec = (vec_float4)dest;
}


inline __attribute__((always_inline)) void	btUnSwapVector3Endian(btVector3& vector)
{
	
	vec_uint4 src = (vec_uint4)vector.mVec;	

	vec_ushort8  dest = (vec_ushort8)vec_rl(src, vec_splats(16u));			
	dest = vec_rl(dest, vec_ushort8(8));	
	vector = (vec_float4)dest;
}

# 705 "..\\..\\src\\LinearMath/btVector3.h"
template <class T>
inline __attribute__((always_inline)) void btPlaneSpace1 (const T& n, T& p, T& q)
{
  if (btFabs(n[2]) > btScalar(0.7071067811865475244008443621048490)) {
    
    btScalar a = n[1]*n[1] + n[2]*n[2];
    btScalar k = ((btScalar)(btScalar(1.0)/btSqrt(btScalar(a))));
    p[0] = 0;
	p[1] = -n[2]*k;
	p[2] = n[1]*k;
    
    q[0] = a*k;
	q[1] = -n[0]*p[2];
	q[2] = n[0]*p[1];
  }
  else {
    
    btScalar a = n[0]*n[0] + n[1]*n[1];
    btScalar k = ((btScalar)(btScalar(1.0)/btSqrt(btScalar(a))));
    p[0] = -n[1]*k;
	p[1] = n[0]*k;
	p[2] = 0;
    
    q[0] = -n[2]*p[1];
	q[1] = n[2]*p[0];
	q[2] = a*k;
  }
}


struct	btVector3FloatData
{
	union
	{
		vec_float4 mVec;
		float m_floats[4];
	};
};

struct	btVector3DoubleData
{
	double	m_floats[4];

};

inline __attribute__((always_inline))	void	btVector3::serializeFloat(struct	btVector3FloatData& dataOut) const
{
	
	dataOut.mVec = mVec;
}

inline __attribute__((always_inline)) void	btVector3::deSerializeFloat(const struct	btVector3FloatData& dataIn)
{
	mVec = dataIn.mVec;
}

inline __attribute__((always_inline))	void	btVector3::serializeDouble(struct	btVector3DoubleData& dataOut) const
{
	((! "implemented") ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btVector3.h" ":" "763" " " "!\"implemented\"", 0));
}

inline __attribute__((always_inline)) void	btVector3::deSerializeDouble(const struct	btVector3DoubleData& dataIn)
{
	((! "implemented") ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btVector3.h" ":" "768" " " "!\"implemented\"", 0));
}

inline __attribute__((always_inline))	void	btVector3::serialize(struct	btVector3FloatData& dataOut) const
{
	
	dataOut.mVec = mVec;
}

inline __attribute__((always_inline)) void	btVector3::deSerialize(const struct	btVector3FloatData& dataIn)
{
	mVec = dataIn.mVec;
}
# 1483 "..\\..\\src\\LinearMath/btVector3.h"



static inline __attribute__((always_inline)) float vec_dot3s( vec_float4 vec0, vec_float4 vec1 )
{
	vec_float4 tmp = vec_madd(vec0, vec1, btZeroVec());
	return vec_extract(vec_add(vec_add(tmp, vec_sld(tmp, tmp, 4)), vec_sld(tmp, tmp, 8)), 0);
}

static inline __attribute__((always_inline)) vec_float4 vec_dot3( vec_float4 vec0, vec_float4 vec1 )
{
	vec_float4 tmp = vec_madd(vec0, vec1, btZeroVec());
	return vec_splat(vec_add(vec_add(tmp, vec_sld(tmp, tmp, 4)), vec_sld(tmp, tmp, 8)), 0);
}

static inline __attribute__((always_inline)) vec_float4 vec_dot3mx(const vec_float4* mx, vec_float4 vec )
{
	
	vec_float4 vec0 = vec_madd(mx[0], vec, btZeroVec());
	vec_float4 vec1 = vec_madd(mx[1], vec, btZeroVec());
	vec_float4 vec2 = vec_madd(mx[2], vec, btZeroVec());

	
	vec_float4 tmp = vec_vmrghw(vec0, vec2);	
	vec_float4 mt0 = vec_vmrghw(tmp, vec1); 
	vec_float4 mt1 = vec_vmrglw(tmp, vec_sld(vec1, vec1, 12));	
	vec_float4 mt2 = vec_vmrghw(vec_vmrglw(vec0, vec2), vec_sld(vec1, vec1,	8)); 

	
	return vec_add(mt2, vec_add(mt0, mt1));
}


# 1522 "..\\..\\src\\LinearMath/btVector3.h"

# 21 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btBroadphaseProxy.h"
# 1 "..\\..\\src\\LinearMath/btAlignedAllocator.h"













 










void*	btAlignedAllocInternal	(size_t size, int alignment,int line,const char* filename);
void	btAlignedFreeInternal	(void* ptr);









typedef int	size_type;

typedef void *(btAlignedAllocFunc)(size_t size, int alignment, int line, const char* file);
typedef void (btAlignedFreeFunc)(void *memblock);
typedef void *(btAllocFunc)(size_t size, int line, const char* file);
typedef void (btFreeFunc)(void *memblock);


void btAlignedAllocSetCustom(btAllocFunc *allocFunc, btFreeFunc *freeFunc);

void btAlignedAllocSetCustomAligned(btAlignedAllocFunc *allocFunc, btAlignedFreeFunc *freeFunc);




template < typename T , unsigned Alignment >
class btAlignedAllocator {
	
	typedef btAlignedAllocator< T , Alignment > self_type;
	
public:

	
	btAlignedAllocator() {}
	

 

	template < typename Other >
	btAlignedAllocator( const btAlignedAllocator< Other , Alignment > & ) {}

	typedef const T*         const_pointer;
	typedef const T&         const_reference;
	typedef T*               pointer;
	typedef T&               reference;
	typedef T                value_type;

	pointer       address   ( reference        ref ) const                           { return &ref; }
	const_pointer address   ( const_reference  ref ) const                           { return &ref; }
	pointer       allocate  ( size_type        n   , const_pointer *      hint = 0 ) {
		(void)hint;
		return reinterpret_cast< pointer >(btAlignedAllocInternal(sizeof(value_type) * n,Alignment,77,"..\\..\\src\\LinearMath/btAlignedAllocator.h"));
	}
	void          construct ( pointer          ptr , const value_type &   value    ) { new (ptr) value_type( value ); }
	void          deallocate( pointer          ptr ) {
		btAlignedFreeInternal(reinterpret_cast< void * >( ptr ));
	}
	void          destroy   ( pointer          ptr )                                 { ptr->~value_type(); }
	

	template < typename O > struct rebind {
		typedef btAlignedAllocator< O , Alignment > other;
	};
	template < typename O >
	self_type & operator=( const btAlignedAllocator< O , Alignment > & ) { return *this; }

	friend bool operator==( const self_type & , const self_type & ) { return true; }
};





# 22 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btBroadphaseProxy.h"






enum BroadphaseNativeTypes
{
	
	BOX_SHAPE_PROXYTYPE,
	TRIANGLE_SHAPE_PROXYTYPE,
	TETRAHEDRAL_SHAPE_PROXYTYPE,
	CONVEX_TRIANGLEMESH_SHAPE_PROXYTYPE,
	CONVEX_HULL_SHAPE_PROXYTYPE,
	CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE,
	CUSTOM_POLYHEDRAL_SHAPE_TYPE,

IMPLICIT_CONVEX_SHAPES_START_HERE,
	SPHERE_SHAPE_PROXYTYPE,
	MULTI_SPHERE_SHAPE_PROXYTYPE,
	CAPSULE_SHAPE_PROXYTYPE,
	CONE_SHAPE_PROXYTYPE,
	CONVEX_SHAPE_PROXYTYPE,
	CYLINDER_SHAPE_PROXYTYPE,
	UNIFORM_SCALING_SHAPE_PROXYTYPE,
	MINKOWSKI_SUM_SHAPE_PROXYTYPE,
	MINKOWSKI_DIFFERENCE_SHAPE_PROXYTYPE,
	BOX_2D_SHAPE_PROXYTYPE,
	CONVEX_2D_SHAPE_PROXYTYPE,
	CUSTOM_CONVEX_SHAPE_TYPE,

CONCAVE_SHAPES_START_HERE,
	
	TRIANGLE_MESH_SHAPE_PROXYTYPE,
	SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE,
	
	FAST_CONCAVE_MESH_PROXYTYPE,
	
	TERRAIN_SHAPE_PROXYTYPE,

	GIMPACT_SHAPE_PROXYTYPE,

    MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE,
	
	EMPTY_SHAPE_PROXYTYPE,
	STATIC_PLANE_PROXYTYPE,
	CUSTOM_CONCAVE_SHAPE_TYPE,
CONCAVE_SHAPES_END_HERE,

	COMPOUND_SHAPE_PROXYTYPE,

	SOFTBODY_SHAPE_PROXYTYPE,
	HFFLUID_SHAPE_PROXYTYPE,
	HFFLUID_BUOYANT_CONVEX_SHAPE_PROXYTYPE,
	INVALID_SHAPE_PROXYTYPE,

	MAX_BROADPHASE_COLLISION_TYPES
	
};




struct __attribute__ ((aligned (16))) btBroadphaseProxy
{

inline __attribute__((always_inline)) void* operator new(size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,88,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btBroadphaseProxy.h"); } inline __attribute__((always_inline)) void operator delete(void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new(size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete(void*, void*) { } inline __attribute__((always_inline)) void* operator new[](size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,88,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btBroadphaseProxy.h"); } inline __attribute__((always_inline)) void operator delete[](void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new[](size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete[](void*, void*) { };
	
	
	enum CollisionFilterGroups
	{
	        DefaultFilter = 1,
	        StaticFilter = 2,
	        KinematicFilter = 4,
	        DebrisFilter = 8,
			SensorTrigger = 16,
			CharacterFilter = 32,
	        AllFilter = -1 
	};

	
	void*	m_clientObject;
	short int m_collisionFilterGroup;
	short int m_collisionFilterMask;
	unsigned int m_interactionLayer;
	void*	m_multiSapParentProxy;		
	int			m_uniqueId;

	btVector3	m_aabbMin;
	btVector3	m_aabbMax;

	inline __attribute__((always_inline)) int getUid() const
	{
		return m_uniqueId;
	}

	
	btBroadphaseProxy() :m_clientObject(0),m_multiSapParentProxy(0)
	{
	}

	btBroadphaseProxy(const btVector3& aabbMin,const btVector3& aabbMax,void* userPtr,short int collisionFilterGroup, short int collisionFilterMask,void* multiSapParentProxy=0, unsigned int interactionLayer = 0xffffffff)
		:m_clientObject(userPtr),
		m_collisionFilterGroup(collisionFilterGroup),
		m_collisionFilterMask(collisionFilterMask),
		m_interactionLayer(interactionLayer),
		m_aabbMin(aabbMin),
		m_aabbMax(aabbMax)
	{
		m_multiSapParentProxy = multiSapParentProxy;
	}

	

	static inline __attribute__((always_inline)) bool isPolyhedral(int proxyType)
	{
		return (proxyType  < IMPLICIT_CONVEX_SHAPES_START_HERE);
	}

	static inline __attribute__((always_inline)) bool	isConvex(int proxyType)
	{
		return (proxyType < CONCAVE_SHAPES_START_HERE);
	}

	static inline __attribute__((always_inline)) bool	isConcave(int proxyType)
	{
		return ((proxyType > CONCAVE_SHAPES_START_HERE) &&
			(proxyType < CONCAVE_SHAPES_END_HERE));
	}
	static inline __attribute__((always_inline)) bool	isCompound(int proxyType)
	{
		return (proxyType == COMPOUND_SHAPE_PROXYTYPE);
	}

	static inline __attribute__((always_inline)) bool	isSoftBody(int proxyType)
	{
		return (proxyType == SOFTBODY_SHAPE_PROXYTYPE);
	}

	static inline __attribute__((always_inline)) bool isInfinite(int proxyType)
	{
		return (proxyType == STATIC_PLANE_PROXYTYPE);
	}

	static inline __attribute__((always_inline)) bool isConvex2d(int proxyType)
	{
		return (proxyType == BOX_2D_SHAPE_PROXYTYPE) ||	(proxyType == CONVEX_2D_SHAPE_PROXYTYPE);
	}

	
}
;

class btCollisionAlgorithm;

struct btBroadphaseProxy;





struct __attribute__ ((aligned (16))) btBroadphasePair
{
	btBroadphasePair ()
		:
	m_pProxy0(0),
		m_pProxy1(0),
		m_algorithm(0),
		m_internalInfo1(0)
	{
	}

inline __attribute__((always_inline)) void* operator new(size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,194,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btBroadphaseProxy.h"); } inline __attribute__((always_inline)) void operator delete(void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new(size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete(void*, void*) { } inline __attribute__((always_inline)) void* operator new[](size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,194,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btBroadphaseProxy.h"); } inline __attribute__((always_inline)) void operator delete[](void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new[](size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete[](void*, void*) { };

	btBroadphasePair(const btBroadphasePair& other)
		:		m_pProxy0(other.m_pProxy0),
				m_pProxy1(other.m_pProxy1),
				m_algorithm(other.m_algorithm),
				m_internalInfo1(other.m_internalInfo1)
	{
	}
	btBroadphasePair(btBroadphaseProxy& proxy0,btBroadphaseProxy& proxy1)
	{

		
		if (proxy0.m_uniqueId < proxy1.m_uniqueId)
        { 
            m_pProxy0 = &proxy0; 
            m_pProxy1 = &proxy1; 
        }
        else 
        { 
			m_pProxy0 = &proxy1; 
            m_pProxy1 = &proxy0; 
        }

		m_algorithm = 0;
		m_internalInfo1 = 0;

	}
	
	btBroadphaseProxy* m_pProxy0;
	btBroadphaseProxy* m_pProxy1;
	
	mutable btCollisionAlgorithm* m_algorithm;
	union { void* m_internalInfo1; int m_internalTmpValue;};

};








 



class btBroadphasePairSortPredicate
{
	public:

		bool operator() ( const btBroadphasePair& a, const btBroadphasePair& b )
		{
			const int uidA0 = a.m_pProxy0 ? a.m_pProxy0->m_uniqueId : -1;
			const int uidB0 = b.m_pProxy0 ? b.m_pProxy0->m_uniqueId : -1;
			const int uidA1 = a.m_pProxy1 ? a.m_pProxy1->m_uniqueId : -1;
			const int uidB1 = b.m_pProxy1 ? b.m_pProxy1->m_uniqueId : -1;

			 return uidA0 > uidB0 || 
				(a.m_pProxy0 == b.m_pProxy0 && uidA1 > uidB1) ||
				(a.m_pProxy0 == b.m_pProxy0 && a.m_pProxy1 == b.m_pProxy1 && a.m_algorithm > b.m_algorithm); 
		}
};


inline __attribute__((always_inline)) bool operator==(const btBroadphasePair& a, const btBroadphasePair& b) 
{
	 return (a.m_pProxy0 == b.m_pProxy0) && (a.m_pProxy1 == b.m_pProxy1);
}




# 24 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btBroadphaseInterface.h"

class btOverlappingPairCache;



struct	btBroadphaseAabbCallback
{
	virtual ~btBroadphaseAabbCallback() {}
	virtual bool	process(const btBroadphaseProxy* proxy) = 0;
};


struct	btBroadphaseRayCallback : public btBroadphaseAabbCallback
{
	
	btVector3		m_rayDirectionInverse;

	vec_bint4 m_signs;
	vec_float4		m_lambda_max;




	
	virtual ~btBroadphaseRayCallback() {}
};






class btBroadphaseInterface
{
public:
	virtual ~btBroadphaseInterface() {}

	virtual btBroadphaseProxy*	createProxy(  const btVector3& aabbMin,  const btVector3& aabbMax,int shapeType,void* userPtr, short int collisionFilterGroup,short int collisionFilterMask, unsigned int interactionLayer,btDispatcher* dispatcher,void* multiSapProxy) =0;
	virtual void	destroyProxy(btBroadphaseProxy* proxy,btDispatcher* dispatcher)=0;
	virtual void	setAabb(btBroadphaseProxy* proxy,const btVector3& aabbMin,const btVector3& aabbMax, btDispatcher* dispatcher)=0;
	virtual void	getAabb(btBroadphaseProxy* proxy,btVector3& aabbMin, btVector3& aabbMax ) const =0;

	virtual void	rayTest(const btVector3& rayFrom,const btVector3& rayTo, btBroadphaseRayCallback& rayCallback, const btVector3& aabbMin, const btVector3& aabbMax) = 0;
	virtual void	rayTest(const btVector3& rayFrom,const btVector3& rayTo, btBroadphaseRayCallback& rayCallback) = 0;

	virtual void	aabbTest(const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback) = 0;

	
	virtual void	calculateOverlappingPairs(btDispatcher* dispatcher)=0;

	virtual	btOverlappingPairCache*	getOverlappingPairCache()=0;
	virtual	const btOverlappingPairCache*	getOverlappingPairCache() const =0;

	
	
	virtual void getBroadphaseAabb(btVector3& aabbMin,btVector3& aabbMax) const =0;

	
	virtual void resetPool(btDispatcher* dispatcher) { (void) dispatcher; };

	virtual void	printStats() = 0;

};

# 19 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.h"
# 1 "..\\..\\src\\LinearMath/btAlignedObjectArray.h"













 


















# 1 "C:/PS3SDK\\target\\ppu\\include/string.h"




 
 









namespace std {

		 




		 

		 
extern "C" {
int memcmp(const void *, const void *, size_t);
void *memcpy(void *, const void *, size_t);
void *memcpy16(void *, const void *, size_t);
void *memset(void *, int, size_t);
void *memset_cs(void *, int, size_t);
char *strcat(char *, const char *);
int strcmp(const char *, const char *);
char *strcpy(char *, const char *);
size_t strlen(const char *);

void *memmove(void *, const void *, size_t);
int strcoll(const char *, const char *);
size_t strcspn(const char *, const char *);
char *strerror(int);
char *strncat(char *, const char *, size_t);
int strncmp(const char *, const char *, size_t);
char *strncpy(char *, const char *, size_t);
char *strncpy_cs(char *, const char *, size_t);
size_t strspn(const char *, const char *);
char *strtok(char *, const char *);
size_t strxfrm(char *, const char *, size_t);
char *strdup(const char *);
int strcasecmp(const char *, const char *);


int strncasecmp(const char *, const char *, size_t);

char *strtok_r(char *, const char *,char **);

}


		


extern "C" {
const char *strchr(const char *, int);
const char *strpbrk(const char *, const char *);
const char *strrchr(const char *, int);
const char *strstr(const char *, const char *);
}

inline char *strchr(char *_Str, int _Ch) {
  
  return (const_cast<char *>(::std:: strchr((const char *)_Str, _Ch)));
}

inline char *strpbrk(char *_Str1, const char *_Str2) {
  
  return (const_cast<char *>(::std:: strpbrk((const char *)_Str1, _Str2)));
}

inline char *strrchr(char *_Str, int _Ch) {
  
  return (const_cast<char *>(::std:: strrchr((const char *)_Str, _Ch)));
}

inline char *strstr(char *_Str1, const char *_Str2) {
  
  return (const_cast<char *>(::std:: strstr((const char *)_Str1, _Str2)));
}

# 99 "C:/PS3SDK\\target\\ppu\\include/string.h"





extern "C" {
const void *memchr(const void *, int, size_t);
}

inline void *memchr(void *_Str, int _Ch, size_t _Num) {
  
  return (const_cast<void *>(::std:: memchr((const void *)_Str, _Ch, _Num)));
}








}





using ::std:: memchr; using ::std:: memcmp;
using ::std:: memcpy; using ::std:: memcpy16; using ::std:: memmove;
using ::std:: memset; using ::std:: memset_cs;
using ::std:: strcat; using ::std:: strchr; using ::std:: strcmp;
using ::std:: strcoll; using ::std:: strcpy; using ::std:: strcspn;
using ::std:: strerror; using ::std:: strlen; using ::std:: strncat;
using ::std:: strncmp; using ::std:: strpbrk;
using ::std:: strncpy; using ::std:: strncpy_cs;
using ::std:: strrchr; using ::std:: strspn; using ::std:: strstr;
using ::std:: strtok; using ::std:: strxfrm; using ::std:: strdup;
using ::std:: strcasecmp; using ::std:: strncasecmp; using ::std:: strtok_r;





 
# 37 "..\\..\\src\\LinearMath/btAlignedObjectArray.h"


# 1 "C:/PS3SDK\\target\\ppu\\include/new"




 

# 1 "C:/PS3SDK\\target\\ppu\\include/exception"

# 1 "C:/PS3SDK\\target\\ppu\\include/xstddef"







# 1 "C:/PS3SDK\\target\\ppu\\include/cstddef"




 





# 1 "C:/PS3SDK\\target\\ppu\\include/stddef.h"




 
 








namespace std {

		 








# 35 "C:/PS3SDK\\target\\ppu\\include/stddef.h"



		 





typedef _Ptrdifft ptrdiff_t;







}


# 62 "C:/PS3SDK\\target\\ppu\\include/stddef.h"




 
# 14 "C:/PS3SDK\\target\\ppu\\include/cstddef"


# 27 "C:/PS3SDK\\target\\ppu\\include/cstddef"




 
# 9 "C:/PS3SDK\\target\\ppu\\include/xstddef"
namespace std {

		

# 31 "C:/PS3SDK\\target\\ppu\\include/xstddef"

# 38 "C:/PS3SDK\\target\\ppu\\include/xstddef"








		
 






 


		


# 95 "C:/PS3SDK\\target\\ppu\\include/xstddef"

		















		
enum _Uninitialized
	{	
	_Noinit};
}





 
# 5 "C:/PS3SDK\\target\\ppu\\include/exception"





namespace std {


		
class exception;
typedef void (*_Prhand)(const exception&);
extern _Prhand _Raise_handler;
void _Throw(const exception&);

# 113 "C:/PS3SDK\\target\\ppu\\include/exception"
		
class exception
	{	
public:
	static _Prhand _Set_raise_handler(_Prhand _Pnew);

	exception()  
		{
		}

	virtual ~exception();





	virtual const char *what() const ;






	void _Raise() const;

protected:
	virtual void _Doraise() const;


	};

		
class bad_exception
	: public exception
	{	
public:
	bad_exception() 
		{	
		}

	virtual ~bad_exception() 
		{	
		}





protected:
	virtual void _Doraise() const
		{	
		::std:: _Throw(*this);
		}


	};


		
typedef void (*terminate_handler)();
typedef void (*unexpected_handler)();

		
terminate_handler set_terminate(terminate_handler) ;
unexpected_handler set_unexpected(unexpected_handler) ;
bool uncaught_exception();
void terminate() __attribute__((__noreturn__));
void unexpected() __attribute__((__noreturn__));





}  



# 200 "C:/PS3SDK\\target\\ppu\\include/exception"







 
# 10 "C:/PS3SDK\\target\\ppu\\include/new"





namespace std {


		
class bad_alloc
	: public ::std:: exception
	{	
public:

# 31 "C:/PS3SDK\\target\\ppu\\include/new"
	bad_alloc() 
		{	
		}

	virtual const char *what() const 
		{	
		return ("bad allocation");
		}


	virtual ~bad_alloc() 
		{}	





protected:
	virtual void _Doraise() const
		{	
		::std:: _Throw(*this);
		}


	};





}  









namespace std {
		
typedef void (*new_handler)();	

struct nothrow_t
	{	
	};

extern const nothrow_t nothrow;	

		
new_handler set_new_handler(new_handler)
	;	

extern new_handler _New_hand;	
}

		


void *operator new(::std:: size_t)
	;	

void *operator new(::std:: size_t, const ::std:: nothrow_t&)
	;	

inline void *operator new(::std:: size_t, void *_Where) 
	{	
	return (_Where);
	}


void *operator new[](::std:: size_t)
	;	

void *operator new[](::std:: size_t, const ::std:: nothrow_t&)
	;	

inline void *operator new[](::std:: size_t, void *_Where) 
	{	
	return (_Where);
	}



void operator delete  (void *) ;	
void operator delete[](void *) ;	



void operator delete  (void *, const ::std:: nothrow_t&) ;	
void operator delete[](void *, const ::std:: nothrow_t&) ;	
void operator delete  (void *, void *) ;	
void operator delete[](void *, void *) ;	



void *operator new(::std:: size_t, ::std:: size_t)
	;	

void *operator new(::std:: size_t, ::std:: size_t, const ::std:: nothrow_t&)
	;	

inline void *operator new(::std:: size_t, ::std:: size_t, void *_Where) 
	{	
	return (_Where);
	}


void *operator new[](::std:: size_t, ::std:: size_t)
	;	

void *operator new[](::std:: size_t, ::std:: size_t, const ::std:: nothrow_t&)
	;	

inline void *operator new[](::std:: size_t, ::std:: size_t, void *_Where) 
	{	
	return (_Where);
	}

void operator delete  (void *, ::std:: size_t) ;	
void operator delete[](void *, ::std:: size_t) ;	
void operator delete  (void *, ::std:: size_t, const ::std:: nothrow_t&) ;
void operator delete[](void *, ::std:: size_t, const ::std:: nothrow_t&) ;	
void operator delete  (void *, ::std:: size_t, void *) ;
void operator delete[](void *, ::std:: size_t, void *) ;








 
# 41 "..\\..\\src\\LinearMath/btAlignedObjectArray.h"





template <typename T> 

class btAlignedObjectArray
{
	btAlignedAllocator<T , 16>	m_allocator;

	int					m_size;
	int					m_capacity;
	T*					m_data;
	
	bool				m_ownsMemory;

	protected:
		inline __attribute__((always_inline))	int	allocSize(int size)
		{
			return (size ? size*2 : 1);
		}
		inline __attribute__((always_inline))	void	copy(int start,int end, T* dest) const
		{
			int i;
			for (i=start;i<end;++i)

				new (&dest[i]) T(m_data[i]);



		}

		inline __attribute__((always_inline))	void	init()
		{
			
			m_ownsMemory = true;
			m_data = 0;
			m_size = 0;
			m_capacity = 0;
		}
		inline __attribute__((always_inline))	void	destroy(int first,int last)
		{
			int i;
			for (i=first; i<last;i++)
			{
				m_data[i].~T();
			}
		}

		inline __attribute__((always_inline))	void* allocate(int size)
		{
			if (size)
				return m_allocator.allocate(size);
			return 0;
		}

		inline __attribute__((always_inline))	void	deallocate()
		{
			if(m_data)	{
				
				if (m_ownsMemory)
				{
					m_allocator.deallocate(m_data);
				}
				m_data = 0;
			}
		}

	


	public:
		
		btAlignedObjectArray()
		{
			init();
		}

		~btAlignedObjectArray()
		{
			clear();
		}

		
		btAlignedObjectArray(const btAlignedObjectArray& otherArray)
		{
			init();

			int otherSize = otherArray.size();
			resize (otherSize);
			otherArray.copy(0, otherSize, m_data);
		}

		
		
		
		inline __attribute__((always_inline))	int size() const
		{	
			return m_size;
		}
		
		inline __attribute__((always_inline)) const T& at(int n) const
		{
			return m_data[n];
		}

		inline __attribute__((always_inline)) T& at(int n)
		{
			return m_data[n];
		}

		inline __attribute__((always_inline)) const T& operator[](int n) const
		{
			return m_data[n];
		}

		inline __attribute__((always_inline)) T& operator[](int n)
		{
			return m_data[n];
		}
		

		
		inline __attribute__((always_inline))	void	clear()
		{
			destroy(0,size());
			
			deallocate();
			
			init();
		}

		inline __attribute__((always_inline))	void	pop_back()
		{
			m_size--;
			m_data[m_size].~T();
		}

		
		
		inline __attribute__((always_inline))	void	resize(int newsize, const T& fillData=T())
		{
			int curSize = size();

			if (newsize < curSize)
			{
				for(int i = newsize; i < curSize; i++)
				{
					m_data[i].~T();
				}
			} else
			{
				if (newsize > size())
				{
					reserve(newsize);
				}

				for (int i=curSize;i<newsize;i++)
				{
					new ( &m_data[i]) T(fillData);
				}


			}

			m_size = newsize;
		}
	
		inline __attribute__((always_inline))	T&  expandNonInitializing( )
		{	
			int sz = size();
			if( sz == capacity() )
			{
				reserve( allocSize(size()) );
			}
			m_size++;

			return m_data[sz];		
		}


		inline __attribute__((always_inline))	T&  expand( const T& fillValue=T())
		{	
			int sz = size();
			if( sz == capacity() )
			{
				reserve( allocSize(size()) );
			}
			m_size++;

			new (&m_data[sz]) T(fillValue); 


			return m_data[sz];		
		}


		inline __attribute__((always_inline))	void push_back(const T& _Val)
		{	
			int sz = size();
			if( sz == capacity() )
			{
				reserve( allocSize(size()) );
			}
			

			new ( &m_data[m_size] ) T(_Val);




			m_size++;
		}

		inline bool insertSorted(const T &a, unsigned int* index = 0)
		{
			int p = 0;
			int r = size() - 1;
			int i;
			for(;r >= p;)
			{
				i = (p + r) / 2;
				if(m_data[i] > a)
				{
					r = i - 1;
				}
				else
				{
					if(m_data[i] == a)
					{
						if (index) *index = i;
						return false;
					}
					p = i + 1;
				}
			}

			int sz = size();
			if( sz == capacity() )
			{
				reserve( allocSize(size()) );
			}

			
			if (m_size - p > 0)
				memmove(&m_data[p + 1], &m_data[p], (m_size - p) * sizeof(T));
			
			
			new ((void*)(&m_data[p])) T;

			m_data[p] = a;
			m_size++;
			if (index) *index = p;
			return true;
		}


		
		inline __attribute__((always_inline))	int capacity() const
		{	
			return m_capacity;
		}
		
		inline __attribute__((always_inline))	void reserve(int _Count)
		{	
			if (capacity() < _Count)
			{	
				T*	s = (T*)allocate(_Count);

				copy(0, size(), s);

				destroy(0,size());

				deallocate();
				
				
				m_ownsMemory = true;

				m_data = s;
				
				m_capacity = _Count;

			}
		}


		class less
		{
			public:

				bool operator() ( const T& a, const T& b )
				{
					return ( a < b );
				}
		};
	
		template <typename L>
		void quickSortInternal(L CompareFunc,int lo, int hi)
		{
		
		
			int i=lo, j=hi;
			T x=m_data[(lo+hi)/2];

			
			do
			{    
				while (CompareFunc(m_data[i],x)) 
					i++; 
				while (CompareFunc(x,m_data[j])) 
					j--;
				if (i<=j)
				{
					swap(i,j);
					i++; j--;
				}
			} while (i<=j);

			
			if (lo<j) 
				quickSortInternal( CompareFunc, lo, j);
			if (i<hi) 
				quickSortInternal( CompareFunc, i, hi);
		}


		template <typename L>
		void quickSort(L CompareFunc)
		{
			
			if (size()>1)
			{
				quickSortInternal(CompareFunc,0,size()-1);
			}
		}


		
		template <typename L>
		void downHeap(T *pArr, int k, int n,L CompareFunc)
		{
			 
			 
			
			T temp = pArr[k - 1];
			 
			while (k <= n/2) 
			{
				int child = 2*k;
				
				if ((child < n) && CompareFunc(pArr[child - 1] , pArr[child]))
				{
					child++;
				}
				 
				if (CompareFunc(temp , pArr[child - 1]))
				{
					 
					pArr[k - 1] = pArr[child - 1];
					k = child;
				}
				else
				{
					break;
				}
			}
			pArr[k - 1] = temp;
		}  

		void	swap(int index0,int index1)
		{
# 419 "..\\..\\src\\LinearMath/btAlignedObjectArray.h"
			T temp = m_data[index0];
			m_data[index0] = m_data[index1];
			m_data[index1] = temp;


		}

	template <typename L>
	void heapSort(L CompareFunc)
	{
		 
		int k;
		int n = m_size;
		for (k = n/2; k > 0; k--) 
		{
			downHeap(m_data, k, n, CompareFunc);
		}

		 
		while ( n>=1 ) 
		{
			swap(0,n-1);  


			n = n - 1;
			 
			downHeap(m_data, 1, n, CompareFunc);
		} 
	}

	
	int	findBinarySearch(const T& key) const
	{
		int first = 0;
		int last = size();

		
		while (first <= last) {
			int mid = (first + last) / 2;  
			if (key > m_data[mid]) 
				first = mid + 1;  
			else if (key < m_data[mid]) 
				last = mid - 1; 
			else
				return mid;     
		}
		return size();    
	}

	int	findLinearSearch(const T& key) const
	{
		int index=size();
		int i;

		for (i=0;i<size();i++)
		{
			if (m_data[i] == key)
			{
				index = i;
				break;
			}
		}
		return index;
	}

	void removeElementOrdered(int n)
	{
		((n < size()) ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btAlignedObjectArray.h" ":" "486" " " "n < size()", 0));
		m_data[n].~T();
		memmove(&m_data[n], &m_data[n + 1], (m_size - n - 1) * sizeof(T));
		m_size--;
	}

	void	removeSorted(const T& key)
	{

		int findIndex = findBinarySearch(key);
		if (findIndex<size())
		{
			removeElementOrdered(findIndex);
		}
	}

	void	remove(const T& key)
	{

		int findIndex = findLinearSearch(key);
		if (findIndex<size())
		{
			swap( findIndex,size()-1);
			pop_back();
		}
	}

	
	void initializeFromBuffer(void *buffer, int size, int capacity)
	{
		clear();
		m_ownsMemory = false;
		m_data = (T*)buffer;
		m_size = size;
		m_capacity = capacity;
	}

	void copyFromArray(const btAlignedObjectArray& otherArray)
	{
		int otherSize = otherArray.size();
		resize (otherSize);
		otherArray.copy(0, otherSize, m_data);
	}

};

# 20 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.h"
# 1 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btOverlappingPairCache.h"













 





# 1 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btOverlappingPairCallback.h"














 




class btDispatcher;
struct  btBroadphasePair;


class btOverlappingPairCallback
{
public:
	virtual ~btOverlappingPairCallback()
	{

	}
	
	virtual btBroadphasePair*	addOverlappingPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1) = 0;

	virtual void*	removeOverlappingPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1,btDispatcher* dispatcher) = 0;

	virtual void	removeOverlappingPairsContainingProxy(btBroadphaseProxy* proxy0,btDispatcher* dispatcher) = 0;

};

# 23 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btOverlappingPairCache.h"


class btDispatcher;

typedef btAlignedObjectArray<btBroadphasePair>	btBroadphasePairArray;

struct	btOverlapCallback
{
	virtual ~btOverlapCallback()
	{}
	
	virtual bool	processOverlap(btBroadphasePair& pair) = 0;

};

struct btOverlapFilterCallback
{
	virtual ~btOverlapFilterCallback()
	{}
	
	virtual bool	needBroadphaseCollision(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1) const = 0;
};







extern int gRemovePairs;
extern int gAddedPairs;
extern int gFindPairs;

const int BT_NULL_PAIR=0xffffffff;
class btInteractionBlock;

class btInteractionBlockHashMap
{
protected:
	typedef btAlignedObjectArray<const btInteractionBlock*> btInteractionBlockArray;

	btInteractionBlockArray*	m_hashMap[256];
public:
	btInteractionBlockHashMap()
	{
		for(int n = 0; n < 256; n++)
			m_hashMap[n] = 0;
	}

	void insert(const btInteractionBlock* ib);
	void remove(const btInteractionBlock* ib);
	bool exists(const btInteractionBlock& other) const;
};




class btOverlappingPairCache : public btOverlappingPairCallback
{
protected:
	unsigned int m_interactionMatrix[32];
	btInteractionBlockHashMap m_interactionBlockHashMap;

public:
	btOverlappingPairCache();
	virtual ~btOverlappingPairCache() {} 
	void setCurrent();
	void insertInteractionBlock(const btInteractionBlock* block);
	void removeInteractionBlock(const btInteractionBlock* block);
	bool checkInteractionBlock(const btInteractionBlock& block) const
	{
		return m_interactionBlockHashMap.exists(block);
	}
	bool checkInteractionBlocks(const btBroadphaseProxy* proxy0, const btBroadphaseProxy* proxy1) const;

	void enableInteractionLayer(unsigned int mask1, unsigned int mask2);
	void disableInteractionLayer(unsigned int mask1, unsigned int mask2);
	inline __attribute__((always_inline)) bool hasInteractionEnabled(unsigned int mask1, unsigned int mask2) const
	{
		int i;
		for(i = 0; i < 32; i++)
		{
			if(((mask1 >> i) & 1) * (m_interactionMatrix[i] & mask2))
				return true;
		}
		return false;
	}

	virtual btBroadphasePair*	getOverlappingPairArrayPtr() = 0;
	
	virtual const btBroadphasePair*	getOverlappingPairArrayPtr() const = 0;

	virtual btBroadphasePairArray&	getOverlappingPairArray() = 0;

	virtual	void	cleanOverlappingPair(btBroadphasePair& pair,btDispatcher* dispatcher) = 0;

	virtual int getNumOverlappingPairs() const = 0;

	virtual void	cleanProxyFromPairs(btBroadphaseProxy* proxy,btDispatcher* dispatcher) = 0;

	virtual	void setOverlapFilterCallback(btOverlapFilterCallback* callback) = 0;

	virtual void	processAllOverlappingPairs(btOverlapCallback*,btDispatcher* dispatcher) = 0;

	virtual btBroadphasePair* findPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) = 0;

	virtual bool	hasDeferredRemoval() = 0;

	virtual	void	setInternalGhostPairCallback(btOverlappingPairCallback* ghostPairCallback)=0;

	virtual void	sortOverlappingPairs(btDispatcher* dispatcher) = 0;


};


class btHashedOverlappingPairCache : public btOverlappingPairCache
{
	btBroadphasePairArray	m_overlappingPairArray;
	btOverlapFilterCallback* m_overlapFilterCallback;
	bool		m_blockedForChanges;

public:
	btHashedOverlappingPairCache();
	virtual ~btHashedOverlappingPairCache();

	
	void	removeOverlappingPairsContainingProxy(btBroadphaseProxy* proxy,btDispatcher* dispatcher);

	virtual void*	removeOverlappingPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1,btDispatcher* dispatcher);
	

	inline __attribute__((always_inline)) bool needsBroadphaseCollision(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1) const
	{
		if (m_overlapFilterCallback)
			return m_overlapFilterCallback->needBroadphaseCollision(proxy0,proxy1);

		bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
		collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);

		if(collides)
		{
			if(!hasInteractionEnabled(proxy0->m_interactionLayer, proxy1->m_interactionLayer))
				return false;

			if(checkInteractionBlocks(proxy0, proxy1))
				return false;
		}

		return collides;
	}

	
	
	virtual btBroadphasePair* 	addOverlappingPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1)
	{
		gAddedPairs++;

		if (!needsBroadphaseCollision(proxy0,proxy1))
			return 0;

		return internalAddPair(proxy0,proxy1);
	}

	

	void	cleanProxyFromPairs(btBroadphaseProxy* proxy,btDispatcher* dispatcher);

	
	virtual void	processAllOverlappingPairs(btOverlapCallback*,btDispatcher* dispatcher);

	virtual btBroadphasePair*	getOverlappingPairArrayPtr()
	{
		return &m_overlappingPairArray[0];
	}

	const btBroadphasePair*	getOverlappingPairArrayPtr() const
	{
		return &m_overlappingPairArray[0];
	}

	btBroadphasePairArray&	getOverlappingPairArray()
	{
		return m_overlappingPairArray;
	}

	const btBroadphasePairArray&	getOverlappingPairArray() const
	{
		return m_overlappingPairArray;
	}

	void	cleanOverlappingPair(btBroadphasePair& pair,btDispatcher* dispatcher);



	btBroadphasePair* findPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1);

	int GetCount() const { return m_overlappingPairArray.size(); }


	btOverlapFilterCallback* getOverlapFilterCallback()
	{
		return m_overlapFilterCallback;
	}

	void setOverlapFilterCallback(btOverlapFilterCallback* callback)
	{
		m_overlapFilterCallback = callback;
	}

	int	getNumOverlappingPairs() const
	{
		return m_overlappingPairArray.size();
	}
private:
	
	btBroadphasePair* 	internalAddPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1);

	void	growTables();

	inline __attribute__((always_inline)) bool equalsPair(const btBroadphasePair& pair, int proxyId1, int proxyId2)
	{	
		return pair.m_pProxy0->getUid() == proxyId1 && pair.m_pProxy1->getUid() == proxyId2;
	}

	













 


	
	inline __attribute__((always_inline))	unsigned int getHash(unsigned int proxyId1, unsigned int proxyId2)
	{
		int key = static_cast<int>(((unsigned int)proxyId1) | (((unsigned int)proxyId2) <<16));
		

		key += ~(key << 15);
		key ^=  (key >> 10);
		key +=  (key << 3);
		key ^=  (key >> 6);
		key += ~(key << 11);
		key ^=  (key >> 16);
		return static_cast<unsigned int>(key);
	}
	




	inline __attribute__((always_inline)) btBroadphasePair* internalFindPair(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1, int hash)
	{
		int proxyId1 = proxy0->getUid();
		int proxyId2 = proxy1->getUid();





		int index = m_hashTable[hash];
		
		while( index != BT_NULL_PAIR && equalsPair(m_overlappingPairArray[index], proxyId1, proxyId2) == false)
		{
			index = m_next[index];
		}

		if ( index == BT_NULL_PAIR )
		{
			return 0;
		}

		((index < m_overlappingPairArray . size()) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btOverlappingPairCache.h" ":" "305" " " "index < m_overlappingPairArray.size()", 0));

		return &m_overlappingPairArray[index];
	}

	virtual bool	hasDeferredRemoval()
	{
		return false;
	}

	virtual	void	setInternalGhostPairCallback(btOverlappingPairCallback* ghostPairCallback)
	{
		m_ghostPairCallback = ghostPairCallback;
	}

	virtual void	sortOverlappingPairs(btDispatcher* dispatcher);
	

protected:
	
	btAlignedObjectArray<int>	m_hashTable;
	btAlignedObjectArray<int>	m_next;
	btOverlappingPairCallback*	m_ghostPairCallback;
	
};






class	btSortedOverlappingPairCache : public btOverlappingPairCache
{
	protected:
		
		btBroadphasePairArray	m_overlappingPairArray;

		
		bool		m_blockedForChanges;

		
		bool		m_hasDeferredRemoval;
		
		
		btOverlapFilterCallback* m_overlapFilterCallback;

		btOverlappingPairCallback*	m_ghostPairCallback;

	public:
			
		btSortedOverlappingPairCache();	
		virtual ~btSortedOverlappingPairCache();

		virtual void	processAllOverlappingPairs(btOverlapCallback*,btDispatcher* dispatcher);

		void*	removeOverlappingPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1,btDispatcher* dispatcher);

		void	cleanOverlappingPair(btBroadphasePair& pair,btDispatcher* dispatcher);
		
		btBroadphasePair*	addOverlappingPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1);

		btBroadphasePair*	findPair(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1);
			
		
		void	cleanProxyFromPairs(btBroadphaseProxy* proxy,btDispatcher* dispatcher);

		void	removeOverlappingPairsContainingProxy(btBroadphaseProxy* proxy,btDispatcher* dispatcher);


		inline bool needsBroadphaseCollision(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1) const
		{
			if (m_overlapFilterCallback)
				return m_overlapFilterCallback->needBroadphaseCollision(proxy0,proxy1);

			bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
			collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);
			
			return collides;
		}
		
		btBroadphasePairArray&	getOverlappingPairArray()
		{
			return m_overlappingPairArray;
		}

		const btBroadphasePairArray&	getOverlappingPairArray() const
		{
			return m_overlappingPairArray;
		}

		


		btBroadphasePair*	getOverlappingPairArrayPtr()
		{
			return &m_overlappingPairArray[0];
		}

		const btBroadphasePair*	getOverlappingPairArrayPtr() const
		{
			return &m_overlappingPairArray[0];
		}

		int	getNumOverlappingPairs() const
		{
			return m_overlappingPairArray.size();
		}
		
		btOverlapFilterCallback* getOverlapFilterCallback()
		{
			return m_overlapFilterCallback;
		}

		void setOverlapFilterCallback(btOverlapFilterCallback* callback)
		{
			m_overlapFilterCallback = callback;
		}

		virtual bool	hasDeferredRemoval()
		{
			return m_hasDeferredRemoval;
		}

		virtual	void	setInternalGhostPairCallback(btOverlappingPairCallback* ghostPairCallback)
		{
			m_ghostPairCallback = ghostPairCallback;
		}

		virtual void	sortOverlappingPairs(btDispatcher* dispatcher);
		

};




class btNullPairCache : public btOverlappingPairCache
{

	btBroadphasePairArray	m_overlappingPairArray;

public:

	virtual btBroadphasePair*	getOverlappingPairArrayPtr()
	{
		return &m_overlappingPairArray[0];
	}
	const btBroadphasePair*	getOverlappingPairArrayPtr() const
	{
		return &m_overlappingPairArray[0];
	}
	btBroadphasePairArray&	getOverlappingPairArray()
	{
		return m_overlappingPairArray;
	}
	
	virtual	void	cleanOverlappingPair(btBroadphasePair&  ,btDispatcher*  )
	{

	}

	virtual int getNumOverlappingPairs() const
	{
		return 0;
	}

	virtual void	cleanProxyFromPairs(btBroadphaseProxy*  ,btDispatcher*  )
	{

	}

	virtual	void setOverlapFilterCallback(btOverlapFilterCallback*  )
	{
	}

	virtual void	processAllOverlappingPairs(btOverlapCallback*,btDispatcher*  )
	{
	}

	virtual btBroadphasePair* findPair(btBroadphaseProxy*  , btBroadphaseProxy*  )
	{
		return 0;
	}

	virtual bool	hasDeferredRemoval()
	{
		return true;
	}

	virtual	void	setInternalGhostPairCallback(btOverlappingPairCallback*  )
	{

	}

	virtual btBroadphasePair*	addOverlappingPair(btBroadphaseProxy*  ,btBroadphaseProxy*  )
	{
		return 0;
	}

	virtual void*	removeOverlappingPair(btBroadphaseProxy*  ,btBroadphaseProxy*  ,btDispatcher*  )
	{
		return 0;
	}

	virtual void	removeOverlappingPairsContainingProxy(btBroadphaseProxy*  ,btDispatcher*  )
	{
	}
	
	virtual void	sortOverlappingPairs(btDispatcher* dispatcher)
	{
        (void) dispatcher;
	}


};





# 21 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.h"


class btBroadphaseInterface;
class btSimpleBroadphase;


typedef btAlignedObjectArray<btBroadphaseInterface*> btSapBroadphaseArray;







class btMultiSapBroadphase :public btBroadphaseInterface
{
	btSapBroadphaseArray	m_sapBroadphases;
	
	btSimpleBroadphase*		m_simpleBroadphase;

	btOverlappingPairCache*	m_overlappingPairs;

	class btQuantizedBvh*			m_optimizedAabbTree;


	bool					m_ownsPairCache;
	
	btOverlapFilterCallback*	m_filterCallback;

	int			m_invalidPair;

	struct	btBridgeProxy
	{
		btBroadphaseProxy*		m_childProxy;
		btBroadphaseInterface*	m_childBroadphase;
	};


public:

	struct	btMultiSapProxy	: public btBroadphaseProxy
	{

		
		btAlignedObjectArray<btBridgeProxy*> m_bridgeProxies;
		btVector3	m_aabbMin;
		btVector3	m_aabbMax;

		int	m_shapeType;




 
		btMultiSapProxy(const btVector3& aabbMin,  const btVector3& aabbMax,int shapeType,void* userPtr, short int collisionFilterGroup,short int collisionFilterMask)
			:btBroadphaseProxy(aabbMin,aabbMax,userPtr,collisionFilterGroup,collisionFilterMask),
			m_aabbMin(aabbMin),
			m_aabbMax(aabbMax),
			m_shapeType(shapeType)
		{
			m_multiSapParentProxy =this;
		}

		
	};

protected:


	btAlignedObjectArray<btMultiSapProxy*> m_multiSapProxies;

public:

	btMultiSapBroadphase(int maxProxies = 16384,btOverlappingPairCache* pairCache=0);


	btSapBroadphaseArray&	getBroadphaseArray()
	{
		return m_sapBroadphases;
	}

	const btSapBroadphaseArray&	getBroadphaseArray() const
	{
		return m_sapBroadphases;
	}

	virtual ~btMultiSapBroadphase();

	virtual btBroadphaseProxy*	createProxy(  const btVector3& aabbMin,  const btVector3& aabbMax,int shapeType,void* userPtr, short int collisionFilterGroup,short int collisionFilterMask, unsigned int interactionLayer,btDispatcher* dispatcher,void* multiSapProxy);
	virtual void	destroyProxy(btBroadphaseProxy* proxy,btDispatcher* dispatcher);
	virtual void	setAabb(btBroadphaseProxy* proxy,const btVector3& aabbMin,const btVector3& aabbMax, btDispatcher* dispatcher);
	virtual void	getAabb(btBroadphaseProxy* proxy,btVector3& aabbMin, btVector3& aabbMax ) const;

	virtual void	rayTest(const btVector3& rayFrom,const btVector3& rayTo, btBroadphaseRayCallback& rayCallback,const btVector3& aabbMin=btZeroVec(),const btVector3& aabbMax=btZeroVec());

	void	addToChildBroadphase(btMultiSapProxy* parentMultiSapProxy, btBroadphaseProxy* childProxy, btBroadphaseInterface*	childBroadphase);

	
	virtual void	calculateOverlappingPairs(btDispatcher* dispatcher);

	bool	testAabbOverlap(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1);

	virtual	btOverlappingPairCache*	getOverlappingPairCache()
	{
		return m_overlappingPairs;
	}
	virtual	const btOverlappingPairCache*	getOverlappingPairCache() const
	{
		return m_overlappingPairs;
	}

	
	
	virtual void getBroadphaseAabb(btVector3& aabbMin,btVector3& aabbMax) const
	{
		aabbMin.setValue(-1e18f,-1e18f,-1e18f);
		aabbMax.setValue(1e18f,1e18f,1e18f);
	}

	void	buildTree(const btVector3& bvhAabbMin,const btVector3& bvhAabbMax);

	virtual void	printStats();

	void quicksort (btBroadphasePairArray& a, int lo, int hi);

	
	virtual void resetPool(btDispatcher* dispatcher);

};

# 17 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp"

# 1 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btSimpleBroadphase.h"













 








struct btSimpleBroadphaseProxy : public btBroadphaseProxy
{
	int			m_nextFree;
	


	
	btSimpleBroadphaseProxy() {};

	btSimpleBroadphaseProxy(const btVector3& minpt,const btVector3& maxpt,int shapeType,void* userPtr,short int collisionFilterGroup,short int collisionFilterMask,void* multiSapProxy)
	:btBroadphaseProxy(minpt,maxpt,userPtr,collisionFilterGroup,collisionFilterMask,multiSapProxy)
	{
		(void)shapeType;
	}
	
	
	inline __attribute__((always_inline)) void SetNextFree(int next) {m_nextFree = next;}
	inline __attribute__((always_inline)) int GetNextFree() const {return m_nextFree;}

	


};



class btSimpleBroadphase : public btBroadphaseInterface
{

protected:

	int		m_numHandles;						
	int		m_maxHandles;						
	int		m_LastHandleIndex;							
	
	btSimpleBroadphaseProxy* m_pHandles;						

	void* m_pHandlesRawPtr;
	int		m_firstFreeHandle;		
	
	int allocHandle()
	{
		((m_numHandles < m_maxHandles) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btSimpleBroadphase.h" ":" "65" " " "m_numHandles < m_maxHandles", 0));
		int freeHandle = m_firstFreeHandle;
		m_firstFreeHandle = m_pHandles[freeHandle].GetNextFree();
		m_numHandles++;
		if(freeHandle > m_LastHandleIndex)
		{
			m_LastHandleIndex = freeHandle;
		}
		return freeHandle;
	}

	void freeHandle(btSimpleBroadphaseProxy* proxy)
	{
		int handle = int(proxy-m_pHandles);
		((handle >= 0 && handle < m_maxHandles) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btSimpleBroadphase.h" ":" "79" " " "handle >= 0 && handle < m_maxHandles", 0));
		if(handle == m_LastHandleIndex)
		{
			m_LastHandleIndex--;
		}
		proxy->SetNextFree(m_firstFreeHandle);
		m_firstFreeHandle = handle;

		proxy->m_clientObject = 0;

		m_numHandles--;
	}

	btOverlappingPairCache*	m_pairCache;
	bool	m_ownsPairCache;

	int	m_invalidPair;

	
	
	inline btSimpleBroadphaseProxy*	getSimpleProxyFromProxy(btBroadphaseProxy* proxy)
	{
		btSimpleBroadphaseProxy* proxy0 = static_cast<btSimpleBroadphaseProxy*>(proxy);
		return proxy0;
	}

	inline const btSimpleBroadphaseProxy*	getSimpleProxyFromProxy(btBroadphaseProxy* proxy) const
	{
		const btSimpleBroadphaseProxy* proxy0 = static_cast<const btSimpleBroadphaseProxy*>(proxy);
		return proxy0;
	}

	
	virtual void resetPool(btDispatcher* dispatcher);


	void	validate();

protected:


	

public:
	btSimpleBroadphase(int maxProxies=16384,btOverlappingPairCache* overlappingPairCache=0);
	virtual ~btSimpleBroadphase();


		static bool	aabbOverlap(btSimpleBroadphaseProxy* proxy0,btSimpleBroadphaseProxy* proxy1);


	virtual btBroadphaseProxy*	createProxy(  const btVector3& aabbMin,  const btVector3& aabbMax,int shapeType,void* userPtr ,short int collisionFilterGroup,short int collisionFilterMask, unsigned int interactionLayer,btDispatcher* dispatcher,void* multiSapProxy);

	virtual void	calculateOverlappingPairs(btDispatcher* dispatcher);

	virtual void	destroyProxy(btBroadphaseProxy* proxy,btDispatcher* dispatcher);
	virtual void	setAabb(btBroadphaseProxy* proxy,const btVector3& aabbMin,const btVector3& aabbMax, btDispatcher* dispatcher);
	virtual void	getAabb(btBroadphaseProxy* proxy,btVector3& aabbMin, btVector3& aabbMax ) const;

	virtual void	rayTest(const btVector3& rayFrom,const btVector3& rayTo, btBroadphaseRayCallback& rayCallback, const btVector3& aabbMin=btZeroVec(),const btVector3& aabbMax=btZeroVec());
	virtual void	aabbTest(const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback);
		
	btOverlappingPairCache*	getOverlappingPairCache()
	{
		return m_pairCache;
	}
	const btOverlappingPairCache*	getOverlappingPairCache() const
	{
		return m_pairCache;
	}

	bool	testAabbOverlap(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1);


	
	
	virtual void getBroadphaseAabb(btVector3& aabbMin,btVector3& aabbMax) const
	{
		aabbMin.setValue(-1e18f,-1e18f,-1e18f);
		aabbMax.setValue(1e18f,1e18f,1e18f);
	}

	virtual void	printStats()
	{


	}
};





# 19 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp"
# 1 "..\\..\\src\\LinearMath/btAabbUtil2.h"












 






# 1 "..\\..\\src\\LinearMath/btTransform.h"












  





# 1 "..\\..\\src\\LinearMath/btMatrix3x3.h"












  



# 1 "..\\..\\src\\LinearMath/btQuaternion.h"












 




# 1 "..\\..\\src\\LinearMath/btQuadWord.h"












 















 

class __attribute__ ((aligned (16))) btQuadWord



{
protected:


public:
	union {
		vec_float4 mVec128;
		btScalar	m_floats[4];
	};
public:
	inline __attribute__((always_inline)) vec_float4	get128() const
	{
		return mVec128;
	}

	inline __attribute__((always_inline)) void 	setValue(const vec_float4 vec)
	{
		mVec128 = vec;
	}
protected:




	public:

	 
		inline __attribute__((always_inline)) const btScalar& getX() const { return m_floats[0]; }
   
		inline __attribute__((always_inline)) const btScalar& getY() const { return m_floats[1]; }
   
		inline __attribute__((always_inline)) const btScalar& getZ() const { return m_floats[2]; }
   
		inline __attribute__((always_inline)) void	setX(btScalar x) { m_floats[0] = x;};
   
		inline __attribute__((always_inline)) void	setY(btScalar y) { m_floats[1] = y;};
   
		inline __attribute__((always_inline)) void	setZ(btScalar z) { m_floats[2] = z;};
   
		inline __attribute__((always_inline)) void	setW(btScalar w) { m_floats[3] = w;};
   
		inline __attribute__((always_inline)) const btScalar& x() const { return m_floats[0]; }
   
		inline __attribute__((always_inline)) const btScalar& y() const { return m_floats[1]; }
   
		inline __attribute__((always_inline)) const btScalar& z() const { return m_floats[2]; }
   
		inline __attribute__((always_inline)) const btScalar& w() const { return m_floats[3]; }

	
	
	
	inline __attribute__((always_inline))	operator       btScalar *()       { return &m_floats[0]; }
	inline __attribute__((always_inline))	operator const btScalar *() const { return &m_floats[0]; }


	inline __attribute__((always_inline))	bool	operator==(const btQuadWord& other) const
	{
		
		return vec_all_eq(mVec128, other.mVec128);
	}
# 102 "..\\..\\src\\LinearMath/btQuadWord.h"

	inline __attribute__((always_inline))	bool	operator!=(const btQuadWord& other) const
	{
		return !(*this == other);
	}

  



 
		inline __attribute__((always_inline)) void 	setValue(const btScalar& x, const btScalar& y, const btScalar& z)
		{
			m_floats[0]=x;
			m_floats[1]=y;
			m_floats[2]=z;
			m_floats[3] = 0.f;
		}







 





 
		inline __attribute__((always_inline)) void	setValue(const btScalar& x, const btScalar& y, const btScalar& z,const btScalar& w)
		{
			m_floats[0]=x;
			m_floats[1]=y;
			m_floats[2]=z;
			m_floats[3]=w;
		}
   
		inline __attribute__((always_inline)) btQuadWord()
		
		{
		}
 
  



 
		inline __attribute__((always_inline)) btQuadWord(const btScalar& x, const btScalar& y, const btScalar& z)		
		{
			
			setValue(x, y, z, 0.f);
		}






 
		inline __attribute__((always_inline)) btQuadWord(const btScalar& x, const btScalar& y, const btScalar& z,const btScalar& w) 
		{
			
			setValue(x, y, z, w);
		}

  

 
		inline __attribute__((always_inline)) void	setMax(const btQuadWord& other)
		{

			
			mVec128 = vec_max(mVec128, other.mVec128);
# 184 "..\\..\\src\\LinearMath/btQuadWord.h"
		}
  

 
		inline __attribute__((always_inline)) void	setMin(const btQuadWord& other)
		{

			
			mVec128 = vec_min(mVec128, other.mVec128);
# 199 "..\\..\\src\\LinearMath/btQuadWord.h"
		}



};

# 20 "..\\..\\src\\LinearMath/btQuaternion.h"

 
class btQuaternion : public btQuadWord {
public:
   
	btQuaternion() {}


	inline __attribute__((always_inline)) btQuaternion(const vec_float4 vec)		
	{
		mVec128 = vec;
	}

	inline __attribute__((always_inline)) btQuaternion(const btVector3 vec)		
	{
		mVec128 = vec.mVec;
	}


	
	
   
	btQuaternion(const btScalar& x, const btScalar& y, const btScalar& z, const btScalar& w) 
		: btQuadWord(x, y, z, w) 
	{}
  

 
	btQuaternion(const btVector3& axis, const btScalar& angle) 
	{ 
		setRotation(axis, angle); 
	}
  


 
	btQuaternion(const btScalar& yaw, const btScalar& pitch, const btScalar& roll)
	{ 

		setEuler(yaw, pitch, roll); 



	}
  

 
	void setRotation(const btVector3& axis, const btScalar& angle)
	{
		btScalar d = axis.length();
		((d != btScalar(0.0)) ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btQuaternion.h" ":" "70" " " "d != btScalar(0.0)", 0));
		btScalar s = btSin(angle * btScalar(0.5)) / d;
		setValue(axis.getX() * s, axis.getY() * s, axis.getZ() * s, 
			btCos(angle * btScalar(0.5)));
	}
  


 
	void setEuler(const btScalar& yaw, const btScalar& pitch, const btScalar& roll)
	{
		btScalar halfYaw = btScalar(yaw) * btScalar(0.5);  
		btScalar halfPitch = btScalar(pitch) * btScalar(0.5);  
		btScalar halfRoll = btScalar(roll) * btScalar(0.5);  
		btScalar cosYaw = btCos(halfYaw);
		btScalar sinYaw = btSin(halfYaw);
		btScalar cosPitch = btCos(halfPitch);
		btScalar sinPitch = btSin(halfPitch);
		btScalar cosRoll = btCos(halfRoll);
		btScalar sinRoll = btSin(halfRoll);
		setValue(cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,
			cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw,
			sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,
			cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw);
	}
  


 
	void setEulerZYX(const btScalar& yaw, const btScalar& pitch, const btScalar& roll)
	{
		btScalar halfYaw = btScalar(yaw) * btScalar(0.5);  
		btScalar halfPitch = btScalar(pitch) * btScalar(0.5);  
		btScalar halfRoll = btScalar(roll) * btScalar(0.5);  
		btScalar cosYaw = btCos(halfYaw);
		btScalar sinYaw = btSin(halfYaw);
		btScalar cosPitch = btCos(halfPitch);
		btScalar sinPitch = btSin(halfPitch);
		btScalar cosRoll = btCos(halfRoll);
		btScalar sinRoll = btSin(halfRoll);
		setValue(sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw, 
                         cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw, 
                         cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw, 
                         cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw); 
	}
  
 
	inline __attribute__((always_inline))	btQuaternion& operator+=(const btQuaternion& q)
	{

		
		mVec128 = vec_add(mVec128, q.mVec128);



		return *this;
	}

  
 
	btQuaternion& operator-=(const btQuaternion& q) 
	{

		
		mVec128 = vec_sub(mVec128, q.mVec128);



		return *this;
	}

  
 
	btQuaternion& operator*=(const btScalar& s)
	{

		
		mVec128 = vec_madd(mVec128, vec_float4(s), btZero);



		return *this;
	}

  

 
	btQuaternion& operator*=(const btQuaternion& q)
	{

		
		vec_float4 xklm = vec_sld(q.mVec128, q.mVec128, 12);
		vec_float4 tmp = vec_madd(mVec128, vec_sld(xklm, q.mVec128, 8), btZeroVec());	
		vec_float4 bmckal = vec_sld(vec_sld(tmp, tmp, 12), tmp, 8);
		tmp = vec_madd(mVec128, vec_sld(xklm, q.mVec128, 12), btZeroVec()); 	
		vec_float4 clambk = vec_sld(vec_sld(tmp, tmp, 12), tmp, 12);

		vec_float4 d = vec_splat(mVec128, 3);
		vec_float4 n = vec_splat(q.mVec128, 3);
		vec_float4 res = vec_madd(q.mVec128, d, vec_madd(mVec128, n, vec_sub(bmckal, clambk)));	

		tmp = vec_madd(mVec128, q.mVec128, btZeroVec());	
		tmp = vec_sub(tmp, vec_sld(tmp, tmp, 4));		
		vec_float4 ret = vec_sel(res, vec_sub(tmp, vec_sld(tmp, tmp, 8)), btSelVec3);		
		mVec128 = ret;
# 181 "..\\..\\src\\LinearMath/btQuaternion.h"
		return *this;
	}
  
 
	btScalar dot(const btQuaternion& q) const
	{

		
		vec_float4 tmp = vec_madd(mVec128, q.mVec128, btZeroVec());
		vec_float4 result = vec_add(tmp, vec_sld(tmp, tmp, 8));
		return btScalar(vec_extract(vec_add(result, vec_sld(result, result, 4)), 0));



	}

   
	btScalar length2() const
	{
		return dot(*this);
	}

   
	btScalar length() const
	{
		return btSqrt(length2());
	}

  
 
	btQuaternion& normalize() 
	{
		return *this /= length();
	}

  
 
	inline __attribute__((always_inline)) btQuaternion
	operator*(const btScalar& s) const
	{

		
		return btQuaternion(vec_madd(mVec128, vec_float4(s), btZero));



	}


  
 
	btQuaternion operator/(const btScalar& s) const
	{
		((s != btScalar(0.0)) ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btQuaternion.h" ":" "234" " " "s != btScalar(0.0)", 0));
		return *this * (btScalar(1.0) / s);
	}

  
 
	btQuaternion& operator/=(const btScalar& s) 
	{
		((s != btScalar(0.0)) ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btQuaternion.h" ":" "242" " " "s != btScalar(0.0)", 0));
		return *this *= btScalar(1.0) / s;
	}

   
	btQuaternion normalized() const 
	{
		return *this / length();
	} 
  
 
	btScalar angle(const btQuaternion& q) const 
	{
		btScalar s = btSqrt(length2() * q.length2());
		((s != btScalar(0.0)) ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btQuaternion.h" ":" "256" " " "s != btScalar(0.0)", 0));
		return btAcos(dot(q) / s);
	}
   
	btScalar getAngle() const 
	{
		btScalar s = btScalar(2.) * btAcos(m_floats[3]);
		return s;
	}

	 
	btVector3 getAxis() const
	{

		
		btScalar s_squared = btScalar(1.) - btPow(getW(), btScalar(2.));
		if (s_squared < btScalar(10.) * 1.1920928955078125e-07f) 
			return btIdentVec0;  
		float res = 1/btSqrt(s_squared);
		return btVector3(vec_madd(mVec128, vec_splats(res), btZeroVec()));
# 283 "..\\..\\src\\LinearMath/btQuaternion.h"
	}

	 
	btQuaternion inverse() const
	{
		return btQuaternion(-m_floats[0], -m_floats[1], -m_floats[2], m_floats[3]);
	}

  
 
	inline __attribute__((always_inline)) btQuaternion
	operator+(const btQuaternion& q2) const
	{

		
		return btQuaternion(vec_add(mVec128, q2.mVec128));




	}

  
 
	inline __attribute__((always_inline)) btQuaternion
	operator-(const btQuaternion& q2) const
	{

		
		return btQuaternion(vec_sub(mVec128, q2.mVec128));




	}

  
 
	inline __attribute__((always_inline)) btQuaternion operator-() const
	{

		
		return btQuaternion(vec_sub(btZero, mVec128));




	}
   
	inline __attribute__((always_inline)) btQuaternion farthest( const btQuaternion& qd) const 
	{

		
		vec_float4 diff = vec_sub(mVec128, qd.mVec128);
		vec_float4 sum = vec_add(mVec128, qd.mVec128);
		vec_float4 tmp = vec_sub(vec_madd(diff, diff, btZero), vec_madd(sum, sum, btZero));
		if(vec_any_gt(vec_sel(btZeroVec(), vec_add(vec_add(tmp, vec_sld(tmp, tmp, 4)), vec_sld(tmp, tmp, 8)), btSelVec0), btZeroVec()));
			return qd;
		return (-qd);
# 350 "..\\..\\src\\LinearMath/btQuaternion.h"
	}

	 
	inline __attribute__((always_inline)) btQuaternion nearest( const btQuaternion& qd) const 
	{

		
		vec_float4 diff = vec_sub(mVec128, qd.mVec128);
		vec_float4 sum = vec_add(mVec128, qd.mVec128);
		vec_float4 tmp = vec_sub(vec_madd(diff, diff, btZero), vec_madd(sum, sum, btZero));
		if(vec_any_lt(vec_sel(btZeroVec(), vec_add(vec_add(tmp, vec_sld(tmp, tmp, 4)), vec_sld(tmp, tmp, 8)), btSelVec0), btZeroVec()));
			return qd;
		return (-qd);
# 371 "..\\..\\src\\LinearMath/btQuaternion.h"
	}


  


 
	btQuaternion slerp(const btQuaternion& q, const btScalar& t) const
	{
		btScalar theta = angle(q);
		if (theta != btScalar(0.0))
		{
			btScalar d = btScalar(1.0) / btSin(theta);
			btScalar s0 = btSin((btScalar(1.0) - t) * theta);
			btScalar s1 = btSin(t * theta);   

			
			if (dot(q) < 0) 
			{
				vec_float4 tmp = vec_madd(mVec128, vec_float4(s0), btZero);
				tmp = vec_sub(tmp, vec_madd(q.mVec128, vec_float4(s1), btZero));
            return btQuaternion(vec_madd(tmp, vec_float4(d), btZero));
			}
         else
			{
				vec_float4 tmp = vec_madd(mVec128, vec_float4(s0), vec_madd(q.mVec128, vec_float4(s1), btZero));
				return btQuaternion(vec_madd(tmp, vec_float4(d), btZero));
			}
# 411 "..\\..\\src\\LinearMath/btQuaternion.h"
                        
		}
		else
		{
			return *this;
		}
	}

	static const btQuaternion&	getIdentity()
	{
		static const btQuaternion identityQuat(btScalar(0.),btScalar(0.),btScalar(0.),btScalar(1.));
		return identityQuat;
	}

	inline __attribute__((always_inline)) const btScalar& getW() const { return m_floats[3]; }
	
};


 
inline __attribute__((always_inline)) btQuaternion
operator-(const btQuaternion& q)
{
	
	return btQuaternion(vec_sub(btZeroVec(), q.mVec128));
}
# 445 "..\\..\\src\\LinearMath/btQuaternion.h"


 
inline __attribute__((always_inline)) btQuaternion
operator*(const btQuaternion& q1, const btQuaternion& q2) {
	

	







 

	vec_float4 nopm = vec_madd(q1.mVec128, q2.mVec128, btZeroVec());
	vec_float4 kcja = vec_madd(q1.mVec128, vec_sld(q2.mVec128, q2.mVec128, 4), btZeroVec());
	vec_float4 hfge = vec_madd(q1.mVec128, vec_sld(q2.mVec128, q2.mVec128, 8), btZeroVec());
	vec_float4 bldi = vec_madd(q1.mVec128, vec_sld(q2.mVec128, q2.mVec128, 12), btZeroVec());

	
	vec_float4 agie = vec_vmrglw(vec_vmrglw(kcja, bldi) , hfge);
	vec_float4 bfjg = vec_vmrghw(vec_vmrghw(bldi, vec_sld(kcja, kcja, 8)) , vec_sld(hfge, hfge, 4));
	vec_float4 cekh = vec_vmrghw(vec_sld(vec_sld(kcja, kcja, 8), kcja, 12) , vec_sld(hfge, hfge, 12));
	vec_float4 dhlf =  vec_vmrghw(vec_vmrglw(bldi, vec_sld(bldi, bldi, 12)) , hfge);

	
	vec_float4 mnop = vec_add(vec_add(nopm, vec_sld(nopm, nopm, 4)), vec_sld(nopm, nopm, 8));
	mnop = vec_sub(nopm, vec_sld(mnop, mnop, 4));

	vec_float4 res = vec_sub(vec_add(vec_add(agie, bfjg), cekh), dhlf);
	
	 return btQuaternion(vec_sel(res, mnop, btSelVec3));
}

inline __attribute__((always_inline)) btQuaternion
operator*(const btQuaternion& q, const btVector3& w)
{
	

	







 

	vec_float4 jklx = vec_madd(q.mVec128, w.mVec, btZeroVec());	
	vec_float4 hbxa = vec_madd(q.mVec128, vec_sld(w.mVec, w.mVec, 4), btZeroVec());	
	vec_float4 fxed = vec_madd(q.mVec128, vec_sld(w.mVec, w.mVec, 8), btZeroVec());	
	vec_float4 xicg = vec_madd(q.mVec128, vec_sld(w.mVec, w.mVec, 12), btZeroVec());	

	vec_float4 adgf = vec_vmrglw(vec_vmrglw(hbxa, xicg) , vec_sld(fxed, fxed, 4));
	vec_float4 behd = vec_vmrghw(vec_sld(vec_sld(hbxa, hbxa, 8), hbxa, 12) , vec_sld(fxed, fxed, 8));
	vec_float4 cfix = vec_vmrghw(vec_vmrglw(xicg, vec_sld(xicg, xicg, 12)) , fxed);

	vec_float4 res = vec_sub(vec_add(adgf, behd), cfix);
	vec_float4 last = vec_sub(btZeroVec(), jklx);
	last = vec_add(vec_add(vec_sld(last, last, 4), vec_sld(last, last, 8)), vec_sld(last, last, 12));
	vec_float4 ret = vec_sel(res, last, btSelVec3);

	return btQuaternion(ret);
}
# 541 "..\\..\\src\\LinearMath/btQuaternion.h"
 
inline __attribute__((always_inline)) btScalar 
dot(const btQuaternion& q1, const btQuaternion& q2) 
{ 
	return q1.dot(q2); 
}


 
inline __attribute__((always_inline)) btScalar
length(const btQuaternion& q) 
{ 
	return q.length(); 
}

 
inline __attribute__((always_inline)) btScalar
angle(const btQuaternion& q1, const btQuaternion& q2) 
{ 
	return q1.angle(q2); 
}

 
inline __attribute__((always_inline)) btQuaternion
inverse(const btQuaternion& q) 
{
	return q.inverse();
}





 
inline __attribute__((always_inline)) btQuaternion
slerp(const btQuaternion& q1, const btQuaternion& q2, const btScalar& t) 
{
	return q1.slerp(q2, t);
}

inline __attribute__((always_inline)) btVector3 
quatRotate(const btQuaternion& rotation, const btVector3& v) 
{
	btQuaternion q = rotation * v;
	q *= rotation.inverse();

	
	return btVector3(q.get128());



}

inline __attribute__((always_inline)) btQuaternion  
shortestArcQuat(const btVector3& v0, const btVector3& v1) 
{
	btVector3 c = v0.cross(v1);
	btScalar  d = v0.dot(v1);

	if (d < -1.0 + 1.1920928955078125e-07f)
	{
		btVector3 n,unused;
		btPlaneSpace1(v0,n,unused);
		return btQuaternion(n.getX(),n.getY(),n.getZ(),0.0f); 
	}

	btScalar tmp = 1.0f + d;
	btScalar  s = btSqrt(tmp + tmp);
	btScalar rs = 1.0f / s;

	return btQuaternion(c.getX()*rs,c.getY()*rs,c.getZ()*rs,s * 0.5f);
}

inline __attribute__((always_inline)) btQuaternion 
shortestArcQuatNormalize2(btVector3& v0,btVector3& v1)
{
	v0.normalize();
	v1.normalize();
	return shortestArcQuat(v0,v1);
}

# 19 "..\\..\\src\\LinearMath/btMatrix3x3.h"








 
class btMatrix3x3 {

	
	btVector3 m_el[3];

public:


	btMatrix3x3(const vec_float4 a, const vec_float4 b, const vec_float4 c)
	{
		m_el[0].mVec = a;
		m_el[1].mVec = b;
		m_el[2].mVec = c;
	};

	inline __attribute__((always_inline)) btVector3&  operator[](int i)
	{ 
		;
		return m_el[i]; 
	}
	
 
	inline __attribute__((always_inline)) const btVector3& operator[](int i) const
	{
		;
		return m_el[i]; 
	} 

	inline __attribute__((always_inline)) const vec_float4*	get128ptr() const
	{
		return (vec_float4*)m_el;
	}


	 
	btMatrix3x3 () {}

	

	 
	explicit btMatrix3x3(const btQuaternion& q) { setRotation(q); }
	





 
	 

	btMatrix3x3(const btScalar& xx, const btScalar& xy, const btScalar& xz,
		const btScalar& yx, const btScalar& yy, const btScalar& yz,
		const btScalar& zx, const btScalar& zy, const btScalar& zz)
	{ 
		setValue(xx, xy, xz, 
			yx, yy, yz, 
			zx, zy, zz);
	}

	 
	inline __attribute__((always_inline)) btMatrix3x3 (const btMatrix3x3& other)
	{
		m_el[0] = other.m_el[0];
		m_el[1] = other.m_el[1];
		m_el[2] = other.m_el[2];
	}
	 
	inline __attribute__((always_inline)) btMatrix3x3& operator=(const btMatrix3x3& other)
	{
		m_el[0] = other.m_el[0];
		m_el[1] = other.m_el[1];
		m_el[2] = other.m_el[2];
		return *this;
	}


	inline __attribute__((always_inline)) btVector3 getDiagonal() const
	{
		
		return btVector3(vec_vmrghw(vec_vmrghw(m_el[0].mVec, vec_sld(m_el[2].mVec, m_el[2].mVec, 8)), vec_sld(m_el[1].mVec, m_el[1].mVec, 4)));
	}


	
 
	inline __attribute__((always_inline)) btVector3 getColumn(int i) const
	{
		return btVector3(m_el[0][i],m_el[1][i],m_el[2][i]);
	}


	
 
	inline __attribute__((always_inline)) const btVector3 getRow(int i) const
	{
		;
		return m_el[i];
	}

	
 
# 144 "..\\..\\src\\LinearMath/btMatrix3x3.h"

	

 
	btMatrix3x3& operator*=(const btMatrix3x3& m); 

	
 
# 160 "..\\..\\src\\LinearMath/btMatrix3x3.h"
	








 
	void setValue(const btScalar& xx, const btScalar& xy, const btScalar& xz, 
		const btScalar& yx, const btScalar& yy, const btScalar& yz, 
		const btScalar& zx, const btScalar& zy, const btScalar& zz)
	{
		m_el[0].setValue(xx,xy,xz);
		m_el[1].setValue(yx,yy,yz);
		m_el[2].setValue(zx,zy,zz);
	}
	
   
	void setRotation(const btQuaternion& q) 
	{
		btScalar d = q.length2();
		;
		btScalar s = btScalar(2.0) / d;
	
# 226 "..\\..\\src\\LinearMath/btMatrix3x3.h"
		btScalar xs = q.x() * s,   ys = q.y() * s,   zs = q.z() * s;
		btScalar wx = q.w() * xs,  wy = q.w() * ys,  wz = q.w() * zs;
		btScalar xx = q.x() * xs,  xy = q.x() * ys,  xz = q.x() * zs;
		btScalar yy = q.y() * ys,  yz = q.y() * zs,  zz = q.z() * zs;
		setValue(btScalar(1.0) - (yy + zz), xy - wz, xz + wy,
			xy + wz, btScalar(1.0) - (xx + zz), yz - wx,
			xz - wy, yz + wx, btScalar(1.0) - (xx + yy));

	}


	



 
	void setEulerYPR(const btScalar& yaw, const btScalar& pitch, const btScalar& roll) 
	{
		setEulerZYX(roll, pitch, yaw);
	}

	







 
	void setEulerZYX(btScalar eulerX,btScalar eulerY,btScalar eulerZ) { 
		
		btScalar ci ( btCos(eulerX)); 
		btScalar cj ( btCos(eulerY)); 
		btScalar ch ( btCos(eulerZ)); 
		btScalar si ( btSin(eulerX)); 
		btScalar sj ( btSin(eulerY)); 
		btScalar sh ( btSin(eulerZ)); 
		btScalar cc = ci * ch; 
		btScalar cs = ci * sh; 
		btScalar sc = si * ch; 
		btScalar ss = si * sh;

		setValue(cj * ch, sj * sc - cs, sj * cc + ss,
			cj * sh, sj * ss + cc, sj * cs - sc, 
			-sj,      cj * si,      cj * ci);
	}

	 
	void setIdentity()
	{ 

		
		m_el[0].mVec = btIdentVec0;
		m_el[1].mVec = btIdentVec1;
		m_el[2].mVec = btIdentVec2;





	}

	static const btMatrix3x3&	getIdentity()
	{

		
		static const btMatrix3x3 identityMatrix(btIdentVec0, btIdentVec1, btIdentVec2);





		return identityMatrix;
	}

	
 
# 321 "..\\..\\src\\LinearMath/btMatrix3x3.h"
	
 
	void getRotation(btQuaternion& q) const
	{
# 386 "..\\..\\src\\LinearMath/btMatrix3x3.h"
		btScalar trace = btVector3(m_el[0]).getX() + btVector3(m_el[1]).getY() + btVector3(m_el[2]).getZ();
		btScalar temp[4];

		if (trace > btScalar(0.0)) 
		{
			btScalar s = btSqrt(trace + btScalar(1.0));
			temp[3]=(s * btScalar(0.5));
			s = btScalar(0.5) / s;

			temp[0]=((btVector3(m_el[2]).getY() - btVector3(m_el[1]).getZ()) * s);
			temp[1]=((btVector3(m_el[0]).getZ() - btVector3(m_el[2]).getX()) * s);
			temp[2]=((btVector3(m_el[1]).getX() - btVector3(m_el[0]).getY()) * s);
		} 
		else 
		{
			int i = btVector3(m_el[0]).getX() < btVector3(m_el[1]).getY() ? 
				(btVector3(m_el[1]).getY() < btVector3(m_el[2]).getZ() ? 2 : 1) :
				(btVector3(m_el[0]).getX() < btVector3(m_el[2]).getZ() ? 2 : 0); 
			int j = (i + 1) % 3;  
			int k = (i + 2) % 3;
			
			btScalar s = btSqrt(m_el[i][i] - m_el[j][j] - m_el[k][k] + btScalar(1.0));
			temp[i] = s * btScalar(0.5);
			s = btScalar(0.5) / s;

			temp[3] = (m_el[k][j] - m_el[j][k]) * s;
			temp[j] = (m_el[j][i] + m_el[i][j]) * s;
			temp[k] = (m_el[k][i] + m_el[i][k]) * s;
		}
		q.setValue(temp[0],temp[1],temp[2],temp[3]);

	}

	


 	

# 519 "..\\..\\src\\LinearMath/btMatrix3x3.h"
	
 

	btMatrix3x3 scaled(const btVector3& s) const
	{

		
		return btMatrix3x3(vec_madd(s.mVec, m_el[0].mVec, btZeroVec()), vec_madd(s.mVec, m_el[1].mVec, btZeroVec()), vec_madd(s.mVec, m_el[2].mVec, btZeroVec()));





	}

	 
	btScalar            determinant() const;
	 
	btMatrix3x3 adjoint() const;
	 
	btMatrix3x3 absolute() const;
	 
	btMatrix3x3 transpose() const;
	 
	btMatrix3x3 inverse() const; 

	btMatrix3x3 transposeTimes(const btMatrix3x3& m) const;
	btMatrix3x3 timesTranspose(const btMatrix3x3& m) const;

# 562 "..\\..\\src\\LinearMath/btMatrix3x3.h"

	







 
	void diagonalize(btMatrix3x3& rot, btScalar threshold, int maxSteps)
	{
# 666 "..\\..\\src\\LinearMath/btMatrix3x3.h"
		rot.setIdentity();
		for (int step = maxSteps; step > 0; step--)
		{
			
			int p = 0;
			int q = 1;
			int r = 2;
			btScalar max = btFabs(m_el[0][1]);
			btScalar v = btFabs(m_el[0][2]);
			if (v > max)
			{
				q = 2;
				r = 1;
				max = v;
			}
			v = btFabs(m_el[1][2]);
			if (v > max)
			{
				p = 1;
				q = 2;
				r = 0;
				max = v;
			}

			btScalar t = threshold * (btFabs(m_el[0][0]) + btFabs(m_el[1][1]) + btFabs(m_el[2][2]));
			if (max <= t)
			{
				if (max <= 1.1920928955078125e-07f * t)
				{
					return;
				}
				step = 1;
			}

			
			btScalar mpq = m_el[p][q];
			btScalar theta = (m_el[q][q] - m_el[p][p]) / (2 * mpq);
			btScalar theta2 = theta * theta;
			btScalar cos;
			btScalar sin;
			if (theta2 * theta2 < btScalar(10 / 1.1920928955078125e-07f))
			{
				t = (theta >= 0) ? 1 / (theta + btSqrt(1 + theta2))
					: 1 / (theta - btSqrt(1 + theta2));
				cos = 1 / btSqrt(1 + t * t);
				sin = cos * t;
			}
			else
			{
				
				t = 1 / (theta * (2 + btScalar(0.5) / theta2));
				cos = 1 - btScalar(0.5) * t * t;
				sin = cos * t;
			}

			
			m_el[p][q] = m_el[q][p] = 0;
			m_el[p][p] -= t * mpq;
			m_el[q][q] += t * mpq;
			btScalar mrp = m_el[r][p];
			btScalar mrq = m_el[r][q];
			m_el[r][p] = m_el[p][r] = cos * mrp - sin * mrq;
			m_el[r][q] = m_el[q][r] = cos * mrq + sin * mrp;

			
			for (int i = 0; i < 3; i++)
			{
				btVector3& row = rot[i];
				mrp = row[p];
				mrq = row[q];
				row[p] = cos * mrp - sin * mrq;
				row[q] = cos * mrq + sin * mrp;
			}
		}

	}




	





 
# 759 "..\\..\\src\\LinearMath/btMatrix3x3.h"

	void	serialize(struct	btMatrix3x3FloatData& dataOut) const;

	void	serializeFloat(struct	btMatrix3x3FloatData& dataOut) const;

	void	deSerialize(const struct	btMatrix3x3FloatData& dataIn);

	void	deSerializeFloat(const struct	btMatrix3x3FloatData& dataIn);

	void	deSerializeDouble(const struct	btMatrix3x3DoubleData& dataIn);
};

inline __attribute__((always_inline)) btMatrix3x3& 
btMatrix3x3::operator*=(const btMatrix3x3& m)
{

	
	btMatrix3x3 mt = transpose();
	
	vec_float4 tmp3, bdg, cfh, aei;
	bdg = cfh = aei = btZeroVec();
	for(int i = 0; i < 3; ++i)
	{
		aei = vec_madd(m.m_el[i].mVec, mt.m_el[i].mVec, aei);
		
		tmp3 = vec_vmrghw(m.m_el[i].mVec, m.m_el[i].mVec);
		bdg = vec_madd(vec_sld(tmp3, tmp3, 12), mt.m_el[i].mVec, bdg); 

		tmp3 = vec_sld(m.m_el[i].mVec, m.m_el[i].mVec, 4);
		tmp3 = vec_vmrghw(tmp3, tmp3);
		cfh = vec_madd(vec_sld(tmp3, tmp3, 8), mt.m_el[i].mVec, cfh);
	}
	
	
	tmp3 = vec_vmrghw(aei, cfh);	
	vec_float4 tmp = vec_vmrghw(tmp3, bdg);
	
	m_el[0].mVec = tmp;
	m_el[1].mVec = vec_sld(tmp, vec_sld(tmp3, tmp3, 8), 12);
	m_el[2].mVec = vec_vmrglw(vec_sld(aei, vec_vmrglw(bdg, aei), 8), cfh);





	return *this;
}

inline __attribute__((always_inline)) btScalar 
btMatrix3x3::determinant() const
{ 




	return btTriple((*this)[0], (*this)[1], (*this)[2]);

}


inline __attribute__((always_inline)) btMatrix3x3 
btMatrix3x3::absolute() const
{

	
	return btMatrix3x3(
		vec_abs(m_el[0].mVec), vec_abs(m_el[1].mVec), vec_abs(m_el[2].mVec));
# 832 "..\\..\\src\\LinearMath/btMatrix3x3.h"
}

inline __attribute__((always_inline)) btMatrix3x3 
btMatrix3x3::transpose() const 
{

	
	vec_float4 tmp = vec_vmrghw(m_el[0].mVec, m_el[2].mVec);	
	return btMatrix3x3(vec_vmrghw(tmp, m_el[1].mVec), 
							vec_vmrglw(tmp, vec_sld(m_el[1].mVec, m_el[1].mVec, 12)),	
							vec_vmrghw(vec_vmrglw(m_el[0].mVec, m_el[2].mVec), vec_sld(m_el[1].mVec, m_el[1].mVec,	8))); 





}

inline __attribute__((always_inline)) btMatrix3x3 
btMatrix3x3::adjoint() const 
{

	
	vec_float4 coxyz = vec_sld(m_el[2].mVec, m_el[2].mVec, 12);

	vec_float4 byzx = vec_sld(vec_sld(m_el[1].mVec, m_el[1].mVec, 12), m_el[1].mVec, 8);
	
	
	vec_float4 icf = vec_madd(m_el[0].mVec, byzx, btZeroVec());
	vec_float4 gad = vec_madd(m_el[1].mVec, vec_sld(coxyz, m_el[2].mVec, 8), btZeroVec());
	vec_float4 ehb = vec_madd(m_el[0].mVec, vec_sld(coxyz, m_el[2].mVec, 12), btZeroVec());

	vec_float4 aoxyz = vec_sld(m_el[0].mVec, m_el[0].mVec, 12);

	
	
	
	icf = vec_sub(icf, vec_madd(m_el[1].mVec, vec_sld(aoxyz, m_el[0].mVec, 8), btZeroVec()));
	gad = vec_sub(gad, vec_madd(m_el[2].mVec, byzx, btZeroVec()));
	ehb = vec_sub(ehb, vec_madd(m_el[2].mVec, vec_sld(aoxyz, m_el[0].mVec, 12), btZeroVec()));

	vec_float4 tmp = vec_vmrghw(gad, icf);		
	
	
	
	return btMatrix3x3(vec_vmrglw(tmp, ehb),
					vec_vmrghw(vec_vmrglw(gad, icf), ehb),
					vec_vmrghw(tmp, vec_sld(ehb, ehb, 4)));





}

inline __attribute__((always_inline)) btMatrix3x3 
btMatrix3x3::inverse() const
{

	
	



 

	vec_float4 coxyz = vec_sld(m_el[2].mVec, m_el[2].mVec, 12);
	vec_float4 czxy = vec_sld(coxyz, m_el[2].mVec, 12);
	vec_float4 co = vec_madd(m_el[1].mVec, vec_sld(coxyz, m_el[2].mVec, 8), btZeroVec());
	vec_float4 cooxyz = vec_sld(co, co, 12);
	co = vec_sld(cooxyz, co, 8);
	vec_float4 tmp = vec_madd(m_el[1].mVec, czxy, btZeroVec());
	cooxyz = vec_sld(tmp, tmp, 12);
	co = vec_sub(co, vec_sld(cooxyz, tmp, 12));

	tmp = vec_madd(m_el[0].mVec, co, btZeroVec());
	vec_float4 s = vec_re(vec_splat(vec_add(vec_add(tmp, vec_sld(tmp, tmp, 4)), vec_sld(tmp, tmp, 8)), 0));
	co = vec_madd(co, s, btZeroVec());	

	vec_float4 ehb = vec_madd(m_el[0].mVec, czxy, btZeroVec());
	tmp = vec_sld(m_el[1].mVec, m_el[1].mVec, 12);	
	vec_float4 icf = vec_madd(m_el[0].mVec, vec_sld(tmp, m_el[1].mVec, 8), btZeroVec());

	tmp = vec_sld(m_el[0].mVec, m_el[0].mVec, 12); 
	ehb = vec_madd(s, vec_sub(ehb, vec_madd(m_el[2].mVec, vec_sld(tmp, m_el[0].mVec, 12), btZeroVec())), btZeroVec());	
	icf = vec_madd(s, vec_sub(icf, vec_madd(m_el[1].mVec, vec_sld(tmp, m_el[0].mVec, 8), btZeroVec())), btZeroVec());	

	
	vec_float4 cfxi = vec_sld(icf, icf, 4);
	vec_float4 bxeh = vec_sld(ehb, ehb, 8);

	return btMatrix3x3(vec_vmrghw(vec_vmrghw(co, cfxi), bxeh),
						vec_vmrglw(vec_vmrghw(co, cfxi), bxeh),
						vec_vmrghw(vec_vmrglw(co, vec_sld(icf, icf, 8)), vec_sld(ehb, ehb, 4)));
# 935 "..\\..\\src\\LinearMath/btMatrix3x3.h"
}

inline __attribute__((always_inline)) btMatrix3x3 
btMatrix3x3::transposeTimes(const btMatrix3x3& m) const
{

	
	vec_float4 aei, dhc, gbf;
	vec_float4 tmp;
	aei = dhc = gbf = btZeroVec();

	for(int i = 0; i < 3; ++i)
	{
		aei = vec_madd(m_el[i].mVec, m.m_el[i].mVec, aei);

		vec_float4 oxyz = vec_sld(m_el[i].mVec, m_el[i].mVec, 12);	
		tmp = vec_sld(oxyz, m_el[i].mVec, 4);								
		
		dhc = vec_madd(vec_sld(tmp, tmp, 4), m.m_el[i].mVec, dhc);			
		gbf = vec_madd(vec_vmrghw(vec_vmrglw(m_el[i].mVec, oxyz), m_el[i].mVec), m.m_el[i].mVec, gbf);
	}
	
	tmp = vec_sld(gbf, gbf, 8);
	return btMatrix3x3(vec_vmrghw(vec_vmrghw(aei, vec_sld(dhc, dhc, 8)), vec_sld(gbf, gbf, 4)),
							vec_vmrghw(vec_vmrghw(dhc, tmp), vec_sld(aei, aei, 4)),
							vec_vmrghw(vec_vmrglw(tmp, aei), vec_sld(dhc, dhc, 4)));
# 973 "..\\..\\src\\LinearMath/btMatrix3x3.h"
}

inline __attribute__((always_inline)) btMatrix3x3 
btMatrix3x3::timesTranspose(const btMatrix3x3& m) const
{

	
	btMatrix3x3 mxtmp;
	btMatrix3x3 ret;
	for(int i = 0; i < 3; ++i)
	{
		mxtmp = btMatrix3x3(vec_madd(m_el[i].mVec, m.m_el[0].mVec, btZeroVec()),
											vec_madd(m_el[i].mVec, m.m_el[1].mVec, btZeroVec()),
											vec_madd(m_el[i].mVec, m.m_el[2].mVec, btZeroVec()));

		mxtmp = mxtmp.transpose();
	
		ret.m_el[i].mVec = vec_add(vec_add(mxtmp.m_el[0].mVec, mxtmp.m_el[1].mVec), mxtmp.m_el[2].mVec);
	}
	
	return ret;
# 1000 "..\\..\\src\\LinearMath/btMatrix3x3.h"
}

inline __attribute__((always_inline)) btVector3 
operator*(const btMatrix3x3& m, const btVector3& v) 
{

	
	btMatrix3x3 tmp;
	tmp[0] = vec_madd(m[0].mVec, v.mVec, btZeroVec());
	tmp[1] = vec_madd(m[1].mVec, v.mVec, btZeroVec());
	tmp[2] = vec_madd(m[2].mVec, v.mVec, btZeroVec());

	tmp = tmp.transpose();

	return vec_add(vec_add(tmp[0].mVec, tmp[1].mVec), tmp[2].mVec);



}

inline __attribute__((always_inline)) btVector3
operator*(const btVector3& v, const btMatrix3x3& m)
{

	
	btMatrix3x3 mt = m.transpose();

	btMatrix3x3 back = btMatrix3x3(
						vec_madd(mt[0].mVec, v.mVec, btZeroVec()),
						vec_madd(mt[1].mVec, v.mVec, btZeroVec()),
						vec_madd(mt[2].mVec, v.mVec, btZeroVec()));

	mt = back.transpose();
	return btVector3(vec_add(mt[2].mVec, vec_add(mt[0].mVec, mt[1].mVec)));



}

inline __attribute__((always_inline)) btMatrix3x3 
operator*(const btMatrix3x3& m1, const btMatrix3x3& m2)
{

	









 
	btMatrix3x3 mt = m1.transpose();
	
	

 
	vec_float4 aei = vec_madd(m2[2].mVec, mt[2].mVec, vec_madd(m2[1].mVec, mt[1].mVec, vec_madd(m2[0].mVec, mt[0].mVec, btZeroVec())));	

	vec_float4 tmp3;
	vec_float4 bdg = btZeroVec();
	vec_float4 cfh = btZeroVec();
	for(int i = 0; i < 3; ++i)
	{
		tmp3 = vec_vmrghw(m2[i].mVec, m2[i].mVec);
		bdg = vec_madd(vec_sld(tmp3, tmp3, 12), mt[i].mVec, bdg); 

		tmp3 = vec_sld(m2[i].mVec, m2[i].mVec, 4);
		tmp3 = vec_vmrghw(tmp3, tmp3);
		cfh = vec_madd(vec_sld(tmp3, tmp3, 8), mt[i].mVec, cfh);	
	}
	
	
	tmp3 = vec_vmrghw(aei, cfh);	
	vec_float4 tmp = vec_vmrghw(tmp3, bdg);
	
	return btMatrix3x3(
		tmp,
		vec_sld(tmp, vec_sld(tmp3, tmp3, 8), 12),
		vec_vmrglw(vec_sld(aei, vec_vmrglw(bdg, aei), 8), cfh));
# 1088 "..\\..\\src\\LinearMath/btMatrix3x3.h"
}














 


 
inline __attribute__((always_inline)) bool operator==(const btMatrix3x3& m1, const btMatrix3x3& m2)
{
# 1116 "..\\..\\src\\LinearMath/btMatrix3x3.h"
	return ( m1[0][0] == m2[0][0] && m1[1][0] == m2[1][0] && m1[2][0] == m2[2][0] &&
		m1[0][1] == m2[0][1] && m1[1][1] == m2[1][1] && m1[2][1] == m2[2][1] &&
		m1[0][2] == m2[0][2] && m1[1][2] == m2[1][2] && m1[2][2] == m2[2][2] );

}



struct	btMatrix3x3FloatData
{
	
	btVector3 m_el[3];
};
# 1135 "..\\..\\src\\LinearMath/btMatrix3x3.h"


struct	btMatrix3x3DoubleData
{
	btVector3DoubleData m_el[3];
};


	
inline __attribute__((always_inline))	void	btMatrix3x3::serialize(struct	btMatrix3x3FloatData& dataOut) const
{
	for (int i=0;i<3;i++)

		dataOut.m_el[i] = m_el[i];



}

inline __attribute__((always_inline))	void	btMatrix3x3::serializeFloat(struct	btMatrix3x3FloatData& dataOut) const
{
	for (int i=0;i<3;i++)

		dataOut.m_el[i] = m_el[i];



}


inline __attribute__((always_inline))	void	btMatrix3x3::deSerialize(const struct	btMatrix3x3FloatData& dataIn)
{
	for (int i=0;i<3;i++)

		m_el[i] = dataIn.m_el[i];



}

inline __attribute__((always_inline))	void	btMatrix3x3::deSerializeFloat(const struct	btMatrix3x3FloatData& dataIn)
{
	for (int i=0;i<3;i++)

		m_el[i] = dataIn.m_el[i];



}

inline __attribute__((always_inline))	void	btMatrix3x3::deSerializeDouble(const struct	btMatrix3x3DoubleData& dataIn)
{




	((0) ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btMatrix3x3.h" ":" "1191" " " "0", 0));


}


# 20 "..\\..\\src\\LinearMath/btTransform.h"











 
class btTransform {
	
  
	btMatrix3x3 m_basis;
  
	btVector3   m_origin;

public:
	
   
	btTransform() {}
  

 
	explicit inline __attribute__((always_inline)) btTransform(const btQuaternion& q, 

		const btVector3& c = btVector3(btZeroVec())) 



		: m_basis(q),
		m_origin(c)
	{}

  

 
	explicit inline __attribute__((always_inline)) btTransform(const btMatrix3x3& b, 

		const btVector3& c = btVector3(btZeroVec())) 



		: m_basis(b),
		m_origin(c)
	{}
	explicit inline __attribute__((always_inline)) btTransform(const btVector3& c)
		: m_basis(),
		m_origin(c)
	{
		m_basis.setIdentity();
	}
   
	inline __attribute__((always_inline)) btTransform (const btTransform& other)
		: m_basis(other.m_basis),
		m_origin(other.m_origin)
	{
	}
   
	inline __attribute__((always_inline)) btTransform& operator=(const btTransform& other)
	{
		m_basis = other.m_basis;
		m_origin = other.m_origin;
		return *this;
	}


  


 
		inline __attribute__((always_inline)) void mult(const btTransform& t1, const btTransform& t2) {
			m_basis = t1.m_basis * t2.m_basis;
			m_origin = t1(t2.m_origin);
		}






 

 
	inline __attribute__((always_inline)) btVector3 operator()(const btVector3& x) const
	{

		
		return btVector3(vec_add(vec_dot3mx(m_basis.get128ptr(), x.mVec), m_origin.mVec));





	}

   
	inline __attribute__((always_inline)) btVector3 operator*(const btVector3& x) const
	{
		return (*this)(x);
	}

   
	inline __attribute__((always_inline)) btQuaternion operator*(const btQuaternion& q) const
	{
		return getRotation() * q;
	}

   
	inline __attribute__((always_inline)) btMatrix3x3&       getBasis()          { return m_basis; }
   
	inline __attribute__((always_inline)) const btMatrix3x3& getBasis()    const { return m_basis; }

   
	inline __attribute__((always_inline)) btVector3&         getOrigin()         { return m_origin; }
   
	inline __attribute__((always_inline)) const btVector3&   getOrigin()   const { return m_origin; }

   
	btQuaternion getRotation() const { 
		btQuaternion q;
		m_basis.getRotation(q);
		return q;
	}
	
# 167 "..\\..\\src\\LinearMath/btTransform.h"

  
 
	inline __attribute__((always_inline)) void setOrigin(const btVector3& origin) 
	{ 
		m_origin = origin;
	}

	inline __attribute__((always_inline)) btVector3 invXform(const btVector3& inVec) const;


   
	inline __attribute__((always_inline)) void setBasis(const btMatrix3x3& basis)
	{ 
		m_basis = basis;
	}

   
	inline __attribute__((always_inline)) void setRotation(const btQuaternion& q)
	{
		m_basis.setRotation(q);
	}


   
	void setIdentity()
	{

		
		m_basis.setIdentity();
		m_origin = btVector3(btZeroVec());





	}

  
 
	btTransform& operator*=(const btTransform& t) 
	{
		m_origin += m_basis * t.m_origin;
		m_basis *= t.m_basis;
		return *this;
	}

   
	btTransform inverse() const
	{ 
		btMatrix3x3 inv = m_basis.transpose();
		return btTransform(inv, inv * -m_origin);
	}

  

 
	btTransform inverseTimes(const btTransform& t) const;  

   
	btTransform operator*(const btTransform& t) const;

   
	static const btTransform&	getIdentity()
	{
		static const btTransform identityTransform(btMatrix3x3::getIdentity());
		return identityTransform;
	}

	void	serialize(struct	btTransformFloatData& dataOut) const;

	void	serializeFloat(struct	btTransformFloatData& dataOut) const;

	void	deSerialize(const struct	btTransformFloatData& dataIn);

	void	deSerializeDouble(const struct	btTransformDoubleData& dataIn);

	void	deSerializeFloat(const struct	btTransformFloatData& dataIn);
};


inline __attribute__((always_inline)) btVector3
btTransform::invXform(const btVector3& inVec) const
{
	btVector3 v = inVec - m_origin;
	return (m_basis.transpose() * v);
}

inline __attribute__((always_inline)) btTransform 
btTransform::inverseTimes(const btTransform& t) const  
{

	
	btVector3 v = vec_sub(t.getOrigin().mVec, m_origin.mVec);



	return btTransform(m_basis.transposeTimes(t.m_basis),
			v * m_basis);
}

inline __attribute__((always_inline)) btTransform 
btTransform::operator*(const btTransform& t) const
{
	return btTransform(m_basis * t.m_basis, 
		(*this)(t.m_origin));
}

 
inline __attribute__((always_inline)) bool operator==(const btTransform& t1, const btTransform& t2)
{
   return ( t1.getBasis()  == t2.getBasis() &&
            t1.getOrigin() == t2.getOrigin() );
}



struct	btTransformFloatData
{
	btMatrix3x3FloatData	m_basis;
	btVector3FloatData	m_origin;
};

struct	btTransformDoubleData
{
	btMatrix3x3DoubleData	m_basis;
	btVector3DoubleData	m_origin;
};


inline __attribute__((always_inline))	void	btTransform::serialize(btTransformFloatData& dataOut) const
{
	m_basis.serialize(dataOut.m_basis);
	m_origin.serialize(dataOut.m_origin);
}

inline __attribute__((always_inline))	void	btTransform::serializeFloat(btTransformFloatData& dataOut) const
{
	m_basis.serializeFloat(dataOut.m_basis);
	m_origin.serializeFloat(dataOut.m_origin);
}


inline __attribute__((always_inline))	void	btTransform::deSerialize(const btTransformFloatData& dataIn)
{
	m_basis.deSerialize(dataIn.m_basis);
	m_origin.deSerialize(dataIn.m_origin);
}

inline __attribute__((always_inline))	void	btTransform::deSerializeFloat(const btTransformFloatData& dataIn)
{
	m_basis.deSerializeFloat(dataIn.m_basis);
	m_origin.deSerializeFloat(dataIn.m_origin);
}

inline __attribute__((always_inline))	void	btTransform::deSerializeDouble(const btTransformDoubleData& dataIn)
{
	m_basis.deSerializeDouble(dataIn.m_basis);
	m_origin.deSerializeDouble(dataIn.m_origin);
}

# 21 "..\\..\\src\\LinearMath/btAabbUtil2.h"




	const vec_int4 btAabbMinus = {1, 2, 4, 0};
	const vec_int4 btAabbPlus = {8, 0x10, 0x20, 0};



inline __attribute__((always_inline)) void AabbExpand (btVector3& aabbMin,
								   btVector3& aabbMax,
								   const btVector3& expansionMin,
								   const btVector3& expansionMax)
{
	aabbMin = aabbMin + expansionMin;
	aabbMax = aabbMax + expansionMax;
}


inline __attribute__((always_inline)) bool TestPointAgainstAabb2(const btVector3 &aabbMin1, const btVector3 &aabbMax1,
								const btVector3 &point)
{	

	
	vec_int4 cond = (vec_int4)vec_or(vec_cmpgt(aabbMin1.mVec, point.mVec), vec_cmplt(aabbMax1.mVec, point.mVec));
	
	cond = vec_sel(cond, btIntZero, btSelVec3);
	bool overlap = vec_all_eq(cond, btIntZero);
# 55 "..\\..\\src\\LinearMath/btAabbUtil2.h"
	return overlap;
}



inline __attribute__((always_inline)) bool TestAabbAgainstAabb2(const btVector3 &aabbMin1, const btVector3 &aabbMax1,
								const btVector3 &aabbMin2, const btVector3 &aabbMax2)
{

	
	vec_int4 cond = (vec_int4)vec_or(vec_cmpgt(aabbMin1.mVec, aabbMax2.mVec), vec_cmplt(aabbMax1.mVec, aabbMin2.mVec));
	
	cond = vec_sel(cond, btIntZero, btSelVec3);
	bool overlap = vec_all_eq(cond, btIntZero);
# 75 "..\\..\\src\\LinearMath/btAabbUtil2.h"
	return overlap;
}


inline __attribute__((always_inline)) bool TestTriangleAgainstAabb2(const btVector3 *vertices,
									const btVector3 &aabbMin, const btVector3 &aabbMax)
{
	const btVector3 &p1 = vertices[0];
	const btVector3 &p2 = vertices[1];
	const btVector3 &p3 = vertices[2];


	
	vec_int4 cond = (vec_int4)vec_or(vec_cmpgt(vec_min(vec_min(p1.mVec, p2.mVec), p3.mVec), aabbMax.mVec), vec_cmplt(vec_max(vec_max(p1.mVec, p2.mVec), p3.mVec), aabbMin.mVec));
	return vec_all_eq(vec_sel(cond, btIntZero, btSelVec3), btIntZero);
# 101 "..\\..\\src\\LinearMath/btAabbUtil2.h"
}

inline __attribute__((always_inline)) int	btOutcode(const btVector3& p,const btVector3& halfExtent) 
{
# 112 "..\\..\\src\\LinearMath/btAabbUtil2.h"
	return (p.getX()  < -halfExtent.getX() ? 0x01 : 0x0) |    
		   (p.getX() >  halfExtent.getX() ? 0x08 : 0x0) |
		   (p.getY() < -halfExtent.getY() ? 0x02 : 0x0) |    
		   (p.getY() >  halfExtent.getY() ? 0x10 : 0x0) |
		   (p.getZ() < -halfExtent.getZ() ? 0x4 : 0x0) |    
		   (p.getZ() >  halfExtent.getZ() ? 0x20 : 0x0);

}



inline __attribute__((always_inline)) bool btRayAabb2(const btVector3& rayFrom,
								  const btVector3& rayInvDirection,

								  const vec_bint4 raySign,
								  const btVector3* bounds,
								  
								  vec_float4 lambda_min,
								  vec_float4 lambda_max)
# 138 "..\\..\\src\\LinearMath/btAabbUtil2.h"
{

	
	btVector3 t0 = (bounds[0] - rayFrom) * rayInvDirection;
	btVector3 t1 = (bounds[1] - rayFrom) * rayInvDirection;
	
	
	vec_float4 bmin = vec_sel(t0.mVec, t1.mVec, raySign);
	vec_float4 bmax = vec_sel(t1.mVec, t0.mVec, raySign);
	
	
	
	
	
	
	vec_float4 xdef = vec_sld(bmax, bmax, 12);
	vec_bint4 cond1 = vec_and(vec_cmplt(bmin, vec_sld(xdef, bmax, 8)), vec_cmplt(bmin, vec_sld(xdef, bmax, 12)));
	vec_bint4 cond2 = vec_and(vec_cmplt(bmin, lambda_max), vec_cmpgt(bmax, lambda_min));
	bool vecret = vec_all_ne(vec_sel((vec_uint4)vec_and(cond1, cond2), btSelVec3, btSelVec3), (vec_uint4)btIntZero);

	return vecret;
# 186 "..\\..\\src\\LinearMath/btAabbUtil2.h"
}

inline __attribute__((always_inline)) bool btRayAabb(const btVector3& rayFrom, 
								 const btVector3& rayTo, 
								 const btVector3& aabbMin, 
								 const btVector3& aabbMax,
					  btScalar& param, btVector3& normal) 
{
	btVector3 aabbHalfExtent = (aabbMax-aabbMin)* btScalar(0.5);
	btVector3 aabbCenter = (aabbMax+aabbMin)* btScalar(0.5);
	btVector3	source = rayFrom - aabbCenter;
	btVector3	target = rayTo - aabbCenter;
	

	
	
	vec_float4 minusHalf = vec_sub(btZeroVec(), aabbHalfExtent.mVec);
	vec_int4 sMinRes = vec_sel((vec_int4)vec_cmplt(source.mVec, minusHalf), btIntZero, btSelVec3);	
	vec_bint4 sPlusRes = vec_cmpgt(source.mVec, aabbHalfExtent.mVec); 
	
	vec_int4 tMinRes = vec_sel((vec_int4)vec_cmplt(target.mVec, minusHalf), btIntZero, btSelVec3);
	vec_bint4 tPlusRes = vec_cmpgt(target.mVec, aabbHalfExtent.mVec); 
	
	
	if(vec_all_eq(vec_and(sMinRes, tMinRes), btIntZero) &&	vec_all_eq(vec_sel((vec_int4)vec_and(sPlusRes, tPlusRes), btIntZero, btSelVec3), btIntZero))
	{
		btScalar lambda_enter = 0;
		btScalar lambda_exit  = param;
		btVector3 r = target - source;
		int i;
		btScalar	normSign = 1;	
		btVector3	hitNormal = btZeroVec();	
		int bit=1;

		if(!(vec_all_eq(sMinRes, btIntZero), vec_all_eq(vec_sel(tMinRes, btIntZero, btSelVec3), btIntZero)))
		{
			btVector3 lambdaPlus = (-source - aabbHalfExtent) / r;
			btVector3 lambdaMinus = (-source + aabbHalfExtent) / r;
			
			
			vec_float4 lpcut = vec_max(btZeroVec(), vec_sel(btZeroVec(), lambdaPlus.mVec, (vec_bint4)sMinRes));	
			vec_float4 lmcut = vec_max(btZeroVec(), vec_sel(btZeroVec(), lambdaMinus.mVec, (vec_bint4)sMinRes));
			
			
			if(vec_any_ne(sMinRes, btIntZero))
			{
				
				lpcut = vec_sel(lpcut, btZeroVec(), btSelVec3);	
				lmcut = vec_sel(lmcut, btZeroVec(), btSelVec3);	
				
				
				btVector3 maxlambda = vec_max(lpcut, lmcut);
				vec_bint4 xy_yz_z0 = vec_cmpgt(maxlambda.mVec, vec_sld(maxlambda.mVec, maxlambda.mVec, 4));
				vec_bint4 xyz_yz0_z0x = vec_and(xy_yz_z0, vec_cmpgt(maxlambda.mVec, vec_sld(maxlambda.mVec, maxlambda.mVec, 8)));
				vec_bint4 xyz0_yz0x_z0xy = vec_and(xyz_yz0_z0x, vec_cmpgt(maxlambda.mVec, vec_sld(maxlambda.mVec, maxlambda.mVec, 12)));	
				hitNormal.mVec = vec_sel(btZeroVec(), vec_float4(1.f), xyz0_yz0x_z0xy);
				vec_bint4 lambdaMask = vec_cmpge(lmcut, lpcut);
				hitNormal.mVec = vec_sel(hitNormal.mVec, vec_sub(btZeroVec(), hitNormal.mVec), lambdaMask);
				
				float *tmp = maxlambda.m_floats;
				
				lambda_enter = (tmp[0] < tmp[1]) ? ((tmp[1] < tmp[2]) ? tmp[2] : tmp[1]) : ((tmp[0] < tmp[2]) ? tmp[2] : tmp[0]);
			}
			
			else if(vec_any_ne(tMinRes, btIntZero))
			{
				btVector3 minlambda = vec_min(lpcut, lmcut);
				float *tmp = minlambda.m_floats;
				
				lambda_exit = (tmp[0] > tmp[1]) ? ((tmp[1] > tmp[2]) ? tmp[2] : tmp[1]) : ((tmp[0] > tmp[2]) ? tmp[2] : tmp[0]);
			}
		}
# 295 "..\\..\\src\\LinearMath/btAabbUtil2.h"
		if (lambda_enter <= lambda_exit)
		{
			param = lambda_enter;
			normal = hitNormal;
			return true;
		}
	}
	return false;
}



inline __attribute__((always_inline))	void btTransformAabb(const btVector3& halfExtents, btScalar margin,const btTransform& t,btVector3& aabbMinOut,btVector3& aabbMaxOut)
{

	
	vec_float4 halfExtentsWithMargin = vec_add(halfExtents.mVec, vec_float4(margin));
	btMatrix3x3 abs_b = t.getBasis().absolute();  
	btVector3 center = t.getOrigin();

	btVector3 extent = btVector3(vec_dot3mx(abs_b.get128ptr(), halfExtentsWithMargin));

	aabbMinOut = vec_sub(center.mVec, extent.mVec);
	aabbMaxOut = vec_add(center.mVec, extent.mVec);
# 330 "..\\..\\src\\LinearMath/btAabbUtil2.h"
}


inline __attribute__((always_inline))	void btTransformAabb(const btVector3& localAabbMin,const btVector3& localAabbMax, btScalar margin,const btTransform& trans,btVector3& aabbMinOut,btVector3& aabbMaxOut)
{
		((localAabbMin . getX() <= localAabbMax . getX()) ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btAabbUtil2.h" ":" "335" " " "localAabbMin.getX() <= localAabbMax.getX()", 0));
		((localAabbMin . getY() <= localAabbMax . getY()) ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btAabbUtil2.h" ":" "336" " " "localAabbMin.getY() <= localAabbMax.getY()", 0));
		((localAabbMin . getZ() <= localAabbMax . getZ()) ? (void)0 : ::std:: _Assert("..\\..\\src\\LinearMath/btAabbUtil2.h" ":" "337" " " "localAabbMin.getZ() <= localAabbMax.getZ()", 0));

		

		
		vec_float4 half = vec_float4(0.5f);
		vec_float4 localHalfExtents = vec_madd(half, vec_sub(localAabbMax.mVec, localAabbMin.mVec), btZeroVec());
		localHalfExtents = vec_add(localHalfExtents, vec_float4(margin));
		btVector3 localCenter = vec_madd(half, vec_add(localAabbMax.mVec, localAabbMin.mVec), btZeroVec());
		btMatrix3x3 abs_b = trans.getBasis().absolute();  
		vec_float4 center = trans(localCenter).mVec;
		vec_float4 extent = vec_dot3mx(abs_b.get128ptr(), localHalfExtents);

		aabbMinOut = vec_sub(center, extent);
		aabbMaxOut = vec_add(center, extent);
# 366 "..\\..\\src\\LinearMath/btAabbUtil2.h"
}



	
	inline __attribute__((always_inline)) unsigned testQuantizedAabbAgainstQuantizedAabb(const unsigned short int* aabbMin1,const unsigned short int* aabbMax1,const unsigned short int* aabbMin2,const unsigned short int* aabbMax2)
	{		
		return static_cast<unsigned int>(btSelect((unsigned)((aabbMin1[0] <= aabbMax2[0]) & (aabbMax1[0] >= aabbMin2[0])
			& (aabbMin1[2] <= aabbMax2[2]) & (aabbMax1[2] >= aabbMin2[2])
			& (aabbMin1[1] <= aabbMax2[1]) & (aabbMax1[1] >= aabbMin2[1])),
			1, 0));
	}
# 388 "..\\..\\src\\LinearMath/btAabbUtil2.h"




# 20 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp"
# 1 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"













 




class btSerializer;


# 30 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"




# 43 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"















struct __attribute__ ((aligned (16))) btQuantizedBvhNode
{
	inline __attribute__((always_inline)) void* operator new(size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,60,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"); } inline __attribute__((always_inline)) void operator delete(void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new(size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete(void*, void*) { } inline __attribute__((always_inline)) void* operator new[](size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,60,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"); } inline __attribute__((always_inline)) void operator delete[](void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new[](size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete[](void*, void*) { };

	
	unsigned short int	m_quantizedAabbMin[3];
	unsigned short int	m_quantizedAabbMax[3];
	
	int	m_escapeIndexOrTriangleIndex;

	bool isLeafNode() const
	{
		
		return (m_escapeIndexOrTriangleIndex >= 0);
	}
	int getEscapeIndex() const
	{
		((!isLeafNode()) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h" ":" "75" " " "!isLeafNode()", 0));
		return -m_escapeIndexOrTriangleIndex;
	}
	int	getTriangleIndex() const
	{
		((isLeafNode()) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h" ":" "80" " " "isLeafNode()", 0));
		
		return (m_escapeIndexOrTriangleIndex&~((~0)<<(31-10)));
	}
	int	getPartId() const
	{
		((isLeafNode()) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h" ":" "86" " " "isLeafNode()", 0));
		
		return (m_escapeIndexOrTriangleIndex>>(31-10));
	}
}
;



struct __attribute__ ((aligned (16))) btOptimizedBvhNode
{
	inline __attribute__((always_inline)) void* operator new(size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,97,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"); } inline __attribute__((always_inline)) void operator delete(void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new(size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete(void*, void*) { } inline __attribute__((always_inline)) void* operator new[](size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,97,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"); } inline __attribute__((always_inline)) void operator delete[](void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new[](size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete[](void*, void*) { };

	
	btVector3	m_aabbMinOrg;
	btVector3	m_aabbMaxOrg;

	
	int	m_escapeIndex;

	
	
	int	m_subPart;
	int	m_triangleIndex;
	int	m_padding[5];


};



class __attribute__ ((aligned (16))) btBvhSubtreeInfo
{
public:
	inline __attribute__((always_inline)) void* operator new(size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,120,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"); } inline __attribute__((always_inline)) void operator delete(void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new(size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete(void*, void*) { } inline __attribute__((always_inline)) void* operator new[](size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,120,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"); } inline __attribute__((always_inline)) void operator delete[](void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new[](size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete[](void*, void*) { };

	
	unsigned short int	m_quantizedAabbMin[3];
	unsigned short int	m_quantizedAabbMax[3];
	
	int			m_rootNodeIndex;
	
	int			m_subtreeSize;
	int			m_padding[3];

	btBvhSubtreeInfo()
	{
		
	}


	void	setAabbFromQuantizeNode(const btQuantizedBvhNode& quantizedNode)
	{
		m_quantizedAabbMin[0] = quantizedNode.m_quantizedAabbMin[0];
		m_quantizedAabbMin[1] = quantizedNode.m_quantizedAabbMin[1];
		m_quantizedAabbMin[2] = quantizedNode.m_quantizedAabbMin[2];
		m_quantizedAabbMax[0] = quantizedNode.m_quantizedAabbMax[0];
		m_quantizedAabbMax[1] = quantizedNode.m_quantizedAabbMax[1];
		m_quantizedAabbMax[2] = quantizedNode.m_quantizedAabbMax[2];
	}
}
;


class btNodeOverlapCallback
{
public:
	virtual ~btNodeOverlapCallback() {};

	virtual void processNode(int subPart, int triangleIndex) = 0;
};







typedef btAlignedObjectArray<btOptimizedBvhNode>	NodeArray;
typedef btAlignedObjectArray<btQuantizedBvhNode>	QuantizedNodeArray;
typedef btAlignedObjectArray<btBvhSubtreeInfo>		BvhSubtreeInfoArray;





class __attribute__ ((aligned (16))) btQuantizedBvh
{
public:
	enum btTraversalMode
	{
		TRAVERSAL_STACKLESS = 0,
		TRAVERSAL_STACKLESS_CACHE_FRIENDLY,
		TRAVERSAL_RECURSIVE
	};

protected:


	btVector3			m_bvhAabbMin;
	btVector3			m_bvhAabbMax;
	btVector3			m_bvhQuantization;

	int					m_bulletVersion;	

	int					m_curNodeIndex;
	
	bool				m_useQuantization;



	NodeArray			m_leafNodes;
	NodeArray			m_contiguousNodes;
	QuantizedNodeArray	m_quantizedLeafNodes;
	QuantizedNodeArray	m_quantizedContiguousNodes;
	
	btTraversalMode	m_traversalMode;
	BvhSubtreeInfoArray		m_SubtreeHeaders;

	
	mutable int m_subtreeHeaderCount;

	



	
	
	void	setInternalNodeAabbMin(int nodeIndex, const btVector3& aabbMin)
	{
		if (m_useQuantization)
		{
			quantize(&m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[0] ,aabbMin,0);
		} else
		{
			m_contiguousNodes[nodeIndex].m_aabbMinOrg = aabbMin;

		}
	}
	void	setInternalNodeAabbMax(int nodeIndex,const btVector3& aabbMax)
	{
		if (m_useQuantization)
		{
			quantize(&m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[0],aabbMax,1);
		} else
		{
			m_contiguousNodes[nodeIndex].m_aabbMaxOrg = aabbMax;
		}
	}

	btVector3 getAabbMin(int nodeIndex) const
	{
		if (m_useQuantization)
		{
			return unQuantize(&m_quantizedLeafNodes[nodeIndex].m_quantizedAabbMin[0]);
		}
		
		return m_leafNodes[nodeIndex].m_aabbMinOrg;

	}
	btVector3 getAabbMax(int nodeIndex) const
	{
		if (m_useQuantization)
		{
			return unQuantize(&m_quantizedLeafNodes[nodeIndex].m_quantizedAabbMax[0]);
		} 
		
		return m_leafNodes[nodeIndex].m_aabbMaxOrg;
		
	}

	
	void	setInternalNodeEscapeIndex(int nodeIndex, int escapeIndex)
	{
		if (m_useQuantization)
		{
			m_quantizedContiguousNodes[nodeIndex].m_escapeIndexOrTriangleIndex = -escapeIndex;
		} 
		else
		{
			m_contiguousNodes[nodeIndex].m_escapeIndex = escapeIndex;
		}

	}

	void mergeInternalNodeAabb(int nodeIndex,const btVector3& newAabbMin,const btVector3& newAabbMax) 
	{
		if (m_useQuantization)
		{
			unsigned short int quantizedAabbMin[3];
			unsigned short int quantizedAabbMax[3];
			quantize(quantizedAabbMin,newAabbMin,0);
			quantize(quantizedAabbMax,newAabbMax,1);
			for (int i=0;i<3;i++)
			{
				if (m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[i] > quantizedAabbMin[i])
					m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMin[i] = quantizedAabbMin[i];

				if (m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[i] < quantizedAabbMax[i])
					m_quantizedContiguousNodes[nodeIndex].m_quantizedAabbMax[i] = quantizedAabbMax[i];

			}
		} else
		{
			
			m_contiguousNodes[nodeIndex].m_aabbMinOrg.setMin(newAabbMin);
			m_contiguousNodes[nodeIndex].m_aabbMaxOrg.setMax(newAabbMax);		
		}
	}

	void	swapLeafNodes(int firstIndex,int secondIndex);

	void	assignInternalNodeFromLeafNode(int internalNode,int leafNodeIndex);

protected:

	

	void	buildTree	(int startIndex,int endIndex);

	int	calcSplittingAxis(int startIndex,int endIndex);

	int	sortAndCalcSplittingIndex(int startIndex,int endIndex,int splitAxis);
	
	void	walkStacklessTree(btNodeOverlapCallback* nodeCallback,const btVector3& aabbMin,const btVector3& aabbMax) const;

	void	walkStacklessQuantizedTreeAgainstRay(btNodeOverlapCallback* nodeCallback, const btVector3& raySource, const btVector3& rayTarget, const btVector3& aabbMin, const btVector3& aabbMax, int startNodeIndex,int endNodeIndex) const;
	void	walkStacklessQuantizedTree(btNodeOverlapCallback* nodeCallback,unsigned short int* quantizedQueryAabbMin,unsigned short int* quantizedQueryAabbMax,int startNodeIndex,int endNodeIndex) const;
	void	walkStacklessTreeAgainstRay(btNodeOverlapCallback* nodeCallback, const btVector3& raySource, const btVector3& rayTarget, const btVector3& aabbMin, const btVector3& aabbMax, int startNodeIndex,int endNodeIndex) const;

	
	void	walkStacklessQuantizedTreeCacheFriendly(btNodeOverlapCallback* nodeCallback,unsigned short int* quantizedQueryAabbMin,unsigned short int* quantizedQueryAabbMax) const;

	
	void	walkRecursiveQuantizedTreeAgainstQueryAabb(const btQuantizedBvhNode* currentNode,btNodeOverlapCallback* nodeCallback,unsigned short int* quantizedQueryAabbMin,unsigned short int* quantizedQueryAabbMax) const;

	
	void	walkRecursiveQuantizedTreeAgainstQuantizedTree(const btQuantizedBvhNode* treeNodeA,const btQuantizedBvhNode* treeNodeB,btNodeOverlapCallback* nodeCallback) const;
	



	void	updateSubtreeHeaders(int leftChildNodexIndex,int rightChildNodexIndex);

public:
	
	inline __attribute__((always_inline)) void* operator new(size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,332,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"); } inline __attribute__((always_inline)) void operator delete(void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new(size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete(void*, void*) { } inline __attribute__((always_inline)) void* operator new[](size_t sizeInBytes) { return btAlignedAllocInternal(sizeInBytes,16,332,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"); } inline __attribute__((always_inline)) void operator delete[](void* ptr) { btAlignedFreeInternal(ptr); } inline __attribute__((always_inline)) void* operator new[](size_t, void* ptr) { return ptr; } inline __attribute__((always_inline)) void operator delete[](void*, void*) { };

	btQuantizedBvh();

	virtual ~btQuantizedBvh();

	
	
	void	setQuantizationValues(const btVector3& bvhAabbMin,const btVector3& bvhAabbMax,btScalar quantizationMargin=btScalar(1.0));
	QuantizedNodeArray&	getLeafNodeArray() {			return	m_quantizedLeafNodes;	}
	
	void	buildInternal();
	

	void	reportAabbOverlappingNodex(btNodeOverlapCallback* nodeCallback,const btVector3& aabbMin,const btVector3& aabbMax) const;
	void	reportRayOverlappingNodex (btNodeOverlapCallback* nodeCallback, const btVector3& raySource, const btVector3& rayTarget) const;
	void	reportBoxCastOverlappingNodex(btNodeOverlapCallback* nodeCallback, const btVector3& raySource, const btVector3& rayTarget, const btVector3& aabbMin,const btVector3& aabbMax) const;

		inline __attribute__((always_inline)) void quantize(unsigned short* out, const btVector3& point,int isMax) const
	{
		((m_useQuantization) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h" ":" "352" " " "m_useQuantization", 0));

		((point . getX() <= m_bvhAabbMax . getX()) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h" ":" "354" " " "point.getX() <= m_bvhAabbMax.getX()", 0));
		((point . getY() <= m_bvhAabbMax . getY()) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h" ":" "355" " " "point.getY() <= m_bvhAabbMax.getY()", 0));
		((point . getZ() <= m_bvhAabbMax . getZ()) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h" ":" "356" " " "point.getZ() <= m_bvhAabbMax.getZ()", 0));

		((point . getX() >= m_bvhAabbMin . getX()) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h" ":" "358" " " "point.getX() >= m_bvhAabbMin.getX()", 0));
		((point . getY() >= m_bvhAabbMin . getY()) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h" ":" "359" " " "point.getY() >= m_bvhAabbMin.getY()", 0));
		((point . getZ() >= m_bvhAabbMin . getZ()) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h" ":" "360" " " "point.getZ() >= m_bvhAabbMin.getZ()", 0));

		btVector3 v = (point - m_bvhAabbMin) * m_bvhQuantization;
		
		
		
		if (isMax)
		{
			out[0] = (unsigned short) (((unsigned short)(v.getX()+btScalar(1.)) | 1));
			out[1] = (unsigned short) (((unsigned short)(v.getY()+btScalar(1.)) | 1));
			out[2] = (unsigned short) (((unsigned short)(v.getZ()+btScalar(1.)) | 1));
		} else
		{
			out[0] = (unsigned short) (((unsigned short)(v.getX()) & 0xfffe));
			out[1] = (unsigned short) (((unsigned short)(v.getY()) & 0xfffe));
			out[2] = (unsigned short) (((unsigned short)(v.getZ()) & 0xfffe));
		}

# 411 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h"

	}


	inline __attribute__((always_inline)) void quantizeWithClamp(unsigned short* out, const btVector3& point2,int isMax) const
	{

		((m_useQuantization) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btQuantizedBvh.h" ":" "418" " " "m_useQuantization", 0));

		btVector3 clampedPoint(point2);
		clampedPoint.setMax(m_bvhAabbMin);
		clampedPoint.setMin(m_bvhAabbMax);

		quantize(out,clampedPoint,isMax);

	}
	
	inline __attribute__((always_inline)) btVector3	unQuantize(const unsigned short* vecIn) const
	{
			btVector3	vecOut;
			vecOut.setValue(
			(btScalar)(vecIn[0]) / (m_bvhQuantization.getX()),
			(btScalar)(vecIn[1]) / (m_bvhQuantization.getY()),
			(btScalar)(vecIn[2]) / (m_bvhQuantization.getZ()));
			vecOut += m_bvhAabbMin;
			return vecOut;
	}

	
	void	setTraversalMode(btTraversalMode	traversalMode)
	{
		m_traversalMode = traversalMode;
	}


	inline __attribute__((always_inline)) QuantizedNodeArray&	getQuantizedNodeArray()
	{	
		return	m_quantizedContiguousNodes;
	}


	inline __attribute__((always_inline)) BvhSubtreeInfoArray&	getSubtreeInfoArray()
	{
		return m_SubtreeHeaders;
	}



	
	unsigned calculateSerializeBufferSize() const;

	
	virtual bool serialize(void *o_alignedDataBuffer, unsigned i_dataBufferSize, bool i_swapEndian) const;

	
	static btQuantizedBvh *deSerializeInPlace(void *i_alignedDataBuffer, unsigned int i_dataBufferSize, bool i_swapEndian);

	static unsigned int getAlignmentSerializationPadding();


	
	virtual	int	calculateSerializeBufferSizeNew() const;

	
	virtual	const char*	serialize(void* dataBuffer, btSerializer* serializer) const;

	virtual	void deSerializeFloat(struct btQuantizedBvhFloatData& quantizedBvhFloatData);

	virtual	void deSerializeDouble(struct btQuantizedBvhDoubleData& quantizedBvhDoubleData);




	inline __attribute__((always_inline)) bool isQuantized()
	{
		return m_useQuantization;
	}

private:
	
	
	
	btQuantizedBvh(btQuantizedBvh &other, bool ownsMemory);

}
;


struct	btBvhSubtreeInfoData
{
	int			m_rootNodeIndex;
	int			m_subtreeSize;
	unsigned short m_quantizedAabbMin[3];
	unsigned short m_quantizedAabbMax[3];
};

struct btOptimizedBvhNodeFloatData
{
	btVector3FloatData	m_aabbMinOrg;
	btVector3FloatData	m_aabbMaxOrg;
	int	m_escapeIndex;
	int	m_subPart;
	int	m_triangleIndex;
	char m_pad[4];
};

struct btOptimizedBvhNodeDoubleData
{
	btVector3DoubleData	m_aabbMinOrg;
	btVector3DoubleData	m_aabbMaxOrg;
	int	m_escapeIndex;
	int	m_subPart;
	int	m_triangleIndex;
	char	m_pad[4];
};


struct btQuantizedBvhNodeData
{
	unsigned short m_quantizedAabbMin[3];
	unsigned short m_quantizedAabbMax[3];
	int	m_escapeIndexOrTriangleIndex;
};

struct	btQuantizedBvhFloatData
{
	btVector3FloatData			m_bvhAabbMin;
	btVector3FloatData			m_bvhAabbMax;
	btVector3FloatData			m_bvhQuantization;
	int					m_curNodeIndex;
	int					m_useQuantization;
	int					m_numContiguousLeafNodes;
	int					m_numQuantizedContiguousNodes;
	btOptimizedBvhNodeFloatData	*m_contiguousNodesPtr;
	btQuantizedBvhNodeData		*m_quantizedContiguousNodesPtr;
	btBvhSubtreeInfoData	*m_subTreeInfoPtr;
	int					m_traversalMode;
	int					m_numSubtreeHeaders;
	
};

struct	btQuantizedBvhDoubleData
{
	btVector3DoubleData			m_bvhAabbMin;
	btVector3DoubleData			m_bvhAabbMax;
	btVector3DoubleData			m_bvhQuantization;
	int							m_curNodeIndex;
	int							m_useQuantization;
	int							m_numContiguousLeafNodes;
	int							m_numQuantizedContiguousNodes;
	btOptimizedBvhNodeDoubleData	*m_contiguousNodesPtr;
	btQuantizedBvhNodeData			*m_quantizedContiguousNodesPtr;

	int							m_traversalMode;
	int							m_numSubtreeHeaders;
	btBvhSubtreeInfoData		*m_subTreeInfoPtr;
};


inline __attribute__((always_inline))	int	btQuantizedBvh::calculateSerializeBufferSizeNew() const
{
	return sizeof(btQuantizedBvhFloatData);
}



# 21 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp"




extern int gOverlappingPairs;












 

btMultiSapBroadphase::btMultiSapBroadphase(int  ,btOverlappingPairCache* pairCache)
:m_overlappingPairs(pairCache),
m_optimizedAabbTree(0),
m_ownsPairCache(false),
m_invalidPair(0)
{
	if (!m_overlappingPairs)
	{
		m_ownsPairCache = true;
		void* mem = btAlignedAllocInternal(sizeof(btSortedOverlappingPairCache),16,49,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp");
		m_overlappingPairs = new (mem)btSortedOverlappingPairCache();
	}

	struct btMultiSapOverlapFilterCallback : public btOverlapFilterCallback
	{
		virtual ~btMultiSapOverlapFilterCallback()
		{}
		
		virtual bool	needBroadphaseCollision(btBroadphaseProxy* childProxy0,btBroadphaseProxy* childProxy1) const
		{
			btBroadphaseProxy* multiProxy0 = (btBroadphaseProxy*)childProxy0->m_multiSapParentProxy;
			btBroadphaseProxy* multiProxy1 = (btBroadphaseProxy*)childProxy1->m_multiSapParentProxy;
			
			bool collides = (multiProxy0->m_collisionFilterGroup & multiProxy1->m_collisionFilterMask) != 0;
			collides = collides && (multiProxy1->m_collisionFilterGroup & multiProxy0->m_collisionFilterMask);
	
			return collides;
		}
	};

	void* mem = btAlignedAllocInternal(sizeof(btMultiSapOverlapFilterCallback),16,70,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp");
	m_filterCallback = new (mem)btMultiSapOverlapFilterCallback();

	m_overlappingPairs->setOverlapFilterCallback(m_filterCallback);


}

btMultiSapBroadphase::~btMultiSapBroadphase()
{
	if (m_ownsPairCache)
	{
		m_overlappingPairs->~btOverlappingPairCache();
		btAlignedFreeInternal(m_overlappingPairs);
	}
}


void	btMultiSapBroadphase::buildTree(const btVector3& bvhAabbMin,const btVector3& bvhAabbMax)
{
	m_optimizedAabbTree = new btQuantizedBvh();
	m_optimizedAabbTree->setQuantizationValues(bvhAabbMin,bvhAabbMax);
	QuantizedNodeArray&	nodes = m_optimizedAabbTree->getLeafNodeArray();
	for (int i=0;i<m_sapBroadphases.size();i++)
	{
		btQuantizedBvhNode node;
		btVector3 aabbMin,aabbMax;
		m_sapBroadphases[i]->getBroadphaseAabb(aabbMin,aabbMax);
		m_optimizedAabbTree->quantize(&node.m_quantizedAabbMin[0],aabbMin,0);
		m_optimizedAabbTree->quantize(&node.m_quantizedAabbMax[0],aabbMax,1);
		int partId = 0;
		node.m_escapeIndexOrTriangleIndex = (partId<<(31-10)) | i;
		nodes.push_back(node);
	}
	m_optimizedAabbTree->buildInternal();
}

btBroadphaseProxy*	btMultiSapBroadphase::createProxy(  const btVector3& aabbMin,  const btVector3& aabbMax,int shapeType,void* userPtr, short int collisionFilterGroup,short int collisionFilterMask, unsigned int interactionLayer,btDispatcher* dispatcher,void*  )
{
	

	void* mem = btAlignedAllocInternal(sizeof(btMultiSapProxy),16,111,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp");
	btMultiSapProxy* proxy = new (mem)btMultiSapProxy(aabbMin,  aabbMax,shapeType,userPtr, collisionFilterGroup,collisionFilterMask);
	m_multiSapProxies.push_back(proxy);

	
	setAabb(proxy,aabbMin,aabbMax,dispatcher);
	return proxy;
}

void	btMultiSapBroadphase::destroyProxy(btBroadphaseProxy*  ,btDispatcher*  )
{
	
	((0) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp" ":" "123" " " "0", 0));

}


void	btMultiSapBroadphase::addToChildBroadphase(btMultiSapProxy* parentMultiSapProxy, btBroadphaseProxy* childProxy, btBroadphaseInterface*	childBroadphase)
{
	void* mem = btAlignedAllocInternal(sizeof(btBridgeProxy),16,130,"e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp");
	btBridgeProxy* bridgeProxyRef = new(mem) btBridgeProxy;
	bridgeProxyRef->m_childProxy = childProxy;
	bridgeProxyRef->m_childBroadphase = childBroadphase;
	parentMultiSapProxy->m_bridgeProxies.push_back(bridgeProxyRef);
}


bool boxIsContainedWithinBox(const btVector3& amin,const btVector3& amax,const btVector3& bmin,const btVector3& bmax);
bool boxIsContainedWithinBox(const btVector3& amin,const btVector3& amax,const btVector3& bmin,const btVector3& bmax)
{

	
	vec_float4 tmp = vec_sel(amin.mVec, bmin.mVec, btSelVec3);
	vec_float4 tmp2 = vec_sel(amax.mVec, bmax.mVec, btSelVec3);
	return vec_all_ge(tmp, bmin.mVec) && vec_all_le(tmp2, bmax.mVec);
# 152 "e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp"
}






void	btMultiSapBroadphase::getAabb(btBroadphaseProxy* proxy,btVector3& aabbMin, btVector3& aabbMax ) const
{
	btMultiSapProxy* multiProxy = static_cast<btMultiSapProxy*>(proxy);
	aabbMin = multiProxy->m_aabbMin;
	aabbMax = multiProxy->m_aabbMax;
}

void	btMultiSapBroadphase::rayTest(const btVector3& rayFrom,const btVector3& rayTo, btBroadphaseRayCallback& rayCallback, const btVector3& aabbMin,const btVector3& aabbMax)
{
	for (int i=0;i<m_multiSapProxies.size();i++)
	{
		rayCallback.process(m_multiSapProxies[i]);
	}
}




void	btMultiSapBroadphase::setAabb(btBroadphaseProxy* proxy,const btVector3& aabbMin,const btVector3& aabbMax, btDispatcher* dispatcher)
{
	btMultiSapProxy* multiProxy = static_cast<btMultiSapProxy*>(proxy);
	multiProxy->m_aabbMin = aabbMin;
	multiProxy->m_aabbMax = aabbMax;
	
	


	


	
	struct MyNodeOverlapCallback : public btNodeOverlapCallback
	{
		btMultiSapBroadphase*	m_multiSap;
		btMultiSapProxy*		m_multiProxy;
		btDispatcher*			m_dispatcher;

		MyNodeOverlapCallback(btMultiSapBroadphase* multiSap,btMultiSapProxy* multiProxy,btDispatcher* dispatcher)
			:m_multiSap(multiSap),
			m_multiProxy(multiProxy),
			m_dispatcher(dispatcher)
		{

		}

		virtual void processNode(int  , int broadphaseIndex)
		{
			btBroadphaseInterface* childBroadphase = m_multiSap->getBroadphaseArray()[broadphaseIndex];

			int containingBroadphaseIndex = -1;
			
			for (int i=0;i<m_multiProxy->m_bridgeProxies.size();i++)
			{

				if (m_multiProxy->m_bridgeProxies[i]->m_childBroadphase == childBroadphase)
				{
					containingBroadphaseIndex = i;
					break;
				}
			}
			if (containingBroadphaseIndex<0)
			{
				
				btBroadphaseProxy* childProxy = childBroadphase->createProxy(m_multiProxy->m_aabbMin,m_multiProxy->m_aabbMax,m_multiProxy->m_shapeType,m_multiProxy->m_clientObject,m_multiProxy->m_collisionFilterGroup,m_multiProxy->m_collisionFilterMask, 0xffffffff,m_dispatcher,m_multiProxy);
				m_multiSap->addToChildBroadphase(m_multiProxy,childProxy,childBroadphase);

			}
		}
	};

	MyNodeOverlapCallback	myNodeCallback(this,multiProxy,dispatcher);



	
	if (m_optimizedAabbTree)
		m_optimizedAabbTree->reportAabbOverlappingNodex(&myNodeCallback,aabbMin,aabbMax);

	int i;

	for ( i=0;i<multiProxy->m_bridgeProxies.size();i++)
	{
		btVector3 worldAabbMin,worldAabbMax;
		multiProxy->m_bridgeProxies[i]->m_childBroadphase->getBroadphaseAabb(worldAabbMin,worldAabbMax);
		bool overlapsBroadphase = TestAabbAgainstAabb2(worldAabbMin,worldAabbMax,multiProxy->m_aabbMin,multiProxy->m_aabbMax);
		if (!overlapsBroadphase)
		{
			
			btBridgeProxy* bridgeProxy = multiProxy->m_bridgeProxies[i];

			btBroadphaseProxy* childProxy = bridgeProxy->m_childProxy;
			bridgeProxy->m_childBroadphase->destroyProxy(childProxy,dispatcher);
			
			multiProxy->m_bridgeProxies.swap( i,multiProxy->m_bridgeProxies.size()-1);
			multiProxy->m_bridgeProxies.pop_back();

		}
	}


	








































































 


	
	for ( i=0;i<multiProxy->m_bridgeProxies.size();i++)
	{
		btBridgeProxy* bridgeProxyRef = multiProxy->m_bridgeProxies[i];
		bridgeProxyRef->m_childBroadphase->setAabb(bridgeProxyRef->m_childProxy,aabbMin,aabbMax,dispatcher);
	}

}
bool stopUpdating=false;



class btMultiSapBroadphasePairSortPredicate
{
	public:

		bool operator() ( const btBroadphasePair& a1, const btBroadphasePair& b1 )
		{
				btMultiSapBroadphase::btMultiSapProxy* aProxy0 = a1.m_pProxy0 ? (btMultiSapBroadphase::btMultiSapProxy*)a1.m_pProxy0->m_multiSapParentProxy : 0;
				btMultiSapBroadphase::btMultiSapProxy* aProxy1 = a1.m_pProxy1 ? (btMultiSapBroadphase::btMultiSapProxy*)a1.m_pProxy1->m_multiSapParentProxy : 0;
				btMultiSapBroadphase::btMultiSapProxy* bProxy0 = b1.m_pProxy0 ? (btMultiSapBroadphase::btMultiSapProxy*)b1.m_pProxy0->m_multiSapParentProxy : 0;
				btMultiSapBroadphase::btMultiSapProxy* bProxy1 = b1.m_pProxy1 ? (btMultiSapBroadphase::btMultiSapProxy*)b1.m_pProxy1->m_multiSapParentProxy : 0;

				 return aProxy0 > bProxy0 || 
					(aProxy0 == bProxy0 && aProxy1 > bProxy1) ||
					(aProxy0 == bProxy0 && aProxy1 == bProxy1 && a1.m_algorithm > b1.m_algorithm); 
		}
};


        
void    btMultiSapBroadphase::calculateOverlappingPairs(btDispatcher* dispatcher)
{



	if (!stopUpdating && getOverlappingPairCache()->hasDeferredRemoval())
	{
	
		btBroadphasePairArray&	overlappingPairArray = getOverlappingPairCache()->getOverlappingPairArray();

	

		overlappingPairArray.quickSort(btMultiSapBroadphasePairSortPredicate());

		
	

		overlappingPairArray.resize(overlappingPairArray.size() - m_invalidPair);
		m_invalidPair = 0;

		
		int i;

		btBroadphasePair previousPair;
		previousPair.m_pProxy0 = 0;
		previousPair.m_pProxy1 = 0;
		previousPair.m_algorithm = 0;
		
		
		for (i=0;i<overlappingPairArray.size();i++)
		{
		
			btBroadphasePair& pair = overlappingPairArray[i];

			btMultiSapProxy* aProxy0 = pair.m_pProxy0 ? (btMultiSapProxy*)pair.m_pProxy0->m_multiSapParentProxy : 0;
			btMultiSapProxy* aProxy1 = pair.m_pProxy1 ? (btMultiSapProxy*)pair.m_pProxy1->m_multiSapParentProxy : 0;
			btMultiSapProxy* bProxy0 = previousPair.m_pProxy0 ? (btMultiSapProxy*)previousPair.m_pProxy0->m_multiSapParentProxy : 0;
			btMultiSapProxy* bProxy1 = previousPair.m_pProxy1 ? (btMultiSapProxy*)previousPair.m_pProxy1->m_multiSapParentProxy : 0;

			bool isDuplicate = (aProxy0 == bProxy0) && (aProxy1 == bProxy1);
			
			previousPair = pair;

			bool needsRemoval = false;

			if (!isDuplicate)
			{
				bool hasOverlap = testAabbOverlap(pair.m_pProxy0,pair.m_pProxy1);

				if (hasOverlap)
				{
					needsRemoval = false;
				} else
				{
					needsRemoval = true;
				}
			} else
			{
				
				needsRemoval = true;
				
				((!pair . m_algorithm) ? (void)0 : ::std:: _Assert("e:\\EnforceNG\\bullet-2.76\\src\\BulletCollision\\BroadphaseCollision\\btMultiSapBroadphase.cpp" ":" "427" " " "!pair.m_algorithm", 0));
			}
			
			if (needsRemoval)
			{
				getOverlappingPairCache()->cleanOverlappingPair(pair,dispatcher);

		
		
				pair.m_pProxy0 = 0;
				pair.m_pProxy1 = 0;
				m_invalidPair++;
				gOverlappingPairs--;
			} 
			
		}

	



		
		
		overlappingPairArray.quickSort(btMultiSapBroadphasePairSortPredicate());

		overlappingPairArray.resize(overlappingPairArray.size() - m_invalidPair);
		m_invalidPair = 0;

		
		
	}


}


bool	btMultiSapBroadphase::testAabbOverlap(btBroadphaseProxy* childProxy0,btBroadphaseProxy* childProxy1)
{
	btMultiSapProxy* multiSapProxy0 = (btMultiSapProxy*)childProxy0->m_multiSapParentProxy;
		btMultiSapProxy* multiSapProxy1 = (btMultiSapProxy*)childProxy1->m_multiSapParentProxy;

		return	TestAabbAgainstAabb2(multiSapProxy0->m_aabbMin,multiSapProxy0->m_aabbMax,
			multiSapProxy1->m_aabbMin,multiSapProxy1->m_aabbMax);
		
}


void	btMultiSapBroadphase::printStats()
{













 

}

void btMultiSapBroadphase::resetPool(btDispatcher* dispatcher)
{
	
}
