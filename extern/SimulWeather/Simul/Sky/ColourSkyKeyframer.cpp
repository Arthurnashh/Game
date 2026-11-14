#include "precomp.h"

#include "ColourSkyKeyframer.h"
#include "Simul/Math/Pi.h"
#include "Simul/Sky/SkyNode.h"
#include <assert.h>
#ifdef _MSC_VER
#else
#include <stdio.h>
#include <strings.h>
#define _stricmp strcasecmp
#endif

using namespace simul::sky;

// Magic numbers for file format:
static const unsigned KEYFRAME_MAGIC=0x09B83FF3;


ColourSkyKeyframer::ColourSkyKeyframer(SkyInterface *si,SkyTexturesCallback *c,int fade_height,int fade_width,float max_dist_km)
	:SkyKeyframer(si,c,fade_height,fade_width,max_dist_km)
{
	colourSky=dynamic_cast<ColourSky*>(si);
	assert(colourSky);
	ClearKeyframes();
	keyframes.push_back(new ColourKeyframe());
	keyframes.push_back(new ColourKeyframe());
	keyframes.push_back(new ColourKeyframe());
	keyframes.push_back(new ColourKeyframe());

	ColourKeyframe *midnight=dynamic_cast<ColourKeyframe*>(keyframes[0]);
	midnight->time=0;
	midnight->Horizon.Set(0,0,0.1f,0);
	midnight->Zenith.Set(0,0,0.1f,0);
	midnight->Sunlight.Set(0,0,0,0);
	midnight->SunElevation=-pi/2.f;
	midnight->SunAzimuth=pi/2.f;

	ColourKeyframe *dawn=dynamic_cast<ColourKeyframe*>(keyframes[1]);
	dawn->time=0.25f;
	dawn->Horizon.Set(1.f,0.5f,0.5f,0);
	dawn->Zenith.Set(0,0.5f,1.f,0);
	dawn->Sunlight.Set(1.f,0.2f,0,0);
	midnight->SunElevation=0;
	midnight->SunAzimuth=-pi/2.f;

	ColourKeyframe *midday=dynamic_cast<ColourKeyframe*>(keyframes[2]);
	midday->time=0.5f;
	midday->Horizon.Set(1.f,1.f,1.f,0);
	midday->Zenith.Set(0,0.5f,1.f,0);
	midday->Sunlight.Set(1.f,1.f,1.f,0);
	midnight->SunElevation=pi/2.f;
	midnight->SunAzimuth=-pi/2.f;

	ColourKeyframe *sunset=dynamic_cast<ColourKeyframe*>(keyframes[3]);
	sunset->time=0.75f;
	sunset->Horizon.Set(1.f,0.5f,0.5f,0);
	sunset->Zenith.Set(0,0.5f,1.f,0);
	sunset->Sunlight.Set(1.f,0.2f,0,0);
	SetNumAltitudes(1);
	midnight->SunElevation=0;
	midnight->SunAzimuth=pi/2.f;
}

ColourSkyKeyframer::~ColourSkyKeyframer()
{
	for(size_t i=0;i<keyframes.size();i++)
	{
		delete keyframes[i];
	}
	keyframes.clear();
}

void ColourSkyKeyframer::StoreSky()
{
	old_dt.push(skyInterface->GetTime());
}

void ColourSkyKeyframer::ApplySkyKeyframe(int i)
{
	if(!keyframes.size())
		return;
	if(i>=(int)keyframes.size())
		i=i%(int)keyframes.size();
	while(i<0)
		i+=(int)keyframes.size();
	if(colourSky)
	{
		ColourKeyframe* K=dynamic_cast<ColourKeyframe*>(keyframes[i]);
		colourSky->SetHorizon(K->Horizon*K->BrightnessAdjustment);
		colourSky->SetZenith(K->Zenith*K->BrightnessAdjustment);
		colourSky->SetSunlight(K->Sunlight*K->BrightnessAdjustment);
		colourSky->SetHazeColour(K->HazeColour);
		colourSky->SetHaze(K->Haze);
		colourSky->SetHazeBaseHeightKm(K->HazeBaseKm);
		colourSky->SetHazeScaleHeightKm(K->HazeScaleKm);
	}
	SkyKeyframer::ApplySkyKeyframe(i);
}

void ColourSkyKeyframer::RestoreSky()
{
	skyInterface->SetTime(old_dt.top());
	old_dt.pop();
}

static float Lerp(float x,float a,float b)
{
	float r=x*b;
	r+=(1.f-x)*a;
	return r;
}

