#include "precomp.h"

#include "Simul/Clouds/ThunderCloudNode.h"
#include "Simul/Math/Noise3D.h"
#include "Simul/Base/EnvironmentVariables.h"
#include "Simul/Math/Pi.h"
#include "Simul/Math/Noise1D.h"
#include "Simul/Geometry/Orientation.h"
#include "Simul/Graph/Meta/MemoryInterface.h"
#include <math.h>
#include <fstream>

// Density of pure water vapour: 0.627 kg/m3

using namespace simul::clouds;
using namespace simul::math;
using namespace simul::base;
using namespace simul::graph::meta;
float nn=1.f;
float n0=0.f;


template<class T> void Limit(T &t,T minimum,T maximum)
{
	if(t<minimum)
		t=minimum;
	if(t>maximum)
		t=maximum;
}

ThunderCloudNode::ThunderCloudNode()
	: FastCloudNode()
	, LightningCentreX(0,0,0)
	,LightningEnabled(false)
{
	lightning_colour[0]=1.5f;
	lightning_colour[1]=1.5f;
	lightning_colour[2]=2.4f;
	lightning_colour[3]=1.f;
	InitializePropertiesDefinition();
	SegmentLength=125.f;
	MemoryInterface *mi=ResourceUser<MemoryInterface>::GetResourceInterface();
	if(mi)
	{
		noise1d=new(mi) Noise1D(mi);
	}
	else
	{
		noise1d=new Noise1D();
	}
	noise1d->Setup(4,1,3);
	time_step=0.f;
	building_source=NULL;
}

void ThunderCloudNode::PrepareToDestroy()
{
	MemoryInterface *mi=ResourceUser<MemoryInterface>::GetResourceInterface();
	if(mi)
	{
		operator delete[](noise1d,mi);
	}
	else
	{
		delete noise1d;
	}
	noise1d=NULL;
	CellularCloudNode::PrepareToDestroy();
}

ThunderCloudNode::~ThunderCloudNode()
{

}

bool ThunderCloudNode::GenerateBranch(CloudInterface *ci)
{
	Branch *branch=building_branches_stack.back();
	const Vector3 &x0=branch->root_pos;
	static float length_factor=0.8f;
	int max_segments=(int)(building_max_segments*pow(length_factor,(float)(building_branches_stack.size()-1.f)));
	Vector3 X=x0;
	float distance=branch->root_distance;
	static float dd=0.01f;
	float azimuth=branch->azimuth;
	float root_elevation=branch->root_elevation;
	float elevation=branch->root_elevation;
	float texel_height=CloudHeight/(float)GridHeight;
	for(int i=0;i<max_segments;i++,distance+=dd)
	{
		azimuth+=random.FRand(-AzimuthKinkAngle,AzimuthKinkAngle);
		elevation+=random.FRand(-KinkAngle,KinkAngle);
		elevation=(1.f-Straighten)*elevation+Straighten*(root_elevation);
		branch->segments.push_back(Segment());
		Segment &S=branch->segments[branch->segments.size()-1];
		S.x1=X;
		S.x2=X;
		S.x2.z+=building_length*sin(elevation);
		S.x2.y+=building_length*cos(elevation)*sin(azimuth);
		S.x2.x+=building_length*cos(elevation)*cos(azimuth);
		X=S.x2;
		S.distance=distance;
		float dens1=0.f,dens2=0.f;
		if(S.x1.z>CloudBaseZ+texel_height)
			dens1=ci->GetCloudDensity(S.x1+WindOffset);
		if(S.x2.z>CloudBaseZ+texel_height)
			dens2=ci->GetCloudDensity(S.x1+WindOffset);
		S.bright1=1.f-dens1;
		S.bright2=1.f-dens2;
		if(X.z<0)
		{
			building_max_segments=max_segments=i;
			break;
		}
	}
	if(distance>building_source->return_stroke)
		building_source->return_stroke=distance;
	branch->branches.resize(0);
	static int max_branch=11;
	static int min_branch=7;
	branch->branch_limit=(unsigned)((random.FRand((float)min_branch,(float)max_branch)));
	//do {} while(BranchLightning());
	return (branch->branch_limit!=0);
	//return false;
}

