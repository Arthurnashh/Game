/*
	(c) Copyright 2011, RTDynamics
	All Rights Reserved.

	Product:		RTDynamics
	

	The content of this work is proprietary and confidential.Permission to use,copy,
	publish and modify this software and its documentation for any commercial and
	non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/VehicleBase.h"
#include "RTD/ForceGenerator.h"
#include <Physics/smGraph2D.h>
#include "RTD/ModelComponentBase.h"
#include "RTD/TraitDriveTrainConnection.h"


namespace RTD
{
	class VehicleBase;
	class FGHeliFuselage100;


	/**
		RotorModelBase is the base class for all rotor models.

	*/
	class RTDYNAMICS_EXPORT RotorModelBase : public ModelComponentBase ,public ForceGenerator , public TraitDriveTrainConnection
	{
	public:
		/**
			Configuration parameters of the rotor.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public ModelComponentBase::ConfigParams
		{
		public:

			ConfigParams();
			virtual ~ConfigParams();


            /**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.
			*/
			virtual bool	readParamStr( const std::string& parameterString , std::string& error );

		};

	public:
		/** Constructor of the class.
		*	\param name				: Name of the rotor.
		*   \param vehicle			: The vehicle to which the rotor is attached.
		*	\param parameters		: Configuration parameters of the rotor.
		*/
		RotorModelBase(	const std::string& name ,
								VehicleBase* vehicle,								
								const ConfigParams& parameters );

        /**
            Destructor of the class.
        */
        virtual ~RotorModelBase();

		/**
			Resets the rotor.
		*/
		virtual void	reset();

		/**
			Updates the RotorModelBase by advancing the simulation \e timestep seconds.
		*/
		virtual void update( smTime timestep ) = 0 ;


		/** Get configuration parameters of the rotor model.  */
		virtual const ConfigParams*	getConfigParams() const	{	return m_configParams;	}

		/** Get configuration parameters of the rotor model.  */
		virtual ConfigParams*		getConfigParams()		{	return m_configParams;	}

		/**
			Sets the collective input of the rotor. Collective increaes the the pitch of all rotor blades
			independent of their position.

			Valid values are in [-1,1] range. Values outside this range will be clamped.
		*/
		virtual void		setCollective( const smReal& coll );

		/**
			Returns the collective input of the rotor.
		*/
		virtual smReal		getCollective() const;

		/**
            Sets rotor brake state where 0 means that no brake is applied and 1 means full brake application.
        */
		virtual void		setRotorBrake( const smReal& rb );

		/**
            Returns actual rotor brake state. Returned value is in [0,1] range where 0 means that no brake is applied and 1 means full brake application.
        */
		virtual smReal		getRotorBrake() const;

		/**
			Sets the cyclic input of the rotor. Cyclic input changes the pitch of the blades depending
			on their position around the rotor head. cyclicY increases/decreases the fore and aft blade pitch
			whereas cyclicX left and right.

			Valid values for both arguments are in [-1,1] range and values outside this range will be clamped.
		*/
		virtual void		setCyclic( const smReal& x , const smReal& y );

		/**
			Returns the cyclic inputs.
		*/
		virtual void		getCyclic( smReal& x , smReal& y ) const;
		
		/**
			Returns the lateral cyclic input.
		*/
		virtual smReal		getLateralCyclic() const;

		/**
			Returns the longitudinal cyclic input.
		*/
		virtual smReal		getLongitudinalCyclic() const;
		/**
			Sets the reverse torque factor.

			The torque applied to the rotor shaft rotates the rotor but it also affects the
			helicopter body by trying to turn it in the opposite direction as the rotor.

			Reverse torque factor is used to scale the effect of this reverse torque.
			By setting it to 0 you can completely disable it. Value larger than 1 is also possible.
		*/
		virtual void	setReverseTorqueFactor( smReal reverseTorqueFactor );

		/**
			Returns the reverse torque factor.

			@see setReverseTorqueFactor()
		*/
		virtual smReal	getReverseTorqueFactor() const;

		/**
			Sets the rotational speed (in rolls per minute) of the rotor.
			@see setAngularVelocity
		*/
		virtual void		setRPM( smDouble rpm );

		/**
			Returns the rotational speed (in rolls per minute) of the rotor.
			@see getAngularVelocity
		*/
		virtual smDouble	getRPM() const;
		
		/**
			Sets the angular velocity (in radians) of the rotor.
			@see setRPM
		*/
		virtual void					setAngularVelocity( smReal w );

		/**
			Returns the angular velocity (in radians) of the rotor.
			@see getRPM
		*/
		virtual smReal					getAngularVelocity() const;


		/**
			The rotor faces aerodynamical and frictional forces. These forces generate a torque
			which usually resists the rotational movement of the rotor and must be overcome
			by the engine. Exception to this situation is autorotation in which aerodynamical forces
			indeed rotate the rotor.

			This method returns the total external torque acting on the rotor. Positive external torque
			means that the external forces help the engine to rotate the rotor in the correct direction.
			This happens in case of autorotation. Negative external torque means that the external 
			forces resist the engine. This is the case in usual powered flight.

			Returned value is in N*m .
		*/
		virtual smDouble				getExternalTorque() const;


		/**
			Returns the power which is used by the rotor in Watt. 
			
			If \e averaged is true, not the instantaneous power but an average of recent power values is returned. 
			Forces, moments and hence the instantaneous power of a rotating rotor can change at a large frequency 
			and amplitude. Hence, the averaged power value provides better information about the current status
			of a rotor than instantaneous power does.

			Note: Positive values mean that rotor is consuming the power produced by the engine, whereas negative 
			      values mean that rotor provides power.
		*/
		virtual smReal					getPowerWatt( bool averaged = false ) const;

		/**
			Returns the power which is used by the rotor in HP.

			If \e averaged is true, not the instantaneous power but an average of recent power values is returned. 
			Forces, moments and hence the instantaneous power of a rotating rotor can change at a large frequency 
			and amplitude. Hence, the averaged power value provides better information about the current status
			of a rotor than instantaneous power does.

			Note: Positive values mean that rotor is consuming the power produced by the engine, whereas negative 
			      values mean that rotor provides power.
		*/
		virtual smReal					getPowerHP( bool averaged = false ) const;

		/**
			Returns the induced velocity in m/s. Positive direction is downwards on the rotor disc.
		*/
		virtual smReal					getInducedVelocity() const = 0;

		/** Rotor wake skew angle is the longitudinal angle between rotor wake stream direction and the vertical 
		    axis (the axis perpendicular to the the rotor disc plane). "Longitunal" is used in the sense that 
			lateral component of the wake direction is ignored.
			
			If the rotor wake is parallel to the disc plane and is backwards, skew angle is 90 degrees. If 
			the wake is parallel but points forward than skew angle is -90 degrees. A wake perpendicular
			to the rotor disc plane has 0 degrees skew angle.						
		*/
		virtual smReal					getRotorWakeSkewAngle() const;

		/**
			Rotor coning angle in degrees.
		*/
		virtual smReal					getConingAngle() const;

		/**
			Longitudinal first harmonic flapping in degrees. Positive values mean that rotor is tilted back.
			
			Blade flapping eguation is as follows:

			Beta = a0 - a1*cos(psi) - b1*sin(psi)

			Where beta is flapping, a0 is coning angle and psi is the azimuth of the blade. Azimuth is zero 
			downstream. For a rotor which has its advancing blade on the right side, azimuth is 90 degrees 
			when the blade is on the right.
		*/
		virtual smReal					get_a1() const;

		/**
			Lateral first harmonic flapping in degrees. For a rotor which has its advancing blade on the right
			side, positive b1 denotes that the rotor is tilted to the right.

			Blade flapping eguation can be build as follows:

			Beta = a0 - a1*cos(psi) - b1*sin(psi)

			Where beta is flapping, a0 is coning angle and psi is the azimuth of the blade. Azimuth is zero 
			downstream. For a rotor which has its advancing blade on the right side, azimuth is 90 degrees 
			when the blade is on the right.
		*/
		virtual smReal					get_b1() const;

		/**
			First harmonic coefficient of lateral blade feathering. This affects the lateral tilt of the
			rotor.

			Feathering equation can be build as follows:

			Theta = Theta0 - A1*cos(psi) - B1*sin(psi)

			Where theta is blade pitch (fethering), Theta0 is collective pitch angle and psi is the azimuth 
			of the blade. Azimuth is zero downstream. For a rotor which has its advancing blade on the 
			right side, azimuth is 90 degrees when the blade is on the right.
		*/
		virtual smReal					get_A1() const;

		/**
			First harmonic coefficient of longitudinal blade feathering.

			Feathering equation can be build as follows:

			Theta = Theta0 - A1*cos(psi) - B1*sin(psi)

			Where theta is blade pitch (fethering),Theta0 is collective pitch angle and psi is the azimuth 
			of the blade. Azimuth is zero downstream. For a rotor which has its advancing blade on the 
			right side, azimuth is 90 degrees when the blade is on the right.
		*/
		virtual smReal					get_B1() const;

		/**
			Collective pitch of the blades.

			Feathering equation can be build as follows:

			Theta = Theta0 - A1*cos(psi) - B1*sin(psi)

			Where theta is blade pitch (fethering),Theta0 is collective pitch angle and psi is the azimuth 
			of the blade. Azimuth is zero downstream. For a rotor which has its advancing blade on the 
			right side, azimuth is 90 degrees when the blade is on the right.
		*/
		virtual smReal			getTheta0();


		/**
            Returns the design RPM of the rotor. In case the rotor is not attached to an engine, it will rotate
            with this RPM. When an engine is available in the model and the rotor is connected to it with a drive
            train, RPM of the rotor will not be constant and will be determined with the aerodynamic loads and 
            available engine power.
        */
		virtual smReal			getDesignRPM() const;

		/**
            Returns the rotational inertia (kg*m^2) of the entire rotor around the shaft axis.
        */
		virtual smDouble		getRotationalInertia() const;

        /**
            This method can be used to set \e interferingRotor as a source of aerodynamic interference
            for this rotor. This method is used, for example, for tail rotors which operate in the downwash of
            the main rotors.
        */
        virtual void			setInterferingRotor( RotorModelBase* interferingRotor );

        /**
            This method can be used to set \e fuselage as a source of aerodynamic interference
            for this rotor. This method is used, for example, for tail rotors where the dynamic pressure is reduced
            due to the flow around the fuselage.
        */
		virtual void			setFuselage( FGHeliFuselage100 *fuselage );
		
		/** Returns thrust force in Newtons. Thrust is perpendicular to the disc plane (not tip path plane),
			i.e. relative to helicopter its direction is constant. */
		virtual smReal			getT() const = 0;

		/** Drag force of the rotor in Newtons. Drag vector is on the disc plane and positive backwards (towards
			the tail). */
		virtual smReal			getH() const = 0;

		/** Side force of the rotor in Newtons. Side force vector is on the disc plane and positive towards right. */
		virtual smReal			getY() const = 0;

	protected:

		StateVariableReal			s_reverseTorqueFactor;
		

		InputVariableReal			i_collective;
		InputVariableReal			i_cyclicX;
		InputVariableReal			i_cyclicY;
		InputVariableReal			i_rotorBrakeInput;

		StateVariableReal			s_rotorRPM ;

		StateVariableDouble			s_coningAngleRad;
		StateVariableDouble			s_a1; // longitudinal flapping angle in radians, w.r.t shaft (rotor local coordinates)
		StateVariableDouble			s_b1; // lateral flapping angle in radians, w.r.t shaft (rotor local coordinates)

		StateVariableDoubleDataHistory	s_powerHistory;

		OutputVariableDouble		o_A1; // in radians
		OutputVariableDouble		o_B1; // in radians
		OutputVariableDouble		o_theta0; // in radians

		OutputVariableReal			o_dynamicPressureRatio;

		OutputVariableReal			o_externalTorque;		
		OutputVariableReal			o_rotorWakeSkewAngle;

		OutputVariableReal			o_power;
		OutputVariableReal			o_averagePower;


		ConfigParams*	            m_configParams;
		
		RotorModelBase*             m_interferingRotor;

		FGHeliFuselage100*	        m_fuselage;

		/*
			This vector shows the direction of x axis in vehicle coordinate system.
			See ForceGenerator::m_mountDirection, which is the direction of up. m_forwardDirection and
			m_mountDirection should be perpendicular to each other and they should have
			unit length.
		*/
		smVec3r			m_sideDirection;

		smVec3r			m_forwardDirection;
		smVec3r			m_upDirection;

		smMatrix		m_coordMatrix;
		smMatrix		m_invCoordMatrix;
		

		/**
			Currently mount position and direction can be given only during initialization.
			Changing them in runtime is not supported hence this method is declared protected.
		*/
		virtual void	setMountPosition( const smPositionVec& mountXYZ ) ;

		/**
			Currently mount position and direction can be given only during initialization.
			Changing them in runtime is not supported hence this method is declared protected.
		*/
		virtual void	setMountPosition( smReal x, smReal y, smReal z ) ;

		/**
			Currently mount position and direction can be given only during initialization.
			Changing them in runtime is not supported hence this method is declared protected.
		*/
		virtual void	setMountDirection( const smVec3r& mountDirVec ) ;

		virtual void	updateCoordMatrix();

		smReal	readValueFromCurves( smReal a1 , smReal skew,
									 smReal low, smReal medium , smReal high,
									 const smGraph2D& curveLow,const smGraph2D& curveMed, const smGraph2D& curveHigh );


        virtual smReal  computeControlValue( const smReal collective, const smReal latCyclic, const smReal lonCyclic , const smReal pedals,
                                             const smReal fromCollective, const smReal fromLatCyclic, const smReal fromLonCyclic , const smReal fromPedals , const smReal offset ) const;

		virtual	void	printInfo();

	};
}