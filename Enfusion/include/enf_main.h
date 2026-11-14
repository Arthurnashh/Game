//-----------------------------------------------------------------------------
// File: enf_main.h
//
// Desc: Common definitions
//
// Copyright (c) 2000-2005 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MAIN_H
#define ENF_MAIN_H

#ifdef CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
#endif

#include "enf_config.h"
#include "common/enf_types.h"
#include "system/enf_mutex.h"

namespace enf
{
class StaticBuffer;

/** @defgroup System System tools
@{*/

//-----------------------------------------------------------------------------
// Little/Big endian stuff
//-----------------------------------------------------------------------------
ENF_INLINEFUNC float SwapEndian(const float& i)
{
	int in;

	*(float *)&in = i;

	in = (((in >> 24) & 0xffL)|((in >> 8) & 0xff00L) | ((in << 8) & 0xff0000L) | ((in << 24) & 0xff000000L));

	return *(float *)&in;
}

ENF_INLINEFUNC double SwapEndian(double i)
{
	ubyte* data = (ubyte*)&i;
	ubyte d0 = data[0]; data[0] = data[7]; data[7] = d0;
	ubyte d1 = data[1]; data[1] = data[6]; data[6] = d1;
	ubyte d2 = data[2]; data[2] = data[5]; data[5] = d2;
	ubyte d3 = data[3]; data[3] = data[4]; data[4] = d3;
	return *(double*)data;
}

ENF_INLINEFUNC unsigned long SwapEndian(const unsigned long& in) { return (((in >> 24) & 0xffL) | ((in >> 8) & 0xff00L) | ((in << 8) & 0xff0000L) | ((in << 24) & 0xff000000L)); }
ENF_INLINEFUNC uint SwapEndian(const uint& in) { return (((in >> 24) & 0xffL) | ((in >> 8) & 0xff00L) | ((in << 8) & 0xff0000L) | ((in << 24) & 0xff000000L)); }
ENF_INLINEFUNC ushort SwapEndian(const ushort& in) { return (((in >> 8) & 0xff)|((in << 8) & 0xff00)); }
ENF_INLINEFUNC long SwapEndian(const long& in) { return (((in >> 24) & 0xffL) | ((in >> 8) & 0xff00L) | ((in << 8) & 0xff0000L) | ((in << 24) & 0xff000000L)); }
ENF_INLINEFUNC int SwapEndian(const int& in) { return (((in >> 24) & 0xffL) | ((in >> 8) & 0xff00L) | ((in << 8) & 0xff0000L) | ((in << 24) & 0xff000000L)); }
ENF_INLINEFUNC short SwapEndian(const short& in) { return (((in >> 8) & 0xff)|((in << 8) & 0xff00)); }

template<class T> ENF_INLINEFUNC void SwapArray(T* in, size_t size)
{
	for(size_t n = 0; n < size; n++)
		in[n] = SwapEndian(in[n]);
}

#ifdef ENF_XENON

ENF_INLINEFUNC void SwapArray(ushort* in, size_t size)
{
	for(size_t n = 0; n < size; n++)
		in[n] = __loadshortbytereverse(n << 1, in);
}

ENF_INLINEFUNC void SwapArray(uint* in, size_t size)
{
	for(size_t n = 0; n < size; n++)
		in[n] = __loadwordbytereverse(n << 2, in);
}

ENF_INLINEFUNC void SwapArray(float* in, size_t size)
{
	for(size_t n = 0; n < size; n++)
		*(unsigned long*)&in[n] = __loadwordbytereverse(n << 2, in);
}

#endif

#if ENF_INTEGER_BIG_ENDIAN
#define BE_INT(a) (a)
#define LE_INT(a) enf::SwapEndian(a)
#define BE_SHORT(a) (a)
#define LE_SHORT(a) enf::SwapEndian(a)
#define BE_INT_ARRAY(a, size) (a)
#define LE_INT_ARRAY(a, size) enf::SwapArray(a, size)
#define BE_SHORT_ARRAY(a, size) (a)
#define LE_SHORT_ARRAY(a, size) enf::SwapArray(a, size)
#define LE_FLOAT_ARRAY(a, size) enf::SwapArray(a, size)

#else
#define BE_INT(a) enf::SwapEndian(a)
#define LE_INT(a) (a)
#define BE_SHORT(a) enf::SwapEndian(a)
#define LE_SHORT(a) (a)
#define BE_INT_ARRAY(a, size) enf::SwapArray((uint*)a, size)
#define LE_INT_ARRAY(a, size) (a)
#define BE_SHORT_ARRAY(a, size) enf::SwapArray((ushort*)a, size)
#define LE_SHORT_ARRAY(a, size) (a)
#define LE_FLOAT_ARRAY(a, size) (a)
#endif

#if ENF_FLOAT_BIG_ENDIAN
	#define BE_FLOAT(a) (a)
	#define LE_FLOAT(a) enf::SwapEndian(a)
	#define BE_DOUBLE(a) (a)
	#define LE_DOUBLE(a) enf::SwapEndian(a)
#else
	#define BE_FLOAT(a) enf::SwapEndian(a)
	#define LE_FLOAT(a) (a)
	#define BE_DOUBLE(a) enf::SwapEndian(a)
	#define LE_DOUBLE(a) (a)
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//global statistics
struct EngineStats
{
	static uint m_iLinkCount;
	static uint m_iBTraceCount;
	static uint m_iLTraceCount;
	static uint m_iEntityCount;
	static uint m_iLightCount;
	static uint m_iBatchCount;
	static uint m_iBatchBytesIndices;
	static uint m_iBatchBytesVertexes;
	static uint m_iDynamicVert;
	static uint m_iStaticVert;
	static uint m_iDynamicInd;
	static uint m_iStaticInd;
	static uint m_iDecals;

