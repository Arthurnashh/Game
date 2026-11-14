#ifndef GRAPHEDITORNODE_H
#define GRAPHEDITORNODE_H

#include "Simul/Graph/StandardNodes/NodeSelectionInterface.h"
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/Tracker.h"
#include "Simul/Graph/Meta/Resource.h"
#include "Simul/Graph/Meta/TrackerMacros.h"
#include "Simul/Graph/StandardNodes/Export.h"
#pragma warning(push)
// C4251 "class '...' needs to have dll-interface to be used by clients of ..."
// This warning can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
#pragma warning(disable:4251)
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			//! A node that can edit the subgraph beneath it.
			SIMUL_STD_EXPORT_CLASS GraphEditorNode:public simul::graph::meta::Group,
				public NodeSelectionInterface
				,public simul::graph::meta::Resource<NodeSelectionInterface>
			{
				simul::graph::meta::Node *selected_node;
				simul::base::SmartPtr<simul::graph::meta::Node> clipboard;
			protected:
				virtual void InitNode(simul::graph::meta::Node *,NodeSelectionInterface**);
			public:
				GraphEditorNode();
				virtual ~GraphEditorNode();
			public:
				BEGIN_TRACKERS
					DECLARE_TRACKER(GraphEditorNode,NodeSelectionInterface,NodeSelectionInterface*,selector)
				END_TRACKERS
// Required for a TrackerInterface:
				virtual simul::graph::meta::Node *GetSelection();
				virtual void Select(simul::graph::meta::Node *node);
				virtual void DeleteSelected();
				virtual void CutSelected();
				virtual void PasteAsChildOfSelected();
			//! Inform this parent that a child has been removed, and recurse up the graph
				virtual void ChildAdded(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path);
				virtual void ChildRemoved(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path);
				simul::graph::meta::Node *GetSelectedNode(){return selected_node;}
			};
		}
	}
}
#pragma warning(pop)
#endif // ifndef WORLD_H

