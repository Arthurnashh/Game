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
#include "RTD/MBDBody.h"

namespace RTD
{
	class VehicleBase ;
	class MBDWheelPrivate;

	/**
		FGGroundContact200 is an advanced multi-body dynamics simulation based ground contact model.
		It supports
		- Inclined surfaces
		- Moving surfaces (e.g. for landing on ship deck)

		It consists of a suspension and a wheel connected to it. 

		Parameters that define the ground contact geometry, suspension behaviour (stiffness, damping),
		wheel friction and braking behavior are fully reconfigurable.
		
	*/
	class RTDYNAMICS_EXPORT FGGroundContact200 : public RTD::FGGroundContactBase
	{
	public:
		/**
			Configuration parameters of the FGGroundContact200.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public FGGroundContactBase::ConfigParams
		{
		public:
			/** Constructor of the class. */
			ConfigParams();
		
			virtual ~ConfigParams();

			/** The position, in body coordinate system, at which the ground contact is connected to the aircraft.  */
			smVec3r m_mountStation ;

			/** This is the axis, in body coordinate system, along which the suspension will compress and decompress.*/
			smVec3r m_sliderAxis ;

			/** This is the axis, in body coordinate system, around which the wheel will rotate. */
			smVec3r m_rollAxis ;			

			/** Radius of the wheel in meters. */
			smReal	m_wheelRadius ;

			/** Mass of the wheel in kg.*/
			smMass  m_wheelMass ;

			/** Spring constant of the suspension in N/m.*/
			smReal	m_springConstant ;

			/** Damping constant of the suspension in N*sec/meter. */
			smReal	m_damperConstant ;
			
			/** Coefficient of friction between the tire and the ground.	*/
			smReal	m_coefOfSlidingFriction ;

			/** 
				An effective resistance torque from linear interpolation of m_resistanceTorqueNoBrake and m_resistanceTorqueFullBrake 
				depending on the amount of applied wheel brake is applied to the wheel continiously to simulate rolling friction.
				The resistance torque acts in a direction that opposes the current rotation of the wheel. In N*m.	*/
			smReal	m_resistanceTorqueNoBrake ;

			/** 
				An effective resistance torque from linear interpolation of m_resistanceTorqueNoBrake and m_resistanceTorqueFullBrake 
				depending on the amount of applied wheel brake is applied to the wheel continiously to simulate rolling friction.
				The resistance torque acts in a direction that opposes the current rotation of the wheel. In N*m.	*/
			smReal	m_resistanceTorqueFullBrake ;

			/**
				Length of the suspension leg in meters. Note that this is \e not the length of the suspension spring. This is the 
				distance from the mount station to the center of the wheel.
			*/
			smReal	m_suspensionLegLength ;

			/** Maximum amount the suspension spring can be compressed. In meters. */
			smReal	m_maxSpringCompression;

			/**
                This parameter is used to adjust the strength of the buoyant force which is generated when the ground contact
                is in water. Buoyant force is modelled as follows

                force = min(depth , m_maxHydrodynamicForceDepth ) * m_buoyancyFactor  

                m_buoyancyFactor is in N/m.

            */
			smReal	m_buoyancyFactor;

	        /**
                This parameter is used to adjust the strength of the hydrodynamic drag force which is produced when the ground
                contact is submerged.

                Hydrodynamic drag is proportional to submersion ratio, speed and m_hydrodynamicDragForceCoeff. Submersion ratio is
                computed with clamp( depth/m_maxHydrodynamicForceDepth , 0.0, 1.0f ).

                m_hydrodynamicDragForceCoeff is in N*s/m .
                
            */
			smReal	m_hydrodynamicDragForceCoeff;

			/**
                This parameter is used to limit the strength of buoyant the force  which is generated when the ground contact
                is in water. Buoyant force is modelled as follows

                force = min(depth , m_maxHydrodynamicForceDepth ) * m_buoyancyFactor  

                m_maxHydrodynamicForceDepth is meters.
            */
            smReal	m_maxHydrodynamicForceDepth;			

			/**
				During landing the forces acting on the wheel are transferred to the fuselage. Beyond a certain threshold the transferred
				forces result in damage in the aircraft. The load defined in \e m_damageThresholdLoad (in N) is the threshold used
				for this purpose.
			*/
			smReal	m_damageThresholdLoad;

