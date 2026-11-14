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
#include <exception>

class smException : public std::exception
{
public:
    smException( std::string errorDescr )
    {
        m_errorDesc	=	errorDescr ;
    }
    smException( )
    {
        m_errorDesc	=	"No message specified" ;
    }

    //
    //  A destructor with a throw specs is needed.
    //
    virtual ~smException() throw () {} ;

    const char *what() const throw()
    {
        return m_errorDesc.c_str () ;
    }

    /**
    *	Each derived class shall return its name
    *	with this method.
    */
    virtual std::string	getExceptionName()	=	0	;

protected:
    std::string	m_errorDesc ;
};

/** A general purpose exception to be throw when a model encounters a singularity.*/
class smExpSingularity : public smException
{
public:
	smExpSingularity( const std::string& errorDesc ) : smException( errorDesc )
	{}

	virtual std::string	getExceptionName()
	{
		return "Singularity Exception. Error description:" + m_errorDesc ;
	}
} ;

/** A general purpose exception to be throw when a model fails to configure correctly (e.g. when file read fails.).*/
class smExpConfiguration : public smException
{
public:
	smExpConfiguration( const std::string& errorDesc ) : smException( errorDesc )
	{}

	virtual std::string	getExceptionName()
	{
		return "Configuration Exception. Error description:" + m_errorDesc ;
	}
} ;
