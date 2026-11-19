#include <algorithm>
#include "precomp.h"

#include "Simul/Sky/SkyKeyframer.h"
#include "Simul/Sky/TextureGenerator.h"
#include "Simul/Sky/OvercastCallback.h"
#include "Simul/Sky/BaseSky.h"
#include "Simul/Sky/Sky.h"

#ifdef _MSC_VER
#include <windows.h>
#undef min
#undef max
#else
#include <stdio.h>
#include <strings.h>
#define _stricmp strcasecmp
#endif

#include "Simul/Math/Pi.h"
#include <math.h>
#include <assert.h>

using namespace simul;
using namespace sky;

static float Lerp(float x,float a,float b)
{
	float r=x*b;
	r+=(1.f-x)*a;
	return r;
}

static simul::sky::float4 AngleLerp(float &az,float &el,float interp,float az1,float el1,float az2,float el2)
{
	simul::sky::float4 dir_1=BaseSky::AzimuthElevationToDirection(az1,el1);
	simul::sky::float4 dir_2=BaseSky::AzimuthElevationToDirection(az2,el2);
	simul::sky::float4 dir=lerp(interp,dir_1,dir_2);
	if(length(dir))
		dir/=length(dir);
	az	=atan2(dir.x,dir.y);
	el	=asin(dir.z);
	//moon_dir=BaseSky::AzimuthElevationToDirection(MoonAzimuth,MoonElevation);
	return dir;
}

SkyKeyframer::SkyKeyframer(SkyInterface *si,SkyTexturesCallback *c,int h,int w,float max_dist_km)
	:alt_interp(0.f)
	,time(0.5f)
	,UseTbb(true)
	,EarthTest(false)
	,initialized(false)
	,AltitudeKM(0.5f)
	,MaxDistanceKm(max_dist_km)
	,UseAltitudeKm(0.5f)
	,TableWidth(w)
	,TableHeight(h)
	,last_checksum(0)
	,which_texture(0)
	,skyInterface(si)
	,interp(0.f)
	,callback(0)
	,recalc(true)
	,overcastCallback(NULL)
	,day(0)
	,BrightnessAdjustment(1.f)
	,FillTexturesAsBlocks(false)
	,CalculateAllAltitudes(false)
	,StartDayNumber(0)
  ,UseKeyframerDate(true)
	,LinkKeyframeTimeAndDaytime(true)
	,AutoBrightness(true)
	,AutoBrightnessStrength(0.5f)
	,MaxAltitudeKm(15.f)
	,StarBrightness(5000.f)
	,MaxStarMagnitude(4.f)
  ,useSunOrMoon(false)
  ,sunOrMoon(1.0f)
{
	ChangedTbb();
	siderealSkyInterface=dynamic_cast<SiderealSkyInterface *>(si);
	InitializePropertiesDefinition();
	InitializePropertiesState();
	SetAltitudeRangeKm(0.f,MaxAltitudeKm);
	SetNumAltitudes(4);
	last_alt=AltitudeKM;
	SetCallback(c);
	ChangedSkyInterface();
	SetUniformKeyframes(8);
//	stars.LoadAsText("UrsaMajor.txt"); used for calibration
	stars.UseDefaultStars(MaxStarMagnitude);
}

SkyKeyframer::~SkyKeyframer()
{
	for(size_t j=0;j<lookups.size();j++)
	{
		delete lookups[j];
	}
	lookups.clear();
}

const SkyKeyframer &SkyKeyframer::operator=(const SkyKeyframer& aft)
{
	int kf=current_keyframe;
	current_keyframe=-100000;
	ClearKeyframes();
	for(int i=0;i<aft.GetNumKeyframes();i++)
	{
		const SkyKeyframe *k=dynamic_cast<const SkyKeyframe *>(aft.GetKeyframe(i));
		if(k)
		{
			SkyKeyframe *K=dynamic_cast<SkyKeyframe *>(InsertKeyframe(k->time));
			*K=*k;
		}
	}
	current_keyframe=kf;
	return *this;
}

void SkyKeyframer::GetStartDate(int &y,int &m,int &d) const
{
	Sky::GetDateFromDayNumber(StartDayNumber,y,m,d);
}

void SkyKeyframer::SetStartDate(int y,int m,int d)
{
	int sdn=Sky::GetDayNumber(y,m,d);

	if(sdn!=StartDayNumber)
	{
		StartDayNumber=sdn;
		last_checksum=0;
	}

	Sky::GetDateFromDayNumber(StartDayNumber,y,m,d);
	SetDaytime(time);
}


void SkyKeyframer::SetLatitude(float lat_radians)
{
	if(siderealSkyInterface)
		return siderealSkyInterface->SetLatitudeRadians(lat_radians);
}

float SkyKeyframer::GetLatitude() const
{
	if(siderealSkyInterface)
		return siderealSkyInterface->GetLatitudeRadians();
	return 0.f;
}

void SkyKeyframer::SetRecalculate()
{
	recalc=true;
}

float SkyKeyframer::GetInterpolation()
{
	return interp;
}

void SkyKeyframer::ChangedSkyInterface()
{
	if(skyInterface)
	{
		which_texture=2;
		SetRecalculate();
	}
}

void SkyKeyframer::SetAltitudeRangeKm(float min_km,float max_km)
{
	if(lookups.size()>1)
	{
		for(size_t j=0;j<lookups.size();j++)
		{
			float range=max_km-min_km;
			float diff=pow((float)j/(float)(lookups.size()-1),2.f);
			lookups[j]->altitude=min_km+range*diff;
		}
	}
	else if(lookups.size()==1)
	{
		lookups[0]->altitude=0;
	}

	UpdateMaxDistanceTexture();
}

void SkyKeyframer::UpdateMaxDistanceTexture()
{
	if(callback)
	{
		float *dist_array=new float[lookups.size()*TableHeight];
		float *d=dist_array;
		for(int i=0;i<(int)lookups.size();i++)
		{
			// add 100 metres below Earth's surface.
			float alt_km=lookups[i]->altitude+1.f;
			for(int j=0;j<(int)TableHeight;j++)
			{
				float sine=-1.f+2.f*((float)j+0.5f)/(float)TableHeight;
				float elev=asin(sine);
				*d=CalcMaxDistanceKm(alt_km,elev);
				*d/=this->MaxDistanceKm;
				d++;
			}
		}
		//callback->FillDistanceTexture(TableHeight,lookups.size(),dist_array);
		delete [] dist_array;
	}
}

float SkyKeyframer::CalcMaxDistanceKm(float alt_km,float elev) const
{
	float d=skyInterface->GetDistanceToEarth(elev,alt_km);
	if(d<0.f)
		d=MaxDistanceKm;
	else
		d=0.7f*d+0.3f*MaxDistanceKm;
	if(d<1.f)
		d=1.f;
	//if(d>MaxDistanceKm)
		d=MaxDistanceKm;
	return d;
}

void SkyKeyframer::SetNumAltitudes(unsigned a)
{
	if(a<1)
		a=1;
	if(a>64)
		a=64;
	float mina=0.f;
	float maxa=MaxAltitudeKm;
	if(a!=lookups.size())
	{
		if(lookups.size())
		{
			mina=lookups[0]->altitude;
			maxa=lookups[lookups.size()-1]->altitude;
		}
		for(size_t j=0;j<lookups.size();j++)
		{
			delete lookups[j];
		}
		lookups.clear();
		lookups.resize(a,NULL);
		for(size_t i=0;i<a;i++)
		{
			TexArray *tex=new TexArray();
			lookups[i]=tex;
			tex->altitude=0.f;
		}
	}
	SetAltitudeRangeKm(mina,maxa);
	recalc=true;
}

unsigned SkyKeyframer::GetNumAltitudes() const
{
	return (unsigned)lookups.size();
}

void SkyKeyframer::RestartNextKeyframe()
{
///	fadeTexIndex=skyTexIndex=0;
}

void SkyKeyframer::InitFadeTexture(int i)
{
	StoreSky();
	ApplySkyKeyframe(current_keyframe+i);
	SkyKeyframe *K=(SkyKeyframe*)this->GetKeyframe(current_keyframe+i);
	if(K)
	{
		if(LinkKeyframeTimeAndDaytime)
			K->daytime=K->time;	
		CalcAutoBrightnessForKeyframe(K);
	}
	ApplySkyKeyframe(current_keyframe+i);
	int idx=(which_texture+1+i)%3;

//UseTbb=false;

	int alt_index=GetAltIndex(AltitudeKM);
	float alt_interp=GetAltInterp(AltitudeKM);
	unsigned TableSize=TableHeight*TableWidth;

	int start_alt=std::max(0,alt_index-1);
	int end_alt=std::min((int)lookups.size(),alt_index+3);
	if(CalculateAllAltitudes)
	{
		start_alt=0;
		end_alt=(int)lookups.size();
	}
	for(int j=0;j<(int)lookups.size();j++)
	{
		TexArray *tex=lookups[j];
		tex->sky_lookup[idx].Setup(TableHeight,false,false);
		tex->sky_lookup[idx].Zero();
		tex->loss_lookup[idx].Setup(TableWidth,TableHeight,false,false);
		tex->loss_lookup[idx].Zero();
		tex->fadeTexIndex[idx]=0;
		tex->skyTexIndex[idx]=0;
		tex->inscatter_lookup[idx].Setup(TableWidth,TableHeight,false,false);
		tex->inscatter_lookup[idx].Zero();
	}
	for(int j=start_alt;j<end_alt;j++)
	{
		TexArray *tex=lookups[j];
		// only fully build the fades at the heights above and below the viewer.
		float complete=1.f;
		// partially build the fades above and below these
		if(j==alt_index-1)
			complete=1.f-alt_interp;
		else if(j==alt_index+2)
			complete=alt_interp;

		unsigned nextFadeIndex=(unsigned)((complete)*TableSize);
		if(nextFadeIndex>TableSize)
			nextFadeIndex=TableSize;
		unsigned nextSkyIndex=(unsigned)((complete)*TableHeight);
		if(nextSkyIndex>TableHeight)
			nextSkyIndex=TableHeight;

		simul::sky::float4 *ptr1=tex->loss_lookup[idx].GetDataPointer();
		simul::sky::float4 *ptr2=tex->inscatter_lookup[idx].GetDataPointer();
		simul::sky::float4 *sky_ptr=tex->sky_lookup[idx].GetDataPointer();
		simul::sky::TextureGenerator::Make2DLossAndInscatterTextures(skyInterface,TableHeight,TableWidth,
			(unsigned char *)ptr1,(unsigned char *)ptr2,(unsigned char *)sky_ptr,
			simul::sky::TextureGenerator::RGBA,tex->altitude,MaxDistanceKm,0,nextFadeIndex,16,EarthTest,UseTbb);


		tex->fadeTexIndex[idx]=nextFadeIndex;
		tex->skyTexIndex[idx]=nextSkyIndex;
	}
	RestoreSky();
	FillCallbackTexture(i);
}

