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
#include "RTD/ParamDb.h"

namespace RTD
{
    /** RigidBodyStateBase provides a list of getter and setters methods which deal with the current
    *   state of the aircraft. This class can be used to set the initial position, rotation and
    *   velocity of an aircraft in addition to setting and getting other state variables.
    *
    *   Subclassing
    *   Concreate implementations of this class are already available and it is not necessary to
    *   subclass or create an instance of this class.
    *
    *   The local coordinate system
    *   The position and rotation of all aircraft and helicopters in RTDynamics library are in the local
    *   coordinates. This is a regular x-y-z coordinate system and doesn't specify north and south. If
    *   you wish to set the initial position of the aircraft with geographic coordinates or get its
    *   current geographic coordinates, please see the chapter "Earth Model" in programmer's guide.
    */
    class RTDYNAMICS_EXPORT RigidBodyStateBase : public Base
    {
    public:
        /** @name Getters and Setters for state variables
        *
        *   The RigidBodyState consist of several state variables and provides
        *   several getter/setter methods for those variables. It also includes some convenience methods.
        *
        *   <B>Position</B>
		*
        *   The current 3d position vector of the body in local coordinates. Unit: meters.
        *                   getPosition <BR>
        *                   setPosition <BR>
        *
        *   <B>Angular Position</B>
		*
        *   The current rotation the body in various forms. Matrix and quaternion forms do not have units.
        *   HPR (heading or yaw, pitch, roll) convention returns rotation in degrees for each axis. All set
        *   methods update other forms as well.
        *
        *                   setRotWithMatrix <BR>
        *                   setRotWithQuat <BR>
        *                   getRotMat/getRotMatInv <BR>
        *                   setRotWithHPR/getRotAsHPR <BR>
        *
        *   <B>Velocity</B>
		*
        *   The current 3d velocity vector of the body in local coordinates. Unit: meters/second.
        *   
        *                   getSpeed <BR>
        *                   setSpeed <BR>
        *
        *   <B>Angular Velocity</B>
		*
        *   The current rotational velocity (rates) in body or world coordinate system as a vector around x,y .
        *   and z axes.Methods with BCS suffix are in body coordinates whereas WCS suffix describe world coordinate 
		*	system.
		*
		*	Methods ending with 'D' use double precision floating points.
		*
		*	Unit: radians/seconds.
        *                   getRotSpeedBCS <BR>
        *                   setRotSpeedBCS <BR>
        *                   getRotSpeedWCS <BR>
        *                   setRotSpeedWCS <BR>
        *                   getRotSpeedBCSD <BR>
        *                   setRotSpeedBCSD <BR>
        *                   getRotSpeedWCSD <BR>
        *                   setRotSpeedWCSD <BR>
        *
        *   <B>Acceleration</B>
		*
        *   The current 3d acceleration vector of the body in local coordinates. Unit: meters/seconds^2.
        *
        *   To set the acceleration during a simulation doesn't make sense as it will be overwritten immediately
        *   in the first update by the dynamics engine. It is provided because the reset method uses it.
        *
        *                   getAccel <BR>
        *                   setAccel <BR>
        *
        *   <B>Angular Acceleration</B>
		*
        *   The current rotational acceleration in body or world coordinate system as a vector around x,y and z axes.
		*   Methods with BCS suffix are in body coordinates whereas WCS suffix describe world coordinate system.
        *   Unit: radians/seconds^2.
        *
        *   To set the acceleration during a simulation doesn't make sense as it will be overwritten immediately
        *   in the first update by the dynamics engine. It is provided because the reset method uses it.
        *
        *                   getRotAccelBCS <BR>
        *                   setRotAccelBCS <BR>
        *                   getRotAccelWCS <BR>
        *                   setRotAccelWCS <BR>
        *
        *   <B>Forces & Torques</B>
		*
        *   The forces and torques from force generators acting on a body are stored in the state class,
        *   although they are not really state variables in a dynamics sense. These getter methods can be
        *   used for debugging force generators.
        *
        *                   getSumForces <BR>
        *                   getSumForcesWithoutWeight <BR>
        *                   getSumTorques <BR>
        *                   setSumForces <BR>
        *                   setSumTorques <BR>
        *                   setSumForcesWithoutWeight <BR>
        *
        *   <B>Momentum</B>
		*
        *                   getAngularMomentum <BR>
        */
        //@{
        // Position
        virtual void                 setPosition( const smVec3r& p ) = 0 ;
        virtual smVec3r				 getPosition() const = 0 ;

