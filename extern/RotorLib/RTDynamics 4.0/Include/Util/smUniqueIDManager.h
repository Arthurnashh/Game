/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#pragma warning ( disable : 4786 )
#include <vector>
#include "smMacros.h"

class smUniqueIDManager
{
protected:
	std::vector<int>	m_ids;
	int					m_startId ;
	int					m_lastUniqueId ;

public:
	smUniqueIDManager()
	{
		m_startId		=	0 ;
		m_lastUniqueId	=	m_startId ;
	}

	virtual ~smUniqueIDManager()
	{
	}

	//
	// When a start id is specified
	// unique ids start with this id.
	//
	void setStartId( int id )
	{
		m_lastUniqueId	=	id ;
	}

	int makeUniqueId()
	{
		// Find a unique id, simply we find the max id.
		std::vector<int>::iterator i = m_ids.begin() ;
		for ( ; i != m_ids.end() ; i++ )
		{
			if ( *i >= m_lastUniqueId )
				m_lastUniqueId = *i ;
		}

		int newId = m_lastUniqueId + 1 ;
		m_ids.push_back( newId ) ;

		m_lastUniqueId	=	newId ;

		return newId ;
	}

	void insertUniqueId( int id )
	{
		// First check if the id exists
		// This should never happen
		std::vector<int>::iterator i = m_ids.begin() ;
		for ( ; i != m_ids.end() ; i++ )
		{
			smValidateAndExit(*i != id, "*i == id", "smUniqueIDManager::insertUniqueId");
		}

		m_ids.push_back( id ) ;
	}
};