void SkyKeyframer::InitFadeTextures()
{
// We're going to DRIVE the sky from the fadetable's time value
//	time=skyInterface->GetDaytime();
	current_keyframe=GetKeyframeAtTime(time);
	if(callback)
	{
		callback->SetFadeTextureSize(TableWidth,TableHeight,(unsigned)lookups.size());
		callback->SetSkyTextureSize(TableHeight);
	}
	UseAltitudeKm=AltitudeKM;
	SetNumAltitudes(lookups.size());
	StoreSky();
	// Make textures:
	for(int i=0;i<3;i++)
	{
		InitFadeTexture(i);
	}
	RestoreSky();
	recalc=false;
	last_checksum=simul::sky::TextureGenerator::Get1DSkyChecksum(skyInterface,0);
	ChecksumsFailed(time);
}

void SkyKeyframer::SetCallback(SkyTexturesCallback *c)
{
	if(callback!=c)
	{
		callback=c;
		if(callback)
		{
			callback->SetFadeTextureSize(TableWidth,TableHeight,(unsigned)lookups.size());
			callback->SetSkyTextureSize(TableHeight);
			if(!recalc)
				FillCallbackTextures();
			UpdateMaxDistanceTexture();
		}
	}
}

void SkyKeyframer::SetOvercastCallback(simul::sky::OvercastCallback *c)
{
	overcastCallback=c;
}

float SkyKeyframer::GetAltInterp(float alt_km) const
{
	if(lookups.size()<2)
	{
		return 0;
	}
	int index=GetAltIndex(alt_km);
	TexArray *tex1=lookups[index];
	TexArray *tex2=lookups[index+1];
	float interp=(alt_km-tex1->altitude)/(tex2->altitude-tex1->altitude);
	if(interp<0)
		interp=0;
	if(interp>1.f)
		interp=1.f;
	return interp;
}
// if alt changed, accelerate interpolation:
void SkyKeyframer::AltitudeChanged()
{
	//alt_index=GetAltIndex(AltitudeKM);
	alt_interp=GetAltInterp(AltitudeKM);
}

void SkyKeyframer::FillCallbackTextures()
{
	for(int i=0;i<3;i++)
		FillCallbackTexture(i);
}

void SkyKeyframer::FillFadeTextures(int alt_index,int texture_index,int texel_index,int num_texels,const float *loss_float4_array,const float *inscatter_float4_array)
{
	if(!callback)
		return;
	if(!FillTexturesAsBlocks)
	{
		texel_index+=alt_index*TableWidth*TableHeight;
		callback->FillFadeTexturesSequentially(texture_index,texel_index,num_texels,loss_float4_array,inscatter_float4_array);
		return;
	}
	int stride=TableWidth;

	int start_row			=texel_index/stride;
	int start_texel_in_row	=texel_index-start_row*stride;

	int end_texel			=texel_index+num_texels;
	int end_row				=end_texel/stride;
	int end_texel_in_row	=end_texel-end_row*stride;

	int first_row_length=stride-start_texel_in_row;
	int num_rows=end_row-start_row-1;
	if(end_row==start_row)
	{
		first_row_length=end_texel_in_row-start_texel_in_row;
		end_texel_in_row=0;
		num_rows=0;
	}
	const float4 *lptr=(const float4 *)loss_float4_array;
	const float4 *iptr=(const float4 *)inscatter_float4_array;
	int num_written=0;
	if(first_row_length>0)
	{
		callback->FillFadeTextureBlocks(texture_index,start_texel_in_row,start_row,alt_index,first_row_length,1,1,(const float *)lptr,(const float *)iptr);
		lptr+=first_row_length;
		iptr+=first_row_length;
		num_written+=first_row_length;
	}
	// The rest of the first slice:
	if(num_rows>0)
	{
		callback->FillFadeTextureBlocks(texture_index,0,start_row+1,alt_index,stride,num_rows,1,(const float *)lptr,(const float *)iptr);
		lptr+=stride*num_rows;
		iptr+=stride*num_rows;
		num_written+=stride*num_rows;
	}
	// and the final row:
	if(end_texel_in_row>0)
	{
		callback->FillFadeTextureBlocks(texture_index,0,end_row,alt_index,end_texel_in_row,1,1,(const float *)lptr,(const float *)iptr);
		lptr+=end_texel_in_row;
		iptr+=end_texel_in_row;
		num_written+=end_texel_in_row;
	}
	if(num_texels!=num_written)
	{
		std::cerr<<"Trying to write too much cloud tex memory"<<std::endl;
		assert(num_texels==num_written);
	}
}

void SkyKeyframer::FillCallbackTexture(int i)
{
	if(!callback)
		return;
	int idx=(which_texture+1+i)%3;
	for(size_t j=0;j<lookups.size();j++)
	{
		TexArray *tex=lookups[j];
		if(tex->loss_lookup[idx].GetDataPointer()&&tex->inscatter_lookup[idx].GetDataPointer())
			FillFadeTextures((int)j,i,0,TableHeight*TableWidth,(const float*)tex->loss_lookup[idx].GetDataPointer(),
				(const float*)tex->inscatter_lookup[idx].GetDataPointer());
		if(tex->sky_lookup[idx].GetDataPointer())
			callback->FillSkyTexture((int)j,i,0,TableHeight,(const float*)tex->sky_lookup[idx].GetDataPointer());
	}
}

void SkyKeyframer::SetSkyInterface(SkyInterface *s)
{
	skyInterface=s;
	siderealSkyInterface=dynamic_cast<SiderealSkyInterface *>(s);
}
SkyInterface *SkyKeyframer::GetSkyInterface()
{
	return skyInterface;
}

int SkyKeyframer::GetAltitudeIndex() const
{
	return GetAltIndex(AltitudeKM);
}
	
float SkyKeyframer::GetAltitudeInterpolation() const
{
	return alt_interp;
}


float SkyKeyframer::GetTime() const
{
	return time;
}

float SkyKeyframer::GetDaytime() const
{
	return interpolatedKeyframe.daytime;
}
void SkyKeyframer::SetDaytime(float d)
{
	if(LinkKeyframeTimeAndDaytime)
		SetTime(d);
}
			
void SkyKeyframer::SortKeyframes()
{
	BaseKeyframer::SortKeyframes();
	LinkChanged();
}

void SkyKeyframer::SetTime(float d)
{
	time=d;
	DoInterpolation();
	
  if (UseKeyframerDate)
  {
	float t=interpolatedKeyframe.daytime;
	int T=(int)t;
	if(t<0)
		T--;
	int DN=StartDayNumber+T;
	if(siderealSkyInterface)
	{
		int y,m,d;
		Sky::GetDateFromDayNumber(DN,y,m,d);
		siderealSkyInterface->SetYear(y);
		siderealSkyInterface->SetDate(m,d);
	}
}
}

float SkyKeyframer::GetMieEccentricity() const
{
	return skyInterface->GetMieEccentricity();
}

const float4 &SkyKeyframer::GetDirectionToSun() const
{
	static float4 sun_dir;
	int k1=GetKeyframeAtTime(time);
	const SkyKeyframe *K1=dynamic_cast<const SkyKeyframe *>(GetKeyframe(k1));
	const SkyKeyframe *K2=dynamic_cast<const SkyKeyframe *>(GetKeyframe(k1+1));
	if((!siderealSkyInterface||!siderealSkyInterface->GetAutomaticSunPosition())&&K1&&K2)
	{
		float SunAzimuth,SunElevation;
		sun_dir=AngleLerp(SunAzimuth,SunElevation,interp,K1->SunAzimuth,K1->SunElevation,K2->SunAzimuth,K2->SunElevation);
	}
	else if(siderealSkyInterface)
	{
		sun_dir=skyInterface->GetDirectionToSun();
	}
	return sun_dir;
}

const float4 &SkyKeyframer::GetDirectionToMoon() const
{
	static float4 moon_dir;
	int k1=GetKeyframeAtTime(time);
	const SkyKeyframe *K1=dynamic_cast<const SkyKeyframe *>(GetKeyframe(k1));
	const SkyKeyframe *K2=dynamic_cast<const SkyKeyframe *>(GetKeyframe(k1+1));
	if((!siderealSkyInterface||!siderealSkyInterface->GetAutomaticMoonPosition())&&K1&&K2)
	{
		float SunAzimuth,SunElevation;
		moon_dir=AngleLerp(SunAzimuth,SunElevation,interp,K1->MoonAzimuth,K1->MoonElevation,K2->MoonAzimuth,K2->MoonElevation);
	}
	else if(siderealSkyInterface)
	{
		moon_dir=skyInterface->GetDirectionToMoon();
	}
	return moon_dir;
}

