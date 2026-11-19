#include <algorithm>
#include "precomp.h"

#if defined(_MSC_VER)
	#if defined(_DLL) && !defined(XENON)
		#define SIMUL_USE_TBB
	#endif
#endif

#include "Simul/Clouds/CellularCloudGrid.h"
#include "Simul/Graph/StandardNodes/ShowProgressInterface.h"
#include "Simul/Graph/Meta/MemoryInterface.h"
#include <math.h>
#include <bitset>
// Include string.h for memset:
#include <string.h>

#ifdef SIMUL_USE_TBB
	#define TBB_DO_THREADING_TOOLS 1
	#include "tbb/parallel_for.h" 
	#include "tbb/blocked_range.h"
	#include "tbb/task_scheduler_init.h"
	#ifndef SIMUL_DLL
		class AutoInitTBB
		{
			tbb::task_scheduler_init *init;
		public:
			AutoInitTBB()
			{
				init=new tbb::task_scheduler_init();
				//init->initialize();
			}
			~AutoInitTBB()
			{
				init->terminate();
				delete init;
			}
		};
		//AutoInitTBB AutoInitTBBInstance;
	#endif
#endif

#include <time.h>
#ifdef _MSC_VER
#include <windows.h>
#undef min
#endif

using namespace simul::clouds;
using namespace std;
static unsigned ActivationRange=1;

static unsigned PowerOfTwo(unsigned unum)
{
	float num=(float)unum;
	if(fabs(num)<1e-8f)
		num=1.f;
	float Exp=log(num);
	Exp/=log(2.f);
	return (unsigned)Exp;
}
using namespace simul::clouds;

CellularCloudGrid::CellularCloudGrid()
	:GridLength(0)
	,GridWidth(0)
	,GridHeight(0)
	,UseTbb(false)
	,enabled(true)
{
	InitializePropertiesDefinition();
	Wrap=true;
	Humidity=.4f;
	cld=NULL;
	RandomSeed=1;
	gridsize=0;
	noiseInterp=0.f;
	noise=NULL;
	active_humidity=Humidity;
	license_code[0]=0x12345678;
	license_code[1]=0xABCDEF12;
	ChangedTbb();
}

void CellularCloudGrid::NeedRecalculate()
{
	GridLength	=1<<PowerOfTwo(GridLength);
	GridWidth	=1<<PowerOfTwo(GridWidth);
	GridHeight	=1<<PowerOfTwo(GridHeight);
	Recalculate();
}

void CellularCloudGrid::SetLicense(const char *txt)
{
	license_code[0]=license_code[1]=0;
	if(!txt)
	{
		std::cerr<<"Simul SDK: no license in CellularCloudGrid::SetLicense"<<std::endl;
		return;
	}
	size_t len=strlen(txt);
	if(len!=16)
	{
		std::cerr<<"Simul SDK: Incorrect license string: ";
		if(len<100)
			std::cerr<<txt;
		std::cerr<<std::endl;
		std::cerr<<"The license string in CellularCloudGrid::SetLicense() must be exactly 16 characters long."<<std::endl;
		return;
    }
#ifdef _MSC_VER
	sscanf_s(txt,"%8X",&(license_code[0]));
	sscanf_s(&(txt[8]),"%8X",&(license_code[1]));
#else
	sscanf(txt,"%8X",&(license_code[0]));
	sscanf(&(txt[8]),"%8X",&(license_code[1]));
#endif
#ifndef XBOX
	std::cout<<"Simul SDK: CellularCloudGrid::SetLicense "<<txt<<std::endl;
#endif
}

bool CellularCloudGrid::CheckLicense() const
{
	bool enabled=false;
#undef VFORGE_OK
	#include "Licensing.inl"
	return enabled;
}

