#include <algorithm>
#include <math.h>
#pragma warning(push)
#pragma warning(disable:4748)
#ifndef SIMUL_XML_DLL
#define SIMUL_XML_DLL
#endif

#include "Simul/Graph/XML/XMLNode.h"
#include "Simul/External/xmlParser/xmlParser.h"
#include "Simul/Reflection/MetaPropertyInterface.h"
#include "Simul/Reflection/NodeFactory.h"
#include <fstream>		
#include <math.h>
// C4512 - "assignment operator could not be generated". Boost classes cause this warning.
#pragma warning(push)
#pragma warning(disable:4512)
#include <boost/algorithm/string/replace.hpp>		// Using boost's string search-replace.
#pragma warning(pop)
#pragma comment(lib,"xmlParser.lib")

using namespace simul::graph::xml;
using namespace simul::graph::meta;
using namespace simul::reflection;
using std::string;

unsigned long GenerateRandomId()
{
	return rand();
}

simul::graph::xml::XMLNode::XMLNode(): Group()
{
}

simul::graph::xml::XMLNode::~XMLNode()
{
}

void NodeToXML(const MetaPropertyInterface &mpi,const PropertyList &L,::XMLNode &xNode)
{
	using ::XMLNode;
	for(size_t i=0;i<L.size();i++)
	{
		const InterfaceProperty *P=L[i].get();
		if(P->ReadOnly)
			continue;
		std::string name=boost::algorithm::replace_all_copy(P->Name,"::",":");
		bool is_property=name.find("Property-")<name.length();
		name=boost::algorithm::replace_all_copy(name,"Property-","");
		std::string subnode_type;
		if(P->Type=="BaseClass")
			subnode_type="Class";
		else
			subnode_type="Property";
		XMLNode xSubNode=xNode.addChild(subnode_type.c_str());
		if(is_property)
		{
			xSubNode.addAttribute("Name",name.c_str());
			xSubNode.addAttribute("Type",P->Type.c_str());
			xSubNode.addAttribute("Value",mpi.GetValue(P->Name).c_str());
		}
		else
		{
			xSubNode.addAttribute("Type",name.c_str());
		}
		if(P->SubProperties.size())
			NodeToXML(mpi,P->SubProperties,xSubNode);
	}
}

void XMLToNode(MetaPropertyInterface &mpi,const PropertyList &L,::XMLNode &xNode)
{
	using ::XMLNode;
	for(int i=0;i<xNode.nChildNode();i++)
	{
		XMLNode xSubNode=xNode.getChildNode(i);
		std::string name=xSubNode.getName();
		if(name=="Property")
		{
			if(xSubNode.getAttribute("Name"))
			{
				std::string prop_name=string("Property-")+xSubNode.getAttribute("Name");
				mpi.SetProperty(prop_name,xSubNode.getAttribute("Value"));
			}
		}
		if(name=="Class")
		{
			std::string class_name=xSubNode.getAttribute("Type");
			boost::algorithm::replace_all(class_name,":","::");
			for(size_t i=0;i<L.size();i++)
			{
				InterfaceProperty *ip=L[i].get();
				if(ip->Name==class_name)
				{
					XMLToNode(mpi,ip->SubProperties,xSubNode);
				}
				else
				{
					XMLToNode(mpi,ip->SubProperties,xNode);
				}
			}
		}
	}
}

void XMLToGraph(simul::graph::xml::XMLNode *xmlgroup,::XMLNode &xNode)
{
	using ::XMLNode;
	std::string name=xNode.getName();
	if(name=="xml")
	{
		for(int i=0;i<xNode.nChildNode();i++)
		{
			XMLNode xmlnode=xNode.getChildNode(i);
			XMLToGraph(xmlgroup,xmlnode);
		}
	}
	if(name=="Graph")
	{
		for(int i=0;i<xNode.nChildNode();i++)
		{
			XMLNode xSubNode=xNode.getChildNode(i);
			XMLToGraph(xmlgroup,xSubNode);
		}
	}
	if(name=="Node")
	{
		string IdString=xNode.getAttribute("Id");
		simul::graph::xml::XMLNode::TrackId Id;
		sscanf_s(IdString.c_str(),"0x%X",&Id);
		simul::graph::meta::Group *group=dynamic_cast<simul::graph::meta::Group *>(xmlgroup->tracker.FindNode(Id));
		for(int i=0;i<xNode.nChildNode();i++)
		{
			XMLNode xSubNode=xNode.getChildNode(i);
			string nodeName=xSubNode.getName();
			if(nodeName=="Children")
			{
				for(int j=0;j<xSubNode.nChildNode();j++)
				{
					XMLNode xChildNode=xSubNode.getChildNode(j);
					xChildNode.getName();
					string child_idString=xChildNode.getAttribute("Id");
					simul::graph::xml::XMLNode::TrackId child_id;
					sscanf_s(child_idString.c_str(),"0x%X",&child_id);
					simul::graph::meta::Node *node=(xmlgroup->tracker.FindNode(child_id));
					if(node)
						group->AddChild(node);
				}
			}
		}
	}
}

