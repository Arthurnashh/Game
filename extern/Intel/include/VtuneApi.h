/*************************************************************************
 *
 *              INTEL CORPORATION PROPRIETARY INFORMATION
 *  This software is supplied under the terms of a license agreement or
 *  nondisclosure agreement with Intel Corporation and may not be copied
 *  or disclosed except in accordance with the terms of that agreement.
 *
 *    Copyright(c) 2002-2009 Intel Corporation. All Rights Reserved.
 *
 ************************************************************************/

/*! @deprecated Legacy API 
 * @file VtuneApi.h
 * @brief
 * This header file provides declarations and usage model for following APIs.
 * 
 *       VTStartSampling, VTStopSampling, VTBindSamplingResults
 *       VTPause, VTResume
 *       VTPauseSampling, VTResumeSampling
 *
 *  LINK REQUIREMENTS :
 *
 *  The user program needs to link with VtuneApi.lib on Windows* and
 *  with libVtuneApi.so on Linux*. Also, some dependent VTune(TM) analyzer 
 *  libraries have to be in the path on Windows* and in the LD_LIBRARY_PATH
 *  on Linux*.
 *
 *  On Linux*, the user program also needs to link with libpthread.so.   
 */

#ifndef _VTUNEAPI_H_
#define _VTUNEAPI_H_
/** @cond exclude_from_documentation */
#ifndef ITT_OS_WIN
#  define ITT_OS_WIN   1
#endif /* ITT_OS_WIN */

#ifndef ITT_OS_LINUX
#  define ITT_OS_LINUX 2
#endif /* ITT_OS_LINUX */

#ifndef ITT_OS_MAC
#  define ITT_OS_MAC   3
#endif /* ITT_OS_MAC */

#ifndef ITT_OS
#  if defined WIN32 || defined _WIN32
#    define ITT_OS ITT_OS_WIN
#  elif defined( __APPLE__ ) && defined( __MACH__ )
#    define ITT_OS ITT_OS_MAC
#  else
#    define ITT_OS ITT_OS_LINUX
#  endif
#endif /* ITT_OS */

#ifndef ITT_PLATFORM_WIN
#  define ITT_PLATFORM_WIN 1
#endif /* ITT_PLATFORM_WIN */ 

#ifndef ITT_PLATFORM_POSIX
#  define ITT_PLATFORM_POSIX 2
#endif /* ITT_PLATFORM_POSIX */

#ifndef ITT_PLATFORM
#  if ITT_OS==ITT_OS_WIN
#    define ITT_PLATFORM ITT_PLATFORM_WIN
#  else
#    define ITT_PLATFORM ITT_PLATFORM_POSIX
#  endif /* _WIN32 */
#endif /* ITT_PLATFORM */

#ifndef __ITT_INTERNAL_INCLUDE
#  if ITT_PLATFORM==ITT_PLATFORM_WIN
#    pragma message("WARNING!!! Include file VTuneApi.h is deprecated and should not be included anymore")
#  else /* ITT_PLATFORM==ITT_PLATFORM_WIN */
#    warning "Include file VTuneApi.h is deprecated and should not be included anymore"
#  endif /* WIN32 */
#endif /* __ITT_INTERNAL_INCLUDE */

#if !defined ITT_PLATFORM
#error "Unsupported operating system detected"
#endif

/*#include "vt_errors.h"*/
#include "VtuneApiTypes.h"
#include "ittnotify.h"

#if ITT_PLATFORM==ITT_PLATFORM_WIN
#   define VTUNEAPI
#   define VTUNEAPICALL __stdcall
#else
#   define VTUNEAPI
#   define VTUNEAPICALL
#endif

/* ********************************************************* */

#define VTAPI_EVENT_SAMPLED                   0x01

/* The following bit-flags can be used to pass in the globalOptions field.*/
#define VTAPI_OPTIONS_ENABLE_LBR              0x01
/** @endcond */

/*************************************************************/

/*! @struct __vtune_event
 * @brief VTUNE_EVENT
 */
typedef struct __vtune_event {
    /** @brief Sample After value for this event.*/
    U64 sampleAfter;     
    /** @brief Reserved field - (MUST be set to 0).*/
    U64 parameter;               
    /** @brief Reserved field - (MUST be set to 0).*/
    U32 options;                 
    /** @brief Field to return status by the VTStartSampling.
      * Used to indicate which events could be sampled in a single run, if there are too many events. 
      */
    U32 status;      
    /** @brief event name.*/
    char *name;                  
} VTUNE_EVENT, *PVTUNE_EVENT;


/*************************************************************/

/*! @struct __vtune_sampling_params 
 * @brief VTUNE_SAMPLING_PARAMS
 */
