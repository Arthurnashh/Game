#include "Simul/Graph/StandardNodes/World.h"
using namespace simul::graph::standardnodes;
World::World(std::string name,std::string skyname)
{
	Name=name;
	sky_name=skyname;
}

World::~World()
{
}

//! NameInterface
const char *World::GetNameAsCString()
{
	return Name.c_str();
}

const std::string &World::GetWorldName()
{
	return Name;
}

void World::SetWorldName(const std::string &name)
{
	Name=name;
}

//! WorldInterface
int World::GetStamp()
{
	return 1;
}

const std::string &World::GetSkyName()
{
	return sky_name;
}