		virtual void                 setPositionD( const smVec3d& p ) = 0 ;
		virtual smVec3d 			 getPositionD() const = 0 ;

        // Angular Position
        virtual smAngleDVec         getRotAsHPR() const = 0 ;
        virtual smVec3d		        getRotAsHPRD() const = 0 ;

        virtual const smMatrix&     getRotMat() const       = 0 ;
        virtual const smMatrix&     getRotMatInv() const    = 0 ;
        virtual void                setRotWithHPR       ( const smAngleDVec& hpr) = 0 ;
		virtual void                setRotWithHPRD      ( const smVec3d& hpr) = 0 ;
        virtual void                setRotWithMatrix    ( const smMatrix& rotMat ) = 0 ;
        virtual void                setRotWithQuat      ( const smQuat& q ) = 0 ;

        // Velocity
        virtual const smSpeedVec&   getSpeed() const = 0 ;
		virtual const smVec3d 		getSpeedD() const = 0 ;
        virtual smReal              getSpeedLength() const = 0 ;
        virtual smDouble            getSpeedLengthD() const = 0 ;
        virtual void                setSpeed(const smSpeedVec&) = 0 ;
		virtual void                setSpeedD(const smVec3d&) = 0 ;

        // Angular Velocity
        virtual smVec3r				getRotSpeedBCS() const = 0 ;
        virtual void                setRotSpeedBCS( const smVec3r& spd ) = 0 ;

        virtual smVec3d 			getRotSpeedBCSD() const = 0 ;
        virtual void                setRotSpeedBCSD( const smVec3d& spd ) = 0 ;

        virtual smVec3r				getRotSpeedWCS() const = 0 ;
        virtual void                setRotSpeedWCS( const smVec3r& spd ) = 0 ;

        virtual smVec3d 			getRotSpeedWCSD() const = 0 ;
        virtual void                setRotSpeedWCSD( const smVec3d& spd ) = 0 ;


        // Acceleration
        virtual const smAccelVec&   getAccel() const = 0 ;
        virtual void                setAccel(const smAccelVec&) = 0 ;

        // Rotational Acceleration
        virtual smVec3r				getRotAccelBCS() const = 0 ;
        virtual void                setRotAccelBCS(const smVec3r&) = 0 ;
        virtual smVec3r				getRotAccelWCS() const = 0 ;
        virtual void                setRotAccelWCS(const smVec3r&) = 0 ;

        // Forces & Torques
        virtual const smForceVec&   getSumForces() const = 0 ;
        virtual void                setSumForces (const smForceVec&) = 0 ;
        virtual const smTorqueVec&  getSumTorques() const = 0 ;
        virtual void                setSumTorques (const smTorqueVec&) = 0 ;
        virtual const smForceVec&   getSumForcesWithoutWeight() const = 0 ;
        virtual void                setSumForcesWithoutWeight(const smForceVec&) = 0 ;

        virtual const smAngularMomentumVec& getAngularMomentum() const = 0 ;
        //@}

        /** Resets all the state variables to zero. */
        virtual void            reset() = 0 ;


        // ======================UTILITY METHODS BEGIN=========================================


        /** Returns the velocity of a point on this rigid body in the local coordinate
        *   system.
        *   When the body has no translational speed, a point on the body may have a tr.
        *   speed due to bodie's rotational speed.
        */
        smSpeedVec              getVelocityAtPointOnBody( const smPositionVec& pointPosInBCS ) const ;