typedef struct __vtune_sampling_params {
  /** @brief caller sets to sizeof(VTUNE_SAMPLING_PARAMS)*/
  U32   sizeVtuneSamplingParams;
  /** @brief caller sets to sizeof(VTUNE_EVENT)*/
  U32   sizeVtuneEvent;
  /** @brief Used to pass sampling options. VTAPI_OPTIONS...
    * specify the bit-flags (above). Should be set to 0 if no options need to be specified.
    */
  U64   globalOptions;
  /** @brief Reserved - (MUST be set to 0).*/
  U32   globalParameter;
  /** @brief If 1, start sampling in pause mode*/
  U32   startInPauseMode;
  /** @brief maximum number of samples to be collected.
    * If this is 0, sampling continues until VTStopSampling is called.
    */
  U32   maxSamples;
  /** @brief number of samples per buffer.*/
  U32   samplesPerBuffer;
  /** @brief the sampling interval in miliseconds.*/
  float fSamplingInterval;
  /** @brief 0 for time based sampling. non-zero for event based sampling. */
  U32   samplingType;
  /** @brief Number of events to be sampled.*/
  U32   nEvents;
  /** @brief Pointer to event information array. Ignored, if TBS is selected. */
  VTUNE_EVENT *pEvents;
  /** @brief tb5 filename. Could be simple or path qualified.*/
  char  *tb5Filename;
  /** @brief Reserved field, must be set to 0.*/
  UIOP  reservedField;
  /** @brief 1 indicates a counts file will be generated. Default is 0. 
      Counts file name is tb5Filename.txt.*/
  U32   count;
  /** @brief Provide a cpu mask to sample on specified cpu's. eg: "0,1,2-15,34" */
  char  *cpu_mask;

} VTUNE_SAMPLING_PARAMS, *PVTUNE_SAMPLING_PARAMS;

#if defined(__cplusplus)

