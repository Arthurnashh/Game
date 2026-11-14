//-----------------------------------------------------------------------------
// File: enf_scriptcompiler.cpp
//
// Desc: Script Compiler
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "script/enf_scriptmain.h"
#include "script/enf_scriptcontext.h"
#include "script/enf_scriptinternal.h"
#include "common/enf_parser.h"
#include "common/enf_staticbuffer.h"
#include "script/enf_scriptcompiler.h"
#include "script/enf_scriptdebuginfo.h"
#include "enf_main.h"

namespace enf
{
//----------------------------------------------------------------------
// static variables
//----------------------------------------------------------------------

CParser*       CCompiler::m_pParser = NULL;
bool           CCompiler::m_bRes = false;
int				CCompiler::m_iSLen[OP_PARM3];
bool				CCompiler::m_bInitialized = false;

//----------------------------------------------------------------------

//----------------------------------------------------------------------
//          Error messages
//----------------------------------------------------------------------
const char  CCompiler::NotInBlockError[]  = "Unknown type or code outside function";
const char  CCompiler::BlockError[]    = "Block { } error";
const char  CCompiler::ParmError[]        = "Not enough parameters";
const char  CCompiler::TooMuchParmError[]= "Too much parameters";
const char  CCompiler::BadVarError[]      = "Bad type '%s'";
const char  CCompiler::NotVarError[]      = "Can't find variable '%s'";
const char  CCompiler::AddVarError[]      = "Can't create variable '%s'";
const char  CCompiler::MultDecError[]     = "Multiple declaration of function '%s'";
const char  CCompiler::BadStatError[]     = "Unknown statement";
const char  CCompiler::IncParmError[]     = "Incompatible parameter '%s'";
const char  CCompiler::OperatorError[] = "Unknown operator '%s'";
const char  CCompiler::ElseError[]        = "Misplaced ELSE";
const char  CCompiler::DupLabelError[] = "Duplicate label '%s'";
const char  CCompiler::Reference2Error[]  = "GOTO '%s' without label";
const char  CCompiler::ProtoError[]    = "Prototype error '%s'";
const char  CCompiler::ExtendsError[]     = "Can't inherit class '%s'";
const char  CCompiler::ClassExistsError[]= "Class '%s' already exists";
const char  CCompiler::FuncNotCompError[]= "Overloaded function '%s' not compatible";
const char  CCompiler::DestructorError[] = "Destructor '%s' must not have parameters";
const char  CCompiler::BreakContError[]   = "Misplaced break/case/continue";
const char  CCompiler::InitVarError[]     = "Can't initialize class variable";
const char  CCompiler::TooManyValsError[]= "Too many values";
const char  CCompiler::NotClassPtrError[]= "Variable '%s' is not a class pointer";
const char  CCompiler::NotRetValueError[]= "Function must return value";
const char  CCompiler::ConstModifyError[]= "Trying to modify write protected variable '%s'";
const char  CCompiler::ThisError[]= "'this' or 'super' outside class is not valid";
const char  CCompiler::ThisStaticError[]= "'this' or 'super' intside static method is not valid";
const char  CCompiler::LocalVarError[] = "Local variable '%s' outside function";
const char  CCompiler::MixedArithmetic[] = "Mixed value and array";
const char  CCompiler::MethodIsPrivate[] = "Method '%s' is private";
const char  CCompiler::UndefinedFunction[] = "Undefined function '%s'";
extern bool CLF;  //line feed

//-----------------------------------------------------------------------
void CCompiler::Scope::AddStat(int t, Stat* st)
{
	for(int n = 0; n < 8; n++)
	{
		if(RStat[t][n] == NULL)
		{
			RStat[t][n] = st;
			return;
		}
	}

	CCompiler::m_pParser->LError("Formula too complex");
	CCompiler::m_bRes = false;
}

//-----------------------------------------------------------------------
Stat *CCompiler::AddStat(OpCode cmd, int size)
{
	Stat* st = m_psStats;

	//it's time for another breakpoint
	if(m_pDLine && cmd != OP_DEBUG)
	{
		if(m_pModule->m_pDebugInfo)
		{
			//add debug info
			uint dline = m_pModule->m_pDebugInfo->AddDebugLine(
				m_pParser->GetModule(m_pDLine->m_iModule)->GetName(),
				m_pDLine->m_iSourceLine,
				((char*)st - (char*)m_Stats));
		}
		m_pDLine = NULL;
	}

	//huh?
	if(cmd == OP_NONE)
		return NULL;

	if(cmd == OP_RTS && IsScopeType(REF_FUNC))
	{
		m_pLastRts = st;
	}

	st->SetOpcode(cmd);
	
	if(size == 0)
	{
		st->SetSizeof(m_iSLen[cmd]);
		m_psStats = (Stat*)((char *)m_psStats + m_iSLen[cmd]);
	}
	else
	{
		st->SetSizeof(size);
		m_psStats = (Stat*)((char *)m_psStats + size);
	}

	st->SetOffset(0);
	st->Op[0] = 0;
	st->Op[1] = 0;
	st->Op[2] = 0;

	m_iNumStats++;
	return st;
}

//-----------------------------------------------------------------------
void CCompiler::FreeAllRegisters()
{
	m_iVRegMask = 0;
	m_iSRegMask = 0;
	m_iCRegMask = 0;
	m_iFRegMask = 0;
	m_iRRegMask = 0;
	for(uint n = 0; n < MAX_REGS; n++)
		m_pContext->m_vCRegs[n].SetType(m_pContext->m_vtClassType);
}

//-----------------------------------------------------------------------
void CCompiler::FreeRegister(Variable* reg)
{
	if(!IsRegister(reg))
		return;

	uint index = atoi(reg->GetName() + 2);
	uint mask = ~(1 << index);

	switch(reg->GetName()[1])
	{
	case 'v':
		m_iVRegMask &= mask;
		break;
	case 's':
		m_iSRegMask &= mask;
		break;
	case 'c':
		m_pContext->m_vCRegs[index].SetType(m_pContext->m_vtClassType);
		m_iCRegMask &= mask;
		break;
	case 'f':
		m_iFRegMask &= mask;
		break;
	case 'm':
		m_iRRegMask &= mask;
		break;
	}
}

uint maxreg = 0;

//-----------------------------------------------------------------------
Variable *CCompiler::GetRegister(VarType* vt)
{
	switch(vt->Type)
	{
	case VARTYPE_VECTOR:
		for(uint n = 0; n < MAX_REGS; n++)
		{
			maxreg = enf_max(maxreg, n);
			if(!(m_iVRegMask & (1 << n)))
			{
				m_iVRegMask |= (1 << n);
				return &m_pContext->m_vVRegs[n];
			}
		}
		break;

	case VARTYPE_STRING:
		for(uint n = 0; n < MAX_REGS; n++)
		{
			maxreg = enf_max(maxreg, n);
			if(!(m_iSRegMask & (1 << n)))
			{
				m_iSRegMask |= (1 << n);
				return &m_pContext->m_vSRegs[n];
			}
		}
		break;

	case VARTYPE_CLASS:
		for(uint n = 0; n < MAX_REGS; n++)
		{
			maxreg = enf_max(maxreg, n);
			if(!(m_iCRegMask & (1 << n)))
			{
				m_iCRegMask |= (1 << n);
				m_pContext->m_vCRegs[n].SetType(vt);
				return &m_pContext->m_vCRegs[n];
			}
		}
		break;

	case VARTYPE_FLOAT:
		for(uint n = 0; n < MAX_REGS; n++)
		{
			maxreg = enf_max(maxreg, n);
			if(!(m_iFRegMask & (1 << n)))
			{
				m_iFRegMask |= (1 << n);
				return &m_pContext->m_vFRegs[n];
			}
		}
		break;

	default:
		for(uint n = 0; n < MAX_REGS; n++)
		{
			maxreg = enf_max(maxreg, n);
			if(!(m_iRRegMask & (1 << n)))
			{
				m_iRRegMask |= (1 << n);
				m_pContext->m_vRegs[n].TypeType = vt->Type;
				m_pContext->m_vRegs[n].TypeIndex = vt->Index;
				return &m_pContext->m_vRegs[n];
			}
		}
		break;
	}

	m_bRes = false;
	static Variable error_reg;
	error_reg.Module = m_pModule;
	m_pParser->LError("Formula too complex");

	return &error_reg;
}

//-----------------------------------------------------------------------
Variable *CCompiler::AddRefVar(Variable* v, Instance** ptr)
{
	enf_assert(ptr);
	CopyMem(&m_RefVars[m_iNumRefVars], v, sizeof(Variable));
	m_RefVars[m_iNumRefVars].OrgVar = v;
	m_RefVars[m_iNumRefVars].IClass = (void**)ptr;
	return &m_RefVars[m_iNumRefVars++];
}

//-----------------------------------------------------------------------
CCompiler::Scope *CCompiler::FindScope(ScopeType type)
{
	if(type != REF_NULL)
	{
		for(int n = m_iNumScopes; n >= 0; n--)
		{
			if(m_rRefs[n].Type == type)
				return &m_rRefs[n];
		}
	}
	else //just find first while/switch/for ref
	{
		for(int n = m_iNumScopes; n >= 0; n--)
		{
			if(m_rRefs[n].Type == REF_WHILE || m_rRefs[n].Type == REF_SWITCH)
				return &m_rRefs[n];
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------
void CCompiler::AddBreakRef(CCompiler::Scope* sc, Stat* st)
{
	if(m_BRefs.IsFull())
	{
		m_bRes = false;
		m_pParser->LError("Too much breaks per scope");
		return;
	}

	m_BRefs.Insert(st);
	sc->NumBRefs++;
}

//-----------------------------------------------------------------------
CCompiler::Scope *CCompiler::BeginScope(ScopeType type)
{
	m_iNumScopes++;
	m_rRefs[m_iNumScopes].Type = type;
	m_rRefs[m_iNumScopes].BRefs = m_BRefs.GetArray() + m_BRefs.GetCardinality();
	m_rRefs[m_iNumScopes].NumBRefs = 0;
	m_rRefs[m_iNumScopes].Stat2 = NULL;
	m_rRefs[m_iNumScopes].Var = NULL;
	m_rRefs[m_iNumScopes].Lines = 0;
	m_rRefs[m_iNumScopes].Name = NULL;
	ZeroMem(&m_rRefs[m_iNumScopes].RStat, sizeof(m_rRefs[m_iNumScopes].RStat));

	return &m_rRefs[m_iNumScopes];
}

//-----------------------------------------------------------------------
CCompiler::Scope *CCompiler::EndScope()
{
	Scope* rref = &m_rRefs[m_iNumScopes--];

	//add lines inside block to parent scope
	if(rref->Type == REF_BLOCK)
		m_rRefs[m_iNumScopes].Lines += rref->Lines;

	return rref;
}

//-----------------------------------------------------------------------
bool CCompiler::EndOfValue()
{
	if(m_pParser->EndOfLine())
		return true;

	const TToken* par = m_pParser->GetToken(m_pParser->CParm);

	return (
		par->Text == CParser::TSeparator ||
		par->Text == CParser::TComma ||
		par->Text == CParser::TRBracket ||
		par->Text == CParser::TRSBracket ||
		par->Text == CParser::TOrOr ||
		par->Text == CParser::TAndAnd
		);
}

//-----------------------------------------------------------------------
Class *CCompiler::AddClass(const char *name)
{
	enf_assert(name);

	//add to vartypes
	Class* cl = (Class *)m_pModule->AddType(name, VARTYPE_CLASS, sizeof(void *));
	return cl;
}

//-----------------------------------------------------------------------
bool CCompiler::ExtendClass(Class* cl, Class* pc)
{
	enf_assert(pc);

	cl->MaxCached = pc->MaxCached;

	//avoid ambiguity between types and variables
	if(pc->GetVariable(cl->Name) != NULL)
		return false;

	//add parent variables
	cl->m_Variables = pc->m_Variables;

	//add parent functions
	cl->m_Functions = pc->m_Functions;

	cl->ISizeof = pc->ISizeof;
	cl->Super = pc;

	while(pc->Super)
		pc = pc->Super;

	cl->TopSuper = (pc == NULL) ? cl : pc;
	return true;
}

//-----------------------------------------------------------------------
const char* CCompiler::FindString(const char* name) const
{
	return m_pModule->FindString(name);
}

//-----------------------------------------------------------------------
const char* CCompiler::AddString(const char* name) const
{
	return m_pModule->AddString(name);
}

//-----------------------------------------------------------------------
Variable* CCompiler::FindVariable(const char *name,const Funct* func,const Class* cl, bool onlyclass)
{
	enf_assert(name);
	name = FindString(name);
	if(name == NULL)
		return NULL;

	//optimize
	if(name == m_pContext->m_pvThis->Name)
	{
		if(cl == NULL)
		{
			m_pParser->LError(ThisError);
			return NULL;
		}
		if(m_pCFunct && m_pCFunct->Flags & FN_STATIC)
		{
			m_pParser->LError(ThisStaticError);
			return NULL;
		}

		return m_pContext->m_pvThis;
	}

	if(!onlyclass && func)
	{
		//At first, try to search local variables
		for(uint n = 0; n < func->GetNumVariables(); n++)
		{
			if(name == func->Variables[n]->Name)
				return func->Variables[n];
		}
	}

	//if we are in class, search first for locals
	if(cl)
	{
		Variable* v = cl->GetVariable(name);

		if(v)
		{
			if(v->IsConst())
				return v;
			else
				return AddRefVar(v, &m_pContext->m_pvThis->InstancePtr);
		}
	}

	if(!onlyclass)
	{
		//try to search global variables
		return m_pModule->GetVariable(name);
	}
	return NULL;
}

//-----------------------------------------------------------------------
Funct *CCompiler::FindFunction(const char *name, Class *cl, bool onlyclass)
{
	enf_assert(name);

	m_pObj = NULL;
	m_iFunctionIndex = 0;

	//if we are in class, search first for locals.
	if(cl)
	{
		uint index = cl->GetMethodIndex(name);
		if(index != INDEX_NOT_FOUND)
		{
			m_pObj = m_pContext->m_pvThis;
			m_iFunctionIndex = index + 1;
			return cl->GetMethod(index);
		}
	}

	//none, search global functions
	if(!onlyclass)
	{
		for(ScriptModule* mod = m_pModule; mod; mod = mod->m_pParentModule)
		{
			if(mod->m_FunctionsHash.Find(name, m_iFunctionIndex))
			{
				m_iFunctionIndex += 1;
				return mod->m_Functions[m_iFunctionIndex - 1];
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------
bool CCompiler::CheckFunctionCompatibility(Funct *fn1, Funct *fn2)
{
	if(fn1->ParmNum < fn2->GetNumParameters() ||
		fn1->ReturnValue->GetTypeIndex() != fn2->ReturnValue->GetTypeIndex())
	{
		m_pParser->LError(FuncNotCompError, fn1->GetName());
		return false;
	}

	for(uint n = 0; n < fn1->GetNumParameters() &&  n < fn2->GetNumParameters(); n++)
	{
		if(fn1->Variables[n]->GetVType() != fn2->Variables[n]->GetVType())
		{
			m_pParser->LError(FuncNotCompError, fn1->Variables[n]->Name);
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------
bool CCompiler::CheckClassCompatibility(Class *cl)
{
	for(Class* scl = cl->Super; scl != NULL; scl = scl->Super)
	{
		//Check contructor
		if(cl->Constructor && scl->Constructor)
		{
			if(!CheckFunctionCompatibility(cl->GetMethod(cl->Constructor - 1),
													scl->GetMethod(scl->Constructor - 1)))
				return false;
		}

		//Check destructor
		if(cl->Destructor && scl->Destructor)
		{
			if(!CheckFunctionCompatibility(cl->GetMethod(cl->Destructor - 1),
													scl->GetMethod(scl->Destructor - 1)))
				return false;

			if(cl->GetMethod(cl->Destructor - 1)->GetNumParameters() != 0 ||
				scl->GetMethod(scl->Destructor - 1)->GetNumParameters() != 0)
			{
				m_pParser->LError(DestructorError, cl->GetMethod(cl->Destructor - 1)->GetName());
				return false;
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------
Funct* CCompiler::AddFunction(const char* name, uint type, Class* cl)
{
	enf_assert(name);

	name = AddString(name);

	Funct* nfunc = ENF_NEW Funct(type, name);

	if(cl)
	{
		Funct* func = FindFunction(name, cl, cl != NULL);

		if(func == NULL)
		{
			cl->AddMethod(nfunc);
		}
		else
		{
/*			if(cl->m_Functions[m_iFunctionIndex - 1] != NULL)
			{
				enf_assert(cl->GetTopSuper() == NULL || cl->m_Functions[m_iFunctionIndex - 1] == cl->GetSuper()->m_Functions[m_iFunctionIndex - 1]);
			}*/
			nfunc->ReturnValue = func->ReturnValue;
			cl->m_Functions[m_iFunctionIndex - 1] = nfunc;
		}

		//isn't it constructor/destructor ?
		if(name[0] == '~' && !strcmp(name + 1, cl->Name))
		{
			cl->Destructor = cl->GetNumMethods();
		}
		else
		{
			if(name == cl->Name)
			{
				cl->Constructor = cl->GetNumMethods();
			}
		}
	}
	else
	{
		m_pModule->AddFunction(nfunc);
	}

	nfunc->FirstStat = m_Stats;
	nfunc->Module = m_pModule;

	m_psStats = m_Stats;

	if(m_pModule->m_pDebugInfo)
		m_iFirstDebugLine = m_pModule->m_pDebugInfo->m_Lines.GetCardinality();

	m_pModule->m_Stats.m_iNumFunction++;

	return nfunc;
}

//-----------------------------------------------------------------------
Variable *CCompiler::AddVariable(const char* name, const VarType* type, Funct* func,Class* cl, int dim, short spec)
{
	if(name)
		name = AddString(name);

	if(func == NULL && cl == NULL)
		spec |= VSPEC_GLOBAL;

//override type?
	if(cl && func == NULL)
	{
		Variable* var = cl->GetVariable(name);

		if(var)
		{
			if(var->GetVType() != type->Type)
			{
				m_pParser->LError("Variable '%s' type mismatch", name);
				return NULL;
			}
			else
			{
				enf_assert(type);
				var->TypeIndex = type->Index;
				return var;
			}
		}
	}

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


	Variable* var = m_pModule->AllocVariable(type, dim, !(spec & VSPEC_CLASSMEMBER), spec);
	var->Name = name;

	if(func)
	{
		func->AddVariable(var);
	}
	else
	{
		if(cl)
		{
			cl->AddVariable(var);
		}
		else
		{
			m_pModule->AddVariable(var);
		}
	}

	m_pModule->m_Stats.m_iNumVariable++;
	return var;
}

//-----------------------------------------------------------------------
void Stat::SetJmp(const Stat *dest)
{
	SetOffset(((intptr_t)dest - (intptr_t)this) >> 2);
}

//-----------------------------------------------------------------------
void* CCompiler::EncodeParm(const Variable* var)
{
	if(IsRefVar(var))
	{
		const TVariable* tvar = (const TVariable*)var;

		int index = tvar - m_RefVars;

		if(index >= 0 && index < MAX_REFS)
		{
			uint n;

			//encode to reference table
			for(n = m_iLastRef; n < m_pModule->m_Refs.GetCardinality(); n++)
			{
				if(m_pModule->m_Refs[n].m_pInst == (Instance**)tvar->IClass &&
					m_pModule->m_Refs[n].m_pVar == tvar->OrgVar)
					break;
			}

			if(n == m_pModule->m_Refs.GetCardinality())
			{
				enf_assert(m_pModule->m_Refs.GetCardinality() < ScriptModule::MAX_REFS);

				ScriptModule::IRef& ref = m_pModule->m_Refs.Append();
				ref.m_pInst = (Instance**)tvar->IClass;
				ref.m_pVar = tvar->OrgVar;
				enf_assert(ref.m_pVar && tvar->IClass);
			}
			return (void*)n;
		}
	}

	return (void*)var;
}

//-----------------------------------------------------------------------
void CCompiler::SetParm(Stat* st, int a, void* parm, bool immed)
{
	//do we have space for it?
	enf_assert(((const char*)st + st->GetSizeof()) >= (const char*)&st->Op[a + 1]);

	if(!immed && parm != NULL)
	{
		st->Op[a] = EncodeParm((Variable*)parm);
		return;
	}

	st->Op[a] = parm;
}

//-----------------------------------------------------------------------
void CCompiler::Push(Variable *var)
{
	Stat* st;
	uint arraySize = 1;
	if(var->IsPtr())
	{
		arraySize = var->ArraySize();
		if(var->GetVType() == VARTYPE_VECTOR)
			arraySize /= 3;
	}

	switch(var->GetVType())
	{
	case VARTYPE_STRING:
		st = AddStat(OP_PUSHS);
		break;
	case VARTYPE_VECTOR:
		st = AddStat(OP_PUSHV);
		break;
	case VARTYPE_CLASS:
		st = AddStat(OP_PUSHC);
		break;
	default:
		enf_assert(var->GetVType() == VARTYPE_INT || var->GetVType() == VARTYPE_FLOAT || var->GetVType() == VARTYPE_CLASSREF);
		st = AddStat(OP_PUSH);
		break;
	}

	st->SetOffset(arraySize);
	SetParm(st, 0, var);
}

//-----------------------------------------------------------------------
void CCompiler::Pop(Variable *var)
{
	Stat* st;
	uint arraySize = 1;
	if(var->IsPtr())
	{
		arraySize = var->ArraySize();
		if(var->GetVType() == VARTYPE_VECTOR)
			arraySize /= 3;
	}

	switch(var->GetVType())
	{
	case VARTYPE_STRING:
		st = AddStat(OP_POPS);
		break;
	case VARTYPE_VECTOR:
		st = AddStat(OP_POPV);
		break;
	case VARTYPE_CLASS:
		st = AddStat(OP_POPC);
		break;
	default:
		enf_assert(var->GetVType() == VARTYPE_INT || var->GetVType() == VARTYPE_FLOAT || var->GetVType() == VARTYPE_CLASSREF);
		st = AddStat(OP_POP);
		break;
	}
	st->SetOffset(arraySize);
	SetParm(st, 0, var);
}

//-----------------------------------------------------------------------
void CCompiler::BeforeCall(Funct* fn)
{
	if(m_pCFunct && (fn == NULL || (fn->Flags & (FN_VOLATILE|FN_SCRIPT))))
	{
		for(uint n = 0; n < m_pCFunct->GetNumVariables(); n++)
		{
			if(m_pCFunct->Variables[n]->Spec & VSPEC_LOCAL)
				Push(m_pCFunct->Variables[n]);
		}
	}
}

//-----------------------------------------------------------------------
void CCompiler::AfterCall(Funct* fn)
{
	if(m_pCFunct && (fn == NULL || (fn->Flags & (FN_VOLATILE|FN_SCRIPT))))
	{
		for(int n = (int)m_pCFunct->GetNumVariables() - 1; n >= 0; n--)
		{
			if(m_pCFunct->Variables[n]->Spec & VSPEC_LOCAL)
				Pop(m_pCFunct->Variables[n]);
		}
	}

}

//-----------------------------------------------------------------------
struct ConvertDef
{
	VType		Type1;
	bool		Index1;
	VType		Type2;
	bool		Index2;
	OpCode	Op;
	bool		Convert;
	bool		Force;
};

static const ConvertDef Converts[] =
{
	VARTYPE_FLOAT, false,VARTYPE_INT,   false,OP_CITF, true, false,
	VARTYPE_INT,   false,VARTYPE_FLOAT, false,OP_CFTI, true, false,
	VARTYPE_INT,   false,VARTYPE_INT,   false,OP_MOVE, false, false,
	VARTYPE_FLOAT, false,VARTYPE_FLOAT, false,OP_MOVE, false, false,
	VARTYPE_STRING,false,VARTYPE_STRING,false,OP_MOVES,false, false,
	VARTYPE_VECTOR,false,VARTYPE_VECTOR,false,OP_MOVEV,false, false,
	VARTYPE_CLASS, false,VARTYPE_CLASS, false,OP_MOVE, false, false,
	VARTYPE_CLASSREF, false,VARTYPE_CLASSREF, false,OP_MOVE, false, false,
	VARTYPE_CLASS, false,VARTYPE_INT,   false,OP_MOVE, false, false,

	VARTYPE_CLASS, false,VARTYPE_CLASSREF, false,OP_MOVEFR, true, false,
	VARTYPE_CLASSREF, false,VARTYPE_CLASS, false,OP_MOVETR, true, false,

	VARTYPE_USER0,false,VARTYPE_USER0,false,OP_MOVE,false, false,
	VARTYPE_USER1,false,VARTYPE_USER1,false,OP_MOVE,false, false,
	VARTYPE_USER2,false,VARTYPE_USER2,false,OP_MOVE,false, false,
	VARTYPE_USER3,false,VARTYPE_USER3,false,OP_MOVE,false, false,
	VARTYPE_USER4,false,VARTYPE_USER4,false,OP_MOVE,false, false,
	VARTYPE_USER5,false,VARTYPE_USER5,false,OP_MOVE,false, false,
	VARTYPE_USER6,false,VARTYPE_USER6,false,OP_MOVE,false, false,
	VARTYPE_USER7,false,VARTYPE_USER7,false,OP_MOVE,false, false,

	VARTYPE_FUNCPTR,false,VARTYPE_INT,false,OP_MOVE,false, false,
	VARTYPE_FUNCPTR,false,VARTYPE_FUNCPTR,false,OP_MOVE,false, false,

//achjo :-(
//	VARTYPE_VOBJECT,false,VARTYPE_INT,false,OP_MOVE,false, false,
//	VARTYPE_INT,false,VARTYPE_VOBJECT,false,OP_MOVE,false, false,

	VARTYPE_INT,   false,VARTYPE_CLASS, false,OP_MOVE, true, true,

//fast vector operations
	VARTYPE_INT,   false,VARTYPE_INT,   true, OP_FFV,  true, false,
	VARTYPE_INT,   true, VARTYPE_INT,   false,OP_FTV,  true, false,

	VARTYPE_FLOAT, false,VARTYPE_FLOAT, true, OP_FFV,  true, false,
	VARTYPE_FLOAT, true, VARTYPE_FLOAT, false,OP_FTV,  true, false,

	VARTYPE_FLOAT, false,VARTYPE_VECTOR,true, OP_FFV,  true, false,
	VARTYPE_VECTOR,true, VARTYPE_FLOAT, false,OP_FTV,  true, false,

//vector operations
	VARTYPE_NONE,  false,VARTYPE_NONE,  true, OP_CFV,  true, false,
	VARTYPE_NONE,  true, VARTYPE_NONE,  false,OP_CTV,  true, false,


	VARTYPE_STRING,false,VARTYPE_INT,   false,OP_MOVE, false, false
};

struct SignDef
{
	const char*	Sign;
	VType			Type;
	OpCode		Op;
};

const SignDef* Signs;
uint NumSigns = 0;

//-----------------------------------------------------------------------
OpCode CCompiler::FindSign(const TToken* sign, VType type) const
{
	if(sign->Type == TT_DEF)
	{
		for(uint n = 0; n < NumSigns; n++)
		{
			if(Signs[n].Type != VARTYPE_NONE)
			{
				if(Signs[n].Type == type && Signs[n].Sign == sign->Text)
					return Signs[n].Op;
			}
			else
			{
				if(Signs[n].Sign == sign->Text)
					return Signs[n].Op;
			}
		}
	}

	return OP_NONE;
}

//-----------------------------------------------------------------------
OpCode CCompiler::Negate(OpCode op)
{
	switch(op)
	{
	case OP_BNE:
		return OP_BEQ;
	case OP_BLE:
		return OP_BGT;
	case OP_BGE:
		return OP_BLT;
	case OP_BGT:
		return OP_BLE;
	case OP_BLT:
		return OP_BGE;
	case OP_BEQ:
		return OP_BNE;
	}
	return OP_NONE;
}

//-----------------------------------------------------------------------
Variable *CCompiler::DoConversion(Variable *out,Variable *oidx,
								Variable *in,Variable *iidx,int move,bool force)
{
VType intype, outtype;
const ConvertDef* c;
Variable* temp;
Stat* stat;
int s;

	intype = in->GetVType();
	outtype = out->GetVType();
	c = Converts;
	if(outtype == VARTYPE_CLASS && intype == VARTYPE_CLASS)
	{
		if(!AreTypesRelated(out, in))
		{
			m_pParser->LError("Types '%s' and '%s' are unrelated", in->GetType()->GetName(), out->GetType()->GetName());
			return NULL;
		}
	}
	else
	{
		if(outtype == VARTYPE_CLASS && intype == VARTYPE_INT)
		{
			if(!(in->Spec & VSPEC_CONST) || in->Int != 0)
			{
				m_pParser->LError("Only NULL assignment to class pointer is allowed");
				return NULL;
			}
		}
	}
	for(s = 0; s < sizeof(Converts) / sizeof(ConvertDef); s++, c++)
	{
		if(c->Force && !force)
			continue;

		if((c->Index1 && oidx == NULL) ||
			(!c->Index1 && oidx != NULL))
			continue;

		if((c->Index2 && iidx == NULL) ||
			(!c->Index2 && iidx != NULL))
			continue;

		if(c->Type1 != VARTYPE_NONE &&
			c->Type1 != outtype)
			continue;

		if(c->Type2 != VARTYPE_NONE &&
			c->Type2 != intype)
			continue;

		if(c->Type1 == VARTYPE_STRING && c->Type2 == VARTYPE_INT)
		{
			//allow only string[] = NULL
			if(!(out->Spec & VSPEC_PTR) || !(in->Spec & VSPEC_CONST) || in->Int != 0)
				continue;
		}

		if(c->Type1 == VARTYPE_CLASSREF || c->Type2 == VARTYPE_CLASSREF)
		{
			if(in->GetName() == NULL)
			{
				m_pParser->LError("Cannot assign reference to temporal variable to '%s&'", out->GetName());
				return NULL;
			}
		}

		if(move)
		{
			if(iidx == NULL && oidx == NULL)
			{
				if(in->Spec & VSPEC_PTR && out->Spec & VSPEC_PTR)
				{
					if(c->Op == OP_MOVE || c->Op == OP_MOVES || c->Op == OP_MOVEI)
						stat = AddStat(OP_MOVEA);
					else if(c->Op == OP_MOVEV && in->Dim[1] != 0)
						stat = AddStat(OP_MOVEA);
					else
						m_PrevCStat = stat = AddStat(c->Op);
				}
				else
				{
					//optimize immediate moves a = const, but don't do it for classes (because of GC)
					if(m_iOptimize && c->Op == OP_MOVE && in->Spec & VSPEC_CONST && out->GetVType() != VARTYPE_CLASS)
					{
						stat = AddStat(OP_IMOVE);
						SetParm(stat, 0, (void*)in->Int, true);
						SetParm(stat, 1, out);
						return out;
					}
					else
					{
						//optimize:
						// cfv @xxx, vec[]
						// move var, @xxx
						// or:
						// cfv @xxx, vec[]
						// move var, @xxx
						// or:
						// citf @xxx, a
						// move b, @xxx
						// or:
						// cfti @xxx, a
						// move b, @xxx
						// or:
						// new @cxx, ...
						// move b, @cxx

						if(c->Op == OP_MOVE && m_iOptimize > 2 && in->Name && in->Name[0] == '@' && out->Name && out->Name[0] != '@')
						{
							//check if it's really following instruction
							if(m_PrevCStat && m_psStats == (Stat*)((char *)m_PrevCStat + m_PrevCStat->GetSizeof()) &&
								(void *)in == m_PrevCStat->Op[1])
							{
								switch(m_PrevCStat->GetOpcode())
								{
								case OP_CFV:
								case OP_FFV:
								case OP_CITF:
								case OP_CFTI:
								case OP_NEW:
								case OP_SIZEOF:
									SetParm(m_PrevCStat, 1, out);
									return out;
								}
							}
						}

						stat = AddStat(c->Op);
					}
				}
				SetParm(stat, 0, in);
				SetParm(stat, 1, out);
				FreeRegister(in);
				return out;
			}
			else
			{
				//optimize f = vec[const], vec[const] = f
				if(m_iOptimize)
				{
					if(c->Op == OP_FFV && iidx->Spec & VSPEC_CONST)
					{
						m_PrevCStat = stat = AddStat(OP_IFV);
						FreeRegister(in);
						SetParm(stat, 0, in);
						SetParm(stat, 1, out);
						stat->SetOffset(iidx->Int);
						return out;
					}
					else
					if(c->Op == OP_FTV && oidx->Spec & VSPEC_CONST)
					{
						m_PrevCStat = stat = AddStat(OP_ITV);
						FreeRegister(in);
						SetParm(stat, 0, in);
						SetParm(stat, 1, out);
						stat->SetOffset(oidx->Int);
						return out;
					}
				}
			}

/*			if(oidx && out->GetVType() == VARTYPE_CLASSREF)
			{
				Variable* temp = GetRegister(out->GetType());
				Stat* s = AddStat(OP_MOVETR);
				SetParm(s, 0, in);
				SetParm(s, 1, temp);
				in = temp;
			}
*/
			m_PrevCStat = stat = AddStat(c->Op);
			FreeRegister(in);
			SetParm(stat, 0, in);
			SetParm(stat, 1, out);
			SetParm(stat, 2, (iidx) ? iidx : oidx);
			return out;
		}

		if(c->Convert)
		{
			FreeRegister(in);
			temp = GetRegister(out->GetType());

			//optimize f = vec[const], vec[const] = f
			if(m_iOptimize)
			{
				if(c->Op == OP_FFV && iidx->Spec & VSPEC_CONST)
				{
					m_PrevCStat = stat = AddStat(OP_IFV);
					SetParm(stat, 0, in);
					SetParm(stat, 1, temp);
					stat->SetOffset(iidx->Int);
					return temp;
				}
				else
				if(c->Op == OP_FTV && oidx->Spec & VSPEC_CONST)
				{
					m_PrevCStat = stat = AddStat(OP_ITV);
					SetParm(stat, 0, in);
					SetParm(stat, 1, temp);
					stat->SetOffset(oidx->Int);
					return temp;
				}
			}
			if(iidx)
				FreeRegister(iidx);
			if(oidx)
				FreeRegister(oidx);

			m_PrevCStat = stat = AddStat(c->Op);
			/*if(c->Op == OP_MOVETR)
			{
				SetParm(stat, 0, in);
				SetParm(stat, 1, temp);
			}
			else if(c->Op == OP_MOVEFR)
			{
				SetParm(stat, 0, in);
				SetParm(stat, 1, temp);
			}
			else*/
			{
				SetParm(stat, 0, in);
				SetParm(stat, 1, temp);
			}
			if(stat->GetSizeof() > 12)
				SetParm(stat, 2, (iidx) ? iidx : oidx);
			return temp;
		}

		return in;
	}

	return NULL;
}

//-----------------------------------------------------------------------
int CCompiler::IsType()
{
	m_iCVSpec = 0;
	m_pCVarType = NULL;
	m_iFuncFlags = 0;

	while(true)
	{
		//filter out mess
		if(m_pParser->CToken->Type != TT_STRING)
		{
			if(m_iCVSpec == 0)
				return 0;
			else
			{
				m_pParser->LError(CParser::ParmParseError);
				return -1;
			}
		}

		if(TTEqual(*m_pParser->CToken, "volatile"))
		{
			m_iFuncFlags |= FN_VOLATILE;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		if(TTEqual(*m_pParser->CToken, "private"))
		{
			m_iFuncFlags |= FN_PRIVATE;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		if(TTEqual(*m_pParser->CToken, "native"))
		{
			m_iFuncFlags &= ~FN_INTERN;
			m_iFuncFlags |= FN_NATIVE;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		if(TTEqual(*m_pParser->CToken, "event"))
		{
			m_iFuncFlags |= FN_EVENT;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		if(TTEqual(*m_pParser->CToken, "owned"))
		{
			m_iCVSpec |= VSPEC_OWNED;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		if(TTEqual(*m_pParser->CToken, "out"))
		{
			m_iCVSpec |= VSPEC_OUT;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		if(TTEqual(*m_pParser->CToken, "inout"))
		{
			m_iCVSpec |= VSPEC_INOUT;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		if(TTEqual(*m_pParser->CToken, "local"))
		{
			m_iCVSpec |= VSPEC_LOCAL;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		if(TTEqual(*m_pParser->CToken, "static"))
		{
			m_iCVSpec |= VSPEC_STATIC;
			m_iFuncFlags |= FN_STATIC;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		if(TTEqual(*m_pParser->CToken, "const"))
		{
			m_iCVSpec |= VSPEC_CONST;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		if(TTEqual(*m_pParser->CToken, "reference"))
		{
			m_iCVSpec |= VSPEC_REFERENCE;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		if(TTEqual(*m_pParser->CToken, "protected"))
		{
			m_iCVSpec |= VSPEC_PROTECT;
			if(!m_pParser->NextToken(true))
				return -1;
			continue;
		}

		int ret = 0;
		if(m_iCVSpec == 0 && m_iFuncFlags == 0)
		{
			//can be type
			m_pCVarType = m_pModule->FindType(m_pParser->CToken->Text);
			ret = (m_pCVarType != NULL) ? 1 : 0;
		}
		else
		{
			//must be type!
			if((m_pCVarType = m_pModule->FindType(m_pParser->CToken->Text)) == NULL)
			{
				m_pParser->LError(BadVarError, m_pParser->CToken->Text);
				ret = -1;
			}
			else
				ret = 1;
		}
		if(ret == 1)
		{
			if(!m_pParser->NextToken(true))
				return -1;

			if(TTDefEqual(m_pParser->CToken, CParser::TAnd))
			{
				if(m_pCVarType->Type != VARTYPE_CLASS)
				{
					m_pParser->LError("Only class ref is supported. Not '%s'", m_pCVarType->Name);
					return -1;
				}
				if(!m_pParser->NextToken(true))
					return -1;
				m_pCVarType = m_pContext->m_vtClassRefType;
			}
		}
		return ret;
	}
}

//-----------------------------------------------------------------------
#define INITV_FALSE  false
#define INITV_TRUE   true
#define INITV_PARM   2

//-----------------------------------------------------------------------
bool CCompiler::ParseArraySize(int& dim)
{
	if(m_pParser->CToken->Type == TT_STRING)
	{
		Variable *v = FindVariable(m_pParser->CToken->Text, m_pCFunct, m_pCClass, false);

		if(!m_pParser->NextToken(true))
			return false;

		if(v && (v->Spec & VSPEC_CONST) && v->GetVType() == VARTYPE_INT)
		{
			dim = v->Int;

			if(dim < 1)
			{
				m_pParser->LError("Wrong array size");
				return false;
			}

			return true;
		}
	}

	if(m_pParser->AssertType(TT_NUMBER))
	{
		dim = atoi(m_pParser->CToken->Text);

		if(!m_pParser->NextToken(true))
			return false;
	}
	else
		return false;

	if(dim < 1)
	{
		m_pParser->LError("Wrong array size");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
bool CCompiler::ParseVarDef(int init, bool multiple, const VarType *vt, short vspec, int num, Variable *avar)
{
	if(vt == NULL)
	{
		if(IsType() != 1)
		{
			m_pParser->LError(CParser::ParmParseError);
			return false;
		}
		vt = m_pCVarType;
		vspec = m_iCVSpec;
	}

	bool global = (m_pCFunct == NULL);

	//when const, behave as for globals
	if(vspec & VSPEC_CONST)
		global = true;

	if(init == INITV_PARM)
	{
		global = true;
	}

	while(true)
	{
		int vvspec = vspec;

		//filter out mess
		if(m_pParser->CToken->Type != TT_STRING)
		{
			m_pParser->LError(CParser::ParmParseError);
			return false;
		}

		const char* vname = m_pParser->CToken->Text;
		
		//don't need to keep all names!
		if(init != INITV_PARM || m_pCFunct->Flags & FN_SCRIPT)
			vname = AddString(vname);

		//ambiguity between types and variables
		if(m_pModule->FindType(vname))
		{
			m_pParser->LError("Variable name already used as type name");
			return false;
		}

		Variable* var = FindVariable(vname, m_pCFunct, m_pCClass, (m_pCClass != NULL && m_pCFunct == NULL));

		//avoid multiplicity!
		if(var != NULL)
		{
			if(!(m_pCClass && IsRefVar(var)))
			{
				m_pParser->LError("Multiple declaration of variable '%s'", m_pParser->CToken->Text);
				return false;
			}
		}

		int dim = 0, dim2 = 1;
//TODO: Fix any number of dimensions
		//array definition
		if(m_pParser->NextToken())
		{
			//1st dimension
			if(TTDefEqual(m_pParser->CToken, CParser::TLSBracket))
			{
				if(!m_pParser->NextToken(true))
					return false;

				vvspec |= VSPEC_PTR;

				if(!TTDefEqual(m_pParser->CToken, CParser::TRSBracket))
				{
					if(!ParseArraySize(dim))
						return false;
				}
				else
				{
					//exception for vector ptr[]. Fake 2nd dimension as at least 1
					if(vt->Type == VARTYPE_VECTOR)
					{
						dim = 1;
					}
				}
				if(!m_pParser->AssertToken(CParser::TRSBracket))
					return false;
			}

			//2nd dimension
			if(TTDefEqual(m_pParser->CToken, CParser::TLSBracket))
			{
				dim2 = dim;

				if(!m_pParser->NextToken(true))
					return false;

				vvspec |= VSPEC_PTR;

				if(!TTDefEqual(m_pParser->CToken, CParser::TRSBracket))
				{
					if(!ParseArraySize(dim))
						return false;
				}
				if(!m_pParser->AssertToken(CParser::TRSBracket))
					return false;
			}
		}

		//add variable
		if((var = AddVariable((init == INITV_PARM && (m_pCFunct->Flags & FN_SCRIPT) == 0) ? NULL : vname, vt, m_pCFunct, m_pCClass, dim2 * dim, vvspec)) == NULL)
		{
			m_pParser->LError(AddVarError, vname);
			return false;
		}

		if(dim2 != 1)
		{
			var->Dim[1] = dim2;
			var->Dim[0] = dim;
		}

		Stat *st;

		if(vvspec & VSPEC_LOCAL)
		{
			//TODO: check constructors/destructors/exceptions - can not contain locals!!!!
			if(m_pCFunct == NULL)
			{
				m_pParser->LError(LocalVarError, var->Name);
				return false;
			}
		}

		if(m_pParser->EndOfLine())
			return true;

		if(!init && !(vspec & VSPEC_CONST))
		{
			if(TTDefEqual(m_pParser->CToken, CParser::TAssign))
			{
				m_pParser->LError(InitVarError);
				return false;
			}
		}

		uint idx = 0;
		Variable *var1,*idxv;

//initialize variable
		if(TTDefEqual(m_pParser->CToken, CParser::TAssign))
		{
			if(!m_pParser->NextToken(true))
				return false;
//initializing array
			if(TTDefEqual(m_pParser->CToken, CParser::TLCurly))
			{
				if(!global && (var->Spec & VSPEC_PTR))
				{
					idxv = GetRegister(m_pContext->m_vtIntType);
					st = AddStat(OP_IMOVE);
					SetParm(st, 0, 0, true);
					SetParm(st, 1, idxv);
				}

				if(!var->IsPtr())
				{
					m_pParser->LError(CParser::ParmParseError);
					return false;
				}

				m_pParser->NextToken();

				bool terminated = false;

				while(true)
				{
					while(!m_pParser->EndOfLine())
					{
						if(idx >= var->ArraySize())
						{
							m_pParser->LError(TooManyValsError);
							return false;
						}

						const char *csymb;

						if(m_pParser->CToken->Type == TT_STRING)
						{
							if((csymb = FindString(m_pParser->CToken->Text)) != NULL)
								m_pParser->CToken->Text = csymb;
						}
						else
							csymb = NULL;

						if(csymb && (var1 = FindVariable(csymb, m_pCFunct, m_pCClass)) != NULL)
						{
							if(var1->GetVType() != var->GetVType())
							{
								//m_pParser->LError(IncParmError, var1->Name);
								//return false;
								var1 = DoConversion(var, NULL, var1, NULL, false);

							}
							if(global)
								((int *)var->Pointer)[idx] = var1->Int;
						}
						else
						{
							if(!global)
							{
								var1 = m_pModule->AddConstant(var->GetType(),m_pParser->CToken);
								if(var1 == NULL)
								{
									m_pParser->LError(IncParmError, m_pParser->CToken->Text);
									return false;
								}
							}
							else
							{
								if(!var->Set(m_pParser->CToken, var, idx))
								{
									m_pParser->LError(CParser::ParmParseError);
									return false;
								}
							}
						}

						if(!global)
						{
							DoConversion(var, idxv, var1, NULL, true);
							st = AddStat(OP_INC);
							SetParm(st, 0, idxv);
						}

						if(!m_pParser->NextToken(true))
							return false;

						if(!TTDefEqual(m_pParser->CToken, CParser::TComma))
						{
							if(TTEqual(*m_pParser->CToken, CParser::TRCurly))
							{
								terminated = true;
								break;
							}
							else
							{
								m_pParser->LError(CParser::ParmParseError);
								return false;
							}
						}

						m_pParser->NextToken();
						idx++;
					}

					if(terminated)
						break;

					if(!m_pParser->NextLine(true))
						return false;
				}
				m_pParser->NextToken();
			}
			else
			{
//initializing simple variable
				if(!m_pParser->AssertEOL())
					return false;

				const char *csymb;

				if(m_pParser->CToken->Type == TT_STRING)
				{
					 if((csymb = FindString(m_pParser->CToken->Text)) != NULL)
						m_pParser->CToken->Text = csymb;
				}
				else
					csymb = NULL;

				if(csymb)
				{
					if(global)
					{
						var1 = FindVariable(csymb, m_pCFunct, m_pCClass);

						if(var1 == NULL || var1->GetVType() != var->GetVType())
						{
							m_pParser->LError(IncParmError, csymb);
//                   m_pParser->LError(IncParmError, var1->Name);
							return false;
						}
						m_pContext->Copy(var1, var);

						if(init == INITV_PARM)
							var->Spec |= VSPEC_DEFAULT;

						m_pParser->NextToken();
					}
				}
				else
				{
					if(global)
					{
						if(!var->Set(m_pParser->CToken, var))
						{
							m_pParser->LError(CParser::ParmParseError);
							return false;
						}
						if(init == INITV_PARM)
							var->Spec |= VSPEC_DEFAULT;
						m_pParser->NextToken();
					}
				}

				if(!global)
				{
					if(!ParseRValue(var, NULL))
						return false;
				}
			}
		}

		if(!multiple) break;

		if(m_pParser->EndOfLine()) break;

		if(TTDefEqual(m_pParser->CToken, CParser::TComma))
			m_pParser->NextToken();
		else
			break;
	}

	return true;
}
//-----------------------------------------------------------------------
bool CCompiler::ParseFunctParms(const VarType* rt, const Funct* pf)
{
bool end = false;
bool exp = false;

	if(m_pCFunct->ReturnValue == NULL)
	{
		//don't alloc void return type!!!
		if(rt->Type == VARTYPE_VOID)
		{
			m_pCFunct->ReturnValue = m_pContext->m_pvVoid;
		}
		else
		{
			/*
			//TODO: parse array size
			int dim = 0, dim2 = 1;
			short vvspec = 0;

			//TODO: Fix any number of dimensions
			//array definition
				//1st dimension
			if(TTDefEqual(m_pParser->CToken, CParser::TLSBracket))
			{
				if(!m_pParser->NextToken(true))
					return false;

				vvspec |= VSPEC_PTR;

				if(!TTDefEqual(m_pParser->CToken, CParser::TRSBracket))
				{
					if(!ParseArraySize(dim))
						return false;
				}
				else
				{
					//exception for vector ptr[]. Fake 2nd dimension as at least 1
					if(rt->Type == VARTYPE_VECTOR)
					{
						dim = 1;
					}
				}
				if(!m_pParser->AssertToken(CParser::TRSBracket))
					return false;
			}

			//2nd dimension
			if(TTDefEqual(m_pParser->CToken, CParser::TLSBracket))
			{
				dim2 = dim;

				if(!m_pParser->NextToken(true))
					return false;

				vvspec |= VSPEC_PTR;

				if(!TTDefEqual(m_pParser->CToken, CParser::TRSBracket))
				{
					if(!ParseArraySize(dim))
						return false;
				}
				if(!m_pParser->AssertToken(CParser::TRSBracket))
					return false;
			}
			m_pCFunct->ReturnValue = m_pModule->AllocVariable(rt, dim * dim2, true, vvspec);
*/
			m_pCFunct->ReturnValue = m_pModule->AllocVariable(rt, 0, true);

		}
	}

	//check compatibility
	if(pf && m_pCFunct->ReturnValue->GetVType() != pf->ReturnValue->GetVType())
	{
		m_pParser->LError(FuncNotCompError, m_pCFunct->GetName());
		return false;
	}

	uint n = 0;

	int vdef = 999, ndef = 999;

	while(!m_pParser->EndOfLine())
	{
		if(TTDefEqual(m_pParser->CToken, CParser::TRBracket))
		{
			m_pParser->NextToken();
			end = true;
			break;
		}

		if(m_pCFunct->GetNumParameters() >= MAX_FUNCTION_PARMS)
		{
			m_pParser->LError(TooMuchParmError);
			return false;
		}

		exp = false;

		//if parent funct, check parms number
		if(pf && (n + 1) > pf->GetNumParameters())
		{
			m_pParser->LError(FuncNotCompError, m_pCFunct->GetName());
			return false;
		}

		//TODO: if(pf != NULL) put there dummy variable and avoid duplicate allocation
		Variable dummy;

		//if(!ParseVarDef(INITV_FALSE, false, NULL, 0, n, m_iOptimize ? &dummy : NULL))
		if(!ParseVarDef(INITV_PARM, false, NULL, 0, n, m_iOptimize ? &dummy : NULL))
			return false;

		if(m_pCFunct->Variables[n]->Spec & VSPEC_LOCAL && m_pCFunct->Variables[n]->IsPtr())
		{
			if(m_pCFunct->Variables[n]->GetVType() != VARTYPE_VECTOR || m_pCFunct->Variables[n]->ArraySize() > 3)
			{
				m_pParser->LError("Cannot declare local[] variable '%s' as function parameter", m_pCFunct->Variables[n]->Name);
			}
			//return false;
		}

		if(!(m_pCFunct->Variables[n]->Spec & VSPEC_DEFAULT))
		{
			ndef = n;
			if(vdef < ndef)
			{
				m_pParser->LError("Misplaced non-default parameter %s", m_pCFunct->Variables[n]->Name);
				return false;
			}
		}
		else
			vdef = n;

		if(pf)
		{
			//check compatibility
			if(m_pCFunct->Variables[n]->GetVType() != pf->Variables[n]->GetVType() || m_pCFunct->Variables[n]->Spec != pf->Variables[n]->Spec)
			{
				m_pParser->LError(FuncNotCompError, m_pCFunct->GetName());
				return false;
			}

			if(m_iOptimize)
			{
				*pf->Variables[n] = *m_pCFunct->Variables[n];
//          *pf->Variables[n] = dummy;
				m_pCFunct->Variables[n] = pf->Variables[n];
			}
		}

		if(!m_pParser->EndOfLine() && TTDefEqual(m_pParser->CToken, CParser::TComma))
		{
			exp = true;
			m_pParser->NextToken();
		}

		n++;
	}

	m_pCFunct->ParmNum = n;

	if(!end || exp)
	{
		m_pParser->LError(AddVarError,"");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------
Variable *CCompiler::ParseRVariable(VarType *vt)
{
Variable *var,*ivar;

	if((var = ParseVariable(vt, &ivar)) == NULL)
	{
	
		//ParseVariable(vt, &ivar);
		return false;
	}

	//get from array if neccessary
	if(ivar)
	{
		if(var->GetVType() == VARTYPE_VECTOR)
		{
			if(vt == NULL || vt == m_pContext->m_vtVectorType && var->Dim[0] == 3 && var->Dim[1] == 0)
				vt = m_pContext->m_vtFloatType;
		}

		if(vt)
		{
			//HELP? to convert Array type to return type
			if(!(vt == m_pContext->m_vtFloatType && var->GetVType() == VARTYPE_VECTOR) && (vt->Type != var->GetVType()))
			{
				var = DoConversion(GetRegister(var->GetType()),NULL,var,ivar,true);

			}
			else
			{
				Variable dummy;
				dummy.SetType(vt);
				var = DoConversion(&dummy,NULL,var,ivar,false);
			}
		}
		else
		{
			var = DoConversion(var,NULL,var,ivar,false);
		}
	}

	return var;
}



//-----------------------------------------------------------------------
Variable cvar;

//-----------------------------------------------------------------------
Variable *CCompiler::Compute(OpCode cmd, Variable *out, Variable *in1, Variable *in2)
{
	if(m_iOptimize > 1 && in1->Spec & VSPEC_CONST && in2->Spec & VSPEC_CONST)
	{
		switch(cmd)
		{
		case OP_MODI:
			cvar.Int = in1->Int % in2->Int;
			break;
		case OP_MULI:
			cvar.Int = in1->Int * in2->Int;
			break;
		case OP_DIVI:
			cvar.Int = in1->Int / in2->Int;
			break;
		case OP_ADDI:
			cvar.Int = in1->Int + in2->Int;
			break;
		case OP_SUBI:
			cvar.Int = in1->Int - in2->Int;
			break;
		case OP_ORI:
			cvar.Int = in1->Int | in2->Int;
			break;
		case OP_ANDI:
			cvar.Int = in1->Int & in2->Int;
			break;

		case OP_MULF:
			cvar.Float = in1->Float * in2->Float;
			break;
		case OP_DIVF:
			cvar.Float = in1->Float / in2->Float;
			break;
		case OP_ADDF:
			cvar.Float = in1->Float + in2->Float;
			break;
		case OP_SUBF:
			cvar.Float = in1->Float - in2->Float;
			break;
		default:
			return out;
		}
		cvar.Spec = VSPEC_CONST;
		cvar.Dim[0] = cvar.Dim[1] = 0;
		cvar.TypeType = out->TypeType;
		cvar.TypeIndex = out->TypeIndex;
		cvar.Name = NULL;
		return &cvar;
	}
	else
		return out;
}

//-----------------------------------------------------------------------
Variable* CCompiler::ParseRValue(Variable *var3, Variable *ivar3, bool justreturn)
{
Variable *var1, *var2;
VarType *type, *type1,*type2;
TToken *cmp;
Stat *stat;
OpCode op;

	type = var3->GetType();

	//if referencing indexed vector, take it as array of floats
	if(ivar3 && type->Type == VARTYPE_VECTOR && (var3->Dim[1] == 0 || m_iIndexCount >= 2))
		var3->SetType(m_pContext->m_vtFloatType); //result type has been temporarily changed

	if((var1 = ParseRVariable(var3->GetType())) == NULL)
		return NULL;

	if(EndOfValue())
	{
		if(var1->GetTypeIndex() != var3->GetTypeIndex())
		{
			Variable *v;

			if((v = DoConversion(var3,NULL,var1,NULL,false)) == NULL)
			{
				m_pParser->LError(IncParmError, m_pCSymbol);
				return NULL;
			}
			var1 = v;
		}

		if(justreturn && ivar3 == NULL && var3->Name && var3->Name[0] == '@')
			return var1;

		if(!DoConversion(var3,ivar3,var1,NULL,true))
		{
			m_pParser->LError(IncParmError, m_pCSymbol);
			return NULL;
		}
		enf_assert(type);
		var3->SetType(type);
		return var3;
	}

	type1 = var1->GetType();

	while(true)
	{
		int equal;

		cmp = m_pParser->CToken;
		if(!m_pParser->NextToken(true))
			return NULL;

		if(EndOfValue() || cmp->Type != TT_DEF)
		{
			m_pParser->LError(CParser::GarbageError, cmp->Text);
			return NULL;
		}

		//bracket?
		if(TTDefEqual(m_pParser->CToken, CParser::TLBracket) && !m_pModule->FindType(m_pParser->CToken[1].Text))
		{
			if(!m_pParser->NextToken(true))
				return NULL;

			Variable *dummy = NULL;

			var2 = GetRegister(var3->GetType());
			if(!ParseRValue(var2, dummy))
				return NULL;

			if(!m_pParser->AssertToken(CParser::TRBracket))
				return NULL;
		}
		else
		{
			var2 = ParseRVariable(var3->GetType());
			if(var2 == NULL)
				return NULL;
		}

		type2 = var2->GetType();

		op = OP_NONE;
		equal = ((var3->GetTypeIndex() == type1->Index && var3->GetTypeIndex() == type2->Index));

		//special cases
		if(!equal && cmp->Type == TT_DEF && TTDefEqual(cmp, CParser::TAsterisk))
		{
			if(type->Type == VARTYPE_FLOAT &&
				type1->Type == VARTYPE_VECTOR &&
				type2->Type == VARTYPE_VECTOR)
				op = OP_DOTV;
			else if(type->Type == VARTYPE_VECTOR &&
				type1->Type == VARTYPE_VECTOR &&
				type2->Type == VARTYPE_FLOAT)
				op = OP_SCALEV;
		}

		if(op == OP_NONE)
		{
			if(!equal)
			{
				var1 = DoConversion(var3,NULL,var1,NULL,false);
				var2 = DoConversion(var3,NULL,var2,NULL,false);
			}

			if(!var1 || !var2)
			{
				m_pParser->LError(IncParmError, m_pCSymbol);
				return NULL;
			}

			op = FindSign(cmp,var3->GetVType());
		}

		if(op == OP_NONE)
		{
			m_pParser->LError(OperatorError, cmp->Text);
			return NULL;
		}

		Variable *tmpv = Compute(op, var3, var1, var2);

		//have to get it from accumulator
		if(tmpv != &cvar && var1 == &cvar)
			var1 = DoConversion(GetRegister(var3->GetType()), NULL, var1, NULL, true);

		if(!EndOfValue())
		{
			if(tmpv == &cvar)
				var1 = tmpv;
			else
			{
				stat = AddStat(op);
				FreeRegister(var1);
				FreeRegister(var2);
				m_UsedReturns.Remove(var1);
				m_UsedReturns.Remove(var2);
				SetParm(stat, 0, var1);
				SetParm(stat, 1, var2);
				SetParm(stat, 2, (var1 = GetRegister(var3->GetType())));
			}
			type1 = var3->GetType();
			continue;
		}

		if(tmpv == &cvar)
		{
			if(ivar3)
				var1 = DoConversion(GetRegister(var3->GetType()), NULL, tmpv, NULL, true);
			else
				var1 = tmpv;

			if(!DoConversion(var3, ivar3, var1, NULL, true))
			{
				m_pParser->LError(IncParmError, m_pCSymbol);
				return NULL;
			}
		}
		else
		{
			if(var1->GetVType() != VARTYPE_VECTOR && var2->GetVType() != VARTYPE_VECTOR && (var1->IsPtr() != var2->IsPtr()) && op != OP_MOVE && op != OP_MOVEA)
			{
				m_pParser->LError(MixedArithmetic);
				return NULL;
			}

			FreeRegister(var1);
			FreeRegister(var2);

			if((var3 == var1 && var2 == m_pContext->m_pvNumbersInt[1]) || (var3 == var2 && var1 == m_pContext->m_pvNumbersInt[1]))
			{
				if(op == OP_ADDI)
				{
					stat = AddStat(OP_INC);
					SetParm(stat, 0, (var1 == var3) ? var1 : var2);
					break;
				}
				if(op == OP_SUBI && var2 == m_pContext->m_pvNumbersInt[1])
				{
					stat = AddStat(OP_DEC);
					SetParm(stat, 0, var1);
					break;
				}
			}

			stat = AddStat(op);
			SetParm(stat, 0, var1);
			SetParm(stat, 1, var2);

			if(ivar3)
			{
				SetParm(stat, 2, (var1 = GetRegister(var3->GetType())));
				if(!DoConversion(var3, ivar3, var1, NULL, true))
				{
					m_pParser->LError(IncParmError, m_pCSymbol);
					return NULL;
				}
			}
			else
				SetParm(stat, 2, var3);
			m_UsedReturns.Remove(var1);
			m_UsedReturns.Remove(var2);
		}
		break;
	}

	enf_assert(type);
	var3->SetType(type);
	return var3;
}

//-----------------------------------------------------------------------
bool CCompiler::AreTypesRelated(const Variable* outVar, const Variable* inVar)
{
	if(outVar->GetVType() != inVar->GetVType())
		return false;

	if(outVar->GetVType() != VARTYPE_CLASS)
		return true;

	const Class* outClass = static_cast<const Class*>(outVar->GetType());
	const Class* inClass = static_cast<const Class*>(inVar->GetType());
	if((outClass->m_iFlags & CF_FORWARD_DECLARATION) == 0 &&
		(inClass->m_iFlags & CF_FORWARD_DECLARATION) == 0)
	{
		//check relationship
		if(!inClass->IsInherited(outClass) &&
			!outClass->IsInherited(inClass) &&
			outClass != m_pContext->m_vtClassType)
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------
bool CCompiler::ParseFuncParm(Funct *f, Variable *var3,Variable **vp)
{
Variable *var1, *var2;
VarType *type, *type1,*type2;
const TToken *cmp;
Stat *stat;
OpCode op;

	type = var3->GetType();

	if((var1 = ParseRVariable(type)) == NULL)
		return false;

	if(EndOfValue())
	{
		if(f->Flags & FN_INTERN)
		{
			if(type->Type == VARTYPE_VOID)
			{
				*vp = var1;
			}
			else
			{
				if((*vp = DoConversion(var3, NULL, var1, NULL, false)) == NULL)
				{
					m_pParser->LError(IncParmError, m_pCSymbol);
					return false;
				}
			}
		}
		else
		{
			if(var3->GetVType() == var1->GetVType() && !IsRefVar(var1))
			{
				*vp = var1;
			}
			else
			{
				if((*vp = DoConversion(var3, NULL, var1, NULL, true)) == NULL)
				{
					m_pParser->LError(IncParmError, m_pCSymbol);
					return false;
				}
			}
		}
		return true;
	}
	//alloc temporary result for function parameter
	*vp = var3 = GetRegister(type);

	type1 = var1->GetType();

	while(true)
	{
		int equal;

		cmp = m_pParser->CToken;

		if(!m_pParser->NextToken(true))
			return false;


		if(EndOfValue() || cmp->Type != TT_DEF)
		{
			m_pParser->LError(CParser::GarbageError, cmp->Text);
			return false;
		}

		//bracket?
		if(TTDefEqual(m_pParser->CToken, CParser::TLBracket) && !m_pModule->FindType(m_pParser->CToken[1].Text))
		{
			if(!m_pParser->NextToken(true))
				return false;

			Variable *dummy = NULL;

			var2 = GetRegister(type);
			if(!ParseRValue(var2, dummy))
				return NULL;

			if(!m_pParser->AssertToken(CParser::TRBracket))
				return NULL;
		}
		else
		{
			var2 = ParseRVariable(type);
			if(var2 == NULL)
				return false;
		}

		type2 = var2->GetType();

		op = OP_NONE;
		equal = ((type == type1 && type == type2));

		//special cases
		if(!equal && cmp->Type == TT_DEF && TTDefEqual(cmp, CParser::TAsterisk))
		{
			if(type->Type == VARTYPE_FLOAT &&
				type1->Type == VARTYPE_VECTOR &&
				type2->Type == VARTYPE_VECTOR)
				op = OP_DOTV;
			else if(type->Type == VARTYPE_VECTOR &&
				type1->Type == VARTYPE_VECTOR &&
				type2->Type == VARTYPE_FLOAT)
				op = OP_SCALEV;
		}

		if(op == OP_NONE)
		{
			if(!equal)
			{
				var1 = DoConversion(var3,NULL,var1,NULL,false);
				var2 = DoConversion(var3,NULL,var2,NULL,false);
			}

			if(!var1 || !var2)
			{
				m_pParser->LError(IncParmError, m_pCSymbol);
				return false;
			}

			op = FindSign(cmp,type->Type);
		}

		if(!op)
		{
			m_pParser->LError(OperatorError, cmp->Text);
			return false;
		}

		Variable *tmpv = Compute(op, var3, var1, var2);

		//have to get it from accumulator
		if(tmpv != &cvar && var1 == &cvar)
		{
			if(var1->GetVType() == VARTYPE_INT)
				var1 = m_pModule->AddConstantI(var1->Int);
			else
				var1 = DoConversion(GetRegister(var3->GetType()), NULL, var1, NULL, true);
		}

		if(var1->GetVType() != VARTYPE_VECTOR && var2->GetVType() != VARTYPE_VECTOR && (var1->IsPtr() != var2->IsPtr()) && op != OP_MOVE && op != OP_MOVEA)
		{
			m_pParser->LError(MixedArithmetic);
			return false;
		}

		if(!EndOfValue())
		{
			if(tmpv == &cvar)
				var1 = tmpv;
			else
			{
				stat = AddStat(op);
				SetParm(stat, 0, var1);
				SetParm(stat, 1, var2);
				FreeRegister(var1);
				FreeRegister(var2);
				SetParm(stat, 2, (var1 = GetRegister(type)));
			}
			type1 = type;
			continue;
		}

		if(tmpv == &cvar)
		{
			var1 = tmpv;

			if(!DoConversion(var3, NULL, var1, NULL, true))
			{
				m_pParser->LError(IncParmError, m_pCSymbol);
				return false;
			}
		}
		else
		{
			stat = AddStat(op);
			SetParm(stat, 0, var1);
			SetParm(stat, 1, var2);
			FreeRegister(var1);
			FreeRegister(var2);
			SetParm(stat, 2, var3);
		}
		break;
	}
	return true;
}

//-----------------------------------------------------------------------
uint CountBits(uint mask)
{
	//get highest bit and make count
	uint count = 0;

	for(uint n = 0; n < MAX_REGS && mask != 0; n++)
	{
		if(mask & (1 << n))
		{
			mask ^= (1 << n);
			count = n + 1;
		}
	}
	return count;
}

//-----------------------------------------------------------------------
uint CCompiler::EncodeCallRef(Funct* fn, uint functionIndex, Variable* objptr)
{
	Instance** iptr = NULL;
	uint typeIndex = 0;
	
	if(objptr)
	{
		iptr = &objptr->InstancePtr;
		typeIndex = objptr->GetType()->Index;
	}

	for(uint n = 0; n < m_pModule->m_FunctRefs.GetCardinality(); n++)
	{
		const ScriptModule::CallRef& ref = m_pModule->m_FunctRefs[n];
		if(ref.Module == fn->Module && ref.pInstance == iptr && ref.TypeIndex == typeIndex && ref.FunctionIndex == functionIndex)
			return n;
	}
	enf_assert(m_pModule->m_FunctRefs.GetCardinality() < 0x2000);

	ScriptModule::CallRef& ref = m_pModule->m_FunctRefs.Append();
	ref.Module = objptr ? objptr->GetType()->Module : fn->Module;
	ref.FunctionIndex = functionIndex;
	ref.TypeIndex = typeIndex;
	ref.pInstance = iptr;
	return m_pModule->m_FunctRefs.GetCardinality() - 1;
}

//-----------------------------------------------------------------------
bool CCompiler::IsConstructorOrDestructor(const Class* cl, const Funct* fn)
{
	while(cl)
	{
		if((cl->Destructor && cl->GetMethod(cl->Destructor - 1) == fn) ||
			(cl->Constructor && cl->GetMethod(cl->Constructor - 1) == fn))
			return true;
		cl = cl->GetSuper();
	}
	return false;
}
//-----------------------------------------------------------------------
Variable *CCompiler::ParseFunctionCall(Variable* optr, bool thread)
{
Variable* vp[MAX_FUNCTION_PARMS];

	Class* cl = (Class*)optr->GetType();

	Funct* f = FindFunction(m_pParser->CToken->Text, cl);

	if(f == NULL)
	{
		m_pParser->LError(UndefinedFunction, m_pParser->CToken->Text);
		return NULL;
	}

	if(f->Flags & FN_PRIVATE)
	{
		m_pParser->LError(MethodIsPrivate, m_pParser->CToken->Text);
		return NULL;
	}

	if(thread && (f->Flags & FN_SCRIPT) == 0)
	{
		m_pParser->LError("Cannot create thread out of non-script function '%s'", m_pParser->CToken->Text);
		return NULL;
	}

	if(cl)
	{
		//isn't it constructor, or destructor?
		if(IsConstructorOrDestructor(cl, f))
		{
			m_pParser->LError("Cannot call constructor or destructor '%s' method directly!", m_pParser->CToken->Text);
			return NULL;
		}
	}

	f->Flags |= FN_USED;

	Variable* locobj = m_pObj ? optr : NULL;
	int locfunc = m_iFunctionIndex;

	if(!m_pParser->NextToken(true))
		return false;

	if(!m_pParser->AssertToken(CParser::TLBracket))
		return false;

	int t = f->ParmNum;

	ushort trans = (m_iOptimize == 0) ? 0xc000 : 0;

	uint n = 0;
	if(t)
	{
		ZeroMem32(vp, t);

		for(; n < f->GetNumVariables() && !m_pParser->EndOfLine() && !TTDefEqual(m_pParser->CToken, CParser::TRBracket);)
		{
			Variable* vf = f->Variables[n];

			if(!ParseFuncParm(f, vf, &vp[n]))
				return NULL;

			//remember, that we have to translate variables
			if(IsRefVar(vp[n]))
			{
				vp[n] = (Variable*)EncodeParm(vp[n]);
				trans |= 0xc000;
			}
			else
			{
				//remember, that we have to copy
				if(vp[n] != vf)
					trans |= 0x4000;
			}

			n++;

			if(--t == 0) break;

			if(!TTDefEqual(m_pParser->CToken, CParser::TComma))
				break;

			m_pParser->NextToken();
		}
	}

	//try default parms
	if(t && TTDefEqual(m_pParser->CToken, CParser::TRBracket))
	{
		for(; n < f->GetNumVariables() && t; n++, t--)
		{
			Variable* vf = f->Variables[n];

			if(!(vf->Spec & VSPEC_DEFAULT))
				break;

			//add constant with default value
			int alloc = m_pModule->m_pStatic->Allocated();

			//FIXME: IS THIS CORRECT???
			vp[n] = m_pModule->AllocVariable(vf->GetType(), 0, true, VSPEC_CONST);
			m_pContext->Copy(vf, vp[n]);
			vp[n] = m_pModule->FoldConstant(vp[n], alloc);

			trans |= 0x4000;
		}
	}

	if(t)
	{
		m_pParser->LError(ParmError);
		return NULL;
	}

	if(!m_pParser->AssertToken(CParser::TRBracket))
	{
		m_pParser->LError("Too much parameters");
		return NULL;
	}

//OP_CALL/OP_NCALL/OP_NCALLI
//xcmd: Opcode + sizeof + index to CallRef
//RegisterMask (only OP_CALL)
//StatementSizeof + ParmNum * sizeof(Variable*)
	BeforeCall(f);

	bool multipleUsage = false;
	if(f->ReturnValue != m_pContext->m_pvVoid && m_UsedReturns.Contains(f->ReturnValue))
	{
		Push(f->ReturnValue);
		multipleUsage = true;
		//m_pParser->LError("Function called more than once");
	}
	else
		m_UsedReturns.Insert(f->ReturnValue);

	ScriptModule::CallRef ref;
	ref.Module = f->Module;
	if(locobj)
	{
		ref.pInstance = &locobj->InstancePtr;
		ref.TypeIndex = locobj->GetType()->Index;
	}
	else
	{
		ref.pInstance = NULL;
		ref.TypeIndex = 0;
	}

	uint fnindex = EncodeCallRef(f, locfunc, locobj);

	OpCode op;
	int ssize;

	if(f->Flags & FN_NATIVE)
	{
		op = locobj == NULL ? OP_NCALL : OP_NCALLI;
		ssize = 4;
	}
	else
	{
		if(f->Flags & FN_INTERN)
		{
			op = OP_ICALL;
			ssize = 4;
			if(m_iOptimize)
			{
				if(locobj == NULL)
				{
					trans |= 0x2000;
				}
			}
		}
		else
		{
			op = thread ? OP_TCALL : OP_CALL;
			ssize = 8;
		}
	}

	Stat* stat = AddStat(op, ssize + f->GetNumParameters() * sizeof(Variable *));
	CopyMem32((void*)((char*)stat + ssize), vp, f->GetNumParameters());
	stat->SetOffset(fnindex | trans);
	if(f->Flags & FN_SCRIPT)
	{
		SetParm(stat, 0, (void*)((CountBits(m_iVRegMask) << 16) | (CountBits(m_iSRegMask) << 12) | (CountBits(m_iCRegMask) << 8) | (CountBits(m_iFRegMask) << 4) | CountBits(m_iRRegMask)), true);
	}

	Variable* v = f->ReturnValue;

	if(multipleUsage)
	{
		v = GetRegister(f->ReturnValue->GetType());
		v = DoConversion(v, NULL, f->ReturnValue, NULL, true, false);
		Pop(f->ReturnValue);
	}
	AfterCall(f);

	return v;
}

//-----------------------------------------------------------------------
Variable *CCompiler::ParseConstructorCall(Class* cl)
{
	Variable* locobj = m_pObj;
	int locfunc = m_iFunctionIndex;
	Funct* f = FindFunction(m_pParser->CToken->Text, cl);
	Variable* params[MAX_FUNCTION_PARMS];
	uint numParams = 0;

	if(f != NULL)
	{
		m_pParser->NextToken();

		if(!m_pParser->EndOfLine() && TTDefEqual(m_pParser->CToken, CParser::TLBracket))
		{
			m_pParser->NextToken();
			numParams = f->GetNumParameters();
			int t = f->GetNumParameters();

			uint n = 0;
			if(t)
			{
				for(; n < f->GetNumVariables() && !m_pParser->EndOfLine() && !TTDefEqual(m_pParser->CToken, CParser::TRBracket); n++)
				{
					Variable* vf = f->Variables[n];

					if(!ParseFuncParm(f, vf, &params[n]))
						return NULL;

					params[n] = (Variable*)EncodeParm(params[n]);
					if(--t == 0) break;

					if(!m_pParser->AssertToken(CParser::TComma))
						break;
				}
			}

			//try default parms
			if(t && TTDefEqual(m_pParser->CToken, CParser::TRBracket))
			{
				for(; n < f->GetNumVariables() && t; n++, t--)
				{
					Variable* vf = f->Variables[n];

					if(!(vf->Spec & VSPEC_DEFAULT))
						break;

					//add constant with default value
					int alloc = m_pModule->m_pStatic->Allocated();

					//FIXME: IS THIS CORRECT???
					params[n] = m_pModule->AllocVariable(vf->GetType(), 0, true, VSPEC_CONST);
					m_pContext->Copy(vf, params[n]);
					params[n] = m_pModule->FoldConstant(params[n], alloc);
				}
			}

			if(t)
			{
				m_pParser->LError(ParmError);
				return NULL;
			}

			if(!m_pParser->AssertToken(CParser::TRBracket))
			{
				m_pParser->LError("Too much parameters");
				return NULL;
			}
		}
	}
	else
		m_pParser->NextToken();

	Variable* var = GetRegister(cl);

	Stat* stat = AddStat(OP_NEW, STRUCT_MEMBER_OFFSET(Stat, Op) + 2 * sizeof(void*) + numParams * sizeof(Variable*));
	m_PrevCStat = stat;

	stat->SetOffset(cl->Index);
	SetParm(stat, 0, (void*)cl->GetModule(), true);

	SetParm(stat, 1, var);
	CopyMem32(&stat->Op[2], params, numParams);

	return var;
}

//-----------------------------------------------------------------------
//Compile Inc/Dec symbol
Variable* CCompiler::CompileID(Variable *dest, Variable *src, post_e symb)
{
	Stat* stat;
	Variable* rvar;

	if(dest == NULL)
	{
		stat = AddStat(OP_MOVE);
		SetParm(stat, 1, (rvar = GetRegister(src->GetType())));
		SetParm(stat, 0, src);
		dest = src;
	}
	else
		rvar = dest;

	if(dest == src && src->GetVType() == VARTYPE_INT)
	{
		switch(symb)
		{
		case PS_INC:
			stat = AddStat(OP_INC);
			break;

		case PS_DEC:
			stat = AddStat(OP_DEC);
			break;
		}
		SetParm(stat, 0, src);
	}
	else
	{
		if(src->GetVType() == VARTYPE_FLOAT)
		{
			switch(symb)
			{
			case PS_INC:
				stat = AddStat(OP_ADDF);
				break;

			case PS_DEC:
				stat = AddStat(OP_SUBF);
				break;
			}
			SetParm(stat, 1, m_pContext->m_pvOneFloat);
		}
		else
		{
			switch(symb)
			{
			case PS_INC:
				stat = AddStat(OP_ADDI);
				break;

			case PS_DEC:
				stat = AddStat(OP_SUBI);
				break;
			}
			SetParm(stat, 1, m_pContext->m_pvNumbersInt[1]);
		}
		SetParm(stat, 0, src);
		SetParm(stat, 2, dest);
	}
	return rvar;
}

//-----------------------------------------------------------------------
int log2(int m)
{
#ifdef ENF_INTEL
	__asm
	{
		bsf   eax, m
		bsr   ebx, m
		cmp   eax, ebx
		je    done
		mov   eax, -1
done:
		mov   m, eax
	}
#else

	for(uint l = 0; l < 32; l++)
	{
		if((1 << l) == m)
			return l;
	}

	return -1;
#endif

	return m;
}

//-----------------------------------------------------------------------
bool CCompiler::ParseIndex(Variable *var, Variable **ivar)
{
	int inum = 0;
	Variable *index[8];

	while(true)
	{
		if(!m_pParser->NextToken(true))
			return NULL;

		//simple case
		if(TTDefEqual(&m_pParser->CToken[1], CParser::TRSBracket))
		{
			Variable *dummy;
			index[inum] = ParseVariable(m_pContext->m_vtIntType, &dummy);

			if(index[inum] == NULL)
				return false;

			if(index[inum]->GetVType() != VARTYPE_INT)
			{
				//Allow only float->int conversion
				if(index[inum]->GetVType() != VARTYPE_FLOAT)
					return false;

				dummy = index[inum];
				index[inum] = GetRegister(m_pContext->m_vtIntType);

				DoConversion(index[inum], NULL, dummy, NULL, true);

			}

			inum++;
			if(!m_pParser->AssertEOL())
				return false;
		}
		else
		{
			index[inum] = GetRegister(m_pContext->m_vtIntType);
			if(!ParseRValue(index[inum++], NULL))
				return false;
		}

		if(!m_pParser->AssertToken(CParser::TRSBracket))
			return false;

		if(m_pParser->EndOfLine() || !TTDefEqual(m_pParser->CToken, CParser::TLSBracket))
			break;
	}

	Variable *lindex;
	Stat *st;
	int d = 0,m = 1;

	lindex = index[inum - 1];
	Variable tmpv;
	tmpv.Spec = VSPEC_CONST;

	for(int n = inum - 1; n >= 0; n--)
	{
		if(d > 1)
		{
			m_pParser->LError("Subscript out of range on variable %s", var->Name);
			return false;
		}
		if(m != 1)
		{
			Variable *mreg;

			if(m_iOptimize > 1 && index[n]->Spec & VSPEC_CONST)
			{
				tmpv.Int = m * index[n]->Int;
				mreg = &tmpv;
			}
			else
			{
				int shift = log2(m);

				if(m_iOptimize > 1 && shift >= 0)
				{
					st = AddStat(OP_LSLI);
					SetParm(st, 0, index[n]);
					SetParm(st, 1, m_pModule->AddConstantI(shift));
				}
				else
				{
					st = AddStat(OP_MULI);
					SetParm(st, 0, index[n]);
					SetParm(st, 1, m_pModule->AddConstantI(m));
				}
				FreeRegister(index[n]);
				mreg = GetRegister(m_pContext->m_vtIntType);
				SetParm(st, 2, mreg);
			}

			if(m_iOptimize > 1 && lindex->Spec & VSPEC_CONST && mreg->Spec & VSPEC_CONST)
			{
				tmpv.Int = lindex->Int + mreg->Int;
				lindex = &tmpv;
			}
			else
			{
				//optimize array[index][0]
				if(!(lindex->Spec & VSPEC_CONST && lindex->Int == 0))
				{
					if(mreg != &tmpv)
					{
						st = AddStat(OP_ADDI);
						SetParm(st, 0, lindex);
						SetParm(st, 1, mreg);
						SetParm(st, 2, mreg);
						FreeRegister(lindex);
					}
					else
					{
						if(mreg->Int == 0)
						{
							//it's zero anyway
							mreg = lindex;
						}
						else
						{
							//yuck!, is it correct?
							st = AddStat(OP_ADDI);
							SetParm(st, 0, lindex);
							SetParm(st, 1, m_pModule->AddConstantI(mreg->Int));
							FreeRegister(lindex);
							mreg = GetRegister(m_pContext->m_vtIntType);
							SetParm(st, 2, mreg);
						}
					}
				}
				lindex = mreg;
			}
		}

		m = m * var->Dim[d++];
	}

	//check non-array
	if(lindex->Spec & VSPEC_CONST && var->Dim[1] == 0 && var->Dim[0] != 0)
	{
		if((uint)lindex->Int >= var->ArraySize())
		{
			m_pParser->LError("Subscript out of range on variable %s", var->Name);
			return false;
		}
	}

	if(lindex == &tmpv)
	{
		if(tmpv.Int >= 0 && tmpv.Int <= 9)
			lindex = m_pContext->m_pvNumbersInt[tmpv.Int];
		else
		{
			lindex = GetRegister(m_pContext->m_vtIntType);
			st = AddStat(OP_IMOVE);
			SetParm(st, 0, (void*)tmpv.Int, true);
			SetParm(st, 1, lindex);
		}
	}
	*ivar = lindex;

	m_iIndexCount = inum;
	return true;
}

//-----------------------------------------------------------------------
bool IsFunctionParam(const Funct* fn, const Variable* var)
{
	for(uint n = 0; n < fn->GetNumParameters(); n++)
	{
		if(fn->GetVariable(n) == var)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------
Variable *CCompiler::ParseVariable(VarType *type, Variable **ivar, bool lside)
{
Variable *var;
bool neg = false;
post_e symb = PS_NONE;
const TToken* tsymb;

	*ivar = NULL;

	//sizeof()
	if(TTDefEqual(m_pParser->CToken, CParser::TSizeof))
	{
		m_pParser->NextToken();

		if(!m_pParser->AssertToken(CParser::TLBracket))
			return NULL;

		Variable *svar = ParseVariable(NULL, &var);

		if(svar == NULL)
			return NULL;

		if(!m_pParser->AssertToken(CParser::TRBracket))
			return NULL;

		if(m_iOptimize > 0 && svar->Spec & VSPEC_CONST && !IsFunctionParam(m_pCFunct, svar))
		{
			var = m_pModule->AddConstantI(svar->GetVType() == VARTYPE_VECTOR ? (svar->ArraySize() / 3) : svar->ArraySize());
		}
		else
		{
			var = GetRegister(m_pContext->m_vtIntType);
			Stat *st = AddStat(OP_SIZEOF);
			m_PrevCStat = st; //hint for optimizer
			SetParm(st, 0, svar);
			SetParm(st, 1, var);
		}
		return var;
	}

	//process symbols
	while(m_pParser->CToken->Type == TT_DEF)
	{
		if(!m_pParser->AssertEOL())
			return NULL;

		//it's not neccesary now!
		if(type && TTDefEqual(m_pParser->CToken, CParser::TAnd) && type->Type == VARTYPE_FUNCPTR)
		{
			m_pParser->NextToken();
			continue;
		}

		if(TTDefEqual(m_pParser->CToken, CParser::TMinus))
		{
			m_pParser->NextToken();
			neg = true;
			continue;
		}

		tsymb = m_pParser->CToken;

		//check pre inc/dec symbols
		if(TTDefEqual(m_pParser->CToken, CParser::TPlusPlus))
		{
			symb = PS_INC;
			m_pParser->NextToken();
			continue;
		}

		if(TTDefEqual(m_pParser->CToken, CParser::TMinusMinus))
		{
			symb = PS_DEC;
			m_pParser->NextToken();
			continue;
		}

		//type casting/()
		if(TTDefEqual(m_pParser->CToken, CParser::TLBracket))
		{
			VarType *vt;

			m_pParser->NextToken();

			if(!m_pParser->AssertEOL())
				return NULL;

			if((vt = m_pModule->FindType(m_pParser->CToken->Text)) == NULL)
			{
				m_pParser->LError(BadVarError, m_pParser->CToken->Text);
				return NULL;
			}
			m_pParser->NextToken();

			if(!m_pParser->AssertToken(CParser::TRBracket))
				return NULL;

			Variable *in;

			in = ParseRVariable(vt);

			if(in && in->GetType() != vt)
			{
				Variable dummy;
				dummy.SetType(vt);
				var = DoConversion(&dummy, NULL, in, NULL, false, true);
			}
			else
				var = in;

			goto gOut;
		}

		m_pParser->LError("Unexpected token '%s'", m_pParser->CToken->Text);
		return NULL;
	}

	if(m_pParser->CToken->Type == TT_STRING)
	{
		bool thread = false;

		if(TTEqual(*m_pParser->CToken, "thread"))
		{
			thread = true;

			if(!m_pParser->NextToken(true))
				return NULL;
		}

		if((m_pParser->CParm + 1) < m_pParser->CLine->m_iNumTokens)
		{
			if(TTDefEqual(&m_pParser->CToken[1], CParser::TLBracket))
			{
				VarType* prevThisType = NULL;

				if(m_pCClass)
				{
					prevThisType = m_pContext->m_pvThis->GetType();
					m_pContext->m_pvThis->SetType(m_pCClass);
				}

				m_pCSymbol = m_pParser->CToken->Text;

				var = ParseFunctionCall(m_pContext->m_pvThis, thread);
				if(prevThisType)
					m_pContext->m_pvThis->SetType(prevThisType);

				if(var == NULL) return NULL;
//				goto gOut;
				goto gNext;
			}
		}

		if(thread)
		{
			m_pParser->LError("Expected thread function name");
			return NULL;
		}

		if(TTEqual(*m_pParser->CToken, "new"))
		{
			Class *cl = NULL;

			if(!m_pParser->NextToken(true))
				return NULL;

			cl = m_pModule->FindClass(m_pParser->CToken->Text);

			if(cl == NULL)
			{
				m_pParser->LError("Can't find class %s", m_pParser->CToken->Text);
				return NULL;
			}

			var = ParseConstructorCall(cl);
			if(var == NULL) return NULL;
			goto gOut;
		}

		m_pCSymbol = m_pParser->CToken->Text;
		var = FindVariable(m_pParser->CToken->Text, m_pCFunct, m_pCClass);
		m_pParser->NextToken();

		if(var == NULL)
		{
			if(type && type->Type == VARTYPE_FUNCPTR)
			{
				//HACK: function pointer
				Funct *pf = FindFunction(m_pCSymbol, m_pCClass, false);

				if(pf != NULL)
				{
					Variable *dummy = m_pModule->AllocVariable(m_pContext->m_vtFuncType, 2, true, VSPEC_PROTECT|VSPEC_PTR);
					dummy->FuncPtr->pInstance = m_pObj ? &m_pObj->InstancePtr : NULL;
					dummy->FuncPtr->FuncIndex = m_iFunctionIndex;

					m_pModule->m_Variables.Insert(dummy);
					return dummy;
				}
			}
			goto gOut;
		}

gNext:;

	//check for class

		while(!m_pParser->EndOfLine())
		{
			if(TTDefEqual(m_pParser->CToken, CParser::TDot))
			{
				if(!m_pParser->NextToken(true))
					return NULL;

				if(var->GetVType() != VARTYPE_CLASS)
				{
					m_pParser->LError(NotClassPtrError, m_pCSymbol);
					return NULL;
				}

				if(IsRefVar(var))
				{
					//add temporary variable
					Variable *nvar = var->Module->m_ClassPointers.GetElement(var->GetTypeIndex());

					//why the hell?
					//Variable *nvar = GetRegister(g_vtClassType);
					//nvar->Type = var->Type;

					Stat *st = AddStat(OP_MOVE);
					SetParm(st, 0, var);
					SetParm(st, 1, nvar);
					var = nvar;
				}

				//variable member
				if(!TTDefEqual(&m_pParser->CToken[1], CParser::TLBracket))
				{
					Variable* mv;

					//search for class member
					m_pCSymbol = AddString(m_pParser->CToken->Text);
					m_pParser->NextToken();

					mv = FindVariable(m_pCSymbol, NULL, (Class*)var->GetType(), true);

					if(mv)
					{
						//HACKHACK
						m_RefVars[m_iNumRefVars - 1].IClass = (void**)&var->InstancePtr;
						var = mv;
					}
					else
					{
						if(type && type->Type == VARTYPE_FUNCPTR)
						{
							//HACK: function pointer
							Funct* pf = FindFunction(m_pCSymbol, (Class*)var->GetType(), false);

							if(pf != NULL)
							{
								Variable* dummy = m_pModule->AllocVariable(type, 2, true, VSPEC_PROTECT|VSPEC_PTR);
//								dummy->FuncPtr->pInstance = (Instance**)var;
								dummy->FuncPtr->pInstance = &var->InstancePtr;
								dummy->FuncPtr->FuncIndex = m_iFunctionIndex;
								m_pModule->m_Variables.Insert(dummy);
								return dummy;
							}
						}

						m_pParser->LError(NotVarError, m_pCSymbol);
						return NULL;
					}
				}
				else
				{//function call

					VarType* prevThisType = NULL;

					if(m_pCClass)
					{
						prevThisType = m_pContext->m_pvThis->GetType();
						m_pContext->m_pvThis->SetType(m_pCClass);
					}

					const char *tsymbol = m_pCSymbol;
					var = ParseFunctionCall(var, false);
					if(prevThisType)
						m_pContext->m_pvThis->SetType(prevThisType);
					if(var == NULL) return NULL;
					m_pCSymbol = tsymbol;
					//goto gOut;
				}
			}
			else if(TTDefEqual(m_pParser->CToken, CParser::TLSBracket))
			{
				const char *tsymbol = m_pCSymbol;

				if(!ParseIndex(var, ivar))
					return NULL;

				m_pCSymbol = tsymbol;

				//index not valid
				if(*ivar == NULL)
				{
					return NULL;
				}
			}
			else
				break;
		}

		if(symb && var->GetVType() == VARTYPE_INT)
		{
			if(*ivar != NULL)
			{
				m_pParser->LError("TEMP_ERROR: nesmis pouzivat '--/++' na poli");
				return NULL;
			}

			var = CompileID(var, var, symb);
			symb = PS_NONE;
		}
	}
	else
	{
		if(type)
		{
			var = m_pModule->AddConstant(type, m_pParser->CToken);
			if(var == NULL)
			{
				m_pParser->LError(IncParmError, m_pParser->CToken->Text);
				return NULL;
			}
			m_pParser->NextToken();
		}
		else
		{
			m_pParser->LError(NotVarError, m_pParser->CToken->Text);
			m_pParser->NextToken();
			return NULL;
		}
	}

gOut:;

	if(symb != PS_NONE)
	{
		m_pParser->LError(OperatorError, tsymb->Text);
		return NULL;
	}

	if(var == NULL)
	{
		m_pParser->LError(NotVarError, m_pCSymbol);
		return NULL;
	}

	//check post inc/dec symbols
	if(!m_pParser->EndOfLine())
	{
		if(TTDefEqual(m_pParser->CToken, CParser::TPlusPlus))
		{
			if(*ivar != NULL)
			{
				m_pParser->LError("TEMP_ERROR: nesmis pouzivat '--/++' na poli");
				return NULL;
			}
			var = CompileID(lside ? NULL : var, var, PS_INC);
			m_pParser->NextToken();
		}
		else if(TTDefEqual(m_pParser->CToken, CParser::TMinusMinus))
		{
			if(*ivar != NULL)
			{
				m_pParser->LError("TEMP_ERROR: nesmis pouzivat '--/++' na poli");
				return NULL;
			}
			var = CompileID(lside ? NULL : var, var, PS_DEC);
			m_pParser->NextToken();
		}
	}

	//negation
	if(neg && var)
	{
		if(*ivar != NULL && var->GetVType() != VARTYPE_VECTOR)
		{
			m_pParser->LError("TEMP_ERROR: nesmis pouzivat '-' pred polem");
			return NULL;
		}

		Variable *vzero, *temp;
		Stat *stat;

		switch(var->GetVType())
		{
		case VARTYPE_FLOAT:
			vzero = m_pContext->m_pvZeroFloat;
			stat = AddStat(OP_SUBF);
			break;

		case VARTYPE_INT:
			vzero = m_pContext->m_pvNumbersInt[0];
			stat = AddStat(OP_SUBI);
			break;

		case VARTYPE_VECTOR:
			vzero = m_pContext->m_pvZeroVector;
			stat = AddStat(OP_SUBV);
			break;

		default:
			m_pParser->LError(BadVarError, var->Name);
			return NULL;
		}

		SetParm(stat, 0, vzero);
		SetParm(stat, 1, var);
		FreeRegister(var);
		temp = GetRegister(var->GetType());
		SetParm(stat, 2, temp);
		return temp;
	}

	return var;
}

//-----------------------------------------------------------------------
int CCompiler::NextCondition()
{
	if(TTDefEqual(m_pParser->CToken, CParser::TRBracket))
		return 0;

	if(TTDefEqual(m_pParser->CToken, CParser::TAndAnd))
		return 1;

	if(TTDefEqual(m_pParser->CToken, CParser::TOrOr))
		return 2;

	return -1;
}

//-----------------------------------------------------------------------
bool CCompiler::IsExpression(bool left) const
{
	/*
	const TToken* tok = m_pParser->CLine->m_pTokens;

	for(uint n = m_pParser->CParm; n++; n < m_pParser->CLine->m_iNumTokens)
	{
		if(left)
		{
			if(TTDefEqual(tok[n], CParser::TRBracket) ||
				TTDefEqual(tok[n], CParser::TAndAnd ||
				TTDefEqual(tok[n], CParser::TOrOr))
				return false;
		}
		else
		{
			if(TTDefEqual(tok[n], CParser::TRBracket) ||
				TTDefEqual(tok[n], CParser::TAndAnd ||
				TTDefEqual(tok[n], CParser::TOrOr))
				return false;
		}
	}
*/
	return false;
}

//-----------------------------------------------------------------------
bool AssertCompareToken(const TToken* tcmp)
{
	if(tcmp->Type != TT_DEF)
		return false;

	const char* cmp = tcmp->Text;
	return (
		cmp == CParser::TAnd ||
		cmp == CParser::TOr ||
		cmp == CParser::TEqual ||
		cmp == CParser::TNotEqual ||
		cmp == CParser::TLess ||
		cmp == CParser::TLessEqual ||
		cmp == CParser::TGreater ||
		cmp == CParser::TGreaterEqual);
}

//-----------------------------------------------------------------------
bool CCompiler::ParseConditions(CommandOp inst)
{
Variable    *var1, *var2;
Stat        *stat;
OpCode        op,op2;
const TToken* cmp;
int cond;
Scope* ref;

	if(inst == CMD_IF || inst == CMD_WAIT)
		ref = BeginScope(REF_IF);
	else
	{
		//WHILE/FOR
		ref = BeginScope(REF_WHILE);
		ref->Stat2 = m_psStats;
	}

	while(true)
	{
		bool neg;

		if(TTDefEqual(m_pParser->CToken, CParser::TExclamation))
		{
			if(!m_pParser->NextToken(true))
				return false;

			neg = true;
		}
		else
			neg = false;

		if((var2 = ParseRVariable(NULL)) == NULL)
			return false;

		if(var2->GetVType() == VARTYPE_VOID)
		{
			m_pParser->LError("Can't condition 'void' type");
			return false;
		}

		var1 = NULL;

		cond = NextCondition();

		//compare sign or error
		if(cond < 0)
		{
			cmp = m_pParser->CToken;   //store operator position

			if(!AssertCompareToken(cmp))
			{
				m_pParser->LError("Expected compare operator but got this '%s'", cmp->Text);
				return false;
			}
			if(m_pParser->CToken->Type != TT_DEF)
			{
				m_pParser->LError(BadStatError);
				return false;
			}
			m_pParser->NextToken();

			if((var1 = ParseRValue(GetRegister(var2->GetType()), NULL, true)) == NULL)
				return false;

			if(var1->GetVType() == VARTYPE_VOID)
			{
				m_pParser->LError("Can't condition 'void' type");
				return false;
			}

			if((var1 = DoConversion(var2, NULL, var1, NULL, false)) == NULL)
			{
				m_pParser->LError(IncParmError);
				return false;
			}
			cond = NextCondition();
		}

		op = OP_NONE;

		//don't have 2nd operand.
		if(var1 == NULL)
		{
			switch(var2->GetVType())
			{
			case VARTYPE_VECTOR:
				var1 = m_pContext->m_pvZeroVector;
				break;
			case VARTYPE_FLOAT:
				var1 = m_pContext->m_pvZeroFloat;
				break;
			case VARTYPE_STRING:
				var1 = m_pContext->m_pvZeroString;
				break;
			default:
				var1 = m_pContext->m_pvNumbersInt[0];
			}
			op2 = OP_BEQ;
		}
		else
		{
			op2 = FindSign(cmp, var2->GetVType());

			if(op2 == OP_ANDI)
			{
				op = OP_ANDI;
				op2 = OP_BEQ;
			}
			else if(op2 == OP_ORI)
			{
				op = OP_ORI;
				op2 = OP_BEQ;
			}
		}

		if(op == OP_NONE)
		{
			switch(var2->GetVType())
			{
			case VARTYPE_VECTOR:
				op = OP_CMPV;
				break;
			case VARTYPE_FLOAT:
				op = OP_CMPF;
				break;
			case VARTYPE_STRING:
				op = OP_CMPS;
				break;
			default:
				op = OP_CMPI;
			}
		}

		if(op2 == OP_NONE || op == OP_NONE)
		{
			m_pParser->LError(OperatorError, cmp->Text);
			return false;
		}

		//if || follows, solve previous NEGATIVE references, invert condition and store to POSITIVE reference table
		int tab = 0;

		//wait conditions have the opposite meaning
		if(inst == CMD_WAIT)
		{
			if(cond == 2)
				cond = 1;
			else if(cond == 1)
				cond = 2;
		}

		if(cond == 2)
		{
			op2 = Negate(op2);
			tab = 1;
		}

		if(neg)
			op2 = Negate(op2);

		//wait conditions have the opposite meaning
		if(inst == CMD_WAIT)
			op2 = Negate(op2);

		//optimizations for const conditions i.e. while(true)
		if(m_iOptimize > 1 && var1->Spec & VSPEC_CONST && var2->Spec & VSPEC_CONST)
		{
			int icmp, sr = 0;
			float fcmp;

			stat = NULL;

			switch(var1->GetVType())
			{
			case VARTYPE_INT:
				icmp = var2->Int - var1->Int;
				if(icmp == 0)
					sr = SF_ZERO;
				else
				if(icmp < 0)
					sr = SF_NEG;
				else
					sr = 0;
				break;

			case VARTYPE_FLOAT:
				fcmp = var2->Float - var1->Float;
				if(fcmp == 0)
					sr = SF_ZERO;
				else
				if(icmp < 0)
					sr = SF_NEG;
				else
					sr = 0;
				break;
			}

			switch(op2)
			{
				case (OP_BEQ):
					if(sr == SF_ZERO)
					{
						stat = AddStat(OP_GOTO);
					}
				break;


				case (OP_BNE):
					if(!(sr & SF_ZERO))
					{
						stat = AddStat(OP_GOTO);
					}
				break;

				case (OP_BGE):
					if(!(sr & SF_NEG))
					{
						stat = AddStat(OP_GOTO);
					}
				break;

				case (OP_BLE):
					if(sr & (SF_NEG | SF_ZERO))
					{
						stat = AddStat(OP_GOTO);
					}
				break;

				case (OP_BLT):
					if(sr == SF_NEG)
					{
						stat = AddStat(OP_GOTO);
					}
				break;

				case (OP_BGT):
					if(sr == 0)
					{
						stat = AddStat(OP_GOTO);
					}
				break;
			}
		}
		else
		{
			stat = AddStat(op);

			SetParm(stat, 0, var1);
			SetParm(stat, 1, var2);
			//OP_CMP case has only 2 operands
			if(m_iSLen[op] > 12)
				SetParm(stat, 2, GetRegister(var1->GetType()));

			stat = AddStat(op2);
			if(m_iSLen[op2] > 4)
				SetParm(stat, 0, NULL);

		}

		if(cond == 2)
			ref->SolveStat(0, m_psStats);

		if(stat != NULL)
			ref->AddStat(tab, stat);

		//right bracket, leave
		if(cond == 0)
		{
			m_pParser->NextToken();
			break;
		}

		//exception for CMD_FOR
		if(cond == -1 && inst == CMD_FOR)
			break;

		FreeAllRegisters();
		m_UsedReturns.Clear();
		if(!m_pParser->NextToken(true))
			return false;

	}
	ref->SolveStat(1, m_psStats);
	return true;
}

//-----------------------------------------------------------------------
bool CCompiler::ParseEquation()
{
int         n;
Variable    *var3, *ivar3;
const TToken* cmp;

	//check '=','+=','-=','/=','*=','^=','&=','|='
	n = 0;
	for(uint t = m_pParser->GetCurrentToken(); t < m_pParser->CLine->m_iNumTokens; t++)
	{
		cmp = m_pParser->GetToken(t);
		if(TTDefEqual(cmp, CParser::TSeparator))
			break;

		if(cmp->Type != TT_DEF) continue;

		if(TTDefEqual(cmp, CParser::TAssign))
		{
			n = 1;
			break;
		}

		if(TTEqual(*cmp, "+="))
		{
			n = 2;
			break;
		}
		if(TTEqual(*cmp, "-="))
		{
			n = 2;
			break;
		}
		if(TTEqual(*cmp, "/="))
		{
			n = 2;
			break;
		}
		if(TTEqual(*cmp, "*="))
		{
			n = 2;
			break;
		}
		if(TTEqual(*cmp, "^="))
		{
			n = 2;
			break;
		}
		if(TTEqual(*cmp, "&="))
		{
			n = 2;
			break;
		}
		if(TTEqual(*cmp, "|="))
		{
			n = 2;
			break;
		}
	}

	if(n)
	{
		if((var3 = ParseVariable(NULL, &ivar3)) == NULL)
			return false;

		if(var3->Spec & (VSPEC_CONST | VSPEC_PROTECT))
		{
			m_pParser->LError(ConstModifyError, var3->Name);
			return false;
		}

		if(m_pParser->CToken != cmp)
		{
			m_pParser->LError(CParser::GarbageError, m_pParser->CToken->Text);
			return false;
		}

		m_pParser->NextToken();
		if(n == 2)
		{
			OpCode op = FindSign(cmp, var3->GetVType());

			if(ivar3 == NULL)
			{
				Variable *reg;

				//optimize simple case a += b
				if(m_iOptimize > 0 && (m_pParser->CParm + 1) >= m_pParser->CLine->m_iNumTokens)
				{
					reg = ParseVariable(var3->GetType(), &ivar3, false);

					if(reg == NULL)
						return false;
				}
				else
				{
					reg = GetRegister(var3->GetType());
					if(!ParseRValue(reg, NULL))
						return false;
				}

				if(op == OP_NONE)
				{
					m_pParser->LError("Internal compiler error %s : %d", __FILE__, __LINE__);
					return false;
				}

				Stat *st = AddStat(op);
				if(var3->GetVType() != VARTYPE_VECTOR && reg->GetVType() != VARTYPE_VECTOR && (var3->IsPtr() != reg->IsPtr()) && op != OP_MOVE && op != OP_MOVEA)
				{
					m_pParser->LError(MixedArithmetic);
					return false;
				}

				SetParm(st, 0, var3);
				SetParm(st, 1, reg);
				SetParm(st, 2, var3);
			}
			else
			{
				m_pParser->LError("TEMP_ERROR: nepouzivej += atd. na poli");
				return false;
			}
		}
		else
		{
			if(!ParseRValue(var3, ivar3))
				return false;
		}

		return true;
	}

//------------------------------------------------
// either function call or variable
//------------------------------------------------
	if((var3 = ParseVariable(NULL, &ivar3, false)) != NULL)
	{
		if(TTDefEqual(m_pParser->CToken, CParser::TSeparator))
			m_pParser->NextToken();
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------
bool CCompiler::ParseClassDef()
{
	if(!m_pParser->NextToken(true))
	{
		return false;
	}

	const char *cname = AddString(m_pParser->CToken->Text);

	m_pCClass = m_pModule->FindClass(cname);
			
	//already defined?
	if(m_pCClass != NULL)
	{
		if(!(m_pCClass->m_iFlags & CF_FORWARD_DECLARATION) || m_pCClass->Module != m_pModule)
		{
			m_pParser->LError(ClassExistsError, cname);
			return false;
		}
	}

	m_pParser->NextToken();

	if(!m_pParser->EndOfLine() && TTEqual(*m_pParser->CToken, "extends"))
	{
		if(!m_pParser->NextToken(true))
			return false;

		Class* pclass = m_pModule->FindClass(m_pParser->CToken->Text);

		if(pclass == NULL)
		{
			m_pParser->LError(ExtendsError, m_pParser->CToken->Text);
			return false;
		}

		/*if(m_pCClass && m_pCClass->m_iFlags & CF_FORWARD_DECLARATION)
		{
			//just check that inheritance is correct
			if(m_pCClass->GetSuper() != pclass)
			{
				m_pParser->LError("Forward declaration of '%s' has different parent class '%s'", m_pCClass->GetName(), m_pCClass->GetSuper()->GetName());
				return false;
			}
		}
		else*/
		{
			Class* nclass = pclass->NewInheritedClass(m_pModule, cname);

			if(m_pCClass != NULL)
			{
				uint index = m_pCClass->Index;
				//replace forward declaration
				delete m_pModule->m_VarTypes[index];
				nclass->Index = index;
				m_pModule->m_VarTypes[index] = nclass;
			}
			else
			{
				bool res = m_pModule->AddClass(nclass);
				enf_assert(res != false);
			}
			m_pCClass = nclass;


			if(!ExtendClass(m_pCClass, pclass))
			{
				m_pParser->LError(ExtendsError, cname);
				return false;
			}
		}

		m_pParser->NextToken();
	}
	else
	{
		if(m_pCClass == NULL)
			m_pCClass = AddClass(cname);		
	}

	//just forward declaration
	if(TTDefEqual(m_pParser->CToken, CParser::TSeparator))
	{
		m_pParser->NextToken();
		m_pCClass->m_iFlags |= CF_FORWARD_DECLARATION;

		m_pCClass = NULL;
		return true;
	}

	if(!m_pParser->EndOfLine())
	{
		m_pParser->LError(CParser::GarbageError, m_pParser->CToken->Text);
		return false;
	}

	m_pCClass->m_iFlags &= ~CF_FORWARD_DECLARATION;
	enf_assert(m_pCClass);
	m_pContext->m_pvThis->SetType(m_pCClass);

	BeginScope(REF_CLASS);
	return true;
}

//-----------------------------------------------------------------------
bool CCompiler::IsFunctionDef()
{
	if(m_pParser->CLine->m_iNumTokens <= (m_pParser->CParm + 2))
		return false;

	int ct = 0;
	for(int c = m_pParser->CParm + 0; c < m_pParser->CLine->m_iNumTokens; c++)
	{
		if(TTDefEqual(&m_pParser->CToken[ct], CParser::TLSBracket))
			continue;
		if(TTDefEqual(&m_pParser->CToken[ct], CParser::TRSBracket))
			continue;
		break;
	}
	if(TTDefEqual(&m_pParser->CToken[ct + 1], CParser::TLBracket) || TTDefEqual(&m_pParser->CToken[ct + 1], CParser::TFourDot))
		return true;

	return false;
}

//-----------------------------------------------------------------------
bool CCompiler::ParseFor()
{
	if(!m_pParser->AssertToken(CParser::TLBracket))
		return false;

	//skip first part?
	if(!TTDefEqual(m_pParser->CToken, CParser::TSeparator))
	{
		if(IsType() == -1)
		{
			return false;
		}

		//is it variable
		if(m_pCVarType == NULL)
		{
			if(!ParseEquation())
				return false;
		}
		else
		{
			//variable definition
			if(!ParseVarDef((m_pCClass == NULL) || (m_pCFunct != NULL), true, m_pCVarType, m_iCVSpec))
			{
				return false;
			}
		}
		if(!m_pParser->AssertToken(CParser::TSeparator))
			return false;
	}
	else
	{
		if(!m_pParser->NextToken(true))
			return false;
	}


	//2nd part just remember. Will be processed as the last one
	int pt, lt;
	Stat *pst;

	pst = NULL;
	//search for 3rd part
	uint n = pt  = m_pParser->GetCurrentToken();

	for(; n < m_pParser->CLine->m_iNumTokens; n++)
	{
		if(TTDefEqual(m_pParser->GetToken(n), CParser::TSeparator))
			break;
	}

	m_pParser->CurrentToken(n);
	if(!m_pParser->AssertToken(CParser::TSeparator))
		return false;

	if(!TTDefEqual(m_pParser->CToken, CParser::TRBracket))
	{
		Stat* stat = AddStat(OP_GOTO);
		//beginning of loop
		pst = m_psStats;
		if(!ParseEquation())
			return false;
		stat->SetJmp(m_psStats);
		if(!m_pParser->AssertToken(CParser::TRBracket))
			return false;
	}
	else
		m_pParser->NextToken();

	lt = m_pParser->GetCurrentToken();

	m_pParser->CurrentToken(pt);

	//force debug info
	m_pDLine = m_pParser->CLine;

	//skip 2nd part?
	if(!TTDefEqual(m_pParser->CToken, CParser::TSeparator))
	{
		if(!ParseConditions(CMD_FOR))
			return false;
	}
	else
	{
		Scope *ref = BeginScope(REF_WHILE);
		ref->Stat2 = m_psStats;
	}

	//HACK: we have to set goto pointer before condition
	if(pst) m_rRefs[m_iNumScopes].Stat2 = pst;

	if(!m_pParser->AssertToken(CParser::TSeparator))
		return false;

	m_pParser->CurrentToken(lt);
	return true;
}

//-----------------------------------------------------------------------
bool CCompiler::ParseCase()
{
	Scope* rf = FindScope(REF_NULL);

	if(rf == NULL || rf->Type != REF_SWITCH)
	{
		m_pParser->LError(BreakContError);
		m_bRes = false;
		return false;
	}

	Variable* ivar1 = NULL;
	Variable* var1 = ParseVariable(rf->Var->GetType(), &ivar1);
	if(var1 == NULL)
	{
		return false;
	}

	if(!m_pParser->AssertToken(CParser::TDoubleDot))
	{
		return false;
	}

	if((var1 = DoConversion(rf->Var, NULL, var1, NULL, false)) == NULL)
	{
		m_pParser->LError(IncParmError,""/*var1->Name*/);
		return false;
	}

	OpCode op;

	switch(rf->Var->GetVType())
	{
	case VARTYPE_VECTOR:
		op = OP_CMPV;
		break;
	case VARTYPE_FLOAT:
		op = OP_CMPF;
		break;
	case VARTYPE_STRING:
		op = OP_CMPS;
		break;
	default:
		op = OP_CMPI;
	}

	Stat* stat = AddStat(op);
	SetParm(stat, 0, var1);
	SetParm(stat, 1, rf->Var);

	//solve OP_BNE from previous case
	rf->SolveStat(0, stat);

	stat = AddStat(OP_BNE);
	rf->AddStat(0, stat);
	return true;
}

//-----------------------------------------------------------------------
bool CCompiler::ParseBreakContinue(CommandOp inst)
{
	if(TTDefEqual(m_pParser->CToken, CParser::TSeparator))
		m_pParser->NextToken();

	//search for nearest while/switch
	Scope* rf = FindScope(REF_WHILE);

	if(inst == CMD_BREAK)
	{
		Scope* rf2 = FindScope(REF_SWITCH);

		if(rf == NULL)
			rf = rf2;

		if(rf2 > rf)
			rf = rf2;

		if(rf)
		{
			AddBreakRef(rf, AddStat(OP_GOTO));
			return true;
		}
	}
	else
	{
		if(rf)
		{
			AddStat(OP_GOTO)->SetJmp(rf->Stat2);
			return true;
		}
	}

	m_pParser->LError(BreakContError);
	return false;
}

//-----------------------------------------------------------------------
bool CCompiler::ParseEndOfFunc()
{
	bool res = true;
	//solve references
	for(uint g = 0; g < m_TRefs.GetCardinality(); g++)
	{
		if(m_TRefs[g].Type == REF_GOTO)
		{
			uint t;
			for(t = 0; t < m_TRefs.GetCardinality(); t++)
			{
				if(m_TRefs[t].Type == REF_LABEL &&
					!strcmp(m_TRefs[t].Name, m_TRefs[g].Name))
				{
					m_TRefs[g].RStat[0][0]->SetJmp(m_TRefs[t].RStat[0][0]);
					break;
				}
			}

			if(t == m_TRefs.GetCardinality())
			{
				const TLine* bl = m_pParser->CLine;

				m_pParser->CLine = (TLine *)m_TRefs[g].Stat2;
				m_pParser->LError(Reference2Error);
				m_pParser->CLine = bl;

				res = false;
			}
		}
	}
	m_TRefs.Clear();
	AddStat(OP_RTS);

	m_pCFunct->Length = (intptr_t)m_psStats - (intptr_t)m_Stats;
	m_pCFunct->FirstStat = (Stat*)m_pModule->m_pStatic->AllocData(m_pCFunct->Length, m_pCFunct->FirstStat);

	//fix debug-info references
	if(m_pModule->m_pDebugInfo)
	{
		DebugInfo::Line* lines = m_pModule->m_pDebugInfo->m_Lines.GetArray();

		for(uint s = m_iFirstDebugLine; s < m_pModule->m_pDebugInfo->m_Lines.GetCardinality(); s++)
			lines[s].m_iOffset += ((char*)m_pCFunct->FirstStat - (char*)m_pModule->m_pStatic->GetData());
	}

	//Add listing
	if(m_bList && m_bRes)
	{
		const Stat* st = m_pCFunct->FirstStat;
		const Stat* endstat = (Stat*)((char*)m_pCFunct->FirstStat + m_pCFunct->Length);

		ScriptListing::Add(m_pCClass, m_pCFunct);

		while(st < endstat)
		{
			ScriptListing::Add(st);
			st = (const Stat*)((char *)st + st->GetSizeof());
		}
	}

	m_pCFunct = NULL;
	if(m_pCClass != NULL && m_bMethodOutsideClass)
	{
		m_pCClass = NULL;
	}
	m_iNumScopes--; //EndScope???
	return res;
}

//-----------------------------------------------------------------------
bool CCompiler::ParseProto()
{
	if(m_iNumScopes > 0)
	{
		if(m_rRefs[m_iNumScopes - 1].Type != REF_CLASS)
		{
			m_pParser->LError(CParser::SequenceError);
			return false;
		}
	}
	if(!m_pParser->NextToken(true))
		return false;

	if(IsType() == -1)
		return false;

	const VarType* vt = m_pCVarType;
	uint flags = m_iFuncFlags;
	if((flags & FN_NATIVE) == 0)
		flags |= FN_INTERN;

	uint varFlags = m_iCVSpec;

	const char *pfname = NULL;
	void *p = NULL;

	//is it DLL function?
	if(m_pParser->CToken->Type == TT_QUOTE)
	{
#ifdef WIN32
		void* mod = _LoadLibrary(m_pParser->CToken->Text);
#endif
		if(!m_pParser->NextToken(true))
			return false;

		pfname = m_pCSymbol = AddString(m_pParser->CToken->Text);
		flags = FN_NATIVE;
#ifdef WIN32
		p = _GetProcAddress(mod, pfname);

		if(p == NULL)
		{
			m_pParser->LError(ProtoError,m_pCSymbol);
			return false;
		}
#endif
	}
	else
	{
		pfname = m_pCSymbol = AddString(m_pParser->CToken->Text);
	}

	if(!m_pParser->NextToken(true))
		return false;

/*
TODO: in case of class-methods, we need to avoid reporting override methods from ancestors!!!
	if(FindFunction(m_pCSymbol, m_pCClass, m_pCClass != NULL))
	{
		m_pParser->LError("Proto for %s already exists", m_pCSymbol);
		return false;
	}
*/
	m_pCFunct = AddFunction(m_pCSymbol, flags, m_pCClass);
	m_pCFunct->Func = (SCAPI_FUNC)p;

	if(!m_pParser->AssertToken(CParser::TLBracket))
		return false;

	ParseFunctParms(vt);

	if(TTDefEqual(m_pParser->CToken, CParser::TSeparator))
		m_pParser->NextToken();

	if(m_pCFunct->ReturnValue->GetVType() == VARTYPE_STRING)
		m_pCFunct->ReturnValue->Spec |= (varFlags & VSPEC_OWNED);

	m_pCFunct = NULL;
	m_pLastRts = NULL;
	return m_pParser->Garbage();
}

//-----------------------------------------------------------------------
bool CCompiler::ParseFunctionDef(const VarType* vt, uint funcFlags)
{
	if(m_pCFunct)
	{
		m_pParser->LError(CParser::SequenceError);
		return false;
	}

	if(funcFlags & (FN_NATIVE|FN_INTERN))
	{
		m_pParser->LError("Script function cannot be native");
		return false;
	}

	Funct* pf = m_pCFunct;

	const char *fname;
	bool prototyped = false;
	
	if(TTDefEqual(&m_pParser->CToken[1], CParser::TFourDot))
	{
		m_pCClass = (Class*)m_pModule->FindType(m_pParser->CToken->Text);

		if(m_pCClass == NULL || m_pCClass->Type != VARTYPE_CLASS)
		{
			m_pParser->LError(BadVarError, m_pParser->CToken->Text);
			return false;
		}
		if(!m_pParser->NextToken(true))
			return false;
		if(!m_pParser->NextToken(true))
			return false;

		m_bMethodOutsideClass = true;
		prototyped = true;

		m_pCFunct = m_pCClass->GetMethod(m_pParser->CToken->Text);
		if(m_pCFunct == NULL)
		{
			m_pParser->LError(UndefinedFunction, m_pParser->CToken->Text);
			return false;
		}

		if((m_pCFunct->Flags & FN_SCRIPT) == 0 || m_pCFunct->FirstStat != NULL)
		{
			m_pParser->LError("Function '%s' already has body", m_pParser->CToken->Text);
			return false;
		}
	}
	else
	{
		m_bMethodOutsideClass = false;

		fname = AddString(m_pParser->CToken->Text);

		m_pParser->NextToken();
		pf = FindFunction(fname, m_pCClass, (m_pCClass != NULL));

		bool inherited = false;
		if(m_pCClass == NULL)
		{
			if(pf && pf->FirstStat)
			{
				m_pParser->LError(MultDecError, fname);
				return false;
			}
		}
		else
		{
			if(pf)
			{
				Funct* ppf = NULL;

				//get it from superclass.
				for(Class *cl = m_pCClass->Super; cl; cl = cl->Super)
				{
					if((ppf = FindFunction(fname, cl, (m_pCClass != NULL))) != NULL)
					{
						inherited = true;
						break;
					}
				}

				//there is not such function in superclasses - it's just duplicity
				if(ppf != pf)
				{
					m_pParser->LError(MultDecError, fname);
					return false;
				}
			}
		}

		//prototyped function
		if(pf != NULL && pf->FirstStat == NULL && !inherited)
		{
			prototyped = true;
			m_pCFunct = pf;
		}
		else
			m_pCFunct = AddFunction(fname, FN_SCRIPT|funcFlags, m_pCClass);
	}

	if(!prototyped)
	{
		m_pParser->NextToken();
		if(!ParseFunctParms(vt, pf))
			return false;
/*
		while(!m_pParser->EndOfLine())
		{
			if(TTEqual(*m_pParser->CToken, "event"))
			{
				m_pCFunct->Flags |= FN_EVENT;
				m_pParser->NextToken();
				continue;
			}
			if(TTEqual(*m_pParser->CToken, "private"))
			{
				m_pCFunct->Flags |= FN_PRIVATE;
				m_pParser->NextToken();
				continue;
			}
			break;
		}
*/
		if(!m_pParser->EndOfLine() && TTDefEqual(m_pParser->CToken, CParser::TSeparator))
		{
			m_pParser->NextToken();
			m_pCFunct->FirstStat = NULL;
			m_pCFunct = NULL;
			return true;
		}
	}
	else
	{
		//TODO: what to do with variables/parameters? Do we need anything to do?

		m_pCFunct->FirstStat = m_psStats = m_Stats;

		if(m_pModule->m_pDebugInfo)
			m_iFirstDebugLine = m_pModule->m_pDebugInfo->m_Lines.GetCardinality();

		m_iStatsPerFunc = 0;

		while(m_pParser->NextToken())
		{
			if(TTDefEqual(m_pParser->CToken, CParser::TRBracket))
			{
				m_pParser->NextToken();
				break;
			}
		}
	}

	BeginScope(REF_FUNC);

	m_iNumRefVars = 0;
	m_iLastRef = m_pModule->m_Refs.GetCardinality();

	if(m_pCFunct)
	{
		if(m_pModule->m_pDebugInfo)
		{
			m_pCFunct->DebugOffset = m_pModule->m_pDebugInfo->m_Lines.GetCardinality();
			m_pDLine = m_pParser->CLine;
		}
	}
	return true;
}

//-----------------------------------------------------------------------
bool CCompiler::ParseDelete()
{
	Variable* var1 = ParseRVariable(NULL);

	if(var1 == NULL || var1->GetVType() != VARTYPE_CLASS)
	{
		m_pParser->LError(IncParmError, "on delete");
		return false;
	}

	Class* cl = (Class*)var1->GetType();
	bool res = true;
	if(cl->Destructor && cl->GetMethod(cl->Destructor - 1))
	{
		if(cl->GetMethod(cl->Destructor - 1)->Flags & FN_PRIVATE)
		{
			m_pParser->LError(MethodIsPrivate, cl->GetMethod(cl->Destructor - 1)->GetName());
			res = false;
		}
	}

	BeforeCall(NULL);
	Stat* stat = AddStat(OP_DELETE);
	SetParm(stat, 0, var1);
	AfterCall(NULL);
	if(TTDefEqual(m_pParser->CToken, CParser::TSeparator))
		m_pParser->NextToken();

	return res;
}

//-----------------------------------------------------------------------
bool CCompiler::ParseTypeDef()
{
	if(!m_pParser->NextToken(true))
		return false;

	const VarType* vt = m_pModule->FindType(m_pParser->CToken->Text);

	if(vt == NULL)
	{
		m_pParser->LError(BadVarError, m_pParser->CToken->Text);
		return false;
	}

	if(!m_pParser->NextToken(true))
		return false;

	//add to vartypes
	VarType* nvt = m_pModule->AddType(m_pParser->CToken->Text, VARTYPE_VOID, 0);

	m_pParser->NextToken();

	nvt->Type = vt->Type;
	nvt->Sizeof = vt->Sizeof;
	if(TTDefEqual(m_pParser->CToken, CParser::TSeparator))
		m_pParser->NextToken();

	return m_pParser->Garbage();
}

//-----------------------------------------------------------------------
bool CCompiler::ParseClamp()
{
	Variable* ivar1;
	Variable* var1 = ParseVariable(NULL, &ivar1);
	if(var1 == NULL)
		return false;

	if(!m_pParser->AssertToken(CParser::TLess))
		return false;

	Variable* var2 = ParseRVariable(var1->GetType());
	if(var2 == NULL)
		return false;

	if(!m_pParser->AssertToken(CParser::TComma))
		return false;

	Variable* var3  = ParseRVariable(var1->GetType());
	if(var3 == NULL)
		return false;
	
	if(!m_pParser->AssertToken(CParser::TGreater))
		return false;

	OpCode op = OP_NONE;

	if(var1->GetVType() == VARTYPE_INT)
		op = OP_CLAMPI;
	else if(var1->GetVType() == VARTYPE_FLOAT)
		op = OP_CLAMPF;
	else
	{
		m_pParser->LError(IncParmError, var1->Name);
		return false;
	}

	Stat* stat = AddStat(op);
	SetParm(stat, 0, var2);
	SetParm(stat, 1, var3);
	SetParm(stat, 2, var1);

	if(TTDefEqual(m_pParser->CToken,CParser::TSeparator))
		m_pParser->NextToken();

	return m_pParser->Garbage();
}

//-----------------------------------------------------------------------
#define CASSERT(a) if(!(a)) {m_bRes = false; continue;}

class CCompilerContext
{
public:
	Class*	m_pPrevClass;
	Class*	m_pPrevThisClass;
	CCompiler* m_pComp;

	CCompilerContext(CCompiler* comp, Class* newClass)
	{
		m_pComp = comp;
		m_pPrevClass = comp->m_pCClass;
		m_pPrevThisClass = static_cast<Class*>(comp->GetContext()->m_pvThis->GetType());
		comp->m_pCClass = newClass;
		if(newClass)
			comp->GetContext()->m_pvThis->SetType(newClass);
	}

	~CCompilerContext()
	{
		m_pComp->m_pCClass = m_pPrevClass;
		m_pComp->GetContext()->m_pvThis->SetType(m_pPrevThisClass);
	}
};

//-----------------------------------------------------------------------
bool CCompiler::CompileFragment(CParser& parser, Class* thisClass)
{
//	AssertMainThread();

	//no code - no fun!
	if(parser.GetNumLines() == 0)
		return true;

	CMutexLock lock(m_pContext->GetMutex());
	CCompilerContext cctx(this, thisClass);

	m_pCFunct = NULL;
	m_pCSymbol = NULL;
	m_pParser = &parser;

	//rewind it
	parser.BeginLine(0);

#if defined(ENF_EXCEPTION) && !defined(_DEBUG)
	try {
#endif

	parser.m_bCLF = false;
	uint tm = INDEX_NOT_FOUND;
	//int lastsrcline = 0;
	const TLine* lastsrcline = NULL;
	//uint lline = 0;

	while(true)
	{
		CommandOp inst;

		//lline++;
		//SString<>str;
		//OutputDebugString(str.format("%s:%d\n", parser.CLine->m_ptmModule->m_strName, parser.CLine->m_iSourceLine));
		
		if(parser.m_bCLF || parser.EndOfLine())
		{
			if(!parser.NextLine(false)) break;
			parser.m_bCLF = false;
			continue;
		}

		//isn't block crossing end of file?
		if(tm != parser.CLine->m_iModule)
		{
			if(m_iNumScopes && (tm == INDEX_NOT_FOUND || parser.GetModule(tm)->m_iSourceLine >= (int)lastsrcline))
			{
				if(parser.CLine == &parser.GetLine(0))
				{
					parser.LError("Unexpected end of file");
				}
				else
				{
					parser.CLine--;
					parser.LError("Unexpected end of file");
					parser.CLine++;
				}
				m_bRes = false;
				parser.m_bCLF = true;
			}
			tm = parser.CLine->m_iModule;
		}
		const TModule* tmod = parser.GetModule(tm);
//		lastsrcline = parser.CLine->m_iSourceLine;
		lastsrcline = parser.CLine;


		m_UsedReturns.Clear();

/*		if(lastsrcline == 486 && CStr(tmod->GetName()).strstr("UEC_ProductionAdvisor.h" ))
		{
			lastsrcline=lastsrcline;
		}
*/
		m_pDLine = parser.CLine;

		FreeAllRegisters();

		m_pObj = NULL;
		m_iFunctionIndex = 0;

		if(TTDefEqual(parser.CToken, CParser::TRCurly))
		{
			inst = CMD_END;
		}
		else if(TTDefEqual(parser.CToken, CParser::TLCurly))
		{
			parser.NextToken();
			BeginScope(REF_BLOCK)->AddStat(0, m_psStats);
			continue;
		}
		else
		{
			if(!m_Keywords.Find(parser.CToken->Text, inst))
				inst = CMD_BAD;
		}

		if(inst != CMD_ELSE)
		{
			if(m_rRefs[m_iNumScopes].Lines > 0 && (IsScopeType(REF_IF) || IsScopeType(REF_ELSE)))
				EndScope()->SolveStat(0, m_psStats);//solve reference
		}

		if(m_rRefs[m_iNumScopes].Lines > 0 && (IsScopeType(REF_WHILE) || IsScopeType(REF_SWITCH)))
		{
			if(IsScopeType(REF_WHILE))
			{
				Stat* stat = AddStat(OP_GOTO);
				stat->SetJmp(m_rRefs[m_iNumScopes].Stat2);
				EndScope()->SolveStat(0, m_psStats); //solve reference
			}
			else
			{
				//solve OP_BNE from previous case
				EndScope()->SolveStat(0, m_psStats);
			}

			//solve references
			for(uint b = 0; b < m_rRefs[m_iNumScopes + 1].NumBRefs; b++)
				m_rRefs[m_iNumScopes + 1].BRefs[b]->SetJmp(m_psStats);

			m_BRefs.SetSize(m_BRefs.GetCardinality() - m_rRefs[m_iNumScopes + 1].NumBRefs);
		}

		if(inst == CMD_END)
		{
			parser.NextToken();

			if(!IsScopeType(REF_BLOCK))
			{
				parser.LError(BlockError);
				m_bRes = false;
				continue;
			}

			EndScope();

			if(IsScopeType(REF_FUNC))
			{
				m_bRes &= ParseEndOfFunc();
				continue;
			}

			if(IsScopeType(REF_CLASS))
			{
				m_bRes &= CheckClassCompatibility(m_pCClass);

				m_pCClass = NULL;
				m_pCFunct = NULL;
				m_iNumScopes--; //EndScope???

				if(TTDefEqual(parser.CToken, CParser::TSeparator))
					parser.NextToken();
			}

			continue;
		}

		m_rRefs[m_iNumScopes].Lines++;

		if(inst == CMD_PROTO)
		{
			m_bRes &= ParseProto();
			continue;
		}

		if(inst == CMD_TYPEDEF)
		{
			m_bRes &= ParseTypeDef();
			continue;
		}

		if(inst == CMD_CLASS)
		{
/*			if(m_iNumScopes > 0)
			{
				parser.LError(CParser::SequenceError);
				m_bRes = false;
				continue;
			}

			m_bRes &= ParseClassDef();
			continue;
*/
			if(m_iNumScopes == 0)
			{
				m_bRes &= ParseClassDef();
				continue;
			}
			inst = CMD_BAD;
			m_bRes = m_bRes;
		}

//Is it type definition?
		if(inst == CMD_BAD)
		{
			if(IsType() == -1)
			{
				m_bRes = false;
				continue;
			}

			const VarType* vt = m_pCVarType;
			short vspec = m_iCVSpec;
			uint funcFlags = m_iFuncFlags;

//------------------------------------------------
//either variable or function declaration
//-------------------------------------------------
			if(vt != NULL)
			{
				if(IsFunctionDef())
				{
					m_bRes &= ParseFunctionDef(vt, funcFlags);
					continue;
				}

				//variable definition

				//don't alloc classmembers
				if(m_pCClass != NULL && m_pCFunct == NULL)
					vspec |= VSPEC_CLASSMEMBER;

				CASSERT(ParseVarDef((m_pCClass == NULL) || (m_pCFunct != NULL), true, vt, vspec));

				if(TTDefEqual(parser.CToken,CParser::TSeparator))
					parser.NextToken();

				continue;
			}
		}

//------------------------------------------------
//next lines must be always in block
//------------------------------------------------
		if(!IsScopeType(REF_BLOCK) &&
			!IsScopeType(REF_IF) &&
			!IsScopeType(REF_ELSE) &&
			!IsScopeType(REF_SWITCH) &&
			!IsScopeType(REF_WHILE))
		{
			parser.LError(NotInBlockError);
			m_bRes = false;
			continue;
		}

		if(m_pCFunct == NULL)
		{
			parser.LError(NotInBlockError);
			m_bRes = false;
			continue;
		}

		if(inst == CMD_BAD)
		{
//------------------------------------------------
//goto label
//-------------------------------------------------
			if((parser.CLine->m_iNumTokens + parser.CParm) >= 2 && TTDefEqual(&parser.CToken[1], CParser::TDoubleDot))
			{
				for(uint n = 0; n < m_TRefs.GetCardinality(); n++)
				{
					if(m_TRefs[n].Type == REF_LABEL &&
					TTEqual(*parser.CToken, m_TRefs[n].Name))
					{
						parser.LError(DupLabelError, m_TRefs[n].Name);
						m_bRes = false;
					}
				}
				Scope& scope = m_TRefs.Append();
				scope.Name = parser.CToken->Text;
				scope.Type = REF_LABEL;
				scope.RStat[0][0] = m_psStats;

				parser.NextToken();
				parser.NextToken();

				if(TTDefEqual(parser.CToken, CParser::TSeparator))
					parser.NextToken();
				continue;
			}

//------------------------------------------------
//equation / variable / function call / error
//-------------------------------------------------
			m_bRes &= ParseEquation();

			if(TTDefEqual(parser.CToken, CParser::TSeparator))
				parser.NextToken();

			continue;
		}

		parser.NextToken();

		// code
		switch(inst)
		{
		case CMD_CACHE:
			CASSERT(parser.AssertEOL());
			if(m_pCClass)
			{
				m_pCClass->MaxCached = enf_min<int>(1024, atoi(parser.CToken->Text));
				parser.NextToken();
			}
			if(TTDefEqual(parser.CToken, CParser::TSeparator))
				parser.NextToken();

			m_bRes &= parser.Garbage();
			break;

		case (CMD_DEBUG):
			{
				Stat* stat = AddStat(OP_DEBUG);
				uint dline = m_pModule->m_pDebugInfo->AddDebugLine(
					parser.GetModule(parser.CLine->m_iModule)->GetName(),
					parser.CLine->m_iSourceLine,
					(char*)stat - (char*)m_Stats);

				stat->SetOffset((dline - m_pCFunct->DebugOffset) | DBGF_BREAK);
			}
			break;

		case (CMD_CASE):
			m_bRes &= ParseCase();
			break;

		case (CMD_DEFAULT):
			{
				Scope* rf = FindScope(REF_NULL);

				if(rf == NULL || rf->Type != REF_SWITCH)
				{
					parser.LError(BreakContError);
					m_bRes = false;
					continue;
				}

				CASSERT(parser.AssertToken(CParser::TDoubleDot));

				//solve OP_BNE from previous case
				rf->SolveStat(0, m_psStats);
			}
			break;

		case (CMD_GOTO):
			{
				CASSERT(parser.AssertEOL());

				Stat* stat = AddStat(OP_GOTO);

				Scope& scope = m_TRefs.Append();
				scope.Type = REF_GOTO;
				scope.RStat[0][0] = stat;
				scope.Stat2 = (Stat *)parser.CLine;  //for error reporting
				scope.Name = parser.CToken->Text;
				parser.NextToken();
				if(TTDefEqual(parser.CToken, CParser::TSeparator))
					parser.NextToken();
			}
			break;

		case (CMD_BREAK):
		case (CMD_CONTINUE):
			m_bRes &= ParseBreakContinue(inst);
			break;

		case (CMD_RETURN):
			if(m_pCFunct->ReturnValue->GetVType() != VARTYPE_VOID)
			{
				if(!parser.AssertEOL())
				{
					parser.LError(NotRetValueError);
					m_bRes = false;
					break;
				}

				if(!parser.EndOfLine())
					m_bRes &= (ParseRValue(m_pCFunct->ReturnValue, NULL) != NULL);
			}

			if(TTDefEqual(parser.CToken, CParser::TSeparator))
				parser.NextToken();

			m_bRes &= parser.Garbage();
			AddStat(OP_RTS);
			break;

		case (CMD_CLAMP):
			m_bRes &= ParseClamp();
			break;

		case (CMD_FOR):
			m_bRes &= ParseFor();
			break;

		case (CMD_IF):
		case (CMD_WHILE):
			CASSERT(parser.AssertToken(CParser::TLBracket));
			CASSERT(ParseConditions(inst));
			break;

		case (CMD_SLEEP):
			{
				CASSERT(parser.AssertToken(CParser::TLBracket));
				//cannot use register - variable should be static!
				Variable* var1 = m_pModule->AllocVariable(m_pContext->m_vtFloatType, 1, true, 0);
				m_pModule->m_Variables.Insert(var1);
				CASSERT((ParseRValue(var1, NULL)) != NULL);
				BeforeCall(m_pCFunct);
				Stat* stat = AddStat(OP_WAIT);
				AfterCall(m_pCFunct);
				SetParm(stat, 0, var1);
				CASSERT(parser.AssertToken(CParser::TRBracket));
				if(TTDefEqual(parser.CToken, CParser::TSeparator))
					parser.NextToken();
			}
			break;

		case (CMD_WAIT):
			{
				Stat* pst = m_psStats;
				CASSERT(parser.AssertToken(CParser::TLBracket));
				CASSERT(ParseConditions(CMD_WAIT));
				BeforeCall(m_pCFunct);
				AddStat(OP_WAIT);
				AfterCall(m_pCFunct);
				AddStat(OP_GOTO)->SetJmp(pst);
				EndScope()->SolveStat(0, m_psStats);//solve reference
				if(TTDefEqual(parser.CToken, CParser::TSeparator))
					parser.NextToken();
			}
			break;

		case (CMD_SWITCH):
			{
				CASSERT(parser.AssertToken(CParser::TLBracket));

				Variable* var1 = ParseRVariable(NULL);
				if(var1 == NULL)
				{
					m_bRes = false;
					continue;
				}

				CASSERT(parser.AssertToken(CParser::TRBracket));

				BeginScope(REF_SWITCH)->Var = var1;
			}
			break;

		case (CMD_ELSE):
			if(!IsScopeType(REF_IF))
			{
				parser.LError(ElseError);
				m_bRes = false;
				break;
			}
			{
				Stat* stat = AddStat(OP_GOTO);

				//solve reference from IF
				m_rRefs[m_iNumScopes].SolveStat(0, m_psStats);

				m_rRefs[m_iNumScopes].Type = REF_ELSE;
				m_rRefs[m_iNumScopes].AddStat(0, stat);
				m_rRefs[m_iNumScopes].Lines = 0;
			}
			break;

		case (CMD_DELETE):
			m_bRes &= ParseDelete();
			break;
		}
	}
	return m_bRes;

#if defined(ENF_EXCEPTION) && !defined(_DEBUG)
	}
	catch(...)
	{
		parser.CLine = NULL;
		parser.LError("Compiler crashed");
		return false;
	};
#endif
	return true;
}

//-----------------------------------------------------------------------
void CCompiler::Begin(ScriptContext* ctx, ScriptModule* mod, const char* listingName)
{
	enf_assert(ctx != NULL);

	m_TRefs.Clear();
	m_bRes = true;
	m_pDLine = NULL;

	m_pContext = ctx;
	if(mod != NULL)
		m_pModule = mod;
	else
		m_pModule = ctx;

	m_iAllocated = m_pModule->m_pStatic->Allocated();

	if(m_pModule->m_pDebugInfo == NULL)
	{
		m_pModule->m_pDebugInfo = ENF_NEW DebugInfo;
	}

	ZeroMem(m_rRefs, sizeof(m_rRefs[0]));
	
	m_BRefs.Clear();

	FreeAllRegisters();

	m_pCFunct = NULL;
	m_pCClass = NULL;
	m_pCSymbol = NULL;
	m_bMethodOutsideClass = false;
	ctx->m_pvThis->SetType(ctx->m_vtClassType);

	m_iNumScopes = 0;
	m_rRefs[0].Type = REF_NULL;

	if(m_bList)
	{
		m_bList &= ScriptListing::Init(this, listingName);
	}
}

//-----------------------------------------------------------------------
void CCompiler::End()
{
	m_pParser->CLine = NULL;
	m_pModule->m_Stats.m_iAllocated = m_iAllocated - m_pModule->m_Stats.m_iAllocated;
	m_pModule->m_Stats.m_iStats = m_iNumStats;
	m_pContext->m_pvThis->SetType(m_pContext->m_vtClassType);

	if(m_bList)
		ScriptListing::Close();
}

//-----------------------------------------------------------------------
bool CCompiler::Compile(CParser& parser, ScriptContext* ctx, ScriptModule* mod, const char* listingName)
{
	Begin(ctx, mod, listingName);

	bool res = CompileFragment(parser);

	End();
	//
	return res;
}

//-----------------------------------------------------------------------
CCompiler::CCompiler():
	m_pContext(NULL),
	m_pModule(NULL),
	m_iNumStats(0),
	m_PrevCStat(NULL),
	m_pLastRts(NULL),
	m_bList(false),
	m_iOptimize(9),
	m_psStats(NULL),
	m_iStatsPerFunc(0),
	m_iNumRefVars(0),
	m_iFirstDebugLine(0),
	m_iFunctionIndex(0),
	m_pCFunct(NULL),
	m_pCClass(NULL),
	m_bMethodOutsideClass(false),
	m_iIndexCount(0),
	m_pDLine(NULL),
	m_pCSymbol(NULL),
	m_pObj(NULL),
	m_iCVSpec(0),
	m_pCVarType(NULL),
	m_iNumScopes(0),
	m_iLastRef(0),
	m_iVRegMask(0),
	m_iSRegMask(0),
	m_iCRegMask(0),
	m_iFRegMask(0),
	m_iRRegMask(0),
	LastError(NULL),
	m_iAllocated(0)
{
	if(!m_bInitialized)
	{
		m_bInitialized = true;
		//init instruction set
		m_iSLen[OP_WAIT] = 8;
		m_iSLen[OP_GOTO] = 4;
		m_iSLen[OP_ICALL] = 4;
		m_iSLen[OP_NCALL] = 4;
		m_iSLen[OP_NCALLI] = 4;
		m_iSLen[OP_TCALL] = 4;
		m_iSLen[OP_RTS] = 4;
		m_iSLen[OP_BGE] = 4;
		m_iSLen[OP_BGT] = 4;
		m_iSLen[OP_BLE] = 4;
		m_iSLen[OP_BLT] = 4;
		m_iSLen[OP_BEQ] = 4;
		m_iSLen[OP_BNE] = 4;
		m_iSLen[OP_NEW] = 16;
		m_iSLen[OP_DELETE] = 8;
		m_iSLen[OP_DEBUG] = 4;
		m_iSLen[OP_PUSH] = 8;
		m_iSLen[OP_PUSHC] = 8;
		m_iSLen[OP_PUSHS] = 8;
		m_iSLen[OP_PUSHV] = 8;
		m_iSLen[OP_POP] = 8;
		m_iSLen[OP_POPC] = 8;
		m_iSLen[OP_POPS] = 8;
		m_iSLen[OP_POPV] = 8;

		m_iSLen[OP_MOVE] = 12;
		m_iSLen[OP_MOVEI] = 12;
		m_iSLen[OP_MOVES] = 12;
		m_iSLen[OP_MOVEV] = 12;
		m_iSLen[OP_MOVEA] = 12;
		m_iSLen[OP_IMOVE] = 12;
		m_iSLen[OP_MOVETR] = 12;
		m_iSLen[OP_MOVEFR] = 12;
		m_iSLen[OP_INC] = 8;
		m_iSLen[OP_DEC] = 8;
		m_iSLen[OP_CITF] = 12;
		m_iSLen[OP_CFTI] = 12;
		m_iSLen[OP_CMPI] = 12;
		m_iSLen[OP_CMPF] = 12;
		m_iSLen[OP_CMPV] = 12;
		m_iSLen[OP_CMPS] = 12;
		m_iSLen[OP_SIZEOF] = 12;

		m_iSLen[OP_CLAMPF] = 16;
		m_iSLen[OP_CLAMPI] = 16;
		m_iSLen[OP_FFV] = 16;
		m_iSLen[OP_FTV] = 16;
		m_iSLen[OP_IFV] = 12;
		m_iSLen[OP_ITV] = 12;
		m_iSLen[OP_CFV] = 16;
		m_iSLen[OP_CTV] = 16;
		m_iSLen[OP_MODI] = 16;
		m_iSLen[OP_ADDI] = 16;
		m_iSLen[OP_ADDF] = 16;
		m_iSLen[OP_ADDV] = 16;
		m_iSLen[OP_ADDS] = 16;
		m_iSLen[OP_ANDI] = 16;
		m_iSLen[OP_ORI] = 16;
		m_iSLen[OP_EORI] = 16;
		m_iSLen[OP_LSLI] = 16;
		m_iSLen[OP_ASRI] = 16;
		m_iSLen[OP_SUBI] = 16;
		m_iSLen[OP_SUBF] = 16;
		m_iSLen[OP_SUBV] = 16;
		m_iSLen[OP_MULI] = 16;
		m_iSLen[OP_MULF] = 16;
		m_iSLen[OP_CROSSV] = 16;
		m_iSLen[OP_SCALEV] = 16;
		m_iSLen[OP_DOTV] = 16;
		m_iSLen[OP_DIVI] = 16;
		m_iSLen[OP_DIVF] = 16;


		static SignDef SignsArray[] =
		{
			CParser::TAsterisk,  VARTYPE_INT,   OP_MULI,
			CParser::TAsterisk,  VARTYPE_FLOAT, OP_MULF,
			CParser::TAsterisk,  VARTYPE_VECTOR,OP_CROSSV,
			CParser::TSlash,  VARTYPE_INT,   OP_DIVI,
			CParser::TSlash,  VARTYPE_FLOAT, OP_DIVF,
			CParser::TPlus,  VARTYPE_INT,   OP_ADDI,
			CParser::TPlus,  VARTYPE_FLOAT, OP_ADDF,
			CParser::TPlus,  VARTYPE_VECTOR,OP_ADDV,
			CParser::TPlus,  VARTYPE_STRING,OP_ADDS,
			CParser::TMinus,  VARTYPE_INT,   OP_SUBI,
			CParser::TMinus,  VARTYPE_FLOAT, OP_SUBF,
			CParser::TMinus,  VARTYPE_VECTOR,OP_SUBV,
			CParser::TPercent,  VARTYPE_INT,   OP_MODI,
			CParser::TPower,  VARTYPE_INT,   OP_EORI,
			CParser::TLShift,  VARTYPE_INT,  OP_LSLI,
			CParser::TRShift,  VARTYPE_INT,  OP_ASRI,
			CParser::TOr,  VARTYPE_INT,   OP_ORI,
			CParser::TAnd,  VARTYPE_INT,   OP_ANDI,
			CParser::TEqual,  VARTYPE_NONE, OP_BNE,
			CParser::TGreater,  VARTYPE_NONE,  OP_BLE,
			CParser::TLess,  VARTYPE_NONE,  OP_BGE,
			CParser::TLessEqual, VARTYPE_NONE,  OP_BGT,
			CParser::TGreaterEqual, VARTYPE_NONE,  OP_BLT,
			CParser::TNotEqual, VARTYPE_NONE,  OP_BEQ,

			CParser::TPlusAssign, VARTYPE_INT, OP_ADDI,
			CParser::TPlusAssign, VARTYPE_FLOAT, OP_ADDF,
			CParser::TPlusAssign, VARTYPE_VECTOR, OP_ADDV,
			CParser::TPlusAssign, VARTYPE_STRING, OP_ADDS,
			CParser::TMinusAssign, VARTYPE_INT, OP_SUBI,
			CParser::TMinusAssign, VARTYPE_FLOAT, OP_SUBF,
			CParser::TMinusAssign, VARTYPE_VECTOR, OP_SUBV,
			CParser::TSlashAssign, VARTYPE_INT, OP_DIVI,
			CParser::TSlashAssign, VARTYPE_FLOAT, OP_DIVF,
			CParser::TAsteriskAssign, VARTYPE_INT, OP_MULI,
			CParser::TAsteriskAssign, VARTYPE_FLOAT, OP_MULF,
			CParser::TPowerAssign, VARTYPE_INT, OP_EORI,
			CParser::TAndAssign, VARTYPE_INT, OP_ANDI,
			CParser::TOrAssign, VARTYPE_INT, OP_ORI,
			CParser::TLShiftAssign, VARTYPE_INT, OP_LSLI,
			CParser::TRShiftAssign, VARTYPE_INT, OP_ASRI
		};

		Signs = SignsArray;
		NumSigns = sizeof(SignsArray) / sizeof(SignsArray[0]);
	}

#define ADD_CMD(s, c) m_Keywords.Insert(s, c)

	ADD_CMD("if", CMD_IF);
	ADD_CMD("else", CMD_ELSE);
	ADD_CMD("break", CMD_BREAK);
	ADD_CMD("continue", CMD_CONTINUE);
	ADD_CMD("class", CMD_CLASS);
	ADD_CMD("while", CMD_WHILE);
	ADD_CMD("for", CMD_FOR);
	ADD_CMD("return", CMD_RETURN);
	ADD_CMD("switch", CMD_SWITCH);
	ADD_CMD("case", CMD_CASE);
	ADD_CMD("default", CMD_DEFAULT);
	ADD_CMD("delete", CMD_DELETE);
	ADD_CMD("proto", CMD_PROTO);
	ADD_CMD("goto", CMD_GOTO);
	ADD_CMD("debug", CMD_DEBUG);
	ADD_CMD("typedef", CMD_TYPEDEF);
	ADD_CMD("clamp", CMD_CLAMP);
	ADD_CMD("cache", CMD_CACHE);
	ADD_CMD("Wait", CMD_WAIT);
	ADD_CMD("Sleep", CMD_SLEEP);
}

}