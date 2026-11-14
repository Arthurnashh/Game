//-----------------------------------------------------------------------------
// File: enf_watchman.h
//
// Desc: Debug library
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_WATCHMAN_H
#define ENF_WATCHMAN_H

#include "enf_config.h"
#include "common/enf_types.h"

namespace enf
{
#ifdef ENF_DEVELOPER
//cannot get address of l-value on lvm compiler
#ifndef __MACOSX__
	#define ENF_WATCHMAN
#endif
#endif

	class PUBLIC_API WatchHandler
	{
		const void* ptr;
	public:
		const void* Value() const { return ptr; }
		virtual const char* GetTypeName(const void* ptr) const { return "WatchHandler"; }
		virtual const char* GetObjectName(const void* ptr) const { return ""; }
		virtual const char* GetInfo(const void* ptr) const { return ""; }
		virtual const char* GetValue(const void* ptr) const=0;
		virtual bool IsPtr() const { return false; }
		WatchHandler(const void* p): ptr(p) {}
		WatchHandler(): ptr(NULL) {}
		virtual ~WatchHandler() {}
	};

	class PUBLIC_API WatchPointer: public WatchHandler
	{
	public:
		virtual const char* GetTypeName() const { return "pointer"; }
		virtual const char* GetValue(const void* ptr) const;
		virtual bool IsPtr() const { return true; }
		WatchPointer(const void* p): WatchHandler(p) {}

	};

	class PUBLIC_API WatchParm: public WatchHandler
	{
	protected:
		int type;
		union
		{
			bool Bool;
			int Int;
			float Float;
			double Double;
			short Short;
			unsigned int UnsignedInt;
			unsigned short UnsignedShort;
			const float* VectorPtr;
		};
	public:
		virtual const char* GetTypeName(const void* ptr) const
		{
			static const char* typeNames[] =
			{
				"bool", "int", "float", "double", "short", "unsigned int", "unsigned short", "Vector3"
			};
			return typeNames[type];
		}
		virtual const char* GetValue(const void* ptr) const;
		WatchParm(bool p): WatchHandler(&p), type(0) {Bool = p;}
		WatchParm(int p): WatchHandler(&p), type(1) {Int = p;}
		WatchParm(float p): WatchHandler(&p), type(2) {Float = p;}
		WatchParm(double p): WatchHandler(&p), type(3) {Double = p;}
		WatchParm(short p): WatchHandler(&p), type(4) {Short = p;}
		WatchParm(unsigned int p): WatchHandler(&p), type(5) {UnsignedInt = p;}
		WatchParm(unsigned short p): WatchHandler(&p), type(6) {UnsignedShort = p;}
		WatchParm(const float* p): WatchHandler(p), type(7) {VectorPtr = p;}
	};

	class PUBLIC_API Watchman
	{
	protected:
		static bool m_bEnabled;
		static bool m_bBreakOn;

	public:
		static void Init();
		static void EnableBreak() { m_bBreakOn = true; }
		static bool IsEnabled() { return m_bEnabled; }
		static void Enable() { m_bEnabled = true; }
		static void Disable() { m_bEnabled = false; }
		static void Call(void* thisPtr, const char* method, const char* file, uint line, uint numParam, ...);
		static void Add(const char* file, uint line, void* ptr, WatchHandler* handler);
		static void Remove(const char* file, uint line, void* ptr);
		static void Destroy(const char* file, uint line, void* ptr);
	};

	#define WPARM(v) enf::WatchParm(v)
	#define WPTR(v) enf::WatchPointer(v)
	#define WCLASS(c, v) c(v)

#ifdef ENF_WATCHMAN
	#define ENF_WATCH_ENABLE() enf::Watchman::Enable()
	#define ENF_WATCH_DISABLE() enf::Watchman::Disable()
	#define ENF_WATCH_BREAK() enf::Watchman::EnableBreak()
	#define ENF_WATCH_INFO(ptr) enf::Watchman::Info(__FILE__, __LINE__, ptr, NULL)
	#define ENF_WATCH_INFO_H(ptr, handler) enf::Watchman::Info(__FILE__, __LINE__, ptr, &handler)
	#define ENF_WATCH_ADD(ptr, handler) enf::Watchman::Add(__FILE__, __LINE__, ptr, &handler)
	#define ENF_WATCH_REMOVE(ptr) enf::Watchman::Remove(__FILE__, __LINE__, ptr)
	#define ENF_WATCH_DESTROY(ptr) enf::Watchman::Destroy(__FILE__, __LINE__, ptr)
	#define ENF_WATCH_CALL() enf::Watchman::Call(this, __FUNCTION__, __FILE__, __LINE__, 0);
	#define ENF_WATCH_CALL_1(parm0) enf::Watchman::Call(this, __FUNCTION__, __FILE__, __LINE__, 1, parm0);
	#define ENF_WATCH_CALL_2(parm0, parm1) enf::Watchman::Call(this, __FUNCTION__, __FILE__, __LINE__, 2, parm0, parm1);
	#define ENF_WATCH_CALL_3(parm0, parm1, parm2) enf::Watchman::Call(this, __FUNCTION__, __FILE__, __LINE__, 3, parm0, parm1, parm2);
#else
	#undef ENF_WATCHMAN

	#define ENF_WATCH_ENABLE
	#define ENF_WATCH_DISABLE
	#define ENF_WATCH_INFO(ptr)
	#define ENF_WATCH_INFO_H(ptr, handler)
	#define ENF_WATCH_REGISTER(handler)
	#define ENF_WATCH_ADD(ptr, handler)
	#define ENF_WATCH_REMOVE(ptr)
	#define ENF_WATCH_DESTROY(ptr)
	#define ENF_WATCH_CALL()
	#define ENF_WATCH_CALL_1(parm0)
	#define ENF_WATCH_CALL_2(parm0, parm1)
#endif

}

#endif //ENF_WATCHMAN_H