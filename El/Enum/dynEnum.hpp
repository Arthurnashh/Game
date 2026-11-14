#ifdef _MSC_VER
#pragma once
#endif

#ifndef _DYN_ENUM_HPP
#define _DYN_ENUM_HPP

#include <El/Enum/enumNames.hpp>

typedef EnumName DynEnumValue;

typedef MapStringToClass<DynEnumValue,AutoArray<DynEnumValue> > DynEnumBankType;

//! case sensitive dynamic enum.

class DynEnumCS
{
	//! quick searching by name
	DynEnumBankType _values;
	//! quick searching by value
	AutoArray<EnumName> _names;

	protected:
	int FindValueString(const char *name) const;
	int AddValueString(const RStringB &name);

	public:
	DynEnumCS();
	~DynEnumCS();

	//! clear enum
	void Clear();
	//! all values added - close enum
	void Close();

	//! add value - case sensitive
	int AddValue(const RStringB &name);
	//! GetValue - case sensitive
	int GetValue(const char *name) const;

	//! get name attached to value
	const RStringB &GetName( int value ) const;
	//! get number of items
	int FirstInvalidValue() const {return _values.NItems();}
	//! get list of names (for serialization templates)
	const EnumName *GetEnumNames() const {return _names.Data();}
};

//! case insensitive dynamic enum.

class DynEnum: public DynEnumCS
{
	public:
	// replace case sensitive functions with insensitive variants
	//! GetValue - case insensitive
	int AddValue( const char *name, bool duplicityWarning = true );
	//! GetValue - case insensitive
	int GetValue( const char *name ) const;
};

#endif
