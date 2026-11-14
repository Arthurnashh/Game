#pragma once

#include <string>
#include "RTD/Common.h"
#include "Util/smNamed.h"
#include "RTD/World.h"
#include "RTD/TraitGroundContact.h"


/**
	This class defines the interface of a generic scenario class that can be executed by a UnitTester instance.

	Note that, normally, users do not need to derive their own classes from this class. The test framework provides two
	more scenario classes which can create vehicles ( Scenaro class ) and execute tests ( TestScenario class ).
	These two should be sufficient for most of the purposes. 
*/
class ScenarioBase
{
public:
    friend class UnitTester;

    ScenarioBase( const std::string& name , const std::string& vehiclename , const std::string& groupname ,const std::string& subgroupname );

    virtual ~ScenarioBase();

    enum TestPhase
    {
        INIT,
        UPDATE,
        FINISH,
        DONE
    };

    virtual void    init()
	{
		m_pauseTimer = -1.0f;
	}

    virtual void    update( smReal dt )
    {

    }

    virtual void    pauseModeUpdate( smReal dt )
    {
		m_pauseTimer -= dt;
		if ( m_pauseTimer <= 0.0f )
			m_pauseTimer = -1.0f;
    }

    virtual void    worldStep( smReal dt )
    {
		getWorld().update( dt , false  );
    }
	

    virtual void    finish() = 0;

    // Reset and initialization of the scenarios should occur in init method.
    // Normally this method is needed to reset the time and phase only!
    virtual void    reset()
    {
        m_testPhase = INIT;
		m_world.reset();
    }

    TestPhase           getPhase() const    {   return m_testPhase; }

    const RTD::World&   getWorld() const    {   return m_world; }
    RTD::World&         getWorld()          {   return m_world; }

    void                setName( const std::string& n ) { m_name = n;   }
    const std::string&  getName() const				    {   return m_name;  }

    void                setVehicleName( const std::string& n ) { m_vehicleName = n;   }
    const std::string&  getVehicleName() const				   { return m_vehicleName;  }

    void                setGroupName( const std::string& n )    { m_groupName = n;  }
    const std::string&  getGroupName() const      {   return m_groupName; }

    void                setSubGroupName( const std::string& n ) { m_subGroupName = n;   }
    const std::string&  getSubGroupName() const                 { return m_subGroupName;    }

	std::string			getFullName() const	{ return getGroupName() + "_" + getSubGroupName() + "_" +  getName() + "_" + getVehicleName();	}

	std::string         generateLogFileName(const std::string& customSuffix = "") const		{ return  getFullName() + customSuffix + ".log" ; }

	smReal				getTime() const	{	return  (smReal)getWorld().getSimulationTime();	}


	float				getPauseTimer() const	{	return m_pauseTimer;	}

protected:
    TestPhase       m_testPhase;
    RTD::World      m_world;
	float			m_pauseTimer;



    std::string     m_vehicleName;
    std::string     m_groupName;
    std::string     m_subGroupName;
    std::string     m_name;


    
};