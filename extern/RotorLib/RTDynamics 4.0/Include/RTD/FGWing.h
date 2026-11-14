/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include <Physics/smGraph2D.h>
#include "RTD/VehicleBase.h"
#include "RTD/ForceGenerator.h"
#include "RTD/RigidBodyState.h"

namespace RTD
{
	/**
	*	FGWing is a specialized force generator which computes lift and drag force
	*	using
	* 	- Surface Area
	*	- Aerodynamic drag and lift coefficients
	*	- Angle of attack
	*	- Air speed
	*	- Air density
	*	- Distance to ground ( ground effect )
	*/
	class RTDYNAMICS_EXPORT	 FGWing : public ForceGenerator
	{
	public:
		/** Configuration parameters of FGWing */
		class RTDYNAMICS_EXPORT ConfigParams
		{
		public:
			ConfigParams();
			virtual ~ConfigParams();

			/** Position of the aerodynamic center of the wing. */
			smVec3r					position;

			/**
				A vector which determines up direction of the wing in airplane's
				body coordinate system.
			*/
			smVec3r					up;

			/**
				A vector which determines the direction of the front in airplane's
				body coordinate system.
			*/
			smVec3r					front;

			/**
				Surface area of the wing. This affects the lift produced by the wing.
			*/
			smReal					surfaceArea;

			/**
				This is a series of curves of lift coefficient vs. angle of attack.
				Depending on wing's current configuration one of the curves can be
				activated ( see setActiveCurve() ). Flaps can be implemented using
				multiple lift curves. Each flap configuration can have a corresponding
				lift curve.
			*/
			std::vector<smGraph2D>	liftCoeffCurves;

			/**
				This is a series of curves of drag coefficient vs. angle of attack.
				Depending on wing's current configuration one of the curves can be
				activated ( see setActiveCurve() ). Flaps can be implemented using
				multiple drag curves. Each flap configuration can have a corresponding
				drag curve.
			*/
			std::vector<smGraph2D>	dragCoeffCurves;

			/**
				This variable is used to adjust the effect of the control surface attached
				to the wing. Setting it to zero disables the control surface.
			*/
			smReal					controlSurfaceLiftCoeffDerivative;

			/**
				Maximum deflection angle of the control surface in degrees. It is the upper travel limit
				of the control surface.
				
				Note: Positive deflection is defined as downward deflection	of the trailing edge.
			*/
			smReal					maxControlSurfaceDeflection;

			/**
				Minimum deflection angle of the control surface in degrees. It is the lower travel limit
				of the control surface. 
				
				Note: Positive deflection is defined as downward deflection of the trailing edge.
			*/
			smReal					minControlSurfaceDeflection;

			/**
				The maximum speed at which the deflection angle of the control surface can change. This
				parameter is in deg/s. It must be a positive value.
			*/
			smReal					controlSurfaceDeflectionRateLimit;

			/**
				Control surfaces on the wings are used to create user controllable forces
				to fly the airplane. This parameter can be used to define where on the wing
				will this force be applied. It defines an offset from areodynamic center posiiton
				in sidewards direction which is equal to  front X up.
			*/
			smReal					controlSurfaceForceArmPos;

			/**
				This is the distance (from ground to the aerodynamic center of the wing, in meters)
				at which ground effect starts to show its effect.
			*/
			smReal					groundEffectStartDistance;

			/**
				This variable is used to adjust the effect of ground effect. The higher
				it is the more will the lift increase with decreasing distance to ground.
				This is a unitless number.
			*/
			smReal					groundEffectStrength;

			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/**
			Constructor of the FGWing class.
		*/
		FGWing( const std::string& name ,
			VehicleBase* vehicle,
			const ConfigParams& params ) ;

		virtual ~FGWing();

		virtual void update( smTime timestep ) ;

		virtual void    setNormalizedControlSurfaceDeflection( smReal def )	{	i_normalizedControlSurfaceDeflection = smMath::clamp( def , -1.0f , 1.0f );	}
		smReal			getNormalizedControlSurfaceDeflection() const		{	return i_normalizedControlSurfaceDeflection;	}

		/** This method returns the deflection of the control surface in degrees. Deflection is positive when the trailing edge
			of the control surface is down (i.e. in the opposite direction of the "up" vector which is defined in the configuration). 
		*/
		smReal			getControlSurfaceDeflection() const		{	return s_controlSurfaceDeflection;	}

		smForceVec		getCurrentLift() const		{	return o_liftForce;	}
		smForceVec		getCurrentDrag() const		{	return o_dragForce;	}
		smTorqueVec		getCurrentTorque() const	{	return o_liftTorque;	}

		smPositionVec	getAerodynamicCenterWCS() const;

		smForceVec		getCurrentControlSurfaceLift() const		{	return o_controlSurfaceLiftForce;	}
		smTorqueVec		getCurrentControlSurfacTorque() const		{	return o_controlSurfaceLiftTorque;	}

		unsigned int	getCurveCount() const;
		smReal			getActiveCurve() const;
		void			setActiveCurve( smReal ac );

		smReal			getAoA() const									{return o_AoA;	}


		void			setInterferenceVelocity( const smVec3r& iv )	{	i_interferenceVelocity = iv;	}
		const smVec3r&	getInterferenceVelocity() const					{	return i_interferenceVelocity.getValue();  }

		void			setDynamicPressureRatio( smReal dpr )			{	i_dynamicPressureRatio = dpr;	}
		smReal			getDynamicPressureRatio() const					{	return i_dynamicPressureRatio;  }

		smReal			getLiftCoeff() const							{	return o_Clift;	}
		smReal			getDragCoeff() const							{	return o_Cdrag;	}

		virtual void	reset();

		ConfigParams*	getConfigParams()			{	return m_configParams;	}
		const ConfigParams*	getConfigParams() const	{	return m_configParams;	}

	protected:
		VehicleBase*			m_vehicle;
		ConfigParams*			m_configParams;

		InputVariableReal		i_normalizedControlSurfaceDeflection;
		InputVariableReal		i_dynamicPressureRatio;
		InputVariableVec3r		i_interferenceVelocity;

		StateVariableReal		s_activeCurveIndex;
		StateVariableReal		s_controlSurfaceDeflection;

		OutputVariableVec3r		o_liftForce;
		OutputVariableVec3r		o_liftTorque;

		OutputVariableVec3r		o_dragForce;
		OutputVariableVec3r		o_dragTorque;

		OutputVariableVec3r		o_controlSurfaceLiftForce;
		OutputVariableVec3r		o_controlSurfaceLiftTorque;

		OutputVariableReal		o_Clift;
		OutputVariableReal		o_Cdrag;

		OutputVariableReal		o_AoA;

		
		virtual void			updateLiftAndDragCoeffs();
		virtual smReal			readFromCurve( const smGraph2D& curve , smReal aoa );

	};

}
