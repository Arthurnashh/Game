#pragma once

#include "ScenarioBase.h"
#include "FixedWingLibCGF/SimplifiedAirplane.h"
#include "FixedWingLibCGF/AirplaneBot.h"
#include "RotorLibCGF/RotorcraftBot.h"
#include "RotorLibFDM/GenHeli500.h"
#include "RotorLibFDM/GenHeli600.h"
#include "FixedWingLibFDM/GenAirplane100.h"
#include "GroundLibDDM/GenGroundVehicle100.h"

/**
	This class implements a generic scenario that can create vehicles and bots. The scenario
	can create multiple vehicles and bots. All the vehicles will belong to the same simulation
	world.

	Users can create vehicles and bots of different types, for example, it is possible to create a GenHeli500
	and two pairs of SimplifiedRotorcraft/RotorcraftBot as part of the same scenario.
*/
class Scenario : public ScenarioBase
{
public:
	enum VehicleType
	{
		GENHELI500,
		GENHELI600,
		GENAIRPLANE100,
		GENGROUNDVEHICLE100,
		SIMPLIFIED_AIRPLANE,
		SIMPLIFIED_ROTORCRAFT
	};

	struct ConfigInfo
	{
		VehicleType	vehicleType;
		std::string dynamicsModelConfig;
		std::string botConfig;
	};

    Scenario( const std::string& name , const std::string& vehiclename , 
						 const std::string& groupname ,const std::string& subgroupname , 
						 VehicleType type,
						 const std::string& dynamicsModelConfig , const std::string& botConfig,
						 unsigned int aircraftCount=1 );

    Scenario( const std::string& name , const std::string& vehiclename , 
						 const std::string& groupname ,const std::string& subgroupname , 
						 std::vector<ConfigInfo> configInfo );

    Scenario( const std::string& name , const std::string& vehiclename , 
						 const std::string& groupname ,const std::string& subgroupname );

	const RTD::VehicleBase*					    getVehicle(unsigned int index = 0) const;
    RTD::VehicleBase*						    getVehicle(unsigned int index = 0);

	const  FixedWingLibCGF::SimplifiedAirplane*	getVehicleAsSimplifiedAirplane(unsigned int index = 0) const;
	 FixedWingLibCGF::SimplifiedAirplane*		getVehicleAsSimplifiedAirplane(unsigned int index = 0);

	const RotorLibCGF::SimplifiedRotorcraft*    getVehicleAsSimplifiedRotorcraft(unsigned int index = 0) const;
	RotorLibCGF::SimplifiedRotorcraft*		    getVehicleAsSimplifiedRotorcraft(unsigned int index = 0);

	const RTD::BotBase*						    getBot(unsigned int index = 0) const;
    RTD::BotBase*							    getBot(unsigned int index = 0);

	const FixedWingLibCGF::AirplaneBot*		    getBotAsAirplaneBot(unsigned int index = 0) const;
    FixedWingLibCGF::AirplaneBot*			    getBotAsAirplaneBot(unsigned int index = 0);

	const RotorLibCGF::RotorcraftBot*		    getBotAsRotorcraftBot(unsigned int index = 0) const;
    RotorLibCGF::RotorcraftBot*					getBotAsRotorcraftBot(unsigned int index = 0);


    virtual void init();

	virtual void reset();

    // Factory method allows other classes to create their own bots like the ACMLBot.
	virtual RTD::BotBase* createBot( VehicleType type , 
									const std::string& botconfig ,  
									RTD::VehicleBase* vehicle , 
									std::string nameSuffix );

	unsigned int								getNumVehicles() const { return (unsigned int) m_vehicles.size(); }

protected:
	std::vector<RTD::VehicleBase*>			m_vehicles;
	std::vector<RTD::BotBase*>		        m_bots;

	std::vector<ConfigInfo>                              m_configInfos;
};

