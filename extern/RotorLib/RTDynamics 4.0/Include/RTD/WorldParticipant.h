/*
(c) Copyright 2009, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Base.h"



namespace RTD
{
	class World;
	
	/** 
		This class is a base class for simulation entities that take part in the simulation world.
		Through WorldParticipant a simple garbage collection mechanism is provided. Objects that
		are derived from WorldParticipant are deleted automatically when their RTD::World is deleted.

		It is possible to delete objects before the destruction of RTD::World by using remove() method.
		See remove() for more information.

	*/
	class RTDYNAMICS_EXPORT WorldParticipant : public Base
	{
		friend class World;
	public:
		/** Constructor of the class. The ownership of the newly generated object is passed to 
			the \e world object.*/
		WorldParticipant( World* world );

		/** Deletes this world participant. Classes that are derived from WorldParticipant can not be 
			deleted with delete operator. Instead, this method must be used.

			In case deleting is dangerous (e.g. deleted participant might be referred from others), deletion
			might fail. In this case, false is returned.

			Note that all world participants will be deleted automatically when World is deleted. Hence,
			most of the time deleting explicitly is not necessary. 
		*/		
		bool	remove();

		/** Returns the world whom this object belongs. */
		virtual World*			getWorld()			{	return m_world;	}

		/** Returns the world whom this object belongs. */
		virtual const World*	getWorld() const	{	return m_world;	}
	protected:
		World*	m_world;

		/**
			Classes that are derived from WorldParticipant override this method to enable/disable
			the use of remove(). If canBeDeleted() returns false remove() method will fail always.
		*/
		virtual bool canBeDeleted() const;

		/** Destructor of the class is protected to prevent calling delete operator.
			A WorldParticipant is deleted automatically when its world is destroyed. Altenatively
			remove() method can be used.
		*/
		virtual ~WorldParticipant();
	};
}