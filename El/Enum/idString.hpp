#ifdef _MSC_VER
#pragma once
#endif

/*!
\file
Interface of class IdString
*/

#ifndef _ID_STRING_HPP
#define _ID_STRING_HPP

#include <Es/Strings/rString.hpp>
#include "dynEnum.hpp"

//! string with ID
/*!
Purpose of this class is compression of string used in network messages.
For strings found in table only table index is transmitted, for other full text.
*/

class IdStringTable;

class IdString
{
	friend class IdStringTable;

	//! if there is id, use it, if not (id<0), use full string
	int _id;
	//! string is always valid (even for id>=0)
	RStringB _string;

	public:
	IdString(){}
	~IdString(){}

	__forceinline int GetID() const {return _id;}
	__forceinline const RStringB &GetValue() const {return _string;}

};

//! case sensitive table
class IdStringTable
{
	DynEnumCS _enum;

	public:
	IdStringTable();
	~IdStringTable();
	//! construct id table from string list
	IdStringTable(const RStringB *strings, int count);

	IdString GetIdString(const char *string);
	int GetId(const char *string);
	const RStringB &GetString(int id) const;

  int GetStringsCount() const {return _enum.FirstInvalidValue();}
};

#endif