const float4 &SkyKeyframer::GetDirectionToLight() const
{
  if (useSunOrMoon)
  {
    float4 sun_dir=skyInterface->GetDirectionToSun();
    float4 moon_dir=skyInterface->GetDirectionToMoon();

    static float4 directionToLight;
    directionToLight=lerp(sunOrMoon,moon_dir, sun_dir);
    return directionToLight;
  }

	int k1=GetKeyframeAtTime(time);
	const SkyKeyframe *K1=dynamic_cast<const SkyKeyframe *>(GetKeyframe(k1));
	const SkyKeyframe *K2=dynamic_cast<const SkyKeyframe *>(GetKeyframe(k1+1));
	static float4 DirectionToLight;
	static float4 sun_dir,moon_dir;
	DirectionToLight.Set(0,0,1.f,0);
	if(K1&&K2)
	{
		float m1=K1->UseMoon?1.f:0.f;
		float m2=K2->UseMoon?1.f:0.f;
		float m=Lerp(interp,m1,m2);
		if(m<1.f)
		{
			if(!siderealSkyInterface||!siderealSkyInterface->GetAutomaticSunPosition())
			{
				float SunAzimuth,SunElevation;
				sun_dir=AngleLerp(SunAzimuth,SunElevation,interp,K1->SunAzimuth,K1->SunElevation,K2->SunAzimuth,K2->SunElevation);
			}
			else if(siderealSkyInterface)
			{
				sun_dir=skyInterface->GetDirectionToSun();
			}
			if(m==0.f)
				return sun_dir;
		}
		if(m>0.f)
		{
			if(siderealSkyInterface&&siderealSkyInterface->GetAutomaticMoonPosition())
			{
				moon_dir=skyInterface->GetDirectionToMoon();
			}
			else
			{
				float MoonAzimuth,MoonElevation;
				moon_dir=AngleLerp(MoonAzimuth,MoonElevation,interp,K1->MoonAzimuth,K1->MoonElevation,K2->MoonAzimuth,K2->MoonElevation);
			}
			if(m==1.f)
				return moon_dir;
		}
		DirectionToLight=lerp(m,sun_dir,moon_dir);
	}
	else if(siderealSkyInterface)
	{
		DirectionToLight=skyInterface->GetDirectionToLight();
	}
	return DirectionToLight;
}

float4 SkyKeyframer::GetLocalIrradiance(float altitude_km) const
{
  if (useSunOrMoon)
  {
    float4 moonlight=skyInterface->GetMoonIrradiance(altitude_km);
    float4 sunlight=skyInterface->GetLocalIrradiance(altitude_km);
    
    return lerp(sunOrMoon,moonlight, sunlight);
  }

	int k1=GetKeyframeAtTime(time);
	const SkyKeyframe *K1=dynamic_cast<const SkyKeyframe *>(GetKeyframe(k1));
	const SkyKeyframe *K2=dynamic_cast<const SkyKeyframe *>(GetKeyframe(k1+1));
	float4 irr;
	if(K1&&K2)
	{
		float m1=K1->UseMoon?1.f:0.f;
		float m2=K2->UseMoon?1.f:0.f;
		float m=Lerp(interp,m1,m2);
		if(m==1.f)
		{
			irr=skyInterface->GetMoonIrradiance(altitude_km);
		}
		else if(m==0.f)
			irr=skyInterface->GetLocalIrradiance(altitude_km);
		else
		{
			float4 moonlight=skyInterface->GetMoonIrradiance(altitude_km);
			float4 sunlight=skyInterface->GetLocalIrradiance(altitude_km);
			irr=lerp(m,sunlight,moonlight);
		}
	}
	else
		irr=skyInterface->GetLocalIrradiance(altitude_km);
	return irr;
}

float4 SkyKeyframer::GetMieRayleighRatio() const
{
	return skyInterface->GetMieRayleighRatio();
}
float SkyKeyframer::GetDistanceToSpace(float Elevation,float h_km) const
{
	return skyInterface->GetDistanceToSpace(Elevation,h_km);
}
float SkyKeyframer::GetDistanceToEarth(float Elevation,float h_km) const
{
	return skyInterface->GetDistanceToEarth(Elevation,h_km);
}

void SkyKeyframer::TimeStep(float dt)
{
	time+=dt;
	DoInterpolation();
}

void SkyKeyframer::Update()
{
	// Now using our time value to DRIVE skyInterface.
	//time=skyInterface->GetDaytime();
	if(ChecksumsFailed(time))
	{
		recalc=true;
	}
	interp=GetInterp(current_keyframe,time);
	unsigned checksum=simul::sky::TextureGenerator::Get1DSkyChecksum(skyInterface,0);
	if(checksum!=last_checksum)
	{	
		last_checksum=checksum;
		recalc=true;
	}
	if(!recalc)
	{
		StoreSky();

ApplySkyKeyframe(current_keyframe+2);
float4 skylight=skyInterface->GetAmbientLight(2.f);
		unsigned TableSize=TableHeight*TableWidth;
		unsigned nextFadeIndex=(unsigned)(interp*TableSize);
		if(nextFadeIndex>TableSize)
			nextFadeIndex=TableSize;
			int alt_index=GetAltitudeIndex();
		int start_alt=std::max(0,alt_index-1);
		int end_alt=std::min((int)lookups.size(),alt_index+3);
		if(CalculateAllAltitudes)
		{
			start_alt=0;
			end_alt=(int)lookups.size();
		}
		if(TableSize>0&&TableHeight>0&&TableWidth>0)
		for(int i=CalculateAllAltitudes?2:0;i<3;i++)
		{
			int idx=(which_texture+1+i)%3;
			ApplySkyKeyframe(current_keyframe+i);
skylight=skyInterface->GetAmbientLight(2.f);
			// only the current pair of lookups, and the ones above and below should be filled in.
			// the altitude index goes from zero to lookups.size()-1.
			for(int j=start_alt;j<end_alt;j++)
			{
				TexArray *tex=lookups[j];
				float complete=(i==2?interp:1.f);
				if(j==alt_index-1)
					complete*=1.f-alt_interp;
				else if(j==alt_index+2)
					complete*=alt_interp;
				unsigned nextFadeIndex=(unsigned)(complete*TableSize);
				if(UseTbb)
				{
					nextFadeIndex/=TableWidth;
					nextFadeIndex*=TableWidth;
				}
				if(nextFadeIndex>TableSize)
					nextFadeIndex=TableSize;
				unsigned nextSkyIndex=(unsigned)(complete*TableHeight);
				if(nextSkyIndex>TableHeight)
					nextSkyIndex=TableHeight;
				unsigned &fadeTexIndex=tex->fadeTexIndex[idx];
				unsigned &skyTexIndex=tex->skyTexIndex[idx];
				if(fadeTexIndex<nextFadeIndex)
				{
					simul::sky::float4 *ptr1=tex->loss_lookup[idx].GetDataPointer();
					simul::sky::float4 *ptr2=tex->inscatter_lookup[idx].GetDataPointer();
					simul::sky::float4 *sky_ptr=tex->sky_lookup[idx].GetDataPointer();
					simul::sky::TextureGenerator::Make2DLossAndInscatterTextures(skyInterface,TableHeight,TableWidth,
						(unsigned char *)ptr1,(unsigned char *)ptr2,(unsigned char *)sky_ptr,
						simul::sky::TextureGenerator::RGBA,tex->altitude,MaxDistanceKm,
						fadeTexIndex,nextFadeIndex-fadeTexIndex,16,EarthTest,UseTbb);
					if(callback)
					{
						FillFadeTextures((int)j,i,fadeTexIndex,nextFadeIndex-fadeTexIndex,(const float*)&(tex->loss_lookup[idx].GetDataPointer()[fadeTexIndex]),
							(const float*)&(tex->inscatter_lookup[idx].GetDataPointer()[fadeTexIndex]));
						if(skyTexIndex<nextSkyIndex)
							callback->FillSkyTexture((int)j,i,skyTexIndex,nextSkyIndex-skyTexIndex,(const float *)(sky_ptr[skyTexIndex]));
					}
					fadeTexIndex=nextFadeIndex;
					skyTexIndex=nextSkyIndex;
				}
			}
		}
		RestoreSky();
	}
	bool changed=false;
	if(interp>=1.f)
	{
		int num_cycles=0;
		while(interp>=1.f)
		{
			current_keyframe++;
			interp=GetInterp(current_keyframe,time);//(time-GetKeyframeTime(current_keyframe))*StepsPerDay;
			num_cycles++;
			which_texture++;
			which_texture=which_texture%3;
		}
		
		if(num_cycles<3)
		{
			//	Texture 3 will be Complete, ok to cycle this in:
			CycleTexturesForward();
			// Now textures 1 and 2 are complete - but texture 3 is blank
			if(num_cycles==2)
			{
			// If we need to cycle in texture 3 now, it must be initialized.
				CycleTexturesForward();
				if(!recalc)
					InitFadeTexture(1);
			}
		}
		else
		{
			recalc=true;
		}
		changed=true;
	}
	if(interp<0.f)
	{
		which_texture--;
		if(which_texture<0)
			which_texture=2;
		CycleTexturesBackward();
		interp=GetInterp(current_keyframe,time);
		if(!recalc&&interp>=0.f)
			InitFadeTexture(0);
		changed=true;
	}
	while(interp<0.f)
	{
		current_keyframe--;
		which_texture--;
		if(which_texture<0)
			which_texture=2;
		interp=GetInterp(current_keyframe,time);
		recalc=true;
	}
	if(changed)
	{
		UseAltitudeKm=AltitudeKM;
		last_alt=AltitudeKM;
		StoreSky();
		ApplySkyKeyframe(current_keyframe+2);
		CalcAutoBrightnessForKeyframe((SkyKeyframe*)this->GetKeyframe(current_keyframe+2));
		ApplySkyKeyframe(current_keyframe+2);
		next_direction_to_sun=skyInterface->GetDirectionToLight();
//ApplySkyKeyframe(current_keyframe+2);
float4 irr=skyInterface->GetLocalIrradiance(58.94f);
float4 skylight=skyInterface->GetAmbientLight(2.f);
		RestoreSky();
		StoreSky();
skylight=skyInterface->GetAmbientLight(2.f);
		RestoreSky();
	}
	if(recalc)
	{
		InitFadeTextures();
	}
	DoInterpolation();
}

