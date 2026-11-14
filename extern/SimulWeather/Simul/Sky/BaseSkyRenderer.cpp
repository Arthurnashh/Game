#include "precomp.h"

#include "Simul/Sky/BaseSkyRenderer.h"
#include "Simul/Sky/SkyKeyframer.h"
#include "Simul/Sky/ColourSkyKeyframer.h"
#include "Simul/Sky/SkyNode.h"
using namespace simul;
using namespace sky;

BaseSkyRenderer::BaseSkyRenderer()
	:skyTexSize(0)
	,fadeTexWidth(0)
	,fadeTexHeight(0)
	,skyTexIndex(0)
	,numAltitudes(0)
	,NumAltitudes(4)
	,NumDistances(16)
	,NumElevations(64)
	,sun_occlusion(0.f)
	,sun_angular_size(3.14159f/180.f/2.f)
	,flare_angular_size(3.14159f/180.f*20.f/2.f)
	,flare_magnitude(0.f)
	,skyInterface(NULL)
	,moon_texture(NULL)
	,moon_index(888)
	,MaxFadeDistanceKm(300.f)
{
}

BaseSkyRenderer::~BaseSkyRenderer()
{
}

bool BaseSkyRenderer::IsColourSkyEnabled()
{
	return(dynamic_cast<simul::sky::ColourSkyKeyframer*>(skyKeyframer.get())!=NULL);
}

float BaseSkyRenderer::GetAltitudeTextureCoordinate() const
{
	if(!skyKeyframer.get())
		return 0.f;
	return skyKeyframer->GetAltitudeTexCoord();
}

void BaseSkyRenderer::EnableColourSky(bool UseColourSky)
{
	if(IsColourSkyEnabled()!=UseColourSky||!skyNode.get())
	{
		if(skyNode.get())
			RemoveChild(skyNode.get());
		if(UseColourSky)
		{
			simul::sky::ColourSkyNode *csn=new simul::sky::ColourSkyNode();
			skyNode=csn;
		}
		else
			skyNode=new simul::sky::SkyNode();
		AddChild(skyNode.get());
	}
	skyInterface			=dynamic_cast<simul::sky::SkyInterface*>(skyNode.get());
	bool is_already_colour	=(dynamic_cast<simul::sky::ColourSkyKeyframer*>(skyKeyframer.get())!=NULL);
	bool as_blocks=false;
	if(skyKeyframer.get())
	{
		as_blocks=skyKeyframer->GetFillTexturesAsBlocks();
	}	
	if(skyKeyframer.get()==NULL||(UseColourSky!=is_already_colour))
	{
		simul::sky::SkyKeyframer *aft=NULL;
		if(UseColourSky)
		{
			aft=new simul::sky::ColourSkyKeyframer(skyInterface,0,NumElevations,NumDistances,MaxFadeDistanceKm);
		}
		else
		{
			aft=new simul::sky::SkyKeyframer(skyInterface,0,NumElevations,NumDistances,MaxFadeDistanceKm);
		}
		aft->SetFillTexturesAsBlocks(as_blocks);
		if(skyKeyframer.get())
		{
			*aft=*skyKeyframer;
		}
		aft->SetNumAltitudes(NumAltitudes);
		skyKeyframer=aft;
		fadeTableInterface=dynamic_cast<simul::sky::FadeTableInterface*>(skyKeyframer.get());
		fadeTableInterface->SetCallback(this);
		if(UseColourSky)
		{
			((simul::sky::ColourSkyKeyframer*)skyKeyframer.get())->GenColours();
		}
	}
}

void BaseSkyRenderer::GridChanged()
{
	if(skyKeyframer.get())
	{
		skyKeyframer->SetNumAltitudes(NumAltitudes);
		skyKeyframer->SetTableWidth(NumDistances);
		skyKeyframer->SetTableHeight(NumElevations);
	}
}

simul::sky::SkyKeyframer *BaseSkyRenderer::GetSkyKeyframer()
{
	return skyKeyframer.get();
}

void BaseSkyRenderer::MaxFadeDistanceChanged()
{
	skyKeyframer->SetMaxDistanceKm(MaxFadeDistanceKm);
}

void BaseSkyRenderer::SetCameraPosition(float x,float y,float z)
{
	cam_pos.Set(x,y,z,0);
	skyKeyframer->SetAltitudeKM((IsYVertical()?cam_pos.y:cam_pos.z)*0.001f);
}

void BaseSkyRenderer::Update(float dt)
{
	skyKeyframer->TimeStep(dt);
	skyKeyframer->Update();
	if(moon_enabled)
	{
		simul::sky::float4 dir=skyInterface->GetDirectionToMoon();
		if(IsYVertical())
			std::swap(dir.y,dir.z);
		SetPlanetDirection(moon_index,dir);
		SetPlanetRadius(moon_index,skyInterface->GetMoonRadiusArcMinutes()*3.14159f/180.f/60.f);
		simul::sky::float4 colr=0.136f*skyInterface->GetSunIrradiance();
		// Don't let the moon get TOO bright:
		float brightness=simul::sky::length(colr);
		if(brightness>10.f)
			colr*=10.f/brightness;
		SetPlanetColour(moon_index,colr);

	}
}

