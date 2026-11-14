#pragma once

#include <RTD/VehicleBase.h>
#include <RTD/BotBase.h>

class TestBase;
class TestExecutor
{
public:
	TestExecutor();

	virtual ~TestExecutor();

	virtual void init();

	virtual void addTest( TestBase* test );

	// Returns true if finished
	virtual bool update( const std::vector<RTD::VehicleBase*> vehicles , const std::vector<RTD::BotBase*> bots , smReal time );
	
	const	TestBase*	getCurrentTest() const;
	TestBase*			getCurrentTest() ;
protected:
	unsigned int			m_currentSubTest;
	std::vector<TestBase*>	m_subTests;
};
