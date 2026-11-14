#ifdef _MSC_VER
#pragma once
#endif

#ifndef _JVM_IMPL_HPP
#define _JVM_IMPL_HPP

#include <El/Evaluator/expressImpl.hpp>

// Convert jobject to GameValue
GameValue GameValueFromJObject(JNIEnv *env, jobject from);

// Convert GameValue to NativeObject (used for helper types not supported directly)
jobject GameValueToNativeObject(JNIEnv *env, GameValuePar from);

// Load the Java class using the custom class loader
jclass LoadJClass(JNIEnv *env, RString name);

// Serialization of Java objects
bool SaveJObject(JNIEnv *env, AutoArray<char> &buffer, jobject object);
jobject LoadJObject(JNIEnv *env, const AutoArray<char> &buffer);

class LocalFPUSettings
{
protected:
#ifdef _WIN32
  static const int _mask87 = _MCW_EM | _MCW_IC | _MCW_RC | _MCW_PC;
  static int _jvm87;
  static int _engine87;
  int _old87;
#if USING_SSE
  static int _jvmSSE;
  static int _engineSSE;
  int _oldSSE;
#endif
#endif

public:
  static void InitJVM()
  {
    // store the FPU state
#ifdef _WIN32
    _jvm87 = _control87(0, 0) & _mask87;
#if USING_SSE
    _jvmSSE = _mm_getcsr();
#endif
#endif
  }
  static void InitEngine()
  {
#ifdef _WIN32
    _engine87 = _control87(0, 0) & _mask87;
#if USING_SSE
    _engineSSE = _mm_getcsr();
#endif
#endif
  }
  static void CheckJVM()
  {
#ifdef _WIN32
    DoAssert(_jvm87 == (_control87(0, 0) & _mask87));
#if USING_SSE
    // ??? sometimes, _MM_EXCEPT_INEXACT and _MM_FLUSH_ZERO_ON differs
    // DoAssert(_jvmSSE == _mm_getcsr());
#endif
#endif
  }
  static void CheckEngine()
  {
#ifdef _WIN32
    DoAssert(_engine87 == (_control87(0, 0) & _mask87));
#if USING_SSE
    DoAssert(_engineSSE == _mm_getcsr());
#endif
#endif
  }
};

#if _DEBUG
# define CHECK_FPU_JVM LocalFPUSettings::CheckJVM();
# define CHECK_FPU_ENGINE LocalFPUSettings::CheckEngine();
#else
# define CHECK_FPU_JVM
# define CHECK_FPU_ENGINE
#endif

class LocalJVMFPU : public LocalFPUSettings
{
public:
  LocalJVMFPU()
  {
    // change
#ifdef _WIN32
    _old87 = _control87(0, 0);
#if USING_SSE
    _oldSSE = _mm_getcsr();
#endif
    _control87(_jvm87, _mask87);
#if USING_SSE
    _mm_setcsr(_jvmSSE);
#endif
#endif
  }
  ~LocalJVMFPU()
  {
    // restore
#ifdef _WIN32
    _control87(_old87, _mask87);
#if USING_SSE
    _mm_setcsr(_oldSSE);
#endif
#endif
  }
};

class LocalEngineFPU : public LocalFPUSettings
{
public:
  LocalEngineFPU()
  {
    // change
#ifdef _WIN32
    _old87 = _control87(0, 0);
#if USING_SSE
    _oldSSE = _mm_getcsr();
#endif
    _control87(_engine87, _mask87);
#if USING_SSE
    _mm_setcsr(_engineSSE);
#endif
#endif
  }
  ~LocalEngineFPU()
  {
    // restore
#ifdef _WIN32
    _control87(_old87, _mask87);
#if USING_SSE
    _mm_setcsr(_oldSSE);
#endif
#endif
  }
};

#define SCOPE_FPU_JVM LocalJVMFPU fpuScope;
#define SCOPE_FPU_ENGINE LocalEngineFPU fpuScope;

// helper for scope of local references of Java objects
class JavaLocalFrame
{
protected:
  JNIEnv *_env;
  jobject *_result;

public:
  JavaLocalFrame(JNIEnv *env, int maxRefs, jobject *result = NULL)
  {
    CHECK_FPU_JVM

    if (env && env->PushLocalFrame(maxRefs) == 0)
    {
      // Push succeeded, call Pop when needed
      _env = env;
      _result = result;
    }
    else
    {
      // failed
      _env = NULL;
      _result = NULL;
    }
  }
  ~JavaLocalFrame()
  {
    CHECK_FPU_JVM

    if (_env)
    {
      if (_result)
        *_result = _env->PopLocalFrame(*_result);
      else
        _env->PopLocalFrame(NULL);
    }
  }
};

#endif