void BaseSkyRenderer::RenderPlanets()
{
	for(std::map<int,PlanetStruct>::iterator i=planets.begin();i!=planets.end();i++)
	{
		RenderPlanet((*i).second.pTexturePtr,i->second.angular_radius,i->second.dir,i->second.colour,i->second.do_lighting);
	}
}

void BaseSkyRenderer::EnableMoon(bool val)
{
	moon_enabled=val;
	if(moon_enabled)
	{
		SetPlanet(moon_index,moon_texture,0.02f,true);
	}
}

bool BaseSkyRenderer::IsMoonEnabled() const
	{
	return moon_enabled;
	}

void BaseSkyRenderer::SetPlanet(int index,void* tex,float rad,bool do_lighting)
{
	if(tex==NULL)
	{
		if(planets.find(index)!=planets.end())
			planets.erase(index);
	}
	if(planets.find(index)==planets.end())
	{
		planets[index].dir[0]=1.f;
		planets[index].dir[1]=0;
		planets[index].dir[2]=0;
		planets[index].colour[0]=1.f;
		planets[index].colour[1]=1.f;
		planets[index].colour[2]=1.f;
		planets[index].angular_radius=rad;
		planets[index].do_lighting=do_lighting;
	}
	planets[index].pTexturePtr=tex;
}

void BaseSkyRenderer::SetPlanetImage(int index,void* tex)
{
	if(planets.find(index)==planets.end())
	{
		planets[index].dir[0]=1.f;
		planets[index].dir[1]=0;
		planets[index].dir[2]=0;
		planets[index].colour[0]=1.f;
		planets[index].colour[1]=1.f;
		planets[index].colour[2]=1.f;
		planets[index].angular_radius=0.02f;
		planets[index].do_lighting=true;
	}
	planets[index].pTexturePtr=tex;
}


void BaseSkyRenderer::SetPlanetDirection(int index,const float *pos)
{
	if(planets.find(index)==planets.end())
		return;
	planets[index].dir[0]=pos[0];
	planets[index].dir[1]=pos[1];
	planets[index].dir[2]=pos[2];
}

void BaseSkyRenderer::SetPlanetColour(int index,const float *c3)
{
	if(planets.find(index)==planets.end())
		return;
	planets[index].colour[0]=c3[0];
	planets[index].colour[1]=c3[1];
	planets[index].colour[2]=c3[2];
}

void BaseSkyRenderer::SetPlanetRadius(int index,float radians)
{
	if(planets.find(index)==planets.end())
		return;
	planets[index].angular_radius=radians;
}

PlanetStruct *BaseSkyRenderer::GetPlanet(int index)
{
	if(planets.find(index)==planets.end())
		return NULL;
	return &(planets[index]);
}

std::ostream &BaseSkyRenderer::Save(std::ostream &os) const
{
	skyKeyframer->Save(os);
	return os;
}

std::istream &BaseSkyRenderer::Load(std::istream &is) const
{
	skyKeyframer->Load(is);
	return is;
}

void BaseSkyRenderer::New()
{
	skyKeyframer->New();
}

simul::sky::SkyInterface *BaseSkyRenderer::GetSkyInterface()
{
	return skyInterface;
}

simul::sky::BaseSkyInterface *BaseSkyRenderer::GetBaseSkyInterface()
{
	return skyKeyframer.get();
}


simul::sky::SiderealSkyInterface *BaseSkyRenderer::GetSiderealSkyInterface()
{
	return dynamic_cast<simul::sky::SiderealSkyInterface*>(skyNode.get());
}

simul::sky::FadeTableInterface *BaseSkyRenderer::GetFadeTableInterface()
{
	return skyKeyframer.get();
}

void BaseSkyRenderer::SetOvercastCallback(simul::sky::OvercastCallback *ocb)
{
	skyKeyframer->SetOvercastCallback(ocb);
}

simul::sky::float4 BaseSkyRenderer::GetLightColour()
{
	return skyKeyframer->GetLocalIrradiance(skyKeyframer->GetAltitudeKM());
}

simul::sky::float4 BaseSkyRenderer::GetAmbientColour()
{
	return skyKeyframer->GetAmbientLight(skyKeyframer->GetAltitudeKM());
}

simul::sky::float4 BaseSkyRenderer::GetDirectionToLight()
{
	simul::sky::float4 dir=skyKeyframer->GetDirectionToLight();
	if(IsYVertical())
		std::swap(dir.y,dir.z);
	return dir;
}

simul::sky::float4 BaseSkyRenderer::GetDirectionToSun()
{
	simul::sky::float4 dir=skyKeyframer->GetDirectionToSun();
	if(IsYVertical())
		std::swap(dir.y,dir.z);
	return dir;
}

float BaseSkyRenderer::CalcSunOcclusion(float cloud_occlusion)
{
	sun_occlusion=cloud_occlusion;
	return sun_occlusion;
}
