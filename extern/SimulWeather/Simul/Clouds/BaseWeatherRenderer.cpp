#include "precomp.h"

#include "BaseWeatherRenderer.h"
#include "Simul/Sky/BaseSkyRenderer.h"
#include "Simul/Clouds/BaseCloudRenderer.h"
#include "Simul/Clouds/BasePrecipitationRenderer.h"
using namespace simul;
using namespace clouds;

static void writechar(std::ostream &os,char c)
{
	os.write((const char*)&c,sizeof(c));
}

static char readchar(std::istream &is)
{
	char c;
	is.read((char*)&c,sizeof(c));
	return c;
}

BaseWeatherRenderer::BaseWeatherRenderer()
	:baseSkyRenderer(NULL)
	,baseCloudRenderer(NULL)
	,base2DCloudRenderer(NULL)
	,basePrecipitationRenderer(NULL)
	,baseLightningRenderer(NULL)
	,layer1(true)
	,layer2(true)
	,Godrays(false)
{
	InitializePropertiesDefinition();
}

BaseWeatherRenderer::~BaseWeatherRenderer()
{
}

// Convenient interfaces:
void BaseWeatherRenderer::SetDaytime(float dt)
{
	if(baseSkyRenderer)
		baseSkyRenderer->GetSkyKeyframer()->SetDaytime(dt);
}

void BaseWeatherRenderer::SetCloudbaseKm(float h_km)
{
	if(baseCloudRenderer)
		baseCloudRenderer->SetCloudBaseKm(h_km);
}

const simul::sky::float4 &BaseWeatherRenderer::GetDirectionToLight()
{
	static simul::sky::float4 result(0.f,0.f,1.f,0.f);
	if(baseSkyRenderer)
		return baseSkyRenderer->GetSkyKeyframer()->GetDirectionToLight();
	return result;
}

const simul::sky::float4 &BaseWeatherRenderer::GetHorizonColour(float view_altitude_km)
{
	static simul::sky::float4 result(1.f,1.f,1.f,1.f);
	if(baseSkyRenderer)
	{
		simul::sky::float4 insc=baseSkyRenderer->GetSkyKeyframer()->GetIsotropicInscatterFactor(view_altitude_km,0.f,0.f,1e5f);
		simul::sky::float4 mul=baseSkyRenderer->GetSkyKeyframer()->GetInscatterAngularMultiplier(0.f,insc.w,view_altitude_km);
		result=insc*mul;
	}
	return result;
}

const simul::sky::float4 &BaseWeatherRenderer::GetLightColour(float view_altitude_km) const
{
	static simul::sky::float4 result(1.f,1.f,1.f,1.f);
	if(baseSkyRenderer)
		result=baseSkyRenderer->GetSkyKeyframer()->GetLocalIrradiance(view_altitude_km);
	return result;
}

const simul::sky::float4 &BaseWeatherRenderer::GetAmbientLightColour(float view_altitude_km) const
{
	static simul::sky::float4 result(0.f,0.f,0.f,1.f);
	if(baseSkyRenderer)
		result=baseSkyRenderer->GetSkyKeyframer()->GetAmbientLight(view_altitude_km);
	return result;
}
//! Convenience function to get the next sky keyframe that can be modified without any recalculation.
simul::sky::SkyKeyframer *BaseWeatherRenderer::GetSkyKeyframer()
{
	if(baseSkyRenderer)
		return baseSkyRenderer->GetSkyKeyframer();
	return NULL;
}
//! Convenience function to get the next cloud keyframe that can be modified without any recalculation.
simul::clouds::CloudKeyframer *BaseWeatherRenderer::GetCloudKeyframer()
{
	if(baseCloudRenderer)
		return baseCloudRenderer->GetCloudKeyframer();
	return NULL;
}

void BaseWeatherRenderer::EnableSky(bool s)
{
	show_sky=s;
}

void BaseWeatherRenderer::EnableLayers(bool clouds3d,bool clouds2d)
{
	layer1=clouds3d;
	layer2=clouds2d;
}

std::ostream &BaseWeatherRenderer::Save(std::ostream &os) const
{
	//\211   P   N   G  \r  \n \032 \n
	writechar(os,-45);
	writechar(os,'S');
	writechar(os,'E');
	writechar(os,'Q');
	writechar(os,'\r');
	writechar(os,'\n');
	writechar(os,32);
	writechar(os,'\n');

	int num_streams=0;
	if(baseSkyRenderer)
		num_streams++;
	if(baseCloudRenderer)
		num_streams++;
	if(base2DCloudRenderer)
		num_streams++;
	if(basePrecipitationRenderer)
		num_streams++;
	os.write((const char*)&num_streams,sizeof(num_streams));
	int stream_type=0;
	if(baseSkyRenderer)
	{
		stream_type=baseSkyRenderer->IsColourSkyEnabled()?2:1;
		os.write((const char*)&stream_type,sizeof(stream_type));
		baseSkyRenderer->Save(os);
	}
	if(layer1&&baseCloudRenderer)
	{
		stream_type=0;
		os.write((const char*)&stream_type,sizeof(stream_type));
		baseCloudRenderer->Save(os);
	}
	if(layer2&&base2DCloudRenderer)
	{
		stream_type=4;
		os.write((const char*)&stream_type,sizeof(stream_type));
		base2DCloudRenderer->Save(os);
	}
	if(basePrecipitationRenderer)
	{
		stream_type=3;
		os.write((const char*)&stream_type,sizeof(stream_type));
		basePrecipitationRenderer->Save(os);
	}
	return os;
}

std::istream &BaseWeatherRenderer::Load(std::istream &is)
{
	if(readchar(is)!=-45) return is;
	if(readchar(is)!='S') return is;
	if(readchar(is)!='E') return is;
	if(readchar(is)!='Q') return is;
	if(readchar(is)!='\r') return is;
	if(readchar(is)!='\n') return is;
	if(readchar(is)!=32) return is;
	if(readchar(is)!='\n') return is;
	int num_streams=0;
	is.read((char*)&num_streams,sizeof(num_streams));
	for(int i=0;i<num_streams;i++)
	{
		int stream_type=-1;
		is.read((char*)&stream_type,sizeof(stream_type));
		if((stream_type==1||stream_type==2)&&baseSkyRenderer)
		{
			baseSkyRenderer->EnableColourSky(stream_type==2);
			baseSkyRenderer->Load(is);
		}
		else if(stream_type==0)
		{
			layer1=true;
			EnableLayers(layer1,layer2);
			if(baseCloudRenderer)
				baseCloudRenderer->Load(is);
		}
		else if(stream_type==4)
		{
			layer2=true;
			EnableLayers(layer1,layer2);
			if(base2DCloudRenderer)
				base2DCloudRenderer->Load(is);
		}
		else if(stream_type==3&&basePrecipitationRenderer)
		{
			basePrecipitationRenderer->Load(is);
		}
		else
		{
			std::cerr<<"Unknown stream type "<<stream_type<<std::endl;
		}
	}
	ConnectInterfaces();
	return is;
}

void BaseWeatherRenderer::New()
{
	if(baseSkyRenderer)
		baseSkyRenderer->New();
	if(baseCloudRenderer)
		baseCloudRenderer->New();
	if(base2DCloudRenderer)
		base2DCloudRenderer->New();
	if(basePrecipitationRenderer)
		basePrecipitationRenderer->New();
}

void BaseWeatherRenderer::SetPrecipitation(float strength)
{
	baseCloudRenderer->SetPrecipitation(strength);
}