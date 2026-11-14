#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PARAM_FILE_CTX_HPP
#define _PARAM_FILE_CTX_HPP

#include <El/ParamFile/paramFile.hpp>


//! replacement of ParamEntry that can be used to perform search with context
/*!
All read functions from ParamEntry interface are simulated.
Most function can be simply passed to corresponding ParamEntry.
Some functions must receive context as it may influence them.
*/
class ParamEntryWithContext
{
	IParamVisibleTest &_visible;
	const ParamEntry &_entry;

	public:
	ParamEntryWithContext(const ParamEntry &entry, IParamVisibleTest &visible)
	:_entry(entry),_visible(visible)
	{
	}

	//@{
	/*!
	\name Functions passed to ParamEntry
	// functions in this group are simply passed to ParamEntry
	// check corresponding ParamEntry function documentation
	*/
	const RStringB &GetName() const {return _entry.GetName();}
	RString GetContext( const char *member=NULL ) const
	{
		return _entry.GetContext();
	}
	const ParamEntry &GetEntry() const {return _entry;}

	bool IsClass() const {return _entry.IsClass();}
	bool IsArray() const {return _entry.IsArray();}
	bool IsError() const {return _entry.IsError();}

	RStringB GetOwnerName() const {return _entry.GetOwnerName();}
	IParamClassSource *GetSource() const {return _entry.GetSource();}

	// value conversions
	operator RStringB() const {return _entry.operator RStringB();}
	operator float() const {return (float)_entry;}
	operator int() const {return (int)_entry;}
	operator bool() const {return (bool)_entry;}
	int GetInt() const {_entry.GetInt();}
	RStringB GetValueRaw() const {_entry.GetValueRaw();}
	const IParamArrayValue &operator [] ( int index ) const {return _entry[index];}
	int GetSize() const {return _entry.GetSize();}
	//@}

	/*!
	\name Context sensitive functions
	// functions in this group must receive context as it may influence them.
	*/
	//@{
	int GetEntryCount() const;
	const ParamEntry &GetEntry( int index ) const;
	ParamEntryWithContext operator >> (const RStringB &name) const;
	ParamEntryPtr FindEntryNoInheritance(const char *name) const;
	ParamEntryPtr FindEntry(const char *name) const;
	//@}

};

#endif