        /** Returns the velocity of a point on this rigid body in the local coordinate
        *   system.
        *   When the body has no translational speed, a point on the body may have a tr.
        *   speed due to bodie's rotational speed.
        */
        smVec3d					getVelocityAtPointOnBody( const smVec3d& pointPosInBCS ) const ;

        /** Returns the accelereration of a point on this rigid body in the local coordinate
        *   system.
        *   When the body has no translational speed, a point on the body may have an acceleration
        *   due to bodie's rotational speed.
        */
        smVec3r              getAccelerationAtPointOnBody( const smVec3r& pointPosInBCS ) const ;

        /** This method calculates the relative wind at the given point on the body. By definition
        *   relative wind is defined in body coordinate system. The method takes :
        *       - aircraft speed
        *       - angular velocity of the point
        *       - wind speed (defined in the local)
        *   into account.
        */
        smSpeedVec              getRelWindAtPointOnBody( const smPositionVec& pointPosInBCS, const smSpeedVec& localWind ) const ;

        /** Returns the angular velocity at the given point on the body. This method
        *   computes only the velocity due to the rotational motion of the aircraft.
        */
        smSpeedVec              getAngularVelAtPointOnBody( const smPositionVec& pointPosInBCS ) const ;

        /** Transforms the given local coordinate system vector to body coordinate system.
        *   This method only rotates the vector and doesn't translate it.
        */
        void                    wcs_to_bcs( const smVec3r& vecInWCS, smVec3r& vecInBCS ) const ;

        /** Transforms the given body coordinate system vector to local coordinate system.
        *   This method only rotates the vector and doesn't translate it.
        */
        void                    bcs_to_wcs( const smVec3r& vecInBCS, smVec3r& vecInWCS ) const ;

        /** Transforms the given local coordinate system vector to body coordinate system.
        *   This method only rotates the vector and doesn't translate it.
        */
        void                    wcs_to_bcs( const smVec3d& vecInWCS, smVec3d& vecInBCS ) const ;

        /** Transforms the given body coordinate system vector to local coordinate system.
        *   This method only rotates the vector and doesn't translate it.
        */
        void                    bcs_to_wcs( const smVec3d& vecInBCS, smVec3d& vecInWCS ) const ;

        /** getAoASideslip computes the angle of attack and the side slip angles as defined
        *   in aerospace engineering literature.
        *
        *   @param  AoA         AoA is usually around 0 degrees. Positive AoA means headwind facing
        *                       the bottom of the aircraft.
        *   @param  sideslip    Sideslip is also usually around 0 degrees. Positive sideslip means headwind
        *                       facing the right side (starboard) of the the aircraft.
		*
		*   @param  wind	    Wind velocity vector in world coordinate system. This velocity is in m/s.
        *
        */
        void                    getAoASideslip( smReal& AoA, smReal& sideslip , const smVec3r& wind ) const  ;

        /** Returns the curvature of the flight path. Throws smException if vel.length() == 0   */
        smReal                  getFlightPathCurvature() const ;
        static smReal           getFlightPathCurvature(const smVec3r& vel, const smVec3r& accel) ;

        /** Returns the instantaneous radius. */
        smReal                  getInstantaneousRadius() const ;
        static smReal           getInstantaneousRadius(const smVec3r& vel, const smVec3r& accel) ;

        /** Returns the instantaneous angular velocity. */
        smVec3r                 getInstantaneousAngularVel() const ;
        static smVec3r          getInstantaneousAngularVel(const smVec3r& vel, const smVec3r& accel) ;

        /** Returns the centrifugal acceleration in local coordinate system. */
        smVec3r                 getCentrifugalAccel() const ;
        static smVec3r          getCentrifugalAccel(const smVec3r& vel, const smVec3r& accel) ;

        /** Returns the Coriolis acceleration in local coordinate system. */
        smVec3r                 getCoriolisAccel() const ;
        static smVec3r          getCoriolisAccel(const smVec3r& vel, const smVec3r& angularVel) ;

        /** Returns the load factor.
        */
        smReal                  getLoadFactor( const smVec3r& gravity ) const ;
    };
}
