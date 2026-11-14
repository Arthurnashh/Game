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

//#define LIMITED_STIFFNESS
//#define SMOOTHED_TERRAIN_INTERACTION

namespace RTD
{
	class VehicleBase ;
	class MBDWheelPrivate;

	/**
		This class implements a ground contact with TMeasy tire model.
	*/
	class RTDYNAMICS_EXPORT FGGroundContactTMEasy : public RTD::FGGroundContactBase
	{
	public:
		/**
			Configuration parameters of the FGGroundContactTMEasy.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public FGGroundContactBase::ConfigParams
		{
		public:
			/** Constructor of the class. */
			ConfigParams();

			virtual ~ConfigParams();
		
			/** The position, in body coordinate system, at which the ground contact is connected to the vehicle.  */
			smVec3r m_mountStation ;

			/** This is the axis, in body coordinate system, along which the suspension will compress and decompress.*/
			smVec3r m_sliderAxis ;

			/** This is the axis, in body coordinate system, around which the wheel will rotate. */
			smVec3r m_rollAxis ;			

			/** Radius of the wheel in meters. */
			smReal	m_wheelRadius ;

			/** Mass of the wheel in kg.*/
			smMass  m_wheelMass ;

			smDouble	m_tire_Cr;
			smDouble	m_tire_FzN;			
			smDouble	m_tire_dFx0;
			smDouble	m_tire_sxM;
			smDouble	m_tire_sxG;
			smDouble	m_tire_FxG;
			smDouble	m_tire_FxM;
			smDouble	m_tire_dFx0_2N;
			smDouble	m_tire_sxM_2N;
			smDouble	m_tire_sxG_2N;
			smDouble	m_tire_FxG_2N;
			smDouble	m_tire_FxM_2N;
			smDouble	m_tire_dFy0;
			smDouble	m_tire_syM;
			smDouble	m_tire_syG;
			smDouble	m_tire_FyG;
			smDouble	m_tire_FyM;
			smDouble	m_tire_dFy0_2N;
			smDouble	m_tire_syM_2N;
			smDouble	m_tire_syG_2N;
			smDouble	m_tire_FyG_2N;
			smDouble	m_tire_FyM_2N;

			smDouble	m_tire_n_over_L0;
			smDouble	m_tire_sy0;
			smDouble	m_tire_n_over_L0_2N;
			smDouble	m_tire_sy0_2N;

			/** Spring constant of the suspension in N/m.*/
			smReal	m_springConstant ;

			/** Damping constant of the suspension in N*sec/meter. */
			smReal	m_damperConstant ;
			
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

			int		m_antiRollBarConnection;

			smReal	m_antiRollBarTransferFactor;

			/**
				During landing the forces acting on the wheel are transferred to the fuselage. Beyond a certain threshold the transferred
				forces result in damage in the vehicle. The load defined in \e m_damageThresholdLoad (in N) is the threshold used
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
				
				In some vehicle the nose or tail gear can rotate freely but the actual steering occurs through differantial braking 
				of main gears. The free rotating wheel then rotates by itself to follow the motion of the vehicle (this is a result
				of dynamics rather than control). In such gear systems it might be necessary to apply a small torque that tries to
				keep the wheel centered.
			*/
			bool	m_steeringKeepCenter;

			/**				
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool	readParamStr( const std::string& parameterString , std::string& error );

		};

	public:
		/**
			Constructor of the class.
			\param attachedVehicle	:	The vehicle to which the ground contact is attached.
			\param name				:   Name of the ground contact instance.
			\param params			:	configuration parameters of the ground contact.
		*/
		FGGroundContactTMEasy(	VehicleBase* attachedVehicle,
								const std::string& name,
								const ConfigParams* params 	) ;

		/**
			Destructor of the class.
		*/
		virtual ~FGGroundContactTMEasy();

		/**
			Creates an instance of FGGroundContactTMEasy.
			\param attachedVehicle		:	The vehicle to which the ground contact is attached.
			\param name					:   Name of the ground contact instance.
			\param params				:	Configuration parameters of the ground contact.
		*/
		static	FGGroundContactTMEasy*	createClass( RTD::VehicleBase* attachedVehicle, const std::string& name , const ConfigParams* params  );

		/**
			Creates a ConfigParams structure which can be used to initialize an FGGroundContactTMEasy instance.
		*/
		static	FGGroundContactTMEasy::ConfigParams*	createParameters();

		virtual void			setAntiRollBarConnection( int arbc );

		int						getAntiRollBarConnection()	const;

		/** Returns FGGroundContactTMEasy string*/
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
		smVec3d				getWheelPositionWCS() const;

		/**
			Returns the position of the mount station in world coordinate system. 
		*/
		smVec3d				getMountPositionWCS() const;
		
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

		virtual smVec3r		getLateralForceAtContact() const;

		virtual smVec3r		getLongitudinalForceAtContact() const;

		/** This method returns the position of contact with ground in WCS. */
		virtual smVec3d		getContactPosition() const;

		/** This method returns the normal vector of the surface at the position of contact with ground. The vector is in WCS. */
		virtual smVec3d		getContactNormal() const;

		/** This method returns the amount of compression of the suspension spring in meters. A negative value means
			that the spring is stretched.*/
		virtual smReal		getSpringCompression() const;

		virtual smDouble	getRotationalInertia() const;

