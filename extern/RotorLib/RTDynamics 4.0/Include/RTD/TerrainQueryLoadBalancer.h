#include "BotBase.h"

namespace RTD
{

class TerrainQueryLoadBalancer 
{
public:
	TerrainQueryLoadBalancer( RTD::BotControllerBase* owner );


	bool update( smReal deltaTime , smReal currentTerrainQueryInterval );

protected:

	RTD::StateVariableReal		s_terrainQueryTimer;
	RTD::StateVariableInt		s_random;
	
};

}