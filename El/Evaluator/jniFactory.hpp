// Framework for registration of new JNI scripting functions

// USAGE:
/*
class RVDirectories
{
private:
  // implementation
  static RString GetUserDirectory();
  static RString GetOtherUserDirectory(RString name);

  // factory definition
#define FUNCTIONS_RV_DIRECTORIES(Class, XX) \
  XX(Class, "returns the user directory", RString, GetUserDirectory) \
  XX(Class, "returns the directory for given user", RString, GetOtherUserDirectory, RString)

public:
  // registration through factory
  JNI_REGISTER_FUNCTIONS(RVDirectories, FUNCTIONS_RV_DIRECTORIES)
  JNI_EXPORT_FUNCTIONS(RVDirectories, FUNCTIONS_RV_DIRECTORIES)
};

// during initialization
RVDirectories::ExportFunctions(jniSources);
RVDirectories::RegisterFunctions(env);
*/

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _JNI_FACTORY_HPP
#define _JNI_FACTORY_HPP

#include <El/Evaluator/jvmImpl.hpp>

// macro for empty list of functions
#define NO_STATIC_FUNCTIONS(XX)
#define NO_FUNCTIONS(Class, XX)

// handling of C++ <-> Java types conversion
template <typename T>
class JavaTypes;

template <typename NativeType>
typename JavaTypes<NativeType>::JNIType NativeToJava(JNIEnv *env, NativeType native)
{
  return JavaTypes<NativeType>::FromNative(env, native);
}

template <>
class JavaTypes<void>
{
public:
  typedef void JNIType;

  static RString GetJavaType() {return "void";}
  static RString GetSignature() {return "V";}
};
template <>
class JavaTypes<bool>
{
public:
  typedef bool NativeType;
  typedef jboolean JNIType;

  static RString GetJavaType() {return "boolean";}
  static RString GetSignature() {return "Z";}
  static JNIType FromNative(JNIEnv *env, NativeType native)
  {
    return native ? JNI_TRUE : JNI_FALSE; 
  }
  static NativeType ToNative(JNIEnv *env, JNIType jni)
  {
    return jni != JNI_FALSE;
  }
};
template <>
class JavaTypes<int>
{
public:
  typedef int NativeType;
  typedef jint JNIType;

  static RString GetJavaType() {return "int";}
  static RString GetSignature() {return "I";}
  static JNIType FromNative(JNIEnv *env, NativeType native)
  {
    return native; 
  }
  static NativeType ToNative(JNIEnv *env, JNIType jni)
  {
    return jni;
  }
};
template <>
class JavaTypes<float>
{
public:
  typedef float NativeType;
  typedef jfloat JNIType;

  static RString GetJavaType() {return "float";}
  static RString GetSignature() {return "F";}
  static JNIType FromNative(JNIEnv *env, NativeType native)
  {
    return native; 
  }
  static NativeType ToNative(JNIEnv *env, JNIType jni)
  {
    return jni;
  }
};
template <>
class JavaTypes<RString>
{
public:
  typedef RString NativeType;
  typedef jstring JNIType;

  static RString GetJavaType() {return "String";}
  static RString GetSignature() {return "Ljava/lang/String;";}
  static JNIType FromNative(JNIEnv *env, const NativeType &native)
  {
    // activate JVM FPU settings
    SCOPE_FPU_JVM

    return env->NewStringUTF(native); 
  }
  static NativeType ToNative(JNIEnv *env, JNIType jni)
  {
    // activate JVM FPU settings
    SCOPE_FPU_JVM

    int len = env->GetStringUTFLength(jni);
    RString str;
    char *buffer = str.CreateBuffer(len + 1);
    env->GetStringUTFRegion(jni, 0, len, buffer);
    return str;
  }
};
template <>
class JavaTypes< Buffer<char> >
{
public:
  typedef Buffer<char> NativeType;
  typedef jbyteArray JNIType;

