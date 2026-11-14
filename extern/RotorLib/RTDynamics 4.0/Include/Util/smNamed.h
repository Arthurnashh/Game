/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include <string>

/**
*	Class for managed a class with a name.
*	Add this class to list of parent classes
*/
class smNamed
{
public:
	explicit smNamed() : m_isExplicitlyNamed( false )
    {
        m_name	=	"Unnamed" ;
    }

    explicit smNamed( std::string name ) : m_isExplicitlyNamed( false )
    {
        m_name	=	name ;
    }

    virtual ~smNamed() {};

    virtual void setName( const std::string& name )
    {
        m_name	=	name ;
		m_isExplicitlyNamed = true ;
    }
	
    virtual const std::string& getName() const
    {
        return m_name ;
    }

    virtual const char* getNameAsCStr() const
    {
        return m_name.c_str() ;
    }

    //
    // Returns true if the string given is same to the name.
    //
    virtual bool isName( const std::string string )
    {
        return ( m_name.compare( string ) == 0 ) ? true : false ;
    }

	/** Returns true if a name is given to this class via setName	*/
	bool	isNameGiven()
	{ return m_isExplicitlyNamed; }

private:

    /**
    *	Keep m_name under private for better prevention against bugs
    */
    std::string m_name ;

	bool	m_isExplicitlyNamed ;
};
