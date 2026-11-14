#include "GraphView.h"
#include "Simul/Base/RealTimeTypes.h"
#include "Simul/Graph/StandardNodes/NameInterface.h"

#include <algorithm>

using namespace simul::graph::standardnodes;
using namespace simul::graph::meta;

struct ConnectTool : public GraphView::Tool
{
	virtual bool Start(int x,int y)
	{
		X=x;
		Y=y;
		StartX=X;
		StartY=Y;
		node1=Interface->NodeAt((float)X,(float)Y);
		return (node1!=NULL);
	}
	virtual void Update(int x,int y)
	{
		X=x;
		Y=y;
	}
	virtual void Draw()
	{
	//	dc.DrawLine(StartX,StartY,X,Y);
	}
	virtual void Finish(int X,int Y)
	{
		if(!node1)
			return;
		node2=Interface->NodeAt((float)X,(float)Y);
		if(!node2)
			return;
		if(node1==node2)
			return;
		if(Y>StartY)
		{
			Group *g1=dynamic_cast<Group*>(node1);
			if(g1)
				g1->AddChild(node2);
		}
		else
		{
			Group *g2=dynamic_cast<Group*>(node2);
			if(g2)
				g2->AddChild(node1);
		}
	}
protected:
	int X,Y,StartX,StartY;
	simul::graph::meta::Node *node1;
	simul::graph::meta::Node *node2;
};

GraphView::GraphView()
{
    Init();
}

GraphView::~GraphView()
{
	delete tool;
}

void GraphView::Init()
{
	tool=NULL;
	ViewValid=false;
	ClientLeft=0;
	ClientTop=0;
	Zoom=50;
	XSpacing=2.4f;
	YSpacing=1.6f;
	RectWidth=2.0f;
	RectHeight=1.f;
	redrawCallbackInterface=NULL;
}

void GraphView::CreateControls()
{
}

void GraphView::InitPathToNode(Node *,GraphNode *t,const simul::graph::meta::NodeStack &path)
{
	if((int)path.size()>t->max_level)
	{
		while(LevelMap.size()<(path.size()+1))
			LevelMap.push_back(Level());
		// this has changed: so
		if(t->max_level>=0)
		{
			Level &l=LevelMap[t->max_level];
			Level::iterator i=find(l.begin(),l.end(),t);
			if(i!=l.end())
				l.erase(i);
		}
		t->max_level=(int)path.size();
		LevelMap[t->max_level].push_back(t);
	}
	Level &L=LevelMap[t->max_level];
	static float separation=2.5f;
//	float x=-separation*((int)L.size()-1)/2.f;
//	UpdateChildLines(t);
	int j=0;
	for(Level::iterator i=L.begin();i!=L.end();i++,j++)
	{
		(*i)->x=j-(float)L.size()/2.f;
	}
}
void GraphView::InitNode(Node *n,GraphNode *t,const NodeStack &path)
{
//	unsigned colour=(unsigned)(*((int*)n));// convert the vftable to a colour
	t->title=simul::base::GetUnqualifiedTypeName(n);
	for(unsigned i=1;i<t->title.size();i++)
	{
		if(t->title[i]>='A'&&t->title[i]<='Z')
		{
			t->title.insert(i," ");
			i++;
		}
	}
	simul::graph::standardnodes::NameInterface *ni=dynamic_cast<simul::graph::standardnodes::NameInterface *>(n);
	if(ni)
	{ 
		t->title+=": ";
		t->title+=ni->GetNameAsCString();
	}
	t->max_level=-1;
	InitPathToNode(n,t,path);
	InvalidateView();
}

void GraphView::InvalidateView()
{
	ViewValid=false;
	if(redrawCallbackInterface)
		redrawCallbackInterface->Redraw();
}

void GraphView::ModifyingNode(Node *,GraphNode *)
{
}

void GraphView::RemovingNode(Node *,GraphNode *t)
{
#if 0
	for(unsigned i=0;i<n->GetNumParents();i++)
	{
		Group *g=n->GetParent(i);
		Node *p=g;
		if(nodePathSelector.selectionMap.find(p)==nodePathSelector.selectionMap.end())
			continue;
		GraphNode *Pt=&nodePathSelector.selectionMap[p]->data;
	}
#endif
	if(t->max_level>=0)
	{
		Level &l=LevelMap[t->max_level];
		Level::iterator i=find(l.begin(),l.end(),t);
		if(i!=l.end())
			l.erase(i);
	}
}

void GraphView::ChildAdded(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path)
{
	//redraw=true;
	nodePathSelector		.ChildAdded(this,node,path);
	Group::ChildAdded(node,path);//< continue traversing.
}
void GraphView::ChildRemoved(Node *node,simul::graph::meta::NodeStack &path)
{
	//redraw=true;
	nodePathSelector		.ChildRemoved(this,node,path);
	Group::ChildRemoved(node,path);//< continue traversing.
}

/*

void GraphView::DrawChildLines(Group *g,wxRect rect,wxDC &dc)
{
	int width=0,height=0;
    GetSize( &width, &height );
	wxPoint p1(rect.GetX()+Zoom*RectWidth/2,rect.GetY()+Zoom*RectHeight);
	wxPoint p2(rect.GetX()+Zoom*RectWidth/2,rect.GetY()+Zoom*(RectHeight+(YSpacing-RectHeight)/2));
	for(unsigned j=0;j<g->GetNumChildren();j++)
	{
		Node *child=g->GetChild(j);
		GraphNode *child_graphnode=&nodePathSelector.selectionMap[child]->data;
		float x2=ClientLeft+child_graphnode->x*XSpacing;
		float y2=ClientTop	+child_graphnode->max_level*YSpacing;
		wxRect rect2(width/2+Zoom*x2,height/2+Zoom*y2,Zoom*RectWidth,Zoom*RectHeight);
		wxPoint p4(rect2.GetX()+Zoom*RectWidth/2,rect2.GetY());
		wxPoint p3(p4.x,p2.y);
		dc.DrawLine(p1,p2);
		dc.DrawLine(p2,p3);
		dc.DrawLine(p3,p4);
	}
}*/

Node *GraphView::NodeAt(float X,float Y)
{
	//int width = 0,height = 0;
  //  GetSize(&width,&height);
//	float level=((Y-height/2)/Zoom-ClientTop)/YSpacing;
//	float along=((X-width/2)/Zoom-ClientLeft)/XSpacing;
	for(PathTracker<GraphView,simul::graph::meta::Node,GraphNode>::selection_map::iterator i
		=nodePathSelector.selectionMap.begin();
		i!=nodePathSelector.selectionMap.end();
		i++)
	{
		float x=i->second->data.x;///XSpacing;
		float y=(float)i->second->data.max_level;///YSpacing;
		//float x_min=width/2+Zoom*x;
		//float y_min=height/2+Zoom*y;
		float x_max=x+RectWidth/XSpacing;
		float y_max=y+RectHeight/YSpacing;
		if(X>x&&X<x_max&&Y>y&&Y<y_max)
		{
			return i->first;
		}
	}
	return NULL;
}