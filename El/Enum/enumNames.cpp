#include <El/elementpch.hpp>
#include "enumNames.hpp"

/*!
\file
Implementation of EnumName functions
*/

extern int DUPLICATE_IDS = INT_MAX;

DEF_RSB_EXT(ERROR)

int GetEnumValue( const EnumName *names, const RStringB &name )
{
	for( int i=0; names[i].IsValid(); i++ )
	{
		if( !strcmpi(names[i].name,name) ) return names[i].value;
		//if( names[i].name==name ) return names[i].value;
	}
	return INT_MIN;
}

int GetEnumValue( const EnumName *names, const char *name )
{
	for( int i=0; names[i].IsValid(); i++ )
	{
		if( !strcmpi(names[i].name,name) ) return names[i].value;
	}
	return INT_MIN;
}

static RStringB EmptyString;

const RStringB &GetEnumName( const EnumName *names, int value )
{
	for( int i=0; names[i].IsValid(); i++ )
	{
		if( names[i].value==value ) return names[i].name;
	}
	return EmptyString;
}


