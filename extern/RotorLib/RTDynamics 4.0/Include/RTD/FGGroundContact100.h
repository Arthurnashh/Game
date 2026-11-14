/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/FGGroundContactBase.h"
#include "RTD/Named.h"

namespace RTD
{
	class VehicleBase ;

	/**
	*	This class implements a ground contact model which assumes that the ground is a horizontal
	*	plane. It consists of a spring and a wheel For a higher fidelity ground contact model see
	*	FGGroundContact200.
	*
	*	By defining the length, spring and damping coefficients it is possible to model landing
	*	gears with suspension. With hard and short springs skids can be simulated.
	*
	*	Friction parameters of the wheel is also fully parametric. Two types of friction are
	*	modeled. The first one is sliding friction between the wheel and the ground.
	*	The second one is rolling friction of the wheel in its rotation axle. It is assumed that
	*	the wheel always rotates without sliding. With this assumption, rolling friction acts along
	*	the direction of rolling (which can be modified by using setForwardDirection()) and sliding
	*	friction acts only sidewards to rolling direction. These features enables implementation
	*	of steering by turning the wheel.
	*
	*	Rolling friction can be defined as a range. Actual friction coefficient applied is found
	*	by linear interpolation using brake amount.
	*
	*	Stability of the simulation depends on the spring and friction coefficients. Higher values
	*	require higher update rates (smaller update times) to be stable.
	*
	*/
	class RTDYNAMICS_EXPORT FGGroundContact100 : public RTD::FGGroundContactBase
	{
	public:
		/**
			Configuration parameters of the FGGroundContact100.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public FGGroundContactBase::ConfigParams
		{
		public:
			ConfigParams();
			virtual ~ConfigParams();

			/**
				Maximum load the ground contact can carry (in Newtons).
				See FGGroundContactBase::getMaximumLoad() for detailed information.
			*/
			smReal				maxSpringLoad;

			/**
				Length of the spring in meters.
			*/
			smReal				springLength ;

			/** Radius of the wheel in meters (if exists). If the contact does not have
				a wheel this parameter is ignored. */
			smReal				wheelRadius;


			/**
				Spring constant of the spring in N/m. Higher values mean harder springs. Too high
				a value might result in instability in the simulation.
			*/
			smReal				springConstant ;

			/**
				Damping constant of the spring in N*sec/meter. Higher values reduces oscillations. Too high
				a value might result in instability in the simulation.
			*/
			smReal				dampingConstant ;


			/**
				Static sliding friction coefficients of the wheel.
			*/
			smReal				staticFrictionCoeff;

			/**
				Kinetic sliding friction coefficients of the wheel.
			*/
			smReal				kineticFrictionCoeff;


			/**
				@name Rolling Friction

				Rolling friction ranges of the wheel. In runtime brake amount
				is used to linearly interpolate in these ranges to find actual
				coefficients.
			*/
			//@{
			smReal				minKineticRollingFrictionCoeff;
			smReal				minStaticRollingFrictionCoeff;
			smReal				maxKineticRollingFrictionCoeff;
			smReal				maxStaticRollingFrictionCoeff;
			//@}

			/**
				Position at which the FGGroundContact100 is connected to the vehicle.
			*/
			smVec3r				mountPosition;


			/**
				\e forwardDirection is the direction to which the wheels look initially.
			*/
			smVec3r				forwardDirection;

			/**
				\e mountDirection is the direction at which the ground contact is attached to the fuselage.
				For a landing gear that is perpendicular it needs to be 0,0,1
			*/
			smVec3r				mountDirection;

			/**
				The threshold speed in m/s at which static friction is used instead of
				dynamic friction.
			*/
			smReal				staticFrictionThreshold;


