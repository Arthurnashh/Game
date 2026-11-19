#include <algorithm>
#include "precomp.h"

#include "Simul/Clouds/CloudKeyframer.h"
#include "Simul/Clouds/TextureGenerator.h"
#include "Simul/Clouds/CloudInterface.h"
#include "Simul/Clouds/LightningRenderInterface.h"
#include "Simul/Graph/StandardNodes/ShowProgressInterface.h"
#include "Simul/Graph/Meta/Resource.h"
#include "Simul/Sky/SkyInterface.h"
#include "Simul/Math/Pi.h"
#include "Simul/Math/Matrix4x4.h"
#include <math.h>
#include <float.h>
#include <assert.h>
#ifndef _MSC_VER
#include <stdio.h>
#include <strings.h>
#define _stricmp strcasecmp
#endif


using namespace simul;
using namespace clouds;

static void InitKeyframeFromClouds(CloudKeyframer::Keyframe *K,CloudInterface *cloudInterface)
{
	K->extinction			=cloudInterface->GetExtinction();
	K->cloudiness			=cloudInterface->GetHumidity();
	K->cloud_base_km		=cloudInterface->GetCloudBaseZ()*0.001f;
	K->cloud_height_km		=cloudInterface->GetCloudHeight()*0.001f;
	K->direct_light			=cloudInterface->GetLightResponse();
	K->indirect_light		=cloudInterface->GetSecondaryLightResponse();
	K->ambient_light		=cloudInterface->GetAmbientLightResponse();
	K->octaves				=cloudInterface->GetNoiseOctaves();
	K->persistence			=cloudInterface->GetNoisePersistence();
	K->fractal_amplitude	=cloudInterface->GetFractalAmplitude();
	K->fractal_wavelength	=cloudInterface->GetFractalWavelength();
}

const bool big_endian=false;
#if 1
// OPENGL style
static unsigned bits4[4]={	(unsigned)15<<12,(unsigned)15<<8,(unsigned)15<<4,(unsigned)15<<0};
#else
static unsigned bits4[4]={	(unsigned)0x0F00,
							(unsigned)0x000F,
							(unsigned)0x00F0,
							(unsigned)0xF000};
#endif
static unsigned bits8[4]={	(unsigned)0x00FF0000,
							(unsigned)0x000000FF,
							(unsigned)0x0000FF00,
							(unsigned)0xFF000000};

static float lerp(float f,float l,float h)
{
	return l+((h-l)*f);
}

CloudKeyframer::CloudKeyframer(CloudInterface *ci,LightningRenderInterface *lri,bool make2d)
								:TimeFactor(24.f*60.f*60.f)	// 1 game-hour time-steps
								,cloudInterface(ci)
								,lightningRenderInterface(lri)
								,skyInterface(NULL)
								,renderCallback(NULL)
								,interp(0.f)
								,fill_texture_size(0)
								,texel_index(0)
								,last_time(0.f)
								,Use16Bit(false)
								,OpenGL(false)
								,WindSpeed(0.f)
								,WindHeadingDegrees(0.f)
								,Make2DTextures(make2d)
								,UserKeyframes(true)
								,sent_width(0)
								,sent_length(0)
								,sent_height(0)
								,FillTexturesAsBlocks(false)
								,OverrideWind(false)
								,PrecalculatedGamma(1.f)
								,CacheClouds(false)
{
	InitializePropertiesDefinition();
	SetBits(bits8[0],bits8[1],bits8[2],bits8[3],(unsigned)4,big_endian);

	for(int i=0;i<4;i++)
	{
		illumination_texel_index[i]=0;
		illumination_texture[i]=NULL;
	}
	fill_texture[0]=NULL;
	fill_texture[1]=NULL;
	fill_texture[2]=NULL;
	fill_texture_16bit[0]=NULL;
	fill_texture_16bit[1]=NULL;
	fill_texture_16bit[2]=NULL;
	texture_complete[0]=false;
	texture_complete[1]=false;
	texture_complete[2]=false;
	SetUniformKeyframes(8);
}

void CloudKeyframer::InitKeyframesFromClouds()
{
	for(int i=0;i<(int)keyframes.size();i++)
	{
		InitKeyframeFromClouds(dynamic_cast<Keyframe *>(keyframes[i]),cloudInterface);
	}
}

void CloudKeyframer::SetUniformKeyframes(int StepsPerDay,float range)
{
	ClearKeyframes();
	LoopRange=range;
	float t=0.f;
	float dt=range/(float)StepsPerDay;
	for(int i=0;i<StepsPerDay;i++)
	{
		Keyframe *K=new Keyframe();
		if(cloudInterface)
			InitKeyframeFromClouds(K,cloudInterface);
		keyframes.push_back(K);

		K->time=t;
		K->wind_speed=(cloudInterface&&cloudInterface->GetWrap())?5.f:0.f;
		K->cloudiness=0.5f;
		t+=dt;
	}
	// Sort and initialize wind offsets:
	SortKeyframes();
}

CloudKeyframer::~CloudKeyframer()
{
	for(int i=0;i<4;i++)
		delete [] illumination_texture[i];
	delete [] fill_texture[0];
	delete [] fill_texture[1];
	delete [] fill_texture[2];
	delete [] fill_texture_16bit[0];
	delete [] fill_texture_16bit[1];
	delete [] fill_texture_16bit[2];
}

void CloudKeyframer::ApplyBits()
{
	simul::clouds::TextureGenerator::SetBits(bits.density,bits.brightness,bits.secondary,bits.ambient,bits.Bytes_per_texel,bits.bigendian);
}

/*	0x00FF0000 x
	0x000000FF z
	0x0000FF00 y
	0xFF000000 w*/
void CloudKeyframer::SetBits(unsigned density,unsigned brightness,unsigned secondary,unsigned ambient,unsigned Bytes_per_texel,bool bigendian)
{
	bits.density=density;
	bits.brightness=brightness;
	bits.secondary=secondary;
	bits.ambient=ambient;
	bits.Bytes_per_texel=Bytes_per_texel;
	bits.bigendian=bigendian;
}

void CloudKeyframer::SetBits(GridVariable x,GridVariable y,GridVariable z,GridVariable w)
{
	unsigned bitx=0x00FF0000;
	unsigned bity=0x0000FF00;
	unsigned bitz=0x000000FF;
	unsigned bitw=0xFF000000;
	if(OpenGL)
	{
		bitx=0xFF000000;
		bity=0x00FF0000;
		bitz=0x0000FF00;
		bitw=0x000000FF;
	}
	switch(x)
	{
		case DENSITY:
			bits.density=bitx;
			break;
		case BRIGHTNESS:
			bits.brightness=bitx;
			break;
		case SECONDARY:
			bits.secondary=bitx;
			break;
		case AMBIENT:
			bits.ambient=bitx;
			break;
	}
	switch(y)
	{
		case DENSITY:
			bits.density=bity;
			break;
		case BRIGHTNESS:
			bits.brightness=bity;
			break;
		case SECONDARY:
			bits.secondary=bity;
			break;
		case AMBIENT:
			bits.ambient=bity;
			break;
	}
	switch(z)
	{
		case DENSITY:
			bits.density=bitz;
			break;
		case BRIGHTNESS:
			bits.brightness=bitz;
			break;
		case SECONDARY:
			bits.secondary=bitz;
			break;
		case AMBIENT:
			bits.ambient=bitz;
			break;
	}
	switch(w)
	{
		case DENSITY:
			bits.density=bitw;
			break;
		case BRIGHTNESS:
			bits.brightness=bitw;
			break;
		case SECONDARY:
			bits.secondary=bitw;
			break;
		case AMBIENT:
			bits.ambient=bitw;
			break;
	}
}

void CloudKeyframer::SetLoopWind()
{
	float w=cloudInterface->GetCloudWidth();
	float required_speed=w/this->GetLoopRange();
	if(!keyframes.size())
		return;
	Keyframe *K0=dynamic_cast<Keyframe *>(keyframes[0]);
//	int w=(int)(K0->wind_direction
	for(int i=0;i<(int)keyframes.size();i++)
	{
		Keyframe *K=dynamic_cast<Keyframe *>(keyframes[i]);
		K->wind_direction=K0->wind_direction;
		K->wind_speed=required_speed;
	}
	cloudInterface->SetNoisePeriod(0.f);
	cloudInterface->SetWindOffset(math::Vector3(0,0,0));
	TimeFactor=1.f;
	CalcOffsets();
}

void CloudKeyframer::SetRenderCallback(CloudRenderCallback *cb)
{
	renderCallback=cb;
	InitCallback();
}

