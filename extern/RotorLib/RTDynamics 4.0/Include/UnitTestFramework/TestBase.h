#pragma once

#include <RTD/VehicleBase.h>
#include <RTD/BotBase.h>


class Scenario;

class TestBase
{
public:


    TestBase( Scenario* scn );

    virtual ~TestBase();

    virtual void init();

	virtual bool    update( const std::vector<RTD::VehicleBase*>& vehicles , const std::vector<RTD::BotBase*>& bots , smReal time );


protected:

	Scenario*	m_scenario;

};