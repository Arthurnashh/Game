#include <El/elementpch.hpp>
#include "idString.hpp"

/*!
\file
Implementation of class IdString
*/



IdStringTable::IdStringTable()
{
}

/*!
Example:
RStringB MPStrings[]=
{
	"aaa",
	"bbb"
};

IdStringTable MPIdStrings(MPStrings,sizeof(MPStrings)/sizeof(*MPStrings));
*/
IdStringTable::IdStringTable(const RStringB *strings, int count)
{
	for (int i=0; i<count; i++)
	{
		_enum.AddValue(strings[i]);
	}
	_enum.Close();
}

IdStringTable::~IdStringTable()
{
}

int IdStringTable::GetId(const char *string)
{
	return _enum.GetValue(string);
}

const RStringB &IdStringTable::GetString(int id) const
{
	return _enum.GetName(id);
}

IdString IdStringTable::GetIdString(const char *string)
{
	IdString id;
	id._id = _enum.GetValue(string);
	id._string = string;
	return id;
}