void CloudKeyframer::SetCloudInterface(CloudInterface *ci)
{
	cloudInterface=ci;
	sent_width=sent_height=sent_length=0;
}

CloudInterface *CloudKeyframer::GetCloudInterface()
{
	return cloudInterface;
}

void CloudKeyframer::SetSkyInterface(simul::sky::BaseSkyInterface *si)
{
	skyInterface=si;
	sent_width=sent_height=sent_length=0;
}


void CloudKeyframer::TimeFactorChanged()
{
	RecalculateOffsets();
}

void CloudKeyframer::CalculateOvercastFactor()
{
	const Keyframe *K2=dynamic_cast<const Keyframe *>(GetKeyframe(current_keyframe+1));
	// NOW calculate how overcast the sky is, taking into account whether we are in or above the clouds:
	float bas,sca;
	if(K2)
		GetOvercastData(K2->time,overcast_factor_2,bas,sca);
}

void CloudKeyframer::GetOvercastData(float time,float &overcast,float &overcast_base_km,float &overcast_scale_km)
{
	int k=GetKeyframeAtTime(time);
	const Keyframe *K1=dynamic_cast<const Keyframe *>(GetKeyframe(k));
	const Keyframe *K2=dynamic_cast<const Keyframe *>(GetKeyframe(k+1));
	float x=0.f;
	float extinction=0.f;
	if(UserKeyframes&&K1&&K2)
	{
		float intrp=GetInterp(k,time);
		x=lerp(intrp,K1->cloudiness,K2->cloudiness);
		overcast_base_km		=lerp(intrp,K1->cloud_base_km,K2->cloud_base_km);
		overcast_scale_km		=lerp(intrp,K1->cloud_height_km,K2->cloud_height_km);
		extinction				=PrecalculatedGamma*lerp(intrp,K1->extinction,K2->extinction);
	}
	else
	{
		overcast_base_km	=cloudInterface->GetCloudBaseZ();
		overcast_scale_km	=cloudInterface->GetCloudHeight();
		extinction			=PrecalculatedGamma*cloudInterface->GetExtinction();
	}
	float g					=(float)cloudInterface->GetGridHeight();
	float h					=overcast_scale_km*(g-2.f)/g;
	x						*=extinction*h*OvercastEffect/(4.f*pi);
	if(!cloudInterface->GetWrap())
	{
		float dist_from_centre=150000.f;
		x*=exp(-dist_from_centre/cloudInterface->GetCloudWidth());
	}
	overcast_base_km		+=overcast_scale_km*.25f;
	overcast_scale_km		*=0.75f;
	overcast				=1.f-exp(-x);
}


void CloudKeyframer::Init()
{
	last_time=0.f;
	if(!cloudInterface)
		return;
	unsigned	w=cloudInterface->GetGridWidth(),
				l=cloudInterface->GetGridLength(),
				d=(Make2DTextures?1:cloudInterface->GetGridHeight());

	unsigned total_texels=w*l*d;
	if(total_texels>fill_texture_size)
	{
		fill_texture_size=total_texels;
		for(int i=0;i<3;i++)
		{
			if(Use16Bit)
			{
				delete [] fill_texture_16bit[i];
				fill_texture_16bit[i]=new unsigned short[fill_texture_size];
			}
			else
			{
				delete [] fill_texture[i];
				fill_texture[i]=new unsigned[fill_texture_size];
			}
			texture_complete[i]=false;
		}
		for(int i=0;i<4;i++)
			illumination_texture[i]=new unsigned char[fill_texture_size/8];
	}
	if(!skyInterface)
		return;
	current_keyframe=GetKeyframeAtTime(skyInterface->GetTime());
	if(!renderCallback)
		return;
	InitCallback();

	CalculateOvercastFactor();
	overcast_factor_1=overcast_factor_2;
	if(overcast_factor_2>0.6f)
		precipitation_2=std::min(1.f,(overcast_factor_2-0.6f)/0.2f);
	else
		precipitation_2=0;
	precipitation_1=precipitation_2;
	InitLightDirection(2);
}

void CloudKeyframer::ApplyCloudKeyframe(const Keyframe *K)
{
	if(!K)
		return;
	cloudInterface->SetHumidity(K->cloudiness);
	cloudInterface->SetNoiseOctaves(K->octaves);
	cloudInterface->SetNoisePersistence(K->persistence);
	cloudInterface->SetExtinction(PrecalculatedGamma*K->extinction);
	cloudInterface->SetDiffusivity(K->diffusivity);

	cloudInterface->SetCloudBaseZ(1000.f*K->cloud_base_km);
	cloudInterface->SetCloudHeight(1000.f*K->cloud_height_km);
	cloudInterface->SetLightResponse(K->direct_light);
	cloudInterface->SetSecondaryLightResponse(K->indirect_light);
	cloudInterface->SetAmbientLightResponse(K->ambient_light);

	cloudInterface->SetFractalAmplitude(K->fractal_amplitude);
	cloudInterface->SetFractalWavelength(K->fractal_wavelength);
	cloudInterface->SetDiffusivity(K->diffusivity);
}

void CloudKeyframer::CacheKeyframes() 
{
	for(size_t k=0;k<keyframes.size();k++)
	{
		CacheKeyframe(k);
	}
}

void CloudKeyframer::InvalidateKeyframeCache(int k)
{
	Keyframe *K=dynamic_cast<Keyframe *>(GetKeyframe(k));
	if(!K||!K->cache_built)
		return;
	delete [] K->cache;
	K->cache=NULL;
	K->cache_built=false;
}

void CloudKeyframer::CacheKeyframe(int k)
{
	if(!UserKeyframes)
		return;
	if(!CacheClouds)
		return;
	unsigned	w=cloudInterface->GetGridWidth(),
				l=cloudInterface->GetGridLength(),
				d=(Make2DTextures?1:cloudInterface->GetGridHeight());
	unsigned total_texels=w*l*d;
	float old_time=0;
	if(skyInterface)
		old_time=skyInterface->GetTime();
	Keyframe *K=dynamic_cast<Keyframe *>(GetKeyframe(k));
	if(!K)
		return;
	if(K->cache_built)
		return;
	ApplyCloudKeyframe(K);
	float dt=GetKeyframeTime(k);
	skyInterface->SetTime(dt);
	cloudInterface->SetTime(dt);
	cloudInterface->Generate();
	if(skyInterface)
	{
		simul::math::Vector3 light_dir((const float*)&(skyInterface->GetDirectionToLight()));
		cloudInterface->SetRelight();
		cloudInterface->ReLight(next_light_direction);
	}
	delete [] K->cache;
	simul::graph::standardnodes::ShowProgressInterface *progress=FindProgressCallback();
	simul::graph::meta::Node *progress_sender=dynamic_cast<simul::graph::meta::Node *>(cloudInterface);
	if(progress)
		progress->SetText(progress_sender,"CloudKeyframer::InitTexture");
	if(Use16Bit)
	{
		K->cache_size_bytes=total_texels*sizeof(short);
		K->cache=new unsigned char[K->cache_size_bytes];
		SetBits(bits4[0],bits4[1],bits4[2],bits4[3],2,big_endian);
		ApplyBits();
		if(Make2DTextures)
			simul::clouds::TextureGenerator::Partial2DCloudTextureFill(cloudInterface,progress,0,total_texels,(unsigned char *)(K->cache));
		else
			simul::clouds::TextureGenerator::PartialCloudTextureFill(cloudInterface,progress,0,total_texels,(unsigned char *)K->cache,false);
	}
	else
	{
		K->cache_size_bytes=total_texels*sizeof(unsigned);
		K->cache=new unsigned char[K->cache_size_bytes];
		ApplyBits();
		if(Make2DTextures)
			simul::clouds::TextureGenerator::Partial2DCloudTextureFill(cloudInterface,progress,0,total_texels,(unsigned char *)K->cache);
		else
			simul::clouds::TextureGenerator::PartialCloudTextureFill(cloudInterface,progress,0,total_texels,(unsigned char *)K->cache,false);
	}
	if(progress)
		progress->SetProgressComplete(progress_sender);
	K->cache_built=true;
	if(skyInterface)
	{
		skyInterface->SetTime(old_time);
		cloudInterface->SetTime(old_time);
	}
}