void CellularCloudGrid::Init(simul::graph::meta::MemoryInterface *mi,
							 simul::math::NoiseInterface *ni,
							 unsigned l,unsigned w,unsigned h)
{
	active_humidity=Humidity;
#define VFORGE_OK
#include "Licensing.inl"
	if(ni)
		noise=ni;
	random.Seed(RandomSeed);
	if((w%8*sizeof(unsigned))!=0)
		w=0;
	if(l)
		GridLength=l;
	if(w)
		GridWidth=w;
	if(h)
		GridHeight=h;
	if(GridLength<1)
		GridLength=1;
	if(GridWidth<1)
		GridWidth=1;
	if(GridHeight<1)
		GridHeight=1;
	unsigned nbits=(8*sizeof(unsigned));
	w32=GridWidth;
	if(w32<32)
		w32=32;
	unsigned W=w32/nbits;
	if(W<1)
		W=1;
	unsigned size=GridLength*w32*GridHeight/nbits;
	if(size!=gridsize)
	{
		gridsize=size;
		if(mi)
		{
			operator delete[](cld,mi);
			cld=new(mi) unsigned[size];
		}
		else
		{
			delete [] cld;
			cld=new unsigned[size];
		}
	}
	memset(cld,0,size*sizeof(unsigned));
	//#pragma omp for
#if 0
#ifdef SIMUL_USE_TBB
	if(!UseTbb)
#endif
	{
		for(unsigned i = 0; i < GridHeight; ++i)
		{
			if(progress)
			{
				progress->SetProgress(caller,(float)i/(float)GridHeight);
				if(progress->GetCancel(caller))
					break;
			}
#ifdef SIMUL_USE_TBB
			ApplyInit af(this,i,W);
			tbb::blocked_range<size_t> b(0,GridLength);
			af(b);
#else
			unsigned index=0;
			for(unsigned j=0;j<GridLength;++j)
			{
				for(unsigned K=0;K<W;++K,++index)
				{
					MakeNoiseCache(i,j,K);
				}
			}
#endif
		}
	}
#ifdef SIMUL_USE_TBB
	else 
	{
		for(unsigned i = 0; i < GridHeight; ++i)
		{
			if(progress)
			{
				progress->SetProgress(caller,(float)i/(float)GridHeight);
				if(progress->GetCancel(caller))
					break;
			}
			tbb::parallel_for(
				tbb::blocked_range<size_t>(0,GridLength),
				ApplyInit(this,i,W),
				tbb::auto_partitioner()); 
		}
	}
#endif
#endif
}

float CellularCloudGrid::GetNoiseAtGrid(int i,int j,int k) const
{
	return noise->PerlinNoise3D((int)k,(int)j,(int)i);
}


void CellularCloudGrid::GetIndexAndBit(unsigned x,unsigned y,unsigned z,unsigned &index,unsigned &b) const
{
	index=z*GridLength*w32+y*w32+x;
	b=index%(8*sizeof(unsigned));
	index/=(8*sizeof(unsigned));
}

void CellularCloudGrid::SetCloud(unsigned x,unsigned y,unsigned z)
{
	unsigned index=0;
	unsigned b=0;
	GetIndexAndBit(x,y,z,index,b);
	cld[index]|=((unsigned)1<<(unsigned)b);
}

float CellularCloudGrid::GetCloud(int x,int y,int z) const
{
	if(!Wrap)
	{
		if(x>=(int)GridWidth||y>=(int)GridLength||z>=(int)GridHeight)
			return 0;
	}
	else
	{
		if(z<0||z>=(int)GridHeight)
			return 0;
		while(x<0)
			x+=GridWidth;
		while(y<0)
			y+=GridLength;
		x=x%GridWidth;
		y=y%GridLength;
	}
	return GetCloud((unsigned)x,(unsigned)y,(unsigned)z);
}
static float lerp(float l,float h,float f)
{
	return l+((h-l)*f);
}

float CellularCloudGrid::InterpCloud(float x,float y,float z) const
{
	int grid_x0=(int)x;
	int grid_y0=(int)y;
	int grid_z0=(int)z;
	if(x<0)
		grid_x0-=1;
	if(y<0)
		grid_y0-=1;
	if(z<0)
		grid_z0-=1;
	int grid_x1=grid_x0+1;
	int grid_y1=grid_y0+1;
	int grid_z1=grid_z0+1;
	float interp_x=x-grid_x0;
	float interp_y=y-grid_y0;
	float interp_z=z-grid_z0;
	float bot_back,bot_front;
	bot_back=lerp((float)GetCloud(grid_x0,grid_y0,grid_z0),
				(float)GetCloud(grid_x1,grid_y0,grid_z0),interp_x);
	bot_front=lerp((float)GetCloud(grid_x0,grid_y1,grid_z0),
					(float)GetCloud(grid_x1,grid_y1,grid_z0),interp_x);
	
	float bot=lerp(bot_back,bot_front,interp_y);
	
	float top_back,top_front;
	top_back=lerp(	(float)GetCloud(grid_x0,grid_y0,grid_z1),
					(float)GetCloud(grid_x1,grid_y0,grid_z1),interp_x);
	top_front=lerp(	(float)GetCloud(grid_x0,grid_y1,grid_z1),
					(float)GetCloud(grid_x1,grid_y1,grid_z1),interp_x);
	
	float top=lerp(top_back,top_front,interp_y);
	float ret=lerp(bot,top,interp_z);
	return ret;
}

float CellularCloudGrid::GetCloud(unsigned x,unsigned y,unsigned z) const
{
	if(x>=GridWidth||y>=GridLength||z>=GridHeight)
		return 0;
	if(!cld)
		return 0;
	unsigned index=0;
	unsigned b=0;
	GetIndexAndBit(x,y,z,index,b);
	bool ret=((cld[index]&((unsigned)1<<(unsigned)b))!=0);
	return (float)ret;
}

