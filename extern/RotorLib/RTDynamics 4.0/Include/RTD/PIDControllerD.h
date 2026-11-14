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
        This class implements a PID controller. All internal calculations are in double precision.
    */
	class RTDYNAMICS_EXPORT PIDControllerD : public SimulationVariableUser
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
        *   \param  offset      :   "Zero point" of the controller. When error, error derivative and error
        *                           sum is zero, resulting output will be \e offset
        *   \param  integralMin :   Minimum value the error sum can reach. If the sum of errors go below
        *                           this value, it is clamped to this value.
        *   \param  integralMax :   Maximum value the error sum can reach. If the sum of errors go above
        *                           this value, it is clamped to this value.
        */
        PIDControllerD(  const std::string& name,
						SimulationVariableUser* parent,
                        smDouble P = 0.0f , smDouble I = 0.0f , smDouble D = 0.0f ,
                        smDouble offset = 0.0f ,
                        smDouble integralMin = -FLT_MAX,
                        smDouble integralMax = FLT_MAX );

        /**
        *   Constructors of the class.
        *
        *   \param  name        :   Name of the PID controller instance.
        *   \param  P           :   Proportional coefficient of the controller.
        *   \param  I           :   Integral coefficient of the controller.
        *   \param  D           :   Derivative coefficient of the controller.
        *   \param  offset      :   "Zero point" of the controller.
        *   \param  integralMin :   Minimum value the error sum can reach. If the sum of errors go below
        *                           this value, it is clamped to this value.
        *   \param  integralMax :   Maximum value the error sum can reach. If the sum of errors go above
        *                           this value, it is clamped to this value.
        */
        PIDControllerD(  const std::string& name,
                        smDouble P = 0.0f , smDouble I = 0.0f , smDouble D = 0.0f ,
                        smDouble offset = 0.0f ,
                        smDouble integralMin = -FLT_MAX,
                        smDouble integralMax = FLT_MAX );


        /**
            Destructor of the class.
        */
        virtual ~PIDControllerD()
        {
        }

        /**
            Updates the controller.
            \param  error       :   Current error.
            \param  deltaTime   :   Time change in seconds.
        */
        smDouble  update( smDouble error , smDouble deltaTime );

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

        */
        virtual void setParameters( smDouble P = 0.0f , smDouble I = 0.0f , smDouble D = 0.0f ,
                                    smDouble offset = 0.0f ,
                                    smDouble integralMin = -FLT_MAX,
                                    smDouble integralMax = FLT_MAX );

        /**
            Sets parameters of the PID controller with the parameters of another controller.
        */
        virtual void setParameters( const PIDControllerD& pid );

        /**
            Returns the parameters of the PID controller.
            @see setParemeters()
        */
        void getParameters      (   smDouble& P, smDouble& I, smDouble& D,
                                    smDouble& offset,
                                    smDouble& integralMin,
                                    smDouble& integralMax ) const;

		/**
            This method changes internal variables of the PID so that its output becomes
            the user provided value when update() is called with the user provided error.
        */
		void	setOutput( smDouble val , smDouble error  );

		smDouble    getCurrentError()	const			{	return i_error;	}
        smDouble    getCurrentErrorIntegral()	const	{	return s_errorSum;	}      
		smDouble	getCurrentPContribution() const		{	return o_currentP;	}
		smDouble	getCurrentIContribution() const		{	return o_currentI;	}
		smDouble	getCurrentDContribution() const		{	return o_currentD;	}
		smDouble	getCurrentOutput() const			{	return o_currentOutput;	}

		void	enableDebugPrint( bool en );
		bool	isDebugPrintEnabled() const;
        void    print();

    public:

        StateVariableDouble    m_P;
        StateVariableDouble    m_I;
        StateVariableDouble    m_D;
        StateVariableDouble    m_offset;
        StateVariableDouble    m_integralMinLimit;
        StateVariableDouble    m_integralMaxLimit;

		bool		m_debugPrintEnabled;

        InputVariableDouble    i_error;
        StateVariableDouble    s_prevError;
        StateVariableDouble    s_errorSum;

        OutputVariableDouble   o_currentP;
        OutputVariableDouble   o_currentI;
        OutputVariableDouble   o_currentD;
        OutputVariableDouble   o_currentOutput;


        StateVariableBool      s_isFirstFrame;

    };
}

