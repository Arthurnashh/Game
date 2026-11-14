#ifdef _MSC_VER
#pragma once
#endif

#ifndef _JVM_FSM_HPP
#define _JVM_FSM_HPP

#include "jvmInit.hpp"
#include <El/Evaluator/jvmImpl.hpp>

class JVMFSM : public RefCount
{
private:
  jobject _jvmObject;
  jmethodID _updateID;

private:
  // used for serialization only
  JVMFSM()
  {
    _jvmObject = NULL;
    _updateID = NULL;
  }
public:
  JVMFSM(RString name, const GameArrayType *args = NULL)
  {
    // initialize
    _jvmObject = NULL;
    _updateID = NULL;

    // activate JVM FPU settings
    SCOPE_FPU_JVM

    JNIEnv *env = GetJNIEnv();
    JavaLocalFrame frame(env, 10);

    // load the class
    jclass cls = LoadJClass(env, name);
    if (!cls) return;
    jmethodID constructorID = env->GetMethodID(cls, "<init>", "()V");
    if (!constructorID) return;
    _updateID = env->GetMethodID(cls, "update", "()Z");
    if (!_updateID) return;

    // create and store the instance
    jobject jvmObject = env->NewObject(cls, constructorID);
    _jvmObject = env->NewGlobalRef(jvmObject);

    if (args)
    {
      // find the Object class (+ caching)
      static jweak GObjectClass = NULL;
      jclass objectClass = (jclass)env->NewLocalRef(GObjectClass);
      if (objectClass == NULL) 
      {
        // class is not valid, initialize and store
        objectClass = env->FindClass("java/lang/Object");
        if (objectClass != NULL)
          GObjectClass = env->NewWeakGlobalRef(objectClass);
      }

      if (objectClass != NULL)
      {
        // convert parameters
        int size = args->Size();
        jobjectArray array = env->NewObjectArray(size, objectClass, NULL);
        for (int i=0; i<size; i++)
        {
          jobject obj = (*args)[i].ToJObject(env);
          env->SetObjectArrayElement(array, i, obj);
          env->DeleteLocalRef(obj);
        }

        // initialize the FSM
        jmethodID initID = env->GetMethodID(cls, "init", "([Ljava/lang/Object;)V");
        if (initID) env->CallVoidMethod(_jvmObject, initID, array);
      }
    }
  }
  ~JVMFSM()
  {
    // activate JVM FPU settings
    SCOPE_FPU_JVM

    JNIEnv *env = GetJNIEnv();
    env->DeleteGlobalRef(_jvmObject);
  }

  bool Update()
  {
    if (!_jvmObject) return true;

    // activate JVM FPU settings
    SCOPE_FPU_JVM

    JNIEnv *env = GetJNIEnv();
    jboolean result = env->CallBooleanMethod(_jvmObject, _updateID);
    return result != JNI_FALSE;
  }

  LSError Serialize(ParamArchive &ar)
  {
    // activate JVM FPU settings
    SCOPE_FPU_JVM

    JNIEnv *env = GetJNIEnv();
    if (!env) return LSOK;

    if (ar.IsSaving())
    {
      AutoArray<char> buffer;
      SaveJObject(env, buffer, _jvmObject);
      ar.Serialize("jvmObject", buffer, 1);
    }
    else if (ar.GetPass() == ParamArchive::PassSecond) // to be sure that referenced objects are created already
    {
      JavaLocalFrame frame(env, 5);

      AutoArray<char> buffer;
      ar.FirstPass(); // simple types are serialized in the first pass
      ar.Serialize("jvmObject", buffer, 1);
      ar.SecondPass(); // switch back
      jobject jvmObject = LoadJObject(env, buffer);
      _jvmObject = env->NewGlobalRef(jvmObject);
      if (jvmObject != NULL)
      {
        jclass cls = env->GetObjectClass(jvmObject);
        _updateID = env->GetMethodID(cls, "update", "()Z");
      }
    }
    return LSOK;
  }
  static JVMFSM *CreateObject(ParamArchive &ar)
  {
    return new JVMFSM();
  }
};

#endif