#include <El/elementpch.hpp>
#include <Es/Strings/bString.hpp>
#include "dynEnum.hpp"

DynEnumCS::DynEnumCS()
{
}

DynEnumCS::~DynEnumCS()
{
}

void DynEnumCS::Clear()
{
	_values.Clear();
	_names.Clear();
}

void DynEnumCS::Close()
{
	_names.Add(EnumName());
}

int DynEnumCS::GetValue( const char *name ) const
{
	const DynEnumValue &value = _values[name];
	if (_values.NotNull(value)) return value.GetValue();
	return -1;
}

int DynEnumCS::FindValueString(const char *name) const
{
	const DynEnumValue &check = _values[name];
	if (_values.NotNull(check)) return check.GetValue();
	return -1;
}

int DynEnumCS::AddValueString(const RStringB &name)
{
	int value = _names.Size();
	DynEnumValue newValue(value,name);
	_values.Add(newValue);
	_names.Add(newValue);
	return value;
}

int DynEnumCS::AddValue(const RStringB &name)
{
	if (_values.NItems()!=_names.Size())
	{
		Fail("DynEnum has already been closed, cannot add more items");
	}
	const DynEnumValue &check = _values[name];
	if (_values.NotNull(check))
	{
		LogF("DynEnum: value %s added twice",(const char *)name);
		return check.GetValue();
	}
	return AddValueString(name);
}

#if _DEBUG
struct GetNameContext
{
	int value;
	RStringB name;
};
static void GetNameFind
(
	const DynEnumValue &value, const DynEnumBankType *bank, void *context
)
{
	GetNameContext *tContext = static_cast<GetNameContext *>(context);
	if (value.GetValue()==tContext->value)
	{
		tContext->name = value.GetName();
	}
}
#endif

/*!
\patch_internal 1.23 Date 09/17/2001 by Ondra
- Optimized: DynEnum::GetName used search that is not neccessary.
_maxValue member contained same information as _names.Size()
*/

const RStringB &DynEnumCS::GetName( int value ) const
{
	#if _DEBUG
		GetNameContext context;
		context.value = value;
		_values.ForEach(GetNameFind,&context);
		Assert (_names[value].name==context.name);
	#endif
	return _names[value].name;
}

int DynEnum::GetValue( const char *name ) const
{
	BString<256> lowName;
  Assert(strlen(name)<sizeof(lowName));
	strcpy(lowName,name);
	strlwr(lowName);
	return FindValueString(lowName);
}

int DynEnum::AddValue( const char *name, bool duplicityWarning )
{
	BString<256> lowName;
  Assert(strlen(name)<sizeof(lowName));
	strcpy(lowName,name);
	strlwr(lowName);
	int index = FindValueString(lowName);
	if (index>=0)
	{
		if (duplicityWarning) LogF("DynEnum: value %s added twice",name);
		return index;
	}
	return AddValueString(lowName.cstr());
}

