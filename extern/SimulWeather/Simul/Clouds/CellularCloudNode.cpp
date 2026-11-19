#include <algorithm>
#include "precomp.h"

#include "Simul/Clouds/CellularCloudNode.h"
#include "Simul/Clouds/TimedNoise3D.h"
#include "Simul/Math/Noise3D.h"
#include "Simul/Math/Matrix.h"
#include "Simul/Base/EnvironmentVariables.h"
#include "Simul/Geometry/Orientation.h"

#include <math.h>
#include <fstream>
// for memset:
#include <string.h>
#if defined(_MSC_VER)
	#if defined(_DLL) && !defined(XENON)&&!defined(_DEBUG)
		#define SIMUL_USE_TBB
	#endif
	#pragma warning(push)
	#pragma warning(disable:4512)
#endif

#ifdef SIMUL_USE_TBB
	#define  TBB_DO_THREADING_TOOLS 1
	#include "tbb/parallel_for.h" 
	#include "tbb/blocked_range.h"
#endif

// Threads to protect from simultaneously modifying and accessing the cloud. Don't combine this with TBB.
#ifdef CLOUDS_THREAD_SAFETY
	#ifdef _MSC_VER
		#include <windows.h>
		#undef max
		#undef min
		static bool try_lock(HANDLE mutex)
		{
			if(WaitForSingleObject(mutex,100)==WAIT_OBJECT_0)
				return true; 
			return false;
		}
	#endif
#endif
#include "Simul/Math/Pi.h"

// Density of pure water vapour: 0.627 kg/m3

// wavelength ext.coef  sca.coef  abs.coef  si.sc.alb  asym.par  ext.nor    ref.real  ref.imag
//     [um]     [1/km]    [1/km]    [1/km]
//
//  .4500		 .1792	 .1792	 1.068E-08	 1.000		 0.870	 .9930 		 1.337	 -1.020E-09
//  .5000		 .1800	 .1800	 0.000E+00	 1.000		 0.867	 .9975 		 1.335	 -1.000E-09
//  .6500		 .1823	 .1823	 3.152E-07	 1.000		 0.860	 1.010 		 1.331	 -1.640E-08


using namespace simul::clouds;
using namespace simul::math;
using namespace simul::base;
using namespace simul::graph::meta;

static float lerp(float l,float h,float f)
{
	return l+((h-l)*f);
}

template<class T> void Limit(T &t,T minimum,T maximum)
{
	if(t<minimum)
		t=minimum;
	if(t>maximum)
		t=maximum;
}

CellularCloudNode::CellularCloudNode() : CellularCloudGrid(),
	Group()
	,simul::graph::meta::ResourceUser<simul::graph::standardnodes::ShowProgressInterface>()
	,simul::graph::meta::ResourceUser< simul::graph::meta::MemoryInterface >()
	,timed_noise(NULL)
	,NoisePersistence(0.5f)
	,light_axis(0)
	,MieAsymmetry(0.87f)
	,access_mutex(0)
	,GenerationNumber(0)
	,RelightNumber(0)
	,SelfShadowScale(0.001f)
	,Diffusivity(0.01f)
	,LightExtend(0.3f)
	,LightDetail(1)
	,MinimumLightElevationDegrees(3.f)
	,secondary_extinction(0.f)
	,coverage(0.f)
	,total_light_remaining(0)
	,UseCUDA(false)
	,lit_humidity(-1.f)
	,LightResponse(1.f)
	,SecondaryLightResponse(1.f)
	,AmbientLightResponse(1.f)
	,relit_checksum(0)
{
	InitializePropertiesDefinition();
	secondary_gridsize=0;
	WindOffset.Zero();
	SetRecalculate(true);
	CloudWidth=80000.f;
	CloudLength=80000.f;
	CloudHeight=6000.f;
	AlphaSharpness=0.f;
	CentreX=0;
	CentreY=0;
	CloudBaseZ=2000;
	light_dir[0]=0;
	light_dir[1]=0;
	light_dir[2]=1;

	light_scales.Define(1, 1, 1);

	FractalAmplitude=2.2f;
	FractalWavelength=220.f;

	ThinLayer=false;
	SetRelight();
	light_gridsize=0;
	ambient_grid=NULL;
	light_grid=NULL;
	secondary_grid=NULL;
	secondary_gridsize=0;
	ambient_gridsize=0;
	light_gridsizes[0]=0;
	light_gridsizes[1]=0;
	light_gridsizes[2]=0;
	ambient_gridsizes[0]=0;
	ambient_gridsizes[1]=0;
	ambient_gridsizes[2]=0;
	OpticalDensity=1.8f;
	Extinction=1.8f;
	extinction=Extinction*0.001f;
	NoiseResolution=32;
    NoiseOctaves = 3;
	NoisePeriod=1.f;
	Time=0.f;
#ifdef CLOUDS_THREAD_SAFETY
	HANDLE m=CreateMutex(NULL,FALSE,NULL);
	access_mutex=(void*)m;
#endif
}

void CellularCloudNode::PrepareToDestroy()
{
  Group::PrepareToDestroy();
	MemoryInterface *mi=ResourceUser<MemoryInterface>::GetResourceInterface();
	if(mi)
	{
		operator delete[](ambient_grid,mi);
		operator delete[](light_grid,mi);
		operator delete[](secondary_grid,mi);
		
		operator delete(timed_noise,mi);
	}
	else
	{
		delete [] secondary_grid;
		delete [] light_grid;
		delete [] ambient_grid;
		
		delete timed_noise;
	}
	CellularCloudGrid::DeallocateMemory(mi);
}

CellularCloudNode::~CellularCloudNode()
{
#ifdef CLOUDS_THREAD_SAFETY
	if(access_mutex)
	{
		HANDLE m=(HANDLE)access_mutex;
		if(ThreadSafety)
		{
			while(!try_lock(m));
			ReleaseMutex(m);
		}
		CloseHandle(m);
	}
#endif
}

float CellularCloudNode::GetAmbient(const simul::math::Vector3 &X)
{
#ifdef CLOUDS_THREAD_SAFETY
	HANDLE m=(HANDLE)access_mutex;
	if(ThreadSafety)
	{
		while(!try_lock(m));
	}
#endif
	Vector3 ambient_pos=X-X1;
	ambient_pos/=DX;
	MultiplyElements(ambient_pos,ambient_pos,Vector3((float)ambient_gridsizes[0],(float)ambient_gridsizes[1],(float)ambient_gridsizes[2]));
	if(ambient_pos.z<0)
		ambient_pos.z=0;
	int x=(int)ambient_pos.x;
	int y=(int)ambient_pos.y;
	int z=(int)ambient_pos.z;	
	if(x>=(int)ambient_gridsizes[0]||y>=(int)ambient_gridsizes[1])
	{
#ifdef CLOUDS_THREAD_SAFETY
		if(ThreadSafety)
			ReleaseMutex(m);
#endif
		return 0;
	}
	if(z>=(int)ambient_gridsizes[2])
		z=(int)ambient_gridsizes[2]-1;
	int zz=GridHeight-1-z;
	if(z<=0)
		zz=GridHeight-1;
	unsigned index=(zz*GridLength+y)*GridWidth+x;// Z=major axis here, NOT x!!!
	if(index>ambient_gridsizes[0]*ambient_gridsizes[1]*ambient_gridsizes[2])
	{
		std::cerr<<"ERROR lookup in CellularCloudNode::GetAmbient\n";
#ifdef CLOUDS_THREAD_SAFETY
		if(ThreadSafety)
			ReleaseMutex(m);
#endif
		return 1.f;
	}
	float ambient=((float)ambient_grid[index])/255.f;
#ifdef CLOUDS_THREAD_SAFETY
	if(ThreadSafety)
		ReleaseMutex(m);
#endif
	return ambient;
}

float CellularCloudNode::GetOpticalPathLength(const simul::math::Vector3 &X0,const simul::math::Vector3 &L)
{
	return InternalCalcOpticalPathLength(X0,L);
}