			/** The steering controller	rotates the wheel left or right depending on the pilot's input for steering on the ground.
			    This parameter defines how fast the wheel can rotate around the vertical axis. This parameter is in deg/sec.  */
			smReal	m_steeringMaxRotSpeed;

			/** The steering controller	rotates the wheel left or right depending on the pilot's input for steering on the ground.
			    This parameter defines how much torque can be applied to the wheel for this purpose. This parameter is in N*m.*/
			smReal	m_steeringMaxTorque;

			/** The steering controller	rotates the wheel left or right depending on the pilot's input for steering on the ground.
			    The torque applied to the wheel to rotate it is proportional to the difference of pilot's desired wheel direction 
				and current direction multiplied by \e m_steeringControllerGain (the result is clamped at \e m_steeringMaxTorque ).
				This parameter is unitless.
			*/
			smReal	m_steeringControllerGain;

			/**
				If this parameter is set to true the wheel will rotate in the opposite direction of the pilot input (if pilot input is
				to the right wheel will rotate to the left etc. ). This is necessary if the steering wheel is at the back instead of front.
			*/
			bool	m_steeringInverseRotation;

			/**
				If this parameter is set to true the wheel will not be affected by the pilot's control input but instead will try to
				keep the wheel in center always. This is useful to model free rotating wheels (a wheel that can rotate around vertical
				axis but is not actively controlled by actuators ). 
				
				In some aircraft the nose or tail gear can rotate freely but the actual steering occurs through differantial braking 
				of main gears. The free rotating wheel then rotates by itself to follow the motion of the aircraft (this is a result
				of dynamics rather than control). In such gear systems it might be necessary to apply a small torque that tries to
				keep the wheel centered.
			*/
			bool	m_steeringKeepCenter;

			/**				
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.

				An example is provided below. The indentation and newlines are for readability
				purposes only. 

				\code
<GroundContact type="FGGroundContact200"  >
	<Features isBrakable="true" isRetractable="false" isSteerable="false" />
	<Suspension>
		<MountStation x="1.408" y="0.848" z="-0.5"/>
		<SliderAxis x="0" y="0" z="1" /> 
		<SuspensionLength length="0.571" />
		<SpringParameters springConstant="220000" damperConstant="17800"/>
	</Suspension>	
	<Wheel mass="25" radius="0.284">
		<RollAxis x="1" y="0" z="0" />
		<Friction slidingFriction="0.002" resistanceTorqueNoBrake="3000.0" resistanceTorqueFullBrake="3200.0" />
	</Wheel>	
	<Damage thresholdLoad="240000" />									
</GroundContact>
				\endcode
			*/
			virtual bool	readParamStr( const std::string& parameterString , std::string& error );

		};

	public:
		/**
			Constructor of the class.
			\param attachedAircraft	:	The aircraft to which the ground contact is attached.
			\param name				:   Name of the ground contact instance.
			\param params			:	configuration parameters of the ground contact.
		*/
		FGGroundContact200(	VehicleBase* attachedAircraft,
								const std::string& name,
								const ConfigParams* params 	) ;

		/**
			Destructor of the class.
		*/
		virtual ~FGGroundContact200();

		/**
			Creates an instance of FGGroundContact200.
			\param attachedAircraft		:	The aircraft to which the ground contact is attached.
			\param name					:   Name of the ground contact instance.
			\param params				:	Configuration parameters of the ground contact.
		*/
		static	FGGroundContact200*	createClass( RTD::VehicleBase* attachedAircraft, const std::string& name , const ConfigParams* params  );

		/**
			Creates a ConfigParams structure which can be used to initialize an FGGroundContact200 instance.
		*/
		static	FGGroundContact200::ConfigParams*	createParameters();

		/** Returns FGGroundContact200 string*/
		virtual std::string		getType() const;

		/** Advances the simulation \e timestep seconds. */
		virtual void update( smTime timestep ) ;

		/** Resets the ground contact. */
		virtual void reset() ;

		/** Returns true if the ground contact is touching to the ground. */
		virtual bool		isTouchingGround() const;

		/**
			Returns the position of the wheel in world coordinate system.
		*/
		smVec3r				getWheelPositionWCS() const;

		/**
			Returns the position of the mount station in world coordinate system. 
		*/
		smVec3r				getMountPositionWCS() const;
		
		virtual bool		isDamageThresholdExceeded() const ;

		virtual void		setDirection( smReal direction );

		virtual smReal		getDirection() const;
	
