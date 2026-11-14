//-----------------------------------------------------------------------------
// File: enf_scriptcontext.h
//
// Desc: Main script context, holds and manage compiled image
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SCRIPTCONTEXT_H
#define ENF_SCRIPTCONTEXT_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "script/enf_scriptmain.h"

namespace enf
{
/** @defgroup EnforceScript Enforce script
@{*/
class Funct;
class ScriptContext;
class ScriptModule;
class ScriptThread;
class CParser;
struct TToken;
class DebugInfo;
class ScriptDebugger;
struct Stat;
struct VMStack;

//-----------------------------------------------------------------------
//! Script context
//-----------------------------------------------------------------------
class PUBLIC_API ScriptContext: public ScriptModule, public SerializeHandler
{
	friend class CCompiler;
	friend class CCompilerContext;
	friend class ScriptDebugger;
	friend class ScriptModule;

protected:
//storage
	Variable*	m_vRegs;
	Variable*	m_vVRegs;
	Variable*	m_vSRegs;
	Variable*	m_vCRegs;
	Variable*	m_vFRegs;
	Variable*	m_pvThis;

	ScriptDebugger*	m_pDebugger;
	typedef Pair<const Stat*, uint> Breakpoint;
	Set<Breakpoint>	m_Breapoints;

	Variable*	m_pvNumbersInt[10];
	Variable*	m_pvOneFloat;
	Variable*	m_pvZeroFloat;
	Variable*	m_pvZeroVector;
	Variable*	m_pvZeroString;
	Variable*	m_pvVoid;

//threads
	ScriptThread*		m_psFirstFree;
	ScriptThread*		m_psFirst;
	ScriptThread*		m_psCurrent;

	static uint			m_iDebugStackPos;
	static ScriptThread*	m_pLastDebugThread;

	static DebugMode	m_DebugMode;
	bool					m_bEnableDebug;
	bool					m_bFlushDebugVars;
	bool					m_bProfile;

	VMErrorHandler*	m_pVMErrorHandler;

	char*					m_pStringPool[16];
	uint					m_iStringCount[16];
	static const uint m_iMaxStringCount[16];
	SerializeHandler*	m_pCustomSerializeHandler;

	static Variable**	m_pParms;

	Set<ScriptModule*>	m_Modules;
	typedef Pair<const Variable*, uint> TWatch;
	Set<TWatch>	m_Watch;

	typedef Pair<intptr_t, const StaticBuffer*> TStaticPair;
	Set<TStaticPair>	m_StaticBuffers;
	CMutex				m_Mutex;

	uint		m_iNumCrashes;
	uint		m_iMaxCrashes;
	bool		m_bNoCrash;
	bool		m_bIgnoreCrash;
	int		m_iRecursion;

	ENF_INLINE void _CopyParameters(const Variable *const*parm, const Funct *fn)
	{
		for(uint n = 0; n < fn->ParmNum; n++)
		{
			if(parm[n] != fn->Variables[n])
				Copy(parm[n], fn->Variables[n]);
		}
	}

	ENF_INLINE void ClearParameters(const Funct* fn)
	{
		for(uint n = 0; n < fn->ParmNum; n++)
		{
			Variable* pvar = fn->Variables[n];

			switch(pvar->GetVType())
			{
			case VARTYPE_STRING:
				pvar->String = (char*)NullString;
				break;

			case VARTYPE_VECTOR:
				((int*)pvar->Pointer)[0] = 0;
				((int*)pvar->Pointer)[1] = 0;
				((int*)pvar->Pointer)[2] = 0;
				break;

			default:
				pvar->Int = 0;
			}
		}
	}

	VMStack* IPushCall(ScriptThread* thread, Instance* pinst, const Class* cl, int func);
	VMStack* PushCall(ScriptModule* mod, ScriptThread* thread, Instance** pinst, const Class* cl, int func);
	VMStack* PopCall(ScriptThread* thread);

	void Run(ScriptThread* thread, VMStack* stack, int bstack);

	~ScriptContext();

	void RegisterModule(ScriptModule* mod);
	void UnregisterModule(ScriptModule* mod);
	void InitCommonConstants();

	int StartDebugger(const Funct* fn, Variable* clas, uint line);
	void TouchVar(ScriptThread* currentThread, ScriptModule* mod, IVariable* var, bool modify);

	ENF_INLINE void SetThis(Instance* inst)
	{
		m_pvThis->InstancePtr = inst;
	}

public:
	ENF_DECLARE_MMOBJECT(ScriptContext);

#ifdef _DEBUG
	Set<Instance*> m_Instances;
	CMutex			m_InstMutex;
	static void RegisterInstance(Instance* inst);
	static void UnregisterInstance(Instance* inst);
#else
	static ENF_INLINE void RegisterInstance(Instance* inst) {}
	static ENF_INLINE void UnregisterInstance(Instance* inst) {}
#endif

	void ClearBreakpoints();
	void AddBreakpoint(const Stat* st, uint flags);
	void RemoveBreakpoint(const Stat* st);

	//TODO: just temporal. Do it better!
	static ScriptModule* m_pDeserializeModule;
	static ScriptContext* m_pCurrent;