float CellularCloudNode::GetOpticalPathLengthFromTo(const simul::math::Vector3 &startPoint,const simul::math::Vector3 &endPoint)
{
  return InternalCalcOpticalPathLengthFromTo(startPoint,endPoint);
}

static inline float GetFromGrid(const float *lgrid,const unsigned *lgridsize,
						 int grid_x,int grid_y,int grid_z)
{
	if(grid_x<0)
		grid_x=0;
	if(grid_y<0)
		grid_y=0;
	if(grid_z<0)
		grid_z=0;
	if(grid_x>=(int)lgridsize[0])
		grid_x=(int)lgridsize[0]-1;
	if(grid_y>=(int)lgridsize[1])
		grid_y=(int)lgridsize[1]-1;
	if(grid_z>=(int)lgridsize[2])
		grid_z=(int)lgridsize[2]-1;
	unsigned index=grid_z*lgridsize[1]*lgridsize[0]+grid_y*lgridsize[0]+grid_x;
	return lgrid[index];
}

static inline float GetFromGrid(const unsigned char *lgrid,const unsigned *lgridsize,
						 int grid_x,int grid_y,int grid_z)
{
	if(grid_x<0)
		grid_x=0;
	if(grid_y<0)
		grid_y=0;
	if(grid_z<0)
		grid_z=0;
	if(grid_x>=(int)lgridsize[0])
		grid_x=(int)lgridsize[0]-1;
	if(grid_y>=(int)lgridsize[1])
		grid_y=(int)lgridsize[1]-1;
	if(grid_z>=(int)lgridsize[2])
		grid_z=(int)lgridsize[2]-1;
	unsigned index=grid_z*lgridsize[1]*lgridsize[0]+grid_y*lgridsize[0]+grid_x;
	return (float)lgrid[index]/255.f;
}

static inline float GetFromGridWrap(const unsigned char *lgrid,const unsigned *lgridsize,
						 int grid_x,int grid_y,int grid_z,unsigned light_axis)
{
	const int g0=lgridsize[0];
	const int g1=lgridsize[1];
	const int g2=lgridsize[2];
	while(grid_x<0)
		grid_x+=g0;
	while(grid_y<0)
		grid_y+=g1;
	while(grid_z<0)
		grid_z+=g2;
	if(light_axis==2)
	{
		grid_x=grid_x%g0;
		grid_y=grid_y%g1;
	}
	else
	{
		if(grid_x>=g0)
			grid_x=g0-1;
		if(grid_x<0)
			grid_x=0;
		if(grid_y>=g1)
			grid_y=g1-1;
		if(grid_y<0)
			grid_y=0;
	}
	if(grid_z>=g2)
		grid_z=g2-1;
	if(grid_z<0)
		grid_z=0;
	unsigned index=grid_z*g1*g0+grid_y*g0+grid_x;
	return ((float)lgrid[index])/255.f;
}

const simul::math::Vector3 &CellularCloudNode::ConvertLightCoordsToReal(float x,float y,float z) const
{
	static Vector3 X;
	Vector3 dir=light_vector;
	unsigned axis=light_axis;
	unsigned axis1=(axis+1)%3;
	unsigned axis2=(axis+2)%3;
	const unsigned *grid=light_gridsizes;

	x+=0.5f/(float)grid[0];
	y+=0.5f/(float)grid[1];
	z+=0.5f/(float)grid[2];

	x*=light_scales(axis1);
	y*=light_scales(axis2);

	x+=light_origin(axis1);
	y+=light_origin(axis2);

	x+=light_scales(axis)*dir(axis1)/dir(axis)*z;
	y+=light_scales(axis)*dir(axis2)/dir(axis)*z;

	z*=light_scales(axis);
	z+=light_origin(axis);

	X(axis)=z;
	X(axis1)=x;
	X(axis2)=y;
	return X;
}

const float *CellularCloudNode::GetTransformToLightspaceMatrix(unsigned axis,const simul::math::Vector3 &dir) const
{
	unsigned axis1=(axis+1)%3;
	unsigned axis2=(axis+2)%3;
	static Matrix4x4 trans;
	trans(0,axis1)	=1.f/light_scales(axis1);
	trans(0,axis2)	=0.f;
	trans(0,axis)	=-dir(axis1)/dir(axis)/light_scales(axis1);
	trans(0,3)		=(dir(axis1)/dir(axis)*light_origin(axis)-light_origin(axis1))/light_scales(axis1);
	trans(1,axis1)	=0.f;
	trans(1,axis2)	=1.f/light_scales(axis2);
	trans(1,axis)	=-dir(axis2)/dir(axis)/light_scales(axis2);
	trans(1,3)		=(dir(axis2)/dir(axis)*light_origin(axis)-light_origin(axis2))/light_scales(axis2);
	trans(2,axis1)	=0;
	trans(2,axis2)	=0;
	trans(2,axis)	=1.f/light_scales(axis);
	trans(2,3)		=-light_origin(axis)/light_scales(axis);
	trans(3,0)		=0;
	trans(3,1)		=0;
	trans(3,2)		=0;
	trans(3,3)		=1.f;
	return trans;
}

const float *CellularCloudNode::GetTransformLightspaceToCloudspaceMatrix(unsigned axis,const simul::math::Vector3 &dir) const
{
	Matrix realworld_to_lightspace(4,4);
	realworld_to_lightspace=GetTransformToLightspaceMatrix(axis,dir);
	static Matrix lightspace_to_realworld(4,4);
	realworld_to_lightspace.Inverse(lightspace_to_realworld);
	static Matrix4x4 lightspace_to_cloudspace;
	lightspace_to_cloudspace=(const float*)lightspace_to_realworld;
	Vector3 XX(0,0,0);
	Vector3 x;
	XX.Define(0.f,0.f,0.f);
	simul::math::Multiply(x,lightspace_to_realworld,XX);
	simul::math::Multiply(XX,realworld_to_lightspace,x);
	XX.Define(0.f,0.f,1.f);
	simul::math::Multiply(x,lightspace_to_realworld,XX);
	simul::math::Multiply(XX,realworld_to_lightspace,x);
	lightspace_to_cloudspace(0,3)-=X1.x;
	lightspace_to_cloudspace(1,3)-=X1.y;
	lightspace_to_cloudspace(2,3)-=X1.z;
	Vector3 DX=X2-X1;
	for(int i=0;i<3;i++)
	for(int j=0;j<4;j++)
	{
		lightspace_to_cloudspace(i,j)/=DX(i);
	}
	XX.Define(0.f,0.f,0.f);
	simul::math::Multiply4(x,lightspace_to_cloudspace,XX);
	XX.Define(0.f,0.f,1.f);
	simul::math::Multiply4(x,lightspace_to_cloudspace,XX);
	return lightspace_to_cloudspace;
}

