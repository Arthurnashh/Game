#pragma once

#include "UnitTestFramework/Scenario.h"
#include "UnitTestFramework/TestExecutor.h"


/**
	This class implements a scenario that can execute tests ( see TestBase ). It contains an instance of
	TestExecutor class.

	Note that, TestScenario is derived from Scenario class and hence has similar capabilities.
*/
class TestScenario : public Scenario
{
public:

	TestScenario( const std::string& name , const std::string& vehiclename , const std::string& groupname ,const std::string& subgroupname , 
				 Scenario::VehicleType type , const std::string& dynamicsModelConfig , const std::string& botConfig,
				 unsigned int aircraftCount=1);

    TestScenario( const std::string& name , const std::string& vehiclename , 
				  const std::string& groupname ,const std::string& subgroupname , 
				  std::vector<ConfigInfo> configInfo );


    TestScenario( const std::string& name , const std::string& vehiclename , 
				  const std::string& groupname ,const std::string& subgroupname );

	virtual ~TestScenario();

	virtual void init();

	virtual void update( smReal dt );

	virtual void finish();

	virtual void addComparison( const std::string& desc , 
								const std::string& firstFile , const std::string& secondFile , 
								const std::vector<std::string>& ignoredChannels,
								const std::string& errorCommand,
								bool suggestReplacingFile );
	
protected:
	struct ComparisonInfo
	{
		std::string	m_comparisonDescription;
		std::string m_firstFile;
		std::string m_secondFile;
		std::vector<std::string>	m_ignoredChannels;
		std::string m_errorCommand;
		bool		m_suggestReplacingFile;
	};

	std::vector<ComparisonInfo>	m_comparisonInfo;

	TestExecutor*	m_testExecutor;
};