extern "C" {

#endif   /* __cplusplus */

/* =================================================================
 * VTStartSampling, VTStopSampling and VTBindSamplingResults
 * =================================================================
 */    
 
 /* example using_VtuneApi.cpp
 * USAGE MODEL :
 *
 * The following example program shows the usage of these APIs.
 * @code
 * #include <stdio.h>
 * #include <windows.h>
 * #include "VtuneApi.h"
 * int main ()
 * {
 *    VTUNE_EVENT events[] = {
 *        { 10000, 0, 0, 0, "Clockticks" },
 *        { 10000, 0, 0, 0, "Instructions Retired" },
 *    };
 *
 *    U32 numEvents = sizeof(events) / sizeof(VTUNE_EVENT);
 *    VTUNE_SAMPLING_PARAMS params = {
 *        sizeof(VTUNE_SAMPLING_PARAMS),
 *        sizeof(VTUNE_EVENT),
 *        0,
 *        0,
 *        0, 
 *        0, 
 *        1000, 
 *        40,    
 *        1, 
 *        numEvents, 
 *        events, 
 *        "results.tb5",
 *        0,
 *        0,
 *        ""
 *    };
 *
 *    U32 u32Return = VTStartSampling(&params);
 *
 *
 *    if (u32Return) { 
 *        // Handle error here...
 *        exit(0);
 *    }
 *
 *    // Some user code goes here...
 *    // ...
 *    // ...
 *
 *    u32Return = VTStopSampling(1);
 *    if (u32Return) {
 *        // handle error...
 *        exit(0);
 *    }
 *    
 *    // The rest of the code goes here...
 *    // ...
 *    // ...
 * } 
 * @endcode
 */

/** @deprecated Non-supported Legacy API
 * @brief VTStartSampling enables users to configure and start local 
 * sampling from within their applications. 
 * @param samParams - Sampling params structure that is used to configuring sampling
 * @return 
 * VTStartSampling return 0 on SUCCESS and a non-zero value if there is an error.
 * The returned value can be used to check for errors. The error codes are defined above.
 * @note VTAPI_MULTIPLE_RUNS is a non-fatal error returned by VTStartSampling when all the events specified cannot be 
 * collected in a single run. In this case, sampling is performed on a subset of events specified by the user. 
 * When this error is returned, the caller is expected to call VTStopSampling to stop sampling.
 * Caller can check the 'status' field of VTUNE_EVENT entries to find out which events were sampled. 
 * (status & VTAPI_EVENT_SAMPLED) will be non-zero for events that were sampled, and 0 otherwise.
 */
VTUNEAPI
U32 VTUNEAPICALL VTStartSampling(VTUNE_SAMPLING_PARAMS *samParams);
/** @deprecated Non-supported Legacy API
 * @brief The user can stop the collection by calling VTStopSampling. 
 * @param bBindSamplingResults - A boolean flag to indicate whether
 * the sampling results (.tb5) file will be processed or not.
 * @return 
 * VTStopSampling return 0 on SUCCESS and a non-zero value if there is an error.
 * The returned value can be used to check for errors. The error codes are defined above.
 */
VTUNEAPI
U32 VTUNEAPICALL VTStopSampling(U32 bBindSamplingResults);
/** @deprecated Non-supported Legacy API
 * @brief The result is a .tb5 file that 
 * the users can open using VTune(TM) Performance Analyzer.
 * @param tb5Filename - Simple or absolute filename of a an unprocessed tb5 filename. 
 */
VTUNEAPI
U32 VTUNEAPICALL VTBindSamplingResults(char *tb5Filename);

/* =================================================================
 *      
 * VTPause and VTResume
 *
 * =================================================================
 */
 /** @brief
 * VTPause pause data collection during a Sampling, Counter Monitor, or Callgraph activity.
 *
 * If VTPause is called while a  Sampling collection is active, a flag 
 * is set which suspends collection of PC samples on the current machine. 
 * The overhead to set the flag is very low, so the VTPause can be called at a high frequency.
 *
 * If VTPause is called while a  Callgraph collection is active, 
 * Callgraph data collection is paused for the current process.
 *
 * If VTPause is called while a  Counter Monitor collection is active, 
 * Counter Monitor data collection is paused.
 *
 * VTPause can be safely called when the Sampling, Counter 
 * Monitor, and Callgraph collectors are not active.In this case, the 
 * VTPause do nothing.
 *
 * @note
 * Sampling, Counter Monitor, and Callgraph activities are typically 
 * started with the VTune(TM) Performance Analyzer application. 
 * The VTune(TM) Performance Analyzer GUI allows Sampling, 
 * Counter Monitor, and Callgraph activities to be started in "Pause" mode
 * which suspends data collection until a VTResume is called.
 * Data collection can also be paused and resumed by the Pause/Resume button 
 * in the VTune(TM) Performance Analyzer GUI.
 * See VTune(TM) Performance Analyzer onlilne help for more details.
 */

VTUNEAPI
void VTUNEAPICALL VTPause(void);

/** @brief
 * VTResume resume data collection during a Sampling, Counter Monitor, or Callgraph activity.
 *
 * Collection of PC samples can be resumed by calling VTResume which clears 
 * the flag. The overhead to clear the flag is very low, so the VTResume 
 * can be called at a high frequency.
 *
 * Callgraph data collection for the current process can be resumed by calling VTResume.
 *
 * Counter Monitor data collection can be resumed by calling VTResume.
 *
 * VTResume can be safely called when the Sampling, Counter Monitor, and Callgraph collectors
 * are not active.In this case, the VTResume do nothing.
 *
 * @note
 * Sampling, Counter Monitor, and Callgraph activities are typically 
 * started with the VTune(TM) Performance Analyzer application. 
 * The VTune(TM) Performance Analyzer GUI allows Sampling, 
 * Counter Monitor, and Callgraph activities to be started in "Pause" mode
 * which suspends data collection until a VTResume is called.
 * Data collection can also be paused and resumed by the Pause/Resume button 
 * in the VTune(TM) Performance Analyzer GUI.
 * See VTune(TM) Performance Analyzer onlilne help for more details.
 */
VTUNEAPI
void VTUNEAPICALL VTResume(void);

VTUNEAPI
void VTUNEAPICALL VTNameThread(const char *);

/* =================================================================
 *      
 * VTPauseSampling and VTResumeSampling
 *
 * =================================================================
 */
/** @deprecated Non-supported Legacy API
 * @brief These APIs are provided for backward compatibility.
 */
VTUNEAPI
void VTUNEAPICALL VTPauseSampling(void);
/** @deprecated Non-supported Legacy API
 * @brief These APIs are provided for backward compatibility.
 */
VTUNEAPI
void VTUNEAPICALL VTResumeSampling(void);
/** @deprecated Non-supported Legacy API
 * @brief These APIs are provided for backward compatibility.
 */
#if ITT_PLATFORM==ITT_PLATFORM_WIN
VTUNEAPI
void VTUNEAPICALL VTPauseCounterMonitor(void);
/** @deprecated Non-supported Legacy API
 * @brief These APIs are provided for backward compatibility.
 */
VTUNEAPI
void VTUNEAPICALL VTResumeCounterMonitor(void);
#endif

#if defined(__cplusplus)
}
#endif   /* __cplusplus*/

#endif  /* _VTUNEAPI_H_*/
