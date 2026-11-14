#include "Simul/Terrain/ToolBase.h"
#include <math.h>
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Terrain/HeightMapInterface.h"
using namespace simul::terrain;
using namespace simul::graph::meta;
using namespace simul::math;

ToolBase::ToolBase():Node()
{
	for(int i=0;i<6;i++)
		mouseButtons[i]=false;
	shift=alt=ctrl=false;
}

ToolBase::~ToolBase()
{
}

void ToolBase::ChangedToolSettings()
{
}
static int num_steps=24;
//VisualDebugInterface for tools:
unsigned ToolBase::GetNumLines() const
{
	return num_steps;
}

void ToolBase::ToMetres(float &x,float &y,int X,int Y) const
{
	int PageSize=heightMapInterface->GetPageSize();
	float PageWorldX=heightMapInterface->GetPageWorldX();
	float PageWorldZ=heightMapInterface->GetPageWorldZ();
	x=PageWorldX*(float)X/(float)PageSize;
	y=PageWorldZ*(float)Y/(float)PageSize;
}

void ToolBase::ToGrid(int &X,int &Y,float x,float y) const
{
	if(!heightMapInterface)
		return;
	int PageSize=heightMapInterface->GetPageSize();
	float PageWorldX=heightMapInterface->GetPageWorldX();
	float PageWorldZ=heightMapInterface->GetPageWorldZ();
	X=(int)((float)PageSize*x/PageWorldX);
	Y=(int)((float)PageSize*y/PageWorldZ);
	if(X<0)
		X=0;
	if(X>PageSize-1)
		X=PageSize-1;
	if(Y<0)
		Y=0;
	if(Y>PageSize-1)
		Y=PageSize-1;
}

void ToolBase::GetLine(unsigned l,float* x1,float* x2,int*) const
{
	float MaxHeight=heightMapInterface->GetMaxHeight();
	const float *data=heightMapInterface->GetData();
	int PageSize=heightMapInterface->GetPageSize();
	float rad=1000;
	float angle1=2.f*3.141f*(l/(float)num_steps);
	float angle2=2.f*3.141f*((l+1.f)/(float)num_steps);
	
	x1[0]=tool_pos.x+rad*cos(angle1);
	x1[1]=tool_pos.y+rad*sin(angle1);

	x2[0]=tool_pos.x+rad*cos(angle2);
	x2[1]=tool_pos.y+rad*sin(angle2);

	int X,Y;
	ToGrid(X,Y,x1[0],x1[1]);
	x1[2]=MaxHeight*data[PageSize*Y+X]+100;

	ToGrid(X,Y,x2[0],x2[1]);
	x2[2]=MaxHeight*data[PageSize*Y+X]+100;
}

void ToolBase::UpdatePositionAndDirection(const float p[],const float d[])
{
	view_pos=p;
	tool_dir=d;
	Vector3 last_tool_pos=tool_pos;
	if(heightMapInterface)
	{
		float intersection[4];
		heightMapInterface->IntersectLineSegment(view_pos,view_pos+100000*tool_dir,intersection);
		tool_pos=intersection;
	}
}

void ToolBase::SetShiftAltCtrlState(bool s,bool a,bool c)
{
	shift=s;
	alt=a;
	ctrl=c;
}

void ToolBase::SetMouseButtonState(int btn,bool state)
{
	mouseButtons[btn]=state;
}

void ToolBase::SetHeightMapInterface(HeightMapInterface *hmi)
{
	heightMapInterface=hmi;
}