void SkyKeyframer::LinkChanged()
{
	if(LinkKeyframeTimeAndDaytime)
	{
		for(unsigned i=0;i<keyframes.size();i++)
		{
			SkyKeyframe *k=(SkyKeyframe*)keyframes[i];
			k->daytime=k->time;
		}
	}
}

void SkyKeyframer::DoInterpolation()
{
	interpolatedKeyframe.time=time;
	if(InterpolateKeyframe(&interpolatedKeyframe))
	{
		skyInterface->SetTime(interpolatedKeyframe.daytime);

		skyInterface->SetHaze(interpolatedKeyframe.Haze);
		skyInterface->SetHazeBaseHeightKm(interpolatedKeyframe.HazeBaseKm);
		skyInterface->SetHazeScaleHeightKm(interpolatedKeyframe.HazeScaleKm);

		skyInterface->SetMieEccentricity(interpolatedKeyframe.HazeEccentricity);

    if (!siderealSkyInterface||!siderealSkyInterface->GetAutomaticSunPosition())
    {
		skyInterface->SetElevationToSunRadians(interpolatedKeyframe.SunElevation);
		skyInterface->SetAzimuthToSunRadians(interpolatedKeyframe.SunAzimuth);
    }

		if(!siderealSkyInterface||!siderealSkyInterface->GetAutomaticMoonPosition())
		{
			skyInterface->SetElevationToMoonRadians(interpolatedKeyframe.MoonElevation);
			skyInterface->SetAzimuthToMoonRadians(interpolatedKeyframe.MoonAzimuth);
		}
		
		if(siderealSkyInterface)
		{
			siderealSkyInterface->SetRayleigh(interpolatedKeyframe.Rayleigh);
			siderealSkyInterface->SetMie(interpolatedKeyframe.Mie);
		}

		float overcast=0.f,overcast_base_km=0.f,overcast_scale_km=1.f;
		if(overcastCallback)
		{
			SkyKeyframe *K1=dynamic_cast<SkyKeyframe *>(GetKeyframe(current_keyframe));
			SkyKeyframe *K2=dynamic_cast<SkyKeyframe *>(GetKeyframe(current_keyframe+1));
			float overcast1=0.f,overcast_base_km1=0.f,overcast_scale_km1=1.f;
			float overcast2=0.f,overcast_base_km2=0.f,overcast_scale_km2=1.f;
			overcastCallback->GetOvercastData(K1->time,overcast1,overcast_base_km1,overcast_scale_km1);
			overcastCallback->GetOvercastData(K2->time,overcast2,overcast_base_km2,overcast_scale_km2);
			
			overcast			=Lerp(interp,overcast1			,overcast2);
			overcast_base_km	=Lerp(interp,overcast_base_km1	,overcast_base_km2);
			overcast_scale_km	=Lerp(interp,overcast_scale_km1	,overcast_scale_km2);
		}
		skyInterface->SetOvercast(overcast);
		skyInterface->SetOvercastBaseKm(overcast_base_km);
		skyInterface->SetOvercastRangeKm(overcast_scale_km);
		// Brightness multipliers:
		static simul::sky::float4 colr(1.f,1.f,1.f,1.f);
		float lf=interpolatedKeyframe.BrightnessAdjustment;
		skyInterface->SetSunIrradiance(colr*25.f*lf);
		dynamic_cast<simul::sky::BaseSky*>(skyInterface)->SetStarlight(simul::sky::float4(1.f,1.f,1.f,1.f)*1e-6f*lf);
	}
	else
	{
		skyInterface->SetElevationToSunRadians(pi/2.f);
		skyInterface->SetAzimuthToSunRadians(0);
		skyInterface->SetOvercast(0.f);
		skyInterface->SetOvercastBaseKm(0.f);
		skyInterface->SetOvercastRangeKm(1.f);
	}
}

void SkyKeyframer::DaytimeToClockTime(float &daytime,int &d,int &h,int &m,int &s,int &ms)
{
	d=(int)daytime;
	if(daytime<0)
		d--;
	float t=daytime-(float)d;
	t*=24.f;
	h=(int)t;
	t-=(float)h;
	t*=60.f;
	m=(int)t;
	t-=(float)m;
	t*=60.f;
	s=(int)t;
	t-=(float)s;
	t*=1000.f;
	ms=(int)t;
}

void SkyKeyframer::CalcAutoBrightnessForKeyframe(SkyKeyframer::SkyKeyframe* K)
{
	if(AutoBrightness)
	{
		StoreSky();
		ApplySkyKeyframe(K);
		static float h=2.f;
		float lf=1.f;
		skyInterface->SetHaze(0.f);
		skyInterface->SetSunIrradiance(simul::sky::float4(1.f,1.f,1.f,1.f)*25.f*lf);
		dynamic_cast<simul::sky::BaseSky*>(skyInterface)->SetStarlight(simul::sky::float4(1.f,1.f,1.f,1.f)*1e-6f*lf);
		float4 sunlight=skyInterface->GetSunlightColour(h)/25.f;
		float sun_bright=.33f*(sunlight.x+sunlight.y+sunlight.z);
		float4 moonlight=skyInterface->GetMoonIrradiance(h)/25.f;
		float moon_bright=.33f*(moonlight.x+moonlight.y+moonlight.z);
		float4 starlight=skyInterface->GetStarlight();
		float star_bright=.33f*(starlight.x+starlight.y+starlight.z);
		float4 skylight=skyInterface->GetAmbientLight(h);
		float sky_bright=.33f*(skylight.x+skylight.y+skylight.z);
		float b=sky_bright+star_bright+1e-5f+moon_bright+sun_bright;
		K->BrightnessAdjustment=1.f+(1.f/b-1.f)*AutoBrightnessStrength;		
		float4 irr=skyInterface->GetLocalIrradiance(58.94f);
		lf=K->BrightnessAdjustment;
		skyInterface->SetSunIrradiance(simul::sky::float4(1.f,1.f,1.f,1.f)*25.f*lf);
		dynamic_cast<simul::sky::BaseSky*>(skyInterface)->SetStarlight(simul::sky::float4(1.f,1.f,1.f,1.f)*1e-6f*lf);

		irr*=K->BrightnessAdjustment;
		skylight*=K->BrightnessAdjustment;
		skylight=skyInterface->GetAmbientLight(h);
		irr=skyInterface->GetLocalIrradiance(58.94f);
		RestoreSky();
	}
}

bool SkyKeyframer::InterpolateKeyframe(SkyKeyframer::SkyKeyframe* K)
{
	float t=DayLoop?FractionalPart(K->time,LoopRange):K->time;
	int k=GetKeyframeAtTime(t);
	SkyKeyframe *K1=dynamic_cast<SkyKeyframe *>(GetKeyframe(k));
	SkyKeyframe *K2=dynamic_cast<SkyKeyframe *>(GetKeyframe(k+1));
	if(!K1||!K2||K1==K||K2==K)
	{
		return false;
	}
	float interp = 0;
	if (K2->time != K1->time) interp = (t - K1->time) / (K2->time - K1->time);
	K->daytime		=Lerp(interp,K1->daytime,K2->daytime);
	K->Haze			=Lerp(interp,K1->Haze,K2->Haze);
	K->HazeBaseKm	=Lerp(interp,K1->HazeBaseKm,K2->HazeBaseKm);
	K->HazeScaleKm	=Lerp(interp,K1->HazeScaleKm,K2->HazeScaleKm);
	K->HazeEccentricity	=Lerp(interp,K1->HazeEccentricity,K2->HazeEccentricity);

	K->Rayleigh		=lerp(interp,K1->Rayleigh,K2->Rayleigh);
	K->Mie			=lerp(interp,K1->Mie,K2->Mie);

	AngleLerp(K->SunAzimuth,K->SunElevation,interp,K1->SunAzimuth,K1->SunElevation,K2->SunAzimuth,K2->SunElevation);
	AngleLerp(K->MoonAzimuth,K->MoonElevation,interp,K1->MoonAzimuth,K1->MoonElevation,K2->MoonAzimuth,K2->MoonElevation);

	float log_b_1=log(K1->BrightnessAdjustment)/log(10.f);
	float log_b_2=log(K2->BrightnessAdjustment)/log(10.f);
	K->BrightnessAdjustment=pow(10.f,Lerp(interp,log_b_1,log_b_2));
	return true;
}

