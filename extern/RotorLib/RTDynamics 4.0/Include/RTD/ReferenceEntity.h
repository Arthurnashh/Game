#pragma once

#include "RTD/VehicleBase.h"
#include "RTD/WorldParticipant.h"
#include "RTD/FrequencyDifferenceFilter.h"


namespace RTD
{

    /** 
		This class is a base class for entity references. An entity reference is used to convey
		physical state information of an internal/external simulation entity to another.

		@see Entity References section in the Programmer's Guide. 
    **/
    class RTDYNAMICS_EXPORT ReferenceEntityBase : public RTD::WorldParticipant, public SimulationVariableUser
    {
		friend class World;
    public:
		/** Constructor of the class. Note that the constructor takes a RTD::World as argument.
			Each Entity Reference belongs to a RTD::World and is automatically deleted when
			the world is deleted.		
		*/
		ReferenceEntityBase( World *world , const std::string& name );

		/** This method should return true if the referenced entity is still alive and active.
			If the entity is deleted or inactivated (shot down, crashed, removed from the simulation manually etc. )
			then isActive should return false. This informs entities which might be executing maneuvers
			which depend on the deleted entity. E.g. if an airplane pursues another airplane and the latter 
			is hit by a missile, pursuer can notice this and assume level flight. */
        virtual bool isActive() = 0;

        /**
            @name Physical State Information Access Methods

			These methods are used to acess physical state information of the referred entity.
			They are all in world coordinate system. As unit metric system is used. Angles are
			in degrees.

			Rotational velocity and acceleration are values around x,y and z axes in rad/s 
			and rad/s^2 respectively.

			The methods whose name has "Filtered" suffix return update frequency difference 
			corrected values. The ones with "NotFiltered" suffix return raw data. Preferred way 
			of accessing state information is using filtered versions.

			@see Entity References section in the Programmer's Guide.
        */
        //@{
			virtual smVec3r getPositionFiltered() const;
			virtual smVec3r getPositionNotFiltered() const;
	        
			virtual smVec3r getVelocityFiltered() const;
			virtual smVec3r getVelocityNotFiltered() const;

			virtual smVec3r getAccelerationFiltered() const;
			virtual smVec3r getAccelerationNotFiltered() const;

			virtual smVec3r getRotationAsHPRFiltered() const;
			virtual smVec3r getRotationAsHPRNotFiltered() const;

			virtual smVec3r getRotVelocityWCSFiltered() const;
			virtual smVec3r getRotVelocityWCSNotFiltered() const;

			virtual smVec3r getRotAccelerationWCSFiltered() const;
			virtual smVec3r getRotAccelerationWCSNotFiltered() const;

        //@}
		
		
		/** For RTDynamics use only. */
		void	enableTestMode( bool en )	{	m_enableTestMode = en;	}

		/** For RTDynamics use only. */
		bool	isTestModeEnabled() const	{	return m_enableTestMode;	}

	protected:

		bool	m_enableTestMode;
		smReal	m_noiseTimer;
		smVec3r m_position;
		smVec3r m_velocity;
		smVec3r m_acceleration;
		smVec3r m_rotHPR;
		smVec3r m_rotVel;
		smVec3r m_rotAccel;

		RTD::FrequencyDifferenceFilter	m_freqDiffFilterPosition;
		RTD::FrequencyDifferenceFilter	m_freqDiffFilterVelocity;
		RTD::FrequencyDifferenceFilter	m_freqDiffFilterAcceleration;
		RTD::FrequencyDifferenceFilter	m_freqDiffFilterHPR;
		RTD::FrequencyDifferenceFilter	m_freqDiffFilterRotVelocity;
		RTD::FrequencyDifferenceFilter	m_freqDiffFilterRotAcceleration;

		/** Returns the position of the vehicle in world coordinates. Members of the vector are in meters. */
        virtual smVec3r getPosition() = 0 ;

		/** Returns the velocity of the vehicle in world coordinates. Members of the vector are in m/s. */
        virtual smVec3r getVelocity() = 0 ;

		/** Returns the acceleration of the vehicle in world coordinates. Members of the vector are in m/s. */
        virtual smVec3r getAcceleration() = 0 ;

		/** Returns the orientation of the vehicle as heading, pitch and roll. All angles are in degrees. Standard RTD
			coordinate system conventions are used. A summary is provided below:

			X: right

			Y: front

			Z: up

			Heading : Rotation around Z, positive in counter clockwise sense when viewed from above.

			Pitch   : Rotation around X, positive nose up when viewed from pilots point of view.

			Roll	: Rotation around Y, positive when banked to right when viewed from pilot's point of view.
			*/
        virtual smVec3r getRotationAsHPR() = 0 ;

