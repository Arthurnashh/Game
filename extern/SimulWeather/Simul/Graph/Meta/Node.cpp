#include "precomp.h"
#include "Simul/Graph/Meta/Node.h"
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/NodeVisitor.h"
#include <algorithm>
#include <assert.h>
#ifdef WIN32 //RVK TEMP!!
#include "Simul/Base/RealTimeTypes.h"
#endif

#include "Simul/Base/SmartPtr.h"

using namespace simul::graph::meta;
using std::string;


Node::Node()
{
}

Node::~Node()
{
	while(parents.size())
    {
		parents[0]->RemoveChild(this);
    }
}

void Node::Clear()
{
}

void Node::operator=(const Node &)
{
}

void Node::Accept(NodeVisitor& nv)
{
	if(nv.Apply(this)&&nv.ShouldTraverseParents())
		TraverseParents(nv);
	nv.PostTraversal(this);
}

void Node::Accept(ConstNodeVisitor& nv) const
{
	if(nv.Apply(this)&&nv.ShouldTraverseParents())
		TraverseParents(nv);
	nv.PostTraversal(this);
}

void Node::TraverseParents(ConstNodeVisitor& nv) const
{
	for(unsigned i=0;i<parents.size();i++)
    {
		parents[i]->Accept(nv);
    }
}

void Node::TraverseParents(NodeVisitor& nv)
{
	for(unsigned i=0;i<parents.size();i++)
    {
		simul::base::SmartPtr<Node> itr=parents[i];
        (itr)->Accept(nv);
    }
}

bool Node::TimeStep(float)
{
	return true;
}
void Node::AddParent(Group* g)
{
    parents.push_back(g);
	g->RecurseChildAdded(this);
}

void Node::RemoveParent(Group* g)
{
	g->RecurseChildRemoved(this);
    ParentList::iterator pitr = std::find(parents.begin(),parents.end(),g);
    if(pitr!=parents.end())
		parents.erase(pitr);
}
void Node::RemoveFromAllParents()
{
	ParentList p=parents;
    for(ParentList::iterator i=p.begin();i!=p.end();i++)
		(*i)->RemoveChild(this);
	// "this" ptr MAY now be invalid!
}
bool Node::ReParent(Group *new_parent)
{
	if(GetNumParents()!=1)
		return false;
	if(!new_parent)
		return false;
	if(new_parent==parents[0])
		return true;
#ifdef DEBUG
	if(new_parent->HasAncestor(this))
		return false;
#endif
	unsigned index=(unsigned)parents[0]->GetChildIndex(this);
	simul::base::SmartPtr<Node> ptr=parents[0]->GetChildPtr(index);
	parents[0]->RemoveChild(this);
	new_parent->AddChild(ptr);
	return true;
}

bool Node::HasAncestor(Node *node)
{
    for(ParentList::iterator pitr = parents.begin();pitr!=parents.end();pitr++)
	{
		if(static_cast<Node*>(*pitr)==node)
			return true;
		if((*pitr)->HasAncestor(node))
			return true;
	}
	return false;
}
	
std::ostream &Node::RecursiveStreamOut(std::ostream &os,bool ) const
{
	if(os.eof())
		return os;
	return StreamOut(os);
}
std::istream &Node::RecursiveStreamIn(std::istream &is,bool )
{
	if(is.eof())
		return is;
	return StreamIn(is);
}
	
std::ostream &Node::RecursiveStateStreamOut(std::ostream &os,bool) const
{
	if(os.eof())
		return os;
	return StateStreamOut(os);
}
std::istream &Node::RecursiveStateStreamIn(std::istream &is,bool)
{
	if(is.eof())
		return is;
	return StateStreamIn(is);
}

std::ostream &Node::StreamOut(std::ostream &os) const
{
	StreamBlockMarker marker=NODE_TYPE;
	os.write((const char*)&marker,sizeof(marker));
#if !defined(__BORLANDC__) && defined(WIN32)
	string str=simul::base::GetQualifiedTypeName(this);
	unsigned len=(unsigned int)str.length();
	os.write((const char*)&len,sizeof(len));
	os.write(str.c_str(),len);
#else
   	unsigned len=0;
	os.write((const char*)&len,sizeof(len));
#endif
	marker=NODE_DATA;
	os.write((const char*)&marker,sizeof(marker));
	return os;
}

std::istream &Node::StreamIn(std::istream &is)
{
	StreamBlockMarker marker;
	is.read((char*)&marker,sizeof(marker));
	if(marker==NODE_TYPE)
	{
		string typestring;
		unsigned len=0;
		is.read((char*)&len,sizeof(len));
		for(unsigned i=0;i<len;i++)
		{
			char c;
			is.read(&c,sizeof(c));
			typestring+=c;
		}
#if !defined(__BORLANDC__) && defined(WIN32)
		assert(typestring==simul::base::GetQualifiedTypeName(this));   
#endif
		is.read((char*)&marker,sizeof(marker));
	}
	assert(marker==NODE_DATA);
	return is;
}
/*
Node *Node::LoadFromStream(std::istream &is)
{
	StreamBlockMarker marker;
	is.read((char*)&marker,sizeof(marker));
	assert(marker==NODE_TYPE);
	

	string typestring;
	unsigned len=0;
	is.read((char*)&len,sizeof(len));
	for(unsigned i=0;i<len;i++)
	{
		char c;
		is.read(&c,sizeof(c));
		typestring+=c;
	}
	//	is.read((char*)&marker,sizeof(marker));

	//assert(marker==NODE_DATA);
	Node *N=NodeFactory::GetInstance().Create(typestring);
	N->RecursiveStreamIn(is);
	return N;
}
*/
namespace simul
{
	namespace graph
	{
		namespace meta
		{
			std::ostream &operator<<(std::ostream &os, const Node &N)
			{
				return N.RecursiveStreamOut(os,false);
			}
			std::istream &operator>>(std::istream &is, Node &N)
			{
				return N.RecursiveStreamIn(is,false);
			}
		}
	}
}