			/**
				If this parameter is set to true the wheel will rotate in the opposite direction of the pilot input (if pilot input is
				to the right wheel will rotate to the left etc. ). This is necessary if the steering wheel is at the back instead of front.
			*/
			bool				steeringInverseRotation;

/**
				
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.

				An example is provided below. The indentation and newlines are for readability
				purposes only. 

				\code
<GroundContact type="FGGroundContact100" >
	<Geometry>			
		<Position x="1.408" y="0.848" z="-0.5"/>
		<Up x="0.0" y="0.0" z="1.0"/>
		<Front x="0" y="1.0" z="0"/>
		<Wheel radius="0.284"/>
	</Geometry>

	<Spring length="0.855" springCoefficient="220000" dampingCoefficient="17800" maxSpringLoad="245000"/>
	<Friction>
		<SlidingFriction static="0.77" kinetic="0.5" thresholdSpeed="0.02"/>				
		<RollingFriction minRollingKinetic="0.1" maxRollingKinetic="0.1"  minRollingStatic="0.2" maxRollingStatic="0.2"/>			
	</Friction>							
</GroundContact>
				\endcode
			*/
			virtual bool	readParamStr( const std::string& parameterString , std::string& error );

		};

	public:
		/**
			Constructor of the class.
			\param parentVehicle	:	The vehicle to which the ground contact is attached.
			\param name				:   Name of the ground contact instance.
			\param params			:	configuration parameters of the ground contact.
		*/
		FGGroundContact100(	VehicleBase* parentVehicle,
								const std::string& name,
								const ConfigParams* params 	) ;

		virtual ~FGGroundContact100();

		/**
			Creates an instance of FGGroundContact100.
			\param vehicle		:	The vehicle to which the ground contact is attached.
			\param name			:   Name of the ground contact instance.
			\param params		:	Configuration parameters of the ground contact.
		*/
		static	FGGroundContact100*	createClass( RTD::VehicleBase* vehicle, const std::string& name , const ConfigParams* params  );

		/**
			Creates a ConfigParams structure which can be used to initialize an FGGroundContact100 instance.
		*/
		static	FGGroundContact100::ConfigParams*	createParameters();

		/** Returns FGGroundContact100 string*/
		virtual std::string		getType() const;

		/** Advances the simulation \e timestep seconds. */
		virtual void update( smTime timestep ) ;

		/** Resets the ground contact. */
		virtual void reset() ;

		/** Returns true if the ground contact is touching to the ground. */
		virtual bool		isTouchingGround() const	{	return c_isTouchingGround;	}

		/** Reports damage status based on the load carried by the suspension spring. If the
			spring force exceeds FGGroundContact100::ConfigParam::maxSpringLoad (also defined in the XML) true is
			returned.*/
		virtual bool		isDamageThresholdExceeded() const ;

		virtual void		setDirection( smReal direction );

		virtual smReal		getDirection() const;

		/**
			Returns the maximum load the ground contact can carry (in Newtons).
			Beyond this value the ground contact will brake. A ground contact implementation
			does not need to do anything in case the maximum load is exceeded. The aircraft 
			to which the ground contact is attached is responsible to query whether maximum
			load is exceeded and respond accordingly.

			Use getSpringForceBCS() to query current load on the spring.
		*/
		smReal		getMaxSpringLoad() const;

		/**
		*	Height of terrain under the vehicle. Currently it is assumed that terrain
		*   has always an upwards normal (0,0,1) hence this method actually defines
		*	a ground plane.
		*/
		virtual void setTerrainHeight( smPosition terZ )	{	m_terrainZ = terZ ; }

		/**
			Sets the forward vector of the FGGroundContact100. Projection of this vector
			to the ground plane is used to determine in which direction the rolling
			friction will be applied.
		*/
		void				setForwardDirection( const smVec3r& fd );

		/**
			Returns the forward vector of the FGGroundContact100.

			@see setForwardDirection
		*/
		const smVec3r&		getForwardDirection() const	{	return m_forwardDirection;	}

		/** Maximum length of the ground contact spring in meters. Maximum length is observed when there is no load
			on the spring.
		*/
		smReal				getMaximumLength() const;