bool ThunderCloudNode::BranchLightning()
{
	static float bunch=1.5f;
	static size_t max_stack=3;
	Branch *branch=building_branches_stack.back();
	if(building_branches_stack.size()>=max_stack)
		return false;
	if(branch->segments.size()<2)
		return false;
	//for(unsigned i=0;i<branch->branch_limit;i++)
	{
		float rr=pow(random.FRand(),bunch);
		//float rr=(float)new_branch_index/(float)branch->branch_limit;
		size_t index=1+(size_t)(rr*(branch->segments.size()-2));
		Segment *S=&branch->segments[index];
		Vector3 X=S->x1;

		// dx is the direction the current bolt is pointing in
		Vector3 dx=S->x2-S->x1;
		dx.Unit();

		Vector3 p1,p2;
		CrossProduct(p1,Vector3(1,0,0.f),dx);
		p1.Unit();
		// This is the turn angle.
		float this_branch_angle=BranchAngle+random.FRand(-BranchAngleVariance,BranchAngleVariance);
		Quaternion q(this_branch_angle,p1);
		p2=q*dx;

		// now rotate by the local azimuth between -90 and +90 degrees:
		float local_azimuth=random.FRand(-0.5f*pi,0.5f*pi);
		q.Define(local_azimuth,dx);
		Vector3 dx_new=q*p2;
		dx_new.Unit();

		float new_el=asin(dx_new.z);
		float new_az=atan2(dx_new.y,dx_new.x);
		if(building_source->branches.size()>=MaxBranches)
			return false;
		if(branch->branches.size()<branch->branch_limit&&index<branch->segments.size())
		{

			building_source->branches.push_back(Branch());
			Branch *b=&building_source->branches[building_source->branches.size()-1];
			building_branches_stack.push_back(b);
		/*	if(GetCloudDensity(X+WindOffset)!=0.f)
			{
				return false;
			}*/
			b->width=branch->width*BranchWidthFactor;
			b->azimuth=new_az;
			b->root_elevation=new_el;
			static float ss=.75f;
			b->spread=ss*branch->spread;
			static float dist_spread=0.f;
			float new_root_dist=branch->segments[index].distance+(dist_spread*random.FRand(.5f,1.5f))*index;
			b->root_pos=X;
			b->root_distance=new_root_dist;
			branch->branches.push_back(b);
			return true;
		}
	}
	return false;
}

static float return_spread=.875f;
static float return_stroke=1.125f;
bool ThunderCloudNode::UpdateLightning()
{
	static float eee=2.f;
	if(!building_source)
	{
		building_source=SeedLightning();
		if(!building_source)
			return false;
		GenerateBranch(this);
	}
	for(int i=0;i<3;i++)
	{
		if(building_branches_stack.size())
		{
			if(BranchLightning())
				GenerateBranch(this);
			else
				building_branches_stack.pop_back();
		}
		else
		{
			if(building_source)
			{
				building_source->return_spread=return_stroke+random.FRand(-return_spread,return_spread);

				building_source->end=building_source->return_stroke+eee*building_source->return_spread;
				if(building_branches_stack.size())
					building_branches_stack.pop_back();
				building_source->building=false;
				building_source=NULL;
			}
			return false;
		}
	}
	return true;
}

void ThunderCloudNode::GenerateLightning()
{
	do
	{
	}
	while (UpdateLightning());
}

ThunderCloudNode::Source *ThunderCloudNode::SeedLightning()
{
	size_t source_index=sources.size();
	static size_t max_sources=4;
	if(sources.size()>=max_sources)
	{
		for(size_t i=0;i<sources.size();i++)
		{
			Source *s=&sources[i];
			if(s->finished)
			{
				source_index=i;
				break;
			}
		}
		if(source_index>=max_sources)
			return NULL;
	}
	else
	{
		sources.push_back(Source());
	}
	GetExtents(X1,X2);
	Subtract(DX,X2,X1);
	Vector3 X;
	static float spread_min=1.5f;
	static float spread_max=2.5f;

	for(int tries=0;tries<32;tries++)
	{
		static float angular_start=0.f;
		static float angular_range=60.f;
		float azimuth=pi/180.f*(angular_start+random.FRand(0,angular_range));
		float distance=random.FRand(LightningZoneMin,0.5f*LightningZoneSize);
		X.Define(distance*sin(azimuth),distance*cos(azimuth),X1.z);
		X+=LightningCentreX;
		X.z=X1.z;
		for(unsigned i=0;i<GridHeight;i++)
		{
			if(i&&GetCloudDensity(X+WindOffset)!=0.f)
				break;
			X.z+=DX.z/(float)GridHeight;
		}
		if(GetCloudDensity(X+WindOffset))
			break;
	}
	Source &source=sources[source_index];
	source.started=false;
	if(!GetCloudDensity(X+WindOffset))
	{
		source.finished=true;
		source.started=false;
		return NULL;
	}
	X.z=X1.z+DX.z/(float)GridHeight;
	source.building=true;
	source.finished=false;
	source.root_pos=X;
	source.progress=0.f;
	source.noise_multiplier=0.f;
	source.branches.reserve(MaxBranches+1);
	source.branches.resize(0);

	source.branches.push_back(Branch());
	Branch *branch=&source.branches[0];
	branch->spread=random.FRand(spread_min,spread_max);
	branch->width=random.FRand(.8f,1.2f);
	source.end=0;

	Branch *b=&source.branches[0];
	b->azimuth=random.FRand(0,2.f*pi);
	b->root_elevation=-pi/2.f;
	source.return_stroke=0.f;
	static float init_dist=0.f;
	float init=init_dist;
	building_branches_stack.resize(0);
	b->root_pos=source.root_pos;
	b->root_distance=init;
	building_branches_stack.push_back(b);
	static int ss=25;
	building_max_segments=ss;
	SegmentLength=X.z/(float)building_max_segments;
	building_length=SegmentLength;

	return &source;
}