// obtain grid_x0,grid_y0,grid_z0 and the three interps
void CellularCloudNode::GetGridCoordsAndInterps(int &grid_x0,int &grid_y0,int &grid_z0,
												 float &interp_x,float &interp_y,float &interp_z,
												 const simul::math::Vector3 &XX,
												 const simul::math::Vector3 &dir,
												 unsigned axis,
												 const unsigned grid[]) const
{
	// co-ordinate of X along
	unsigned axis1=(axis+1)%3;
	unsigned axis2=(axis+2)%3;
	unsigned axes[]={axis1,axis2,axis};
	Vector3 x;

	Matrix4x4 trans=GetTransformToLightspaceMatrix(axis,dir);
	simul::math::Multiply4(x,trans,XX);
////////////////////////////////////////////
	// proportion along the major axis from the entering face:
	//float dz=(XX(axis)-light_origin(axis));
/*	x(2)=(XX(axis)-light_origin(axis));
	// project the others back along to that face:
	x(0)=XX(axis1)-dir(axis1)/dir(axis)*(XX(axis)-light_origin(axis));
	x(1)=XX(axis2)-dir(axis2)/dir(axis)*(XX(axis)-light_origin(axis));

	x(0)-=light_origin(axis1);
	x(1)-=light_origin(axis2);

	x(0)/=light_scales(axis1);
	x(1)/=light_scales(axis2);
	x(2)/=light_scales(axis);*/
////////////////////////////////////////////
	x(0)*=(float)grid[0];
	x(1)*=(float)grid[1];
	x(2)*=(float)grid[2];

	x(0)-=0.5f;
	x(1)-=0.5f;
	x(2)-=0.5f;

	int I[3];
	I[0]=(int)x(0);
	I[1]=(int)x(1);
	I[2]=(int)x(2);
	if(x(0)<0)
		I[0]--;
	if(x(1)<0)
		I[1]--;
	if(x(2)<0)
		I[2]--;

	interp_x=x(0)-(float)I[0];
	interp_y=x(1)-(float)I[1];
	interp_z=x(2)-(float)I[2];

	for(unsigned i=0;i<3;i++)
	{
		if(axis==2&&axes[i]!=2)
		{
			while(I[i]<0)
				I[i]+=grid[i];
			while(I[i]>=(int)grid[i])
				I[i]-=grid[i];
		}
		else
		{
			if(I[i]<0)
				I[i]=0;
			if(I[i]>=(int)grid[i])
				I[i]=grid[i]-1;
		}
	}

	grid_x0=(unsigned)I[0];
	grid_y0=(unsigned)I[1];
	grid_z0=(unsigned)I[2];

}


float CellularCloudNode::InterpGridValues(int grid_x0,int grid_y0,int grid_z0,
											float interp_x,float interp_y,float interp_z,
											const unsigned char *lgrid,const unsigned *lgridsize) const
{
	int grid_x1=grid_x0+1;
	int grid_y1=grid_y0+1;
	int grid_z1=grid_z0+1;
	float bot_back,bot_front;
	if(Wrap)
	{
		bot_back=lerp(GetFromGridWrap(lgrid,lgridsize,grid_x0,grid_y0,grid_z0,light_axis),
					GetFromGridWrap(lgrid,lgridsize,grid_x1,grid_y0,grid_z0,light_axis),interp_x);
		bot_front=lerp(GetFromGridWrap(lgrid,lgridsize,grid_x0,grid_y1,grid_z0,light_axis),
					GetFromGridWrap(lgrid,lgridsize,grid_x1,grid_y1,grid_z0,light_axis),interp_x);
	}
	else
	{
		bot_back=lerp(GetFromGrid(lgrid,lgridsize,grid_x0,grid_y0,grid_z0),
					GetFromGrid(lgrid,lgridsize,grid_x1,grid_y0,grid_z0),interp_x);
		bot_front=lerp(GetFromGrid(lgrid,lgridsize,grid_x0,grid_y1,grid_z0),
					GetFromGrid(lgrid,lgridsize,grid_x1,grid_y1,grid_z0),interp_x);
	}
	float bot=lerp(bot_back,bot_front,interp_y);
	
	float top_back,top_front;
	if(Wrap)
	{
		top_back=lerp(	GetFromGridWrap(lgrid,lgridsize,grid_x0,grid_y0,grid_z1,light_axis),
						GetFromGridWrap(lgrid,lgridsize,grid_x1,grid_y0,grid_z1,light_axis),interp_x);
		top_front=lerp(	GetFromGridWrap(lgrid,lgridsize,grid_x0,grid_y1,grid_z1,light_axis),
						GetFromGridWrap(lgrid,lgridsize,grid_x1,grid_y1,grid_z1,light_axis),interp_x);
	}
	else
	{
		top_back=lerp(	GetFromGrid(lgrid,lgridsize,grid_x0,grid_y0,grid_z1),
						GetFromGrid(lgrid,lgridsize,grid_x1,grid_y0,grid_z1),interp_x);
		top_front=lerp(	GetFromGrid(lgrid,lgridsize,grid_x0,grid_y1,grid_z1),
						GetFromGrid(lgrid,lgridsize,grid_x1,grid_y1,grid_z1),interp_x);
	}
	float top=lerp(top_back,top_front,interp_y);
	float ret=lerp(bot,top,interp_z);
	return ret;
}
float CellularCloudNode::GetLight(const simul::math::Vector3 &X) const
{
#ifdef CLOUDS_THREAD_SAFETY
	HANDLE m=(HANDLE)access_mutex;
	if(ThreadSafety)
	{
		while(!try_lock(m));
	}
#endif
	int grid_x0,grid_y0,grid_z0;
	float interp_x,interp_y,interp_z;
	// get the grid co-ords (axis+1),(axis+2),(axis)
	// for the skewed lightspace.
	GetGridCoordsAndInterps(grid_x0,grid_y0,grid_z0,
							interp_x,interp_y,interp_z,
							X,
							light_vector,
							light_axis,
							light_gridsizes);
	float ret=InterpGridValues(grid_x0,grid_y0,grid_z0,
							interp_x,interp_y,interp_z,
							light_grid,light_gridsizes);
#ifdef CLOUDS_THREAD_SAFETY
	if(ThreadSafety)
		ReleaseMutex(m);
#endif
	return ret;
}

float CellularCloudNode::GetSecondaryLight(const simul::math::Vector3 &X) const
{
#ifdef CLOUDS_THREAD_SAFETY
	HANDLE m=(HANDLE)access_mutex;
	if(ThreadSafety)
	{
		while(!try_lock(m));
	}
#endif
	int grid_x0,grid_y0,grid_z0;
	float interp_x,interp_y,interp_z;
	// get the grid co-ords (axis+1),(axis+2),(axis)
	// for the skewed lightspace.
	GetGridCoordsAndInterps(grid_x0,grid_y0,grid_z0,
							interp_x,interp_y,interp_z,
							X,
							light_vector,
							light_axis,
							light_gridsizes);
	float ret=InterpGridValues(grid_x0,grid_y0,grid_z0,
							interp_x,interp_y,interp_z,
							secondary_grid,light_gridsizes);
#ifdef CLOUDS_THREAD_SAFETY
	if(ThreadSafety)
	{
		ReleaseMutex(m);
	}
#endif
	return ret;
}

float CellularCloudNode::InternalCalcOpticalPathLengthFromTo(const simul::math::Vector3 &startPoint,const simul::math::Vector3 &endPoint)
{
  simul::math::Vector3 pStart = startPoint;
  simul::math::Vector3 pEnd = endPoint;
  if (pStart.z > pEnd.z)
  {
    // swap vectors - we want pStart to be always lower then pEnd for simplicity
    pStart = endPoint;
    pEnd = startPoint;
  }

  simul::math::Vector3 direction = pEnd - pStart;

  // traveled distance
  float distanceToLight = 0;

  if (pEnd.z <= X1.z)
    return distanceToLight; //both points are under the cloud grid
  if (pStart.z >= X2.z)
    return distanceToLight; //both points are above the cloud grid

  if (direction.z != 0)
  {   
    // move start point so that it starts at the beginning of the cloud grid
    // this comes simply from the parametric equation of the line
    float t = (X1.z - pStart.z)/ direction.z;
    if (t > 0)
    {
      pStart = pStart + t*direction;
    }

    // move end point so that it ends on the end of cloud grid
    t = (X2.z - pStart.z)/ direction.z;
    if (t < 1)
    {
      pEnd = pStart + t*direction;
    }
  }

  /// recompute direction
  direction = pEnd - pStart;

  // determine the step of movement
  float stepX = (GridWidth > 0) ? (DX.x / (float)GridWidth) : 0.0f;
  float stepY = (GridLength > 0) ? (DX.y / (float)GridLength) : 0.0f;
  float stepZ = (GridHeight > 0) ? (DX.z / (float)GridHeight) : 0.0f;

  float dX = (direction.x != 0) ? fabs(stepX / direction.x) : FLT_MAX;
  float dY = (direction.y != 0) ? fabs(stepY / direction.y) : FLT_MAX;
  float dZ = (direction.z != 0) ? fabs(stepZ / direction.z) : FLT_MAX;

  float stepT = std::min(dX, std::min(dY, dZ));

  if (stepT > 0)
  {
    // safety counter to prevent infinite loops
    int safetyCounter = 0;
    const int safetyCounterMax = 10000;
    simul::math::Vector3 stepDistance = direction * stepT;
    float stepSize = stepDistance.Magnitude();
    float currentT = 0.0f;
    while (currentT <= 1.0f && safetyCounter < safetyCounterMax)
    {
      float dens=GetCloudDensity(pStart - WindOffset);
      distanceToLight+=dens*stepSize;

      ++safetyCounter;
      currentT += stepT;
      pStart += stepDistance;
    }
  }

  distanceToLight=std::max(distanceToLight,0.f);
  return distanceToLight; 
}

