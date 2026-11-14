/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/RigidBodyStateBase.h"

namespace RTD
{
	class MBDBody ;

	/**	MBDRigidBodyState provides state information about a MBDBody
	*	such as position and velocity.
	*
	*	All methods in this class are re-implementeations of the RigidBodyStateBase,
	*	therefore please refer to the RigidBodyStateBase class documentation for detailed
	*	information regarding the class methods.
	*/
	class RTDYNAMICS_EXPORT MBDRigidBodyState : public RigidBodyStateBase
	{		
	public:
		MBDRigidBodyState( const std::string& name, MBDBody& body );

		virtual void			reset();

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
		virtual void			setRotWithHPR		( const smAngleDVec& hpr) ;
		virtual void			setRotWithHPRD		( const smVec3d& hpr) ;
		virtual void			setRotWithMatrix	( const smMatrix& rotMat ) ;
		virtual void			setRotWithQuat		( const smQuat& q ) ;

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

		// Acceleration
		virtual const smAccelVec&	getAccel() const;
		virtual void				setAccel(const smAccelVec& a);

		// Rotational Acceleration
		virtual smVec3r				getRotAccelBCS() const	;
		virtual void				setRotAccelBCS(const smVec3r& a)	;

        virtual smVec3r				getRotAccelWCS() const;
        virtual void                setRotAccelWCS(const smVec3r&);

		// Forces & Torques
		virtual const smForceVec&	getSumForces() const ;
		virtual void				setSumForces (const smForceVec& s) ;
		virtual const smTorqueVec&	getSumTorques() const ;
		virtual void				setSumTorques (const smTorqueVec& s);
		virtual const smForceVec&	getSumForcesWithoutWeight() const ;
		virtual void				setSumForcesWithoutWeight(const smForceVec& s) ;

		const smAngularMomentumVec& getAngularMomentum() const ;
	private:
		mutable MBDBody&	m_body ;
		mutable smVec3d		m_dummyVecPos ;
		mutable smVec3r		m_dummyVecVelocity;
		mutable smVec3r		m_dummyVecRotVelocity;
		mutable smVec3r		m_dummyVecAngMom;
		mutable smMatrix	m_dummyMatrix ;
		
		// MBD doesn't store acceleration. Kinematically computed acceleration is stored here.
		smAccelVec			c_accel ;
		smAngularAccelVec	c_angAccel ;

		smForceVec			c_sumForces ;				//!<	Sum of all forces acting on the body [N]
		smForceVec			c_sumForcesWithoutWeight ;	//!<	Sum of all forces acting on the body excluding the weight[N]
		smTorqueVec			c_sumTorques ;				//!<	Sum of all torques(moments) acting on the body [Nm]
	};
}