void ColourSkyKeyframer::Update()
{
	colourSky->SetTime(skyInterface->GetTime());
	SkyKeyframer::Update();
	ColourKeyframe K;
	InterpKeyframe(&K);
	colourSky->SetHorizon(K.Horizon*K.BrightnessAdjustment);
	colourSky->SetZenith(K.Zenith*K.BrightnessAdjustment);
	colourSky->SetSunlight(K.Sunlight*K.BrightnessAdjustment);
	colourSky->SetHazeColour(K.HazeColour);
	colourSky->SetHaze(K.Haze);
	colourSky->SetHazeBaseHeightKm(K.HazeBaseKm);
	colourSky->SetHazeScaleHeightKm(K.HazeScaleKm);
}

void ColourSkyKeyframer::InterpKeyframe(ColourSkyKeyframer::ColourKeyframe *K)
{
	if(!keyframes.size())
		return;
	int i=current_keyframe;
	if(i>=(int)keyframes.size())
		i=i%(int)keyframes.size();
	while(i<0&&keyframes.size())
		i+=(int)keyframes.size();
	int next_keyframe=(i+1)%(int)keyframes.size();
	ColourKeyframe *K1=dynamic_cast<ColourKeyframe *>(keyframes[i]);
	ColourKeyframe *K2=dynamic_cast<ColourKeyframe *>(keyframes[next_keyframe]);
	K->Horizon=lerp(interp,K1->Horizon,K2->Horizon);
	K->Zenith=lerp(interp,K1->Zenith,K2->Zenith);
	K->Sunlight=lerp(interp,K1->Sunlight,K2->Sunlight);
	K->HazeColour=lerp(interp,K1->HazeColour,K2->HazeColour);
	K->Haze=Lerp(interp,K1->Haze,K2->Haze);
	K->HazeBaseKm=Lerp(interp,K1->HazeBaseKm,K2->HazeBaseKm);
	K->HazeScaleKm=Lerp(interp,K1->HazeScaleKm,K2->HazeScaleKm);
}

float4 ColourSkyKeyframer::GetLocalIrradiance(float altitude_km) const
{
	static float4 irr;
	irr=colourSky->GetLocalIrradiance(altitude_km);
	return irr;
}

const float4 &ColourSkyKeyframer::GetAmbientLight(float ) const
{
	static float4 amb;
	amb=colourSky->GetZenith();
	return amb;
}

std::ostream &ColourSkyKeyframer::ColourKeyframe::Save(std::ostream &os) const
{
	SkyKeyframer::SkyKeyframe::Save(os);
	os.write((const char*)&KEYFRAME_MAGIC,sizeof(KEYFRAME_MAGIC));
	os.write((const char*)&Horizon,sizeof(Horizon));
	os.write((const char*)&Zenith,sizeof(Zenith));
	os.write((const char*)&Sunlight,sizeof(Sunlight));
	os.write((const char*)&HazeColour,sizeof(HazeColour));

	return os;
}

std::istream &ColourSkyKeyframer::ColourKeyframe::Load(std::istream &is)
{
	SkyKeyframer::SkyKeyframe::Load(is);
	unsigned magic;
	is.read((char*)&magic,sizeof(magic));
	if(magic!=KEYFRAME_MAGIC)
	{
#ifndef XENON
		std::cerr<<"ColourSkyKeyframer::ColourKeyframe::Load - not found magic number in file - possibly out of date file."<<std::endl;
#endif
		return is;
	}
	is.read((char*)&Horizon,sizeof(Horizon));
	is.read((char*)&Zenith,sizeof(Zenith));
	is.read((char*)&Sunlight,sizeof(Sunlight));
	is.read((char*)&HazeColour,sizeof(HazeColour));

	return is;
}


BaseKeyframe *ColourSkyKeyframer::InsertKeyframe(float t)
{
	int i=GetKeyframeAtTime(t);
	// This is the keyframe to the left of t. We want to INSERT at the next point so:
	i++;
	if(i>=(int)keyframes.size())
	{
		i=(int)keyframes.size();
		keyframes.push_back(new ColourKeyframe());
	}
	else
	{
		if(i<0)
			i=0;
		keyframes.insert(keyframes.begin()+i,new ColourKeyframe());
	}
	keyframes[i]->time=t;
	// Does this new keyframe affect us?
	if(current_keyframe>i)
		current_keyframe++;
	if(i>=current_keyframe&&i<=current_keyframe+2)
	{
		interp=GetInterp(current_keyframe,skyInterface->GetTime());
		InitFadeTextures();
	}
	InterpKeyframe((ColourKeyframe*)(keyframes[i]));
	return keyframes[i];
}