float CellularCloudNode::InternalCalcOpticalPathLength(const simul::math::Vector3 &X0,const simul::math::Vector3 &lightDir)
{
	float distance_to_light=0;
	float step_size=DX.z/(float)GridHeight;
	Vector3 dx=step_size*lightDir;
	Vector3 X=X0;
	if(step_size)
	for(int i=0;i<256;i++)
	{
		float dens=GetCloudDensity(X - WindOffset);
		distance_to_light+=dens*step_size;
		X+=dx;
		if(!Wrap)
		{
			if(dx.x>=0&&(X.x>X2.x))
				break;
			if(dx.x<=0&&(X.x<X1.x))
				break;
			if(dx.y>=0&&(X.y>X2.y))
				break;
			if(dx.y<=0&&(X.y<X1.y))
				break;
		}
		if(dx.z>=0&&(X.z>X2.z))
			break;
		if(dx.z<=0&&(X.z<X1.z))
			break;
	}
	//distance_to_light-=step_size;
	distance_to_light=std::max(distance_to_light,0.f);
	return distance_to_light;
}

bool CellularCloudNode::Generate()
{
	if(!changed)
		return true;
#ifdef CLOUDS_THREAD_SAFETY
	HANDLE m=(HANDLE)access_mutex;
	if(ThreadSafety)
	{
		while(!try_lock(m));
	}
#endif
	MemoryInterface *mi=ResourceUser<MemoryInterface>::GetResourceInterface();
	if(!timed_noise)
	{
		if(mi)
			timed_noise=new(mi) TimedNoise3D(mi);
		else
			timed_noise=new TimedNoise3D();
		DaytimeChanged();
	}
	StartMarching();
	GetExtents(X1,X2);
	DX=X2-X1;
	timed_noise->Setup(NoiseResolution,RandomSeed,NoiseOctaves,NoisePersistence);
	timed_noise->SetCacheGrid(GridWidth,GridLength,GridHeight);
	Init(mi,timed_noise,GridLength,GridWidth,GridHeight);

	changed=false;
	SetRelight();
#ifdef CLOUDS_THREAD_SAFETY
	if(ThreadSafety)
		ReleaseMutex(m);
#endif
	GenerationNumber++;
	return true;
}

void CellularCloudNode::ChangedNoise()
{
	CellularCloudGrid::ChangedNoise();
	if(timed_noise)
	{
		timed_noise->Setup(NoiseResolution,RandomSeed,NoiseOctaves,NoisePersistence);
		timed_noise->SetCacheGrid(GridWidth,GridLength,GridHeight);
	}
}
void CellularCloudNode::SaveGrid(std::ostream &os) const
{
	RecursiveStreamOut(os,false);
	RecursiveStateStreamOut(os,false);
	CellularCloudGrid::SaveGrid(os);
}

void CellularCloudNode::LoadGrid(std::istream &is)
{
	RecursiveStreamIn(is,false);
	RecursiveStateStreamIn(is,false);
	CellularCloudGrid::LoadGrid(is);

	NeedRelight();
}
bool CellularCloudNode::ReLight()
{
	simul::math::Vector3 current_light(light_dir);
	return ReLight(current_light);
}
int CellularCloudNode::GetGenerationNumber() const
{
	return GenerationNumber;
}

int CellularCloudNode::GetRelightNumber() const
{
	return RelightNumber;
}

float CellularCloudNode::GetCoverage() const
{
	return coverage;
}

bool CellularCloudNode::ReLight(const simul::math::Vector3 &lightDir)
{
	SetLightDirection(lightDir);
	if(GetRelightChecksum()==relit_checksum)
		return true;
	ReLightPartial(1.f);
	return true;
}

Vector3 CellularCloudNode::ProjectToEdge(const Vector3 &X0,const Vector3 &D,const Vector3 &x1,const Vector3 &x2)
{
	float a_min=1e27f;
	for(int i=0;i<3;i++)
	{
		float a=0;
		if(D[i]<-.0001f)
		{
			a=(x1[i]-X0[i])/D[i];
			if(a<a_min)
				a_min=a;
		}
		else if(D[i]>.0001f)
		{
			a=(x2[i]-X0[i])/D[i];
			if(a<a_min)
				a_min=a;
		}
	}
	Vector3 X=X0+a_min*D;
	return X;
}
void CellularCloudNode::CalcLightParameters(	const simul::math::Vector3 &light,
												unsigned &axis,
												simul::math::Vector3 &light_vec)
{
	static float cc=6.f;
	// which axis is closest to the light direction?
	axis=0;
	// above 2 degrees? Use z-axis
	static float zlimit=0.2f;//052f;// 3 degrees
	if(fabs(light.z)>zlimit||GridHeight<4)
	{
		axis=2;
		// make it 1 on the axis, in the direction light is shining:
		light_vec=-(light/fabs(light(axis)));
	}
	else
	{
		if(fabs(light.y)>fabs(light.x))
			axis=1;
		else
			axis=0;
		light_vec=-(light/fabs(light(axis)));
	}
	/// NEGATIVE LIGHTING CONDITION - caution!
	if(light.z<0&&GridHeight>2)
	{
		under_light_multiplier=exp(-cc*Humidity);
	}
	else
		under_light_multiplier=1.f;
	unsigned axis1=(axis+1)%3;
	unsigned axis2=(axis+2)%3;
	// scale
	light_scales.Define(DX.x,DX.y,DX.z);
	light_origin=X1;
	if(light_vec(axis)<0)
	{
		light_origin(axis)=X2(axis);
		light_scales(axis)*=-1.f;
	}
	if(!Wrap)
	{
		if(light_vec(axis2)<0)
		{
			light_origin(axis2)=X2(axis2);
			light_scales(axis2)*=-1.f;
		}
		if(light_vec(axis1)<0)
		{
			light_origin(axis1)=X2(axis1);
			light_scales(axis1)*=-1.f;
		}
		// in order to enclose the entire volume, the prism must be expanded.

		// Go to corner below entrant corner:
		Vector3 lower_corner=light_origin;
		lower_corner(axis)+=light_scales(axis);
		// trace back to top.
		Vector3 entrant_point=lower_corner-light_vec*DX(axis);
		Vector3 entrant_offset=light_origin-entrant_point;
		light_scales(axis1)+=entrant_offset(axis1);
		light_scales(axis2)+=entrant_offset(axis2);
		light_origin=entrant_point;
	}
	else
	{
		if(axis!=2)
		{
			if(GridHeight>2)
			{
	#if 0
				light_scales.z+=light_origin.z;
				light_origin.z=0;
	#endif
			}
			// extend the main axis back by a full length:
			/*if(light_vec(axis)<0)
			{
				light_origin(axis)-=light_scales(axis);
			}
			else*/
			{
				light_origin(axis)-=LightExtend*light_scales(axis);
			}
			light_scales(axis1)*=1.f+fabs(light_vec(axis1));
			light_scales(axis2)*=1.f+fabs(light_vec(axis2));
			// extend the vertical axis:
			light_scales(axis1)+=2.f*fabs(LightExtend*light_scales(axis)*light_vec(axis1));
			light_scales(axis2)+=2.f*fabs(LightExtend*light_scales(axis)*light_vec(axis2));
			light_origin(axis1)-=fabs(LightExtend*light_scales(axis))*light_vec(axis1);
			light_origin(axis2)-=fabs(LightExtend*light_scales(axis))*light_vec(axis2);
			//light_origin(axis1)-=sgn(light_scales(axis1))*fabs(light_scales(axis)*light_vec(axis1)/light_vec(axis));
			//light_origin(axis2)-=sgn(light_scales(axis2))*fabs(light_scales(axis)*light_vec(axis2)/light_vec(axis));
			light_scales(axis)*=(1.f+LightExtend);
		}
		if(light_vec(axis2)>0)
		{
			light_origin(axis2)=X2(axis2);
			light_scales(axis2)*=-1.f;
		}
		if(light_vec(axis1)>0)
		{
			light_origin(axis1)=X2(axis1);
			light_scales(axis1)*=-1.f;
		}
	}
}

