/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Base.h"
#include "RTD/EarthModelWGS84.h"
#include "RTD/UTM.h"
#include "Physics/smGraph2D.h"
#include "RTD/Terrain.h"




namespace RTD
{
    class VehicleBase ;
	class ReferenceEntityBase;
    class WorldPrivate ;
	class WorldParticipant;
	class BotBase ;



    /** World represents a virtual simulation environment. This model contains environmental
    *   parameters like gravity acceleration vector and air density.
    *
    *	For wind simulation models, please see WindsSystem.
    *
    *   Local Coordinate Center
    *   If you would like to set an initial position for your aircraft and helicopters using
    *   latitude,longitude and altitude, it is suggested to do this by positioning the initial center of
    *   the local coordinate system at a point on the earth surface.
    *   Please chapter "Earth Model" in Programmer's Guide.
    */
    class RTDYNAMICS_EXPORT World : public RTD::Base
    {
		friend class WorldParticipant;
		friend class BotBase;
		friend class VehicleBase;
		friend class ReferenceEntityBase;
    public:


        /** Constructor of the class. Constructor initializes the World to a reasonable state.
        *
        *   @param  massScale   rigid body dynamics models in RTDynamics libs work at higher
        *                       precision when the masses of bodies are around 1.0 kg.
        *
        *                       If your simulation simulates only one type of aircraft, set this
        *                       value to 1/mass_of_the_aircraft.
        *
        *                       If you simulate several different types of aircraft,the massScale shall
        *                       be set to a value roughly equal to 1 divided by the mass of the heavest
        *                       aircraft in your simulation. This value doesn't need to be exact, an
        *                       average value can be selected.
        */
        explicit World( smReal massScale =  1.0f / 200000.0f );
        virtual ~World( ) ;

        /** Returns the mass scale. */
        smReal getMassScale( ) const {return m_massScale;};


		/**
			Experimental feature.
		*/
		virtual bool	loadTerrain(const std::string& terrainDB , 
									const smVec2d& latLonOfDBOriginRadians , 
									const smDouble& altitude , 
									const std::vector<std::string>& searchPaths = std::vector<std::string>() );


        /** Use the setLocalCoordinateSystemCenter method to set the center of the world.
        *
        *   @param  lat,lon     latitude and longitude geographic coordinates of the aircraft. [radians]
        *   @param  alt         altitude of the aircraft relative to earth surface at the given lat,lon point. [m]
        *
        *   See also getLocalCoordinateSystemCenter
        */
        void    setLocalCoordinateSystemCenter( const EarthModelWGS84::Double& lat,
                                  const EarthModelWGS84::Double& lon,
                                  const EarthModelWGS84::Double& alt ) ;
        /** @deprecated Please use setLocalCoordinateSystemCenter   */
        void    setSurfaceCenter( const EarthModelWGS84::Double& lat,
                                  const EarthModelWGS84::Double& lon,
                                  const EarthModelWGS84::Double& alt ) { setLocalCoordinateSystemCenter( lat,lon,alt ); }

        /** This is an overloaded member function, provided for convenience.   */
        void    setLocalCoordinateSystemCenter( const smVec3d& lla ) ;

        /** @deprecated Please use setLocalCoordinateSystemCenter   */
        void    setSurfaceCenter( const smVec3d& lla ) { setLocalCoordinateSystemCenter( lla ); }

        /** Similar to the latitude longitude version, the world surface center can be specified with
        *   with the UTM coordinate system.
        *
        *   @param  UTMNorthing UTM northing coordinate in meters [out]
        *   @param  UTMEasting  UTM easting coordinate in meters [out]
        *   @param  UTMZone     String value of the UTM zone [out]
        */
        void    setLocalCoordinateSystemCenter( const EarthModelWGS84::Double UTMNorthing,
                                                const EarthModelWGS84::Double UTMEasting,
                                                const std::string& UTMZone ) ;

        /** @deprecated Please use setLocalCoordinateSystemCenter   */
        void    setSurfaceCenter( const EarthModelWGS84::Double UTMNorthing,
                                  const EarthModelWGS84::Double UTMEasting,
                                  const std::string& UTMZone ) {  setLocalCoordinateSystemCenter( UTMNorthing, UTMEasting, UTMZone ); }

        /** Return the specified geographic surface center position as latitude [rad], longitude [rad] and altitude [m].
         *  See also setLocalCoordinateSystemCenter.*/
        EarthModelWGS84::VecD   getLocalCoordinateSystemCenter() const;

        /** @deprecated Please use getLocalCoordinateSystemCenter   */
        EarthModelWGS84::VecD   getSurfaceCenter() const { return getLocalCoordinateSystemCenter(); }

