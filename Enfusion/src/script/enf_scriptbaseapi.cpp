//-----------------------------------------------------------------------------
// File: enf_scriptbaseapi.cpp
//
// Desc: Basic script API
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "script/enf_scriptmain.h"
#include "script/enf_scriptcontext.h"
#include "script/enf_scriptthread.h"
#include "script/enf_scriptdebuginfo.h"
#include "common/enf_parser.h"
#include "common/enf_staticbuffer.h"
#include "system/enf_timer.h"
#include "system/enf_time.h"


using namespace enf;

//-----------------------------------------------------------------------
ENF_INLINE Instance* AssertInstance(Instance* en)
{
	if(en == NULL)
		ScriptContext::m_pCurrent->Error(ScriptContext::RTE_NullPointerError);

	return en;
}

#define RegFunctBaseAPI(b) LinkFunction(#b, (SCAPI_FUNC)BaseAPI:: b)

namespace BaseAPI
{
//-----------------------------------------------------------------------
//class Spawn(ctype)
//-----------------------------------------------------------------------
SCAPI(Spawn)
{
	result->InstancePtr = NULL;
	if(parm[0]->Pointer)
	{
		result->InstancePtr = ScriptContext::m_pCurrent->New((Class*)parm[0]->Pointer, true);
	}
}

//-----------------------------------------------------------------------
// proto int		Type(string var)
//-----------------------------------------------------------------------
VarType* NMETHOD Type(const char *name)
{
	return ScriptContext::m_pCurrent->FindType(name, true);
}

//-----------------------------------------------------------------------
// proto int		IsInherited(class cl,int type)
//-----------------------------------------------------------------------
uint NMETHOD IsInherited(Instance *inst, VarType *vt)
{
	AssertInstance(inst);

	if(vt && vt->Type == VARTYPE_CLASS)
	{
		if(inst->InstanceOf(static_cast<const Class*>(vt)))
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------
// proto int		IsTypeInherited(int clType,int type)
//-----------------------------------------------------------------------
uint NMETHOD IsTypeInherited( VarType* clType, VarType* vt )
{
	enf_assert( clType && vt && clType->Type == VARTYPE_CLASS && vt->Type == VARTYPE_CLASS );

	return ScriptContext::m_pCurrent->FindClass( clType->GetName() )->IsInherited( vt );
}

//-----------------------------------------------------------------------
// proto string	ClassName(class cl)
//-----------------------------------------------------------------------
const char* NMETHOD ClassName(Instance *inst)
{
	return AssertInstance(inst)->GetClass()->GetName();
}

//----------------------------------------------------------------------
//proto void Idle()
//----------------------------------------------------------------------
void NMETHOD Idle()
{
	ScriptContext::m_pCurrent->ProcessThreads(0, 0);
}

SCAPI(Watch)
{
	IVariable* var = parm[0];

	ScriptContext::m_pCurrent->AddWatchVariable((const Variable*)var, parm[1]->Int);
	//	lvar = *g_pScript->GetParmDef(0);
	//	lvar.Pointer = ;
}

//----------------------------------------------------------------------
//proto int KillThread(class owner, string name)
//----------------------------------------------------------------------
int NMETHOD KillThread(Instance* inst, const char* name)
{
	AssertInstance(inst);
	uint res = 0;

	for(ScriptThread *st = inst->m_psThread; st; st = st->m_pUserNext)
	{
		if(st->GetStackPos() >= 1 && st->GetStack(1)->m_pfFunc)
		{
			if(CStr(name) == st->GetStack(1)->m_pfFunc->GetName())
			{
				st->Kill();
				res++;
			}
		}
	}

	return res;
}

//----------------------------------------------------------------------
//proto string ThreadFunction(class owner, string name, int backtrace, int line)
//----------------------------------------------------------------------
SCAPI(ThreadFunction)
{
	Instance *inst = parm[0]->InstancePtr;
	const char *name = parm[1]->String;

	for(const ScriptThread *st = inst ? inst->m_psThread : ScriptContext::m_pCurrent->GetFirstThread(); st; st = inst ? st->m_pUserNext : st->GetNext())
	{
		if(st->GetStackPos() >= 1 && st->GetStack(1)->m_pfFunc)
		{
			if(CStr(name) == st->GetStack(1)->m_pfFunc->GetName())
			{
				int spos = st->GetStackPos() - parm[2]->Int;
				spos = enf_max(1, spos);

				result->String = (char*)st->GetStack(spos)->m_pfFunc->GetName();

				if((ScriptContext::m_pCurrent->GetParmDef(3)->Spec & (VSPEC_CONST|VSPEC_PROTECT)) == 0)
				{
					const VMStack* stack = st->GetStack(spos);
					if(stack->m_pfFunc->GetModule()->GetDebugInfo())
						stack->m_pfFunc->GetModule()->GetDebugInfo()->GetLineInfoByOffset(stack->GetDebugOffset(), NULL, (uint*)&parm[3]->Int);
				}
				return;
			}
		}
	}
	result->String = (char*)ScriptContext::m_pCurrent->NullString;
}


//-----------------------------------------------------------------------
//	proto string		itoa(int number)
//-----------------------------------------------------------------------
SCAPI(itoa)
{
	sprintf((char *)result->Pointer, "%d", parm[0]->Int);
}

//-----------------------------------------------------------------------
//	proto string		tostr(int ascii)
//-----------------------------------------------------------------------
SCAPI(tostr)
{
	sprintf((char *)result->Pointer, "%c", parm[0]->Int);
}

//-----------------------------------------------------------------------
//	proto string		ftoa(float number)
//-----------------------------------------------------------------------
SCAPI(ftoa)
{
	sprintf((char *)result->Pointer, "%g", (double)parm[0]->Float);
}

//-----------------------------------------------------------------------
//	proto float		atof(string number)
//-----------------------------------------------------------------------
float NMETHOD atof(const char* number)
{
	return (float)::atof(number);
}

//-----------------------------------------------------------------------
//	proto int		atoi(string str)
//-----------------------------------------------------------------------
int NMETHOD atoi(const char *str)
{
	return atol(str);
}

//-----------------------------------------------------------------------
//	proto string		substr(string s, int from, int len)
//-----------------------------------------------------------------------
SCAPI(substr)
{
	if(parm[1]->Int < 0 || (uint)(parm[1]->Int + parm[2]->Int) > enf::strlen(parm[0]->String))
		ScriptContext::m_pCurrent->Error(ScriptContext::RTE_WrongParameterValue);

	if(parm[2]->Int > 0)
		CopyMem(result->Pointer, parm[0]->String + parm[1]->Int, parm[2]->Int + 1);
	else
		if(parm[2]->Int < 0)
			ScriptContext::m_pCurrent->Error(ScriptContext::RTE_WrongParameterValue);

	result->String[parm[2]->Int] = 0;

}

//-----------------------------------------------------------------------
//	proto string trim(string s)
//-----------------------------------------------------------------------
SCAPI(trim)
{
	const char* str = parm[0]->String;

	while(*str == ' ')
		str++;

	uint lastchar = 0;

	for(uint s = 0;; s++)
	{
		char chr = result->String[s] = *str++;

		if(chr == 0)
			break;

		if(chr != ' ')
			lastchar = s;
	}
	result->String[lastchar + 1] = 0;
}

//-----------------------------------------------------------------------
//	proto int triminplace(string s)
//-----------------------------------------------------------------------
SCAPI(triminplace)
{
	if((ScriptContext::m_pCurrent->GetParmDef(0)->Spec & (VSPEC_CONST|VSPEC_PROTECT)) != 0)
	{
		result->Int = 0;
		return;
	}

	char* str = parm[0]->String;
	uint off;
	for(off = 0; str[off] == ' '; off++);

	int lastchar = -1;

	if(off == 0)
	{
		for(uint s = 0;; s++)
		{
			char chr = str[s];

			if(chr == 0)
				break;

			if(chr != ' ')
				lastchar = s;
		}
	}
	else
	{
		for(uint s = 0;; s++)
		{
			char chr = str[s] = str[s + off];

			if(chr == 0)
				break;
			if(chr != ' ')
				lastchar = s;
		}
	}
	str[lastchar + 1] = 0;
	result->Int = lastchar + 1;
}

//-----------------------------------------------------------------------
//	proto int		strlen(string s)
//-----------------------------------------------------------------------
int NMETHOD strlen(const char *str)
{
	return enf::strlen(str);
}

//-----------------------------------------------------------------------
//	proto int		strstr(string s1, string sample)
//-----------------------------------------------------------------------
int NMETHOD strstr(const char *src, const char *sample)
{
	const char *str = ::strstr(src, sample);

	if(str == NULL)
		return -1;

	return (int)(str - src);
}

//-----------------------------------------------------------------------
//	proto int		strstroff(string s1, int offset, string sample)
//-----------------------------------------------------------------------
int NMETHOD strstroff(const char *src, uint offset, const char *sample)
{
	size_t srclen = enf::strlen(src);
	size_t samplen = enf::strlen(sample);
	
	if((offset + samplen) > srclen)
		return -1;

	const char *str = ::strstr(src + offset, sample);

	if(str == NULL)
		return -1;

	return (int)(str - src);
}

//-----------------------------------------------------------------------
//	proto int		strlower(string s)
//-----------------------------------------------------------------------
SCAPI(strlower)
{
	char *p = parm[0]->String;
	int l;
	for(l = 0; p[l]; l++)
		p[l] = tolower(p[l]);

	result->Int = l;
}

//-----------------------------------------------------------------------
//	proto int		strupper(string s)
//-----------------------------------------------------------------------
SCAPI(strupper)
{
	char *p = parm[0]->String;
	int l;
	for(l = 0; p[l]; l++)
		p[l] = toupper(p[l]);

	result->Int = l;
}

//-----------------------------------------------------------------------
//proto int floor(float f)
//-----------------------------------------------------------------------
int NMETHOD floor(float f)
{
	return (int)::floorf(f);
}

//-----------------------------------------------------------------------
//proto float sin(float rad)
//-----------------------------------------------------------------------
float NMETHOD sin(float rad)
{
	return ::sinf(rad);
}

//-----------------------------------------------------------------------
//proto float pow(float val, float power)
//-----------------------------------------------------------------------
float NMETHOD pow(float val, float power)
{
	return ::powf(val, power);
}

//-----------------------------------------------------------------------
//proto float fabs(float val)
//-----------------------------------------------------------------------
float NMETHOD fabs(float val)
{
	return ::fabsf(val);
}

//-----------------------------------------------------------------------
//proto int abs(int val)
//-----------------------------------------------------------------------
int NMETHOD abs(int val)
{
	return ::abs(val);
}

//-----------------------------------------------------------------------
float NMETHOD FixAngle(float v)
{
	while(v < 0)
		v += 360.0f;

	while(v >= 360.0f)
		v -= 360.0f;

	return v;
}

//-----------------------------------------------------------------------
float NMETHOD cos(float rad)
{
	return ::cosf(rad);
}

//-----------------------------------------------------------------------
float NMETHOD sqrt(float in)
{
	return ::sqrtf(in);
}

//-----------------------------------------------------------------------
float NMETHOD tan(float rad)
{
	return ::tanf(rad);
}

//-----------------------------------------------------------------------
float NMETHOD asin(float v)
{
	return ::asinf(enf_limit(v, -1.0f, 1.0f));
}

//-----------------------------------------------------------------------
float NMETHOD acos(float v)
{
	return ::acosf(enf_limit(v, -1.0f, 1.0f));
}

//-----------------------------------------------------------------------
float NMETHOD atan2(float y, float x)
{
	return ::atan2f(y, x);
}

//-----------------------------------------------------------------------
void CopyToken(const char *v1, int len, char **v2)
{
	if(ScriptContext::m_pCurrent->IsStatic(v1) && v1[len] == 0)
	{
		if(!ScriptContext::m_pCurrent->IsStatic(*v2))
			ScriptContext::m_pCurrent->FreeString(*v2);

		*v2 = (char*)v1;
	}
	else
	{
		uint l = len + 1;

		if(ScriptContext::m_pCurrent->IsStatic(*v2))
			*v2 = (char*)ScriptContext::m_pCurrent->AllocString(l, v1);
		else
		{
			if(l > ScriptContext::MemSize(*v2))
			{
				ScriptContext::m_pCurrent->FreeString(*v2);
				*v2 = (char*)ScriptContext::m_pCurrent->AllocString(l, v1);
			}
			else
				CopyMem(*v2, v1, len);
		}

		(*v2)[len] = 0;
	}
}

//-----------------------------------------------------------------------
// proto int ParseString(string input, string tokens[])
//-----------------------------------------------------------------------
SCAPI(ParseString)
{
	int ntokens = ScriptContext::m_pCurrent->GetParmDef(1)->ArraySize();

	const char *lptr, *eptr;
	bool	comm, lcomm;
	char **tokens = (char**)parm[1]->Pointer;
	int ntoken = 0;

	lptr = parm[0]->String;
	eptr = lptr + enf::strlen(parm[0]->String);

	comm = lcomm = false;

	while(lptr < eptr && ntoken < ntokens)
	{
		const char *t;
		const char *tdef;
		int l,s;

		t = CParser::SkipSpaces(lptr, eptr, &s);

		if(t != lptr)
		{
			lptr = t;
			continue;
		}

		//eol
		if(*lptr == 0x0d)
			break;

		//check lf
		if(*lptr == 0x0a)
			break;

		//if rest of line commented, skip
		if(lcomm)
		{
			lptr++;
			continue;
		}

		//if we are in commented block, find eoc
		if(comm)
		{
			if((eptr - lptr) >= 2 && lptr[0] == '*' && lptr[1] == '/')
			{
				lptr += 2;
				comm = false;
			}
			else
				lptr++;

			continue;
		}

		if(!comm && !lcomm)
		{
			enf::TokenType ttype;
			//try to parse number
			//numbers must precede FindToken because of '-' and '.'
			t = CParser::ParseNumber(lptr, eptr, ttype);
			if(t)
			{
				CopyToken(lptr, t - lptr, &tokens[ntoken++]);
				lptr = t;
				continue;
			}
		}

		//try to find defined token
		if((tdef = CParser::FindToken(lptr, eptr, &l)) != NULL)
		{
			lptr += l;	//skip token

			if(t == CParser::TComment)
			{
				lcomm = true;
				continue;
			}

			if(tdef == CParser::TLComment)
			{
				comm = true;
				continue;
			}

			//add token
			CopyToken(tdef, l, &tokens[ntoken++]);
			continue;
		}

		if(comm || lcomm)
			continue;

		enf::TokenType ttype;
		//try to find free tokens
		t = CParser::ParseString(lptr, eptr, ttype);
		if(t == NULL)
			break;

		if(t > lptr)
		{
			CopyToken(lptr, t - lptr, &tokens[ntoken++]);

			lptr = t;
			continue;
		}

		//nothing works, alloc one char as token - is it neccessary?
		CopyToken(lptr, 1, &tokens[ntoken++]);
		lptr++;
	}

	result->Int = ntoken;
}

//-----------------------------------------------------------------------
void CopyToken2(const char *v1, uint len, char **v2)
{
	uint l = len + 1;

	if(ScriptContext::m_pCurrent->IsStatic(*v2))
		*v2 = ScriptContext::m_pCurrent->AllocString(l, v1);
	else
	{
		if(l > ScriptContext::MemSize(*v2))
		{
			ScriptContext::m_pCurrent->FreeString(*v2);
			*v2 = ScriptContext::m_pCurrent->AllocString(l, v1);
		}
		else
			CopyMemOverlapped(*v2, v1, len);
	}

	(*v2)[len] = 0;
}

//-----------------------------------------------------------------------
// proto int ParseStringEx(string input, string token)
//-----------------------------------------------------------------------
SCAPI(ParseStringEx)
{
	if(ScriptContext::m_pCurrent->GetParmDef(0)->IsConst() ||
		ScriptContext::m_pCurrent->GetParmDef(1)->IsConst())
	{
		ScriptContext::m_pCurrent->Error("parameter is constant");
	}

	const char *lptr = parm[0]->String;
	bool	comm = false, lcomm = false;
	int rtoken = 0;

	while(*lptr)
	{
		const char *tdef;
		int l;

		const char *t = CParser::SkipSpaces(lptr);

		if(t != lptr)
		{
			lptr = t;
			continue;
		}

		//eol
		if(*lptr == 0x0d)
			lptr++;

		//check lf
		if(*lptr == 0x0a)
		{
			CopyToken2(lptr, 1, &parm[1]->String);
			lptr++;
			rtoken = 5;
			break;
		}

		//if rest of line commented, skip
		if(lcomm)
		{
			//TODO: remove it after converting .ent files to XML!!!!
			if(*lptr == ';')
				lcomm = false;

			lptr++;
			continue;
		}

		//if we are in commented block, find eoc
		if(comm)
		{
			if(lptr[0] == '*' && lptr[1] == '/')
			{
				lptr += 2;
				comm = false;
			}
			else
				lptr++;

			continue;
		}

		if(!comm && !lcomm)
		{
			enf::TokenType ttype;
			//try to parse number
			//numbers must precede FindToken because of '-' and '.'
			t = CParser::ParseNumber(lptr, ttype);
			if(t)
			{
				CopyToken2(lptr, t - lptr, &parm[1]->String);
				lptr = t;
				rtoken = 4;
				break;
			}
		}

		//try to find defined token
		if((tdef = CParser::FindToken(lptr, &l)) != NULL)
		{
			lptr += l;	//skip token

			if(tdef == CParser::TComment)
			{
				lcomm = true;
				continue;
			}

			if(tdef == CParser::TLComment)
			{
				comm = true;
				continue;
			}

			//add token
			CopyToken2(tdef, l, &parm[1]->String);
			rtoken = 1;
			break;
		}

		if(comm || lcomm)
			continue;

		enf::TokenType ttype;
		//try to find free tokens
		t = CParser::ParseString(lptr, ttype);
		if(t == NULL)
			break;

		if(t > lptr)
		{
			if(ttype == TT_QUOTE)
			{
				CopyToken2(lptr + 1, t - lptr - 2, &parm[1]->String);
				rtoken = 2;
			}
			else
			{
				CopyToken2(lptr, t - lptr, &parm[1]->String);
				rtoken = 3;
			}

			lptr = t;
		}
		break;
	}

	if(rtoken != 0)
	{
		CopyToken2(lptr, enf::strlen(lptr), &parm[0]->String);
	}

	result->Int = rtoken;
}

//-----------------------------------------------------------------------
bool GetVariable(Instance *inst, char *name, VType type, IVariable **ivar, Variable **var)
{
	*ivar = NULL;
	*var = NULL;

	//find variable, either in Global variables, or Class variables
	if(inst == NULL)
	{
		*var = ScriptContext::m_pCurrent->GetVariable(name);
		*ivar = (IVariable*)*var;
	}
	else
	{
		if((*var = inst->GetClass()->GetVariable(name)) != NULL)
			*ivar = inst->GetIVariable((*var)->Pos);
	}

	if(*ivar == NULL)
		return false;

	if((*var)->GetVType() != type)
		return false;

	return true;
}

//-----------------------------------------------------------------------
// proto int movearray(void dst[], int dstoffset, void src[], int srcoffset, int count)
//-----------------------------------------------------------------------
SCAPI(movearray)
{
	const Variable *dstdef = ScriptContext::m_pCurrent->GetParmDef(0);
	const Variable *srcdef = ScriptContext::m_pCurrent->GetParmDef(2);

	if(srcdef->GetVType() != dstdef->GetVType() || !srcdef->IsPtr() || !dstdef->IsPtr())
	{
		ScriptContext::m_pCurrent->Error("Type is not compatible");
	}

	int size = parm[4]->Int * srcdef->GetType()->Sizeof;
	ubyte* dstptr = (ubyte*)parm[0]->Pointer + parm[1]->Int * dstdef->GetType()->Sizeof;
	ubyte* srcptr = (ubyte*)parm[2]->Pointer + parm[3]->Int * srcdef->GetType()->Sizeof;

	memmove(dstptr, srcptr, size);
}
SCAPI(reversearray)
{
	const Variable* def = ScriptContext::m_pCurrent->GetParmDef(0);
//	int size = def->GetType()->Sizeof;
	int size = def->ArraySize();

	uint* dstptr = (uint*)parm[0]->Pointer;
	uint* srcptr = (uint*)parm[0]->Pointer + (size - 1);

	while(srcptr > dstptr)
	{
		enf_swap(*srcptr, *dstptr);
		srcptr--;
		dstptr++;
	}
}
SCAPI(copyarray)
{
	const Variable* dstDef = ScriptContext::m_pCurrent->GetParmDef(0);
	const Variable* srcDef = ScriptContext::m_pCurrent->GetParmDef(1);
	int size = enf_min(dstDef->ArraySize(), srcDef->ArraySize());

	uint* dstptr = (uint*)parm[0]->Pointer;
	uint* srcptr = (uint*)parm[1]->Pointer;

	for(uint i = 0; i < size; i++)
	{
		dstptr[i] = srcptr[i];
	}
}

//-----------------------------------------------------------------------
// proto int	GetValue(class ptr, string var, int index, void var)
//-----------------------------------------------------------------------
SCAPI(GetValue)
{
Variable *var;
IVariable *ivar;
IVariable *dvar = parm[3];
const Variable *def = ScriptContext::m_pCurrent->GetParmDef(3);

	result->Int = false;
	if(GetVariable(parm[0]->InstancePtr, parm[1]->String, def->GetVType(), &ivar, &var))
	{
		int index = parm[2]->Int;

		if(index > 0 && (var->ArraySize() <= 1))
			return;

		int dim;

		if(var->ArraySize() <= 1)
		{
			if(var->GetVType() == VARTYPE_STRING)
				ScriptContext::m_pCurrent->CopyString(ivar->String, def, &dvar->String);
			else
				dvar->Int = ivar->Int;
		}
		else
		{
			if((dim = def->ArraySize()) <= 1)
				dvar->Int = ((int *)ivar->Pointer)[index];
			else
			{
				uint size = def->GetType()->Sizeof;
				CopyMem(dvar->Pointer,
							&((char *)ivar->Pointer)[index * dim * size],
							dim * size);
			}
		}

		result->Int = true;
	}
}

//-----------------------------------------------------------------------
// proto int	SetVar(void var, string value)
//-----------------------------------------------------------------------
bool IsNum(char *s)
{
	while(*s)
	{
		if((*s >= '0' && *s <= '9') ||
			*s == '.' ||
			*s == '-' ||
			*s == ' ')
		{
			s++;
			continue;
		}
		return false;
	}
	return true;
}

SCAPI(SetVar)
{
Variable *var = (Variable*)parm[0];

	TToken tt;
	char temp[65536] = "\"";
	tt.Spaces = 0;

	switch(ScriptContext::m_pCurrent->GetParmDef(0)->GetVType())
	{
	case VARTYPE_INT:
		if(!IsNum(parm[1]->String))
		{
			result->Int = false;
			return;
		}
		tt.Type = TT_NUMBER;
		tt.Text = parm[1]->String;
	break;

	case VARTYPE_FLOAT:
		if(!IsNum(parm[1]->String))
		{
			result->Int = false;
			return;
		}
		tt.Type = TT_NUMBER;
		tt.Text = parm[1]->String;
	break;

	case VARTYPE_STRING:
	case VARTYPE_VECTOR:
		tt.Type = TT_STRING;
		enf::strcpy(&temp[1], parm[1]->String);
		enf::strcat(temp, "\"");
		tt.Text = temp;
	break;

	default:
		result->Int = false;
		return;
	}

	result->Int = var->Set(&tt, var, 0);
}

//-----------------------------------------------------------------------
// proto void GetHourMinuteSecond(int hour, int minute, int second);
//-----------------------------------------------------------------------
SCAPI(GetHourMinuteSecond)
{
	SDateTime curTime;
	curTime.SetCurrent();
	parm[0]->Int = (int)curTime.Hour;
	parm[1]->Int = (int)curTime.Minute;
	parm[2]->Int = (int)curTime.Second;
}

SCAPI(GetHourMinuteSecondUTC)
{
	SDateTime curTime;
	curTime.SetCurrentUTC();
	parm[0]->Int = (int)curTime.Hour;
	parm[1]->Int = (int)curTime.Minute;
	parm[2]->Int = (int)curTime.Second;
}


//-----------------------------------------------------------------------
// proto void GetYearMonthDay(int year, int month, int day);
//-----------------------------------------------------------------------
SCAPI(GetYearMonthDay)
{
	SDateTime curTime;
	curTime.SetCurrent();
	parm[0]->Int = (int)curTime.Year;
	parm[1]->Int = (int)curTime.Month;
	parm[2]->Int = (int)curTime.Day;
}

SCAPI(GetYearMonthDayUTC)
{
	SDateTime curTime;
	curTime.SetCurrentUTC();
	parm[0]->Int = (int)curTime.Year;
	parm[1]->Int = (int)curTime.Month;
	parm[2]->Int = (int)curTime.Day;
}

//-----------------------------------------------------------------------
// proto int	SetValue(class ptr, string var, int index, void var)
//-----------------------------------------------------------------------
SCAPI(SetValue)
{
	Variable *var;
	IVariable *ivar;
	IVariable *svar = parm[3];
	const Variable *def = ScriptContext::m_pCurrent->GetParmDef(3);

	result->Int = false;
	if(GetVariable(parm[0]->InstancePtr, parm[1]->String, def->GetVType(), &ivar, &var))
	{
		int index = parm[2]->Int;

		if(index > 0 && (var->ArraySize() <= 1))
			return;

		int dim;

		if(var->ArraySize() <= 1)
		{
			if(var->GetVType() == VARTYPE_STRING)
				ScriptContext::m_pCurrent->CopyString(svar->String, var, &ivar->String);
			else
				ivar->Int = svar->Int;
		}
		else
		{
			//not supported yet
			if(var->GetVType() == VARTYPE_STRING)
				return;

			if((dim = def->ArraySize()) <= 1)
				((int *)ivar->Pointer)[index] = svar->Int;
			else
			{
				uint size = def->GetType()->Sizeof;

				CopyMem(&((char *)ivar->Pointer)[index * dim * size],
							svar->Pointer,
							dim * size);
			}
		}

		result->Int = true;
	}
}
//-----------------------------------------------------------------------
int NMETHOD rand(int min, int max)
{
	int dif,res;

	dif = max - min;
	if(dif > 0)
	{
		int rmax = RAND_MAX;
		if(dif <= 0xffff && rmax <= 0xffff)
			res = ((::rand() * dif) / RAND_MAX) + min;
		else
		{
			res = (int)((((int64)::rand() * (int64)dif) / rmax) + min);
		}

		if(res < min) res = min;
		if(res >= max) res = max - 1;
	}
	else
		res = min;

	return res;
}

//-----------------------------------------------------------------------
int NMETHOD randomize(int seed)
{
	if (seed == -1)
	{
		seed = (int) Timer::GetTime();
	}

	::srand(seed);

	return seed;
}


//-----------------------------------------------------------------------
float NMETHOD frand(float fmin, float fmax)
{
	double r = (double)::rand() / RAND_MAX;

	return (float)(fmin + (r * (fmax - fmin - 0.00001f)));
}

int ENF_CDECL _CompareString(const void *a, const void *b)
{
	char *e1, *e2;

	e1 = *(char **)a;
	e2 = *(char **)b;

	return enf::enf_strcmpi(e1, e2);
}

int ENF_CDECL _CompareInt(const void *a, const void *b)
{
	int e1, e2;

	e1 = *(int *)a;
	e2 = *(int *)b;

	return (e1 - e2);
}

int ENF_CDECL _CompareFloat(const void *a, const void *b)
{
	float e1, e2;

	e1 = *(float *)a;
	e2 = *(float *)b;

	if(e1 < e2)
		return -1;

	if(e1 > e2)
		return 1;

	return 0;
}

//----------------------------------------------------------------------
//proto int		Sort(void array[], int num)
//----------------------------------------------------------------------
SCAPI(Sort)
{
	void *array = parm[0]->Pointer;
	const Variable *vdef = ScriptContext::m_pCurrent->GetParmDef(0);

	if(vdef->Spec & (VSPEC_CONST|VSPEC_PROTECT))
		return;

	switch(vdef->GetVType())
	{
	case VARTYPE_INT:
		qsort(array, parm[1]->Int, sizeof(int), _CompareInt);
		break;
	case VARTYPE_FLOAT:
		qsort(array, parm[1]->Int, sizeof(float), _CompareFloat);
		break;
	case VARTYPE_STRING:
		qsort(array, parm[1]->Int, sizeof(char *), _CompareString);
		break;
	}
}

void NMETHOD MemoryValidation(int enable)
{
	if(enable)
	{
		EnableMemoryValidation();
		MemoryManager::Validate();
	}
	else
		DisableMemoryValidation();
}

}

//-----------------------------------------------------------------------
void ScriptContext::AfterCompile()
{
	RegFunctBaseAPI(Type);
	RegFunctBaseAPI(IsInherited);
	RegFunctBaseAPI(IsTypeInherited);
	RegFunctBaseAPI(ClassName);
	RegMethod2(ScriptModule, ScriptModule, Call, _Call, this);
	RegMethod2(ScriptModule, ScriptModule, CallFunction, _CallFunction, this);
	RegFunctBaseAPI(Spawn);
	RegFunctBaseAPI(Idle);
	RegFunctBaseAPI(Watch);
	RegFunctBaseAPI(KillThread);
	RegFunctBaseAPI(ThreadFunction);
	RegFunctBaseAPI(tostr);
	RegFunctBaseAPI(itoa);
	RegFunctBaseAPI(ftoa);
	RegFunctBaseAPI(substr);
	RegFunctBaseAPI(trim);
	RegFunctBaseAPI(triminplace);
	RegFunctBaseAPI(atoi);
	RegFunctBaseAPI(atof);
	RegFunctBaseAPI(strlen);
	RegFunctBaseAPI(strstr);
	RegFunctBaseAPI(strstroff);
	RegFunctBaseAPI(strlower);
	RegFunctBaseAPI(strupper);
	RegFunctBaseAPI(FixAngle);
	RegFunctBaseAPI(sqrt);
	RegFunctBaseAPI(fabs);
	RegFunctBaseAPI(abs);
	RegFunctBaseAPI(floor);
	RegFunctBaseAPI(sin);
	RegFunctBaseAPI(pow);
	RegFunctBaseAPI(cos);
	RegFunctBaseAPI(tan);
	RegFunctBaseAPI(asin);
	RegFunctBaseAPI(acos);
	RegFunctBaseAPI(atan2);
	RegFunctBaseAPI(ParseString);
	RegFunctBaseAPI(ParseStringEx);
	RegFunctBaseAPI(GetValue);
	RegFunctBaseAPI(SetValue);
	RegFunctBaseAPI(SetVar);
	RegFunctBaseAPI(rand);
	RegFunctBaseAPI(randomize);
	RegFunctBaseAPI(frand);
	RegFunctBaseAPI(Sort);
	RegFunctBaseAPI(MemoryValidation);
	RegFunctBaseAPI(GetHourMinuteSecond);
	RegFunctBaseAPI(GetYearMonthDay);
	RegFunctBaseAPI(GetHourMinuteSecondUTC);
	RegFunctBaseAPI(GetYearMonthDayUTC);
	RegFunctBaseAPI(reversearray);
	RegFunctBaseAPI(copyarray);
}