simul::math::NoiseInterface *CellularCloudNode::GetNoiseInterface()
{
	return timed_noise;
}


const float *CellularCloudNode::GetLightspaceExtinctions() const
{
	Vector3 direct_step			=fabs(light_scales(light_axis))*light_vector/(float)light_gridsizes[2];
	float direct_indir_step_size=direct_step.Magnitude();
	float ambient_step_size		=CloudHeight/(float)GridHeight;
	static float ext[4];
	ext[0]=extinction*direct_indir_step_size;
	ext[1]=secondary_extinction*direct_indir_step_size;
	ext[2]=extinction*ambient_step_size;
	ext[3]=secondary_extinction*ambient_step_size;
	//direct_light=exp(-extinction*distance_to_light);
	return ext;
}

void CellularCloudNode::SetLightValues(const float *target_direct_grid,const float *target_indirect_grid,const float *target_ambient_grid)
{
	for(unsigned i=0;i<light_gridsize;i++)
	{
		light_grid[i]			=(unsigned char)(255.f*target_direct_grid[i]);
		secondary_grid[i]		=(unsigned char)(255.f*target_indirect_grid[i]);
		ambient_grid[i]			=(unsigned char)(255.f*target_ambient_grid[i]);
			//light_gridsizes
	}
}

float CellularCloudNode::DoQuicklightStep(	unsigned i,
											unsigned j,
											unsigned char *lightspace_grid,
											unsigned char *secondary_grid,
											bool separate_secondary,
											const simul::math::Vector3 &light_vec,
											const simul::math::Vector3 &orig,
											const simul::math::Vector3 &scales,
											unsigned axis,
											float primary_ratio,
											float global_multiplier,
											const unsigned grid[3])
{
	// i and j represent indices into the non-major axes, e.grid. x and y if the major axis is z.
	// Light is traced in the light_vec direction from the point of entry on the major surface
	// to the point of exit on the opposite surface.
	unsigned axis1=(axis+1)%3;
	unsigned axis2=(axis+2)%3;
	Vector3 pos=orig;
	Vector3 step=fabs(scales(axis))*light_vec/(float)grid[2];
	pos+=0.5f*step;
	pos(axis1)+=((float)i+0.5f)/(float)grid[0]*scales(axis1);
	pos(axis2)+=((float)j+0.5f)/(float)grid[1]*scales(axis2);
	float step_size=step.Magnitude();
	float fade=SelfShadowScale*(float)GridWidth/CloudWidth;
	float distance_to_light=0.f;
	int index=0;
	int k=0;
	index=(k*grid[1]+j)*grid[0]+i;
	int index_step=grid[0]*grid[1];
	Vector3 d,D;
	float direct_light=1.f;
	for(;k<(int)grid[2];k++,index+=index_step)
	{
		// going downwards:
		Subtract(d,pos,X1);
		d/=DX;
		MultiplyElements(D,d,Vector3((float)GridWidth,(float)GridLength,(float)GridHeight));
		float dens1=(float)GetCloud((int)D.x,(int)D.y,(int)D.z);
		float dens2=(float)GetCloud((int)D.x,(int)D.y,(int)D.z+1);
		float dens=lerp(dens1,dens2,D.z-(int)D.z);
    	float distance_step=dens*step_size;
		if(!dens)
			distance_to_light*=exp(-fade*step_size);
		direct_light=exp(-extinction*distance_to_light);
		float secondary_light=exp(-secondary_extinction*distance_to_light);
		if(k>=0)
		{
			if(separate_secondary)
			{
				secondary_grid[index]=(unsigned char)(255.f*std::min(1.f,global_multiplier*secondary_light));
				lightspace_grid[index]=(unsigned char)(255.f*std::min(1.f,global_multiplier*direct_light));
			}
			else
			{
				lightspace_grid[index]=(unsigned char)(255.f*std::min(1.f,global_multiplier*(primary_ratio*direct_light+(1.f-primary_ratio)*secondary_light)));
			}
		}
		distance_to_light+=distance_step;
		pos+=step;
	}
	return direct_light;
}

namespace simul
{
	namespace clouds
	{
		class DoLightSteps
		{ 
			CellularCloudNode *ccn;
			unsigned i;
			unsigned j;
			mutable unsigned max_step;
			unsigned max_steps;
			unsigned char *lightspace_grid;
			unsigned char *secondary_grid;
			const simul::math::Vector3 &light_vec;
			const simul::math::Vector3 &orig;
			const simul::math::Vector3 &scales;
			unsigned axis;
			float primary_ratio;
			float global_multiplier;
			bool separate_secondary;
			unsigned grid[3];
		public: 
#ifdef SIMUL_USE_TBB
			void operator()( const tbb::blocked_range<size_t>& r ) const
#else
			void operator()( unsigned begin,unsigned end ) const
#endif
			{ 
				simul::graph::standardnodes::ShowProgressInterface *progress=
					ccn->simul::graph::meta::ResourceUser<simul::graph::standardnodes::ShowProgressInterface>::GetResourceInterface();
#ifdef SIMUL_USE_TBB
				unsigned begin=(unsigned)r.begin();
				unsigned end=(unsigned)r.end();
#endif
				for(unsigned step=begin;step!=end;++step )
				{
					if(progress&&!(step%256)&&!begin)
					{
						progress->SetProgress(ccn,(float)step/(end-1.f));
						if(progress->GetCancel(ccn))
							break;
					}
					unsigned i=step/grid[1];
					unsigned j=step-i*grid[1];
					ccn->total_light_remaining+=ccn->DoQuicklightStep(i,j,lightspace_grid,secondary_grid,
						separate_secondary,light_vec,orig,scales,axis,
						primary_ratio,global_multiplier,grid);
				}
			} 
			DoLightSteps(	CellularCloudNode *c,
							unsigned char *lsg,
							unsigned char *sg,
							const simul::math::Vector3 &lv,
							const simul::math::Vector3 &orr,
							const simul::math::Vector3 &sc,
							unsigned ax,
							float pr,
							float gm,
							unsigned G[3])
				: ccn(c)
				, lightspace_grid(lsg)
				, secondary_grid(sg)
				, separate_secondary(false)
				, light_vec(lv)
				, orig(orr)
				, scales(sc)
				, axis(ax)
				, primary_ratio(pr)
				, global_multiplier(gm)
				, max_step(0)
			{
				separate_secondary=(secondary_grid!=NULL);
				grid[0]=G[0];
				grid[1]=G[1];
				grid[2]=G[2];
				max_steps=grid[0]*grid[1];
			} 
		};
	}
}

