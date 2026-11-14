//#include "precomp.h"
#include "common/enf_defineset.h"
#include "common/enf_staticbuffer.h"


namespace enf
{
//-----------------------------------------------------------------------
Define::Define(CStr name, CStr def):
	Name(name.Static()),
	Value(def.Static())
{
}

//-----------------------------------------------------------------------
Define::Define(CStr name, int value):
	Name(name.Static()),
	Value(Num2Str(value))
{
}

//-----------------------------------------------------------------------
const char* Num2Str(uint v)
{
	SString<> str;
	return g_pStatic->AddString(str.format("%d", v));
}

//-----------------------------------------------------------------------
Define::Define(CStr name):
	Name(name.Static()),
	Value(NULL)
{
}

//-----------------------------------------------------------------------
const char* DefineSet::Find(CStr define) const
{
	Define def;

	def.Name = define;

	uint index = GetIndexOf(def);
	if(index != INDEX_NOT_FOUND)
	{
		if(!GetElement(index).Value.IsEmpty())
			return GetElement(index).Value.cstr();
		else
			return GetElement(index).Name.cstr();
	}
	return NULL;
}

//-----------------------------------------------------------------------
void DefineSet::Append(const char* const* defines)
{
	enf_assert(defines != NULL);

	while(*defines)
	{
		const char* str = *defines;

		const char* eq = strchr(str, '=');

		Define def;

		if(eq)
		{
			def.Name = g_pStatic->AddString(str, eq - str);
			def.Value = g_pStatic->AddString(eq + 1);
		}
		else
		{
			def.Name = g_pStatic->AddString(str);
		}

		Insert(def);
		defines++;
	}
}


}
