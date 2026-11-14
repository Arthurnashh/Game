/*
(c) Copyright 2008, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include <Physics/smGraph2D.h>
#include "RTD/VehicleBase.h"
#include "RTD/ForceGenerator.h"
#include "RTD/RigidBodyState.h"
#include "RTD/FGWing.h"
#include "RotorLibFDM/FGHeliFuselage100.h"
#include "RTD/RotorModelBase.h"

namespace RotorLibFDM
{
	/** 
		FGHelicopterStabilizerWing is a wing model which is used to simulate helicopter stabilizers.
		It is derived from FGWing and inherits most of the functionality from it. It extends 
		FGWing by adding downwash and dynamic pressure changes effects.
	*/
	class RTDYNAMICS_EXPORT	 FGHelicopterStabilizerWing : public RTD::FGWing
	{
	public:
		/** Configuration parameters of FGHelicopterStabilizerWing */
		class RTDYNAMICS_EXPORT ConfigParams : public RTD::FGWing::ConfigParams
		{
		public:
			ConfigParams();			
			virtual ~ConfigParams();			

			smReal		m_mainRotorDownwashLowA1;
			smReal		m_mainRotorDownwashMediumA1;
			smReal		m_mainRotorDownwashHighA1;
			smGraph2D	m_mainRotorDownwashCurveLowA1;
			smGraph2D	m_mainRotorDownwashCurveMediumA1;
			smGraph2D	m_mainRotorDownwashCurveHighA1;

			smReal		m_mainRotorInplanewashLowA1;
			smReal		m_mainRotorInplanewashMediumA1;
			smReal		m_mainRotorInplanewashHighA1;
			smGraph2D	m_mainRotorInplanewashCurveLowA1;
			smGraph2D	m_mainRotorInplanewashCurveMediumA1;
			smGraph2D	m_mainRotorInplanewashCurveHighA1;

			smGraph2D	m_dynamicPressureRatioAoA;
			smGraph2D	m_dynamicPressureRatioSideslip;

			virtual bool			readParamStr( const std::string& parameterString , std::string& error );
		};

		/**
			Constructor of the FGHelicopterStabilizerWing class.
		*/
		FGHelicopterStabilizerWing( const std::string& name ,
			RTD::VehicleBase* vehicle,
			RTD::RotorModelBase* mainRotor,
			RTD::RotorModelBase* tailRotor,
			RTD::FGHeliFuselage100*fuselage,
			const ConfigParams* params ) ;

		virtual ~FGHelicopterStabilizerWing();

		virtual void update( smTime timestep ) ;

		virtual void reset();

		FGHelicopterStabilizerWing::ConfigParams*	getConfigParams();
	protected:

		smReal	readValueFromCurves( smReal a1 , smReal skew,
									 smReal low, smReal medium , smReal high,
									 const smGraph2D& curveLow,const smGraph2D& curveMed, const smGraph2D& curveHigh );

		RTD::RotorModelBase*	m_mainRotor;
		RTD::RotorModelBase*	m_tailRotor;
		RTD::FGHeliFuselage100*			m_fuselage;


	};

}