		virtual smVec3r		getWheelTransOffsetToRestPosition() const; 
		
		virtual smVec3r		getWheelRotOffsetToRestPosition() const; 

		/** Returns the force vector that acts on the ground contact at the position of contact with terrain. The vector is in WCS
			and its components are in meters.	*/
		virtual smVec3r		getForceAtContact() const;

		/** Returns the force vector component, which is perpendicular to the surface, that acts on the ground contact at the position 
			of contact with terrain. This vector can be interpreted as ground reaction force.
			
			The vector is in WCS and its components are in meters.	*/
		virtual smVec3r		getNormalForceAtContact() const;

		/** Returns the force vector component, which is on the plane of the surface, that acts on the ground contact at the position 
			of contact with terrain. This vector can be interpreted as friction force.
			
			The vector is in WCS and its components are in meters.	*/
		virtual smVec3r		getInplaneForceAtContact() const;

		/** This method returns the position of contact with ground in WCS. */
		virtual smVec3r		getContactPosition() const;

		/** This method returns the normal vector of the surface at the position of contact with ground. The vector is in WCS. */
		virtual smVec3r		getContactNormal() const;

		/** This method returns the amount of compression of the suspension spring in meters. A negative value means
			that the spring is stretched.*/
		virtual smReal		getSpringCompression() const;


		/** Returns m_configParams casted to FGGroundContact200::ConfigParams. */
		ConfigParams*			getDerivedConfigParams();

		/** Returns m_configParams casted to FGGroundContact200::ConfigParams. */
		const ConfigParams*		getDerivedConfigParams() const;
	protected:
		RTD::StateVariableBool	s_isDamageThresholdExceeded;
		RTD::StateVariableBool	s_isFirstFrameAfterReset;
		RTD::StateVariableVec3d	s_wheelPos;
		RTD::StateVariableVec3d	s_mountBodyPos;
		RTD::StateVariableVec3d	s_mountBodyVelocity;
		RTD::StateVariableVec3d	s_mountBodyAngularVelocity;

		RTD::StateVariableVec4d	s_wheelQuat;
		RTD::StateVariableVec4d	s_mountBodyQuat;
		RTD::StateVariableVec3d	s_wheelVelocity;
		RTD::StateVariableVec3d	s_wheelAngularVelocity;

		RTD::StateVariableVec3d	s_fixedJointOffset;
		RTD::StateVariableVec4d	s_fixedJointQuat;

		RTD::StateVariableVec3d	s_jointAnchor1;
		RTD::StateVariableVec3d	s_jointAnchor2;
		RTD::StateVariableVec3d	s_jointAxis1;
		RTD::StateVariableVec3d	s_jointAxis2;
		RTD::StateVariableDouble s_jointC0;
		RTD::StateVariableDouble s_jointS0;
		RTD::StateVariableVec3d	s_jointV1;
		RTD::StateVariableVec3d	s_jointV2;


		RTD::StateVariableVec2d	s_jointSuspensionState;

		RTD::StateVariableDouble s_jointParamsState1;
		RTD::StateVariableDouble s_jointParamsState2;
		RTD::StateVariableDouble s_jointParamsState3;
		RTD::StateVariableDouble s_jointParamsState4;
		RTD::StateVariableDouble s_jointParamsState5;


		smVec3r				c_contactPos;
		smVec3r				c_contactNormal;

		
		virtual	void			computeWheelTerrainContactPoint( smDouble P0x , smDouble P0y , smDouble P0z,
																 smDouble P1x , smDouble P1y , smDouble P1z,
																 smDouble nx , smDouble ny , smDouble nz,
																 smDouble& P2x,smDouble& P2y,smDouble& P2z );

		virtual	void			updateSuspension( smTime timestep );
		virtual void			updateFriction( smTime timestep );
		virtual void			updateTerrainContacts( smTime timestep );
		virtual void			updateDamageDetection( smTime timestep );
		
		virtual void			positionRigidBodies();

		virtual void			onExternalPhysicalStateChange();
		virtual bool			checkTerrainCollision( const smVec3d& wheelPos , smVec3d& hitXYZ ,  smVec3d& hitNormal ,  smVec3d& hitVelocity , smReal& frictionFactor );

		virtual void			applySimVarsToMBD(  bool stateConsistent );
		virtual void			updateSimVarsFromMBD();

		MBDWheelPrivate*	m_p;		

		
	};
}