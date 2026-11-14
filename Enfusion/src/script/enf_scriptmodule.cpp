//-----------------------------------------------------------------------------
// File: enf_scriptmodule.cpp
//
// Desc: Script module. Holds compiled image
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "enf_main.h"
#include "script/enf_scriptcontext.h"
#include "script/enf_scriptinternal.h"
#include "script/enf_scriptthread.h"
#include "common/enf_parser.h"
#include "common/enf_staticbuffer.h"
#include "script/enf_scriptdebuginfo.h"
#include "system/enf_profiler.h"

namespace enf
{
//-----------------------------------------------------------------------
int ConstantHashFunc::compare(const Variable* var1, const Variable* var2) const
{
	if(var1->GetTypeIndex() < var2->GetTypeIndex()) return -1;
	if(var1->GetTypeIndex() > var2->GetTypeIndex()) return 1;
	if(var1->IsPtr() < var2->IsPtr()) return -1;
	if(var1->IsPtr() > var2->IsPtr()) return 1;
	if(var1->IsPtr())
	{
		int arr1 = var1->ArraySize();
		int arr2 = var2->ArraySize();
		if(arr1 < arr2) return -1;
		if(arr1 > arr2) return -1;
		return (memcmp(var1->Pointer, var2->Pointer, var1->GetType()->Sizeof * arr1));
	}
	else
	{
		if(var1->Pointer < var2->Pointer) return -1;
		if(var1->Pointer > var2->Pointer) return 1;
	}
	return 0;
}

//-----------------------------------------------------------------------
ScriptModule::~ScriptModule()
{
	if(this != m_pContext)
	{
		CMutexLock lock1(GetContext()->m_Mutex);

#ifdef ENF_DEVELOPER
		for(ScriptThread *st = m_pContext->m_psFirst; st; st = st->m_pNext)
		{
			if(st->m_sState == ScriptThread::TF_KILL)
				continue;
			for(uint n = 1; n <= st->m_iStackPos; n++)
			{
				if(st->m_sStack[n].m_pfFunc && st->m_sStack[n].m_pfFunc->GetModule() == this)
				{
					SString<> temp;
					temp.format("Trying to delete script module '%s' when there is thread running its function '%s'",
						GetName().cstr(), st->m_iStackPos >= 1 ? st->m_sStack[1].m_pfFunc->GetName() : "");

					GetContext()->Error(temp.cstr());
				}
			}
		}
#endif

#ifdef _DEBUG
		CMutexLock lock2(GetContext()->m_InstMutex);

		const Array<VarType*>& types = GetVarTypes();
		bool firstReport = true;
		for(uint n = 0; n < GetContext()->m_Instances.GetCardinality(); n++)
		{
			uint t;
			for(t = 0; t < types.GetCardinality(); t++)
			{
				const Instance* inst = GetContext()->m_Instances[n];
				if(types[t]->Type == VARTYPE_CLASS && inst->GetClass() == static_cast<Class*>(types[t]))
				{
					if(firstReport)
					{
						firstReport = false;
						LogF(LL_ERROR, "Object leaks when deleting module %s", GetName().cstr());
					}
					LogF(LL_ERROR, "Object %x class %s", inst, static_cast<Class*>(types[t])->GetName());
				}
			}
		}
#endif

		Cleanup();
		m_pContext->UnregisterModule(this);
		delete m_pStatic;
		m_pStatic = NULL;
	}

	for(uint n = 0; n < m_pContext->m_Watch.GetCardinality();)
	{
		if(m_pContext->m_Watch[n].Key->Module == this)
		{
			m_pContext->m_Watch.RemoveElement(n);
			continue;
		}
		n++;
	}

	if(m_pParentModule)
	{
		//remove from chain
		ScriptModule** prev = &m_pParentModule->m_pChildModule;
		while(*prev)
		{
			if(*prev == this)
			{
				*prev = m_pSiblingModule;
				break;
			}

			prev = &(*prev)->m_pSiblingModule;
		}

		m_pParentModule->Release();
	}
}

//-----------------------------------------------------------------------
ScriptModule::ScriptModule(const char* name, ScriptModule* parentModule, StaticBuffer* buff):
	m_Name(name),
	m_pStatic(buff),
	m_pDebugInfo(NULL),
	m_pContext(NULL),
	m_pParentModule(parentModule),
	m_pChildModule(NULL),
	m_pSiblingModule(NULL)
{
	if(parentModule)
	{
		m_pSiblingModule = parentModule->m_pChildModule;
		parentModule->m_pChildModule = this;
	}

	m_pcClass = m_pClassPtr;
}

//-----------------------------------------------------------------------
VarType* ScriptModule::AddType(const char* name, VType type, int size)
{
	enf_assert(name != NULL);

	name = AddString(name);

	VarType* vt = FindType(name);

	if(vt != NULL && vt->Module == this)
		return vt;

	if(type == VARTYPE_CLASS)
	{
		vt = ENF_NEW Class(this, name, sizeof(Instance), NULL);
		m_Stats.m_iNumClass++;
	}
	else
		vt = ENF_NEW VarType(this, type, name);

	vt->Sizeof = size;
	vt->Index = m_VarTypes.GetCardinality();
	vt->Module = this;

	m_VarTypes.Insert(vt);
	m_VarTypesHash.Insert(vt->GetName(), vt->Index);

	if(type == VARTYPE_CLASS)
		m_ClassPointers.Insert(AllocVariable(vt, 1, true));
	else
		m_ClassPointers.Insert((Variable*)NULL);

	return vt;
}

//-----------------------------------------------------------------------
bool ScriptModule::AddClass(Class* cl, uint flags)
{
	enf_assert(cl != NULL);

	const VarType* vt = FindType(cl->GetName());
	if(vt != NULL && vt->Module == this)
		return false;

	cl->m_iFlags |= flags;
	cl->Index = m_VarTypes.GetCardinality();
	m_VarTypes.Insert(cl);
	m_VarTypesHash.Insert(cl->GetName(), cl->Index);

	m_ClassPointers.Insert(AllocVariable(cl, 1, true));
	m_Stats.m_iNumClass++;

	return true;
}

//-----------------------------------------------------------------------
void ScriptModule::r_BindInheritedClasses(Array<VarType*>& varTypes, Class* oldAncestor, Class* newAncestor)
{
	for(uint n = 0; n < varTypes.GetCardinality(); n++)
	{
		if(varTypes[n]->Type != VARTYPE_CLASS)
			continue;
		Class* clc = (Class*)varTypes[n];
		if(clc->TopSuper == oldAncestor)
			clc->TopSuper = newAncestor;
		if(clc->Super == oldAncestor)
		{
			clc->Super = newAncestor;

			if(clc->m_iFlags & (CF_FORWARD_DECLARATION|CF_BOUND))
			{
				r_BindInheritedClasses(varTypes, clc, clc);
			}
			else
			{
				Class* newClass = newAncestor->NewInheritedClass(clc->GetModule(), clc->GetName());

				newClass->m_Variables.Swap(clc->m_Variables);
				newClass->m_Functions.Swap(clc->m_Functions);
				newClass->m_VariablesHash.Swap(clc->m_VariablesHash);
				newClass->m_MethodsHash.Swap(clc->m_MethodsHash);
				newClass->VSizeof = clc->VSizeof;
				newClass->m_iFlags = clc->m_iFlags;
				newClass->Index = clc->Index;
				newClass->Constructor = clc->Constructor;
				newClass->Destructor = clc->Destructor;
				newClass->TopSuper = clc->TopSuper;
				newClass->Super = clc->Super;
				varTypes[n] = newClass;
				r_BindInheritedClasses(varTypes, clc, newClass);
				delete clc;
			}
		}
	}
}

//-----------------------------------------------------------------------
bool ScriptModule::BindClass(Class* cl, uint flags)
{
	enf_assert(cl != NULL);

	Class* oldClass = FindClass(cl->GetName());
	if(oldClass == NULL || cl->Module != oldClass->Module)
		return false;

	cl->m_Variables.Swap(oldClass->m_Variables);
	cl->m_Functions.Swap(oldClass->m_Functions);
	cl->m_VariablesHash.Swap(oldClass->m_VariablesHash);
	cl->m_MethodsHash.Swap(oldClass->m_MethodsHash);
	cl->VSizeof = oldClass->VSizeof;
	cl->m_iFlags |= flags | oldClass->m_iFlags | CF_BOUND;
	cl->Index = oldClass->Index;
	cl->Constructor = oldClass->Constructor;
	cl->Destructor = oldClass->Destructor;
	m_VarTypes[oldClass->Index] = cl;
	
	r_BindInheritedClasses(m_VarTypes, oldClass, cl);

	//if someone did forward declare it prior to this module...
	ScriptModule* parentModule = m_pParentModule;
	while(parentModule)
	{
		if(parentModule->m_VarTypes[oldClass->Index] == oldClass)
			parentModule->m_VarTypes[oldClass->Index] = cl;
		parentModule = parentModule->GetParentModule();
	}

	delete oldClass;
	return true;
}

//-----------------------------------------------------------------------
bool ScriptModule::IsClass(const Class* cl) const
{
	for(uint n = 0; n < m_VarTypes.GetCardinality(); n++)
	{
		if(m_VarTypes[n] == cl)
			return true;
	}
	if(m_pParentModule)
		return m_pParentModule->IsClass(cl);
	return false;
}

//-----------------------------------------------------------------------
Class* ScriptModule::FindClass(const char *name) const
{
	enf_assert(name != NULL);

	VarType* vt = FindType(name);

	if(vt == NULL || vt->Type != VARTYPE_CLASS)
		return NULL;

	return (Class*)vt;
}

//-----------------------------------------------------------------------
Funct* ScriptModule::GetFunction(const char *name) const
{
	enf_assert(name != NULL);

	for(const ScriptModule* mod = this; mod; mod = mod->m_pParentModule)
	{
		uint index = mod->GetFunctionIndex(name);
		if(index != INDEX_NOT_FOUND)
			return mod->GetFunction(index);
	}
	return NULL;
}

//-----------------------------------------------------------------------
Variable* ScriptModule::FoldConstant(Variable* v, int alloc)
{
	enf_assert(v != NULL);

	Variable* cv;
	if(m_ConstantsHash.Find(v, cv))
	{
		m_Stats.m_iFoldedConstant++;
		m_pStatic->FreeData(m_pStatic->Allocated() - alloc);
		return cv;
	}
	else
	{
		m_ConstantsHash.Insert(v, v);
		return v;
	}
}

//-----------------------------------------------------------------------
Variable* ScriptModule::AddConstant(VarType* type, const TToken *init, bool optimize)
{
	enf_assert(type != NULL);

	//exception
	if(type->Type == VARTYPE_VECTOR && init->Type == TT_NUMBER)
	{
		type = m_pContext->m_vtFloatType;
	}
	else
	{
		//figure out type, when we have void on left side
		if(type->Type == VARTYPE_VOID)
		{
			if(init->Type == TT_QUOTE)
			{
				type = m_pContext->m_vtStringType;
			}
			else if(init->Type == TT_NUMBER)
			{
				if(strchr(init->Text, '.'))
					type = m_pContext->m_vtFloatType;
				else
					type = m_pContext->m_vtIntType;
			}
		}
	}

//optimize often used constants
	if(optimize)
	{
		if(init->Type == TT_NUMBER && init->Text[1] == 0)
		{
			if(type->Type == VARTYPE_INT)
				return m_pContext->m_pvNumbersInt[init->Text[0] - '0'];

			if(type->Type == VARTYPE_FLOAT && init->Text[0] == '0')
				return m_pContext->m_pvZeroFloat;
		}
		else if(init->Type == TT_QUOTE)
		{
			if(init->Text[0] == 0)
				return m_pContext->m_pvZeroString;
		}
	}


	int alloc = m_pStatic->Allocated();
	Variable* v = AllocVariable(type, 0, true, VSPEC_CONST);

	if(!v->Set(init, v))
		return NULL;

	return FoldConstant(v, alloc);
}

//-----------------------------------------------------------------------
Variable* ScriptModule::AddConstantF(float value)
{
	if(value == 0.0f)
		return m_pContext->m_pvZeroFloat;

	if(value == 1.0f)
		return m_pContext->m_pvOneFloat;

	int alloc = m_pStatic->Allocated();

	Variable* v = AllocVariable(m_pContext->m_vtFloatType, 0, true, VSPEC_CONST);

	v->Float = value;

	return FoldConstant(v, alloc);
}

//-----------------------------------------------------------------------
Variable* ScriptModule::AddConstantI(int value)
{
	//optimize often used constants
	if(value >= 0 && value <= 9)
		return m_pContext->m_pvNumbersInt[value];

	int alloc = m_pStatic->Allocated();

	Variable* v = AllocVariable(m_pContext->m_vtIntType, 0, true, VSPEC_CONST);

	v->Int = value;

	return FoldConstant(v, alloc);
}

//-----------------------------------------------------------------------
Variable* ScriptModule::CreateClassVariable(const char* name, const char* type, int dim)
{
	VarType* vt = FindType(type);
	if(vt == NULL)
		return NULL;

	uint spec = dim > 1 ? VSPEC_PTR : 0;
	Variable* var = AllocVariable(vt, dim, false, spec|VSPEC_CLASSMEMBER);
	var->Name = AddString(name);
	return var;
}

//-----------------------------------------------------------------------
const char* ScriptModule::AddString(const char* str)
{
	const char* nstr = FindString(str);
	if(nstr == NULL)
	{
		nstr = m_pStatic->AddString(str);
	}

	return nstr;
}

//-----------------------------------------------------------------------
const char* ScriptModule::FindString(const char* str) const
{
	for(const ScriptModule* mod = this; mod; mod = mod->m_pParentModule)
	{
		const char* nstr = mod->m_pStatic->FindString(str);
		if(nstr != NULL)
			return nstr;
	}
	return NULL;
}

//-----------------------------------------------------------------------
void ScriptModule::Cleanup()
{
	SAFE_DELETE(m_pDebugInfo);

	//free global strings
	for(uint n = 0; n < m_Variables.GetCardinality(); n++)
		m_Variables[n]->Cleanup(NULL);
	m_Variables.Clear();

	//free local strings, and functions itself
	for(uint f = 0; f < m_Functions.GetCardinality(); f++)
	{
		delete m_Functions[f];
	}
	m_Functions.Clear();

	//free cached instances & free class method locals
	for(int n = (int)m_VarTypes.GetCardinality() - 1; n >=0; n--)
	{
		if(m_VarTypes[n]->Type == VARTYPE_CLASS)
		{
			Class *cl = (Class*)m_VarTypes[n];

			Instance *inst, *ninst;

			for(inst = cl->Cached; inst; inst = ninst)
			{
				ninst = inst->m_pNextCached;
				m_pContext->FreeInstance(inst, false);
			}

			cl->NumCached = 0;

			for(uint f = 0; f < cl->GetNumMethods(); f++)
			{
				Funct* fn = cl->GetMethod(f);
				//if we are not inherited, delete function
				const Class* pcl;
				for(pcl = cl->GetSuper(); pcl; pcl = pcl->GetSuper())
				{
					if(pcl->GetNumMethods() > f && pcl->GetMethod(f) == fn)
						break;
				}
				if(pcl == NULL)
				{
					delete fn;
				}
			}
		}
	}
	ForEach<DeleteItem>(m_VarTypes);
	
	m_FunctionsHash.Clear();
	m_VariablesHash.Clear();
	m_VarTypesHash.Clear();
	m_VarTypes.Clear();
	m_ConstantsHash.Clear();
	m_ClassPointers.Clear();
	m_Refs.Clear();
	m_FunctRefs.Clear();
}


//-----------------------------------------------------------------------
Variable* ScriptModule::GetVariable(const char *name) const
{
	enf_assert(name != NULL);

	name = FindString(name);
	if(name == NULL)
		return NULL;

	for(const ScriptModule* mod = this; mod; mod = mod->m_pParentModule)
	{
		uint varIndex;
		if(mod->m_VariablesHash.Find(name, varIndex))
			return mod->m_Variables[varIndex];
	}

	return NULL;
}

//-----------------------------------------------------------------------
uint ScriptModule::GetVariableIndex(const char *name) const
{
	enf_assert(name != NULL);

	name = FindString(name);
	if(name != NULL)
	{
		uint varIndex;
		if(m_VariablesHash.Find(name, varIndex))
			return varIndex;
	}
	return INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------
void ScriptModule::AddVariable(Variable* var)
{
	uint varIndex = m_Variables.Insert(var);
	if(var->Name)
		m_VariablesHash.Insert(var->Name, varIndex);

}

//-----------------------------------------------------------------------
Variable* ScriptModule::AddVariable(const char* name, VarType* type, int dim, short spec)
{
	if(name)
	{
		name = AddString(name);
	}
	spec |= VSPEC_GLOBAL;

	if(type->Type == VARTYPE_VECTOR)
	{
		if((dim * VEC_SIZE * type->Sizeof) > 131072)
			return NULL;
	}
	else
	{
		if((dim * type->Sizeof) > 131072)
			return NULL;
	}

	if(name)
	{
		Variable* var = GetVariable(name);
		if(var != NULL)
			return var;
	}

	Variable* var = AllocVariable(type, dim, true, spec);
	var->Name = name;

	AddVariable(var);

	m_Stats.m_iNumVariable++;
	return var;
}

//-----------------------------------------------------------------------
uint ScriptModule::GetFunctionIndex(const char* name) const
{
	enf_assert(name != NULL);

	name = FindString(name);
	if(name == NULL)
		return SCRIPT_INDEX_NOT_FOUND;

	uint index;
	if(m_FunctionsHash.Find(name, index))
		return index;

	return SCRIPT_INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------
VarType* ScriptModule::r_FindTypeInChildren(const char* name) const
{
	for(ScriptModule* mod = m_pChildModule; mod; mod = mod->m_pSiblingModule)
	{
		uint vt;
		if(mod->m_VarTypesHash.Find(name, vt))
			return mod->m_VarTypes[vt];

		VarType* ctyp = mod->r_FindTypeInChildren(name);
		if(ctyp) return ctyp;
	}
	return NULL;
}

//-----------------------------------------------------------------------
VarType* ScriptModule::FindType(const char* type, bool inChildren) const
{
	enf_assert(type != NULL);

	const char* name = FindString(type);
	if(name != NULL)
	{
		for(const ScriptModule* mod = this; mod; mod = mod->m_pParentModule)
		{
			uint vt;
			if(mod->m_VarTypesHash.Find(name, vt))
				return mod->m_VarTypes[vt];
		}
		if(inChildren)
			return r_FindTypeInChildren(name);
	}
	return NULL;
}

//-----------------------------------------------------------------------
uint ScriptModule::AddFunction(Funct* func)
{
	enf_assert(m_Functions.GetCardinality() < 0x1fff);		
	uint index = m_Functions.Insert(func);
	m_FunctionsHash.Insert(func->GetName(), index);
	return index;
}

//-----------------------------------------------------------------------
Funct* ScriptModule::LinkFunction(const char* name, SCAPI_FUNC func)
{
	enf_assert(name != NULL);

	Funct* fn = GetFunction(name);

	if(fn) fn->Func = func;
	return fn;
}

//-----------------------------------------------------------------------
Funct* ScriptModule::LinkMethod(const char* className, const char* methodName, SCAPI_METHOD method)
{
	enf_assert(className != NULL);
	enf_assert(methodName != NULL);


	Funct* fn = NULL;

	Class* cl = (Class*)FindType(className);
	if(cl == NULL)
		return NULL;

	if(cl->Type == VARTYPE_CLASS)
	{
		fn = cl->GetMethod(methodName);

		if(fn && !(fn->Flags & (FN_NATIVE|FN_INTERN)))
			return NULL;
	}

	if(fn) fn->Method = method;
	return fn;
}

}
