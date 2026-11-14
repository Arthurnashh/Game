#ifdef _MSC_VER
#pragma once
#endif

#ifndef _JVM_INIT_HPP
#define _JVM_INIT_HPP

#ifdef _WIN32
#include <Java/jni.h>
#else
#include <jni.h>
#endif

#include <Es/Strings/rString.hpp>


// Return true if Java VM was successfully initialized
bool IsJavaVMInitialized();

// Access to the JNI interface
JNIEnv *GetJNIEnv();

// Initialization of Java VM
void InitJavaVM(RString jre, int debugPort, bool suspend);

// Destruction of Java VM
void DestroyJavaVM();

// Creation of RVEngine.java interface file
void ExportJavaInterface(RString jniSources);

// Performance test
void PerfTestJavaVM();

#endif

