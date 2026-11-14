#include "Simul/Graph/StandardNodes/GraphEditorNode.h"
using namespace simul::graph::standardnodes;
using namespace simul::graph::meta;
GraphEditorNode::GraphEditorNode()
{
	selected_node=NULL;
}

GraphEditorNode::~GraphEditorNode()
{
	Select(NULL);
}

void GraphEditorNode::InitNode(simul::graph::meta::Node *node,NodeSelectionInterface** nsi)
{
	*nsi=dynamic_cast<NodeSelectionInterface*>(node);
}
simul::graph::meta::Node *GraphEditorNode::GetSelection()
{
	return selected_node;
}
void GraphEditorNode::Select(simul::graph::meta::Node *node)
{
	selected_node=node;
	// now inform all children
	for(Tracker<GraphEditorNode,NodeSelectionInterface,NodeSelectionInterface*>::selection_map::iterator i=selector.selectionMap.begin();i!=selector.selectionMap.end();i++)
	{
		i->second->data->Select(node);
	}
}
void GraphEditorNode::DeleteSelected()
{
	if(!selected_node)
		return;
	simul::graph::meta::Node *node=selected_node;
	Select(NULL);
	while(node->GetNumParents())
	{
		node->GetParent(0)->RemoveChild(node);
	}
}

void GraphEditorNode::CutSelected()
{
	clipboard=selected_node;	// smart pointer to the selected node
	DeleteSelected();			// should have at least one reference left, so it's not actually deleted!
}

void GraphEditorNode::PasteAsChildOfSelected()
{
	Group *group=dynamic_cast<Group*>(selected_node);
	if(!group)
		return;
	group->AddChild(clipboard.get());
	clipboard=NULL;
}

void GraphEditorNode::ChildAdded(Node *node,NodeStack &stack)
{
	Resource<NodeSelectionInterface>::ChildAdded(node,this);
	SelectorsChildAdded(node,stack);
	//selector.ChildAdded(this,this,node);
	Group::ChildAdded(node,stack);
}

void GraphEditorNode::ChildRemoved(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path)
{
	if(node==selected_node)
		Select(NULL);
	Resource<NodeSelectionInterface>::ChildRemoved(node,path);
	NodeStack stack;
	SelectorsChildRemoved(node,stack);
	Group::ChildRemoved(node,path);
}