#ifdef _MSC_VER
#pragma once
#endif

#ifndef _XBOX_CONTEXT_HPP
#define _XBOX_CONTEXT_HPP

// following structures are taken from Microsotf XDK winnt.h

#ifndef __cplusplus
#error C++ required
#endif

//
//  Define the size of the 80387 save area, which is in the context frame.
//

//#define SIZE_OF_80387_REGISTERS      80

//
// Define the size of FP registers in the FXSAVE format.
//

#define SIZE_OF_FX_REGISTERS_XBOX        128

#include <pshpack1.h>
struct FLOATING_SAVE_AREA_XBOX
{
    WORD    ControlWord;
    WORD    StatusWord;
    WORD    TagWord;
    WORD    ErrorOpcode;
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;
    DWORD   MXCsr;
    DWORD   Reserved2;
    BYTE    RegisterArea[SIZE_OF_FX_REGISTERS_XBOX];
    BYTE    XmmRegisterArea[SIZE_OF_FX_REGISTERS_XBOX];
    BYTE    Reserved4[224];
    DWORD   Cr0NpxState;
};
#include <poppack.h>

//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) is is used to constuct a call frame for APC delivery,
//  and 3) it is used in the user level thread creation routines.
//
//  The layout of the record conforms to a standard call frame.
//

struct CONTEXT_XBOX
{

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a threads context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    DWORD ContextFlags;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_FLOATING_POINT.
    //

    FLOATING_SAVE_AREA_XBOX FloatSave;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_INTEGER.
    //

    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;
    DWORD   Ecx;
    DWORD   Eax;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_CONTROL.
    //

    DWORD   Ebp;
    DWORD   Eip;
    DWORD   SegCs;              // MUST BE SANITIZED
    DWORD   EFlags;             // MUST BE SANITIZED
    DWORD   Esp;
    DWORD   SegSs;

};

#endif
