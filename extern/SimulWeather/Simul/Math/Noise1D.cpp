#include "precomp.h"

#include <math.h>
#include <stdlib.h>
#include "./Noise1D.h"
#include "Simul/Math/RandomNumberGenerator.h"
#include "Simul/Graph/Meta/MemoryInterface.h"

using namespace simul::math;

static float cos_interp(float x, float a, float b)
{
	float ft = x * 3.1415927f;
	float f = (1.f - cos(ft)) * .5f;
	return  a*(1.f-f) + b*f;
}

simul::math::Noise1D::Noise1D(simul::graph::meta::MemoryInterface *mem)
	:buffer_size(0)
	,noise_buffer(NULL)
	,memoryInterface(mem)
{
	frequency=0;
	filter=NULL;
	if(memoryInterface)
	{
		noise_random=new(memoryInterface) RandomNumberGenerator;
	}
	else
	{
		noise_random=new RandomNumberGenerator;
	}
}

simul::math::Noise1D::~Noise1D()
{
	if(memoryInterface)
	{
		operator delete[](noise_buffer,memoryInterface);
		operator delete(noise_random,memoryInterface);
	}
	else
	{
		delete [] noise_buffer;
		delete noise_random;
	}
}

void simul::math::Noise1D::Setup(unsigned freq,int RandomSeed,int octaves,float p)
{
	persistence=p;
	numOctaves=octaves;
	noise_random->Seed(RandomSeed);
	frequency=freq;
	if(memoryInterface)
	{
		operator delete[](noise_buffer,memoryInterface);
		buffer_size=frequency;
		noise_buffer=new(memoryInterface) float[frequency];
	}
	else
	{
		delete [] noise_buffer;
		buffer_size=frequency;
		noise_buffer=new float[frequency];
	}
	float *ptr=noise_buffer;
	for(unsigned i=0;i<frequency;i++)
	{
		*ptr =noise_random->FRand();
		ptr++;
	}
}

int simul::math::Noise1D::GetNoiseFrequency() const
{
	return frequency;
}

float simul::math::Noise1D::value_at(unsigned i) const
{
	i=i%frequency;
	return noise_buffer[i];
}

float simul::math::Noise1D::noise1(float p) const
{
	float x=p;
	int i1=((int)x);
	float sx=x-i1;
	int i2=(i1+1);

	float u,v,a;

	u=value_at(i1);		// left back
	v=value_at(i2);		// right front
	a = cos_interp(sx, u, v);		// back x=i1
	return a;
}

float simul::math::Noise1D::PerlinNoise1D(float x) const
{
	int i;
	float pos;
	float val,result = 0,sum=0;
	float scale = .5f;
	pos = x*frequency;
	for (i=0;i<numOctaves;i++)
	{
		val = noise1(pos);
		if(filter)
			val=filter->Filter(val);
		result += val * scale;
		sum+=scale;
		scale *= persistence;
		pos *= 2.f;		// double the position values
	}
	result/=sum;
	return(result);
}
void simul::math::Noise1D::SetFilter(NoiseFilter *nf)
{
	filter=nf;
}

unsigned simul::math::Noise1D::GetMemoryUsage() const
{
	return sizeof(this)+sizeof(RandomNumberGenerator)+buffer_size*sizeof(float);
}