/*
(c) Copyright 2008, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/ForceGenerator.h"
#include "RTD/VehicleBase.h"
#include "RTD/LookUpTable.h"
#include "Physics/smGraph2D.h"

namespace RTD
{
	class RotorModelBase;

	/**
		This class is used to compute the forces and moments produced by a helicopter fuselage.

		Motion of the fuselage in the air and interaction between fuselage and rotors are taken into
		account in computing the forces and moments.
	*/
	class RTDYNAMICS_EXPORT FGHeliFuselage100 : public ForceGenerator
	{
	public:
		

		/**
			Configuration parameters.
		*/		
		class RTDYNAMICS_EXPORT ConfigParams
		{
			public:
				/** Constructor of the class. */
				ConfigParams();

				smReal				m_mainRotorInplanewashLowA1;
				smReal				m_mainRotorInplanewashMedA1;
				smReal				m_mainRotorInplanewashHighA1;
				smGraph2D			m_mainRotorInplanewashLowA1Curve;
				smGraph2D			m_mainRotorInplanewashMedA1Curve;
				smGraph2D			m_mainRotorInplanewashHighA1Curve;
				
				smReal				m_mainRotorDownwashLowA1;
				smReal				m_mainRotorDownwashMedA1;
				smReal				m_mainRotorDownwashHighA1;
				smGraph2D			m_mainRotorDownwashLowA1Curve;
				smGraph2D			m_mainRotorDownwashMedA1Curve;
				smGraph2D			m_mainRotorDownwashHighA1Curve;

				bool				m_use2DTablesForAeroCoefficients;

				smGraph2D			m_dragDueToAoA;				
				smGraph2D			m_dragDueToSideslip;// Beta positive if relative wind comes from right

				smGraph2D			m_sideforceDueToSideslip;
				smGraph2D			m_sideforceDueToAoA;

				smGraph2D			m_liftDueToAoA;
				smGraph2D			m_liftDueToSideslip;
				
				smGraph2D			m_rollingMomentDueToSideslip;
				smGraph2D			m_rollingMomentDueToAoA;

				smGraph2D			m_pitchingMomentDueToAoA;
				smGraph2D			m_pitchMomentDueToSideslip;

				smGraph2D			m_yawingMomentDueToSideslip;				
				smGraph2D			m_yawingMomentDueToAoA;				

				RTD::LookUpTable	m_dragTable;
				RTD::LookUpTable	m_sideForceTable;
				RTD::LookUpTable	m_liftTable;
				RTD::LookUpTable	m_rollingMomentTable;
				RTD::LookUpTable	m_pitchingMomentTable;
				RTD::LookUpTable	m_yawingMomentTable;


				smReal				m_rotationalResistance;

				/**
					Relative to attached vehicle.
				*/
				smVec3r				m_centerOfPressure;

				virtual bool	readParamStr( const std::string& parameterString , std::string& error );
		};

		/**
			Constructor of the class.

			\param	name		:	Name of the instance.
			\param	vehicle	    :	The vehicle on which the drag will act.
			\param	parameters	:	Configuration parameters.
		*/
		FGHeliFuselage100(	const std::string& name,
							VehicleBase* vehicle,
							const ConfigParams& parameters  ) ;




		/** Updates the force generator. */
		virtual void update( smTime /* timestep */ ) ;


		/**
			Returns fuselage AoA including the effect of rotor downwash. Since downwash is
			included the returned AoA will be different than aircraft's AoA which 
			solely takes the attitude and flight path into account.
		*/
		smReal	getAoA() const		{	return o_AoA;	}

		/**
			Returns fuselage side slip including the effect of rotor downwash. Since downwash is
			included the returned side slip will be different than aircraft's side slip which 
			solely takes the attitude and flight path into account.
		*/
		smReal	getSideSlip() const	{	return o_sideslip;	}

		ConfigParams&		getConfigParams()		{	return m_configParams;	}
		const ConfigParams&	getConfigParams() const	{	return m_configParams;	}

		const	smVec3r&	getLift()		{	return o_lift;	}
		const	smVec3r&	getDrag()		{	return o_drag;	}
		const	smVec3r&	getSideforce()	{	return o_sideforce;	}

		const	smReal		getDynamicPressure() const			{	return	o_dynamicPressure;}
		const	smReal		getLiftCoeffAoA() const				{	return	o_Cliftaoa;}
		const	smReal		getLiftCoeffSideslip() const		{	return	o_Cliftsideslip;}
		const	smReal		getDragCoeffAoA() const				{	return	o_Cdragaoa;}
		const	smReal		getDragCoeffSideslip() const		{	return	o_Cdragsideslip;}
		const	smReal		getSideforceCoeffAoA() const		{	return	o_Csideforceaoa;}
		const	smReal		getSideforceCoeffSideslip() const	{	return	o_Csideforcesideslip;}

		const	smReal		getPitchMomentCoeffAoA() const		{	return	o_Cpitchmomentaoa;}
		const	smReal		getPitchMomentCoeffSideslip() const	{	return	o_Cpitchmomentsideslip;}

		const	smReal		getRollMomentCoeffAoA() const		{	return	o_Crollmomentaoa;}
		const	smReal		getRollMomentCoeffSideslip() const	{	return	o_Crollmomentsideslip;}

		const	smReal		getYawMomentCoeffAoA() const		{	return	o_Cyawmomentaoa;}
		const	smReal		getYawMomentCoeffSideslip() const	{	return	o_Cyawmomentsideslip;}

		virtual void		reset();
	
		void				setInterferingRotor( RotorModelBase* ir );

	protected:

		VehicleBase*					m_vehicle;

		RTD::RotorModelBase*			m_interferingRotor;

		smCoefPos						m_airDensity;

		ConfigParams					m_configParams;


		OutputVariableVec3r				o_lift;
		OutputVariableVec3r				o_drag;
		OutputVariableVec3r				o_sideforce;

		OutputVariableReal				o_AoA;
		OutputVariableReal				o_sideslip;

		OutputVariableReal				o_dynamicPressure;
		OutputVariableReal				o_Cliftaoa;
		OutputVariableReal				o_Cliftsideslip;
		OutputVariableReal				o_Cdragaoa;
		OutputVariableReal				o_Cdragsideslip;
		OutputVariableReal				o_Csideforceaoa;
		OutputVariableReal				o_Csideforcesideslip;
		OutputVariableReal				o_Cpitchmomentaoa;
		OutputVariableReal				o_Cpitchmomentsideslip;
		OutputVariableReal				o_Crollmomentaoa;
		OutputVariableReal				o_Crollmomentsideslip;
		OutputVariableReal				o_Cyawmomentaoa;
		OutputVariableReal				o_Cyawmomentsideslip;

		smReal	readValueFromCurves( smReal a1 , smReal skew,
				   				     smReal low, smReal medium , smReal high,
									 const smGraph2D& curveLow,const smGraph2D& curveMed, const smGraph2D& curveHigh );
	};

}
