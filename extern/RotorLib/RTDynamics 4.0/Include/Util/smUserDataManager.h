/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include "vector"

/**
*	smUserDataManager provides an interface to a class
*	to add and get any number of user data for an instance.
*
*	Data is stored as pointers. The templated architecture removes the
*	requirement for typecasting void* to storage types.
*
*	Use with multiple inheritance with your classes.
*	e.g
*		class MyClass: public Drawable, public smUserDataManager<SomeOtherClass>
*
*	In this case a std::vector<SomeOtherClass> is created.
*
*	Then you can add and get user data for MyClass.
*
*/
template<class ObjType>
class smUserDataManager
{
public:
	smUserDataManager( size_t size = 5 ) : m_userDataVec( size, NULL )
	{}
 
	// @depracted due to bad naming, use setUserData instead.
    void addUserData( ObjType* data, int slot = 0)
    {
        setUserData( data, slot );
    }

    void setUserData( ObjType* data, int slot = 0 )
    {
		m_userDataVec.at(slot)	=	data ;		
    }

    ObjType* getUserData( int slot = 0 )
    {
        return m_userDataVec[ slot ] ;
    }
protected:
    std::vector<ObjType*>	m_userDataVec ;
};