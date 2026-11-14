//-----------------------------------------------------------------------------
// File: enf_enforcescriptcontext.cpp
//
// Desc: Script-engine API
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"

#include "enf_main.h"
#include "baseworld/enf_entity.h"
#include "script/enf_scriptapi.h"
#include "script/enf_scriptapivars.h"
#include "script/enf_scriptmain.h"
#include "script/enf_scriptdebuginfo.h"
#include "script/enf_scriptcompiler.h"
#include "script/enf_scriptthread.h"
//#include "common/enf_cpucounter.h"
#include "common/enf_parser.h"
#include "common/enf_serialize.h"
//#include "baseworld/enf_baseworld.h"
#include "common/enf_iff.h"
#include "system/enf_vfilesystem.h"

namespace enf
{
class VObjectResource;
const char EnforceScriptContext::RTE_DeletedEntityPointerError[] = "Entity is already deleted";

//-----------------------------------------------------------------------------
int DefaultEntityEventHandler::HandleEvent(const Event& event)
{
	//pass the entity event to script
	const EntityEvent& eevent = (const EntityEvent&)event;
	
	//ENF_BEGIN_PROFILE(PROFILE_API_CALL);
	uint ev = enf_log2(eevent.GetEventMask());

	SArray<const Class *, 64> classes;

	const Stat* nullCode = g_pScript->m_pEntityClass->GetMethod(ev)->GetCode();

	const Stat* lastStat = NULL;

	for(const Class *cc = eevent.GetEntity()->GetClass(); cc; cc = cc->GetSuper())
	{
		if(cc->GetNumMethods() < ev)
			continue;
		const Funct* method = cc->GetMethod(ev);
		if(method == NULL)
			continue;
		
		//if null definition, everything above is the same
		if(method->GetCode() == nullCode)
			break;

		//avoid mangled types with reused Class to be called more than once
		if(method->GetCode() == lastStat)
			continue;
		lastStat = method->GetCode();

		classes.Insert(cc);
	}

	int res = false;


	if(!classes.IsEmpty())
	{
/*		LogF(LL_NORMAL, "%s::%s actor %s", 
			classes[0]->GetName(),
			classes[0]->Functions[event]->Name,
			actor->GetTypeName());
*/
		ScriptThread *rt = g_pScript->GetCurrThread();

		while(true)
		{
			const Class* cc = classes.Pop();

/*
			LogF(LL_NORMAL, "%s::%s actor %s", 
			affected->GetTypeName(),
			cc->Functions[event]->Name,
			actor->GetTypeName());
*/
			//copy parameters
			Funct* fn = cc->GetMethod(ev);
			fn->GetVariable(0)->InstancePtr = eevent.GetOther();
			fn->GetVariable(1)->Pointer = eevent.GetExtra();

			g_pScript->IExecute(rt, ev + 1, eevent.GetEntity(), cc);

			res = cc->GetMethod(ev)->GetReturnVariable()->Int;

			if(classes.IsEmpty())
				break;

			if(eevent.GetEntity()->IsFlagSet(TFL_REMOVE))
				break;
		}
	}

	//ENF_END_PROFILE();

	return res;
}

//-----------------------------------------------------------------------
void EnforceScriptContext::StoreThread(Instance *ins, ScriptThread *rt)
{
#ifdef ENF_DONE
	enf_assert(BaseWorld::Current() != NULL);
	rt->m_pOwner = ins;
/*	if(ins)
		LogF(LL_DEBUG, "Thread %s class %s %x th %x", rt->m_UserName.cstr(), ins->GetClass()->GetName(), ins, rt);
	else
		LogF(LL_DEBUG, "Thread %s th %x", rt->m_UserName.cstr(), rt);*/
	BaseWorld::Current()->AddThread(ins, rt);
#endif
}

//-----------------------------------------------------------------------
void EnforceScriptContext::FreeMem(void *ptr)
{
	if(ptr == NULL)
		Error("Attempt to free NULL pointer");

	if(IsStatic(ptr))
		Error("Attempt to free pointer to static heap");

	int *mptr = (int *)ptr;
	mptr--;

	m_Stats.m_iAllocatedMemory -= (*mptr + sizeof(int));
	MemoryManager::Free(mptr);
}

//-----------------------------------------------------------------------
void *EnforceScriptContext::AllocMem(int size, const void *ptr, const char* file, int line)
{
	enf_assert(size > 0);

	char *cptr = (char*)MemoryManager::Alloc(size + sizeof(int), file, line);

	int *mptr = (int *)cptr;

	if(mptr == NULL)
		Error("Out of memory!");

	*mptr++ = size;

	if(ptr) CopyMem(mptr, ptr, size);

	m_Stats.m_iAllocatedMemory += (size + sizeof(int));

	return (void *)mptr;
}
//-----------------------------------------------------------------------
bool EnforceScriptContext::LoadScript(const char *name, bool checklinking)
{
	enf_assert(name);

	CParser scripttp;

	if(scripttp.Parse(name) != PE_OK)
	{
		return false;
	}

	CCompiler ccomp;

	SString<> dname;
	dname = name;
	if(MainConfig::m_bList)
	{
		dname.ReplaceExt("lst");
	}

	ccomp.m_iOptimize = MainConfig::m_iOptimize;
	ccomp.m_bList = MainConfig::m_bList;
	
	static bool initAPI = false;
	if(!initAPI)
	{
		initAPI = true;
		InitAPI();
	}

	if(!ccomp.Compile(scripttp, this, NULL, dname.cstr()))
	{
		return false;
	}

	AfterCompile();

	if(checklinking)
	{
		if(!CheckLinking(&scripttp))
		{
			return false;
		}
	}

	return true;
}
//-----------------------------------------------------------------------
bool EnforceScriptContext::CompileModule(const char *name, ScriptModule* module, bool checkLink)
{
	enf_assert(name);

	CParser scripttp;

	if(scripttp.Parse(name) != PE_OK)
	{
		return false;
	}

	CCompiler ccomp;

	SString<> dname;
	dname = name;
	if(MainConfig::m_bList)
	{
		dname.ReplaceExt("lst");
	}

	ccomp.m_iOptimize = MainConfig::m_iOptimize;
	ccomp.m_bList = MainConfig::m_bList;
	
	if(!ccomp.Compile(scripttp, this, module, dname.cstr()))
	{
		return false;
	}
	
	if(checkLink && !module->CheckLinking(&scripttp))
	{
		return false;
	}

	return true;
}
}