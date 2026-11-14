#include "precomp.h"
#include "NodeVisitor.h"

using namespace simul::graph::meta;

NodeVisitor::NodeVisitor(bool tp)
{
	traverse_parents=tp;
}

NodeVisitor::~NodeVisitor()
{
}

TimeStepVisitor::TimeStepVisitor():NodeVisitor()
{
	delta_t=0;
}

TimeStepVisitor::~TimeStepVisitor()
{
}

bool TimeStepVisitor::Apply(Node *n)
{
	return n->TimeStep(delta_t);
}


ConstNodeVisitor::ConstNodeVisitor(bool tp)
{
	traverse_parents=tp;
}

ConstNodeVisitor::~ConstNodeVisitor()
{
}
