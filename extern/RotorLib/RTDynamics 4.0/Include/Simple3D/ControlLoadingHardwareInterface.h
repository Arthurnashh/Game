#pragma once
#include "Simple3D/Simple3D.h"
#include "Physics/smVecTypes.h"

//
// This class abstracts control loading hardware interfaces. It is an emulator that is used for demonstration purposes in the examples.
// The methods of the class correspond to API calls that are common to control loading hardware.
// See also Simple3DControlLoadingHardwareInterface class below.
//
class SIMPLE3D_EXPORT ControlLoadingHardwareInterface
{
	public:
		ControlLoadingHardwareInterface() {}

		// Flight control and Trim positions:
		// There are two different types of position of interest for pilot controls: trim 
		// position and control position. The trim position is the position of the flight control 
		// (e.g. cyclic) with zero restoring force. Control position is the actual physical 
		// position of the flight control.

		// For example: if current trim position of the collective lever is 25 cm from bottom and 
		// the collective is at this position (i.e control pos=trim pos) collective lever would not 
		// move. If the pilot moves the collective away from this position, say 10 cm higher, 
		// control position will be 35 cm. In this case a restoring force will arise and will try 
		// to move the collective back to 25 cm from bottom. Pilot will feel this force when 
		// holding the collective at 35 cm position.
		virtual smReal	getLateralCyclicPosition() const = 0;
		virtual smReal	getLongitudinalCyclicPosition() const = 0;
		virtual smReal	getPedalPosition() const = 0;
		virtual smReal	getCollecivePosition() const = 0;
		virtual smReal	getLateralCyclicTrimPosition() const = 0;
		virtual smReal	getLongitudinalCyclicTrimPosition() const = 0;
		virtual smReal	getPedalTrimPosition() const = 0;
		virtual smReal	getColleciveTrimPosition() const = 0;

		// Trim position setters:
		// Autopilot and SAS systems use a combination of parallel and series actuators to help the 
		// pilot fly the helicopter. Series actuators have a limited authortiy but a fast reaction time. 
		// They work by changing the actual control position that is seen by the rotors. They, however,
		// do not change the control or trim position in the cockpit.
		//
		// Parallel actuators are able to move both the rotor control rods and the cockpit controls. 
		// The action of parallel actuators are obseved by the pilot as a change in trim position.
		//
		// Most of the SAS and autopilot modes use parallel actuators to control the helicopter. To
		// simulate these we need a means of setting the trim positions of flight controls. When
		// the desired trim position is set, control loading hardware actuates the trim motor to reach 
		// this position. It may take some time until the desired trim position is actually reached 
		// hence it is possible that get*TrimPosition returns a different value than the one set with 
		// setDesired*TrimPosition.
		virtual void	setDesiredLateralCyclicTrimPosition( smReal tp ) = 0;
		virtual void	setDesiredLongitudinalCyclicTrimPosition( smReal tp ) = 0;
		virtual void	setDesiredPedalTrimPosition( smReal tp ) = 0;
		virtual void	setDesiredCollectiveTrimPosition( smReal tp ) = 0;

		// Trim release buttons
		// Both collective and cyclic have trim release buttons. Trim release button is used to 
		// lock/unlock the trim position of a control. When the pilot presses the trim release 
		// button, the cyclic gets much lighter (artifical feel system disengages) and could be 
		// moved easily. After the pilot brings the cyclic to the desired position s/he stops 
		// pressing the trim release button and the cyclic remains at that position.
		virtual bool	getCyclicTrimRelease() const = 0;		
		virtual bool	getCollectiveTrimRelease() const = 0;
		virtual bool	getPedalTrimRelease() const = 0;

		// A desired feature in autopilot and SAS systems is giving the pilot a chance to override 
		// SAS and/or autopilot commands without permanently disengaging it and without changing 
		// references (e.g. reference attitude). This is called fly-through handling capability.
		//
		// Helicopter controls usually have a sensor that is used to determine whether the pilot is 
		// applying force to them. If this is the case, SAS/Autopilot enters fly-through mode. This is 
		// implemented in different ways in different SAS/AP modes but the general idea is to prevent 
		// the control system from fighting against the pilot. 
		//
		// For example, attitude hold system tries to hold the helicopter at a certain pitch and roll 
		// attitude. If the pilot applies force to the cyclic for a left bank, attitude hold would enter 
		// fly-through mode and stop applying any force to the cyclic. Once the pilot leaves the cyclic, 
		// attitude hold system brings the helicopter to its previous attitude. Note that, even though 
		// parallel actuator does not apply force, the artifical feel system is still active so the 
		// cyclic will have a rather large restoring force on it.
		virtual bool	getCyclicOutOfDetent() const = 0;		
		virtual bool	getCollectiveOutOfDetent() const = 0;
		virtual bool	getPedalOutOfDetent() const = 0;

