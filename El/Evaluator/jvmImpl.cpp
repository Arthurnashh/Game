#include <El/elementpch.hpp>

#include "express.hpp"
#include "jvmImpl.hpp"

static jweak GFloatClass = NULL;
static jmethodID GFloatGetValueID = NULL;
static jclass InitFloatClass(JNIEnv *env)
{
  // activate JVM FPU settings
  CHECK_FPU_JVM

  // check the cached class
  jclass result = (jclass)env->NewLocalRef(GFloatClass);
  if (result == NULL) 
  {
    // class is not valid, initialize and store
    result = env->FindClass("java/lang/Float");
    GFloatClass = env->NewWeakGlobalRef(result);
    GFloatGetValueID = env->GetMethodID(result, "floatValue", "()F");
  }
  return result;
}

static jweak GBooleanClass = NULL;
static jmethodID GBooleanGetValueID = NULL;
static jclass InitBooleanClass(JNIEnv *env)
{
  // activate JVM FPU settings
  CHECK_FPU_JVM

  // check the cached class
  jclass result = (jclass)env->NewLocalRef(GBooleanClass);
  if (result == NULL) 
  {
    // class is not valid, initialize and store
    result = env->FindClass("java/lang/Boolean");
    GBooleanClass = env->NewWeakGlobalRef(result);
    GBooleanGetValueID = env->GetMethodID(result, "booleanValue", "()Z");
  }
  return result;
}

static jweak GListClass = NULL;
static jclass InitListClass(JNIEnv *env)
{
  // activate JVM FPU settings
  CHECK_FPU_JVM

  // check the cached class
  jclass result = (jclass)env->NewLocalRef(GListClass);
  if (result == NULL) 
  {
    // class is not valid, initialize and store
    result = env->FindClass("java/util/List");
    GListClass = env->NewWeakGlobalRef(result);
  }
  return result;
}

static jweak GStringClass = NULL;
static jclass InitStringClass(JNIEnv *env)
{
  // activate JVM FPU settings
  CHECK_FPU_JVM

  // check the cached class
  jclass result = (jclass)env->NewLocalRef(GStringClass);
  if (result == NULL) 
  {
    // class is not valid, initialize and store
    result = env->FindClass("java/lang/String");
    GStringClass = env->NewWeakGlobalRef(result);
  }
  return result;
}

static CachedJavaClass GNativeClass("com/bistudio/JNIScripting/NativeObject");

jobject GameValueToNativeObject(JNIEnv *env, GameValuePar from)
{
  return GNativeClass.FromGameValue(env, from);
}

GameValue GameValueFromJObject(JNIEnv *env, jobject from)
{
  if (from == NULL) return GameValue(); // GameNothing

  // activate JVM FPU settings
  SCOPE_FPU_JVM

  jclass clsFloat = InitFloatClass(env);
  jclass clsBoolean = InitBooleanClass(env);
  jclass clsList = InitListClass(env);
  jclass clsString = InitStringClass(env);

  if (env->IsInstanceOf(from, clsFloat)) // GameScalar
  {
    jfloat value = env->CallFloatMethod(from, GFloatGetValueID);
    return ConvertTypes<&GTypeGameScalar>::ToGameValue(env, value);
  }
  else if (env->IsInstanceOf(from, clsBoolean)) // GameBoolean
  {
    jboolean value = env->CallBooleanMethod(from, GBooleanGetValueID);
    return ConvertTypes<&GTypeGameBool>::ToGameValue(env, value);
  }
  else if (env->IsInstanceOf(from, clsList)) // GameArray
  {
    return ConvertTypes<&GTypeGameArray>::ToGameValue(env, from);
  }
  else if (env->IsInstanceOf(from, clsString)) // GameString
  {
    return ConvertTypes<&GTypeGameString>::ToGameValue(env, (jstring)from);
  }
  else
  {
    // this can handle any object, including GameNamespace
    return GNativeClass.ToGameValue(env, from);
  }
}

static jweak GLoader = NULL;
static jmethodID GLoadID = NULL;

