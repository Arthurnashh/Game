#include "Property.h"
using namespace simul::graph::exporttable;

Property::Property()
{
	GetFunction.reset();
	SetFunction.reset();
	GetMinimumFunction.reset();
	GetMaximumFunction.reset();
	GetInfoFunction.reset();
	GetOrdinalFunction.reset();
	declared_in_header=false;
	open=false;
	use_reference=false;
	limits=false;
}