		void				enableLateralForce( bool en )					{	m_enableLateralForce = en;	}
		bool				isLateralForceEnabled( bool en ) const			{	return m_enableLateralForce;	}

		void				enableLongitudinalForce( bool en )				{	m_enableLongitudinalForce = en;	}
		bool				isLongitudinalForceEnabled( bool en ) const		{	return m_enableLongitudinalForce;	}

		void				enableSelfAligningTorque( bool en )				{	m_enableSelfAligningTorque = en;	}
		bool				iselfAligningTorqueEnabled( bool en ) const		{	return m_enableSelfAligningTorque;	}

		void				enableSlipStickModeForce( bool en )				{	m_enableSlipStickModeForce = en;	}
		bool				isSlipStickModeForceEnabled( bool en ) const	{	return m_enableSlipStickModeForce;	}

		smDouble			getLongitudinalSlip() const						{	return c_tireModelOutput.sx;	}
		smDouble			getLateralSlip() const							{	return c_tireModelOutput.sy;	}
		smDouble			getSlipAngle() const							{	return c_tireModelOutput.slipangle;	}

		smDouble			getDynamicRadius() const						{	return c_tireModelOutput.Rd;	}
		smDouble			getContactPatchLength() const					{	return c_tireModelOutput.L;	}
		

		virtual smDouble	getRPM() const;
	protected:
		struct	TireModelOutput
		{
			void reset()
			{
				Vx= 0.0; 
				Vy= 0.0; 
				sx= 0.0; 
				sy= 0.0; 
				Fx= 0.0; 
				Fy= 0.0; 
				Ts= 0.0;  
				L= 0.0;  
				deltaz= 0.0;
				Rd= 0.0;
				slipangle= 0.0;
			}

			smDouble Vx; // longitudinal speed
			smDouble Vy; // lateral slip
			smDouble sx; // longitudinal slip
			smDouble sy; // lateral slip
			smDouble Fx; // longitudinal force 
			smDouble Fy; // lateral force
			smDouble Ts; // self aligning torque 
			smDouble L;  // contact patch length
			smDouble deltaz; // tire deflection
			smDouble Rd;	// tire dynamic radius
			smDouble slipangle; // tire slip angle
		};

		TireModelOutput		c_tireModelOutput;

		bool				m_enableLateralForce;
		bool				m_enableLongitudinalForce;
		bool				m_enableSelfAligningTorque;
		bool				m_enableSlipStickModeForce;

		bool				c_isDamageThresholdExceeded;
		bool				c_isFirstFrameAfterReset;

		smVec3d				c_contactPos;
		smVec3d				c_contactNormal;

		smVec3d				c_wheelLongitudinalDirection;
		smVec3d				c_wheelLateralDirection;
		smVec3d				c_tireLongitudinalForce;
		smVec3d				c_tireLateralForce;
		smVec3d				c_tireSelfAligningTorque;
		smDouble			c_mixingFactor;
		

		bool				c_isTouchingGround;

		/** Returns m_configParams casted to FGGroundContactTMEasy::ConfigParams. */
		ConfigParams*			getDerivedConfigParams();

		/** Returns m_configParams casted to FGGroundContactTMEasy::ConfigParams. */
		const ConfigParams*		getDerivedConfigParams() const;
		
		virtual	void			computeWheelTerrainContactPoint( smDouble P0x , smDouble P0y , smDouble P0z,
																 smDouble P1x , smDouble P1y , smDouble P1z,
																 smDouble nx , smDouble ny , smDouble nz,
																 smDouble& P2x,smDouble& P2y,smDouble& P2z );

		virtual	void			updateSuspension( smTime timestep );
		virtual void			updateTorqueAndRPM( smTime timestep );
		virtual void			updateTerrainContacts( smTime timestep );
		virtual void			updateDamageDetection( smTime timestep );


		virtual bool			updateTireModel( smTime timestep  , const smVec3d& contactPatchPos , 
												 TireModelOutput& tmo );
		
		virtual void			positionRigidBodies();

		virtual void			onExternalPhysicalStateChange();


		virtual smDouble		computeTireForceParams( const smDouble& valAtNominalForce , const smDouble& valAtDoubleNominalForce );
		virtual smDouble		computeTireSlipParams( const smDouble& valAtNominalForce , const smDouble& valAtDoubleNominalForce );

		virtual smDouble		computeTireForce(	const smDouble& slip , 
													const smDouble& dF0,
													const smDouble& sM,
													const smDouble& FM,
													const smDouble& sG,
													const smDouble& FG );


		virtual smDouble		computeTireTorque(	const smDouble& sy , 
													const smDouble& syG,
													const smDouble& Fy,
													const smDouble& Fz);

		MBDWheelPrivate*	m_p;		

		
		unsigned int		m_firstDataLoggerChannelIndex;

#ifdef LIMITED_STIFFNESS
		smDouble			c_wheelAngularPosition;
		smDouble			c_shaftAngularPosition;
#endif

#ifdef SMOOTHED_TERRAIN_INTERACTION
		smVec3d				c_currentHitXYZ;
		smVec3d				c_currentHitNormal;
		bool				c_currentHitsValid;
#endif

		virtual bool			checkTerrainCollision( const smVec3d& wheelPos , smVec3d& hitXYZ ,  smVec3d& hitNormal ,  smVec3d& hitVelocity , smReal& frictionFactor );
	};
}