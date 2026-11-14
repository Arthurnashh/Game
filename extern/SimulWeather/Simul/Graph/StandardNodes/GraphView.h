#ifndef GAMEGRAPHCONTROL_H
#define GAMEGRAPHCONTROL_H
#include <vector>
#include <string>
#include "Simul/Base/PropertyMacros.h"
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/TrackerMacros.h"
#include "Simul/Graph/StandardNodes/NodeSelectionInterface.h"
#include "Simul/Graph/Meta/Resource.h"
#include "Simul/Graph/StandardNodes/Export.h"

// C4251 "class '...' needs to have dll-interface to be used by clients of ..."
// This warning can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
#pragma warning(push)
#pragma warning(disable:4251)
namespace simul
{
	namespace graph
	{
		namespace meta
		{
			class Node;
			class Group;
			typedef std::vector<simul::graph::meta::Node*> NodeStack;
		}
	}
	namespace graph
	{
		namespace standardnodes
		{
			class GraphNode
			{
			public:
				int max_level;
				float x;
				std::string title;
			};
			class GameGraphControlInterface
			{
			public:
				virtual simul::graph::meta::Node *NodeAt(float X,float Y)=0;
			};
			SIMUL_STD_EXPORT_CLASS RedrawCallbackInterface
			{
			public:
				virtual void Redraw()=0;
			};
			SIMUL_STD_EXPORT_CLASS GraphView: public GameGraphControlInterface, public simul::graph::meta::Group,
				public simul::graph::meta::ResourceUser<simul::graph::standardnodes::NodeSelectionInterface>
			{
			public:
				GraphView();
				~GraphView();
				void Init();

				void CreateControls();
				bool ShowToolTips();

				BEGIN_TRACKERS
					DECLARE_PATH_TRACKER(GraphView,simul::graph::meta::Node,GraphNode,nodePathSelector)
				END_TRACKERS
				META_BeginProperties
					META_ValueProperty(float,Zoom,"Zoom level for this view")
					META_ValueProperty(float,XSpacing,"How far apart to draw nodes in the x direction")
					META_ValueProperty(float,YSpacing,"How far apart to draw nodes in the y direction")
					META_ValueProperty(float,RectWidth,"Size to draw nodes in the x direction")
					META_ValueProperty(float,RectHeight,"Size to draw nodes in the y direction")
					META_ValueProperty(float,ClientLeft,"Left edge of client area")
					META_ValueProperty(float,ClientTop,"Top edge of client area")
				META_EndProperties

				META_BeginStates
					META_ValueState(bool,ViewValid,"Is the view that uses this graph view valid? Otherwise a redraw is called for.")
				META_EndStates
				void InitPathToNode(simul::graph::meta::Node *n,GraphNode *t,const simul::graph::meta::NodeStack &path);
				void InitNode(simul::graph::meta::Node *n,GraphNode *t,const simul::graph::meta::NodeStack &path);
				void ModifyingNode(simul::graph::meta::Node *n,GraphNode *t);
				void RemovingNode(simul::graph::meta::Node *n,GraphNode *t);

				void ChildAdded(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path);
				void ChildRemoved(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path);

				simul::graph::meta::Node *NodeAt(float X,float Y);

				struct Tool
				{
					GameGraphControlInterface *Interface;
					void SetInterface(GameGraphControlInterface *i)
					{
						Interface=i;
					}
					virtual bool Start(int ,int ){return true;}
					virtual void Update(int ,int ){}
					virtual void Draw(){}
					virtual void Finish(int ,int ){}
				};
				typedef std::vector<GraphNode*> Level;
				Tool *tool;
				std::vector<Level> LevelMap;
				simul::graph::meta::Node *selected_node;
				void InvalidateView();
				void SetRedrawCallback(RedrawCallbackInterface *i)
				{
					redrawCallbackInterface=i;
				}

			private:
				RedrawCallbackInterface *redrawCallbackInterface;
			};
		}
	}
}
#pragma warning(pop)
#endif