/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include "Util/smManager.h"

/*
* This class is derived from smManager allows calling certain methods of the
* ObjType class. All ObjType class must define the update,init,exit and getName
* methods if they are exported out of a DLL.
*
* @Depreceated, please don't use any more.
*
**/
template <typename ObjType>
class smModelManager : public smManager<ObjType>
{
        public:
            /**
            *	initiates an update call for all models managed by this class.
            */
            void	update();
            void	update( double time );

            /**
            *	calls init for all classes
            */
            void	init();
	
            ObjType*	get( size_t idx ) const ;

	    size_t 	getNum() const ;

            /**
            *	calls exit for all classes
            */
            void	exit();

            /**
            *	Finds the objects by name. If not found, NULL
            *	is returned.
            */
            ObjType*	findByName( std::string name ) ;

            /**
            *
            *
            */
			// printAll prints to stderr since we don't want to include smPrinter.h
			// here. We need a .ii work to be done to be able to use smPrinter
			void		printAll( );
			void		printAddresses( );
};

template <typename ObjType>
ObjType*
smModelManager<ObjType>::findByName( std::string name )
{
	typename smManager<ObjType>::ObjectsVectorIt i = smManager<ObjType>::m_objects.begin();
	for( ; i != smManager<ObjType>::m_objects.end() ; i++ )
	{
		if ( (*i)->getName().compare( name ) == 0 )
			return *i ;
	}

	// When no items are found in the m_objects
	return NULL ;
}


template <typename ObjType>
inline size_t
smModelManager<ObjType>::getNum() const
{
    return smManager<ObjType>::m_objects.size();
}

template <typename ObjType>
void
smModelManager<ObjType>::update()
{
    for ( size_t i = 0 ; i < getNum() ; i++ )
    {
        get(i)->update();
    }
}


template <typename ObjType>
ObjType*
smModelManager<ObjType>::get(size_t idx) const
{
    if ( static_cast<unsigned int>(idx) >= smManager<ObjType>::m_objects.size() ) {
        return NULL ;
    }else
        return smManager<ObjType>::m_objects[idx];
}

template <typename ObjType>
void
smModelManager<ObjType>::update( double time )
{
    for ( size_t i = 0 ; i < getNum() ; i++ )
    {
        get(i)->update( time );
    }
}

template <typename ObjType>
void
smModelManager<ObjType>::init()
{
    for ( size_t i = 0 ; i < getNum() ; i++ )
    {
        get(i)->init();
    }
}

template <typename ObjType>
void
smModelManager<ObjType>::exit()
{
    for ( size_t i = 0 ; i < getNum() ; i++ )
    {
        get(i)->exit();
    }
}

template <typename ObjType>
void
smModelManager<ObjType>::printAll( )
{
	// fprintf has problem with Visual c++ .net so I switched to std::cout
	std::cout << "smModelManager Data, num data:" << smManager<ObjType>::m_objects.size()  << std::endl ;

    for ( size_t i = 0 ; i < getNum() ; i++ )
    {
		std::cout << "Id:" << i  ;
		std::cout << " Ptr:" << std::hex << get(i) ;
		std::cout << " Name:" << get(i)->getName() ;
		std::cout << " Type info" << typeid( get(i) ).name() ;
		std::cout << std::endl ;
    }
}

template <typename ObjType>
void
smModelManager<ObjType>::printAddresses( )
{
	// fprintf has problem with Visual c++ .net so I switched to std::cout
	std::cout << "smModelManager Data, num data:" << smManager<ObjType>::m_objects.size()  << std::endl ;

	for ( size_t i = 0 ; i < getNum() ; i++ )
	{
		std::cout << "Id:" << i  ;
		std::cout << " Ptr:" << std::hex << get(i) ;
		std::cout << " Type info" << typeid( get(i) ).name() ;
		std::cout << std::endl ;
	}
}
