#include "Simul/Terrain/Road.h"
#include <math.h>
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Terrain/HeightMapInterface.h"
#include "Simul/Math/RandomNumberGenerator.h"
#include "Simul/Math/float2.h"
using namespace simul::terrain;
using namespace simul::graph::meta;
using namespace simul::math;

simul::math::RandomNumberGenerator random;

Road::Road(HeightMapInterface *hmi):Node()
	,Width(30.f)
{
	inputHeightMapInterface=hmi;
	length=0;
	data=NULL;
	splineNodes.resize(15);
	Vector3 pos;
	random.Seed(513);
	pos.x=random.FRand(-20000.f,0);
	pos.y=random.FRand(-400.f,400.f);
	pos.z=2000.f;
	for(size_t i=0;i<splineNodes.size();i++)
	{
		SplineNode &sn=splineNodes[i];
		pos.x+=random.FRand( 2500.f,4000.f);
		pos.y+=random.FRand(-4500.f,4500.f);
		if(inputHeightMapInterface)
			pos.z=inputHeightMapInterface->GetHeightAt(pos.x,pos.y);
		sn.pos=pos;
	}
	Changed();
}

Road::~Road()
{
}

void Road::Changed()
{
}

void Road::SetInputHeightMapInterface(HeightMapInterface *hmi)
{
	inputHeightMapInterface=hmi;
	Changed();
}

static int num_steps=3;
//VisualDebugInterface :
unsigned Road::GetNumLines() const
{
	return (unsigned)(num_steps*(splineNodes.size()-3));
}

void Road::GetLine(unsigned l,float* x1,float* x2,int*) const
{
	int spline=l/num_steps;
	int segment=l-spline*num_steps;
	GetSubSegment(spline,segment,x1,x2);
}

void Road::GetSubSegment(unsigned spline,unsigned segment,float* x1,float* x2) const
{
	const SplineNode &sn0=splineNodes[spline];
	const SplineNode &sn1=splineNodes[spline+1];
	const SplineNode &sn2=splineNodes[spline+2];
	const SplineNode &sn3=splineNodes[spline+3];

	float interp1=(float)segment/(float)num_steps;
	float interp2=(float)(segment+1)/(float)num_steps;

	Vector3 X1=simul::math::CatmullRom(interp1,sn0.pos,sn1.pos,sn2.pos,sn3.pos);
	Vector3 X2=simul::math::CatmullRom(interp2,sn0.pos,sn1.pos,sn2.pos,sn3.pos);

	x1[0]=X1.x;
	x1[1]=X1.y;
	x1[2]=X1.z;

	x2[0]=X2.x;
	x2[1]=X2.y;
	x2[2]=X2.z;
}

//RoadInterface
float Road::GetLength() const
{
	return 100.f;
}

int Road::GetNumSegments() const
{
	return (int)(splineNodes.size()-3);
}

int Road::GetSegmentLength() const
{
	return num_steps;
}

const float *Road::GetSegmentData() const
{
	delete [] data;
	data=new float[3*(GetNumSegments()*num_steps+1)*2];
	Vector3 c1,c2;
	Vector3 z(0,0,1.f);
	Vector3 right;
	float *ptr=data;
	for(int i=0;i<GetNumSegments();i++)
	{
		for(int j=0;j<num_steps;j++)
		{
			GetSubSegment(i,j,c1.FloatPointer(0),c2.FloatPointer(0));
			Vector3 road_dir=c2-c1;
			CrossProduct(right,road_dir,z);
			Vector3 x1=c1-Width*right;
			Vector3 x2=c1+Width*right;

            *ptr++ = x1.x;
            *ptr++ = x1.y;
            *ptr++ = x1.z;
            *ptr++ = x2.x;
            *ptr++ = x2.y;
            *ptr++ = x2.z;

			if(j==num_steps-1&&i==GetNumSegments()-1)
			{
				x1=c2-Width*right;
				x2=c2+Width*right;
				*ptr++ = x1.x;
				*ptr++ = x1.y;
				*ptr++ = x1.z;
				*ptr++ = x2.x;
				*ptr++ = x2.y;
				*ptr++ = x2.z;
			}
		}
	}
	return data;
}

int Road::GetNumSubSegments(int ) const
{
	return num_steps;
}

const float *Road::GetCentreLine(float t) const
{
	float ct=t*GetNumSegments();
	unsigned spline=(unsigned)ct;
	if(spline>=(unsigned)GetNumSegments())
		spline=(unsigned)GetNumSegments()-1;
	float interp=ct-(float)spline;
	return GetPosOnSegment(spline,interp);
}

const float *Road::GetEdgeOffset(float t) const
{
	float ct=t*GetNumSegments();
	unsigned spline=(unsigned)ct;
	if(spline>=(unsigned)GetNumSegments())
		spline=(unsigned)GetNumSegments()-1;
	float interp=ct-(float)spline;
	float2 x1=GetPosOnSegment(spline,interp);
	float2 x2=GetPosOnSegment(spline,interp+0.001f);
	float2 dx=x2-x1;
	float2 side(-dx.y,dx.x);
	side/=float2::length(side);
	side*=Width;
	return side;
}

const float *Road::GetPosOnSegment(unsigned spline,float interp) const
{
	static float x[4];
	const SplineNode &sn0=splineNodes[spline];
	const SplineNode &sn1=splineNodes[spline+1];
	const SplineNode &sn2=splineNodes[spline+2];
	const SplineNode &sn3=splineNodes[spline+3];

	Vector3 X=simul::math::CatmullRom(interp,sn0.pos,sn1.pos,sn2.pos,sn3.pos);

	x[0]=X.x;
	x[1]=X.y;
	x[2]=X.z;

	return x;
}