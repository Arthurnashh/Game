#include "Simul/Graph/StandardNodes/NamedGroup.h"
#include <algorithm>

using namespace simul::graph::meta;
using namespace simul::graph::standardnodes;
using std::string;

NamedGroup::NamedGroup():Group()
{
}

NamedGroup::NamedGroup(const std::string &n):Group()
{
	name=n;
}

NamedGroup::~NamedGroup()
{
}
