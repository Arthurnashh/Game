//-----------------------------------------------------------------------------
// File: enf_scriptprofile.h
//
// Desc: Script profiler
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SCRIPTPROFILE_H
#define ENF_SCRIPTPROFILE_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "script/enf_scripttype.h"

namespace enf
{
/** @addtogroup EnforceScript
@{*/

class Class;
class Funct;

enum ProfilerFlags
{
	SPF_RESET			= BITF(1),
	SPF_RECURSIVE	= BITF(2)
};

#ifdef ENF_DEVELOPER

//-----------------------------------------------------------------------
class ScriptProfile
{
public:
	void Dump();

	struct Item
	{
		Class*		m_pClass;
		Funct*		m_pFunct;

		Item(): m_pClass(NULL), m_pFunct(NULL) {}
		Item(Funct* fn);
		Item(Class* clss, Funct* fn);
	};

	ScriptProfile();
	void AddFunction(const Item& item);
	void AddClass(const Class* cl);
	void Clear();
	void Sort();

	typedef Pair<TimeStamp, uint> TByTime;
	typedef Pair<uint, uint> TByCount;

	Array<TByTime> m_FunctionByTime;
	Array<TByCount> m_FunctionByCount;
	Array<TByTime> m_ClassByTime;
	Array<Item>	m_Functions;
	Array<const Class*>	m_Classes;
};
#endif
	/**
	@}*/

}

#endif
