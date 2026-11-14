#pragma once

#include <vector>

class Scenario;

class UnitTester
{
public:
	UnitTester();
	virtual ~UnitTester();

	virtual void	processCommandLine( int argc, char* argv[] , std::vector<unsigned int>& scenarioIndices );

	virtual void	update( float dt );

	virtual void	addScenario( Scenario* test )	{	m_scenarios.push_back( test );	}

	unsigned int	getNumScenarios() const				{	return (unsigned int) m_scenarios.size();		}
	Scenario*		getScenario( unsigned int index );
	const Scenario*	getScenario( unsigned int index ) const;

	virtual void	setCurrentScenarioByIndex( unsigned int t);

	unsigned int	getCurrentScenarioIndex() const	{	return m_currentScenario;	}

	virtual void	printScenarios() const;

	const Scenario*	getCurrentTestScenario() const;
	Scenario*		getCurrentTestScenario();

	virtual void		runScenarios( const std::vector<unsigned int>& scenarioIndices );
protected:
	
	unsigned int				m_currentScenario;
	std::vector< Scenario* >	m_scenarios;
};