void ColourSkyKeyframer::GenColours(simul::sky::ColourSkyKeyframer::ColourKeyframe *K)
{
	if(!K)
		return;
	static simul::base::SmartPtr<simul::sky::SkyNode> trueSky;
	if(!trueSky.get())
		trueSky=new simul::sky::SkyNode();
	static float alt_km=0.f;
	static float hh=1.f;
	trueSky->SetHaze(hh);
	trueSky->SetMieEccentricity(0.9f);
	trueSky->SetTime(K->time);
	simul::sky::float4 dir(1.f,0,0,0);
	K->Horizon=trueSky->GetSkyColour(dir,alt_km);
	simul::sky::float4 up(0,0,1.f,0);
	K->Zenith=trueSky->GetSkyColour(up,alt_km);
	K->Sunlight=trueSky->GetLocalIrradiance(alt_km)/25.f;
}

void ColourSkyKeyframer::GenColours()
{
	for(size_t k=0;k<keyframes.size();k++)
	{
		ColourKeyframe *K=dynamic_cast<ColourKeyframe *>(keyframes[k]);
		GenColours(K);
	}
	SetRecalculate();
}

float4 ColourSkyKeyframer::GetKeyframeHorizonColour(int k)
{
	if(k>=(int)keyframes.size())
		k=k%(int)keyframes.size();
	while(k<0)
		k+=(int)keyframes.size();
	if(k>=(int)keyframes.size())
		return float4(0,0,0,0);
	ColourKeyframe *K=dynamic_cast<ColourKeyframe *>(keyframes[k]);
	return K->Horizon;
}

float4 ColourSkyKeyframer::GetKeyframeZenithColour(int k)
{
	if(k>=(int)keyframes.size())
		k=k%(int)keyframes.size();
	while(k<0)
		k+=(int)keyframes.size();
	if(k>=(int)keyframes.size())
		return float4(0,0,0,0);
	ColourKeyframe *K=dynamic_cast<ColourKeyframe *>(keyframes[k]);
	return K->Zenith;
}

float4 ColourSkyKeyframer::GetKeyframeSunlightColour(int k)
{
	if(k>=(int)keyframes.size())
		k=k%(int)keyframes.size();
	while(k<0)
		k+=(int)keyframes.size();
	if(k>=(int)keyframes.size())
		return float4(0,0,0,0);
	ColourKeyframe *K=dynamic_cast<ColourKeyframe *>(keyframes[k]);
	return K->Sunlight;
}


bool ColourSkyKeyframer::ColourKeyframe::HasValue(const char *name) const
{
	if(_stricmp(name,"Horizon")==0)
		return true;
	else if(_stricmp(name,"Zenith")==0)
		return true;
	else if(_stricmp(name,"Sunlight")==0)
		return true;
	else if(_stricmp(name,"HazeColour")==0)
		return true;
	return SkyKeyframe::HasValue(name);
}

float4 ColourSkyKeyframer::ColourKeyframe::GetColour(const char *name) const
{
	if(_stricmp(name,"Horizon")==0)
		return Horizon;
	else if(_stricmp(name,"Zenith")==0)
		return Zenith;
	else if(_stricmp(name,"Sunlight")==0)
		return Sunlight;
	else if(_stricmp(name,"HazeColour")==0)
		return HazeColour;
	return SkyKeyframe::GetColour(name);
}

void ColourSkyKeyframer::ColourKeyframe::SetColour(const char *name,float4 val)
{
	if(_stricmp(name,"Horizon")==0)
		Horizon=val;
	else if(_stricmp(name,"Zenith")==0)
		Zenith=val;
	else if(_stricmp(name,"Sunlight")==0)
		Sunlight=val;
	else if(_stricmp(name,"HazeColour")==0)
		HazeColour=val;
	SkyKeyframe::SetColour(name,val);
}

unsigned ColourSkyKeyframer::ColourKeyframe::GetPropertiesChecksum() const
{
	unsigned new_checksum=SkyKeyframer::SkyKeyframe::GetPropertiesChecksum();
	new_checksum+=*reinterpret_cast<const unsigned*>(&Horizon.x			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Horizon.y			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Horizon.z			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Horizon.w			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Zenith.x			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Zenith.y			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Zenith.z			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Zenith.w			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Sunlight.x		);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Sunlight.y		);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Sunlight.z		);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Sunlight.w		);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Sunlight.x		);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Sunlight.y		);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Sunlight.z		);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Sunlight.w		);

	return new_checksum;
}
