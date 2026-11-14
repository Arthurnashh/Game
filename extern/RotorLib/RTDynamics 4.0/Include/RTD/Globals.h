/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "Physics/smTypes.h"
#include "RTD/Common.h"

namespace RTD
{
	class RTDYNAMICS_EXPORT Globals
	{
	private:
		explicit Globals() ;
		virtual	~Globals() ;

		static Globals *mInstance ;
	public:
		static Globals*	instance ( void )
		{
			if ( mInstance == 0 )
				mInstance = new Globals ;
			return mInstance ;
		}

		static void destroy ( void )
		{
			if ( mInstance != 0 )
			{
				delete mInstance ;
				mInstance = 0;
			}
		}

	public:
		smPrinter*			m_printer ;
		smStdErrPrinter*	m_stdErrPrinter;
		smLogFilePrinter*	mlogFilePrinter;
		bool				m_noIntegration ;
		smTime				m_currentTime ;

		bool				m_printForceAndTorqueAccumulators;
		std::string			m_ecfgPassword;

        bool                m_loggingWithHotkeyEnabled;
        std::string         m_loggingWithHotkeyTargetPath;
        std::string         m_loggingWithHotkeyVehicleName;
        int                 m_loggingWithHotkeyDownsampling;
	};

}