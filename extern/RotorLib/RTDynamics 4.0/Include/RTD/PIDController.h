/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "Common.h"
#include "ParamDb.h"

namespace RTD
{
    /**
        This class implements a PID controller.
    */
	class RTDYNAMICS_EXPORT PIDController : public SimulationVariableUser
    {
    public:

        /**
        *   Constructors of the class.
        *
        *   \param  name        :   Name of the PID controller instance.
        *   \param  parent      :   Parent state variable user. This could be a simulation entity, a force generator, a model component or any other state variable user.
        *   \param  P           :   Proportional coefficient of the controller.
        *   \param  I           :   Integral coefficient of the controller.
        *   \param  D           :   Derivative coefficient of the controller.
        *   \param  offset      :   "Zero point" of the controller.
        *   \param  integralMin :   Minimum value the error sum can reach. If the sum of errors go below
        *                           this value, it is clamped to this value.
        *   \param  integralMax :   Maximum value the error sum can reach. If the sum of errors go above
        *                           this value, it is clamped to this value.
		*   \param  proportionalOutputMin : Minimum value that can be the contribution of the proportional
		*  									control to the final output. Values below this will be clamped.
		*   \param  proportionalOutputMax : Maximum value that can be the contribution of the proportional
		*  									control to the final output. Values above this will be clamped.
		*   \param  derivativeOutputMin : Minimum value that can be the contribution of the derivative
		*  									control to the final output. Values below this will be clamped.
		*   \param  derivativeOutputMax : Maximum value that can be the contribution of the derivative
		*  									control to the final output. Values above this will be clamped.
        */
        PIDController(  const std::string& name,
						SimulationVariableUser* parent,
                        smReal P = 0.0f , smReal I = 0.0f , smReal D = 0.0f ,
                        smReal offset = 0.0f ,
                        smReal integralMin = -FLT_MAX,
                        smReal integralMax = FLT_MAX,
                        smReal proportionalOutputMin = -FLT_MAX,
                        smReal proportionalOutputMax = FLT_MAX,
                        smReal derivativeOutputMin = -FLT_MAX,
                        smReal derivativeOutputMax = FLT_MAX );


        /**
        *   Constructors of the class.
        *
        *   \param  name        :   Name of the PID controller instance.
        *   \param  P           :   Proportional coefficient of the controller.
        *   \param  I           :   Integral coefficient of the controller.
        *   \param  D           :   Derivative coefficient of the controller.
        *   \param  offset      :   "Zero point" of the controller. When error, error derivative and error
        *                           sum is zero, resulting output will be \e offset
        *   \param  integralMin :   Minimum value the error sum can reach. If the sum of errors go below
        *                           this value, it is clamped to this value.
        *   \param  integralMax :   Maximum value the error sum can reach. If the sum of errors go above
        *                           this value, it is clamped to this value.
		*   \param  proportionalOutputMin : Minimum value that can be the contribution of the proportional
		*  									control to the final output. Values below this will be clamped.
		*   \param  proportionalOutputMax : Maximum value that can be the contribution of the proportional
		*  									control to the final output. Values above this will be clamped.
		*   \param  derivativeOutputMin : Minimum value that can be the contribution of the derivative
		*  									control to the final output. Values below this will be clamped.
		*   \param  derivativeOutputMax : Maximum value that can be the contribution of the derivative
		*  									control to the final output. Values above this will be clamped.
        */
        PIDController(  const std::string& name,
                        smReal P = 0.0f , smReal I = 0.0f , smReal D = 0.0f ,
                        smReal offset = 0.0f ,
                        smReal integralMin = -FLT_MAX,
                        smReal integralMax = FLT_MAX,
						smReal proportionalOutputMin = -FLT_MAX,
                        smReal proportionalOutputMax = FLT_MAX,
                        smReal derivativeOutputMin = -FLT_MAX,
                        smReal derivativeOutputMax = FLT_MAX );

        /**
            Destructor of the class.
        */
        virtual ~PIDController()
        {
        }

        /**
            Updates the controller.
            \param  error       :   Current error.
            \param  deltaTime   :   Time change in seconds.
        */
        smReal  update( smReal error , smReal deltaTime );

        /**
            Resets the controller. Error, error derivative and error sum is set to zero.
        */
        void    reset();