		/**
			When there is load on the spring it is compressed, i.e. shorter than its default size.
			This actual length of the ground contact spring can be retrieved using this method.

			Returned value is in meters.
		*/
		smReal				getCompressedLength();

		/**
			When there is load on the spring it is compressed, i.e. shorter than its default size.
			The amount of compression can be retrieved usign this method.

			Returned value is in meters.
		*/
		smReal				getCompressionLength() const	{	return c_springCompressionLength.val();	}

		/**
			When a moving FGGroundContact100 touches the ground, the contact point (lower end of the spring)
			has a two dimensional velocity vector on the ground plane. The component of this velocity vector
			along forward vector can be retrieved using this method. It is in body coordinate system
			of the attached vehicle.

			@see setForwardDirection()
		*/
		smVec3r				getForwardVelocity() const		{	return c_forwardVelocityBCS;	}


		/**
			Returns the force generated by spring reaction in Newtons. Returned vector is in
			body coordinate system of the connected vehicle.
		*/
		const smVec3r&		getSpringForceBCS() const;

		/**
			Returns the force generated by friction in Newtons. Returned vector is in
			body coordinate system of the connected vehicle.
		*/
		const smVec3r&		getFrictionForceBCS() const;

		virtual smVec3r		getWheelTransOffsetToRestPosition() const;
	
		virtual smVec3r		getWheelRotOffsetToRestPosition() const;

		/** This method returns the position of contact with ground in WCS. */
		virtual smVec3r		getContactPosition() const	{	return c_groundContactPosInWCS;	}

	protected:
		/** Returns m_configParams casted to FGGroundContact100::ConfigParams. */
		ConfigParams*			getDerivedConfigParams();

		/** Returns m_configParams casted to FGGroundContact100::ConfigParams. */
		const ConfigParams*		getDerivedConfigParams() const;

		smVec3r				m_forwardDirection;
		
		smDouble			m_terrainZ ;

		smCoefNegPos		c_springCompressionLength;
		smPositionVec		c_groundContactPosInWCS;

		smForceVec			c_frictionForceBCS;
		smForceVec			c_groundReactionBCS;

		

		bool				c_isInStaticSlidingFrictonMode;
		bool				c_isInStaticRollingFrictonMode;
		bool				c_contactWithWheel;
		bool				c_isTouchingGround;
		smSpeedVec			c_groundContactVelocityWCS;
		smSpeedVec			c_forwardVelocityBCS;
		smReal				c_wheelRotationAngle;

		
		virtual void		computeGeometryAndVelocities(	smVec3r& forwardDirectionWCSOnGroundPlane,
															smVec3r& groundContactVelocityOnGroundPlane,
															smVec3r& prevGroundContactVelocityOnGroundPlane,
															smVec3r& forwardVelocityWCS,
															smDouble& springLength,
															smTime timestep );

		/**
			Computes the force that is exerted on the aircraft (via landing gear) from the ground. 
			This force is affected by the ground contact spring. It is always normal to the ground surface.
			The returned vector is in body coordinate system and is in Newtons.
		*/
		virtual void		computeGroundReaction(  smTime timestep,
													smReal springLength,
													smVec3r& springReactionWCS);

		/**
			Computes the force exerted on the vehicle due to friction (frictionForceWCS). In world
			coordinate system. Returned vector is in Newtons.
		*/
		virtual void		computeFriction(	smTime timestep ,
												const smVec3r& groundContactVelocityOnGroundPlane,
												const smVec3r& prevGroundContactVelocityOnGroundPlane,
												const smVec3r& springReactionWCS,
												const smVec3r& forwardDirectionWCS,
												smVec3r& frictionForceWCS );

		/**
			Computes the amount of total external horizontal force affecting this FGGroundContact100.
			In this context external force means force generated by other force generators,excluding
			other FGGroundContacts, belonging to the same vehicle.
		*/
		virtual void		computeExternalHorizontalForce( smForceVec& horizontalForce );
		
	};
}