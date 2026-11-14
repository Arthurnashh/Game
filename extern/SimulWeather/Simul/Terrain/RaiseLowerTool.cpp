#include "Simul/Terrain/RaiseLowerTool.h"
#include <math.h>
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Terrain/HeightMapInterface.h"
using namespace simul::terrain;
using namespace simul::graph::meta;
using namespace simul::math;

RaiseLowerTool::RaiseLowerTool():ToolBase()
{
	OuterSize=5000.f;
	InnerSize=1000.f;
	Strength=100.f;
	tool_active=false;
}

RaiseLowerTool::~RaiseLowerTool()
{
}

void RaiseLowerTool::ChangedToolSettings()
{
	if(InnerSize>OuterSize*.95f)
		InnerSize=OuterSize*.95f;
}
static int num_steps=24;
//VisualDebugInterface for tools:
unsigned RaiseLowerTool::GetNumLines() const
{
	return num_steps*2;
}

void RaiseLowerTool::GetLine(unsigned l,float* x1,float* x2,int*) const
{
	float MaxHeight=heightMapInterface->GetMaxHeight();
	const float *data=heightMapInterface->GetData();
	int PageSize=heightMapInterface->GetPageSize();
	float rad=OuterSize;
	if((int)l>=num_steps)
	{
		l-=num_steps;
		rad=InnerSize;
	}
	float angle1=2.f*3.141f*(l/(float)num_steps);
	float angle2=2.f*3.141f*((l+1.f)/(float)num_steps);
	
	x1[0]=tool_pos.x+rad*cos(angle1);
	x1[1]=tool_pos.y+rad*sin(angle1);

	x2[0]=tool_pos.x+rad*cos(angle2);
	x2[1]=tool_pos.y+rad*sin(angle2);

	int X,Y;
	ToGrid(X,Y,x1[0],x1[1]);
	x1[2]=MaxHeight*data[PageSize*Y+X];

	ToGrid(X,Y,x2[0],x2[1]);
	x2[2]=MaxHeight*data[PageSize*Y+X];
}

void RaiseLowerTool::UpdatePositionAndDirection(const float p[],const float d[])
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
	Vector3 diff=tool_pos-last_tool_pos;
	diff.z=0;
	float str=1.f;//diff.Magnitude();
	if(mouseButtons[0])//&&diff.Magnitude()>1.f)
		tool_active=true;
	if(!mouseButtons[0])
		tool_active=false;
	if(tool_active)
	{
		if(!shift)
			Raise(str);
		else
			Raise(-str);
	}
}



void RaiseLowerTool::Raise(float h)
{
	float a=InnerSize;
	float b=OuterSize;

	float a2=a*a;
	float a3=a2*a;
	float a4=a3*a;
	float b2=b*b;
	float b3=b2*b;

	float alpha=0.9f;

	h*=Strength;

	float A=-h*(1-alpha+2*a*a/(b*b)*(a/b-3.f/2.f))/(a4+a2*b2-2*a3*b);
	float B=2*h/b3-2*A*b;
	float C=-(2*A*b2+3.f/2.f*B*b);

	int X,Y;
	ToGrid(X,Y,tool_pos.x,tool_pos.y);
	int cover_grid=(int)(heightMapInterface->GetPageSize()*b/heightMapInterface->GetPageWorldX());

	int startX=X-cover_grid;
	int endX=X+cover_grid;
	int startY=Y-cover_grid;
	int endY=Y+cover_grid;

	int PageSize=heightMapInterface->GetPageSize();
	if(startX<0)
		startX=0;
	if(endX>PageSize-1)
		endX=PageSize-1;
	if(startY<0)
		startY=0;
	if(endY>PageSize-1)
		endY=PageSize-1;
	
	for(int i=startX;i<endX;i++)
	{
		for(int j=startY;j<endY;j++)
		{
			float x,y;
			ToMetres(x,y,i,j);
			float dx=x-tool_pos.x;
			float dy=y-tool_pos.y;
			float r=sqrt(dx*dx+dy*dy);
			if(r>b)
				continue;
			float r2=r*r;
			float r3=r2*r;
			float r4=r3*r;
			float dh=A*r4+B*r3+C*r2+h;
			float H=heightMapInterface->GetHeightAt(i,j);
			heightMapInterface->SetHeightAt(i,j,H+dh);
		}
	}
}

bool RaiseLowerTool::IsActive() const
{
	return tool_active;
}