int SkyKeyframer::GetAltIndex(float AltKm) const
{
	if(lookups.size()<2)
		return 0;
	size_t j=0;
	for(j=0;j<lookups.size();j++)
	{
		if(AltKm<lookups[j]->altitude)
			break;
	}
	if(j>0)
		j--;
	if(j>lookups.size()-2)
		j=lookups.size()-2;
	return (int)j;
}

float4 SkyKeyframer::GetIsotropicColourLossFactor(float alt_km,float elevation,float min_dist_km,float max_dist_km,bool b,int i) const
{
	float4 max_loss=GetIsotropicColourLossFactor(alt_km,elevation,max_dist_km,b,i);
	if(min_dist_km==0)
		return max_loss;
	float4 min_loss=GetIsotropicColourLossFactor(alt_km,elevation,min_dist_km,b,i);
	float4 loss=max_loss/min_loss;
	return loss;
}

float4 SkyKeyframer::GetIsotropicInscatterFactor(float alt_km,float elevation,float min_dist_km,float max_dist_km,bool b,int i) const
{
	float4 max_insc=GetIsotropicInscatterFactor(alt_km,elevation,max_dist_km,b,i);
	if(min_dist_km==0)
		return max_insc;
	float4 min_insc=GetIsotropicInscatterFactor(alt_km,elevation,min_dist_km,b,i);
	float4 insc=max_insc-min_insc;
	return insc;
}

float4 SkyKeyframer::GetIsotropicColourLossFactor(float alt_km,float elevation,float dist_km,bool ,int ) const
{
	if(recalc)
		return float4(1,1,1,1);
	float sine=sin(elevation);
	int alt_index=GetAltIndex(alt_km);
	TexArray *tex1=lookups[alt_index];
	float maxd=CalcMaxDistanceKm(alt_km,elevation);
	float4 loss11=tex1->loss_lookup[(which_texture+1)%3].Get(dist_km/maxd,(1.f-sine)*0.5f);
	float4 loss12=tex1->loss_lookup[(which_texture+2)%3].Get(dist_km/maxd,(1.f-sine)*0.5f);
	float4 loss1=lerp(interp,loss11,loss12);
	if(lookups.size()==1)
		return loss1;
	
	TexArray *tex2=lookups[alt_index+1];
	float4 loss21=tex2->loss_lookup[(which_texture+1)%3].Get(dist_km/maxd,(1.f-sine)*0.5f);
	float4 loss22=tex2->loss_lookup[(which_texture+2)%3].Get(dist_km/maxd,(1.f-sine)*0.5f);
	float4 loss2=lerp(interp,loss21,loss22);

	float4 loss=lerp(alt_interp,loss1,loss2);
	return loss;
}

float4 SkyKeyframer::GetIsotropicInscatterFactor(float view_alt_km,float elevation,float dist_km,bool ,int) const
{
elevation=0;

	if(recalc)
		return float4(0,0,0,0);
	float sine=sin(elevation);
	int alt_index=GetAltIndex(view_alt_km);
	TexArray *tex1=lookups[alt_index];
	float maxd=CalcMaxDistanceKm(view_alt_km,elevation);
	float4 insc11=tex1->inscatter_lookup[(which_texture+1)%3].Get(dist_km/maxd,(1.f-sine)*0.5f);
	float4 insc12=tex1->inscatter_lookup[(which_texture+2)%3].Get(dist_km/maxd,(1.f-sine)*0.5f);
	float4 insc1=lerp(interp,insc11,insc12);
	if(lookups.size()==1)
		return insc1;
	TexArray *tex2=lookups[alt_index+1];
	float4 insc21=tex2->inscatter_lookup[(which_texture+1)%3].Get(dist_km/maxd,(1.f-sine)*0.5f);
	float4 insc22=tex2->inscatter_lookup[(which_texture+2)%3].Get(dist_km/maxd,(1.f-sine)*0.5f);
	float4 insc2=lerp(interp,insc21,insc22);
	float4 insc=lerp(alt_interp,insc1,insc2);
	return insc;
}

float4 SkyKeyframer::GetInscatterAngularMultiplier(float cosine,float mie_factor,float alt_km) const
{
	return skyInterface->GetInscatterAngularMultiplier(cosine,mie_factor,alt_km);
}

float SkyKeyframer::GetVisibilityDistance(float view_alt_km)
{
	return skyInterface->GetVisibilityDistance(view_alt_km);
}
float SkyKeyframer::GetHazeForVisibilityDistance(float view_alt_km,float horiz_dist_km)
{
	return skyInterface->GetHazeForVisibilityDistance(view_alt_km,horiz_dist_km);
}


float4 SkyKeyframer::GetIsotropicColourLossFactor(float elevation,float min_dist_km,float max_dist_km) const
{
	return GetIsotropicColourLossFactor(AltitudeKM,elevation,min_dist_km,max_dist_km);
}

float4 SkyKeyframer::GetIsotropicInscatterFactor(float elevation,float min_dist_km,float max_dist_km) const
{
	return GetIsotropicInscatterFactor(AltitudeKM,elevation,min_dist_km,max_dist_km);
}

void SkyKeyframer::Reset()
{
	InitFadeTextures();
}

void SkyKeyframer::SetAltitude(unsigned i,float a)
{
	lookups[i]->altitude=a;
}

float SkyKeyframer::GetAltitude(unsigned i) const
{
	return lookups[i]->altitude;
}

float SkyKeyframer::GetAltitudeTexCoord() const
{
	float scale=lookups.size()-1.f;
	float texc=((float)GetAltitudeIndex()+GetAltitudeInterpolation())/scale;

	if(texc<0)
		texc=0;
	if(texc>1.f)
		texc=1.f;
	return texc;
}

void SkyKeyframer::CalcOnTheFly(TexArray *tex,int idx) const
{
	simul::sky::float4 *ptr1=tex->loss_lookup[idx].GetDataPointer();
	simul::sky::float4 *ptr2=tex->inscatter_lookup[idx].GetDataPointer();
	simul::sky::float4 *sky_ptr=tex->sky_lookup[idx].GetDataPointer();

	unsigned &fadeTexIndex=tex->fadeTexIndex[idx];
	unsigned &skyTexIndex=tex->skyTexIndex[idx];
	if(fadeTexIndex>=TableWidth)
		return;
	if(skyTexIndex>=1)
		return;
	simul::sky::TextureGenerator::Make2DLossAndInscatterTextures(skyInterface,TableHeight,TableWidth,
		(unsigned char *)ptr1,(unsigned char *)ptr2,(unsigned char *)sky_ptr,
		simul::sky::TextureGenerator::RGBA,tex->altitude,MaxDistanceKm,fadeTexIndex,TableWidth-fadeTexIndex,16,EarthTest,UseTbb);

	fadeTexIndex=TableWidth;
	skyTexIndex=1;
}

const float4 &SkyKeyframer::GetAmbientLight(float alt_km) const
{
	static float4 ambient;
	if(recalc)
		return ambient;
	float elevation=pi/2.f;
	float sine=sin(elevation);
	int alt_index=GetAltIndex(alt_km);
	TexArray *tex1=lookups[alt_index];

	int t1=(which_texture+1)%3;
	int t2=(which_texture+2)%3;
	if(tex1->skyTexIndex[t1]<TableHeight)
	{
		CalcOnTheFly(tex1,t1);
	}
	if(tex1->skyTexIndex[t2]<TableHeight)
	{
		CalcOnTheFly(tex1,t2);
	}

	float4 insc11=tex1->sky_lookup[t1].Get((1.f-sine)*0.5f);
	float4 insc12=tex1->sky_lookup[t2].Get((1.f-sine)*0.5f);
	float4 insc1=lerp(interp,insc11,insc12);
	float4 insc;
	if(lookups.size()>1)
	{
		TexArray *tex2=lookups[alt_index+1];
		if(tex2->skyTexIndex[t1]<TableHeight)
		{
			CalcOnTheFly(tex2,t1);
		}
		if(tex2->skyTexIndex[t2]<TableHeight)
		{
			CalcOnTheFly(tex2,t2);
		}

		float4 insc21=tex2->sky_lookup[t1].Get((1.f-sine)*0.5f);
		float4 insc22=tex2->sky_lookup[t2].Get((1.f-sine)*0.5f);
		float4 insc2=lerp(interp,insc21,insc22);
		insc=lerp(alt_interp,insc1,insc2);
	}
	else
	{
		insc=insc1;
	}

	ambient=insc*skyInterface->GetInscatterAngularMultiplier(0,insc.w,alt_km);
	return ambient;
}

void SkyKeyframer::SetOvercastChecksum(SkyKeyframe *K)
{
	float overcast=0.f,overcast_base_km=0.f,overcast_scale_km=1.f;
	if(overcastCallback)
		overcastCallback->GetOvercastData(K->time,overcast,overcast_base_km,overcast_scale_km);
	skyInterface->SetOvercast(overcast);
	skyInterface->SetOvercastBaseKm(overcast_base_km);
	skyInterface->SetOvercastRangeKm(overcast_scale_km);
	K->OvercastChecksum =*reinterpret_cast<const unsigned*>(&overcast);
	K->OvercastChecksum+=*reinterpret_cast<const unsigned*>(&overcast_base_km);
	K->OvercastChecksum+=*reinterpret_cast<const unsigned*>(&overcast_scale_km);
}

