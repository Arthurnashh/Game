//-----------------------------------------------------------------------------
// File: enf_scriptprofile.cpp
//
// Desc: Script profiler
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "script/enf_scriptmain.h"
#include "script/enf_scriptprofile.h"
#include "common/enf_quicksort.h"

namespace enf
{
#ifdef ENF_DEVELOPER

//-----------------------------------------------------------------------
bool FromSuper(Class* c, Funct* f, uint findex)
{
	for(c = c->GetSuper(); c; c = c->GetSuper())
	{
		if(c->GetNumMethods() <= findex)
			return false;

		if(c->GetMethod(findex) && c->GetMethod(findex)->GetName() == f->GetName())
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------
ScriptProfile::Item::Item(Funct* fn): m_pClass(NULL), m_pFunct(fn)
{
}
//-----------------------------------------------------------------------
ScriptProfile::Item::Item(Class* clss, Funct* fn): m_pClass(clss), m_pFunct(fn)
{
}
//-----------------------------------------------------------------------
void ScriptProfile::Clear()
{
	m_FunctionByTime.Clear();
	m_FunctionByCount.Clear();
	m_ClassByTime.Clear();
	m_Functions.Clear();
	m_Classes.Clear();
}

//-----------------------------------------------------------------------
ScriptProfile::ScriptProfile(): m_Functions(128), m_Classes(128), m_FunctionByTime(128), m_ClassByTime(128), m_FunctionByCount(128)
{
}

//-----------------------------------------------------------------------
void ScriptProfile::Sort()
{
	QuickSortCmp<TByTime, ReverseCompareT<TByTime> >(m_ClassByTime.GetCardinality(), m_ClassByTime.GetArray());
	QuickSortCmp<TByCount, ReverseCompareT<TByCount> >(m_FunctionByCount.GetCardinality(), m_FunctionByCount.GetArray());
	QuickSortCmp<TByTime, ReverseCompareT<TByTime> >(m_FunctionByTime.GetCardinality(), m_FunctionByTime.GetArray());
}

//-----------------------------------------------------------------------
void ScriptProfile::Dump()
{
	LogF(LL_NORMAL, "==================================================================================================");
	LogF(LL_NORMAL, "Begin profile script.\n");
	LogF(LL_NORMAL, "==================================================================================================\n");

	if(!m_ClassByTime.IsEmpty())
	{
		LogF(LL_NORMAL, "==================================================================================================");
		LogF(LL_NORMAL, "Profile sorted by class consumption");
		LogF(LL_NORMAL, "==================================================================================================\n");

		for(uint n = 0; n < m_ClassByTime.GetCardinality(); n++)
		{
			const Class* cl = m_Classes[m_ClassByTime[n].Data];
			LogF(LL_NORMAL,"%08d \t%s", (int)m_ClassByTime[n].Key, cl->GetName());
		}
	}

	if(!m_FunctionByCount.IsEmpty())
	{
		LogF(LL_NORMAL, "==================================================================================================");
		LogF(LL_NORMAL, "Profile sorted by call count");
		LogF(LL_NORMAL, "==================================================================================================\n");

		for(uint n = 0; n < m_FunctionByCount.GetCardinality(); n++)
		{
			const ScriptProfile::Item& item = m_Functions[m_FunctionByCount[n].Data];

			if(item.m_pClass)
				LogF(LL_NORMAL,"%s.%s \t\t%d", item.m_pClass->GetName(), item.m_pFunct->GetName(), item.m_pFunct->Profile);
			else
				LogF(LL_NORMAL,"%s \t\t%d", item.m_pFunct->GetName(), item.m_pFunct->Profile);
		}

		LogF(LL_NORMAL, "\n\n==================================================================================================");
		LogF(LL_NORMAL, "Profile sorted by time in call\n");
		LogF(LL_NORMAL, "==================================================================================================\n");

		for(uint n = 0; n < m_FunctionByTime.GetCardinality(); n++)
		{
			const ScriptProfile::Item& item = m_Functions[m_FunctionByTime[n].Data];
			if(item.m_pClass)
				LogF(LL_NORMAL,"%s.%s \t\t%d", item.m_pClass->GetName(), item.m_pFunct->GetName(), item.m_pFunct->Profile);
			else
				LogF(LL_NORMAL,"%s \t\t%d", item.m_pFunct->GetName(), item.m_pFunct->Profile);
		}
/*
		LogF(LL_NORMAL, "\n\n==================================================================================================");
		LogF(LL_NORMAL, "Profile sorted by time per call\n");
		LogF(LL_NORMAL, "==================================================================================================\n");
		QuickSortCmp<ProfFunc, CompareProf3>(fn.GetCardinality(), (ProfFunc*)fn.GetArray());
		for(uint n = 0; n < fn.GetCardinality(); n++)
		{
			if(fn[n].c)
				LogF(LL_NORMAL,"%s.%s \t\t%d", fn[n].c->GetName(), fn[n].f->GetName(), fn[n].f->Time / fn[n].f->Profile);
			else
				LogF(LL_NORMAL,"%s \t\t%d", fn[n].f->GetName(), fn[n].f->Time / fn[n].f->Profile);
		}
*/
	}
	LogF(LL_NORMAL, "==================================================================================================");
	LogF(LL_NORMAL, "End profile script.");
	LogF(LL_NORMAL, "==================================================================================================\n");
}

//-----------------------------------------------------------------------
void ScriptProfile::AddClass(const Class* cl)
{
	uint index = m_Classes.Insert(cl);
	m_ClassByTime.Insert(MakePair(cl->Time, index));
}

//-----------------------------------------------------------------------
void ScriptProfile::AddFunction(const ScriptProfile::Item& item)
{
	uint index = m_Functions.Insert(item);
	m_FunctionByTime.Insert(MakePair(item.m_pFunct->Time, index));
	m_FunctionByCount.Insert(MakePair(item.m_pFunct->Profile, index));
}
#endif

//-----------------------------------------------------------------------
void ScriptModule::ResetProfile(uint flags)
{
#ifdef ENF_DEVELOPER
	for(uint n = 0; n < m_Functions.GetCardinality(); n++)
	{
		m_Functions.GetElement(n)->Time = 0;
		m_Functions.GetElement(n)->Profile = 0;
	}

	for(uint n = 0; n < m_VarTypes.GetCardinality(); n++)
	{
		Class *cl = (Class*)m_VarTypes.GetElement(n);

		if(cl->Type == VARTYPE_CLASS)
		{
			cl->Time = 0;
			for(uint f = 0; f < cl->GetNumMethods(); f++)
			{
				if(cl->GetMethod(f))
				{
					cl->GetMethod(f)->Profile = 0;
					cl->GetMethod(f)->Time = 0;
				}
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------
void ScriptModule::Profile(ScriptProfile& prof, uint flags)
{
#ifdef ENF_DEVELOPER
	for(uint n = 0; n < m_Functions.GetCardinality(); n++)
	{
		if(m_Functions.GetElement(n)->Profile && m_Functions.GetElement(n)->Time)
		{
			prof.AddFunction(ScriptProfile::Item(m_Functions.GetElement(n)));
			if(flags & SPF_RESET)
			{
				m_Functions.GetElement(n)->Profile = 0;
				m_Functions.GetElement(n)->Time = 0;
			}
		}
	}

	for(uint n = 0; n < m_VarTypes.GetCardinality(); n++)
	{
		Class *cl = (Class*)m_VarTypes.GetElement(n);

		if(cl->Type != VARTYPE_CLASS)
			continue;

		if(!cl->Time)
			continue;

		prof.AddClass(cl);
		if(flags & SPF_RESET)
			cl->Time = 0;

		for(uint f = 0; f < cl->GetNumMethods(); f++)
		{
			if(cl->GetMethod(f) && cl->GetMethod(f)->Profile && cl->GetMethod(f)->Time)
			{
				if(!FromSuper(cl, cl->GetMethod(f), f))
				{
					prof.AddFunction(ScriptProfile::Item(cl, cl->GetMethod(f)));
					if(flags & SPF_RESET)
					{
						cl->GetMethod(f)->Profile = 0;
						cl->GetMethod(f)->Time = 0;
					}
				}
			}
		}
	}

	if(flags & SPF_RECURSIVE)
	{
		for(ScriptModule* m = m_pChildModule; m; m = m->m_pSiblingModule)
			m->Profile(prof, flags);
	}
#endif
}

}