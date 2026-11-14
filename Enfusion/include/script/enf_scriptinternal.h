//-----------------------------------------------------------------------------
// File: enf_scriptinternal.h
//
// Desc: Script itnernals
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SCRIPTINTERNAL_H
#define ENF_SCRIPTINTERNAL_H

#include "enf_config.h"
#include "common/enf_types.h"

namespace enf
{
/** @defgroup EnforceScript Enforce script
@{*/
//-----------------------------------------------------------------------
//Some painful limits
//-----------------------------------------------------------------------
const uint	MAX_FUNCTION_PARMS	= 16;
const uint	MAX_REGS					= 16;
const uint	VEC_SIZE					= 3;
const uint	MAX_TYPES				= 65535;

//-----------------------------------------------------------------------
//Status register flags. Used both by compiler and virtual machine
//-----------------------------------------------------------------------
const uint	SF_NEG		= 1;
const uint	SF_ZERO		= 64;


enum DebugFlags
{
	DBGF_BREAK = 0x4000,
	DBGF_TOCURSOR = 0x8000,
	DBGF_MASK = 0xc000
};

struct FuncPtrDef
{
	Instance**	pInstance;
	uint			FuncIndex;
};

//-----------------------------------------------------------------------
//Virtual machine opcodes
//-----------------------------------------------------------------------
enum OpCode
{
	OP_NONE = 0,
	OP_WAIT,
	OP_GOTO,OP_TCALL, OP_NCALL, OP_NCALLI,OP_CALL,OP_ICALL,OP_RTS,
	OP_BGE,OP_BGT,OP_BLE,OP_BLT, OP_BEQ,OP_BNE,
	OP_NEW,OP_DELETE,
	OP_DEBUG,
	OP_IMOVE,
	OP_INC,
	OP_DEC,
	OP_PUSH,
	OP_PUSHC,
	OP_PUSHS,
	OP_PUSHV,
	OP_POP,
	OP_POPC,
	OP_POPS,
	OP_POPV,
	OP_PARM0,

	OP_MOVE,OP_MOVEI,OP_MOVES,OP_MOVEV,OP_MOVEA,OP_MOVEFR,OP_MOVETR,
	OP_CITF,OP_CFTI,
	OP_IFV, OP_ITV,
	OP_CMPI,OP_CMPF,OP_CMPV,OP_CMPS,
	OP_SIZEOF,
	OP_PARM2,

	OP_CLAMPF,OP_CLAMPI,
	OP_FFV,OP_FTV,
	OP_CFV,OP_CTV,
	OP_MODI,
	OP_ADDI,OP_ADDF,OP_ADDV,OP_ADDS,
	OP_ANDI,OP_ORI, OP_EORI, OP_LSLI, OP_ASRI,
	OP_SUBI,OP_SUBF,OP_SUBV,
	OP_MULI,OP_MULF,OP_CROSSV,OP_SCALEV,OP_DOTV,
	OP_DIVI,OP_DIVF,
	OP_PARM3
};

#define XCMD_OPCODE(xcmd) ((xcmd) & 0xff)
#define XCMD_SIZEOF(xcmd) (((xcmd) >> 8) & 0xff)
#define XCMD_OFFSET(xcmd) ((short)(((xcmd) >> 16) & 0xffff))

//-----------------------------------------------------------------------
//!One VM instruction. Variable sized
//-----------------------------------------------------------------------
struct Stat
{
	uint			XCmd;
	
	ENF_INLINE OpCode GetOpcode() const
	{
		return (OpCode)XCMD_OPCODE(XCmd);
	}
	ENF_INLINE int GetSizeof() const
	{
		return XCMD_SIZEOF(XCmd);
	}
	ENF_INLINE int GetOffset() const
	{
		return (int)XCMD_OFFSET(XCmd);
	}

	ENF_INLINE void SetOpcode(OpCode op)
	{
		enf_assert(op < 256);
		XCmd = (XCmd & 0xffffff00) | op;
	}
	ENF_INLINE void SetSizeof(uint size)
	{
		enf_assert(size < 256);
		XCmd = (XCmd & 0xffff00ff) | (size << 8);
	}
	ENF_INLINE void SetOffset(int off)
	{
		enf_assert(off < 65536 && off > -32768);
		XCmd = (XCmd & 0x0000ffff) | (off << 16);
	}
	//!optional parameters
	void*			Op[3];

	void SetJmp(const Stat *dest);
};


//@}
}

#endif //ENF_SCRIPTINTERNAL_H