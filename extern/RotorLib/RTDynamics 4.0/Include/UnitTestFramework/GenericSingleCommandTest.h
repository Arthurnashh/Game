#pragma once

#include <RTD/VehicleBase.h>
#include <RTD/BotBase.h>
#include "TestBase.h"

class GenericSingleCommandTest : public TestBase
{
public:

    enum Phase
    {
        INITCOMMAND,
        TESTCOMMAND,
        CHECKEXITCONDITION,
        POSTFINISH,
        DONE
    };

    /**
    *   @param  sleepTimeBeforeTestCommand      the time which needs to pass after initCommand is called
    *                                           before testCommand is called.
    *
    */
    GenericSingleCommandTest( Scenario* scn, smReal sleepTimeBeforeTestCommand , smReal timeAfterExitCondition );

    virtual ~GenericSingleCommandTest();

    virtual void init();

	virtual bool    update( const std::vector<RTD::VehicleBase*>& vehicles , const std::vector<RTD::BotBase*>& bots , smReal time );

	const Phase	getPhase() const	{	return m_phase;	}
protected:
    Phase   m_phase;
    smReal  m_sleepTimeBeforeTestCommand;
    smReal  m_timeAfterExitCondition;
    smReal  m_startTime;
    smReal  m_exitConditionTime;
    smReal  m_timeOut;
    smReal  m_noDataRecordScale ;


    virtual void    initCommand(const std::vector<RTD::VehicleBase*>& vehicles , const std::vector<RTD::BotBase*>& bots , smReal time ) = 0;
    virtual void    testCommand(const std::vector<RTD::VehicleBase*>& vehicles , const std::vector<RTD::BotBase*>& bots , smReal time ) = 0;
    virtual bool    checkForTestEnd(const std::vector<RTD::VehicleBase*>& vehicles , const std::vector<RTD::BotBase*>& bots , smReal time ) = 0;

};