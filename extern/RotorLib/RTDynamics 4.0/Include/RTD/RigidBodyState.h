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
	/**	RigidBodyState contains momentary state variables which are computed
	*	in each simulation update.
	*
	*	This class is used with aircraft models which utilize GAVDynamics class.
	*/
	class RTDYNAMICS_EXPORT RigidBodyState : public RigidBodyStateBase
	{
		friend class GAVDynamics ;
	public:
		smVec3r				c_pos ;				//!<	Current position in [m]
		smSpeedVec			c_speed ;			//!<	Current translation speed in [m/s]
		smReal				c_speedLength ;		//!<	Current speed in [m/s]
		smAccelVec			c_accel ;			//!<	Current acceleration [m/s^2]

		smForceVec			c_sumForces ;				//!<	Sum of all forces acting on the body [N]
		smForceVec			c_sumForcesWithoutWeight ;	//!<	Sum of all forces acting on the body excluding the weight[N]
		smTorqueVec			c_sumTorques ;				//!<	Sum of all torques(moments) acting on the body [Nm]

		smLinearMomentumVec		c_P ;		//!<	Current linear momentum vector
		smMatrix				c_IInv ;	//!<	Current inverse inertia matrix
		smAngularMomentumVec	c_L ;		//!<	Current angular momentum vector

		/**
			If you prefer to get the rotation as a quaternion,
			c_rotQuatNormalized can be used.

			@deprecated Please use setRotWithQuat and other rotation functions
			to set the rotation.
		*/
		smQuat					c_rotQuatNormalized ;

		/** c_rot, c_rotInv are protected as they shall not be set by
		*	any external user as these matrices are always regenerated from
		*	the quaternions.
		*/
	protected:
		smMatrix				c_rot ;
		smMatrix				c_rotInv ;
		smQuat					c_rotQuatPreNorm ;

		smVecIntegrator			m_posIntegrator;
		smVecIntegrator			m_speedIntegrator;
		smVecIntegrator			m_LIntegrator;

	protected:
		/**
			@name Rotation speed and acceleration
			The rotation speed and acceleration shall not be set by any external
			user as these vectors are always recomputed by the rigid body dynamics.
			getRotSpeedBCS() and getRotAccelBCS() can be used to read current values.
		*/
		//@{
		smAngularRateVec    c_rotSpeed ;	//!<	Current rotational speed vector around x,y and z axes [radians/s]
		smAngularAccelVec   c_rotAccl ;		//!<	Current rotational acceleration vector [radians/s^2]
		//@}

	private:
		/** RigidBodyState copy constructor and =operator are not allowed.
		*	Please use the copy method for copying RigidBodyState data from one
		*	to another.
		*/
		// The Param database system doesn't play nicely with copy ctor and =operator.
		// This is the reason why these methods are not allowed.
		RigidBodyState( const RigidBodyState& right );
		RigidBodyState operator=( const RigidBodyState& right ) ;

	public:
		RigidBodyState( const std::string& name );

		/** Copy method	can be used to copy the data of another RigidBodyState to this one.
		*	This method shall be used instead of a copy constructor or = operator.
		*/
		virtual void			copy( const RigidBodyState& right );

		virtual void			reset();

		// Position
		virtual void		    setPosition( const smVec3r& p )			{ c_pos = p ; }
		virtual smVec3r		    getPosition() const						{ return c_pos; }

		virtual void		    setPositionD( const smVec3d& p )		{ c_pos = smVec3dToVec3r(p) ; }
		virtual smVec3d		    getPositionD() const					{ return smVec3rToVec3d(c_pos); }

		// Angular Position
		virtual smAngleDVec		getRotAsHPR() const ;
		virtual smVec3d			getRotAsHPRD() const ;
		const smMatrix&			getRotMat() const		{ return c_rot; }
		const smMatrix&			getRotMatInv() const	{ return c_rotInv; }
		virtual void			setRotWithHPR		( const smAngleDVec& hpr) ;
		virtual void			setRotWithHPRD		( const smVec3d& hpr) ;
		virtual void			setRotWithMatrix	( const smMatrix& rotMat ) ;
		virtual void			setRotWithQuat		( const smQuat& q ) ;

		// Velocity
		virtual const smSpeedVec&	getSpeed() const				{ return c_speed; } 
		smReal						getSpeedLength() const			{ return c_speedLength; } 
		virtual void				setSpeed(const smSpeedVec& s)	{ c_speed = s ; c_speedLength = c_speed.length () ; } 

		virtual const smVec3d		getSpeedD() const				{ return smVec3rToVec3d(c_speed); } 
		smDouble					getSpeedLengthD() const			{ return (smDouble)c_speedLength; } 
		virtual void				setSpeedD(const smVec3d& s)		{ c_speed = smVec3dToVec3r(s) ; c_speedLength = c_speed.length () ; } 

		// Angular Velocity
		virtual smVec3r				getRotSpeedBCS() const { return c_rotSpeed; } 
		void						setRotSpeedBCS( const smVec3r& spd ) { c_rotSpeed = spd ; }	
		virtual smVec3d				getRotSpeedBCSD() const { return smVec3rToVec3d(c_rotSpeed); } 
		void						setRotSpeedBCSD( const smVec3d& spd ) { c_rotSpeed = smVec3dToVec3r(spd); }	


        virtual smVec3r				getRotSpeedWCS() const;
        virtual void                setRotSpeedWCS( const smVec3r& spd );
        virtual smVec3d				getRotSpeedWCSD() const;
        virtual void                setRotSpeedWCSD( const smVec3d& spd );

        virtual smVec3r			    getRotAccelWCS() const;
        virtual void                setRotAccelWCS(const smVec3r&) ;




		// Acceleration
		virtual const smAccelVec&	getAccel() const				{ return c_accel; } 
		virtual void				setAccel(const smAccelVec& a)	{ c_accel = a; } 

		// Rotational Acceleration
		virtual smVec3r				getRotAccelBCS() const				{ return c_rotAccl; }
		virtual void				setRotAccelBCS(const smVec3r& a)	{ c_rotAccl = a; }

		// Forces & Torques
		virtual const smForceVec&	getSumForces() const { return c_sumForces; } 
		virtual void				setSumForces (const smForceVec& s) { c_sumForces = s; }
		virtual const smTorqueVec&	getSumTorques() const { return c_sumTorques; } 
		virtual void				setSumTorques (const smTorqueVec& s) { c_sumTorques = s; }
		virtual const smForceVec&	getSumForcesWithoutWeight() const { return c_sumForcesWithoutWeight; } 
		virtual void				setSumForcesWithoutWeight(const smForceVec& s) { c_sumForcesWithoutWeight = s; } ;

		const smAngularMomentumVec& getAngularMomentum() const { return c_L; }

	};
}