void SkyKeyframer::ApplySkyKeyframe(SkyKeyframe *K)
{
	skyInterface->SetTime(FractionalPart(K->daytime,LoopRange));


	if(!siderealSkyInterface||!siderealSkyInterface->GetAutomaticSunPosition())
	{
		skyInterface->SetElevationToSunRadians(K->SunElevation);
		skyInterface->SetAzimuthToSunRadians(K->SunAzimuth);
	}
	if(!siderealSkyInterface||!siderealSkyInterface->GetAutomaticMoonPosition())
	{
		skyInterface->SetElevationToMoonRadians(K->MoonElevation);
		skyInterface->SetAzimuthToMoonRadians(K->MoonAzimuth);
	}
	skyInterface->SetHaze(K->Haze);
	skyInterface->SetHazeBaseHeightKm(K->HazeBaseKm);
	skyInterface->SetHazeScaleHeightKm(K->HazeScaleKm);
	
	skyInterface->SetMieEccentricity(K->HazeEccentricity);

	if(siderealSkyInterface)
	{
		siderealSkyInterface->SetRayleigh(K->Rayleigh);
		siderealSkyInterface->SetMie(K->Mie);
	}
	SetOvercastChecksum(K);
	// Brightness multipliers:
	float lf=K->BrightnessAdjustment;
	skyInterface->SetSunIrradiance(simul::sky::float4(1.f,1.f,1.f,1.f)*25.f*lf);
	dynamic_cast<simul::sky::BaseSky*>(skyInterface)->SetStarlight(simul::sky::float4(1.f,1.f,1.f,1.f)*1e-6f*lf);
}

void SkyKeyframer::ApplySkyKeyframe(int i)
{
	if(!keyframes.size())
		return;
	if(i>=(int)keyframes.size())
		i=i%(int)keyframes.size();
	while(i<0)
		i+=(int)keyframes.size();
	SkyKeyframe *K=dynamic_cast<SkyKeyframe *>(keyframes[i]);
	ApplySkyKeyframe(K);
}

void SkyKeyframer::StoreSky()
{
	old_dt.push(time);
	old_SunElevation.push(skyInterface->GetElevationToSunRadians());
	old_SunAzimuth.push(skyInterface->GetAzimuthToSunRadians());
	old_MoonElevation.push(skyInterface->GetElevationToMoonRadians());
	old_MoonAzimuth.push(skyInterface->GetAzimuthToMoonRadians());
	if(siderealSkyInterface)
		old_doy.push(siderealSkyInterface->GetDayOfTheYear());
	else
		old_doy.push(0);
}

void SkyKeyframer::RestoreSky()
{
	if(siderealSkyInterface)
		siderealSkyInterface->SetDayOfTheYear(old_doy.top());
	old_doy.pop();
	ApplySkyKeyframe(&interpolatedKeyframe);
	skyInterface->SetElevationToSunRadians	(old_SunElevation.top());
	old_SunElevation.pop();
	skyInterface->SetAzimuthToSunRadians	(old_SunAzimuth.top());
	old_SunAzimuth.pop();
	skyInterface->SetElevationToMoonRadians	(old_MoonElevation.top());
	old_MoonElevation.pop();
	skyInterface->SetAzimuthToMoonRadians	(old_MoonAzimuth.top());
	old_MoonAzimuth.pop();
	SetDaytime(old_dt.top());
	old_dt.pop();
}

SkyTexturesCallback *SkyKeyframer::GetCallback()
{
	return callback;
}

void SkyKeyframer::SetUniformKeyframes(int StepsPerDay,float range)
{
	keyframes.clear();
	float t=0.f;
	LoopRange=range;
	float dt=range/(float)StepsPerDay;
	int kf=current_keyframe;
	current_keyframe=-100000;
	for(int i=0;i<StepsPerDay;i++)
	{
		SkyKeyframe *K=dynamic_cast<SkyKeyframe *>(InsertKeyframe(t));
		if(LinkKeyframeTimeAndDaytime)
			K->daytime=t;
		else
			K->daytime=0.5f;
		K->UseMoon=false;
		K->SunAzimuth=-pi/2.f;
		K->MoonAzimuth=-pi/2.f;
		K->SunElevation=pi*(-0.5f+2.f*(float)i/(float)StepsPerDay);
		K->MoonElevation=K->SunElevation+pi;
		if(K->SunElevation>pi/2.f)
		{
			K->SunAzimuth=pi/2.f;
			K->SunElevation=pi-K->SunElevation;
		}
		if(K->MoonElevation>pi/2.f)
		{
			K->MoonAzimuth=pi/2.f;
			K->MoonElevation=pi-K->MoonElevation;
		}
		t+=dt;
	}
	current_keyframe=kf;
}

float SkyKeyframer::GetHazeForVisibilityDistance(int keyframe_index,float alt_km,float dist_km)
{
	StoreSky();
	ApplySkyKeyframe(keyframe_index);
	float ret=skyInterface->GetHazeForVisibilityDistance(alt_km,dist_km);
	RestoreSky();
	return ret;
}

float SkyKeyframer::GetVisibilityDistance(int keyframe_index,float alt_km)
{
	StoreSky();
	ApplySkyKeyframe(keyframe_index);
	float ret=skyInterface->GetVisibilityDistance(alt_km);
	RestoreSky();
	return ret;
}

void SkyKeyframer::ChangedTbb()
{
	#ifndef _DLL
	if(UseTbb)
	{
		UseTbb=false;
#ifndef XENON
		std::cerr<<"SkyKeyframer::ChangedTbb - Can't use Threading Building Blocks with Static Runtimes (MT, MTd)"<<std::endl;
#endif
	}
	#endif
#ifdef _MSC_VER
	if(UseTbb)
	{
		// Is 
		HMODULE hModule=LoadLibrary("tbb.dll");
		if(!hModule)
		{
#ifndef XENON
			std::cerr<<"SkyKeyframer::ChangedTbb - Can't use Threading Building Blocks as the tbb.dll is not found on the DLL search path."<<std::endl;
#endif
			UseTbb=false;
			return;
		}
	}
#endif
}

bool SkyKeyframer::GetAutomaticSunPosition() const
{
	if(siderealSkyInterface)
		return siderealSkyInterface->GetAutomaticSunPosition();
	else
		return false;
}

bool SkyKeyframer::GetAutomaticMoonPosition() const
{
	if(siderealSkyInterface)
		return siderealSkyInterface->GetAutomaticMoonPosition();
	else
		return false;
}

void SkyKeyframer::SetAutomaticSunPosition(bool val)
{
	float t=GetDaytime();
	for(int i=0;i<this->GetNumKeyframes();i++)
	{
		SkyKeyframe *K=dynamic_cast<SkyKeyframe *>(GetKeyframe(i));
		SetDaytime(K->daytime);
		if(!siderealSkyInterface||!siderealSkyInterface->GetAutomaticSunPosition())
		{
		}
		else
		{
			K->SunElevation=skyInterface->GetElevationToSunRadians();
			K->SunAzimuth	=skyInterface->GetAzimuthToSunRadians();
		}
	}
	SetDaytime(t);
	if(siderealSkyInterface)
		siderealSkyInterface->SetAutomaticSunPosition(val);
}

void SkyKeyframer::SetAutomaticMoonPosition(bool val)
{
	float t=GetDaytime();
	for(int i=0;i<this->GetNumKeyframes();i++)
	{
		SkyKeyframe *K=dynamic_cast<SkyKeyframe *>(GetKeyframe(i));
		SetDaytime(K->daytime);
		if(!siderealSkyInterface||!siderealSkyInterface->GetAutomaticMoonPosition())
		{
		}
		else
		{
			K->MoonElevation=skyInterface->GetElevationToMoonRadians();
			K->MoonAzimuth	=skyInterface->GetAzimuthToMoonRadians();
		}
	}
	SetDaytime(t);
	if(siderealSkyInterface)
		siderealSkyInterface->SetAutomaticMoonPosition(val);
}

static const unsigned ALTITUDEFADETABLE_MAGIC=0x917F9A93;

std::ostream &SkyKeyframer::Save(std::ostream &os) const
{
	BaseKeyframer::Save(os);
	os.write((const char*)&ALTITUDEFADETABLE_MAGIC,sizeof(ALTITUDEFADETABLE_MAGIC));

	unsigned tell_1=(unsigned)os.tellp();
	unsigned chunk_size=0;
	os.write((const char*)&chunk_size,sizeof(chunk_size));

	char c=0;
	if(siderealSkyInterface)
		c=siderealSkyInterface->GetAutomaticSunPosition()?1:0;
	os.write((const char*)&c,sizeof(c));
	if(siderealSkyInterface)
		c=siderealSkyInterface->GetAutomaticMoonPosition()?1:0;
	os.write((const char*)&c,sizeof(c));

	float val=0.f;
	if(siderealSkyInterface)
		val=siderealSkyInterface->GetLatitudeRadians();
	os.write((const char*)&val,sizeof(val));
	if(siderealSkyInterface)
		val=siderealSkyInterface->GetLongitudeRadians();
	os.write((const char*)&val,sizeof(val));
	if(siderealSkyInterface)
		val=siderealSkyInterface->GetTimezone();
	os.write((const char*)&val,sizeof(val));

	int y,m,d;
	Sky::GetDateFromDayNumber(StartDayNumber,y,m,d);
	int day_of_the_year=StartDayNumber-Sky::GetDayNumber(y,1,1);
	
	os.write((const char*)&y,sizeof(y));

	os.write((const char*)&day_of_the_year,sizeof(day_of_the_year));

	os.write((const char*)&MaxDistanceKm,sizeof(MaxDistanceKm));

	unsigned numalt=this->GetNumAltitudes();
	os.write((const char*)&numalt,sizeof(numalt));
	unsigned numel=this->GetTableHeight();
	os.write((const char*)&numel,sizeof(numel));
	unsigned numdis=this->GetTableWidth();
	os.write((const char*)&numdis,sizeof(numdis));

	c=GetLinkKeyframeTimeAndDaytime()?1:0;
	os.write((const char*)&c,sizeof(c));

	c=GetAutoBrightness()?1:0;
	os.write((const char*)&c,sizeof(c));

	os.write((const char*)&time,sizeof(time));

	os.write((const char*)&MaxAltitudeKm,sizeof(MaxAltitudeKm));
	
	os.write((const char*)&MaxStarMagnitude,sizeof(MaxStarMagnitude));
	os.write((const char*)&StarBrightness,sizeof(StarBrightness));

	unsigned tell_2=(unsigned)os.tellp();
	chunk_size=tell_2-tell_1;
	os.seekp(tell_1);
	os.write((const char*)&chunk_size,sizeof(chunk_size));
	os.seekp(tell_2);

	return os;
}
#define TELL_CHECK {						\
	unsigned tell_2=(unsigned)is.tellg();	\
	if(tell_2>tell_2_required)				\
	{										\
		std::cerr<<"SkyKeyframer::Load - Position mismatch in seq file."<<" expected pos "<<tell_2_required<<" got pos "<<tell_2<<std::endl;	\
		is.seekg(tell_2_required);			\
		return is;							\
	}										\
}