bool CellularCloudNode::CUDALight(unsigned char *&lightspace_grid,unsigned char *&secondary_grid,
				const simul::math::Vector3 &light_vec,const simul::math::Vector3 &orig,
				const simul::math::Vector3 &scales,unsigned axis,
				float global_multiplier,unsigned grid[3])
{
	if(!(GridLength*GridWidth*GridHeight))
		return false;
	// 
	unsigned num_steps=grid[0]*grid[1];
	for(unsigned step=0;step<(unsigned)num_steps;step++)
	{
		unsigned i=step/grid[1];
		unsigned j=step-i*grid[1];
		total_light_remaining+=DoQuicklightStep(i,j,lightspace_grid,secondary_grid,true,light_vec,orig,scales,axis,.5f,global_multiplier,grid);
	}
	return true;
}


const simul::math::Vector3 &CellularCloudNode::GetLightDir()
{
	static Vector3 l;
	l.DefineValues(light_dir);
	return l;
}
const simul::math::Vector3 &CellularCloudNode::GetLightVector() const
{
	return light_vector;
}

unsigned CellularCloudNode::GetLightAxis() const
{
	return light_axis;
}

void CellularCloudNode::GetExtents(simul::math::Vector3 &x1,simul::math::Vector3 &x2) const
{
	x1.Define(CentreX-CloudWidth/2.f,CentreY-CloudLength/2.f,CloudBaseZ);
	x2.Define(CentreX+CloudWidth/2.f,CentreY+CloudLength/2.f,CloudBaseZ+CloudHeight);
}

const simul::math::Vector3 &CellularCloudNode::GetOrigin() const
{
	static Vector3 ret;
	ret.Define(CentreX-CloudWidth/2.f,CentreY-CloudLength/2.f,CloudBaseZ);
	ret+=WindOffset;
	return ret;
}

const simul::math::Vector3 &CellularCloudNode::GetInverseScales() const
{
	static Vector3 ret;
	ret.Define(1.f/CloudWidth,1.f/CloudLength,1.f/CloudHeight);
	return ret;
}

void CellularCloudNode::GetCoordsFromPosition(const simul::math::Vector3 &X,unsigned &x,unsigned &y,unsigned &z) const
{
	Vector3 D;
	D=X-X1;
	D/=DX;
	MultiplyElements(D,D,Vector3((float)GridWidth,(float)GridLength,(float)GridHeight));

	x=(unsigned)D.x;
	y=(unsigned)D.y;
	z=(unsigned)D.z;
}

float CellularCloudNode::GetCloudDensity(const simul::math::Vector3 &X) const
{
	int x=0,y=0,z=0;

	Vector3 D,d;
	Subtract(d,X,X1);
	d/=DX;
	MultiplyElements(D,d,Vector3((float)GridWidth,(float)GridLength,(float)GridHeight));

	x=(int)D.x;
	y=(int)D.y;
	z=(unsigned)D.z;

	if(!Wrap)
	{
		if(x<0||x>=(int)GridWidth)
			return 0;
		if(y<0||y>=(int)GridLength)
			return 0;
	}
	if(z>=(int)GridHeight)
		return 0;
	// no storage of floats:
	float val=(float)GetCloud(x,y,z);
	return val;
}

static bool show_original=false;
void CellularCloudNode::GetGridValues(float &density,float &light,float &ambient,
									float &second,int x,int y,int z) const
{
#ifdef CLOUDS_THREAD_SAFETY
	HANDLE m=(HANDLE)access_mutex;
	if(ThreadSafety)
	{
		while(!try_lock(m));
	}
#endif
	density=(float)GetCloud(x,y,z);
	int zz=GridHeight-1-z;
	if(z<=0)
		zz=GridHeight-1;
	unsigned index=(zz*GridLength+y)*GridWidth+x;// z=major axis here, NOT X!!!
	ambient=((float)ambient_grid[index])/255.f;
	Vector3 X;
	X.x=lerp(X1.x,X2.x,((float)x+0.5f)/(float)(GridWidth));
	X.y=lerp(X1.y,X2.y,((float)y+0.5f)/(float)(GridLength));
	X.z=lerp(X1.z,X2.z,((float)z+0.5f)/(float)(GridHeight));

	int grid_x0,grid_y0,grid_z0;
	float interp_x,interp_y,interp_z;
	// get the grid co-ords (axis+1),(axis+2),(axis)
	// for the skewed lightspace.
	// light vector points in the direction the light shines.
	GetGridCoordsAndInterps(grid_x0,grid_y0,grid_z0,
							interp_x,interp_y,interp_z,
							X,
							light_vector,
							light_axis,
							light_gridsizes);
	if(show_original)
	{
		int g[3];
		g[0]=x;
		g[1]=y;
		g[2]=z;
		light=InterpGridValues(g[(light_axis+1)%3],g[(light_axis+2)%3],g[light_axis],
							0,0,0,
							light_grid,light_gridsizes);
		second=InterpGridValues(g[(light_axis+1)%3],g[(light_axis+2)%3],g[light_axis],
							0,0,0,
							secondary_grid,light_gridsizes);
	}
	else
	{
		light=InterpGridValues(grid_x0,grid_y0,grid_z0,
							interp_x,interp_y,interp_z,
							light_grid,light_gridsizes);
		second=InterpGridValues(grid_x0,grid_y0,grid_z0,
							interp_x,interp_y,interp_z,
							secondary_grid,light_gridsizes);
	}
#ifdef CLOUDS_THREAD_SAFETY
	if(ThreadSafety)
		ReleaseMutex(m);
#endif
}
void CellularCloudNode::GetGrid(unsigned &w,unsigned &l,unsigned &h) const
{
	w=GridWidth;
	l=GridLength;
	h=GridHeight;
}

void CellularCloudNode::SetGrid(unsigned w,unsigned l,unsigned h)
{
	SetGridWidth(w);
	SetGridHeight(h);
	SetGridLength(l);
}

void CellularCloudNode::Seed(unsigned x,unsigned y,bool val)
{
	unsigned index=0,bit=0;
	GetIndexAndBit(x,y,0,index,bit);
	if(val)
	{
		cld[index]|=1<<bit;
	}
	else
	{
		cld[index]&=~(1<<bit);
	}
}
#if 0
unsigned CellularCloudNode::GetNumLines() const
{
	return 12;
}

void CellularCloudNode::GetLine(unsigned i,float* x1,float* x2,int*) const
{
	simul::math::Vector3 X1,X2;
	GetExtents(X1,X2);
	if(i<4)
	{
		x1[0]=X1.x;
		x2[0]=X1.x;
	}
	else if(i<8)
	{
		x1[0]=X2.x;
		x2[0]=X2.x;
		i-=4;
	}
	else
	{
		x1[0]=X1.x;
		x2[0]=X2.x;
		x1[1]=(i%2)?X1.y:X2.y;
		x1[2]=((i%4)/2)?X1.z:X2.z;
		x2[1]=x1[1];
		x2[2]=x1[2];
	}
	if(i<4)
	{
		if(i==0||i==2)
		{
			x1[2]=(i/2-1)?X1.z:X2.z;
			x2[2]=x1[2];
			x1[1]=i%2?X1.y:X2.y;
			x2[1]=(i+1)%2?X1.y:X2.y;
		}
		else
		{
			x1[1]=(i/2)?X1.y:X2.y;
			x2[1]=x1[1];
			x1[2]=(i/2)%2?X1.z:X2.z;
			x2[2]=(i/2)%2?X2.z:X1.z;
		}
	}
}	
#endif
std::ostream &CellularCloudNode::StateStreamOut(std::ostream &os) const
{
	return os;
}
std::istream &CellularCloudNode::StateStreamIn(std::istream &is)
{
	return is;
}

std::ostream &CellularCloudNode::StreamOut(std::ostream &os) const
{
	CellularCloudGrid::StreamOutAutoPropertiesDefinition(os);
	return os;
}
std::istream &CellularCloudNode::StreamIn(std::istream &is)
{
	CellularCloudGrid::StreamInAutoPropertiesDefinition(is);
	return is;
}