simul::graph::meta::Node *root=NULL;
void XMLToNodes(simul::graph::xml::XMLNode *xmlgroup,::XMLNode &xNode)
{
	using ::XMLNode;
	std::string name=xNode.getName();
	if(name=="xml")
	{
		for(int i=0;i<xNode.nChildNode();i++)
		{
			XMLNode xSubNode=xNode.getChildNode(i);
			XMLToNodes(xmlgroup,xSubNode);
		}
	}
	if(name=="Graph")
	{
		for(int i=0;i<xNode.nChildNode();i++)
		{
			XMLNode xSubNode=xNode.getChildNode(i);
			XMLToNodes(xmlgroup,xSubNode);
		}
	}
	if(name=="Node")
	{
		string IdString=xNode.getAttribute("Id");
		const char * root_string=xNode.getAttribute("Root");
		string RootString;
		if(root_string)
			RootString=root_string;
		simul::graph::xml::XMLNode::TrackId Id;
		sscanf_s(IdString.c_str(),"0x%X",&Id);

		std::string class_name=xNode.getAttribute("Class");
		boost::algorithm::replace_all(class_name,":","::");
		boost::algorithm::replace_all(class_name,"class ","");
		Node *node=NodeFactory::GetInstance().Create(class_name);
		if(RootString=="true")
			root=node;
		if(!node)
			return;
		xmlgroup->AddChild(node);
		xmlgroup->tracker.selectionMap[node]->data=Id;

		MetaPropertyInterface mpi(NULL);
		mpi.SetFlat(false);
		mpi.SetNode(node);
		mpi.InitializeIfNecessary();
		const PropertyList &L=mpi.GetProperties();
		XMLToNode(mpi,L,xNode);
	}
} 
			
// Create an XML tree of a single simul Node and its base classes
void simul::graph::xml::XMLNode::SaveNode(Node *node,const string &filename,bool include_subgraph)
{
	MetaPropertyInterface mpi(NULL);
	mpi.SetFlat(false);
	mpi.SetNode(node);
	mpi.InitializeIfNecessary();
	const PropertyList &L=mpi.GetProperties();

	using ::XMLNode;
	XMLNode xMainNode=XMLNode::createXMLTopNode("xml",TRUE);
    xMainNode.addAttribute("version","1.0");
	if(include_subgraph)
	{
		XMLNode graphNode=xMainNode.addChild("Graph");
		// child this node to make the list of the subgraph:
		AddChild(node);
		for(Typetracker::selection_map::iterator i=tracker.selectionMap.begin();i!=tracker.selectionMap.end();i++)
		{
			i->second->data=GenerateRandomId();
		}
		for(Typetracker::selection_map::iterator i=tracker.selectionMap.begin();i!=tracker.selectionMap.end();i++)
		{
			Node *n=i->first;
			char txt[100];
			sprintf(txt,"0x%X",i->second->data);
			XMLNode xmlnode=graphNode.addChild("Node");
			if(n==node)
				xmlnode.addAttribute("Root","true");
			xmlnode.addAttribute("Id",txt);

			const type_info &ti=typeid(*(n));
			std::string class_name=ti.name();
			
			boost::algorithm::replace_all(class_name,"::",":");
			xmlnode.addAttribute("Class",class_name.c_str());
			mpi.SetNode(n);
			mpi.UnInitialize();
			mpi.InitializeIfNecessary();
			NodeToXML(mpi,L,xmlnode);

			Group *g=dynamic_cast<Group*>(n);
			if(g)
			{
				XMLNode xSubNode=xmlnode.addChild("Children");
				for(unsigned int j=0;j<g->GetNumChildren();j++)
				{
					Node *child=g->GetChild(j);
					assert(tracker.selectionMap.find(child)!=tracker.selectionMap.end());
					TrackId child_id=tracker.selectionMap[child]->data;
					XMLNode xChildNode=xSubNode.addChild("Child");
					sprintf(txt,"0x%X",child_id);
					xChildNode.addAttribute("Id",txt);
				}
			}
		}
	}
	else
	{
		//just a single node:
		NodeToXML(mpi,L,xMainNode);
	}
	xMainNode.writeToFile(filename.c_str());
	if(include_subgraph)
	{
		RemoveChild(node);
	}
}
void simul::graph::xml::XMLNode::LoadNode(Node *node,const string &filename,bool )
{
	using ::XMLNode;
	{
		std::ifstream ifs(filename.c_str());
		if(!ifs.good())
			return;
	}
	try
	{
		XMLNode xMainNode=XMLNode::openFileHelper(filename.c_str());
		MetaPropertyInterface mpi(NULL);
		mpi.SetFlat(false);
		mpi.SetNode(node);
		mpi.InitializeIfNecessary();
		const PropertyList &L=mpi.GetProperties();
		XMLToNode(mpi,L,xMainNode);
	}
	catch(...)
	{
		return;
	}
}

simul::graph::meta::Node *simul::graph::xml::XMLNode::LoadNode(const std::string &filename,bool)
{
	using ::XMLNode;
	{
		std::ifstream ifs(filename.c_str());
		if(!ifs.good())
			return NULL;
	}
	try
	{
		root=NULL;
		XMLNode xMainNode=XMLNode::openFileHelper(filename.c_str());
		XMLToNodes(this,xMainNode);
		XMLToGraph(this,xMainNode);
		simul::base::SmartPtr<simul::graph::meta::Node> node=root;
		return node.get();
	/*	for(int i=0;i<xMainNode.nChildNode();i++)
		{
			XMLNode xSubNode=xNode.getChildNode(i);
			std::string name=xSubNode.getName();		
			if(name=="Class")
			{
			}
			{
				std::string class_name=xSubNode.getAttribute("Type");
		}*/
	}
	catch(...)
	{
		return NULL;
	}
}
#pragma warning(pop)