void CloudKeyframer::CopyToCache(int i,int k)
{
	Keyframe *K=dynamic_cast<Keyframe *>(GetKeyframe(k));
	if(!K)
		return;
	if(K->cache_built)
		return;
	delete [] K->cache;
	unsigned	w=cloudInterface->GetGridWidth(),
				l=cloudInterface->GetGridLength(),
				d=(Make2DTextures?1:cloudInterface->GetGridHeight());
	unsigned total_texels=w*l*d;
	K->cache_size_bytes=total_texels;
	if(Use16Bit)
		K->cache_size_bytes*=sizeof(short);
	else
		K->cache_size_bytes*=sizeof(unsigned);
	K->cache=new unsigned char[K->cache_size_bytes];
	if(K->cache_size_bytes)
	{
		if(Use16Bit)
			memcpy(K->cache,fill_texture_16bit[i],K->cache_size_bytes);
		else
			memcpy(K->cache,fill_texture[i],K->cache_size_bytes);
	}
	K->cache_built=true;
}
void CloudKeyframer::CopyFromCache(int i,int k)
{
	Keyframe *K=dynamic_cast<Keyframe *>(GetKeyframe(k));
	if(!K)
		return;
	if(!K->cache_built)
		return;
	if(K->cache_size_bytes)
	{
		if(Use16Bit)
			memcpy(fill_texture_16bit[i],K->cache,K->cache_size_bytes);
		else
			memcpy(fill_texture[i],K->cache,K->cache_size_bytes);
	}
}

float CloudKeyframer::InitLightDirection(int i)
{
	float t=0.f;
	float old_time=0;
	if(skyInterface)
		old_time=skyInterface->GetTime();
	t=(GetKeyframeTime(current_keyframe+i));
	if(skyInterface)
	{
		skyInterface->SetTime(t);
		if(i==0)
			Time0=skyInterface->GetTime();
		if(i==1)
			Time1=skyInterface->GetTime();
		if(i==2)
			Time2=skyInterface->GetTime();
		
		next_light_direction=simul::math::Vector3((const float*)&(skyInterface->GetDirectionToLight()));
		skyInterface->SetTime(old_time);
		cloudInterface->SetLightDirection(next_light_direction);
	}
	return t;
}

void CloudKeyframer::InitTexture(int i)
{
	unsigned	w=cloudInterface->GetGridWidth(),
				l=cloudInterface->GetGridLength(),
				d=(Make2DTextures?1:cloudInterface->GetGridHeight());
	unsigned total_texels=w*l*d;
	const Keyframe *K=dynamic_cast<const Keyframe *>(GetKeyframe(current_keyframe+i));
	if(UserKeyframes&&K&&K->cache_built&&IsKeyframeChecksumValid(K))
	{
		CopyFromCache(i,current_keyframe+i);
		texture_complete[i]=true;
	}
	else
	{
		if(K)
			ApplyCloudKeyframe(K);
		float dt=InitLightDirection(i);
		cloudInterface->SetTime(dt);
		cloudInterface->Generate();
		cloudInterface->SetRelight();
		simul::graph::standardnodes::ShowProgressInterface *progress=FindProgressCallback();
		simul::graph::meta::Node *progress_sender=dynamic_cast<simul::graph::meta::Node *>(cloudInterface);
		if(!renderCallback->CanPerformGPULighting())
		{
			if(progress)
				progress->SetText(progress_sender,"CloudKeyframer::InitTexture - relighting");
			cloudInterface->ReLight(next_light_direction);
			if(progress)
				progress->SetProgressComplete(progress_sender);
		}
		else
		{
			// send densities to texture:
			ApplyBits();
			simul::clouds::TextureGenerator::PartialCloudTextureFillDensityOnly(cloudInterface,NULL,0,total_texels,(unsigned char *)fill_texture[i],false);
			SendTextureToCallback(i);

			const unsigned *gridsizes=cloudInterface->GetLightGridsizes();
			const float *extinctions=cloudInterface->GetLightspaceExtinctions();
			float ambient_extinctions[]={extinctions[2],extinctions[3]};
			renderCallback->SetGPULightingParameters(cloudInterface->GetTransformLightspaceToCloudspaceMatrix(cloudInterface->GetLightAxis(),cloudInterface->GetLightVector()),gridsizes,extinctions);
			float *target_direct_grid=new float[gridsizes[0]*gridsizes[1]*gridsizes[2]];
			float *target_indirect_grid=new float[gridsizes[0]*gridsizes[1]*gridsizes[2]];
			float *target_ambient_grid=new float[gridsizes[0]*gridsizes[1]*gridsizes[2]];
			//! If GPU lighting is supported, fully relight the target grid as an array of float4 structures.
			renderCallback->PerformFullGPURelight(i,target_direct_grid,target_indirect_grid);
			unsigned ambient_gridsizes[]={cloudInterface->GetGridWidth(),cloudInterface->GetGridLength(),cloudInterface->GetGridHeight()};
			simul::math::Matrix4x4 ident;
			ident.Identity();
			ident(2,2)=-1.f;
			ident(2,3)=1.f;
			renderCallback->SetGPULightingParameters(ident,ambient_gridsizes,ambient_extinctions);
			renderCallback->PerformFullGPURelight(i,target_ambient_grid,NULL);
			cloudInterface->SetLightValues(target_direct_grid,target_indirect_grid,target_ambient_grid);

		}
		if(progress)
			progress->SetText(progress_sender,"CloudKeyframer::InitTexture - copying to texture");

		if(Use16Bit)
		{
			SetBits(bits4[0],bits4[1],bits4[2],bits4[3],2,big_endian);
			ApplyBits();
			if(Make2DTextures)
				simul::clouds::TextureGenerator::Partial2DCloudTextureFill(cloudInterface,progress,0,total_texels,(unsigned char *)(fill_texture_16bit[i]));
			else
				simul::clouds::TextureGenerator::PartialCloudTextureFill(cloudInterface,progress,0,total_texels,(unsigned char *)fill_texture_16bit[i],false);
		}
		else
		{
			ApplyBits();
			if(Make2DTextures)
				simul::clouds::TextureGenerator::Partial2DCloudTextureFill(cloudInterface,progress,0,total_texels,(unsigned char *)(fill_texture[i]));
			else
				simul::clouds::TextureGenerator::PartialCloudTextureFill(cloudInterface,progress,0,total_texels,(unsigned char *)fill_texture[i],false);
		}
		if(progress)
			progress->SetProgressComplete(progress_sender);
		if(CacheClouds)
			CopyToCache(i,current_keyframe+i);
	}
	SendTextureToCallback(i);
	texture_complete[i]=true;
}

void CloudKeyframer::RestartNextKeyframe()
{
	texel_index=0;
}

void CloudKeyframer::PrecalculatedGammaChanged()
{
}

void CloudKeyframer::WindSpeedChanged()
{
	simul::clouds::CloudKeyframer::Keyframe *K=GetNextModifiableKeyframe();
	if(!OverrideWind&&K)
	{
		K->wind_speed=WindSpeed;
	}
}

void CloudKeyframer::WindHeadingChanged()
{
	simul::clouds::CloudKeyframer::Keyframe *K=GetNextModifiableKeyframe();
	if(!OverrideWind&&K)
	{
		K->wind_direction=WindHeadingDegrees*pi/180.f;
	}
}

void CloudKeyframer::UpdateLightning(float dt)
{
	if(!lightningRenderInterface)
		return;
//	lightningRenderInterface->SetLightningCentreX(simul::math::Vector3(cam_pos.x,cam_pos.z,cam_pos.y));
	// RGBA bit-shift is 12,8,4,0
	simul::clouds::TextureGenerator::SetBits((unsigned)255<<0,(unsigned)255<<0,(unsigned)255<<0,(unsigned)255<<0,1,false);
	unsigned w=sent_width/2;
	unsigned l=sent_length/2;
	unsigned h=sent_height/2;
	simul::math::Vector3 X1,X2,DX;
	X1=lightningRenderInterface->GetIlluminationOrigin();
	DX=lightningRenderInterface->GetIlluminationScales();
	X2=X1+DX;
	unsigned max_texels=w*l*h;
	// lightning rate : strikes per second
	static float rr=.5f;
	float lightning_rate=rr;
	unsigned texels=(unsigned)(lightning_rate*dt/4.f*(float)max_texels);
	static unsigned tt=1024;
	texels=tt;
	float u[4];
	float t[4];
	static float thunder_volume=0.f;
	for(unsigned i=0;i<4;i++)
	{
		unsigned char *lightning_cloud_tex_data=illumination_texture[i];
		//thunder_volume[i]*=0.95f;
		if(lightningRenderInterface->IsSourceStarted(i))
		{
			u[i]=lightningRenderInterface->GetLightSourceProgress(i);
			if(u[i]<0.5f)
			{
				static float ff=0.05f;
				thunder_volume+=ff;
			}
			continue;
		}
		else
			u[i]=0;
		t[i]=(float)illumination_texel_index[i]/(float)(max_texels);
		unsigned &index=illumination_texel_index[i];
		if(index+texels>max_texels)
			texels=max_texels-index;
		simul::clouds::TextureGenerator::PartialMake3DLightningTexture(
			lightningRenderInterface,i,
			w,l,h,
			X1,X2,
			index,
			texels,
			lightning_cloud_tex_data,
			cloudInterface->GetUseTbb());
		FillIlluminationTexture(i,index,texels,&(lightning_cloud_tex_data[index]));
		index+=texels;
		if(index>=max_texels)
		{
			lightningRenderInterface->StartSource(i);
			index=0;
		}
	}
}