void CellularCloudNode::ChangedSec()
{
	NeedRender();
}
void CellularCloudNode::ChangedCloudPosition()
{
	// in case we then relight: the light origin has changed:
	GetExtents(X1,X2);
	light_origin=X1;
	for(int i=0;i<3;i++)
	{
		if(light_scales(i)<0)
		{
			light_origin(i)=X2(i);
		}
	}
	NeedRender();
}
void CellularCloudNode::NeedRender()
{
}

void CellularCloudNode::NeedRelight()
{
	extinction=Extinction*0.001f;
	//SetRelight();
}

void CellularCloudNode::SetRelight()
{
	relit_checksum=0;
}

void CellularCloudNode::Recalculate()
{
 	active_humidity=Humidity;
	SetRecalculate(true);
}

void CellularCloudNode::ChangedGrid()
{
	CellularCloudGrid::ChangedGrid();
	if(noise)
		noise->SetCacheGrid(GridWidth,GridLength,GridHeight);
}


/*
CellularCloudNode &CellularCloudNode::operator=(const CellularCloudNode &C)
{
	std::ofstream temp("temp",std::ios_base::binary);
	C.RecursiveStreamOut(temp,false);
	temp.close();
	std::ifstream temp_in("temp",std::ios_base::binary);
	this->RecursiveStreamIn(temp_in,false);
	return *this;
}*/

IMPLEMENT_Action(CellularCloudNode,Cumulus)
{
#ifdef CLOUDS_THREAD_SAFETY
	HANDLE m=(HANDLE)access_mutex;
	if(ThreadSafety)
	{
		while(!try_lock(m));
	}
#endif
	SetCentreX(0.f);
	SetCentreY(0.f);
	SetCloudBaseZ(1000.f);
	SetCloudWidth(40000.f);
	SetCloudHeight(3000.f);
	SetCloudLength(40000.f);
	
	SetExtinction(.17f);
	SetLightResponse(1.f);
	SetMieAsymmetry(0.87f);
	SetAmbientLightResponse(1.f);
	SetSecondaryLightResponse(1.f);
	SetSelfShadowScale(0.001f);

	SetOpticalDensity(2.f);
	SetFractalAmplitude(2.5f);
	SetFractalWavelength(30.f);

	SetGridLength(256);
	SetGridWidth(256);
	SetGridHeight(16);

	SetNoisePersistence(0.75f);
	SetHumidity(0.37f);
	SetWrap(true);
	
	SetNoiseResolution(8);
	SetNoiseOctaves(3);
#ifdef CLOUDS_THREAD_SAFETY
	if(ThreadSafety)
		ReleaseMutex(m);
#endif
}
IMPLEMENT_Action(CellularCloudNode,Stratus)
{
#ifdef CLOUDS_THREAD_SAFETY
	HANDLE m=(HANDLE)access_mutex;
	if(ThreadSafety)
	{
		while(!try_lock(m));
	}
#endif
	SetCentreX(0.f);
	SetCentreY(0.f);
	SetCloudBaseZ(1000.f);
	SetCloudWidth(40000.f);
	SetCloudHeight(3000.f);
	SetCloudLength(40000.f);
	
	SetExtinction(.17f);
	SetLightResponse(1.f);
	SetMieAsymmetry(0.5f);
	SetAmbientLightResponse(1.f);
	SetSecondaryLightResponse(1.f);
	SetSelfShadowScale(0.001f);

	SetOpticalDensity(2.f);
	SetFractalAmplitude(2.f);
	SetFractalWavelength(30.f);

	SetGridLength(128);
	SetGridWidth(128);
	SetGridHeight(8);

	SetNoisePersistence(0.6f);
	SetHumidity(0.63f);
	SetWrap(true);
	
	SetNoiseResolution(8);
	SetNoiseOctaves(3);
#ifdef CLOUDS_THREAD_SAFETY
	if(ThreadSafety)
		ReleaseMutex(m);
#endif
}
IMPLEMENT_Action(CellularCloudNode,Cirrus)
{
#ifdef CLOUDS_THREAD_SAFETY
	HANDLE m=(HANDLE)access_mutex;
	if(ThreadSafety)
	{
		while(!try_lock(m));
	}
#endif
	SetCentreX(0.f);
	SetCentreY(0.f);
	SetCloudBaseZ(10000.f);
	SetCloudWidth(200000.f);
	SetCloudHeight(2500.f);
	SetCloudLength(200000.f);
	
	SetExtinction(0.1f);
	SetLightResponse(1.f);
	SetMieAsymmetry(0.85f);
	SetAmbientLightResponse(1.f);
	SetSecondaryLightResponse(1.f);
	SetSelfShadowScale(0.05f);

	SetOpticalDensity(0.05f);
	SetFractalAmplitude(2.f);
	SetFractalWavelength(30.f);

	SetGridLength(512);
	SetGridWidth(512);
	SetGridHeight(4);

	SetNoisePersistence(0.7f);
	SetHumidity(0.4f);
	SetWrap(true);
	
	SetNoiseResolution(32);
	SetNoiseOctaves(3);
#ifdef CLOUDS_THREAD_SAFETY
	if(ThreadSafety)
		ReleaseMutex(m);
#endif
}


void CellularCloudNode::SetLightDirection(const simul::math::Vector3 &dir)
{
	Vector3 dir_to_sun=dir;
	if(MinimumLightElevationDegrees>-90.f)
	{
		float MinimumLightElevation=MinimumLightElevationDegrees*pi/180.f;
		float elev=asin(dir_to_sun.z);
		if(elev<MinimumLightElevation&&fabs(cos(MinimumLightElevation))<0.99999f)
		{
			float sz=sqrt(dir_to_sun.x*dir_to_sun.x+dir_to_sun.y*dir_to_sun.y)/cos(MinimumLightElevation);
			dir_to_sun.z=sin(MinimumLightElevation)*sz;
			dir_to_sun.Unit();
		}
	}

	lighting_index=0;
	
	simul::math::Vector3 current_light(light_dir);
	//static float cosine_threshold=0.999998f;
	//if((dir_to_sun*current_light)<cosine_threshold)
	//	SetRelight();
	light_dir[0]=dir_to_sun(0);
	light_dir[1]=dir_to_sun(1);
	light_dir[2]=dir_to_sun(2);
	if(!NeedsRelight())
		return;
	total_light_remaining=0;
	static float sec_mult=1.f/(4.f*pi);
	secondary_extinction=extinction*sec_mult;
#ifdef CLOUDS_THREAD_SAFETY
	HANDLE m=(HANDLE)access_mutex;
	if(ThreadSafety)
	{
		while(!try_lock(m));
	}
#endif
	
	CalcLightParameters(dir_to_sun,light_axis,light_vector);
	unsigned h[3]={GridWidth,GridLength,GridHeight};
	unsigned axis1=(light_axis+1)%3;
	unsigned axis2=(light_axis+2)%3;
	light_gridsizes[0]=h[axis1];
	light_gridsizes[1]=h[axis2];
	light_gridsizes[2]=h[light_axis];
	ambient_gridsizes[0]=h[0];
	ambient_gridsizes[1]=h[1];
	ambient_gridsizes[2]=h[2];
	if(light_gridsizes[0]*light_gridsizes[1]*light_gridsizes[2]!=light_gridsize
		||ambient_gridsize!=GridLength*GridWidth*GridHeight)
	{
		MemoryInterface *mi=ResourceUser<MemoryInterface>::GetResourceInterface();
		light_gridsize=light_gridsizes[0]*light_gridsizes[1]*light_gridsizes[2];
		ambient_gridsize=GridLength*GridWidth*GridHeight;
		if(mi)
		{
			operator delete[](ambient_grid,mi);
			operator delete[](light_grid,mi);
			operator delete[](secondary_grid,mi);
		}
		else
		{
			delete [] ambient_grid;
			delete [] light_grid;
			delete [] secondary_grid;
		}
		ambient_grid=NULL;
		light_grid=NULL;
		secondary_grid=NULL;
		secondary_gridsize=0;
		if(light_gridsize)
		{
			if(mi)
			{
				ambient_grid=new(mi) unsigned char[ambient_gridsize];
				light_grid=new(mi) unsigned char[light_gridsize];
			}
			else
			{
				ambient_grid=new unsigned char[ambient_gridsize];
				light_grid=new unsigned char[light_gridsize];
			}
			if(mi)
				secondary_grid=new(mi) unsigned char[light_gridsize];
			else
				secondary_grid=new unsigned char[light_gridsize];
			secondary_gridsize=light_gridsize;
		}
	}
#ifdef CLOUDS_THREAD_SAFETY
	if(ThreadSafety)
		ReleaseMutex(m);
#endif
}