jclass LoadJClass(JNIEnv *env, RString name)
{
  // activate JVM FPU settings
  SCOPE_FPU_JVM

  jobject cls = NULL;
  {
    JavaLocalFrame frame(env, 5, &cls);

    // check the cached loader class
    jclass loader = (jclass)env->NewLocalRef(GLoader);
    if (loader == NULL) 
    {
      // class is not valid, initialize and store
      loader = env->FindClass("com/bistudio/JNIScripting/ClassReloader");
      if (loader == NULL)
      {
        LogF("Class loader not found");
        return NULL;
      }

      GLoader = env->NewWeakGlobalRef(loader);
      GLoadID = env->GetStaticMethodID(loader, "initClass", "(Ljava/lang/String;)Ljava/lang/Class;");
      if (GLoadID == NULL)
      {
        LogF("initClass method not found");
        return NULL;
      }
    }

    jstring className = env->NewStringUTF(name);

    cls = env->CallStaticObjectMethod(loader, GLoadID, className);
  }
  return (jclass)cls;
}

static jweak GSerialization = NULL;
// public static byte[] saveObject(Object object)
static jmethodID GSerializationSaveObject = NULL;
// public static Object loadObject(byte[] array)
static jmethodID GSerializationLoadObject = NULL;

// cache the Serialization class and methods
static jclass InitSerializationClass(JNIEnv *env)
{
  // activate JVM FPU settings
  CHECK_FPU_JVM

  jclass cls = (jclass)env->NewLocalRef(GSerialization);
  if (cls == NULL) 
  {
    // class is not valid, initialize and store
    cls = env->FindClass("com/bistudio/JNIScripting/Serialization");
    if (cls == NULL)
    {
      LogF("Class Serialization not found");
      return NULL;
    }
    GSerialization = env->NewWeakGlobalRef(cls);
    GSerializationSaveObject = env->GetStaticMethodID(cls, "saveObject", "(Ljava/lang/Object;)[B");
    if (GSerializationSaveObject == NULL)
    {
      LogF("Serialization::saveObject method not found");
      return NULL;
    }
    GSerializationLoadObject = env->GetStaticMethodID(cls, "loadObject", "([B)Ljava/lang/Object;");
    if (GSerializationLoadObject == NULL)
    {
      LogF("Serialization::loadObject method not found");
      return NULL;
    }
  }
  return cls;
};

bool SaveJObject(JNIEnv *env, AutoArray<char> &buffer, jobject object)
{
  // activate JVM FPU settings
  SCOPE_FPU_JVM

  JavaLocalFrame frame(env, 5);

  jclass cls = InitSerializationClass(env);
  if (cls == NULL) return false;

  jbyteArray array = (jbyteArray)env->CallStaticObjectMethod(cls, GSerializationSaveObject, object);
  if (env->ExceptionCheck())
  {
    env->ExceptionClear();
    LogF("Serialization.saveObject failed");
    return false;
  }
  if (array == NULL)
  {
    LogF("Serialization.saveObject failed");
    return false;
  }

  int size = env->GetArrayLength(array);
  buffer.Realloc(size);
  buffer.Resize(size);
  env->GetByteArrayRegion(array, 0, size, (jbyte *)buffer.Data());
  return true;
}

jobject LoadJObject(JNIEnv *env, const AutoArray<char> &buffer)
{
  // activate JVM FPU settings
  SCOPE_FPU_JVM

  jobject result = NULL;
  {
    JavaLocalFrame frame(env, 5, &result);

    jclass cls = InitSerializationClass(env);
    if (cls == NULL) return NULL;

    int size = buffer.Size();
    jbyteArray array = env->NewByteArray(size);
    env->SetByteArrayRegion(array, 0, size, (const jbyte *)buffer.Data());

    result = env->CallStaticObjectMethod(cls, GSerializationLoadObject, array);
    if (env->ExceptionCheck())
    {
      env->ExceptionClear();
      LogF("Serialization.loadObject failed");
      return NULL;
    }
  }
  return result;
}

#ifdef _WIN32
int LocalFPUSettings::_jvm87;
int LocalFPUSettings::_engine87;
#if USING_SSE
int LocalFPUSettings::_jvmSSE;
int LocalFPUSettings::_engineSSE;
#endif
#endif
