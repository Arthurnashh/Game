#include "precomp.h"

#include "Simul/Clouds/GeoCloudNode.h"
#include "Simul/Clouds/TimedNoise3D.h"
using namespace simul;
using namespace clouds;
GeoCloudNode::GeoCloudNode():ThunderCloudNode()
	,WindowX(0)
	,WindowY(0)
{
}

GeoCloudNode::~GeoCloudNode()
{
}

float GeoCloudNode::GetCloud(unsigned x,unsigned y,unsigned z) const
{
	if(x>=GridWidth||y>=GridLength||z>=GridHeight)
		return 0;
	if((z==0||z>=GridHeight-1)&&GridHeight>2)
		return 0;
	float hm=1.f;
	int offsetted_x=((int)x+WindowX);
	int offsetted_y=((int)y+WindowY);
	hm=0.5f+timed_noise->PerlinNoise3D((float)offsetted_x/(float)GridWidth,(float)offsetted_y/(float)GridLength,0.f);
	for(size_t i=0;i<humidityCallbacks.size();i++)
		hm=humidityCallbacks[i]->GetHumidityMultiplier(CloudWidth*(float)offsetted_x/(float)GridWidth,
		CloudLength*(float)offsetted_y/(float)GridLength,
		CloudHeight*(float)z/(float)GridHeight);
	hm*=(1.5f-(float)z/(float)GridHeight);
	float hhh=1.f-active_humidity*hm;
	float val=GetNoiseAtGrid(z,offsetted_y,offsetted_x);
	val=std::max(std::min(1.f,(val-hhh)/Diffusivity),0.f);

	return val;
}

void GeoCloudNode::WindowChanged()
{
	CentreX=(float)WindowX/(float)GridWidth *CloudWidth;
	CentreY=(float)WindowY/(float)GridLength*CloudLength;
}