void CellularCloudNode::StartMarching()
{
	if(NoiseResolution&&GridHeight)
		timed_noise->SetInitialGridHeight((float)NoiseResolution*(float)GridHeight/(float)GridWidth);
	SetRecalculate(true);
}

void CellularCloudNode::DaytimeChanged()
{
	if(!timed_noise)
		return;
	if(NoisePeriod)
		timed_noise->SetTime(Time/NoisePeriod);
	else
		timed_noise->SetTime(0.f);
}

bool CellularCloudNode::UpdatePartial(float proportion)
{
	return ReLightPartial(proportion);
}

static bool disable_ambient=false;

bool CellularCloudNode::ReLightPartial(float proportion)
{
	if(!NeedsRelight())
		return false;
	if(proportion<0.f)
		proportion=0.f;
	if(proportion>1.f)
		proportion=1.f;
	lit_humidity=Humidity;
	unsigned ambient_steps=ambient_gridsizes[0]*ambient_gridsizes[1];
	unsigned direct_steps=light_gridsizes[0]*light_gridsizes[1];
	unsigned gridsize=(ambient_steps+direct_steps);
	unsigned steps=std::max((unsigned)1,(unsigned)(1.f+proportion*(float)(gridsize)));// Round UP!
	if(steps>gridsize)
		steps=gridsize;
	if(lighting_index>steps)
		return true;
	if(!UseTbb)
	{
		for(lighting_index;lighting_index<steps;lighting_index++)
		{
			if(lighting_index<ambient_steps)
			{
				unsigned i=lighting_index/ambient_gridsizes[1];
				unsigned j=lighting_index-ambient_gridsizes[1]*i;
				if(!disable_ambient)
				DoQuicklightStep(i,j,ambient_grid,NULL,false,Vector3(0,0,-1.f),Vector3(X1.x,X1.y,X2.z),
					Vector3(CloudWidth,CloudLength,CloudHeight),2,0.5f,1.f,ambient_gridsizes);
				total_light_remaining=0;
			}
			else
			{
				if(lighting_index<gridsize)
				{
					unsigned i=(lighting_index-ambient_steps)/light_gridsizes[1];
					unsigned j=(lighting_index-ambient_steps)-light_gridsizes[1]*i;
					total_light_remaining+=DoQuicklightStep(i,j,light_grid,secondary_grid,true,
						light_vector,light_origin,light_scales,light_axis,1.f,under_light_multiplier,
						light_gridsizes);
				}
			}
		}
	}
	else
	{	
		if(lighting_index<ambient_steps)
		{
			int amb=std::min(steps,ambient_steps);
#ifdef SIMUL_USE_TBB
			if(UseTbb)
			{
				DoLightSteps dls(this,ambient_grid,NULL,Vector3(0,0,-1.f),Vector3(X1.x,X1.y,X2.z),
					Vector3(CloudWidth,CloudLength,CloudHeight),2,0.5f,1.f,ambient_gridsizes);
				if(!disable_ambient)
					tbb::parallel_for(tbb::blocked_range<size_t>(lighting_index,amb),
						dls,
						tbb::auto_partitioner());
			}
			else
#endif
			{
				DoLightSteps dls(this,ambient_grid,NULL,Vector3(0,0,-1.f),Vector3(X1.x,X1.y,X2.z),
					Vector3(CloudWidth,CloudLength,CloudHeight),2,0.5f,1.f,ambient_gridsizes);
				if(!disable_ambient)
#ifdef SIMUL_USE_TBB
					dls(tbb::blocked_range<size_t>(lighting_index,amb));
#else
					dls(lighting_index,amb);
#endif				
			}
			total_light_remaining=0;
			lighting_index=amb;
		}
		if(lighting_index>=ambient_steps&&steps>ambient_steps)
		{
			unsigned start=lighting_index-ambient_steps;
			unsigned end=steps-ambient_steps;
#ifdef SIMUL_USE_TBB
			if(UseTbb)
			{
				DoLightSteps dls(this,light_grid,secondary_grid,
						light_vector,light_origin,light_scales,light_axis,1.f,
						under_light_multiplier,light_gridsizes);
				tbb::parallel_for(tbb::blocked_range<size_t>(start,end),
					dls,
					tbb::auto_partitioner());
			}
			else
#endif
			{
				DoLightSteps dls(this,light_grid,secondary_grid,
						light_vector,light_origin,light_scales,light_axis,1.f,
						under_light_multiplier,light_gridsizes);
#ifdef SIMUL_USE_TBB
				dls(tbb::blocked_range<size_t>(start,end));
#else
				dls(start,end);
#endif
			}
		}
		lighting_index=steps;
	}
	if(lighting_index>=gridsize)
	{
		relit_checksum=GetRelightChecksum();
		coverage=1.f-total_light_remaining/(float)(light_gridsizes[0]*light_gridsizes[1]);
		lighting_index=0;
		return false;
	}
	return true;
}

unsigned CellularCloudNode::GetMemoryUsage() const
{
	unsigned memory=CellularCloudGrid::GetMemoryUsage();
	memory+=sizeof(*this);
	memory-=sizeof(CellularCloudGrid);
	memory+=light_gridsize*sizeof(unsigned char);
	memory+=light_gridsize*sizeof(unsigned char);
	memory+=ambient_gridsize*sizeof(unsigned char);
	if(timed_noise)
		memory+=timed_noise->GetMemoryUsage();
	return memory;
}

float CellularCloudNode::GetFractalOffsetScale() const
{
	return FractalAmplitude*sqrt(CloudWidth*CloudLength/(float)(GridWidth*GridLength));
}

float CellularCloudNode::GetFractalRepeatLength() const
{	
	return FractalWavelength*sqrt(CloudWidth*CloudLength/(float)(GridWidth*GridLength));
}

unsigned CellularCloudNode::GetRelightChecksum() const
{
	unsigned new_checksum=CellularCloudGrid::GetRebuildChecksum();
	new_checksum+=*reinterpret_cast<const unsigned*>(&SelfShadowScale);
	new_checksum+=*reinterpret_cast<const unsigned*>(&LightExtend);
	new_checksum+=*reinterpret_cast<const unsigned*>(&LightDetail);
	
	new_checksum+=*reinterpret_cast<const unsigned*>(&MinimumLightElevationDegrees);
	new_checksum+=*reinterpret_cast<const unsigned*>(&CloudWidth);
	new_checksum+=*reinterpret_cast<const unsigned*>(&CloudLength);
	new_checksum+=*reinterpret_cast<const unsigned*>(&CloudHeight);
	
	new_checksum+=*reinterpret_cast<const unsigned*>(&NoisePersistence);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Diffusivity);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Extinction);

	new_checksum+=*reinterpret_cast<const unsigned*>(&light_dir[0]);
	new_checksum+=*reinterpret_cast<const unsigned*>(&light_dir[1]);
	new_checksum+=*reinterpret_cast<const unsigned*>(&light_dir[2]);
	
	new_checksum+=*reinterpret_cast<const unsigned*>(&Time);
	new_checksum+=*reinterpret_cast<const unsigned*>(&NoisePeriod);
	return new_checksum;
}

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif