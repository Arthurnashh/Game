//-----------------------------------------------------------------------------
// File: enf_scriptdebuginfo.cpp
//
// Desc: Interface to remote script debugger
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"

#include "common/enf_set.h"
#include "script/enf_scriptdebuginfo.h"

namespace enf
{

//-----------------------------------------------------------------------
uint DebugInfo::GetLineByOffset(uint offset) const
{
	uint n = BinarySearchGE<Line>(m_Lines.GetArray(), m_Lines.GetCardinality(), Line(offset));

	if(n == -1)
		n = m_Lines.GetCardinality() - 1;

	return n;
}
//-----------------------------------------------------------------------
uint DebugInfo::GetExactLineByOffset(uint offset) const
{
	return BinarySearchEQ<Line>(m_Lines.GetArray(), m_Lines.GetCardinality(), Line(offset));
}

//-----------------------------------------------------------------------
bool DebugInfo::GetLineInfoByOffset(uint offset, const char** module, uint* srcline) const
{
	uint n = GetLineByOffset(offset);

	if(module)
		*module = m_Modules[m_Lines[n].m_iModule].m_strName.cstr();

	if(srcline)
		*srcline = m_Lines[n].m_iSourceLine;

	return true;
}

//-----------------------------------------------------------------------
bool DebugInfo::GetLineInfo(uint line, const char** module, uint* srcline) const
{
	if(line >= m_Lines.GetCardinality())
		return false;

	const Line& pline = m_Lines[line];

	if(module)
		*module = m_Modules[pline.m_iModule].m_strName.cstr();

	if(srcline)
		*srcline = pline.m_iSourceLine;

	return true;
}

}