std::istream &SkyKeyframer::Load(std::istream &is)
{
	SetLinkKeyframeTimeAndDaytime(true);
	LinkChanged();

	BaseKeyframer::Load(is);
	unsigned magic;
	is.read((char*)&magic,sizeof(magic));
	if(magic!=ALTITUDEFADETABLE_MAGIC)
	{
#ifndef XENON
		std::cerr<<"ColourSkyKeyframer::ColourKeyframe::Load - not found magic number in file - possibly out of date file."<<std::endl;
#endif
		return is;
	}
// Where are we now, and how big is the data chunk?
	unsigned tell_1=(unsigned)is.tellg();
	unsigned chunk_size=0;
	is.read((char*)&chunk_size,sizeof(chunk_size));
	unsigned tell_2_required=tell_1+chunk_size;
	char c;
	is.read((char*)&c,sizeof(c));
	if(siderealSkyInterface)
		siderealSkyInterface->SetAutomaticSunPosition(c!=0);
	is.read((char*)&c,sizeof(c));
	if(siderealSkyInterface)
		siderealSkyInterface->SetAutomaticMoonPosition(c!=0);
	TELL_CHECK;
	float val=0.f;
	is.read((char*)&val,sizeof(val));
	if(siderealSkyInterface)
		siderealSkyInterface->SetLatitudeRadians(val);
	TELL_CHECK;
	is.read((char*)&val,sizeof(val));
	if(siderealSkyInterface)
		siderealSkyInterface->SetLongitudeRadians(val);
	TELL_CHECK;
	is.read((char*)&val,sizeof(val));
	if(siderealSkyInterface)
		siderealSkyInterface->SetTimezone(val);
	TELL_CHECK;
	int y,d;
	is.read((char*)&y,sizeof(y));
	TELL_CHECK;
	is.read((char*)&d,sizeof(d));
	StartDayNumber=Sky::GetDayNumber(y,1,1)+d;
	TELL_CHECK;
	is.read((char*)&MaxDistanceKm,sizeof(MaxDistanceKm));
	TELL_CHECK;
	unsigned numalt=8;
	is.read((char*)&numalt,sizeof(numalt));
	SetNumAltitudes(numalt);
	unsigned numel=64;
	TELL_CHECK;
	is.read((char*)&numel,sizeof(numel));
	this->SetTableHeight(numel);
	unsigned numdis=32;
	TELL_CHECK;
	is.read((char*)&numdis,sizeof(numdis));
	this->SetTableWidth(numdis);
	TELL_CHECK;
	is.read((char*)&c,sizeof(c));
	SetLinkKeyframeTimeAndDaytime(c!=0);
	LinkChanged();

	TELL_CHECK;
	is.read((char*)&c,sizeof(c));
	SetAutoBrightness(c!=0);

	TELL_CHECK;
	is.read((char*)&val,sizeof(val));
	SetTime(val);

	TELL_CHECK;
	is.read((char*)&val,sizeof(val));
	SetMaxAltitudeKm(val);

	TELL_CHECK;
	is.read((char*)&MaxStarMagnitude,sizeof(MaxStarMagnitude));

	TELL_CHECK;
	is.read((char*)&StarBrightness,sizeof(StarBrightness));
	
	stars.UseDefaultStars(MaxStarMagnitude);

// Are we in the expected place?
	unsigned tell_2=(unsigned)is.tellg();
	if(tell_2!=tell_2_required)
		is.seekg(tell_2_required);

	return is;
}

// Clear the keyframe sequence:
void SkyKeyframer::New()
{
	BaseKeyframer::ClearKeyframes();
	interpolatedKeyframe=SkyKeyframe();
	if(siderealSkyInterface)
		siderealSkyInterface->SetAutomaticSunPosition(true);
}


BaseKeyframe *SkyKeyframer::InsertKeyframe(float t)
{
	int i=GetKeyframeAtTime(t);
	
	SkyKeyframe* K=NULL;
	// This is the keyframe to the left of t. We want to INSERT at the next point so:
	if(i>0||!keyframes.size()||keyframes[0]->time<=t)
		i++;
	K=new SkyKeyframe();
	if(i>=(int)keyframes.size())
	{
		i=(int)keyframes.size();
		keyframes.push_back(K);
		K->BrightnessAdjustment=1.f;
	}
	else
	{
		if(i<0)
			i=0;
		keyframes.insert(keyframes.begin()+i,K);
	}
	K=(SkyKeyframe*)keyframes[i];
	K->time=t;
	if(this->LinkKeyframeTimeAndDaytime)
		K->daytime=t;
	CalcAutoBrightnessForKeyframe(K);
	InterpolateKeyframe(K);
	// Does this new keyframe affect us?
	if(current_keyframe>i)
		current_keyframe++;
	if(i>=current_keyframe&&i<=current_keyframe+2)
	{
		if(skyInterface)
			interp=GetInterp(current_keyframe,skyInterface->GetTime());
		InitFadeTextures();
	}
	sorted=false;
	return keyframes[i];
}

float4 SkyKeyframer::GetKeyframeHorizonColour(int k)
{
	StoreSky();
	ApplySkyKeyframe(k);
	SkyKeyframe *K=dynamic_cast<SkyKeyframe *>(GetKeyframe(k));
	float4 ret=skyInterface->GetSkyColour(float4(1.f,0,0,0),0)*K->BrightnessAdjustment;
	RestoreSky();
	return ret;
}

float4 SkyKeyframer::GetKeyframeZenithColour(int k)
{
	StoreSky();
	ApplySkyKeyframe(k);
	SkyKeyframe *K=dynamic_cast<SkyKeyframe *>(GetKeyframe(k));
	float4 ret=skyInterface->GetSkyColour(float4(0,0,1.f,0),0)*K->BrightnessAdjustment;
	RestoreSky();
	return ret;
}

float4 SkyKeyframer::GetKeyframeSunlightColour(int k)
{
	StoreSky();
	ApplySkyKeyframe(k);
	SkyKeyframe *K=dynamic_cast<SkyKeyframe *>(GetKeyframe(k));
	float4 ret=skyInterface->GetLocalIrradiance(0)*K->BrightnessAdjustment;
	RestoreSky();
	return ret;
}

void SkyKeyframer::CycleTexturesForward()
{
	if(callback)
		callback->CycleTexturesForward();
	for(int j=0;j<(int)lookups.size();j++)
	{
		TexArray *tex=lookups[j];
		tex->fadeTexIndex[which_texture]=0;
		tex->skyTexIndex[which_texture]=0;
	}
	BaseKeyframe *K3=GetKeyframe(current_keyframe+2);
	SetOvercastChecksum(static_cast<SkyKeyframe *>(K3));
	unsigned glob=GetGlobalChecksum();
	K3->checksum=glob+K3->GetPropertiesChecksum();
}

void SkyKeyframer::CycleTexturesBackward()
{
	if(callback)
	{
		callback->CycleTexturesForward();
		callback->CycleTexturesForward();
	}
	for(int j=0;j<(int)lookups.size();j++)
	{
		TexArray *tex=lookups[j];
		tex->fadeTexIndex[(which_texture+1)%3]=0;
		tex->skyTexIndex[(which_texture+1)%3]=0;
	}
	current_keyframe--;
	BaseKeyframe *K1=GetKeyframe(current_keyframe);

	SetOvercastChecksum(static_cast<SkyKeyframe *>(K1));
	unsigned glob=GetGlobalChecksum();
	K1->checksum=glob+K1->GetPropertiesChecksum();
}

void SkyKeyframer::StarsChanged()
{
	stars.UseDefaultStars(MaxStarMagnitude);
}
// Magic numbers for file format:
static const unsigned KEYFRAME_MAGIC=0xB1B26A22;
std::ostream &SkyKeyframer::SkyKeyframe::Save(std::ostream &os) const
{
	BaseKeyframe::Save(os);
	os.write((const char*)&KEYFRAME_MAGIC,sizeof(KEYFRAME_MAGIC));

	unsigned tell_1=(unsigned)os.tellp();
	unsigned chunk_size=0;
	os.write((const char*)&chunk_size,sizeof(chunk_size));

	os.write((const char*)&Rayleigh,sizeof(Rayleigh));
	os.write((const char*)&Haze,sizeof(Haze));
	os.write((const char*)&HazeBaseKm,sizeof(HazeBaseKm));
	os.write((const char*)&HazeScaleKm,sizeof(HazeScaleKm));

	os.write((const char*)&SunAzimuth,sizeof(SunAzimuth));
	os.write((const char*)&SunElevation,sizeof(SunElevation));
	os.write((const char*)&MoonAzimuth,sizeof(MoonAzimuth));
	os.write((const char*)&MoonElevation,sizeof(MoonElevation));
	char c=UseMoon?1:0;
	os.write((const char*)&c,sizeof(c));
	os.write((const char*)&BrightnessAdjustment,sizeof(BrightnessAdjustment));

	os.write((const char*)&Mie,sizeof(Mie));

	os.write((const char*)&HazeEccentricity,sizeof(HazeEccentricity));
	os.write((const char*)&daytime,sizeof(daytime));

	unsigned tell_2=(unsigned)os.tellp();
	chunk_size=tell_2-tell_1;
	os.seekp(tell_1);
	os.write((const char*)&chunk_size,sizeof(chunk_size));
	os.seekp(tell_2);

	return os;
}