	static uint m_iMeshItemCount;
	static uint m_iModelContextCount;

	static uint m_iParticleCount;
	static uint m_iSpriteCount;
	static uint m_iMeshCount;
	static uint m_iApplyMaterialCount;
	static uint m_iMaterialCount;
	static uint m_iActiveBodies;
	static uint m_iPassiveBodies;

	static void Reset()
	{
		m_iLinkCount = 0;
		m_iBTraceCount = 0;
		m_iLTraceCount = 0;
		m_iEntityCount = 0;
		m_iLightCount = 0;
		m_iBatchCount = 0;
		m_iBatchBytesIndices = 0;
		m_iBatchBytesVertexes = 0;
		m_iMeshItemCount = 0;
		m_iModelContextCount = 0;
		m_iParticleCount = 0;
		m_iSpriteCount = 0;
		m_iMeshCount = 0;
		m_iApplyMaterialCount = 0;
		m_iMaterialCount = 0;
		m_iActiveBodies = 0;
		m_iPassiveBodies = 0;
		m_iDynamicVert = 0;
		m_iStaticVert = 0;
		m_iDynamicInd = 0;
		m_iStaticInd = 0;
		m_iDecals = 0;
	}
};

//-----------------------------------------------------------------------------
// Basic types
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------
enum LogFlags
{
	LOGF_CREATE			= BITF(0),
	LOGF_APPEND			= BITF(1),
	LOGF_INITIALIZED	= BITF(31)
};
enum LogFormat
{
	LOG_FORM_HEXA		= BITF(0),
	LOG_FORM_DEC		= BITF(1),
	LOG_FORM_INTEGER	= BITF(2),
	LOG_FORM_FLOAT		= BITF(3),
};

enum LogLevel
{
	LL_NORMAL,
	LL_DEBUG,
	LL_PROFILE,
	LL_WARNING,
	LL_ERROR,
	LL_FATAL
};

//-----------------------------------------------------------------------
class TLog
{
private:
	void*		LogFile;
	char		Name[256];
	int		Open();
	void		Close();
	uint		m_iFlags;
	CMutex	m_Mutex;

public:
   TLog();
   TLog(const char* name, uint flags);
   ~TLog();
	void TimeStamp();
	bool IsOpen() const;
   void Open(const char* name, uint flags);
	void Printf(const char* text,...);
	void TSPrintf(const char* text,...);
	void operator<<(const char *text);
   void operator<<(int inum);
   void operator<<(float fnum);
   void operator<<(char chr);
   void Dump(const void *data,int num, int size, int row, uint flags);
};

//-----------------------------------------------------------------------
class MainConfig
{
public:
	static bool		m_bEnableScreenshots;
	static bool		m_bEnableSuperScreenshots;
	static bool		m_bEnableVideoGrabber;
	static bool		m_bEnableDebugRender;
	static bool		m_bEnableConsole;