unsigned ThunderCloudNode::GetNumBranches(unsigned source_index) const
{
	if(sources[source_index].building||sources[source_index].finished||!sources[source_index].started)
		return 0;
	return (unsigned)(sources[source_index].branches.size());
}

float ThunderCloudNode::GetBranchWidth(unsigned source_index,unsigned branch_index) const
{
	return sources[source_index].branches[branch_index].width;
}

unsigned ThunderCloudNode::GetNumSegments(unsigned source_index,unsigned branch_index) const
{
	return (unsigned)(sources[source_index].branches[branch_index].segments.size());
}

static float s_extend=0.005f;
static float aaa=2.f;
static float extra_light=0.7f;

void ThunderCloudNode::GetSegmentVertex(unsigned source_index,unsigned branch_index,unsigned index,float &bright1,float x[]) const
{
	bright1=0;
	const Source &source=sources[source_index];
	if(!source.branches.size())
		return;
	const Branch *branch=&source.branches[branch_index];
	if(!branch->segments.size())
		return;
	const Segment &S=branch->segments[index];
	x[0]=S.x1.x;
	x[1]=S.x1.y;
	x[2]=S.x1.z;
	float bright=0;
	{
		float n=nn*source.noise_multiplier;
		float rel=source.progress-source.return_stroke;
		if(rel>0&&branch_index<1)
		{
			float peak=exp(-rel*rel/(source.return_spread*source.return_spread));
			bright=std::max(0.f,peak-n);
		}
		else
		{
			bright=0.f;
		}
	}
	{
		float n=n0*source.noise_multiplier;
		float rel=source.progress-S.distance;
		float s=(time_step+s_extend)*branch->spread;
		float peak=rel/s*exp(-rel/s)*(exp(1.f));
		bright+=std::max(0.f,peak-n);
	}
	bright*=branch->width;
	bright+=extra_light*std::min(source.progress/source.return_stroke,(float)exp(-aaa*source.progress));
	bright1=bright*S.bright1;
}

void ThunderCloudNode::GetSegmentVertexDirection(unsigned source_index,unsigned branch_index,unsigned segment_index,float d[]) const
{
	const Source &source=sources[source_index];
	if(!source.branches.size())
		return;
	const Branch *branch=&source.branches[branch_index];
	if(!branch->segments.size())
		return;
	const Segment &S2=branch->segments[segment_index];
	simul::math::Vector3 dir=S2.x2-S2.x1;
	dir.Normalize();
	if(segment_index>0)
	{
		const Segment &S1=branch->segments[segment_index-1];
		simul::math::Vector3 dir2=S1.x2-S1.x1;
		dir2.Normalize();
		dir*=.5f;
		dir+=0.5f*dir2;
	}
	d[0]=dir.x;
	d[1]=dir.y;
	d[2]=dir.z;
}

static float max_light=0.5f;

