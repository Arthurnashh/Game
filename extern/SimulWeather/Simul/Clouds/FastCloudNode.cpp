#include "precomp.h"

#include "Simul/Clouds/FastCloudNode.h"
#include "Simul/Math/Noise3D.h"
#include "Simul/Base/EnvironmentVariables.h"
#include "Simul/Math/Pi.h"
#include "Simul/Geometry/Orientation.h"
#include <math.h>
#include <fstream>

// Density of pure water vapour: 0.627 kg/m3
using namespace simul::clouds;
using namespace simul::math;
using namespace simul::base;
using namespace simul::graph::meta;

template<class T> void Limit(T &t,T minimum,T maximum)
{
	if(t<minimum)
		t=minimum;
	if(t>maximum)
		t=maximum;
}

FastCloudNode::FastCloudNode() : CellularCloudNode()
{
	InitializePropertiesDefinition();
}

void FastCloudNode::PrepareToDestroy()
{
	CellularCloudNode::PrepareToDestroy();
}

FastCloudNode::~FastCloudNode()
{
}

bool FastCloudNode::Generate()
{
	return CellularCloudNode::Generate();
}

bool FastCloudNode::UpdatePartial(float proportion)
{
	if(NeedsRelight())
	{
		active_humidity=Humidity;
		return ReLightPartial(proportion);
	}
	return false;
}

float FastCloudNode::GetCloud(int x,int y,int z) const
{
	if(!Wrap)
	{
		if(x<=0||y<=0||z<0)
			return 0;
		if(x>=(int)GridWidth-1||y>=(int)GridLength-1||z>=(int)GridHeight)
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

float FastCloudNode::GetCloud(unsigned x,unsigned y,unsigned z) const
{
	if(!enabled||x>=GridWidth||y>=GridLength||z>=GridHeight)
		return 0;
	if((z==0||z>=GridHeight-1)&&GridHeight>2)
		return 0;
	float noise_val=GetNoiseAtGrid(z,y,x);
	float hm=1.f;
	for(size_t i=0;i<humidityCallbacks.size();i++)
		hm*=humidityCallbacks[i]->GetHumidityMultiplier((float)x/(float)GridWidth,(float)y/(float)GridLength,(float)z/(float)GridHeight);

//	float threshold=pow(1.f-active_humidity,hm);
//	float ret=(noise_val-threshold)/Diffusivity;
	static float threshold=1.f;
	float ret=(noise_val+active_humidity*hm-threshold)/Diffusivity;
	float val=std::max(std::min(1.f,ret),0.f);

	return val;
}

float FastCloudNode::GetCloudDensity(const simul::math::Vector3 &X) const
{
	int x=0,y=0,z=0;
	Vector3 D,d;
	Subtract(d,X,X1);
	d/=DX;
	MultiplyElements(D,d,Vector3((float)GridWidth,(float)GridLength,(float)GridHeight));
	x=(int)D.x;
	y=(int)D.y;
	z=(int)D.z;
	if(z<=0||z>=(int)GridHeight-1)
		return 0;
	if(!Wrap)
	{
		if(x<0||x>=(int)GridWidth)
			return 0;
		if(y<0||y>=(int)GridLength)
			return 0;
	}
	else
	{
		while(x<0)
			x+=GridWidth;
		while(y<0)
			y+=GridLength;
		if(x>=(int)GridWidth)
			x=x%GridWidth;
		if(y>=(int)GridLength)
			y=y%GridLength;
	}
	return GetCloud(x,y,z);
}

std::ostream &FastCloudNode::StateStreamOut(std::ostream &os) const
{
	return os;
}

std::istream &FastCloudNode::StateStreamIn(std::istream &is)
{
	return is;
}

std::ostream &FastCloudNode::StreamOut(std::ostream &os) const
{
	return os;
}

std::istream &FastCloudNode::StreamIn(std::istream &is)
{
	return is;
}

void FastCloudNode::AddHumidityCallback(HumidityCallbackInterface *hc)
{
	for(size_t i=0;i<humidityCallbacks.size();i++)
		if(humidityCallbacks[i]==hc)
			return;
	humidityCallbacks.push_back(hc);
}

void FastCloudNode::RemoveHumidityCallback(HumidityCallbackInterface *hc)
{
	for(size_t i=0;i<humidityCallbacks.size();i++)
		if(humidityCallbacks[i]==hc)
			humidityCallbacks.erase(humidityCallbacks.begin()+i);
}

void FastCloudNode::ClearHumidityCallbacks()
{
	humidityCallbacks.clear();
}