/*
(c) Copyright 2006, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/ForceGenerator.h"
#include <Physics/smGraph2D.h>
#include "RTD/ModelComponentBase.h"
#include "RTD/TraitDriveTrainConnection.h"

namespace RTD
{
	class VehicleBase;
	/**
		Ground contacts compute the effects of interactions between an object and terrain. Different landing gear types and
		tire models	can be simulated using ground contacts. 
			
		To create a new ground contact model developers have to:
		- Derive their own class from ground FGGroundContactBase.
		- Override getType() method to return the name of the new class (without namespace, just the class name).
		- Derive their own class from ground FGGroundContactBase::ConfigParams within the new ground contact class.
		- Write creator methods for the ground contact and ground contact parameters class( see registerClass() ).
		- Register the ground contact class using registerClass().
		- Override the update() method to implement the ground contact model.
		- Implement several pure virtual methods ( such as isDamageThresholdExceeded() ) to finalize integration with
		  the system.
		
		As a force generator, the derived class should write the computed force and moment to o_forceInWorldCS 
		and o_momentAtCogInBodyCS.

	*/
	class RTDYNAMICS_EXPORT FGGroundContactBase : public ModelComponentBase , public ForceGenerator , public TraitDriveTrainConnection
	{
	public:
	
		/**
			Base class for defining parameters of ground contact model implementations. A new 
			ground contact implementation should define its own ConfigParams derived from this one.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public ModelComponentBase::ConfigParams
		{
		public:
			/** Constructor of the class. */
			ConfigParams();

			virtual ~ConfigParams();

			/** If set to true the ground contact will receive steering control inputs( setDirection() method
				will be called every frame ). */
			bool				m_isSteerable;


			/** If set to true the ground contact will be retractable, i.e. it will respond to retract/extend gears
				control input. When retracted a ground contact is disabled. Moreover, retractable gears also exert
				air drag on the vehicle depending on the amount of extension.
			*/
			bool				m_isRetractable;

			/** If set to true the ground contact will receive brake control inputs ( setBrake() will be called every
				frame ). */
			bool				m_isBrakable;

			/**
                Equivalent flat plate area in m^2. This value is used in computing the air drag due to the ground contact.
            */
			smReal				m_equiuvalentFlatPlateArea;

			/**
                This parameter determines whether this ground contact is an inflatable buoy or not. Inflatable buoys can 
                provide buoyance when landed on water. This parameter is false by default.
            */
			bool				m_isInflatableBuoy;

			/**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.

				Derived classes should override this method to read parameters from a stream. Usually
				the vehicle which creates the class will read the stream from a file and hand to this
				method. Derived classes should call the default implementation so that \e m_isSteerable,
				\e m_isRetractable etc. are read correctly. 
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

        /**
            This enum defines possible malfunctions of the ground contacts.

            \see setMalfunctionState()
            \see getMalfunctionState()
        */
		enum Malfunction
		{
			FUNCTIONAL = 0,
				STUCK = 1,				
		};


		/**
			Constructor of the class.
			\param attachedVehicle	The vehicle to which the ground contact will be attached.
			\param name				The name of the ground contact instance.
			\param parameters		Configuration parameters of the ground contact model.
		*/
		FGGroundContactBase( RTD::VehicleBase* attachedVehicle, const std::string& name, const ConfigParams* parameters  );

		/**
			Destructor of the class.
		*/
		virtual ~FGGroundContactBase();

		/** Updates the ground contact model by advancing the simulation \e stepTime seconds. */
		virtual void			update( smTime stepTime );

		/** Resets the model. */
		virtual void			reset();		

		/** Returns true if the ground contact is steerable. */
		virtual bool			isSteerable() const;

		/** Returns true if the ground contact is retractable. */
		virtual bool			isRetractable() const;

		/** Returns true if the ground contact is brakable. */
		virtual bool			isBrakable() const;

		/** Returns true if the ground contact is defined as an inflatable buoy. If this the case, the ground contact
			will be enabled/disabled with m_inflateBuoys pilot input. */
		virtual bool			isInflatableBuoy() const;

		/** Returns true if the ground contact (i.e. the wheel) is connected to the power train.
		    if this is the case, the transmission which is connected to the ground contact will
			deliver torque to the wheel. */
		virtual bool			isPowered() const;

		/** Enables/disables transmission connection. This method is called internally by the transmission
		   to inform the ground contact that it has a power train connection. Developers should and need not
		   call this method unless they are developing a new transmission model.
		   @see isPowered() */
		virtual void			setPowered(bool powered);

		/**
			Returns true if the ground contact is touching to the ground.
		*/
		virtual bool		isTouchingGround() const = 0;

		/**
			Ground contacts transfer the forces from the ground to the vehicle body. If these
			forces exceed certain thresholds the vehicle will be damaged. Whether the transferred
			forces will result in a damage or not is decided by the ground contact itself. If damage
			should occur, true should be returned. 

			TraitGroundContact calls isDamageThresholdExceeded() every frame to check whether vehicle 
			is damaged or not. A "true" return value means that vehicle should receive damage. The 
			value returned from the method is computed every frame so if in a frame damage is reported 
			(return value true) in the next one the method can return false.		

			Note that damage as defined here will affect the operation of the vehicle as a whole
			and \b not only the ground contact itself. It is up to the implementor whether a separate
			damage model for the ground contact itself is to be implemented.
		*/
		virtual bool		isDamageThresholdExceeded() const = 0;

		/**
			Enable/disable the ground contact. When disabled it will not produce
			any force.
		*/
		void				enable( bool e )				{	s_isEnabled	=	e;	}

		/**
			Returns true if enabled. False otherwise.
		*/
		bool				isEnabled() const				{	return s_isEnabled;	}


		/** Returns a vector containing x,y,z offsets (in meters) of the wheel (if a wheel exists) to the default wheel position. 
			Returned vector is in body coordinate system and to be interpreted in RTDynamics coordinate system convention
			(Y forward, X rightward, Z upward).

			This method is useful for animation of the landing gears. */
		virtual smVec3r		getWheelTransOffsetToRestPosition() const = 0; 
	
		/** Returns a vector containing heading, pitch and roll offsets (in degrees) of the wheel (if a wheel exists) to the default wheel orientation. 
			Returned vector is in body coordinate system and to be interpreted in RTDynamics coordinate system convention
			(Y forward, X rightward, Z upward, heading positive in counter clockwise around Z, pitch positive counter clockwise around
			X and roll positive counter clocwise around the Y).

			This method is useful for animation of the landing gears. 
		*/
		virtual smVec3r		getWheelRotOffsetToRestPosition() const = 0; 

		/**
			Sets the direction of the wheel (if ground contact is a steerable gear). \e direction 
			is in degrees. 0 means forward looking wheels (in this case wheel axis is sidewards). Positive
			values turn the wheel to left and negative ones to right.
			
			This method is called by TraitGroundContact	to turn the wheels depending on the user input. 
			
		*/
		virtual void		setDirection( smReal direction ) = 0;


		/**
			Returns the direction of the wheel. 

			@see setDirection
		*/
		virtual smReal		getDirection() const = 0;

		/**
			Sets the amount of brake applied to the wheel (if exists). Valid range is [0,1] and 
			values outside this range will be clamped. 0 means that brakes are fully released 
			and 1 means fully applied.
		*/
		void				setBrake( smReal b );

		/**
			Returns the amount of brake applied to the wheel.

			@see setBrake
		*/
		smReal				getBrake() const;

        /** Set the RPM of the wheel if available. */
		virtual void		setRPM( smDouble rpm );

        /** Returns the wheel RPM. */
		virtual smDouble	getRPM() const;

		/** Returns the torque delivered by the transmission to the wheel. Returned torque is in N*m. */
		virtual smDouble	getDeliveredTorque() const;
		

		/** getExternalTorque() returns the torque acting on the wheel due to external effects such as
			traction (If the wheel is touching to the terrain and has a non-zero velocity a friction
			force will be exerted on the wheel. This force, in turn, has a rotating moment.)
			
			The returned torque is around the rotation axis of the wheel (other components, if any, are ignored)
			and is in N*m. It is positive if the wheel is rotating in the direction which would make the vehicle
			go forward.
		*/
		virtual smDouble	getExternalTorque() const;

		/**
            Returns the current malfunction state of the ground contact instance. The returned value is a bitwise
            ORed combination of the values defined in the Malfunction enum.
            \see FGGroundContactBase::Malfunction
        */
		unsigned int		getMalfunctionState() const ;

		/**
            Sets the current malfunction state of the ground contact instance. The input argument is a bitwise
            ORed combination of the values defined in the Malfunction enum.
            \see FGGroundContactBase::Malfunction
        */
		virtual void		setMalfunctionState( unsigned int mfbits )  ;

	    /**
		    Sets the gear extension amount (for retractable gears). Valid values are in [0,1] range.
		    0 Means gears are fully retracted. 1 Means gears are fully extended.
	    */
		virtual void		setExtensionAmount( smReal extensionAmount );

	    /**
		    Returns the gear extension amount (for retractable gears). Valid values are in [0,1] range.
		    0 Means gears are fully retracted. 1 Means gears are fully extended.
	    */
		virtual smReal		getExtensionAmount() const;

		/** Returns the rotational inertia of the wheel (if there is any) in Kg*m^2. Default implementation returns 0.*/
		virtual smDouble	getRotationalInertia() const;

		/** Get configuration parameters of the FGGroundContactBase.  */
		const ConfigParams*	getConfigParams() const	{	return m_configParams;	}

		/** Get configuration parameters of the FGGroundContactBase.  */
		ConfigParams*		getConfigParams()		{	return m_configParams;	}



	protected:	
		ConfigParams*		m_configParams ;

		unsigned int		m_firstDataLoggerChannelIndex;
		smReal				m_brakeAmount;
		smDouble			m_externalTorque;
		smDouble			m_rpm;
		smDouble			m_deliveredTorque;
		
		bool				m_isPowered;
		
		RTD::StateVariableBool		s_isEnabled;
		RTD::StateVariableUInt		s_malfunctionState;
		RTD::StateVariableReal		s_extensionAmount;

		virtual unsigned int	writeLog();
	};


} 