		/** Returns the rotational velocity of the vehicle. The vector should contain rotational speed
			around x,y and z axes in \e world coordinates and in \e rad/sec.
			
			Note: This vector is not simply first derivative of getRotationAsHPR() of above. Please note the
			unit and order difference.
		*/
        virtual smVec3r getRotVelocityWCS() = 0 ;

		/** Returns the rotational acceleration of the vehicle. The vector should contain rotational acceleration
			around x,y and z axes in \e world coordinates and in \e rad/sec^2.
			
			Note: This vector is not simply second derivative of getRotationAsHPR() of above. Please note the
			unit difference and order difference. 
			
			On the other hand getRotAccelerationWCS \e is first derivative of getRotVelocityWCS. */
		virtual smVec3r getRotAccelerationWCS() = 0 ;

    
		/**
			This method returns always false to disable the remove() method.
		*/
        virtual bool canBeDeleted() const;

        virtual ~ReferenceEntityBase();

		/**
			This method is called by the RTD::World at every simulation step. Filtering of 
			physical state information is performed in this method.
		*/
		virtual void	update( const smDouble& deltaTime );

		virtual void	reset();

		/**
			This method is made protected to prevent that the name that is given in the constructor is
			changed later. Entity reference names are also unique ID's and changing them might create problems.
		*/
		virtual void setName( const std::string& name )  {}
    };

    /** 
		This class implements an entity reference for RTDynamics vehicle models. 
		@see Entity References section in the Programmer's Guide. 
	*/
    class RTDYNAMICS_EXPORT ReferenceVehicle : public ReferenceEntityBase
    {
        friend class VehicleBase;
    public:
        virtual bool isActive();

		/** Returns the simulation entity this object is referring to. */
		VehicleBase*    getVehicle() ;

	protected:
        virtual smVec3r getPosition();
        virtual smVec3r getVelocity() ;
        virtual smVec3r getAcceleration();

        virtual smVec3r getRotationAsHPR();

        virtual smVec3r getRotVelocityWCS();
        virtual smVec3r getRotAccelerationWCS();

        /** Constructor of the class. Note that the constructor is protected hence this class can not be created explicitly.
            An instance can be obtained from a vehicle by using getReference() method. */
		ReferenceVehicle( VehicleBase* vh , const std::string& name );


        VehicleBase*   m_vehicle;

		const RTD::SimulationVariableVec3d*	m_vehPosition;
		const RTD::SimulationVariableVec3d*	m_vehVelocity;
		const RTD::SimulationVariableVec4d*	m_vehAttitudeQuad;
		const RTD::SimulationVariableVec3d*	m_vehAngularVelocityWCS ;		

		const RTD::SimulationVariableVec3d*	m_vehAttitudeHPR ;		
		const RTD::SimulationVariableVec3r*	m_vehAcceleration ;		
		const RTD::SimulationVariableVec3r*	m_vehAngularAccelerationWCS ;		
    };

    /**
     *  
     */
    class RTDYNAMICS_EXPORT SimpleReference : public ReferenceEntityBase
    {
        friend class VehicleBase;
    public:
        virtual bool isActive()
        {
            return true;
        }
        SimpleReference( World* world ,
						VehicleBase* owner,
						const std::string& name,
                        const smVec3r& xyz      = smVec3r(0,0,0), 
                        const smVec3r& vel      = smVec3r(0,0,0), 
                        const smVec3r& acc      = smVec3r(0,0,0), 
                        const smVec3r& hpr      = smVec3r(0,0,0), 
                        const smVec3r& rotVel   = smVec3r(0,0,0), 
                        const smVec3r& rotAcc   = smVec3r(0,0,0) 
                        ) : ReferenceEntityBase(world,name)
        , m_xyz   (xyz   )
        , m_vel   (vel   )
        , m_acc   (acc   )
        , m_hpr   (hpr   )
        , m_rotVel(rotVel)
        , m_rotAcc(rotAcc)
        {
        }

        virtual smVec3r getPosition()       { return m_xyz   ; }
        virtual smVec3r getVelocity()       { return m_vel   ; }
        virtual smVec3r getAcceleration()   { return m_acc   ; }
        virtual smVec3r getRotationAsHPR()  { return m_hpr   ; }
        virtual smVec3r getRotVelocityWCS()    { return m_rotVel; }
        virtual smVec3r getRotAccelerationWCS(){ return m_rotAcc; }

        smVec3r m_xyz       ;
        smVec3r m_vel       ;
        smVec3r m_acc       ;
        smVec3r m_hpr       ;
        smVec3r m_rotVel    ;
        smVec3r m_rotAcc    ;
    };


}