		// Beep trim
		// Beep trim is a control hat (like in some joysticks) on cyclic which is used to fine
		// tune the trim position. Pilots can change the trim position slowly using beep trim. 
		// This allows for precision trimming, for example, for achieveing a perfect hover. 
		// Collective also has a similar functionality.
		virtual smVec2r	getBeepTrim() const = 0;
		virtual smVec2r	getCollectiveBeepTrim() const = 0;
		virtual smVec2r	getPedalBeepTrim() const = 0;

		virtual smReal	getLeftToePedal() const = 0;
		virtual smReal	getRightToePedal() const = 0;



};


// Simple3DControlLoadingHardwareInterface class implements an advanced control scheme with mouse and joystick. Due to the 
// limitations of joysticks (e.g. lack of trim locking) a complex control scheme is required for simulating all control modes
// and sensors that are available in real helicopter flight controls.
//
// This class is used for demonstration purposes only. If you would like to integrate RL FDM with a real control loading hardware
// you will need to use the API of the hardware to set/query flight control states and communicate with the RL FDM using a PilotInputData
// structure. RL FDM example application demonstrate how this can be accomplished.
//
// Explanation of the Control Scheme
// ----------------------------------------------------------------------------------------------------------------
// Cyclic:
// Mouse is used for large scale cyclic movements and joystick is used for small scale offsets. The zero restoring
// force position of a joystick is always its center position. This is different than an actual cyclic where the zero restoring force position
// is the trim position and can be changed dynamically. We assume that the center position of the joystick corresponds to trim position
// of the cyclic.
//
// Cyclic trim release button is the left mouse button. If you press left mouse button and move the mouse, both cyclic trim position
// and cyclic position will move. When you release the mouse button, trim position will be locked to the current position. Moving joystick 
// away from center moves the cyclic position (but not the cyclic trim pos!).
//
// Note that, cyclic position and *not* the cyclic trim position is used to set m_rollPitchControl variables.
//
// If joystick is not at the center (i.e. trim pos is not equal to control pos) than pilot must be applying force on the cyclic. When this happens
// we activate the detention sensor of the cyclic. This, in turn, activates fly-through handling mode in the active autopilot mode.
//
// Collective:
// We use a similar method as above for collective. This time, mouse wheel is used to make large scale movement on collective and joystick
// throttle is used for finer adjustments. Middle position of the joystick throttle is assumed to correspond the trim position of collective.
//
// Moving the mouse wheel corresponds to pressing the collective trim release, moving the collective and releasing the trim release. Hence,
// when the mouse wheel is used to adjust collective, collective trim position changes.
//
// Displacing the joystick throttle relative to middle ( i.e. 0.5 ) position corresponds to applying force on the collective without pressing
// the trim release button. This does not change the trim position but only the control position. In a real helicopter, pilot would feel a
// restoring force on the collective. If the throttle is not at the middle we activate collective detention sensor.
//
// Pedals:
// Pedal control position can be changes using Z axis of the joystick. This example does not provide a means of changing pedal trim positions.
// Pedal detention sensor is activated if the joystick Z axis is not centered.
//
// Beep trim:
// POV hat of the joystick is used as beep trim.
//
class SIMPLE3D_EXPORT Simple3DControlLoadingHardwareInterface : public ControlLoadingHardwareInterface
{
public:
	Simple3DControlLoadingHardwareInterface( Simple3D* s3d );

	virtual smReal	getLateralCyclicPosition() const;
	virtual smReal	getLongitudinalCyclicPosition() const;
	virtual smReal	getPedalPosition() const;
	virtual smReal	getCollecivePosition() const;

	virtual smReal	getLateralCyclicTrimPosition() const;
	virtual smReal	getLongitudinalCyclicTrimPosition() const;
	virtual smReal	getPedalTrimPosition() const;
	virtual smReal	getColleciveTrimPosition() const;

	virtual bool	getCyclicTrimRelease() const;
	virtual bool	getCollectiveTrimRelease() const;
	virtual bool	getPedalTrimRelease() const;

	virtual bool	getCyclicOutOfDetent() const;
	virtual bool	getCollectiveOutOfDetent() const;
	virtual bool	getPedalOutOfDetent() const ;

	virtual smVec2r	getBeepTrim() const;
	virtual smVec2r	getCollectiveBeepTrim() const ;
	virtual smVec2r	getPedalBeepTrim() const;

	virtual smReal	getLeftToePedal() const;
	virtual smReal	getRightToePedal() const;

	virtual void	setDesiredLateralCyclicTrimPosition( smReal tp );
	virtual void	setDesiredLongitudinalCyclicTrimPosition( smReal tp ) ;
	virtual void	setDesiredPedalTrimPosition( smReal tp );
	virtual void	setDesiredCollectiveTrimPosition( smReal tp );

	virtual void	update();
	virtual void	acquireState( RTD::PilotInputData* pid );
protected:
	smReal		getJoyX() const;
	smReal		getJoyY() const;
	smReal		getJoyZ() const;
	smReal		getJoyThrottle() const;

    smReal		getPedal() const;

	Simple3D*	m_s3d;
	smReal		m_lateralCyclicTrimPosition;
	smReal		m_longitudinalCyclicTrimPosition;
	smReal		m_pedalTrimPosition;
	smReal		m_collectiveTrimPosition;

};