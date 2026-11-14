/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once


#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "Util/smPrinter.h"

/**
*	This class tries to mimick QString class in Qt.
*
*/
class smString
{
public:
	enum Base
	{
		Octal,
		Decimal,
		Hex,
		None
	};

//	/**
//	*
//	*/
//	template <class TYPE>
//	static std::string number( const TYPE &t, Base base = Decimal )
//	{
//		std::ostringstream os;
//		setNumericBase( os, base ) ;
//		os << t ;
//		return os.str();
//	}

	/**
	*	See number().
	*/
	template <class TYPE>
	static std::string number( const TYPE t, Base base = Decimal )
	{
		std::ostringstream os;
		setNumericBase( os, base ) ;
		os << t ;
		return os.str();
	}

	/**
	*
	*/
	template <class TYPE>
	static std::string number( const TYPE t, int width, int precision, Base base = Decimal )
	{
		std::ostringstream os;
		setNumericBase( os, base ) ;
		os.width( width );
		os.precision( precision ) ;
		os << t ;
		return os.str();
	}

private:
	static void setNumericBase(std::ostringstream& os, Base base )
	{
		//
		//	os.flags() is the only method working. Others just don't work or
		//	compile.
		//
		switch( base )
		{
		case Octal	: os.flags( std::ios::oct ) ; break;
		case Decimal: os.flags( std::ios::dec ) ; break;
		case Hex	: os.flags( std::ios::hex | std::ios::showbase ); break;
		}
	}
};

class smStringList
{
private:
	std::vector<std::string> mPaths ;

public:
	smStringList( const std::string& inPath )
	{
		std::string path = inPath ;

		//
		//  To be able to write the algorithm below in a generic way
		//  We must have : at the beginning.
		//
		if ( path[0] != ':' )
			path = ":" + path ;
		// And no : at the end
		if ( path[path.size()-1] != ':' )
			path = path + ":" ;

		// Decompose path
		std::vector<size_t> sepIndices ;
		size_t sepIdx = path.find( ":" );
		while( sepIdx != std::string::npos )
		{
			sepIndices.push_back( sepIdx );
			sepIdx = path.find( ":", sepIdx+1 );
		}

		std::vector<size_t>::iterator idx = sepIndices.begin() ;
		for ( ; idx != (sepIndices.end()-1) ; idx++ )
		{
			std::string substr = path.substr( *(idx)+1, *(idx+1) - *idx - 1 );
			if ( substr.size() != 0 )
			{
				mPaths.push_back( substr );
			}
		}
	}

	std::string toString()
	{
		std::string fullStr ;
		std::vector<std::string>::iterator i = mPaths.begin() ;
		for ( ; i != mPaths.end() ; i++, fullStr = fullStr + ":"  )
		{
			fullStr += *i ;
		}

		return fullStr ;
	}

	void remove( const std::string& remStr )
	{
		// Check if the given path is in the list
		std::vector<std::string>::iterator i = std::find( mPaths.begin(), mPaths.end(), remStr ) ;
		while ( i != mPaths.end() )
		{
			mPaths.erase( i );

			i = std::find( mPaths.begin(), mPaths.end(), remStr ) ;
		}
	}

	void print( smPrinter* pr )
	{
		std::vector<std::string>::iterator i = mPaths.begin() ;
		for( ; i != mPaths.end() ; i++ )
			pr->always( "Str : %s", (*i).c_str() );
	}
};