        /** Return the specified geographic surface center position as latitude [rad], longitude [rad] and altitude [m].
         *  See also setLocalCoordinateSystemCenter.*/
        void    getLocalCoordinateSystemCenter(EarthModelWGS84::Double& lat,
                                               EarthModelWGS84::Double& lon,
                                               EarthModelWGS84::Double& alt ) const;

        /** @deprecated Please use getLocalCoordinateSystemCenter   */
        void    getSurfaceCenter(EarthModelWGS84::Double& lat,
                                 EarthModelWGS84::Double& lon,
                                 EarthModelWGS84::Double& alt ) const { getLocalCoordinateSystemCenter( lat, lon, alt ); }

        /**
        *   @name Environmental and atmospheric parameters of the world.
        *
        *   setGravity sets the gravity vector. Vector components are in m/sec^2 .
        *   getGravity returns the gravity vector. Vector components are in m/sec^2 .
        *
		*	getUpDirection() returns a unit vector in the opposite direction of gravity vector.
		*
        *   setAirDensityCurve sets the air density curve. This curve gives the density(kg/m^3) of the air
        *   in given altitude (in meters). 0 altitude is sea level.
        *
        *   getAirDensity returns the density of the air in given altitude. Altitude is in meters
        *   and 0 altitude is sea level. Returned value is in kg/m^3.
        *   Density is looked up from air density curve. If the value can not be found
        *   by look up, then 1.22 is returned by default.
        *
		*	getSpeedOfSound returns the speed of sound at the given altitude in m/s. The altitude argument
		*	is in meters.
        */
        //@{
        virtual void        setGravity( const smAccelVec& gravity ) ;
        const smAccelVec&   getGravity() const                      { return m_gravity; }
		const smAccelVec&   getUpDirection() const                  { return m_upDirection; }
        void                setAirDensityCurve( const smGraph2D& airDensityCurve );
        const smGraph2D&    getAirDensityCurve() const ;
        smGraph2D&			getAirDensityCurve();
        smReal              getAirDensity( smReal altitude ) const;		
		void                setSpeedOfSoundCurve( const smGraph2D& speedOfSoundCurve );
		const smGraph2D&    getSpeedOfSoundCurve() const ;
		smGraph2D&			getSpeedOfSoundCurve();
		smReal              getSpeedOfSound( smReal altitude ) const;
		void                setAirTemperatureCurve( const smGraph2D& airTemperatureCurve );
		const smGraph2D&    getAirTemperatureCurve() const ;
		smGraph2D&			getAirTemperatureCurve();
		smReal              getAirTemperature( smReal altitude ) const;
        //@}



        /**
        *   update() method updates the simulation by advancing it \e timestep seconds. By default an iterative update
        *   scheme is used. This means, if \e timestep is larger than the fixed delta time
        *   multiple iterations, each with a user provided fixed delta time, are performed. This increases the
        *   stability of the simulation. To disable iterative updates set \e useIterativeUpdate to false.
		*
		*   update() returns the time that has actually passed within the simulation. E.g. if fixed delta time is
		*   0.01, timestep = 0.032 and useIterativeUpdate is true than return value will be 0.03. Three consecutive
		*   updates each with 0.01s time step will be executed, simulation time will advance 0.03s and 0.002s will
		*   remain for the next update.
        */
        virtual smDouble    update( const smDouble& timestep , bool useIterativeUpdate = true ) ;

        /**
        *   advanceSimulationTime() can be used to implement a custom simulation update loop.
        *
        *   This method advances the simulation time \e timestep seconds and returns the fixed delta time
        *   , simulation time that will pass after the update loop, and the number of iterations that
        *   have to be done inside the loop. These values have to be used in the custom simulation loop.
        *
        */
        virtual void        advanceSimulationTime( const smDouble& timestep , smDouble& fixedDeltaTime , smDouble& timeThatWillPass , unsigned int& iterationCount );

        /**
        *   Sets the fixed delta time. \e fdt is in seconds. See update().
        */
        virtual void        setFixedDeltaTime( const smDouble fdt ) {   m_fixedDeltaTime = fdt; }

        /**
        *   Returns the fixed delta time in seconds.
        */
        smDouble            getFixedDeltaTime() const               {   return m_fixedDeltaTime;}		


        /** Utility method which creates a collision plane which
        *   can be used to represent a flat terrain.
        *
        *   @param  a,b,c,d plane parameters
        */
        void addCollisionPlane( smReal a, smReal b, smReal c, smReal d ) ;

        WorldPrivate*   getPrivate() { return m_p; }


        /**
            Returns the total time passed in the simulation in seconds. Simulation time is advanced
            every time internalUpdate method is called.
        */
        double  getSimulationTime() const   {   return m_simulationTime;    }