        /**
            Sets parameters of the PID controller.

            \param  P           :   Proportional coefficient of the controller.
            \param  I           :   Integral coefficient of the controller.
            \param  D           :   Derivative coefficient of the controller.
            \param  offset      :   "Zero point" of the controller.
            \param  integralMin :   Minimum value the error sum can reach. If the sum of errors go below this value it is clamped to this value.
            \param  integralMax :   Maximum value the error sum can reach. If the sum of errors go above this value it is clamped to this value.
		    \param  proportionalOutputMin : Minimum value that can be the contribution of the proportional
		  									control to the final output. Values below this will be clamped.
		    \param  proportionalOutputMax : Maximum value that can be the contribution of the proportional
		  									control to the final output. Values above this will be clamped.
		    \param  derivativeOutputMin : Minimum value that can be the contribution of the derivative
		  									control to the final output. Values below this will be clamped.
		    \param  derivativeOutputMax : Maximum value that can be the contribution of the derivative
		  									control to the final output. Values above this will be clamped.
        */
        virtual void setParameters( smReal P = 0.0f , smReal I = 0.0f , smReal D = 0.0f ,
                                    smReal offset = 0.0f ,
                                    smReal integralMin = -FLT_MAX,
                                    smReal integralMax = FLT_MAX,
									smReal proportionalOutputMin = -FLT_MAX,
									smReal proportionalOutputMax = FLT_MAX,
									smReal derivativeOutputMin = -FLT_MAX,
									smReal derivativeOutputMax = FLT_MAX  );

        /**
            Sets parameters of the PID controller with the parameters of another controller.
        */
        virtual void setParameters( const PIDController& pid );

        /**
            Returns the parameters of the PID controller.
            @see setParemeters()
        */
        void getParameters      (   smReal& P, smReal& I, smReal& D,
                                    smReal& offset,
                                    smReal& integralMin,
                                    smReal& integralMax,
									smReal& proportionalOutputMin,
									smReal& proportionalOutputMax,
									smReal& derivativeOutputMin,
									smReal& derivativeOutputMax) const;


		/**
            This method changes internal variables of the PID so that its output becomes
            the user provided value when update() is called with the user provided error.
        */
		void	setOutput( smReal value , smReal error  );


		void	        enableDebugPrint( bool en );
		bool	        isDebugPrintEnabled() const;
        void            print();


		smReal          getCurrentError()	const			{	return i_error;	}
        smReal          getCurrentErrorIntegral()	const	{	return s_errorSum;	}      
		smReal			getCurrentPContribution() const		{	return o_currentP;	}
		smReal			getCurrentIContribution() const		{	return o_currentI;	}
		smReal			getCurrentDContribution() const		{	return o_currentD;	}
		smReal			getCurrentOutput() const			{	return o_currentOutput;	}

		void			setPScale( smReal s )				{	s_PScale = s ;	}
		void			setIScale( smReal s )				{	s_IScale = s ;	}
		void			setDScale( smReal s )				{	s_DScale = s ;	}

		smReal			getPScale()	const					{	return s_PScale;}
		smReal			getIScale()	const					{	return s_IScale;}
		smReal			getDScale()	const					{	return s_DScale;}

		void	        setCurrentErrorIntegral(smReal ei)	{	s_errorSum = ei;	}      
    public:

		bool				m_debugPrintEnabled;

        StateVariableReal   m_P;
        StateVariableReal   m_I;
        StateVariableReal   m_D;
        StateVariableReal   m_offset;
        StateVariableReal   m_integralMinLimit;
        StateVariableReal   m_integralMaxLimit;
        StateVariableReal   m_proportionalOutputMinLimit;
        StateVariableReal   m_proportionalOutputMaxLimit;
        StateVariableReal   m_derivativeOutputMinLimit;
        StateVariableReal   m_derivativeOutputMaxLimit;

		StateVariableReal	s_PScale;
		StateVariableReal	s_IScale;
		StateVariableReal	s_DScale;		

        InputVariableReal   i_error;
        StateVariableReal   s_prevError;
        StateVariableReal   s_errorSum;

		OutputVariableReal  o_currentP;
        OutputVariableReal  o_currentI;
        OutputVariableReal  o_currentD;
        OutputVariableReal  o_currentOutput;
        OutputVariableReal  o_currentIntegral;

        StateVariableBool   s_isFirstFrame;

    };
}