  static RString GetJavaType() {return "byte[]";}
  static RString GetSignature() {return "[B";}
  static JNIType FromNative(JNIEnv *env, const NativeType &native)
  {
    // activate JVM FPU settings
    SCOPE_FPU_JVM

    jbyteArray result = env->NewByteArray(native.Size());
    env->SetByteArrayRegion(result, 0, native.Size(), (const jbyte *)native.Data());
    return result;
  }
  static NativeType ToNative(JNIEnv *env, JNIType jni)
  {
    // activate JVM FPU settings
    SCOPE_FPU_JVM

    Buffer<char> result;
    int size = env->GetArrayLength(jni);
    result.Init(size);
    if (size > 0) env->GetByteArrayRegion(jni, 0, size, (jbyte *)result.Data());
    return result;
  }
};

// convert the function name to Java conventions
static RString NameToJava(RString name)
{
  if (name.IsEmpty()) return name;
  char *ptr = name.MutableData();
  *ptr = myLower(*ptr);
  return name;
}

// helper template to handle variant list of arguments
// TODO: support for more arguments when needed
// 4 arguments
template <typename T1=void, typename T2=void, typename T3=void, typename T4=void>
class JavaArgsHelper
{
public:
  // arguments in Java method declaration
  static RString Export()
  {
    return JavaTypes<T1>::GetJavaType() + " arg1, " +
      JavaTypes<T2>::GetJavaType() + " arg2, " +
      JavaTypes<T3>::GetJavaType() + " arg3, " +
      JavaTypes<T4>::GetJavaType() + " arg4";
  }
  // signature of Java method
  static RString GetSignature()
  {
    return JavaTypes<T1>::GetSignature() +
      JavaTypes<T2>::GetSignature() +
      JavaTypes<T3>::GetSignature() +
      JavaTypes<T4>::GetSignature();
  }
  // cast JNI function pointer to void * (select the correct function overload)
  template <typename RetType>
  static void *CastFunction(typename JavaTypes<RetType>::JNIType (JNICALL *func)(
    typename JavaTypes<T1>::JNIType, typename JavaTypes<T2>::JNIType,
    typename JavaTypes<T3>::JNIType, typename JavaTypes<T4>::JNIType))
  {
    return (void *)func;
  }
};
// specialization for 3 arguments
template <typename T1, typename T2, typename T3>
class JavaArgsHelper<T1, T2, T3, void>
{
public:
  static RString Export()
  {
    return JavaTypes<T1>::GetJavaType() + " arg1, " +
      JavaTypes<T2>::GetJavaType() + " arg2, " +
      JavaTypes<T3>::GetJavaType() + " arg3";
  }
  static RString GetSignature()
  {
    return JavaTypes<T1>::GetSignature() +
      JavaTypes<T2>::GetSignature() +
      JavaTypes<T3>::GetSignature();
  }
  template <typename RetType>
  static void *CastFunction(typename JavaTypes<RetType>::JNIType (JNICALL *func)(
    typename JavaTypes<T1>::JNIType, typename JavaTypes<T2>::JNIType,
    typename JavaTypes<T3>::JNIType))
  {
    return (void *)func;
  }
};
// specialization for 2 arguments
template <typename T1, typename T2>
class JavaArgsHelper<T1, T2, void, void>
{
public:
  static RString Export()
  {
    return JavaTypes<T1>::GetJavaType() + " arg1, " +
      JavaTypes<T2>::GetJavaType() + " arg2";
  }
  static RString GetSignature()
  {
    return JavaTypes<T1>::GetSignature() +
      JavaTypes<T2>::GetSignature();
  }
  template <typename RetType>
  static void *CastFunction(typename JavaTypes<RetType>::JNIType (JNICALL *func)(
    typename JavaTypes<T1>::JNIType, typename JavaTypes<T2>::JNIType))
  {
    return (void *)func;
  }
};
// specialization for 1 argument
template <typename T1>
class JavaArgsHelper<T1, void, void, void>
{
public:
  static RString Export()
  {
    return JavaTypes<T1>::GetJavaType() + " arg1";
  }
  static RString GetSignature()
  {
    return JavaTypes<T1>::GetSignature();
  }
  template <typename RetType>
  static void *CastFunction(typename JavaTypes<RetType>::JNIType (JNICALL *func)(JNIEnv *, jobject,
    typename JavaTypes<T1>::JNIType))
  {
    return (void *)func;
  }
};
// specialization for 0 arguments
template <>
class JavaArgsHelper<void, void, void, void>
{
public:
  static RString Export()
  {
    return RString();
  }
  static RString GetSignature()
  {
    return RString();
  }
  template <typename RetType>
  static void *CastFunction(typename JavaTypes<RetType>::JNIType (JNICALL *func)(JNIEnv *, jobject))
  {
    return (void *)func;
  }
};

