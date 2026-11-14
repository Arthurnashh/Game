/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/VehicleBase.h"

namespace FixedWingLibCGF
{
	class SimplifiedAirplane;
	class AirplaneBot;
}
namespace RotorLibCGF
{
	class SimplifiedRotorcraft;
	class RotorcraftBot;
}

namespace RTD
{
	class World;
	class RTDYNAMICS_EXPORT CGFBase
	{
	public:
		enum StabilizeOnGroundStatus
		{
			StabilizeOnGround_Stabilized,	// Aircraft is stabilized on the ground
			StabilizeOnGround_Damaged,		// Aircraft has damaged. 
			StabilizeOnGround_Damaged_Due_DropAltitude,
			StabilizeOnGround_TimeOut
		};

		enum StabilityCriteria
		{
			StabilityCriteria_Altitude		=	0x00000001,
			StabilityCriteria_Orientation	=	0x00000010,

			No_StabilityCriteria
		};

	protected:
		CGFBase(RTD::World* world);
		virtual ~CGFBase() ;

		bool m_isCorrectlyConfigured;
		std::string m_configurationErrors;

	public:
		bool isCorrectlyConfigured() { return m_isCorrectlyConfigured; }
		std::string getConfigurationErrors() { return m_configurationErrors; }

		virtual RTD::VehicleBase*	getRTDVehicle() = 0;
		virtual RTD::BotBase*		getRTDBot() = 0;
		
		virtual StabilizeOnGroundStatus runDropSimulation(smVec2r posXY, float heading, float dropAltitude, StabilityCriteria stabCriteria) = 0 ;

		/** stabilizeOnGround can be used to put an aircraft/helicopter on the ground and make sure it has reached
		*	positional (or optionally orientational) stability.
		*
		*	@returns	true if the aircraft is stable. false if it wasn't possible to stabilize the aircraft/helicopter
		*				on the ground. This can happen if the drop altitude is to large and causes the aircraft/helicopter
		*				landing gear to be damaged.
		*
		*	This function sets the master controller of the bot to none. The landing gear is also extended fully.
		*
		*/
		bool stabilizeOnGround(smVec2r posXY, float dropAltitude, StabilityCriteria stabCriteria);

		bool	isUsingTerrainQuery( ) ;

		virtual void	pushController( ) ;
		virtual void	popController( ) {};

	protected:
		RTD::World* m_world;
		RTD::BotControllerBase* m_prevCtrl;
		
		bool m_groundStabilizationPosAvailable ;
		smVec2r m_lastGroundStabilizationPos ;
		float  m_lastGroundStabilizationAlt ;
	};

	class FwBot;
	class RTDYNAMICS_EXPORT AircraftCGF :  public CGFBase
	{
	protected:
		FixedWingLibCGF::SimplifiedAirplane*	m_simplifiedAirplane;
		FixedWingLibCGF::AirplaneBot*			m_controllerBot;

		static unsigned int						s_instanceCounter;

	public:
		AircraftCGF( RTD::World* world ) ;
		virtual ~AircraftCGF() ;

		virtual void									construct(const std::string& vehicleParameterFile, const std::string& botParameterFile);
		virtual RTD::VehicleBase*						getRTDVehicle();
		virtual RTD::BotBase*							getRTDBot();
		virtual FixedWingLibCGF::SimplifiedAirplane*	getVehicle() { return m_simplifiedAirplane; }
		virtual FixedWingLibCGF::AirplaneBot*			getBot() { return m_controllerBot; }

		virtual FixedWingLibCGF::SimplifiedAirplane*	makeVehicle(std::string uniqueName, std::string configFile);
		virtual FixedWingLibCGF::AirplaneBot*			makeBot(FixedWingLibCGF::SimplifiedAirplane* vehicle, std::string uniqueName, std::string configFile);

		/** This method is used by the stabilizeOnGround().*/
		virtual StabilizeOnGroundStatus runDropSimulation(smVec2r posXY, float heading, float dropAltitude, StabilityCriteria stabCriteria);

		virtual bool canPrevControllerPerformedOnGround(RTD::BotControllerBase* prevCtrl);
		
		virtual void	popController( );
	};

	class RwBot;
	class RTDYNAMICS_EXPORT HeliCGF :  public CGFBase
	{
	protected:
		RotorLibCGF::SimplifiedRotorcraft*		m_simpleFDM;
		RotorLibCGF::RotorcraftBot*				m_controllerBot;

		static unsigned int						s_instanceCounter;

	public:
		HeliCGF( RTD::World* world ) ;
		virtual ~HeliCGF() ;

		virtual void								construct(const std::string& vehicleParameterFile, const std::string& botParameterFile);
		virtual RTD::VehicleBase*					getRTDVehicle();
		virtual RTD::BotBase*						getRTDBot();
		virtual RotorLibCGF::SimplifiedRotorcraft*	getVehicle() { return m_simpleFDM; }
		virtual RotorLibCGF::RotorcraftBot*			getBot() { return m_controllerBot; }

		virtual RotorLibCGF::SimplifiedRotorcraft*	makeVehicle(std::string uniqueName, std::string configFile);
		virtual RotorLibCGF::RotorcraftBot*			makeBot(RotorLibCGF::SimplifiedRotorcraft* vehicle, std::string uniqueName, std::string configFile);

		virtual StabilizeOnGroundStatus runDropSimulation(smVec2r posXY, float heading, float dropAltitude, StabilityCriteria stabCriteria) ;
		virtual void	popController( );
	};
}