bool ThunderCloudNode::TimeStep(float dt)
{
	static float ff=8640.f;
	if(LightningEnabled)
	{
		UpdateLightning();
		time_step=ff*dt;
		for(size_t i=0;i<sources.size();i++)
		{
			Source &source=sources[i];
			//if(CanStartSource(i))
			//	StartSource(i);
			if(!source.started)
				continue;
			if(source.finished)
				continue;
			source.progress+=time_step;
			source.noise_multiplier=noise1d->PerlinNoise1D(source.progress);
			if(source.progress>source.end)
			{
				source.progress=0;
				source.finished=true;
				source.started=false;
			}
		}
	}
	return FastCloudNode::TimeStep(dt);
}

unsigned ThunderCloudNode::GetNumLightSources() const
{
	return (unsigned)sources.size();
}

float ThunderCloudNode::GetLightAt(unsigned source_index,const float x[]) const
{
	simul::math::Vector3 X(x);
	float total_light=0;
	simul::math::Vector3 dx;
	static float mm=400.f;
	const Source &source=sources[source_index];
	if(source.branches.size())
	{
		const Branch &branch=source.branches[0];
		static int steps=10;
		size_t Step=std::max((size_t)1,branch.segments.size()/steps);
		for(size_t j=0;j<branch.segments.size();j+=Step)
		{
			Subtract(dx,branch.segments[j].x1,X);
			//dx.z=0;
			total_light+=1.f/dx.SquareSize();
		}
	}
	total_light*=mm*mm;
	static float ext_m=0.001f;
	if(X.z>this->CloudBaseZ)
	{
		total_light*=exp(ext_m*this->Extinction*(CloudBaseZ-X.z)/(4.f*pi));
	}
	if(total_light>1.f)
		total_light=1.f;
	if(total_light>max_light)
		max_light=total_light;
	return total_light;
}

float ThunderCloudNode::GetLightSourceProgress(unsigned source_index) const
{
	if(source_index>=sources.size())
		return 0.f;
	return sources[source_index].progress;
}

float ThunderCloudNode::GetLightSourceBrightness(unsigned source_index) const
{
  if (source_index >= sources.size())
    return 0.0f;
	const Source &source=sources[source_index];
	if(source.building||source.finished||!source.started)
		return 0.f;
	float bright=0;
	if(source.branches.size())
	{
		float n=n0*source.noise_multiplier;
		const Branch *branch=&source.branches[0];
		float s=(time_step+s_extend)*branch->spread;
		static unsigned steps=5;
		for(unsigned i=0;i<branch->segments.size();i+=steps)
		{
			const Segment &S=branch->segments[i];
			float rel=source.progress-S.distance;
			float peak=exp(-rel*rel/(s*s));
			bright+=S.bright1*std::max(0.f,peak-n);
		}
		bright*=steps;
		bright/=(float)source.branches[0].segments.size();
	}

	float n=nn*source.noise_multiplier;
	float rel=source.progress-source.return_stroke;
	if(rel>0)
	{
		float peak=exp(-rel*rel/(source.return_spread*source.return_spread));
		bright+=std::max(0.f,peak-n);
	}
	
	bright+=extra_light*std::min(source.progress/source.return_stroke,(float)exp(-aaa*source.progress));
	if(bright>1.f)
		bright=1.f;
	return bright;
}

bool ThunderCloudNode::IsSourceStarted(unsigned source_index) const
{
	if(source_index>=sources.size())
		return false;
	const Source &source=sources[source_index];
	return source.started;
}

bool ThunderCloudNode::CanStartSource(unsigned source_index) const
{
	if(source_index>=sources.size())
		return false;
	const Source &source=sources[source_index];
	if(source.finished||source.building)
		return false;
	return(!source.started);
}

void ThunderCloudNode::StartSource(unsigned source_index)
{
	if(source_index>=sources.size())
		return;
	Source &source=sources[source_index];
	if(source.finished||source.building)
		return;
	source.started=true;
}

const float *ThunderCloudNode::GetLightningColour() const
{
	return lightning_colour;
}

const float *ThunderCloudNode::GetIlluminationOrigin() const
{
	Vector3 DX(	2.f*(LightningZoneMin+LightningZoneSize),
				2.f*(LightningZoneMin+LightningZoneSize),
				GetCloudBaseZ()+GetCloudHeight());
	static Vector3 X1;
	X1=GetLightningCentreX()-0.5f*DX;
	X1.z=0;
	return X1;
}

const float *ThunderCloudNode::GetIlluminationScales() const
{
	static Vector3 DX;
	DX.Define(	2.f*(LightningZoneMin+LightningZoneSize),
				2.f*(LightningZoneMin+LightningZoneSize),
				GetCloudBaseZ()+GetCloudHeight());
	return DX;
}