// Exports Java code for a single method
#define JNI_EXPORT_STATIC_FUNCTION_IMPL(description, rettype, name, ...) \
  out << "  /** \n"; \
  out << description; \
  out << "\n"; \
  out << "   */\n"; \
  \
  out << "  public native static "; \
  out << JavaTypes<rettype>::GetJavaType(); \
  out << " "; \
  out << NameToJava(#name); \
  out << "("; \
  out << JavaArgsHelper<__VA_ARGS__>::Export();\
  out << ");\n";
#define JNI_EXPORT_FUNCTION_IMPL(clstype, description, rettype, retdefault, name, ...) \
  out << "  /** \n"; \
  out << description; \
  out << "\n"; \
  out << "   */\n"; \
  \
  out << "  public native "; \
  out << JavaTypes<rettype>::GetJavaType(); \
  out << " "; \
  out << NameToJava(#name); \
  out << "("; \
  out << JavaArgsHelper<__VA_ARGS__>::Export();\
  out << ");\n";

// Exports Java code for a whole class
#define JNI_EXPORT_FUNCTIONS(clstype, MACRO_STATIC, MACRO) \
  static void ExportFunctions(RString jniSources) \
{ \
  QOFStream out(jniSources + "/" #clstype ".java"); \
  out << "package com.bistudio.JNIScripting;\n\n"; \
  out << "public class " #clstype " implements java.io.Serializable\n"; \
  out << "{\n"; \
  out << "  private static final long serialVersionUID = 1;\n"; \
  out << "  private int _jID = -1;\n"; \
  MACRO_STATIC(JNI_EXPORT_STATIC_FUNCTION_IMPL) \
  MACRO(clstype, JNI_EXPORT_FUNCTION_IMPL) \
  out << "}\n"; \
}

// Creates a JNI function to convert parameters and redirect to the native function
// Function for 0-4 parameters + helper to select the correct one based on number of macro parameters
#define JNI_BRIDGE_STATIC_IMPL(description, rettype, func, ...) \
template <typename R, int DummyPos=0> \
class JNI_##func \
{ \
public: \
  template <typename Dummy, typename T1, typename T2, typename T3, typename T4> \
  static typename JavaTypes<R>::JNIType JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2, \
    typename JavaTypes<T3>::JNIType arg3, \
    typename JavaTypes<T4>::JNIType arg4) \
  { \
    SCOPE_FPU_ENGINE \
    R result = func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2), \
    JavaTypes<T3>::ToNative(env, arg3), JavaTypes<T4>::ToNative(env, arg4)); \
    return NativeToJava(env, result); \
  } \
  template <typename Dummy, typename T1, typename T2, typename T3> \
  static typename JavaTypes<R>::JNIType JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2, \
    typename JavaTypes<T3>::JNIType arg3) \
  { \
    SCOPE_FPU_ENGINE \
    R result = func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2), \
    JavaTypes<T3>::ToNative(env, arg3)); \
    return NativeToJava(env, result); \
  } \
  template <typename Dummy, typename T1, typename T2> \
  static typename JavaTypes<R>::JNIType JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2) \
  { \
    SCOPE_FPU_ENGINE \
    R result = func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2)); \
    return NativeToJava(env, result); \
  } \
  template <typename Dummy, typename T1> \
  static typename JavaTypes<R>::JNIType JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1) \
  { \
    SCOPE_FPU_ENGINE \
    R result = func(JavaTypes<T1>::ToNative(env, arg1)); \
    return NativeToJava(env, result); \
  } \
  template <typename Dummy> \
  static typename JavaTypes<R>::JNIType JNICALL invoke(JNIEnv *env, jobject obj) \
  { \
    SCOPE_FPU_ENGINE \
    R result = func(); \
    return NativeToJava(env, result); \
  } \
}; \
template <int DummyPos> \
class JNI_##func<void, DummyPos> \
{ \
public: \
  template <typename Dummy, typename T1, typename T2, typename T3, typename T4> \
  static void JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2, \
    typename JavaTypes<T3>::JNIType arg3, \
    typename JavaTypes<T4>::JNIType arg4) \
  { \
    SCOPE_FPU_ENGINE \
    func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2), \
      JavaTypes<T3>::ToNative(env, arg3), JavaTypes<T4>::ToNative(env, arg4)); \
  } \
  template <typename Dummy, typename T1, typename T2, typename T3> \
  static void JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2, \
    typename JavaTypes<T3>::JNIType arg3) \
  { \
    SCOPE_FPU_ENGINE \
    func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2), \
      JavaTypes<T3>::ToNative(env, arg3)); \
  } \
  template <typename Dummy, typename T1, typename T2> \
  static void JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2) \
  { \
    SCOPE_FPU_ENGINE \
    func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2)); \
  } \
  template <typename Dummy, typename T1> \
  static void JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1) \
  { \
    SCOPE_FPU_ENGINE \
    func(JavaTypes<T1>::ToNative(env, arg1)); \
  } \
  template <typename Dummy> \
  static void JNICALL invoke(JNIEnv *env, jobject obj) \
  { \
    SCOPE_FPU_ENGINE \
    func(); \
  } \
}; \
static void *GetPtrJNI_##func() \
{ \
  return JavaArgsHelper<__VA_ARGS__>::CastFunction<rettype>(&JNI_##func<rettype>::invoke<void, ##__VA_ARGS__>); \
};

#define JNI_BRIDGE_IMPL(clstype, description, rettype, retdefault, func, ...) \
template <typename R, int DummyPos=0> \
class JNI_##func \
{ \
public: \
  template <typename Dummy, typename T1, typename T2, typename T3, typename T4> \
  static typename JavaTypes<R>::JNIType JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2, \
    typename JavaTypes<T3>::JNIType arg3, \
    typename JavaTypes<T4>::JNIType arg4) \
  { \
    clstype *nativeObj = JavaTypes<clstype>::ToNative(env, obj); \
    if (nativeObj == NULL) return retdefault; \
    SCOPE_FPU_ENGINE \
    R result = nativeObj->func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2), \
      JavaTypes<T3>::ToNative(env, arg3), JavaTypes<T4>::ToNative(env, arg4)); \
    return NativeToJava(env, result); \
  } \
  template <typename Dummy, typename T1, typename T2, typename T3> \
  static typename JavaTypes<R>::JNIType JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2, \
    typename JavaTypes<T3>::JNIType arg3) \
  { \
    clstype *nativeObj = JavaTypes<clstype>::ToNative(env, obj); \
    if (nativeObj == NULL) return retdefault; \
    SCOPE_FPU_ENGINE \
    R result = nativeObj->func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2), \
      JavaTypes<T3>::ToNative(env, arg3)); \
    return NativeToJava(env, result); \
  } \
  template <typename Dummy, typename T1, typename T2> \
  static typename JavaTypes<R>::JNIType JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2) \
  { \
    clstype *nativeObj = JavaTypes<clstype>::ToNative(env, obj); \
    if (nativeObj == NULL) return retdefault; \
    SCOPE_FPU_ENGINE \
    R result = nativeObj->func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2)); \
    return NativeToJava(env, result); \
  } \
  template <typename Dummy, typename T1> \
  static typename JavaTypes<R>::JNIType JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1) \
  { \
    clstype *nativeObj = JavaTypes<clstype>::ToNative(env, obj); \
    if (nativeObj == NULL) return retdefault; \
    SCOPE_FPU_ENGINE \
    R result = nativeObj->func(JavaTypes<T1>::ToNative(env, arg1)); \
    return NativeToJava(env, result); \
  } \
  template <typename Dummy> \
  static typename JavaTypes<R>::JNIType JNICALL invoke(JNIEnv *env, jobject obj) \
  { \
    clstype *nativeObj = JavaTypes<clstype>::ToNative(env, obj); \
    if (nativeObj == NULL) return retdefault; \
    SCOPE_FPU_ENGINE \
    R result = nativeObj->func(); \
    return NativeToJava(env, result); \
  } \
}; \
template <int DummyPos> \
class JNI_##func<void, DummyPos> \
{ \
public: \
  template <typename Dummy, typename T1, typename T2, typename T3, typename T4> \
  static void JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2, \
    typename JavaTypes<T3>::JNIType arg3, \
    typename JavaTypes<T4>::JNIType arg4) \
  { \
    clstype *nativeObj = JavaTypes<clstype>::ToNative(env, obj); \
    if (nativeObj == NULL) return; \
    SCOPE_FPU_ENGINE \
    nativeObj->func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2), \
      JavaTypes<T3>::ToNative(env, arg3), JavaTypes<T4>::ToNative(env, arg4)); \
  } \
  template <typename Dummy, typename T1, typename T2, typename T3> \
  static void JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2, \
    typename JavaTypes<T3>::JNIType arg3) \
  { \
    clstype *nativeObj = JavaTypes<clstype>::ToNative(env, obj); \
    if (nativeObj == NULL) return; \
    SCOPE_FPU_ENGINE \
    nativeObj->func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2), \
      JavaTypes<T3>::ToNative(env, arg3)); \
  } \
  template <typename Dummy, typename T1, typename T2> \
  static void JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1, \
    typename JavaTypes<T2>::JNIType arg2) \
  { \
    clstype *nativeObj = JavaTypes<clstype>::ToNative(env, obj); \
    if (nativeObj == NULL) return; \
    SCOPE_FPU_ENGINE \
    nativeObj->func(JavaTypes<T1>::ToNative(env, arg1), JavaTypes<T2>::ToNative(env, arg2)); \
  } \
  template <typename Dummy, typename T1> \
  static void JNICALL invoke(JNIEnv *env, jobject obj, \
    typename JavaTypes<T1>::JNIType arg1) \
  { \
    clstype *nativeObj = JavaTypes<clstype>::ToNative(env, obj); \
    if (nativeObj == NULL) return; \
    SCOPE_FPU_ENGINE \
    nativeObj->func(JavaTypes<T1>::ToNative(env, arg1)); \
  } \
  template <typename Dummy> \
  static void JNICALL invoke(JNIEnv *env, jobject obj) \
  { \
    clstype *nativeObj = JavaTypes<clstype>::ToNative(env, obj); \
    if (nativeObj == NULL) return; \
    SCOPE_FPU_ENGINE \
    nativeObj->func(); \
  } \
}; \
static void *GetPtrJNI_##func() \
{ \
  return JavaArgsHelper<__VA_ARGS__>::CastFunction<rettype>(&JNI_##func<rettype>::invoke<void, ##__VA_ARGS__>); \
};


// Register a single JNI function
#define JNI_REGISTER_STATIC_FUNCTION_IMPL(description, rettype, func, ...) \
{ \
  JNINativeMethod helper; \
  RString bufName = NameToJava(#func); \
  helper.name = unconst_cast(cc_cast(bufName)); \
  RString bufSignature = "(" + JavaArgsHelper<__VA_ARGS__>::GetSignature() + ")" + JavaTypes<rettype>::GetSignature(); \
  helper.signature = unconst_cast(cc_cast(bufSignature)); \
  helper.fnPtr = GetPtrJNI_##func(); \
  env->RegisterNatives(cls, &helper, 1); \
}
#define JNI_REGISTER_FUNCTION_IMPL(clstype, description, rettype, retdefault, func, ...) \
{ \
  JNINativeMethod helper; \
  RString bufName = NameToJava(#func); \
  helper.name = unconst_cast(cc_cast(bufName)); \
  RString bufSignature = "(" + JavaArgsHelper<__VA_ARGS__>::GetSignature() + ")" + JavaTypes<rettype>::GetSignature(); \
  helper.signature = unconst_cast(cc_cast(bufSignature)); \
  helper.fnPtr = GetPtrJNI_##func(); \
  env->RegisterNatives(cls, &helper, 1); \
}

// Create and register all JNI functions in the class
#define JNI_REGISTER_FUNCTIONS(clstype, MACRO_STATIC, MACRO) \
  MACRO_STATIC(JNI_BRIDGE_STATIC_IMPL) \
  MACRO(clstype, JNI_BRIDGE_IMPL) \
  static bool RegisterFunctions(JNIEnv *env) \
  { \
    CHECK_FPU_JVM \
    jclass cls = env->FindClass("com/bistudio/JNIScripting/" #clstype); \
    if (!cls) return false; \
    MACRO_STATIC(JNI_REGISTER_STATIC_FUNCTION_IMPL) \
    MACRO(clstype, JNI_REGISTER_FUNCTION_IMPL) \
    return true; \
  }

#endif