void CloudKeyframer::FillIlluminationTexture(int i,int texel_index,int num_texels,const unsigned char *uchar8_array)
{
	if(!renderCallback)
		return;

	if(!FillTexturesAsBlocks)
	{
		renderCallback->FillIlluminationSequentially(i,texel_index,num_texels,uchar8_array);
		return;
	}
}

void CloudKeyframer::FillCloudTexture(int i,int texel_index,int num_texels,const unsigned *uint32_array)
{
	if(!renderCallback)
		return;
	if(!num_texels)
		return;
	if(!FillTexturesAsBlocks)
	{
		renderCallback->FillCloudTextureSequentially(i,texel_index,num_texels,uint32_array);
		return;
	}
	int stride=sent_width;
	int sliceStride=sent_length*stride;

	int start_slice			=texel_index/sliceStride;
	int start_texel_in_slice=texel_index-start_slice*sliceStride;
	int start_row			=start_texel_in_slice/stride;
	int start_texel_in_row	=start_texel_in_slice-start_row*stride;

	int end_slice			=(texel_index+num_texels)/sliceStride;
	int end_texel_in_slice	=(texel_index+num_texels)-end_slice*sliceStride;
	int end_row				=end_texel_in_slice/stride;
	int end_texel_in_row	=end_texel_in_slice-end_row*stride;

	int first_row_length=stride-start_texel_in_row;
	int first_slice_rows=sent_length-start_row-1;

	if(first_row_length==stride)
	{
		first_row_length=0;
		first_slice_rows++;
		start_row--;
	}
	int num_slices=end_slice-start_slice-1;
	if(first_slice_rows==(int)sent_length)
	{
		first_slice_rows=0;
		num_slices++;
		start_slice--;
	}

	if(end_slice==start_slice)
	{
		if(end_row==start_row)
		{
			first_row_length=end_texel_in_row-start_texel_in_row;
			end_texel_in_row=0;
		}
		first_slice_rows=end_row-start_row-1;
	}
	const unsigned *uptr=uint32_array;
	int num_written=0;
	if(first_row_length>0)
	{
		renderCallback->FillCloudTextureBlock(i,start_texel_in_row,start_row,start_slice,first_row_length,1,1,uptr);
		uptr+=first_row_length;
		num_written+=first_row_length;
	}
	// The rest of the first slice:
	if(first_slice_rows>0)
	{
		renderCallback->FillCloudTextureBlock(i,0,start_row+1,start_slice,stride,first_slice_rows,1,uptr);
		uptr+=stride*first_slice_rows;
		num_written+=stride*first_slice_rows;
	}
	// The main block:
	if(num_slices>0)
	{
		renderCallback->FillCloudTextureBlock(i,0,0,start_slice+1,stride,sent_length,num_slices,uptr);
		uptr+=stride*sent_length*num_slices;
		num_written+=stride*sent_length*num_slices;
	}
	// the rows at the end:
	if(end_slice!=start_slice&&end_row>0)
	{
		renderCallback->FillCloudTextureBlock(i,0,0,end_slice,stride,end_row,1,uptr);
		uptr+=stride*end_row;
		num_written+=stride*end_row;
	}
	// and the final row:
	if(end_texel_in_row>0)
	{
		renderCallback->FillCloudTextureBlock(i,0,end_row,end_slice,end_texel_in_row,1,1,uptr);
		uptr+=end_texel_in_row;
		num_written+=end_texel_in_row;
	}
	if(num_texels!=num_written)
	{
		std::cerr<<"Trying to write too much cloud tex memory"<<std::endl;
		assert(num_texels==num_written);
	}
}

void CloudKeyframer::SendTextureToCallback(int i)
{
	if(!renderCallback)
		return;
	unsigned	w=cloudInterface->GetGridWidth(),
				l=cloudInterface->GetGridLength(),
				d=(Make2DTextures?1:cloudInterface->GetGridHeight());
	unsigned total_texels=w*l*d;
	if(Use16Bit)
	{
		SetBits(bits4[0],bits4[1],bits4[2],bits4[3],2,big_endian);
		ApplyBits();
		FillCloudTexture(i,0,total_texels,(const unsigned*)(fill_texture_16bit[i]));
	}
	else
	{
		ApplyBits();
		FillCloudTexture(i,0,total_texels,fill_texture[i]);
	}
}

void CloudKeyframer::InitCallback()
{
	unsigned	w=cloudInterface->GetGridWidth(),
				l=cloudInterface->GetGridLength(),
				d=(Make2DTextures?1:cloudInterface->GetGridHeight());
	if(renderCallback)
	{
		sent_width=w;
		sent_length=l;
		sent_height=d;
		renderCallback->SetCloudTextureSize(w,l,d);
		// Arbitrary:
		renderCallback->SetIlluminationGridSize(w/2,l/2,d/2);
	}
	unsigned total_texels=w*l*d;
	if(total_texels>fill_texture_size)
	{
		Init();
		return;
	}
	if(!total_texels)
		return;
	// Only need to init the first TWO textures, the third will be built along the way...
	for(int i=0;i<2;i++)
	{
		if(!texture_complete[i])
			InitTexture(i);
		else if(renderCallback)
			SendTextureToCallback(i);
	}
}

void CloudKeyframer::RecalculateOffsets()
{
	if(offset_checksum!=GetOffsetChecksum())
		CalcOffsets();
}

unsigned CloudKeyframer::GetGlobalChecksum() const
{
	unsigned checksum=cloudInterface->GetWrap()?1:0;
	checksum+=cloudInterface->GetNoiseResolution();
	float val=cloudInterface->GetNoisePeriod();
	checksum+=*reinterpret_cast<const unsigned*>(&val);
	return checksum;
}

