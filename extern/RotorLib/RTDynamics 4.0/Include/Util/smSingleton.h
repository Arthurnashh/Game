/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include "Util/smMacros.h"
#include "Util/smUtil.h"

/**
* Template'd singleton implementation.
*/
template <class ObjType>
class smSingleton{
	private:
		/**
		*	The c'tor and d'tor shall never
		*	be called. When called, they will
		*	assert.
		*/
		explicit smSingleton() ;
		virtual	~smSingleton() ;

	public:
		/**
		*	instance creates a new singleton ObjType if required
		*	and return a pointer to the object.
		*
		*	shutdown will delete the static instance of the ObjType.
		*/
		static ObjType *	instance ( void ) ;
		static void			shutdown( void ) ;

	protected:
		static ObjType *mInstance ;
} ;

template<class ObjType>
smSingleton<ObjType>::smSingleton()
{
	smValidateAndExit(false,"smSingleton<ObjType>::smSingleton()","");
}

template<class ObjType>
smSingleton<ObjType>::~smSingleton()
{
	smValidateAndExit(false,"smSingleton<ObjType>::~smSingleton()","");
}

template<class ObjType>
ObjType *
smSingleton<ObjType>::instance ( void )
{
	if ( mInstance == 0 ) {
		mInstance = new ObjType ;
	}

	return mInstance ;
}

template<class ObjType>
void
smSingleton<ObjType>::shutdown( void )
{
	smDelete( mInstance );
}

template <class ObjType>
ObjType* smSingleton<ObjType>::mInstance = 0 ;

