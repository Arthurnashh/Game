#include <El/elementpch.hpp>

static GlobalAliveInterface GGlobalAliveInterface;
static GlobalAliveInterface *GlobalAlivePtr = &GGlobalAliveInterface;

void GlobalAliveInterface::Set(GlobalAliveInterface *functor)
{
	GlobalAlivePtr = functor;
}

typedef void AtAliveFunction();

TypeIsSimple(AtAliveFunction *);

void GlobalAlive()
{
	GlobalAlivePtr->Alive();
}

void GlobalAtAlive(AtAliveFunction *function)
{
	GlobalAlivePtr->AtAlive(function);
}
void GlobalCancelAtAlive(AtAliveFunction *function)
{
	GlobalAlivePtr->CancelAtAlive(function);
}