void CloudKeyframer::Update(float current_time)
{
	if(!cloudInterface)
		return;
	// Any change in texture size?
	unsigned	w=cloudInterface->GetGridWidth(),
				l=cloudInterface->GetGridLength(),
				h=(Make2DTextures?1:cloudInterface->GetGridHeight());
	if(sent_width!=w||sent_length!=l||sent_height!=h)
		Init();
	else if(ChecksumsFailed(current_time))
	{
		if(!texture_complete[0])
			InvalidateKeyframeCache(current_keyframe);
		if(!texture_complete[1])
			InvalidateKeyframeCache(current_keyframe+1);
		if(!texture_complete[2])
			InvalidateKeyframeCache(current_keyframe+2);
		RecalculateOffsets();
		Init();
	}
	interp=GetInterp(current_keyframe,current_time);

	float old_time=cloudInterface->GetTime();
	float next_time=old_time;
	
	{
		next_time=GetKeyframeTime(current_keyframe+2);
		ApplyCloudKeyframe((const Keyframe*)GetKeyframe(current_keyframe+2));
	}
	cloudInterface->SetTime(next_time);
	if(interp>=0&&interp<=3.f&&!texture_complete[2]&&!cloudInterface->UpdatePartial(interp/(0.45f)))
		UpdateCloudTexture();
	cloudInterface->SetTime(old_time);
	if(interp>=1.f||interp<0.f)
	{
		if(texture_complete[2]||interp>=3.f||interp<0.f)
		{
			if(interp>=1.f)
			{
				IncrementKeyframe();
				interp=GetInterp(current_keyframe,current_time);

				std::swap(Time0,Time1);
				std::swap(Time1,Time2);
				CycleTexturesForward();
			}
			bool relight=false;
			while(interp>=1.f)
			{
				IncrementKeyframe();
				CycleTexturesForward();
				interp=GetInterp(current_keyframe,current_time);
				relight=true;
			}
			if(interp<0.f)
			{
				DecrementKeyframe();
				CycleTexturesBackward();
				interp=GetInterp(current_keyframe,current_time);
				std::swap(Time1,Time2);
				std::swap(Time0,Time1);
			
				relight=true;
			}
			while(interp<0.f)
			{
				DecrementKeyframe();
				interp=GetInterp(current_keyframe,current_time);
				CycleTexturesBackward();
				relight=true;
			}
			texel_index=0;

			if(relight)
			{
				InitCallback();
			}
			else
			{
				// overcast factor for next keyframe:
				overcast_factor_1=overcast_factor_2;
				precipitation_1=precipitation_2;

				if(UserKeyframes)
				{
					const Keyframe *K=dynamic_cast<const Keyframe *>(GetKeyframe(current_keyframe+2));
					ApplyCloudKeyframe(K);
				}
				CalculateOvercastFactor();

				if(overcast_factor_2>0.6f)
					precipitation_2=std::min(1.f,(overcast_factor_2-0.6f)/0.2f);
				else
					precipitation_2=0;
				// Get the sun direction for the next time step:
				InitLightDirection(2);
				cloudInterface->SetLightDirection(next_light_direction);
			}
			cloudInterface->StartMarching();
			//texture_complete[2]=false;
		}
		else
		{
			interp=1.f;
		}
	}
	float time_step_seconds=0.f;
	if(last_time!=0.f)
	{
		time_step_seconds=current_time-last_time;
		//if(LinkKeyframeTimeAndDaytime)
			time_step_seconds*=TimeFactor;
	}
	UpdateLightning(time_step_seconds);
	if(UserKeyframes)
	{
		InterpolatedKeyframe.time=current_time;
		InterpKeyframe(&InterpolatedKeyframe);
		if(keyframes.size()>=2)
		{
			if(!OverrideWind)
			{
				WindSpeed=InterpolatedKeyframe.wind_speed;
				WindHeadingDegrees=InterpolatedKeyframe.wind_direction*180.f/pi;
			}
			ApplyCloudKeyframe(&InterpolatedKeyframe);
			// offset = integral of vdt
			// v=v0+t/DT *(v1-v0)
			// offset= v0t=t^2/2DT
			simul::math::Vector3 wind_offset=cloudInterface->GetWindOffset();
			if(!OverrideWind)
			{
				wind_offset.x=InterpolatedKeyframe.GetOffsetX();
				wind_offset.y=InterpolatedKeyframe.GetOffsetY();
			}
			else
			{
				float WindHeading=WindHeadingDegrees*pi/180.f;
				simul::math::Vector3 WindVelocity(WindSpeed*sin(WindHeading),WindSpeed*cos(WindHeading),0.f);
				wind_offset+=time_step_seconds*WindVelocity;
			}
			cloudInterface->SetWindOffset(wind_offset);
			precipitation=InterpolatedKeyframe.precipitation;
			float overcast_base_km,overcast_scale_km;
			GetOvercastData(current_time,overcast_factor,overcast_base_km,overcast_scale_km);

			if(lightningRenderInterface)
				lightningRenderInterface->SetLightningEnabled(InterpolatedKeyframe.lightning>0);
		}
	}
	else
	{
		simul::math::Vector3 wind_offset=cloudInterface->GetWindOffset();
		cloudInterface->SetWindOffset(wind_offset);
		precipitation	=lerp(interp,precipitation_1,precipitation_2);
		overcast_factor	=lerp(interp,overcast_factor_1,overcast_factor_2);
	}
	last_time=current_time;
}

void CloudKeyframer::UserKeyframesChanged()
{
}

void CloudKeyframer::InterpKeyframe(CloudKeyframer::Keyframe *K)
{
	int i=GetKeyframeAtTime(K->time);
	const Keyframe *K1	=dynamic_cast<const Keyframe *>(GetKeyframe(i));
	const Keyframe *K2	=dynamic_cast<const Keyframe *>(GetKeyframe(i+1));
	if(!K1||!K2)
	{
		K->cloudiness=0;
		return;
	}
	float intrp				=GetInterp(i,K->time);
	K->cloudiness			=lerp(intrp,K1->cloudiness,K2->cloudiness);


	K->octaves				=(int)lerp(intrp,(float)K1->octaves,(float)K2->octaves);

	K->wind_speed			=lerp(intrp,K1->wind_speed,K2->wind_speed);
	K->wind_direction		=lerp(intrp,K1->wind_direction,K2->wind_direction);
	K->cloud_base_km		=lerp(intrp,K1->cloud_base_km,K2->cloud_base_km);

	K->cloud_height_km		=lerp(intrp,K1->cloud_height_km,K2->cloud_height_km);

	K->direct_light			=lerp(intrp,K1->direct_light,K2->direct_light);
	K->indirect_light		=lerp(intrp,K1->indirect_light,K2->indirect_light);
	K->ambient_light		=lerp(intrp,K1->ambient_light,K2->ambient_light);
	K->precipitation		=lerp(intrp,K1->precipitation,K2->precipitation);

	K->fractal_amplitude	=lerp(intrp,K1->fractal_amplitude,K2->fractal_amplitude);
	K->fractal_wavelength	=lerp(intrp,K1->fractal_wavelength,K2->fractal_wavelength);
	K->extinction			=PrecalculatedGamma*lerp(intrp,K1->extinction,K2->extinction);

	K->rain_to_snow			=lerp(intrp,K1->rain_to_snow,K2->rain_to_snow);
	
	K->diffusivity			=lerp(intrp,K1->diffusivity,K2->diffusivity);
	K->lightning			=lerp(intrp,K1->lightning,K2->lightning);

	float dt=(K->time-K1->time)*TimeFactor;
	float wind_speed_x1		=K1->wind_speed*sin(K1->wind_direction);
	float wind_speed_x2		=K2->wind_speed*sin(K2->wind_direction);
	float wind_speed_y1		=K1->wind_speed*cos(K1->wind_direction);
	float wind_speed_y2		=K2->wind_speed*cos(K2->wind_direction);
	K->offsetx				=K1->offsetx+wind_speed_x1*dt+dt*intrp/2.f*(wind_speed_x2-wind_speed_x1);
	K->offsety				=K1->offsety+wind_speed_y1*dt+dt*intrp/2.f*(wind_speed_y2-wind_speed_y1);
}

float CloudKeyframer::GetVisibility(const float *pos,const float *dir)
{
	float len=GetOpticalPathLength(pos,dir);
	float vis=exp(-cloudInterface->GetOpticalDensity()*len*0.001f);
	return vis;
}

float CloudKeyframer::GetVisibilityFromTo(const simul::math::Vector3 &startPoint, const simul::math::Vector3 &endPoint)
{
  if (cloudInterface)
  {
    float len=GetOpticalPathLengthFromTo(startPoint,endPoint);
    float vis=exp(-cloudInterface->GetOpticalDensity()*len*0.001f);
    return vis;
  }
  else
  {
    return 0.0f;
  }
}

float CloudKeyframer::GetOpticalPathLength(const simul::math::Vector3 &X,const simul::math::Vector3 &dir)
{
	float h=cloudInterface->GetHumidity();
	float e=cloudInterface->GetExtinction();
	int i=GetKeyframeAtTime(InterpolatedKeyframe.time);
	if(UserKeyframes)
	{
		const Keyframe *K1	=dynamic_cast<const Keyframe *>(GetKeyframe(i));
		if(K1)
		{
			cloudInterface->SetHumidity(K1->cloudiness);
			cloudInterface->SetExtinction(K1->extinction);
		}
	}
	float l1=cloudInterface->GetOpticalPathLength(X,dir);
	if(UserKeyframes)
	{
		const Keyframe *K2	=dynamic_cast<const Keyframe *>(GetKeyframe(i+1));
		if(K2)
		{
			cloudInterface->SetHumidity(K2->cloudiness);
			cloudInterface->SetExtinction(K2->extinction);
		}
	}
	float l2=cloudInterface->GetOpticalPathLength(X,dir);
	if(UserKeyframes)
	{
		cloudInterface->SetHumidity(h);
		cloudInterface->SetExtinction(e);
	}
	float l=lerp(interp,l1,l2);
	return l;
}

float CloudKeyframer::GetOpticalPathLengthFromTo(const simul::math::Vector3 &startPoint,const simul::math::Vector3 &endPoint)
{
  float h=cloudInterface->GetHumidity();
  float e=cloudInterface->GetExtinction();
  int i=GetKeyframeAtTime(InterpolatedKeyframe.time);
  if(UserKeyframes)
  {
    const Keyframe *K1	=dynamic_cast<const Keyframe *>(GetKeyframe(i));
    if(K1)
    {
      cloudInterface->SetHumidity(K1->cloudiness);
      cloudInterface->SetExtinction(K1->extinction);
    }
  }
  float l1=cloudInterface->GetOpticalPathLengthFromTo(startPoint,endPoint);
  if(UserKeyframes)
  {
    const Keyframe *K2	=dynamic_cast<const Keyframe *>(GetKeyframe(i+1));
    if(K2)
    {
      cloudInterface->SetHumidity(K2->cloudiness);
      cloudInterface->SetExtinction(K2->extinction);
    }
  }
  float l2=cloudInterface->GetOpticalPathLengthFromTo(startPoint,endPoint);
  if(UserKeyframes)
  {
    cloudInterface->SetHumidity(h);
    cloudInterface->SetExtinction(e);
  }
  float l=lerp(interp,l1,l2);
  return l;
}

