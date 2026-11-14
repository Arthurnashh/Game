/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once


#include <iostream>
#include <string>
#include <vector>
// Inclusion of typeinfo is needed for gcc 2.95.3
#include <typeinfo>
#include "Util/smMacros.h"

/*
* Manager class serves as a template class for managables.
* Derived classes can manage objects of type "ObjType" (template class).
* Simple vector storage operations are implemented.
*
**/
template <typename ObjType>
class smManager
{
        public:
            /**
            *	If deleteMembersAtExit is true, members will be deleted
            *	at exit ( in destructor method ) .
            */
            explicit	smManager	  ( void );
            virtual 	~smManager    ( );

        public:
            typedef typename std::vector<ObjType*> 				ObjectsVector;
            typedef typename std::vector<ObjType*>::iterator  	ObjectsVectorIt;

		protected:
            ObjectsVector                                    m_objects;

        public:
            /** add an object to this manager.		*/
			void	add(ObjType* obj, bool allowDuplicate = true );

			/**	add another smManager to this one.	*/
			void	addManager(smManager& other, bool allowDuplicate = true );

			/** prepends object to the vector. It is named slowly to make sure that
			*	users notices that it is a slow function.
			*/
            void	prependSlowly(ObjType* obj, bool allowDuplicate = true );

            /** removes the pointer from list.  */
            void	remove			( ObjType* obj ) ;
            void	remove			( int idx ) ;

			/** returns the object at the given position */
			ObjType*	get( size_t idx ) const ;

			/** returns the index of the given object. Return -1 if obj not
			*	found.
			*/
			int			findIdxOf( ObjType* idx ) const ;

			/** returns the object at the given position. */
			// Note: all [] operators must match the canonical type of the original
			// operator, which says: object &operator [] (int x)'
			ObjType*	operator[]  (  int idx ) ;

            /**
            *	Prefer using plain find to findIterOf since
            *	when it findIterOf returns end, de-ferencing causes
            *	crash.
            */
            ObjectsVectorIt		findIterOf	( ObjType* obj ) ;

			/** find returns the object if it is found, otherwise NULL. */
            ObjType*            find        ( ObjType* obj ) ;

            /**
            *
            */
            size_t 	getNum() const ;

			/** Return the STL vector object which this smManager works on. */
			const ObjectsVector&	getVector() const	
			{ return m_objects; }

			/** Return the STL vector object which this smManager works on. */
			ObjectsVector&	getVector() 
			{ return m_objects; }

            /**
            *
            */
            void	removeAll();

            /** */
			std::string	toString(const std::string& separator="/") const;
};

/**
*	Use this method to clean the data stored in an smManager
*
*	It wasn't possible to put this code in smManager d'tor. MSVC++
*	doesn't like template object delete in template classes.
*
*	It wasn't possible to make it a static method in smManager, since
*	MSVC++ desn't like static methods in template classes.
*
*	So we have a global function.
*
**/
template <typename ObjType>
inline void smManagerDeleteData( smManager<ObjType>& mngr )
{
    // Cleanup all data
    for ( size_t i = 0 ; i < mngr.getNum() ; i++ )
    {
        // For some reason it is not possible to use smDelete
        smDelete2( mngr.get( i ) ) ;
    }
}

template <typename ObjType>
inline std::string
smManager<ObjType>::toString(const std::string& separator) const
{
	std::string str ;
	
	for( size_t i = 0 ; i < getNum() ; i++ )
    {
		str += smToString( get( i ) );

		if ( i != (getNum()-1) )
			str += separator ;
    }

	return str ;
}


template <typename ObjType>
inline size_t
smManager<ObjType>::getNum() const
{
    return m_objects.size();
}

template <typename ObjType>
inline
smManager<ObjType>::smManager( )
{
}

template <typename ObjType>
inline
smManager<ObjType>::~smManager()
{
    m_objects.clear() ;
}


template <typename ObjType>
int
smManager<ObjType>::findIdxOf( ObjType* obj ) const
{
	for ( size_t i = 0 ; i < getNum() ; i++ )
	{
		if ( obj == get(i) )
			return (int) i ;
	}

	return -1 ;
}



template <typename ObjType>
void
smManager<ObjType>::add(ObjType* obj, bool allowDuplicate )
{
    if ( allowDuplicate == false )
    {
        // first check if there is one already in the vector
        if ( find( obj ) != NULL )
            throw std::string("Duplicate entries are not allowed in this vector.");
    }

    m_objects.push_back( obj );
}

template <typename ObjType>
void
smManager<ObjType>::addManager(smManager<ObjType>& other, bool allowDuplicate )
{
    // This implementation is not fast, it uses a for look instead of std::vector::insert.
	// But it is easy-to-use and supports preventing duplicates.
	ObjectsVectorIt i = other.getVector().begin() ;
	for ( ; i != other.getVector().end() ; i++ )
		add( *i );
}


template <typename ObjType>
void
smManager<ObjType>::prependSlowly(ObjType* obj, bool allowDuplicate )
{
	if ( allowDuplicate == false )
	{
		// first check if there is one already in the vector
		if ( find( obj ) != NULL )
			throw std::string("Duplicate entries are not allowed in this vector.");
	}

	// We use a vector in the smManager. This is in general good unless a prepend
	// is needed. However implementing a manual prepend is still much faster than
	// using deque internally in the smManager because deque is in general slower.
	// So we have a manual prepend here.

	ObjectsVector tmpVec;

	// Add to front
	tmpVec.push_back( obj ) ;

	// Add the rest ( original )
	ObjectsVectorIt i = m_objects.begin() ;
	for( ; i != m_objects.end() ; i++ )
		tmpVec.push_back( *i ) ;

	// Copy to original
	m_objects = tmpVec ;

	// Shift one element
	//std::copy( m_objects.begin(), m_objects.end(), m_objects.begin() + 1 ) ;

	// Write to first element
	//m_objects[0] = obj ;
}

template <typename ObjType>
typename smManager<ObjType>::ObjectsVectorIt
smManager<ObjType>::findIterOf( ObjType* obj )
{
    ObjectsVectorIt i = m_objects.begin();
    for( ; i != m_objects.end() ; i++ )
    {
        if ( *i == obj )
            return i ;
    }

    // When no items are found in the m_objects
    return m_objects.end() ;
}

template <typename ObjType>
ObjType*
smManager<ObjType>::find( ObjType* obj )
{
    ObjectsVectorIt i = m_objects.begin();
    for( ; i != m_objects.end() ; i++ )
    {
        if ( *i == obj )
            return *i ;
    }

    // When no items are found in the m_objects
    return NULL ;
}

template <typename ObjType>
void
smManager<ObjType>::remove(ObjType* obj)
{
    typename smManager<ObjType>::ObjectsVectorIt i = findIterOf( obj ) ;

    if ( i == m_objects.end() )
        throw std::string("Can not find given object in its manager.") ;

    m_objects.erase( i ) ;
}

template <typename ObjType>
void
smManager<ObjType>::remove(int idx)
{
    m_objects.erase( m_objects.begin() + idx ) ;
}

template <typename ObjType>
void
smManager<ObjType>::removeAll()
{
    m_objects.clear() ;
}

template <typename ObjType>
ObjType*
smManager<ObjType>::get(size_t idx) const
{
    if ( static_cast<unsigned int>(idx) >= m_objects.size() ) {
        return NULL ;
    }else
        return m_objects[idx];
}

template <typename ObjType>
ObjType*
smManager<ObjType>::operator[](  int idx )
{
	return get(idx) ;
}