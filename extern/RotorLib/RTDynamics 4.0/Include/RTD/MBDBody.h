/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/World.h"
#include "RTD/MBDRigidBodyState.h"

namespace RTD
{
    class MBDVehicle ;
    class World ;
    class MBDBodyPrivate;

    /** MBDBody is a body in the world. All helicopters and aircraft
    *   in the World create and manage at least one body (e.g. fuselage) object.
    *
    *   It is not necessary to create an instance of this class or call methods it's
    *   methods.
    */
    class RTDYNAMICS_EXPORT MBDBody
    {
    public:
        MBDBody( World* world ) ;
        virtual ~MBDBody() ;

        MBDBodyPrivate* getPrivate() { return m_p; }

		
        World*  getWorld() { return m_world; }
		const World*  getWorld() const { return m_world; }
        void    setMass( smDouble mass, const smVec4d& MoI, const smVec3d& cog ) ;
		void    setMass( smDouble mass, const smVec4d& MoI  ) ;
        void    getMomentsOfInertia( smVec4d& inertia );

        /** Returns the mass of this body which is set with setMass method. */
        smDouble  getMass() const;

		const smVec3d&	getCenterOfGravityBCS() const;

        /** Turn on/off the gravity on this body. Gravity is on by default. */
        void    setGravityMode(bool on);

        /** Enables/disables this body. When disabled the body remains
         *  unchanged during simulation.
         */
        void    setEnabled( bool enabled );

        /** Returns true if this body is enabled.   */
        bool    isEnabled( ) ;

        void                    wcs_to_bcs( const smVec3r& vecInWCS, smVec3r& vecInBCS ) const ;
        void                    bcs_to_wcs( const smVec3r& vecInBCS, smVec3r& vecInWCS ) const ;
        void                    wcs_to_bcs( const smVec3d& vecInWCS, smVec3d& vecInBCS ) const ;
        void                    bcs_to_wcs( const smVec3d& vecInBCS, smVec3d& vecInWCS ) const ;

		// Position
		virtual void			setPosition( const smVec3r& p );
		virtual smVec3r			getPosition() const;

		virtual void			setPositionD( const smVec3d& p );
		virtual smVec3d			getPositionD() const;

		// Angular Position
		virtual smAngleDVec		getRotAsHPR() const ;
		virtual smVec3d			getRotAsHPRD() const ;
		const smMatrix&			getRotMat() const ;
		const smMatrix&			getRotMatInv() const ;
		const smVec4d			getRotAsQuat() const ;
		
		void					setRotAsQuat( const smVec4d& quat );

		virtual void			setRotWithHPR		( const smAngleDVec& hpr) ;
		virtual void			setRotWithHPRD		( const smVec3d& hpr) ;

		// Velocity
		virtual const smSpeedVec&	getSpeed() const;
		smReal						getSpeedLength() const;
		virtual void				setSpeed(const smSpeedVec& s);
		virtual const smVec3d 		getSpeedD() const;
		smDouble					getSpeedLengthD() const;
		virtual void				setSpeedD(const smVec3d& s);

		// Angular Velocity
		virtual smVec3r				getRotSpeedBCS() const ;
		void						setRotSpeedBCS( const smVec3r& spd );

		virtual  smVec3d			getRotSpeedBCSD() const ;
		void						setRotSpeedBCSD( const smVec3d& spd );

        virtual smVec3r				getRotSpeedWCS() const;
        virtual void                setRotSpeedWCS( const smVec3r& spd );

        virtual smVec3d 			getRotSpeedWCSD() const;
        virtual void                setRotSpeedWCSD( const smVec3d& spd );


		virtual void				addForceAtRelPos( const smVec3r& force , const smVec3d& relativePos );
		virtual void				addRelTorque( const smVec3r& torque );

		virtual void				setState(  const SimulationVariableVec3d& weightBodyPosBCS ,
									  		   const SimulationVariableVec3d& weightBodyPos , 
											   const SimulationVariableVec4d& weightBodyQuat , 
										   	   const SimulationVariableVec3d& weightBodyVel , 
											   const SimulationVariableVec3d& weightBodyAngVel,
											   const SimulationVariableDouble& weightBodyMass,
											   const SimulationVariableVec4d& weightBodyMomentsOfInertia  );

		virtual void				getState(	SimulationVariableVec3d& weightBodyPosBCS,
												SimulationVariableVec3d& weightBodyPos , 
												SimulationVariableVec4d& weightBodyQuat , 
												SimulationVariableVec3d& weightBodyVel , 
												SimulationVariableVec3d& weightBodyAngVel,
											    SimulationVariableDouble& weightBodyMass,
												SimulationVariableVec4d& weightBodyMomentsOfInertia	) const;
    protected:
        MBDBodyPrivate*		m_p;
        World*				m_world ;
        smDouble			m_givenMass ;
		smVec4d				m_givenMoI ;
		mutable smVec3d		m_dummyVecPos ;
		mutable smVec3r		m_dummyVecVelocity;
		mutable smVec3r		m_dummyVecRotVelocity;
		mutable smVec3r		m_dummyVecAngMom;
		mutable smMatrix	m_dummyMatrix ;
		
		smVec3d				m_cogBCS;

		void	updateBodyAndCoGPos(const smVec3d& newPosition ,const smVec3d& newCoG );
		void	updateCoGLinearAndAngularVelocity( const smVec3d& vel, const smVec3d& angVel );	
		void	computeRefPointVelocity( smVec3d& result ) const;

    };
}