simul::graph::standardnodes::ShowProgressInterface *CloudKeyframer::FindProgressCallback()
{
		simul::graph::meta::ResourceUser<simul::graph::standardnodes::ShowProgressInterface> *ru=dynamic_cast<simul::graph::meta::ResourceUser<simul::graph::standardnodes::ShowProgressInterface>*>(cloudInterface);
		simul::graph::standardnodes::ShowProgressInterface *progress=NULL;
		if(ru)
			progress=ru->GetResourceInterface();
		return progress;
}

void CloudKeyframer::UpdateCloudTexture()
{
	static float fff=1.f;
	float fill_interp=fff*((interp-0.5f)/0.45f);
	if(texture_complete[2]||fill_interp<=0.f)
		return;
	unsigned	w=cloudInterface->GetGridWidth(),
				h=cloudInterface->GetGridLength(),
				d=(Make2DTextures?1:cloudInterface->GetGridHeight());
	unsigned total_texels=w*h*d;
	if(total_texels>fill_texture_size)
		Init();
	unsigned next_index=(unsigned)((float)(total_texels*fill_interp));
	if(next_index>total_texels)
		next_index=total_texels;
	if(next_index<texel_index)
		return;
	unsigned num_texels=next_index-texel_index;
	if(num_texels<=0)
		return;
	Keyframe *K=static_cast<CloudKeyframer::Keyframe *>(GetKeyframe(current_keyframe+2));
	if(CacheClouds&&UserKeyframes&&K&&K->cache_built)
	{
		unsigned char *dest=(unsigned char *)(fill_texture[2]);
		if(Use16Bit)
			dest=(unsigned char *)(fill_texture_16bit[2]);
		size_t sz=Use16Bit?sizeof(short):sizeof(unsigned);
		sz*=texel_index;
		unsigned char *src=(unsigned char *)(K->cache);
		src+=sz;
		dest+=sz;
		if(num_texels)
			memcpy(dest,src,num_texels*(Use16Bit?(sizeof(short)):(sizeof(unsigned))));
	}
	else
	{
		simul::graph::standardnodes::ShowProgressInterface *progress=FindProgressCallback();
		if(Use16Bit)
		{
			SetBits(bits4[0],bits4[1],bits4[2],bits4[3],2,big_endian);
			ApplyBits();
			if(Make2DTextures)
				simul::clouds::TextureGenerator::Partial2DCloudTextureFill(
					cloudInterface,progress,texel_index,num_texels,(unsigned char *)(fill_texture_16bit[2]));
			else
				simul::clouds::TextureGenerator::PartialCloudTextureFill(cloudInterface,progress,texel_index,num_texels,(unsigned char *)(fill_texture_16bit[2]),false);
		}
		else
		{
			ApplyBits();
			if(Make2DTextures)
				simul::clouds::TextureGenerator::Partial2DCloudTextureFill(
					cloudInterface,progress,texel_index,num_texels,(unsigned char *)(fill_texture[2]));
			else
				simul::clouds::TextureGenerator::PartialCloudTextureFill(cloudInterface,progress,texel_index,num_texels,(unsigned char *)fill_texture[2],false);
		}
	}
	if(Use16Bit)
		FillCloudTexture(2,texel_index,num_texels,(const unsigned*)&((fill_texture_16bit[2])[texel_index]));
	else
		FillCloudTexture(2,texel_index,num_texels,&((fill_texture[2])[texel_index]));
	texel_index+=num_texels;
	// Only mark as complete if all texels have been filled:
	if(texel_index>=total_texels)
	{
		texel_index=0;
		texture_complete[2]=true;
	}
}

CloudKeyframer::Keyframe *CloudKeyframer::GetNextModifiableKeyframe()
{
	int k=current_keyframe+3;
	if(!DayLoop&&k>=this->GetNumKeyframes())
		return NULL;
	return static_cast<CloudKeyframer::Keyframe *>(GetKeyframe(k));
}

CloudKeyframer::Keyframe *CloudKeyframer::InsertKeyframe(float t)
{
	int i=GetKeyframeAtTime(t);
	// This is the keyframe to the left of t. We want to INSERT at the next point so:
	i++;
	Keyframe *K=new Keyframe();
	K->time=t;
	if(cloudInterface&&!keyframes.size())
	{
		InitKeyframeFromClouds(K,cloudInterface);
	}
	InterpKeyframe(K);
	if(i>=(int)keyframes.size())
	{
		i=(int)keyframes.size();
		keyframes.push_back(K);
	}
	else
	{
		if(i<0)
			i=0;
		keyframes.insert(keyframes.begin()+i,K);
	}
	if(DayLoop)
	{
		int T=(int)t;
		if(t<0)
			T--;
		t=t-(float)T;
	}
	// Does this new keyframe affect us?
	if(current_keyframe>i)
		current_keyframe++;
	if(i>=current_keyframe&&i<=current_keyframe+2)
	{
		interp=GetInterp(current_keyframe,skyInterface->GetTime());
		Init();
	}
	return K;
}

void CloudKeyframer::DeleteKeyframe(int i)
{
	if(i<(int)keyframes.size()&&i>=0)
		keyframes.erase(keyframes.begin()+i);
}

int CloudKeyframer::GetNumKeyframes() const
{
	return (int)keyframes.size();
}

void CloudKeyframer::SortKeyframes()
{
	/*
	bool out_of_place=(keyframes.size()>1);
	while(out_of_place)
	for(int i=0;i<(int)keyframes.size()-1;i++)
	{
		out_of_place=false;
		sky::BaseKeyframe *&K1=keyframes[i];
		sky::BaseKeyframe *&K2=keyframes[i+1];
		if(K1->time>K2->time)
		{
			std::swap(K1,K2);
			out_of_place=true;
		}
	}*/
	BaseKeyframer::SortKeyframes();
	CalcOffsets();
}

void CloudKeyframer::CalcOffsets()
{
	for(int i=0;i<(int)keyframes.size()-1;i++)
	{
		Keyframe *K1=dynamic_cast<Keyframe *>(keyframes[i]);
		Keyframe *K2=dynamic_cast<Keyframe *>(keyframes[i+1]);
		// from days to seconds:
		float dt=(K2->time-K1->time)*(TimeFactor);
		float wind_speed_x1=K1->wind_speed*sin(K1->wind_direction);
		float wind_speed_x2=K2->wind_speed*sin(K2->wind_direction);
		float wind_speed_y1=K1->wind_speed*cos(K1->wind_direction);
		float wind_speed_y2=K2->wind_speed*cos(K2->wind_direction);
		K2->offsetx		=K1->offsetx+dt/2.f*(wind_speed_x2+wind_speed_x1);
		K2->offsety		=K1->offsety+dt/2.f*(wind_speed_y2+wind_speed_y1);
	}
	offset_checksum=GetOffsetChecksum();
}

void CloudKeyframer::ForceRelight()
{
	for(int i=0;i<(int)keyframes.size();i++)
	{
		Keyframe *K=dynamic_cast<Keyframe *>(keyframes[i]);
		K->checksum=0;
	}
	for(int i=0;i<3;i++)
		texture_complete[i]=false;
}

float CloudKeyframer::GetInterp(int k,float t) const
{
	return BaseKeyframer::GetInterp(k,t);
}

float CloudKeyframer::GetPrecipitation() const
{
	return precipitation;
}

void CloudKeyframer::IncrementKeyframe()
{
	if(CacheClouds)
		CopyToCache(2,current_keyframe+2);
	{
		current_keyframe++;
	}
}

void CloudKeyframer::DecrementKeyframe()
{
	if(UserKeyframes)
	{
		current_keyframe--;
	}
}