void CellularCloudGrid::DeallocateMemory(simul::graph::meta::MemoryInterface *mi)
{
	if(mi)
	{
		operator delete[](cld,mi);
	}
	else
	{
		delete [] cld;
	}
	cld=NULL;
}

CellularCloudGrid::~CellularCloudGrid()
{
}


void CellularCloudGrid::SaveGrid(std::ostream &os) const
{
	os.write((const char *)&GridLength	,sizeof(GridLength));
	os.write((const char *)&GridWidth	,sizeof(GridWidth));
	os.write((const char *)&GridHeight	,sizeof(GridHeight));

	os.write((const char *)cld,gridsize*sizeof(unsigned));
}

void CellularCloudGrid::LoadGrid(std::istream &is)
{
	is.read((char *)&GridLength	,sizeof(GridLength));
	is.read((char *)&GridWidth	,sizeof(GridWidth));
	is.read((char *)&GridHeight	,sizeof(GridHeight));
	w32=GridWidth;
	if(w32<32)
		w32=32;
	unsigned nbits=(8*sizeof(unsigned));
	unsigned size=GridLength*w32*GridHeight/nbits;
	if(size!=gridsize)
	{
		gridsize=size;
		delete [] cld;
		cld=new unsigned[size];
	}
	memset(cld,0,size*sizeof(unsigned));
	is.read((char *)cld,gridsize*sizeof(unsigned));
}

unsigned CellularCloudGrid::GetMemoryUsage() const
{
	unsigned memory=sizeof(*this);
	memory+=2*gridsize*sizeof(unsigned);
	return memory;
}

void CellularCloudGrid::ChangedTbb()
{
	#ifndef _DLL
	if(UseTbb)
	{
		UseTbb=false;
#ifndef XENON
		std::cerr<<"CellularCloudGrid::ChangedTbb - Can't use Threading Building Blocks with Static Runtimes (MT, MTd)"<<std::endl;
#endif
	}
	#endif
	#ifdef _DEBUG
	if(UseTbb)
	{
		UseTbb=false;
#ifndef XENON
		std::cerr<<"CellularCloudGrid::ChangedTbb - Can't use Threading Building Blocks with Debug mode."<<std::endl;
#endif
	}
	#endif
#ifdef SIMUL_USE_TBB
	if(UseTbb)
	{
		// Is 
		HMODULE hModule=LoadLibrary("tbb.dll");
		if(!hModule)
		{
#ifndef XENON
			std::cerr<<"Can't use Threading Building Blocks as the tbb.dll is not found on the DLL search path."<<std::endl;
#endif
			UseTbb=false;
			return;
		}
		ThreadSafety=false;
		// should lock/unlock here!
	}
#endif
// Should NOT need to recalculate!
	//NeedRecalculate();
}

void CellularCloudGrid::ChangedThreadSafety()
{
	if(ThreadSafety)
		UseTbb=false;
	else
	{
		// should lock/unlock here!
	}
	NeedRecalculate();
}

void CellularCloudGrid::ChangedNoise()
{
	NoiseResolution	=1<<PowerOfTwo(NoiseResolution);
	LimitNoiseByGrid();
	Recalculate();
}

void CellularCloudGrid::ChangedGrid()
{
	GridLength	=1<<PowerOfTwo(GridLength);
	GridWidth	=1<<PowerOfTwo(GridWidth);
	GridHeight	=1<<PowerOfTwo(GridHeight);
	LimitNoiseByGrid();
	Recalculate();
}

void CellularCloudGrid::LimitNoiseByGrid()
{
	if(NoiseResolution>GridWidth)
		NoiseResolution=GridWidth;
	if(NoiseResolution>GridLength)
		NoiseResolution=GridLength;
	unsigned final_noise_octaves=NoiseResolution<<(NoiseOctaves-1);
	if(final_noise_octaves>GridLength||final_noise_octaves>GridWidth)
	{
		unsigned ratio=std::min(GridLength,GridWidth)/NoiseResolution;
		NoiseOctaves=1+PowerOfTwo(ratio);
	}
}

unsigned CellularCloudGrid::GetRebuildChecksum() const
{
	unsigned new_checksum=0;
	new_checksum+=*reinterpret_cast<const unsigned*>(&GridLength);
	new_checksum+=*reinterpret_cast<const unsigned*>(&GridWidth);
	new_checksum+=*reinterpret_cast<const unsigned*>(&GridHeight);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Humidity);
	new_checksum+=*reinterpret_cast<const unsigned*>(&RandomSeed);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Wrap);
	new_checksum+=*reinterpret_cast<const unsigned*>(&NoiseResolution);
	new_checksum+=*reinterpret_cast<const unsigned*>(&NoiseOctaves);
	return new_checksum;
}