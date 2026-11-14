/*
(c) Copyright 2007, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "RTD/ModelComponentBase.h"
#include "RTD/InputData.h"

namespace RTD
{
	class VehicleBase;
	/**
		ControlSystemComponentBase class provides a base class for control system component implementations. Control systems
		are used for different purposes such as auto pilot, stability augmentation and driver asistant systems. 		

		To create a new control system component developers have to:
		- Derive their own class from ControlSystemComponentBase.
		- Override getType() method to return the name of the new class (without namespace, just the class name).
		- Derive their own parameters class from ControlSystemComponentBase::ConfigParams within the new class.
		- Write creator methods for the control system component and control system component parameters class( see registerClass() ).
		- Register the control system component class using registerClass().
		- Override the update() method to implement the control system component model.

		Control systems consistsof multiple components which are connected to each other in cascade fashion. The first 
		component receives the pilot input directly. It processes the pilot/driver input and outputs a modified 
		input which is received by the next component in the cascade. If a component is disabled it should pass 
		its input to the output without any change. The output of the last component is applied to the vehicle.
	*/
	class RTDYNAMICS_EXPORT ControlSystemComponentBase : public ModelComponentBase
	{
	public:

		/**
			Base class for defining parameters of control system component implementations. A new control system component implementation
			should define its own ConfigParams derived from this one.
		*/
		class RTDYNAMICS_EXPORT ConfigParams : public ModelComponentBase::ConfigParams
		{
		public:

			/**
				A utility method that parses a character stream which contains initialization
				parameters. The \e parameterString is in XML. If there is an error false is returned
				and \e error will contain the reason.

				Derived classes should override this method to read parameters from a stream. Usually
				the vehicle which creates the control system component will read the stream from a file and hand to this
				method.
			*/
			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};


		/**
			Constructor of the class.
			\param vehicle		The vehicle to which the control system component will be attached.
			\param name			The name of the control system component instance.
			\param parameters	Parameters of the control system component model.
		*/
		ControlSystemComponentBase( RTD::VehicleBase* vehicle, const std::string& name, const ConfigParams* parameters  );

		/**
			Destructor of the class.
		*/
		virtual ~ControlSystemComponentBase();



        /** Updates the control system component by advancing the simulation \e stepTime
            seconds. Default implementation is empty.

			See FCSRotationDamper::update() documentation for an example implementation.
		*/
		virtual void			update( smTime stepTime  );

		/** Resets the control system component.*/
		virtual void			reset();

        /** Enables/disables the control system component. A disabled component passes
         *  its input directly to its output.
		 *
		 *	Note that, being enabled does not mean that a controller has partial or complete
		 *	control authority. Whether a control system component is actually controlling
		 *  the vehicle also depends on other factors, such as auto pilot states. 
		 *
		 *  Example: If attitude hold controller is disabled, it will simply pass all its
		 *  inputs to the next controller in the cascade without any changes. If it is
		 *  enabled but autopilot master switch or ATT switches are off ( see PilotInputData class )
		 *  the result will be the same. Attitude hold will actually work only if the controller is
		 *  enabled and all related cockpit switches are on.
		 *
		 *	Disabling a control system component makes it practically non-existent. When enabled,
		 *  control system components will be watching cockpit inputs to activate/deactivate themselves.
         */
		virtual void			enable( bool en );

		/** Returns true if the control system component is enabled. Note that, being enabled does not mean that 
		 *	a controller has partial or complete control authority. Whether a control system component is actually 
		 *  controlling the vehicle also depends on other factors, such as auto pilot states. See enable() method for
		 *  more detail.
		*/
		bool					isEnabled();

		/**
			Returns the current output.
		*/
		InputData*				getControlOutput()			{	return m_controlOutput;	}

		/**
			Returns the current output.
		*/
		const InputData*		getControlOutput() const	{	return m_controlOutput;	}

		/**
			Sets the control system component that comes before this one in the cascade.
		*/
		virtual void			setPreviousStage( ControlSystemComponentBase* ps )	{	m_previousStage = ps;	}

		/**
			Returns the previous stage in the cascade.
		*/
		ControlSystemComponentBase*		getPreviousStage()			{	return m_previousStage;	}

		/**
			Returns the previous stage in the cascade.
		*/
		const ControlSystemComponentBase*	getPreviousStage() const	{	return m_previousStage;	}


		/**
			Sets the control system component that comes before this one in the cascade.
		*/
		virtual void			setNextStage( ControlSystemComponentBase* ps )	{	m_nextStage = ps;	}

		/**
			Returns the next stage in the cascade.
		*/
		ControlSystemComponentBase*		getNextStage()			{	return m_nextStage;	}

		/**
			Returns the next stage in the cascade.
		*/
		const ControlSystemComponentBase*	getNextStage() const	{	return m_nextStage;	}

		/**
            Sometimes it is desired that a control system component does not read certain inputs or compute internal variables itself but gets them 
            from an external source. In this case, enableSlaveMode() method is used to inform the component that its inputs will be driven externally
            and it should not overwrite the variables with its own values.
            
            For example, FCSHelicopterVelocityController normally reads the desired speed from the m_autoPilotSPDSpeedKts pilot input.
            However, when FCSHelicopterHoverController is activated, it calls enableSlaveMode() on FCSHelicopterVelocityController, 
            disables the read from m_autoPilotSPDSpeedKts and sets the desired speed itself.

            The slave mode is automatically disabled after one simulation step. If a controller drives another one continuously, it must
            call enableSlaveMode() at every update.

        */
		void								enableSlaveMode( bool en );

		/**
            Returns true if the control system component is currently in slave mode.
            \see enableSlaveMode()
        */
		bool								isSlaveModeEnabled() const;

	protected:
		InputData*						m_controlOutput;

		ControlSystemComponentBase*		m_previousStage;
		ControlSystemComponentBase*		m_nextStage;

		StateVariableBool				s_enabled;
		
		bool							m_isInSlaveMode;
	};


}