static const unsigned MAGIC=0x1039274;
static const unsigned GRID_MAGIC=0xF36A6AAB;
static const unsigned KEYFRAME_MAGIC=0x10239413;
std::ostream &CloudKeyframer::Save(std::ostream &os) const
{
	os.write((const char*)&MAGIC,sizeof(MAGIC));
	std::streamoff tell_1=os.tellp();
	unsigned chunk_size=0;
	os.write((const char*)&chunk_size,sizeof(chunk_size));
	int kf=(int)keyframes.size();
	os.write((const char*)&kf,sizeof(kf));
	if(CacheClouds)
	{
		(const_cast<CloudKeyframer*>(this))->CacheKeyframes();
	}
	for(size_t i=0;i<keyframes.size();i++)
	{
		keyframes[i]->Save(os);
	}
	os.write((const char*)&GRID_MAGIC,sizeof(GRID_MAGIC));
	int grid=cloudInterface->GetGridWidth();
	os.write((char*)&grid,sizeof(grid));
	grid=cloudInterface->GetGridHeight();
	os.write((char*)&grid,sizeof(grid));
	float width=cloudInterface->GetCloudWidth();
	os.write((char*)&width,sizeof(width));
	char c=cloudInterface->GetWrap()?1:0;
	os.write((char*)&c,sizeof(c));
	c=CacheClouds?1:0;
	os.write((char*)&c,sizeof(c));

	unsigned res=cloudInterface->GetNoiseResolution();
	os.write((char*)&res,sizeof(res));
	float period=cloudInterface->GetNoisePeriod();
	os.write((char*)&period,sizeof(period));

	std::streamoff tell_2=os.tellp();
	chunk_size=(unsigned)tell_2-(unsigned)tell_1;
	os.seekp(tell_1);
	os.write((const char*)&chunk_size,sizeof(chunk_size));
	os.seekp(tell_2);

	return os;
}
#define TELL_CHECK {						\
	std::streamoff tell_2=(unsigned)is.tellg();	\
	if(tell_2>tell_2_required)				\
	{										\
		std::cerr<<"CloudKeyframer::Load - Position mismatch in seq file."<<" expected pos "<<tell_2_required<<" got pos "<<tell_2<<std::endl;	\
		is.seekg(tell_2_required);			\
		return is;							\
	}										\
}


std::istream &CloudKeyframer::Load(std::istream &is)
{
	SetUserKeyframes(true);
	int kf;
	if(!is.good())
		return is;
	for(int i=0;i<3;i++)
		texture_complete[i]=false;
	unsigned magic;
	is.read((char*)&magic,sizeof(magic));
	if(magic!=MAGIC)
		return is;

	std::streamoff tell_1=is.tellg();
	unsigned chunk_size=0;
	is.read((char*)&chunk_size,sizeof(chunk_size));
	std::streamoff tell_2_required=tell_1+chunk_size;

	is.read((char*)&kf,sizeof(kf));
	ClearKeyframes();
	for(size_t i=0;i<(size_t)kf;i++)
	{
		keyframes.push_back(new Keyframe());
		keyframes[i]->Load(is);
		TELL_CHECK;
	}
	is.read((char*)&magic,sizeof(magic));
	if(magic!=GRID_MAGIC)
	{
		return is;
	}
	TELL_CHECK;
	int grid=0;
	is.read((char*)&grid,sizeof(grid));
	cloudInterface->SetGridWidth(grid);
	cloudInterface->SetGridLength(grid);
	TELL_CHECK;
	is.read((char*)&grid,sizeof(grid));
	cloudInterface->SetGridHeight(grid);
	float width=40000.f;
	TELL_CHECK;
	is.read((char*)&width,sizeof(width));
	cloudInterface->SetCloudWidth(width);
	cloudInterface->SetCloudLength(width);
	char c;
	TELL_CHECK;
	is.read((char*)&c,sizeof(c));
	cloudInterface->SetWrap(c!=0);
	TELL_CHECK;
	is.read((char*)&c,sizeof(c));
	CacheClouds=(c!=0);

	TELL_CHECK;
	unsigned uint=0;
	is.read((char*)&uint,sizeof(uint));
	cloudInterface->SetNoiseResolution(uint);
	TELL_CHECK;
	float period=0;
	is.read((char*)&period,sizeof(period));
	cloudInterface->SetNoisePeriod(period);

	std::streamoff tell_2=is.tellg();
	if(tell_2!=tell_2_required)
		is.seekg(tell_2_required);

	// Must now calculate offsets
	CalcOffsets();

	// Initialize the checksums:
	unsigned glob=GetGlobalChecksum();
	for(size_t i=0;i<(size_t)kf;i++)
	{
		Keyframe *K=dynamic_cast<Keyframe *>(GetKeyframe((int)i));
		if(K)
			K->checksum=glob+K->GetPropertiesChecksum();
	}
	return is;
}

void CloudKeyframer::New()
{
	ClearKeyframes();
}

bool CloudKeyframer::IsKeyframeChecksumValid(const CloudKeyframer::Keyframe *K) const
{
	unsigned glob=GetGlobalChecksum();
	return (K->checksum==glob+K->GetPropertiesChecksum());
}


std::ostream &CloudKeyframer::Keyframe::Save(std::ostream &os) const
{
	os.write((const char*)&KEYFRAME_MAGIC,sizeof(KEYFRAME_MAGIC));

	std::streamoff tell_1=os.tellp();
  // LUKE - sizeof(std::streamoff) varies in VS2005 and VS2010 - so if SkySequencer is compiled in VS2005 and we read values in VS2010, it will corrupt values
  // so we use unsigned instead
  unsigned chunk_size=0;
	os.write((const char*)&chunk_size,sizeof(chunk_size));

	os.write((const char*)&time,sizeof(time));
	os.write((const char*)&cloudiness,sizeof(cloudiness));
	os.write((const char*)&wind_speed,sizeof(wind_speed));
	os.write((const char*)&wind_direction,sizeof(wind_direction));
	os.write((const char*)&cloud_base_km,sizeof(cloud_base_km));
	os.write((const char*)&cloud_height_km,sizeof(cloud_height_km));
	
	os.write((const char*)&direct_light,sizeof(direct_light));
	os.write((const char*)&indirect_light,sizeof(indirect_light));
	os.write((const char*)&ambient_light,sizeof(ambient_light));

	os.write((const char*)&precipitation,sizeof(precipitation));
	os.write((const char*)&fractal_amplitude,sizeof(fractal_amplitude));
	os.write((const char*)&fractal_wavelength,sizeof(fractal_wavelength));
	
	os.write((const char*)&octaves,sizeof(octaves));
	os.write((const char*)&persistence,sizeof(persistence));

	os.write((const char*)&extinction,sizeof(extinction));

	os.write((const char*)&rain_to_snow,sizeof(rain_to_snow));

	os.write((const char*)&diffusivity,sizeof(diffusivity));

	char c=cache_built?1:0;
	os.write((const char*)&c,sizeof(c));

	if(cache_built)
	{
		os.write((const char*)&cache_size_bytes,sizeof(cache_size_bytes));
		os.write((const char*)cache,cache_size_bytes);
	}

	os.write((const char*)&lightning,sizeof(lightning));

	std::streamoff tell_2=os.tellp();
	chunk_size=tell_2-tell_1;
	os.seekp(tell_1);
	os.write((const char*)&chunk_size,sizeof(chunk_size));
	os.seekp(tell_2);

	return os;
}