	static bool		m_bShowCrash;
	static bool		m_bShowConsole;
	static LogLevel	m_LogLevel;
	static bool		m_bLogConsole;
	static TLog		m_ConsoleLog;
	static TLog		m_CrashLog;
	static bool		m_bShowConfig;
	static uint		m_iMemMegabytes;

//script
	static int		m_iOptimize;
	static bool		m_bList;
};


//-----------------------------------------------------------------------------
// Error handling
//-----------------------------------------------------------------------------
void InternalError(const char *text,...);
void Error2(const char *type, const char *text,...);
void Error2Log(const char *error);

enum MsgBoxResult
{
	MBR_IDABORT,
	MBR_IDCANCEL,
	MBR_IDCONTINUE,
	MBR_IDIGNORE,
	MBR_IDNO,
	MBR_IDOK,
	MBR_IDRETRY,
	MBR_IDTRYAGAIN,
	MBR_IDYES
};

enum MsgBoxType
{
	MBT_OK,
	MBT_OKCANCEL,
	MBT_RETRYCANCEL,
	MBT_YESNO,
	MBT_YESNOCANCEL,
	MBT_ABORTRETRYIGNORE,
};

PUBLIC_API MsgBoxResult _MessageBox(MsgBoxType type, const char* caption, const char *text);

//-----------------------------------------------------------------------
enum FPUPrecision {FPU_PRECISION_MINIMUM, FPU_PRECISION_MEDIUM, FPU_PRECISION_MAXIMUM};
enum FPURoundMode {FPU_CHOP, FPU_NEAR, FPU_UP, FPU_DOWN};

void SetFPUPrecision(FPUPrecision prec);
void SetFPURoundMode(FPURoundMode mode);

#ifdef ENF_XENON
	#define ENF_CACHE_ZERO_OFF(ptr, off) __dcbz128((off), (ptr))
	#define ENF_CACHE_ZERO(ptr) __dcbz128(0, (ptr))
	#define ENF_CACHE_FLUSH(ptr) __dcbf(0, (ptr))
	#define ENF_CACHE_FLUSH_OFF(ptr, off) __dcbf((off), (ptr))
	#define ENF_PREFETCH_OFF(ptr, off) __dcbt((off), (ptr))
	#define ENF_PREFETCH(ptr) __dcbt(0, (ptr))
#elif defined(ENF_PS3)
	#define ENF_CACHE_ZERO_OFF(ptr, off) __dcbz((off) + (uchar*) (ptr)) 
	#define ENF_CACHE_ZERO(ptr) __dcbz((ptr))
	#define ENF_CACHE_FLUSH(ptr) __dcbf((ptr))
	#define ENF_CACHE_FLUSH_OFF(ptr, off) __dcbf((off) + (uchar*) (ptr))
	#define ENF_PREFETCH_OFF(ptr, off) __dcbt((off) + (uchar*) (ptr))
	#define ENF_PREFETCH(ptr) __dcbt((ptr))
#elif defined(ENF_SSE)
	//TODO: store from cache
	#define ENF_CACHE_ZERO_OFF(ptr, off) 
	#define ENF_CACHE_ZERO(ptr) 
	#define ENF_CACHE_STORE(ptr) 
	#define ENF_CACHE_STORE_OFF(ptr, off) 
	#define ENF_PREFETCH_OFF(ptr, off) _mm_prefetch((char*)(ptr) + (off), _MM_HINT_T0)
	#define ENF_PREFETCH(ptr) _mm_prefetch((char*)(ptr), _MM_HINT_T0)
#else
	#define ENF_CACHE_ZERO_OFF(ptr, off) 
	#define ENF_CACHE_ZERO(ptr) 
	#define ENF_CACHE_STORE(ptr) 
	#define ENF_CACHE_STORE_OFF(ptr, off) 
	#define ENF_PREFETCH_OFF(ptr, off)
	#define ENF_PREFETCH(ptr)
#endif

unsigned long PUBLIC_API InitDbgSymbols();

class PUBLIC_API SysKey
{
public:
	static int SPACE;
	static int ESC;
	static int Fn;
	static int UP;
	static int DOWN;
	static int LEFT;
	static int RIGHT;
	static int LSHIFT;
	static int RSHIFT;
	static int LCTRL;
	static int RCTRL;
	static int LALT;
	static int RALT;
	static int LSUPER;
	static int RSUPER;
	static int TAB;
	static int ENTER;
	static int BACKSPACE;
	static int INSERT;
	static int DEL;
	static int PAGEUP;
	static int PAGEDOWN;
	static int HOME;
	static int END;
	static int KP_n;
	static int KP_DIVIDE;
	static int KP_MULTIPLY;
	static int KP_SUBTRACT;
	static int KP_ADD;
	static int KP_DECIMAL;
	static int KP_EQUAL;
	static int KP_ENTER;
	static int KP_NUM_LOCK;
	static int CAPS_LOCK;
	static int SCROLL_LOCK;
	static int PAUSE;
	static int MENU;
};

//to avoid include windows.h
PUBLIC_API void _OutputDebugString(const char* str);
PUBLIC_API short _GetAsyncKeyState(int vkey);
PUBLIC_API const char* _ImportFromClipboard();
PUBLIC_API void _ExportToClipboard(const char* str);
PUBLIC_API void _OpenURL(const char* urlAddress, const char* param);
void* _LoadLibrary(const char* name);
void* _GetProcAddress(void* module, const char* procname);
void _DebugBreak();


PUBLIC_API int ExceptionHandler(unsigned int code, void* ep);

PUBLIC_API void InitMainThread();
PUBLIC_API bool CheckMainThread();

PUBLIC_API void LogF(LogLevel level, const char *text,...);

//@}
}
#endif //ENF_MAIN_H