		/**
			Returns a pointer to the Vehicle instance with the given name. If such a vehicle 
			does not exist, NULL is returned.
		*/
		VehicleBase*				getVehicle( const std::string& name );

		/**
			Returns a pointer to the vehicle instance with the given name. If such a vehicle 
			does not exist, NULL is returned.
		*/
		const VehicleBase*			getVehicle( const std::string& name ) const;
		
		/**
			Returns a pointer to the bot instance with the given name. If such a bot 
			does not exist, NULL is returned.
		*/
		BotBase*					getBot( const std::string& name );
		
		/**
			Returns a pointer to the bot instance with the given name. If such a bot 
			does not exist, NULL is returned.
		*/
		const BotBase*				getBot( const std::string& name ) const;

		/**
			Returns a pointer to the entity reference instance with the given name. If such an entity reference
			does not exist, NULL is returned.
		*/
		ReferenceEntityBase*		getEntityReference( const std::string& name );

		/**
			Returns a pointer to the entity reference instance with the given name. If such an entity reference
			does not exist, NULL is returned.
		*/
		const ReferenceEntityBase*	getEntityReference( const std::string& name ) const;

		/**
			Experimental feature.
		*/
		void						setTerrain(Terrain*);

		/**
			Experimental feature.
		*/
		Terrain*					getTerrain()		{	return m_terrain;	}

		/**
			Experimental feature.
		*/
		const Terrain*				getTerrain() const	{	return m_terrain;	}




		/** This method can be used used to reset the World. reset()
			- sets the simulation time to 0
			- deletes all bots
			- deletes all vehicles
			- deletes all entity references
			- makes it possible to reuse entity names

			Note that, after reset() is called, pointers to previously created simulation
			entities will be invalid.
		*/
		virtual void				reset();

	protected:
		/**
			Return value of entity add methods (addBot,addVehicle,addEntityReference).
		*/
		enum AddResult
		{	
			Added = 0,
			BelongsToAnotherWorld,
			EntityNameNotUnique,
			EntityNameInvalid
		};

		/**
			Return value of entity remove methods (addBot,addVehicle,addEntityReference).
		*/
		enum RemoveResult
		{	
			Removed = 0,
			NotPartOfThisWorld,
			NULLPointer
		};

		smManager<WorldParticipant>      m_participants;

        virtual void    internalUpdate( const smDouble& timestep );

		virtual void	registerParticipant( WorldParticipant* participant );
		virtual void	deregisterParticipant( WorldParticipant* participant );

    
        smAccelVec  m_gravity ;
		smVec3r		m_upDirection;
        smGraph2D   m_airDensity;
		smGraph2D	m_speedOfSound;
		smGraph2D	m_airTemperature;
        smVec3r     m_windVelocity;

        smDouble    m_timeAccumulator;
        smDouble    m_fixedDeltaTime;

        smReal                                  m_massScale ;
        WorldPrivate*                           m_p ;
		std::map<std::string,VehicleBase*>				m_vehicles ;
        std::map<std::string,BotBase*>					m_bots ;
		std::map<std::string,ReferenceEntityBase*>		m_entityReferences ;
		
		Terrain*								m_terrain;


        // Center
        EarthModelWGS84::Double m_lat,m_lon,m_alt ;

        // Cached position of center in ECEF
        smVec3d                 m_initPosAsXYZ ;

        double                  m_simulationTime;


		/**
			Adds a bot to the World. This method is automatically called by the BotBase constructor
			when a new bot is created. 			
		*/
		virtual AddResult		addBot( BotBase* bot );

		/**
			Adds a vehicle to the World. This method is automatically called by the VehicleBase constructor
			when a new vehicle is created. 			
		*/
		virtual AddResult		addVehicle( VehicleBase* vehicle );

		/**
			Adds a entity reference to the World. This method is automatically called by the ReferenceEntityBase constructor
			when a new entity reference is created. 			
		*/
		virtual AddResult		addEntityReference( ReferenceEntityBase* ref );

		/**
			Removes a bot from the World. This method is automatically called by the BotBase destructor
			when a bot is deleted. 			
		*/
		virtual RemoveResult	removeBot( BotBase* bot );

		/**
			Removes a vehicle from the World. This method is automatically called by the VehicleBase destructor
			when a vehicle is deleted. 			
		*/
		virtual RemoveResult	removeVehicle( VehicleBase* vehicle );
		
		/**
			Removes an entity reference from the World. This method is automatically called by the ReferenceEntityBase
			destructor when an entity reference is deleted. 			
		*/
		virtual RemoveResult	removeEntityReference( ReferenceEntityBase* ref );

	};

	/** MBDWorld is deprecated and now it is called World.  */
	typedef World   MBDWorld ;

}