std::istream &CloudKeyframer::Keyframe::Load(std::istream &is)
{
	unsigned magic;
	is.read((char*)&magic,sizeof(magic));
	if(magic!=KEYFRAME_MAGIC)
		return is;

	std::streamoff tell_1=is.tellg();
  // LUKE - sizeof(std::streamoff) varies in VS2005 and VS2010 - so if SkySequencer is compiled in VS2005 and we read values in VS2010, it will corrupt values
  // so we use unsigned instead
	unsigned chunk_size=0;
	is.read((char*)&chunk_size,sizeof(chunk_size));
	std::streamoff tell_2_required=tell_1+chunk_size;

	is.read((char*)&time,sizeof(time));
	is.read((char*)&cloudiness,sizeof(cloudiness));
	is.read((char*)&wind_speed,sizeof(wind_speed));
	is.read((char*)&wind_direction,sizeof(wind_direction));
	is.read((char*)&cloud_base_km,sizeof(cloud_base_km));
	is.read((char*)&cloud_height_km,sizeof(cloud_height_km));
	TELL_CHECK;
	is.read((char*)&direct_light,sizeof(direct_light));
	TELL_CHECK;
	is.read((char*)&indirect_light,sizeof(indirect_light));
	TELL_CHECK;
	is.read((char*)&ambient_light,sizeof(ambient_light));
	TELL_CHECK;

	is.read((char*)&precipitation,sizeof(precipitation));
	TELL_CHECK;
	is.read((char*)&fractal_amplitude,sizeof(fractal_amplitude));
	TELL_CHECK;
	is.read((char*)&fractal_wavelength,sizeof(fractal_wavelength));
	TELL_CHECK;

	is.read((char*)&octaves,sizeof(octaves));
	TELL_CHECK;
	is.read((char*)&persistence,sizeof(persistence));
	TELL_CHECK;
	is.read((char*)&extinction,sizeof(extinction));
	TELL_CHECK;
	is.read((char*)&rain_to_snow,sizeof(rain_to_snow));
	TELL_CHECK;
	is.read((char*)&diffusivity,sizeof(diffusivity));

	TELL_CHECK;

	cache_built=false;
	delete [] cache;
	cache=NULL;
	cache_size_bytes=0;

	char c;
	is.read((char*)&c,sizeof(c));
	TELL_CHECK;
	cache_built=(c!=0);
	if(cache_built)
	{
		is.read((char*)&cache_size_bytes,sizeof(cache_size_bytes));
		cache=new unsigned char[cache_size_bytes];
		TELL_CHECK;
		is.read((char*)cache,cache_size_bytes);
	}

	TELL_CHECK;
	is.read((char*)&lightning,sizeof(lightning));

	std::streamoff tell_2=is.tellg();
	if(tell_2!=tell_2_required)
		is.seekg(tell_2_required);

	return is;
}
bool CloudKeyframer::Keyframe::HasValue(const char *name) const
{
	if(_stricmp(name,"cloudiness")==0)
		return true;
	else if(_stricmp(name,"windSpeed")==0)
		return true;
	else if(_stricmp(name,"windDirection")==0)
		return true;
	else if(_stricmp(name,"octaves")==0)
		return true;
	else if(_stricmp(name,"persistence")==0)
		return true;
	else if(_stricmp(name,"cloudBase")==0)
		return true;
	else if(_stricmp(name,"cloudHeight")==0)
		return true;
	else if(_stricmp(name,"directLight")==0)
		return true;
	else if(_stricmp(name,"indirectLight")==0)
		return true;
	else if(_stricmp(name,"ambientLight")==0)
		return true;
	else if(_stricmp(name,"precipitation")==0)
		return true;
	else if(_stricmp(name,"fractalWavelength")==0)
		return true;
	else if(_stricmp(name,"fractalAmplitude")==0)
		return true;
	else if(_stricmp(name,"extinction")==0)
		return true;
	else if(_stricmp(name,"rainToSnow")==0)
		return true;
	else if(_stricmp(name,"diffusivity")==0)
		return true;
	else if(_stricmp(name,"cache_built")==0)
		return true;
	else if(_stricmp(name,"lightning")==0)
		return true;
	return BaseKeyframe::HasValue(name);
}

float CloudKeyframer::Keyframe::GetValue(const char *name) const
{
	if(_stricmp(name,"cloudiness")==0)
		return cloudiness;
	else if(_stricmp(name,"windSpeed")==0)
		return wind_speed;
	else if(_stricmp(name,"windDirection")==0)
		return wind_direction;
	else if(_stricmp(name,"persistence")==0)
		return persistence;
	else if(_stricmp(name,"cloudBase")==0)
		return cloud_base_km;
	else if(_stricmp(name,"cloudHeight")==0)
		return cloud_height_km;
	else if(_stricmp(name,"directLight")==0)
		return direct_light;
	else if(_stricmp(name,"indirectLight")==0)
		return indirect_light;
	else if(_stricmp(name,"ambientLight")==0)
		return ambient_light;
	else if(_stricmp(name,"precipitation")==0)
		return precipitation;
	else if(_stricmp(name,"fractalWavelength")==0)
		return fractal_wavelength;
	else if(_stricmp(name,"fractalAmplitude")==0)
		return fractal_amplitude;
	else if(_stricmp(name,"extinction")==0)
		return extinction;
	else if(_stricmp(name,"rainToSnow")==0)
		return rain_to_snow;
	else if(_stricmp(name,"diffusivity")==0)
		return diffusivity;
	else if(_stricmp(name,"lightning")==0)
		return lightning;
	return BaseKeyframe::GetValue(name);
}

void CloudKeyframer::Keyframe::SetValue(const char *name,float val)
{
	if(_stricmp(name,"cloudiness")==0)
		cloudiness=val;
	else if(_stricmp(name,"windSpeed")==0)
		wind_speed=val;
	else if(_stricmp(name,"windDirection")==0)
		wind_direction=val;
	else if(_stricmp(name,"persistence")==0)
		persistence=val;
	else if(_stricmp(name,"cloudBase")==0)
		cloud_base_km=val;
	else if(_stricmp(name,"cloudHeight")==0)
		cloud_height_km=val;
	else if(_stricmp(name,"directLight")==0)
		direct_light=val;
	else if(_stricmp(name,"indirectLight")==0)
		indirect_light=val;
	else if(_stricmp(name,"ambientLight")==0)
		ambient_light=val;
	else if(_stricmp(name,"precipitation")==0)
		precipitation=val;
	else if(_stricmp(name,"fractalWavelength")==0)
		fractal_wavelength=val;
	else if(_stricmp(name,"fractalAmplitude")==0)
		fractal_amplitude=val;
	else if(_stricmp(name,"extinction")==0)
		extinction=val;
	else if(_stricmp(name,"rainToSnow")==0)
		rain_to_snow=val;
	else if(_stricmp(name,"diffusivity")==0)
		diffusivity=val;
	else if(_stricmp(name,"lightning")==0)
		lightning=val;
	BaseKeyframe::SetValue(name,val);
}

int CloudKeyframer::Keyframe::GetInt(const char *name) const
{
	if(_stricmp(name,"octaves")==0)
		return octaves;
	if(_stricmp(name,"cache_built")==0)
		return cache_built;
	return BaseKeyframe::GetInt(name);
}

void CloudKeyframer::Keyframe::SetInt(const char *name,int val)
{
	if(_stricmp(name,"octaves")==0)
		octaves=val;
	if(_stricmp(name,"cache_built")==0)
		cache_built=(val!=0);
	BaseKeyframe::SetInt(name,val);
}

unsigned CloudKeyframer::Keyframe::GetPropertiesChecksum() const
{
	unsigned new_checksum=0;
	new_checksum+=*reinterpret_cast<const unsigned*>(&cloudiness);
	new_checksum+=*reinterpret_cast<const unsigned*>(&octaves);
	new_checksum+=*reinterpret_cast<const unsigned*>(&persistence);
	new_checksum+=*reinterpret_cast<const unsigned*>(&wind_speed);
	new_checksum+=*reinterpret_cast<const unsigned*>(&wind_direction);
	new_checksum+=*reinterpret_cast<const unsigned*>(&offsetx);
	new_checksum+=*reinterpret_cast<const unsigned*>(&offsety);
	new_checksum+=*reinterpret_cast<const unsigned*>(&extinction);
	new_checksum+=*reinterpret_cast<const unsigned*>(&diffusivity);
	return new_checksum;
}

unsigned CloudKeyframer::GetOffsetChecksum() const
{
	unsigned new_checksum=0;
	for(size_t i=0;i<keyframes.size();i++)
	{
		const Keyframe *K=dynamic_cast<const Keyframe *>(GetKeyframe((int)i));
		new_checksum+=*reinterpret_cast<const unsigned*>(&K->wind_speed);
		new_checksum+=*reinterpret_cast<const unsigned*>(&K->wind_direction);
		new_checksum+=*reinterpret_cast<const unsigned*>(&K->offsetx);
		new_checksum+=*reinterpret_cast<const unsigned*>(&K->offsety);
	}
	return new_checksum;
}

void CloudKeyframer::CycleTexturesForward()
{
	if(renderCallback)
		renderCallback->CycleTexturesForward();
	std::swap(fill_texture[0],fill_texture[1]);
	std::swap(fill_texture[1],fill_texture[2]);

	std::swap(fill_texture_16bit[0],fill_texture_16bit[1]);
	std::swap(fill_texture_16bit[1],fill_texture_16bit[2]);
	

	std::swap(texture_complete[0],texture_complete[1]);
	std::swap(texture_complete[1],texture_complete[2]);
	texture_complete[2]=false;
}

void CloudKeyframer::CycleTexturesBackward()
{
	if(renderCallback)
	{
		renderCallback->CycleTexturesForward();
		renderCallback->CycleTexturesForward();
	}
	std::swap(fill_texture[1],fill_texture[2]);
	std::swap(fill_texture[0],fill_texture[1]);

	std::swap(fill_texture_16bit[1],fill_texture_16bit[2]);
	std::swap(fill_texture_16bit[0],fill_texture_16bit[1]);
	
	std::swap(texture_complete[1],texture_complete[2]);
	std::swap(texture_complete[0],texture_complete[1]);
	texture_complete[0]=false;
}
	