	static const char RTE_WrongParameterValue[];
	static const char RTE_NewClass[];
	static const char RTE_NullPointerError[];
	static const char RTE_VIndexOutOfRange[];
	static const char RTE_BadInstancePointer[];

	VarType* m_vtIntType;
	VarType* m_vtArrayType;
	VarType* m_vtFloatType;
	VarType* m_vtStringType;
	VarType* m_vtVoidType;
	VarType* m_vtClassType;
	VarType* m_vtClassRefType;
	VarType* m_vtVectorType;
	VarType* m_vtFuncType;

	const char*	NullString;

	const Variable* GetParmDef(uint p) const;
	void SerializeVariable(SerializeInputAdapter* adapter, const Variable* var, IVariable* ivar, uint serializeType);
	void SerializeVariable(SerializeOutputAdapter* adapter, const Variable* var, const IVariable* ivar, uint serializeType) const;
	void SerializeVariable(SerializeOutputAdapter* adapter, const Variable* var, const IVariable* ivar, uint size, uint serializeType) const;
	uint SerializeVariableSize(const Variable* var, const IVariable* ivar) const;

	bool DoSerializeVariable(const Variable* var, uint& serializeType) const;
	void FlushThreads();
	void AddWatchVariable(const Variable* var, uint flags);

	void SetSerializeHandler(SerializeHandler* handler)
	{
		m_pCustomSerializeHandler = handler;
	}

	CMutex& GetMutex()
	{
		return m_Mutex;
	}

/** \name Modules methods
*/
//@{
	ScriptModule* CreateModule(const char* name, ScriptModule* parent, size_t staticSize);
	uint GetNumModules() const
	{
		return m_Modules.GetCardinality();
	}

	ScriptModule* GetModule(uint mod) const
	{
		return m_Modules[mod];
	}

//@}

/** \name Debugger methods
*/
//@{

	void InitDebugger(const char* ipaddress, int port);
	void RemoveDebugger();
	ENF_INLINE bool IsDebugging() const
	{
		return m_DebugMode != SDM_NONE;
	}

	void ClearCrashCounter()
	{
		m_iNumCrashes = 0;
		m_bIgnoreCrash = false;
	}

	uint GetNumCrashes() const
	{
		return m_iNumCrashes;
	}
	void SetNoCrashMode(bool noCrash)
	{
		m_bNoCrash = noCrash;
		m_bIgnoreCrash = false;
	}

	void SetVMErrorHandler(VMErrorHandler* errorHandler)
	{
		m_pVMErrorHandler = errorHandler;
	}
//@}


/** \name Variable methods
Variables creating & manipulating
*/
//@{
	void Copy(const Variable* src, Variable* dest);
//@}


/** \name Memory methods
Memory allocating/deallocating
*/
//@{
	ENF_INLINE static uint MemSize(void* ptr)
	{
		enf_assert(ptr);

		int* iptr = (int*)ptr;

		return *(iptr - 1);
	}

	virtual void FreeMem(void* ptr);
	virtual void* AllocMem(uint size, const void* ptr, const char* file, int line);

	const StaticBuffer* GetStaticOwner(const void* ptr) const;
	//!Tells whether is memory allocated from StaticBuffer of any of registered ScriptModule
	ENF_INLINE bool IsStatic(const void* ptr) const
	{
		return GetStaticOwner(ptr) != NULL;
	}

//@}

/** \name String methods
Strings allocating/deallocating
*/
//@{
	void FreeString(char* ptr);
	char* AllocString(uint length, const void* ptr);
	void CopyString(const char* v1, char** v2, bool forceCopy = false);
	void CopyString(const char* str1, const Variable* v2, char** str2, bool forceCopy = false);

//@}

/** \name Thread methods
Thread management
*/
//@{
	ScriptThread* CreateThread(bool waitable = false);
	void StopThread();
	bool QuitThread();
	void ProcessThreads(float time, intptr_t ID);
	void CleanupThreads();
	virtual void StoreThread(Instance* inst, ScriptThread* rt)	{} ///<virtual method for custom storing active threads
	const ScriptThread* GetFirstThread() const
	{
		return m_psFirst;
	}

	ScriptThread* GetCurrThread() const
	{
		return m_psCurrent;
	}
//@}

/** \name Instance methods
Class instance management
*/
//@{
	Instance* AllocInstance(const Class* cl);
	void ZeroInstance(const Class* cl, Instance* inst);

	/*!
	Free all allocated memory of Instance.
	\param inst
	Instance to be deleted
	\param keepcached
	When Class has allowed caching, it's cached there
	*/
	void FreeInstance(Instance* inst, bool keepcached = true);

	void CallDestructors(const Class* cl,Instance* ent);
	void CallConstructors(const Class* cl,Instance* ent, Variable **parms);

//@}

	//!Cleans up everything
	virtual void Cleanup();

	ENF_INLINE void SetProfiler(bool enable)
	{
		m_bProfile = enable;
	}

	ENF_INLINE bool IsProfiler() const
	{
		return m_bProfile;
	}

	void Error(const char* reason);
	uint GetCallStack(char* callStack, uint maxLength);
	virtual void AfterCompile();

//Constructor
	ScriptContext(StaticBuffer* stat);
};

}
#endif //ENF_SCRIPTCONTEXT_H
