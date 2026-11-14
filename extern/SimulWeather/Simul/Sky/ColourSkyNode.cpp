#include "precomp.h"

#include "ColourSkyNode.h"
#include "Simul/Base/EnvironmentVariables.h"
#include <fstream>
#include <sstream>

using namespace simul::sky;
ColourSkyNode::ColourSkyNode() : Node(), ColourSky()
{
}

void ColourSkyNode::PrepareToDestroy()
{
}

ColourSkyNode::~ColourSkyNode()
{
}

bool ColourSkyNode::TimeStep(float dt)
{
	ColourSky::TimeStep(dt);
	return true;
}

std::ostream &ColourSkyNode::StateStreamOut(std::ostream &os) const
{
	ColourSky::StreamOutAutoPropertiesState(os);
	return os;
}
std::istream &ColourSkyNode::StateStreamIn(std::istream &is)
{
	ColourSky::StreamInAutoPropertiesState(is);
	return is;
}

std::ostream &ColourSkyNode::StreamOut(std::ostream &os) const
{
	ColourSky::StreamOutAutoPropertiesDefinition(os);
	return os;
}
std::istream &ColourSkyNode::StreamIn(std::istream &is)
{
	ColourSky::StreamInAutoPropertiesDefinition(is);
	return is;
}
