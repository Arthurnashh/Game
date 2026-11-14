#include "precomp.h"

// Copyright (c) 2007-2011 Simul Software Ltd
// All Rights Reserved.
//
// This source code is supplied under the terms of a license agreement or
// nondisclosure agreement with Simul Software Ltd and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.

#include "BasePrecipitationRenderer.h"

#include "Simul/Math/Pi.h"
using namespace simul;
using namespace clouds;

BasePrecipitationRenderer::BasePrecipitationRenderer()
	:WindEffectForSnow(.5f)
	,WindEffectForRain(.4f)
	,radius(50.f)
	,height(50.f)
{
	New();
}

BasePrecipitationRenderer::~BasePrecipitationRenderer()
{
}

void BasePrecipitationRenderer::TextureRepeatChanged()
{
}

void BasePrecipitationRenderer::SetLightColour(const float c[4])
{
	static float cc=0.07f;
	light_colour[0]=cc*c[0];
	light_colour[1]=cc*c[1];
	light_colour[2]=cc*c[2];
	light_colour[3]=1.f;
}


void BasePrecipitationRenderer::Update(float )
{
	rain_speed=RainSpeedMin+(RainSpeedMax-RainSpeedMin)*rain_intensity;
	offs+=rain_speed*0.01f;
}

static const unsigned MAGIC=0x1039274;
std::ostream &BasePrecipitationRenderer::Save(std::ostream &os) const
{
	os.write((const char*)&MAGIC,sizeof(MAGIC));
	std::streamoff tell_1=os.tellp();
  // LUKE - sizeof(std::streamoff) varies in VS2005 and VS2010 - so if SkySequencer is compiled in VS2005 and we read values in VS2010, it will corrupt values
  // so we use unsigned instead
  unsigned chunk_size=0;
	os.write((const char*)&chunk_size,sizeof(chunk_size));

	RecursiveStreamOut(os,false);

	std::streamoff tell_2=os.tellp();
	chunk_size=tell_2-tell_1;
	os.seekp(tell_1);
	os.write((const char*)&chunk_size,sizeof(chunk_size));
	os.seekp(tell_2);
	return os;
}

std::istream &BasePrecipitationRenderer::Load(std::istream &is)
{
	if(!is.good())
		return is;
	unsigned magic;
	is.read((char*)&magic,sizeof(magic));
	if(magic!=MAGIC)
		return is;
	std::streamoff tell_1=is.tellg();
  // LUKE - sizeof(std::streamoff) varies in VS2005 and VS2010 - so if SkySequencer is compiled in VS2005 and we read values in VS2010, it will corrupt values
  // so we use unsigned instead
  unsigned chunk_size=0;
	is.read((char*)&chunk_size,sizeof(chunk_size));
	std::streamoff tell_2_required=tell_1+chunk_size;

	RecursiveStreamIn(is,false);

	std::streamoff tell_2=is.tellg();
	if(tell_2!=tell_2_required)
		is.seekg(tell_2_required);
	return is;
}

void BasePrecipitationRenderer::New()
{
	DefaultRainSpeedMin=1.6f;
	DefaultRainSpeedMax=10.f;
	DefaultSnowSpeedMin=0.6f;
	DefaultSnowSpeedMax=3.0f;
	radius=50.f;
	height=50.f;
	rain_intensity=0.f;
	rain_speed=1.6f;
	RainSpeedMin=1.6f;
	RainSpeedMax=10.f;
	WindEffect=.2f;
	Aspect=20.f;
	Waver=0.f;
	TextureRepeat=RainTextureRepeat=14;
	offs=0;
	ApplyDefaultRainSettings();
}

void BasePrecipitationRenderer::ApplyDefaultRainSettings()
{
	radius=50.f;
	height=100.f;
	RainSpeedMin=DefaultRainSpeedMin;
	RainSpeedMax=DefaultRainSpeedMax;
	WindEffect=WindEffectForRain;
	Waver=0.2f;
	SetAspect(20.f);
	SetTextureRepeat(RainTextureRepeat);
}

void BasePrecipitationRenderer::ApplyDefaultSnowSettings()
{
	radius=50.f;
	height=50.f;
	RainSpeedMin=DefaultSnowSpeedMin;
	RainSpeedMax=DefaultSnowSpeedMax;
	WindEffect=WindEffectForSnow;
	Waver=1.f;
	SetAspect(0.6f);
	SetTextureRepeat(2);
}