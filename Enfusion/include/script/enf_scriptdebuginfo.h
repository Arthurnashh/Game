//-----------------------------------------------------------------------------
// File: enf_scriptdebuginfo.h
//
// Desc: 
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SCRIPTDEBUGINFO_H
#define ENF_SCRIPTDEBUGINFO_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_strings.h"

namespace enf
{
/** @addtogroup EnforceScript
@{*/

//-----------------------------------------------------------------------
// Debugger info
//-----------------------------------------------------------------------
class PUBLIC_API DebugInfo
{
friend class ScriptDebugger;

	uint			m_iLastModule;

public:
	ENF_DECLARE_SMALLOBJECT(DebugInfo);

	struct Module
	{
		DString	m_strName;
	};

	struct Line
	{
		uint		m_iOffset;
		ushort	m_iSourceLine;
		ushort	m_iModule;
		bool operator==(const Line& other) const { return m_iOffset == other.m_iOffset; }
		bool operator<(const Line& other) const { return m_iOffset < other.m_iOffset; }
		bool operator>(const Line& other) const { return m_iOffset > other.m_iOffset; }
		Line() {}
		Line(uint offset): m_iOffset(offset) {}
	};

	Array<Module>	m_Modules;
	Array<Line>	m_Lines;

	~DebugInfo() {}
	DebugInfo(): m_iLastModule(-1) {}

	uint AddDebugLine(const char* mod, uint line, uint offset);
	uint GetLineByOffset(uint offset) const;
	uint GetExactLineByOffset(uint offset) const;
	bool GetLineInfoByOffset(uint offset, const char** module, uint* srcline) const;
	bool GetLineInfo(uint line, const char** module, uint* srcline) const;
};
//@}
}

#endif //ENF_SCRIPTDEBUGINFO_H