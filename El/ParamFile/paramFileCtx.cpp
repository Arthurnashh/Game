#include <El/elementpch.hpp>
#include <Es/Framework/appFrame.hpp>
#include "paramFileCtx.hpp"

int ParamEntryWithContext::GetEntryCount() const
{
	// count entries that are visible in given context
	int c = _entry.GetEntryCount();
	int count = 0;
	for (int i=0; i<c; i++)
	{
		ParamEntryVal e = _entry.GetEntry(i);
		if (e.IsClass())
		{
			// check if entry is visible
			if (!_visible(_entry,e)) continue;
		}
		count++;
	}
	return count++;
}

class ParamEntryCtxError: public ParamClass
{
	public:
	ParamEntryCtxError(){}

	virtual bool IsError() const {return true;}
};

//! global instance of value "error"
static ParamEntryCtxError GParamEntryCtxError;

const ParamEntry &ParamEntryWithContext::GetEntry( int index ) const
{
	// note: can be slow
	// some enumerator interface should be used instead
	int c = _entry.GetEntryCount();
	for (int i=0; i<c; i++)
	{
		ParamEntryVal e = _entry.GetEntry(i);
		if (!e.IsClass())
		{
			// check if entry is visible
			if (!_visible(_entry,e)) continue;
		}
		if (--index<0) return e;
	}
	Fail("Entry not found");
	return GParamEntryCtxError;

}

ParamEntryPtr ParamEntryWithContext::FindEntryNoInheritance
(
	const char *name
) const
{
	return _entry.FindEntryNoInheritance(name,_visible);
}

ParamEntryPtr ParamEntryWithContext::FindEntry
(
	const char *name
) const
{
	return _entry.FindEntry(name,_visible);
}

ParamEntryWithContext ParamEntryWithContext::operator >> (const RStringB &name) const
{
	ConstParamEntryPtr entry=FindEntry(name);
	if( entry ) return ParamEntryWithContext(*entry,_visible);
	ErrorMessage(EMError,"Cannot acces entry '%s'.",(const char *)GetContext(name));
	return ParamEntryWithContext(GParamEntryCtxError,_visible);
}


bool ParamOwnerList::operator () (const ParamEntry &entry)
{
	RStringB entryOwner = entry.GetOwnerName();
	if (entry.GetOwnerName().GetLength()<=0) return true;
	// check if owner is in the list of known owners
	for (int i=0; i<_addons.Size(); i++)
	{
		if (_addons[i]==entryOwner) return true;
	}
	LogF
	(
		"Access denied: %s (owner %s)",
		(const char *)entry.GetContext(),(const char *)entryOwner
	);
	return false;
}

bool ParamOwnerList::operator () (const ParamEntry &parent, const ParamEntry &entry)
{
	RStringB parentOwner = parent.GetOwnerName();
	RStringB entryOwner = entry.GetOwnerName();
	if (parentOwner==entryOwner) return true;
	return (*this)(entry);
}