std::istream &SkyKeyframer::SkyKeyframe::Load(std::istream &is)
{
	BaseKeyframe::Load(is);
	unsigned magic;
	is.read((char*)&magic,sizeof(magic));
	if(magic!=KEYFRAME_MAGIC)
	{
#ifndef XENON
		std::cerr<<"ColourSkyKeyframer::ColourKeyframe::Load - not found magic number in file - possibly out of date file."<<std::endl;
#endif
		return is;
	}
	unsigned tell_1=(unsigned)is.tellg();
	unsigned chunk_size=0;
	is.read((char*)&chunk_size,sizeof(chunk_size));
	unsigned tell_2_required=tell_1+chunk_size;

	is.read((char*)&Rayleigh,sizeof(Rayleigh));
	TELL_CHECK;
	is.read((char*)&Haze,sizeof(Haze));
	TELL_CHECK;
	is.read((char*)&HazeBaseKm,sizeof(HazeBaseKm));
	TELL_CHECK;
	is.read((char*)&HazeScaleKm,sizeof(HazeScaleKm));
	TELL_CHECK;

	is.read((char*)&SunAzimuth,sizeof(SunAzimuth));
	TELL_CHECK;
	is.read((char*)&SunElevation,sizeof(SunElevation));
	TELL_CHECK;
	is.read((char*)&MoonAzimuth,sizeof(MoonAzimuth));
	TELL_CHECK;
	is.read((char*)&MoonElevation,sizeof(MoonElevation));
	TELL_CHECK;
	
	char c;
	is.read((char*)&c,sizeof(c));
	UseMoon=(c!=0);
	TELL_CHECK;
	is.read((char*)&BrightnessAdjustment,sizeof(BrightnessAdjustment));
	TELL_CHECK;
	is.read((char*)&Mie,sizeof(Mie));

	TELL_CHECK;
	is.read((char*)&HazeEccentricity,sizeof(HazeEccentricity));
	TELL_CHECK;
	is.read((char*)&daytime,sizeof(daytime));

	unsigned tell_2=(unsigned)is.tellg();
	if(tell_2!=tell_2_required)
		is.seekg(tell_2_required);
	return is;
}

bool SkyKeyframer::SkyKeyframe::HasValue(const char *name) const
{
	if(_stricmp(name,"haze")==0)
		return true;
	else if(_stricmp(name,"daytime")==0)
		return true;
	else if(_stricmp(name,"HazeBaseKm")==0)
		return true;
	else if(_stricmp(name,"HazeScaleKm")==0)
		return true;
	else if(_stricmp(name,"HazeEccentricity")==0)
		return true;
	else if(_stricmp(name,"SunElevation")==0)
		return true;
	else if(_stricmp(name,"MoonElevation")==0)
		return true;
	else if(_stricmp(name,"SunAzimuth")==0)
		return true;
	else if(_stricmp(name,"MoonAzimuth")==0)
		return true;
	else if(_stricmp(name,"RayleighRed")==0)
		return true;
	else if(_stricmp(name,"RayleighGreen")==0)
		return true;
	else if(_stricmp(name,"RayleighBlue")==0)
		return true;
	else if(_stricmp(name,"MieRed")==0)
		return true;
	else if(_stricmp(name,"MieGreen")==0)
		return true;
	else if(_stricmp(name,"MieBlue")==0)
		return true;
	else if(_stricmp(name,"BrightnessAdjustment")==0)
		return true;
	else if(_stricmp(name,"UseMoon")==0)
		return true;
	return BaseKeyframe::HasValue(name);
}

float SkyKeyframer::SkyKeyframe::GetValue(const char *name) const
{
	if(_stricmp(name,"haze")==0)
		return Haze;
	else if(_stricmp(name,"daytime")==0)
		return daytime;
	else if(_stricmp(name,"HazeBaseKm")==0)
		return HazeBaseKm;
	else if(_stricmp(name,"HazeScaleKm")==0)
		return HazeScaleKm;
	else if(_stricmp(name,"HazeEccentricity")==0)
		return HazeEccentricity;
	else if(_stricmp(name,"SunElevation")==0)
		return SunElevation;
	else if(_stricmp(name,"MoonElevation")==0)
		return MoonElevation;
	else if(_stricmp(name,"SunAzimuth")==0)
		return SunAzimuth;
	else if(_stricmp(name,"MoonAzimuth")==0)
		return MoonAzimuth;
	else if(_stricmp(name,"RayleighRed")==0)
		return Rayleigh.x*1000.f;
	else if(_stricmp(name,"RayleighGreen")==0)
		return Rayleigh.y*1000.f;
	else if(_stricmp(name,"RayleighBlue")==0)
		return Rayleigh.z*1000.f;
	else if(_stricmp(name,"MieRed")==0)
		return Mie.x*1000.f;
	else if(_stricmp(name,"MieGreen")==0)
		return Mie.y*1000.f;
	else if(_stricmp(name,"MieBlue")==0)
		return Mie.z*1000.f;
	else if(_stricmp(name,"BrightnessAdjustment")==0)
		return BrightnessAdjustment;
	return BaseKeyframe::GetValue(name);
}

void SkyKeyframer::SkyKeyframe::SetValue(const char *name,float val)
{
	if(_stricmp(name,"haze")==0)
		Haze=val;
	else if(_stricmp(name,"daytime")==0)
		daytime=val;
	else if(_stricmp(name,"HazeBaseKm")==0)
		HazeBaseKm=val;
	else if(_stricmp(name,"HazeScaleKm")==0)
		HazeScaleKm=val;
	else if(_stricmp(name,"HazeEccentricity")==0)
		HazeEccentricity=val;
	else if(_stricmp(name,"SunElevation")==0)
		SunElevation=val;
	else if(_stricmp(name,"MoonElevation")==0)
		MoonElevation=val;
	else if(_stricmp(name,"SunAzimuth")==0)
		SunAzimuth=val;
	else if(_stricmp(name,"MoonAzimuth")==0)
		MoonAzimuth=val;
	else if(_stricmp(name,"RayleighRed")==0)
		Rayleigh.x=0.001f*val;
	else if(_stricmp(name,"RayleighGreen")==0)
		Rayleigh.y=0.001f*val;
	else if(_stricmp(name,"RayleighBlue")==0)
		Rayleigh.z=0.001f*val;
	else if(_stricmp(name,"MieRed")==0)
		Mie.x=0.001f*val;
	else if(_stricmp(name,"MieGreen")==0)
		Mie.y=0.001f*val;
	else if(_stricmp(name,"MieBlue")==0)
		Mie.z=0.001f*val;
	else if(_stricmp(name,"BrightnessAdjustment")==0)
		BrightnessAdjustment=val;
	BaseKeyframe::SetValue(name,val);
}

int SkyKeyframer::SkyKeyframe::GetInt(const char *name) const
{
	if(_stricmp(name,"UseMoon")==0)
		return UseMoon?1:0;
	return BaseKeyframe::GetInt(name);
}

void SkyKeyframer::SkyKeyframe::SetInt(const char *name,int val)
{
	if(_stricmp(name,"UseMoon")==0)
		UseMoon=(val!=0);
	BaseKeyframe::SetInt(name,val);
}


SkyKeyframer::SkyKeyframe *SkyKeyframer::GetNextModifiableKeyframe()
{
	int k=current_keyframe+3;
	if(!DayLoop&&k>=this->GetNumKeyframes())
		return NULL;
	return static_cast<SkyKeyframer::SkyKeyframe *>(GetKeyframe(k));
}



unsigned SkyKeyframer::SkyKeyframe::GetPropertiesChecksum() const
{
	unsigned new_checksum=0;
	new_checksum+=*reinterpret_cast<const unsigned*>(&Rayleigh.x			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Rayleigh.y			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Rayleigh.z			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Rayleigh.w			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Mie.x					);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Mie.y					);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Mie.z					);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Mie.w					);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Haze					);
	new_checksum+=*reinterpret_cast<const unsigned*>(&HazeBaseKm			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&HazeScaleKm			);
	new_checksum+=*reinterpret_cast<const unsigned*>(&BrightnessAdjustment	);
	new_checksum+=*reinterpret_cast<const unsigned*>(&OvercastChecksum		);
	new_checksum+=UseMoon?1:0;

	float t=sin(SunElevation);
	new_checksum+=*reinterpret_cast<const unsigned*>(&t		);
	t=sin(SunAzimuth);
	new_checksum+=*reinterpret_cast<const unsigned*>(&t		);
	t=sin(MoonElevation);
	new_checksum+=*reinterpret_cast<const unsigned*>(&t		);
	t=sin(MoonAzimuth);
	new_checksum+=*reinterpret_cast<const unsigned*>(&t		);
	return new_checksum;
}

void SkyKeyframer::SetSunOrMoon(bool use, float value)
{
  useSunOrMoon = use;
  sunOrMoon = value;
}
