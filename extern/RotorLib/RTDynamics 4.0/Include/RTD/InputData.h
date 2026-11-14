/*
(c) Copyright 2007, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Base.h"
#include "RTD/ParamDb.h"


namespace RTD
{
	/**
		Base class for vehicle inputs.
	*/
	class RTDYNAMICS_EXPORT InputData : public SimulationVariableUser
	{
	public:
		InputData(const std::string& name , SimulationVariableUser* parent );


		/** Resets the member variables.
		*
		*	Derived classes should override this method to add new inputs as necessary. This method
		*	should be called from within the overriding one.
		*/
		virtual void clear();

		/** Checks the limits of each pilot input and clamps if necessary. This function is internally called
		*	by the dynamics before running the simulation model.
		*
		*	Derived classes should override this method to add new inputs as necessary. This method
		*	should be called from within the overriding one.
		*/
		virtual void checkAndClampToLimits();

		/**
			Copies the contents of \e inputdata to this Input instance.
		*/
		virtual void copy( const InputData* inputdata );

	};


	/**
	*	This class contains pilot inputs.
	*/
	class RTDYNAMICS_EXPORT PilotInputData : public InputData
	{
	public:

		enum
		{
			MAX_ENGINE_COUNT = 16
		};


		/** Constructor of the class. */
		PilotInputData(const std::string& name , SimulationVariableUser* parent);

		/** Destructor of the class. */
		virtual ~PilotInputData();

		virtual void clear();

		virtual void checkAndClampToLimits();

		virtual void copy( const InputData* pid );

		/**
			This input models:
			- Airplane flight stick
			- Airplane flight yoke
			- Helicopter cyclic control

			m_rollPitchControl[0]  : Roll control. Positive values result in clockwise roll (from pilot's point of view).

			m_rollPitchControl[1]  : Pitch control. Positive values result in nose down pitch.

			Valid values are in [-1,1].
		*/
		InputVariableVec2r	m_rollPitchControl;


        /**
            Cyclic beep trim. m_beepTrim[0] is the lateral beep trim and m_beepTrim[1] is the longitudinal one.

            Valid range for both lateral and longitudinal beep trim is [-1,1]. Positive beep trim values indicate 
            right and forward beep trims respectively.
        */
		InputVariableVec2r	m_beepTrim;

		/**
			Airplane rudder pedals or helicopter anti-torque pedals.
			Positive values turn the nose to the right whereas negative values to the left.
			Valid values are in [-1,1].
		*/
		InputVariableReal	m_pedals ;

		/**	Collective input of the helicopter. Valid values are in [0,1]. */
		InputVariableReal	m_collective ;

		/**
			Current pitch trim position. Positive is nose down, negative is up. Valid values are in [-1,1].
		*/
		InputVariableReal	m_pitchTrim;

		/**
			Current roll trim position. Positive values result in clockwise roll (from pilot's point of view).
			Valid values are in [-1,1]
		*/
		InputVariableReal	m_rollTrim;

		/**
			Current yaw trim position. Positive is nose right (from pilots point of view), negative is nose left.
			Valid values are in [-1,1]
		*/
		InputVariableReal	m_yawTrim;

		/** Current collective trim position. Valid values are in [0,1] range. */
		InputVariableReal	m_collectiveTrim;

		/** Pedal detention sensor state. True if pedal detention sensor is activated, i.e. pilot applies force. */
		InputVariableBool	m_pedalOutOfDetent;

		/** Cyclic detention sensor state. True if cyclic detention sensor is activated, i.e. pilot applies force. */
		InputVariableBool	m_cyclicOutOfDetent;

		/** Collective detention sensor state. True if collective detention sensor is activated, i.e. pilot applies force. */
		InputVariableBool	m_collectiveOutOfDetent;

		/** Cyclic trim release button state. True if the cyclic trim release button is pressed. */
		InputVariableBool	m_cyclicTrimRelease;

		/** Collective trim release button state. True if the collective trim release button is pressed. */
		InputVariableBool	m_collectiveTrimRelease;

		/** Pedal trim release button state. True if the pedal trim release button is pressed. */
		InputVariableBool	m_pedalTrimRelease;

		/**
			Throttle state. Valid values are in [0,1]. At most MAX_ENGINE_COUNT throttle values are allowed.
		*/
		InputVariableReal	m_throttle[MAX_ENGINE_COUNT] ;

		/**
            This input is used to control starter motors. Valid values are in [0,1] range, where 0
            means that the starter is off and 1 means that it is fully powered.
        */
		InputVariableReal	m_starter[MAX_ENGINE_COUNT] ;

		/**
            This input is used to control fuel pumps. Valid values are in [0,1] range, where 0
            means that the fuel pump is off and 1 means that it is fully powered.
        */
		InputVariableBool	m_fuelPump[MAX_ENGINE_COUNT] ;


		/**
			Valid values are in [0,1]. At most MAX_ENGINE_COUNT afterburner values are allowed.
		*/
		InputVariableReal	m_afterBurner[MAX_ENGINE_COUNT] ;


		/**
			Wheel brakes for left side.	0 means no brake, 1 means full brake. Values outside these limits are invalid.
		*/
		InputVariableReal	m_wheelBrakesLeft;

		/**
			Wheel brakes for right side. 0 means no brake, 1 means full brake. Values outside these limits are invalid.
		*/
		InputVariableReal	m_wheelBrakesRight;

		/**
			Park brakes. 0 means no brake, 1 means full brake. Values outside these limits are invalid.
		*/
		InputVariableReal	m_parkBrakes;

		/**
			If this variable is true the aircraft will extend the landing gears.
			If it is false landing gears will be retracted.
		*/
		InputVariableBool	m_extendGears;

		/**
			If \e m_spoilers is true the airplane will extend the spoilers.
			If \e m_spoilers is false spoilers will be retracted.
		*/
		InputVariableBool	m_spoilers;

		/**
			0 means airbrakes are fully retracted, 1 means fully deployed. Values between
			0,1 can be used to determine with what percentage airbrakes are extended.
		*/
		InputVariableReal	m_airbrakes;

		/**
			This variable shows which flap configuration is chosen by the pilot.
			0 means flaps are retracted. Increasing values show increasing amount of
			flap extension. What those settings really mean depends on the aircraft, e.g.
			for one aircraft m_flapSetting=1 might mean 2.5 degrees flap extension
			whereas or another 5 degrees. Maximum allowed value depends on the
			aircraft type.
		*/
		InputVariableUInt	m_flapSetting;

		/**
            This input is used to control the jet exhaust nozzle direction (if applicable). 
            Default state is zero degree (nozzle bacwards). 90 degree corresponds to downward
            directed nozzle.
        */
		InputVariableReal	m_nozzleDirection;

        /**
            This input is used to control rotor brakes. Valid values are in [0,1] range.
        */
		InputVariableReal	m_rotorBrake;

		/**
            This input is used to control clutch if available. In case of a helicopter, this variable 
            is used to engage/disengage rotor clutch.

            Valid values are in [0,1] range where 0 means that the clutch is engaged (default state),
            and 1 means that the clutch is disengaged.
        */
		InputVariableReal	m_clutch;

		/**
            This input is used to control emergency buoys for emergency landing on sea. 
        */
		InputVariableBool	m_inflateBuoys;

		/** Enable/disable pitch damping function of SAS. */
		InputVariableBool	m_sasPitchDamper;
        
        /** Enable/disable roll damping function of SAS. */
		InputVariableBool	m_sasRollDamper;

        /** Enable/disable yaw damping function of SAS. */
		InputVariableBool	m_sasYawDamper;

        /** Enable/disable pitch hold function of SAS. */
		InputVariableBool	m_sasPitchHold;

        /** Enable/disable roll hold function of SAS. */
		InputVariableBool	m_sasRollHold;

        /** Enable/disable heading hold function of SAS. 
            \see m_autoPilotHDG */
		InputVariableBool	m_sasHeadingHold;

        /** Enable/disable coordinated turn function of SAS. */
		InputVariableBool	m_sasCoordinatedTurn;
		
		/** Enable/disablle SAS turbulence mode. */
		InputVariableBool	m_sasTurbulenceMode;

		/** This input is the master switch for all autopilot functions. In RTDynamics
            autopilot implementation both SAS and AP is turned on/off with this input. */
		InputVariableBool	m_autoPilotMasterSwitch;

		/**
            Enable/disable HDG (heading acquire and hold) mode of the autopilot.
            \see m_autoPilotHDGHeading
        */
		InputVariableBool	m_autoPilotHDG;

		/**
            Desired heading for HDG mode.
            \see m_autoPilotHDG
        */
		InputVariableReal	m_autoPilotHDGHeading;

		/**
            Enable/disable SPD (air speed acquire and hold) mode of the autopilot.
            \see m_autoPilotSPDSpeedKts
        */
		InputVariableBool	m_autoPilotSPD;

		/**
            Desired speed for SPD (air speed acquire and hold) mode of the autopilot in knots.
            \see m_autoPilotSPD
        */
		InputVariableReal	m_autoPilotSPDSpeedKts;

		/**
            Enable/disable ALT (altitude hold) mode of the autopilot.
        */
		InputVariableBool	m_autoPilotALT;

		/**
            Enable/disable ALT.A (altitude acquire and hold) mode of the autopilot.
            \see m_autoPilotALTAAltitudeFeet
        */
		InputVariableBool	m_autoPilotALTA;

		/**
            Desired altitude for ALT.A (altitude acquire and hold) mode of the autopilot in feet.
            \see m_autoPilotALTA
        */
		InputVariableReal	m_autoPilotALTAAltitudeFeet;

		/**
            Enable/disable V/S (vertical speed hold) mode of the autopilot.
            \see m_autoPilotVSVerticalSpeedFtMin
        */
		InputVariableBool	m_autoPilotVS;

		/**
            Desired vertical speed for V/S (vertical speed hold) mode of the autopilot in feet per minute.
            \see m_autoPilotVSVerticalSpeedFtMin
        */
		InputVariableReal	m_autoPilotVSVerticalSpeedFtMin;

		/**
            Enable/disable HVR (hover) mode of the autopilot. HVR mode is a horizontal mode and does not control
            altitude.

            \see m_autoPilotHHT
        */		
        InputVariableBool	m_autoPilotHover;

		/**
            Enable/disable H.HT (hover height) mode of the autopilot. This is a vertical mode and controls only
            the height above ground.

            \see m_autoPilotHHT
            \see m_autoPilotHoverHeightFeet
        */		
		InputVariableBool	m_autoPilotHHT;

		/**
            Desired height above ground H.HT (hover height) mode of the autopilot in feet.
            \see m_autoPilotHHT
        */
		InputVariableReal	m_autoPilotHoverHeightFeet;

		/**
            Desired cruise height above ground in feet. In RTDynamics autopilot implementation, T.UP
            mode uses this input.
            \see m_autoPilotTUp
        */
		InputVariableReal	m_autoPilotCruiseHeightFeet;

		/**
            Enable/disable T.DWN (transition down or transition to hover) mode.
        */
		InputVariableBool	m_autoPilotTDown;

		/**
            Enable/disable T.UP (transition up or transition to forward flight ) mode.
            \see m_autoPilotCruiseHeightFeet
        */
		InputVariableBool	m_autoPilotTUp;

		/**
            Enable/disable NAV (navigation ) mode.
            \see m_autoPilotNAVRoute
            \see m_autoPilotNAVWPIndex
        */
		InputVariableBool	m_autoPilotNAV;

		/**
            The route to follow in NAV mode. This string has following format:
            WP1x,WP1y;WP2x,WP2y; ... ;WPnx,WPny;

            Where WPx and WPy are world coordinate positions of the waypoints in meters.

            \see m_autoPilotNAV
            \see m_autoPilotNAVWPIndex
        */
		InputVariableString	m_autoPilotNAVRoute;

		/**
            The waypoint towards which the autopilot must steer. This index refers to the waypoints that
            are defined in the m_autoPilotNAVRoute.

            \see m_autoPilotNAV
            \see m_autoPilotNAVRoute
        */
		InputVariableUInt	m_autoPilotNAVWPIndex;



	};

	/**
	*	This class contains driver inputs.
	*/
	class RTDYNAMICS_EXPORT DriverInputData : public InputData
	{
	public:
		/** Constructor of the class. */
		DriverInputData(const std::string& name , SimulationVariableUser* parent);

		/** Destructor of the class. */
		virtual ~DriverInputData();

		virtual void clear();

		virtual void checkAndClampToLimits();

		virtual void copy( const InputData* pid );
		

		InputVariableReal	m_gas;
		
		/**
			Wheel brakes for left side.	0 means no brake, 1 means full brake. Values outside these limits are invalid.
		*/
		InputVariableReal	m_brakeLeft;

		/**
			Wheel brakes for right side. 0 means no brake, 1 means full brake. Values outside these limits are invalid.
		*/
		InputVariableReal	m_brakeRight;

		InputVariableReal	m_clutch;
		InputVariableReal	m_directionControl;
		InputVariableReal	m_parkBrake;
		InputVariableReal	m_gear;

		InputVariableBool	m_engineStart;
		InputVariableBool	m_engineStop;

	};

}