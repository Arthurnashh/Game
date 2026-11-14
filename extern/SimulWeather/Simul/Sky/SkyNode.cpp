#include "precomp.h"

#include "SkyNode.h"
#include "Simul/Base/EnvironmentVariables.h"
#include <fstream>
#include <sstream>

using namespace simul::sky;
SkyNode::SkyNode() : Node(), Sky()
{
	TimeMultiplier=1.f;
}

void SkyNode::PrepareToDestroy()
{
}

SkyNode::~SkyNode()
{
}

bool SkyNode::TimeStep(float dt)
{
	Sky::TimeStep(TimeMultiplier*dt);
	return true;
}

std::ostream &SkyNode::StateStreamOut(std::ostream &os) const
{
	Sky::StreamOutAutoPropertiesState(os);
	return os;
}
std::istream &SkyNode::StateStreamIn(std::istream &is)
{
	Sky::StreamInAutoPropertiesState(is);
	return is;
}

std::ostream &SkyNode::StreamOut(std::ostream &os) const
{
	Sky::StreamOutAutoPropertiesDefinition(os);
	return os;
}
std::istream &SkyNode::StreamIn(std::istream &is)
{
	Sky::StreamInAutoPropertiesDefinition(is);
	return is;
}
