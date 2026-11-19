#include <algorithm>
#include "precomp.h"

#include "Simul/Sky/BaseSky.h"
#include "Simul/Math/Pi.h"
#include <math.h>
using namespace simul;
using namespace sky;

BaseSky::BaseSky()
	:Time(0.f)
	,DirectionToSun(0,0,1.f,0)
	,AzimuthToSunRadians(0)
	,ElevationToSunRadians(0)
	,Overcast(0)
	,OvercastBaseKm(0)
	,OvercastRangeKm(1.f)
	,MieEccentricity(0.8f)
	,PlanetRadius(6378.f)
	,SunIrradiance(25.f,25.f,25.f,1.f)//26.7f,27.8f,30.f,0)
	,SunRadiusArcMinutes(16.f)
	,MoonRadiusArcMinutes(16.f)
	,AtmosphereThickness(64.f)
	,HazeScaleHeightKm(0.5f)
	,HazeBaseHeightKm(0.5f)
{
	use_haze_scale_km=HazeScaleHeightKm;
	use_haze_base_km=HazeBaseHeightKm;
}

BaseSky::~BaseSky()
{
}

float4 BaseSky::GetSunlightFactor(float alt_km) const
{
	float4 Pos(0,0,alt_km,0);
	return GetSunlightFactor(Pos,DirectionToSun);
}


float4 BaseSky::GetSunlightFactor(float alt_km,const float4 &DirectionToLight) const
{
	float4 Pos(0,0,alt_km,0);
	return GetSunlightFactor(Pos,DirectionToLight);
}

float4 BaseSky::GetSunlightFactor(const float4 &Pos) const
{
	return GetSunlightFactor(Pos,DirectionToSun);
}

// Get a single colour for the average sunlight illuminating the ray pointing
// in the given azimuth and elevation
float4 BaseSky::GetSunlightColour(const float4 &Pos) const
{
	float4 ret=GetSunlightFactor(Pos);
	float w=ret.w;
	ret=ret*SunIrradiance;
	ret.w=w;
	return ret;
}

float4 BaseSky::GetSunlightColour(float alt_km) const
{
	return GetSunlightFactor(float4(0,0,alt_km,0))*SunIrradiance;
}

float4 BaseSky::GetMoonIrradiance(float alt_km) const
{
	float moon_angular_radius=pi/180.f/2.f;
	float moon_angular_area_ratio=pi*moon_angular_radius*moon_angular_radius/(4.f*pi);
	return GetSunlightFactor(alt_km,DirectionToMoon)*SunIrradiance*0.136f*moon_angular_area_ratio * GetMoonIrradianceCoef();
}

float BaseSky::GetVisibilityDistance(float )
{
	return 0.f;
}

float BaseSky::GetHazeForVisibilityDistance(float ,float )
{
	return 0.f;
}

void BaseSky::TimeChanged()
{
}

float BaseSky::GetHazeAtAltitude(float alt_km) const
{
	return Haze*GetHazeFactorAtAltitude(alt_km);
}

float BaseSky::GetHazeFactorAtAltitude(float alt_km) const
{
	if(alt_km<0)
		alt_km=0;
	float val=exp((use_haze_base_km-alt_km)/HazeScaleHeightKm);
	val=std::min(val,1.f);
	return val/HazeScaleHeightKm;
}

const float4 &BaseSky::GetDirectionToLight() const
{
	return DirectionToSun;
}

float BaseSky::GetOvercastAtAltitude(float h_km) const
{
	if(h_km<OvercastBaseKm)
		return Overcast;
	if(h_km>OvercastBaseKm+OvercastRangeKm)
		return 0.f;
	return Overcast*(h_km-OvercastBaseKm)/OvercastRangeKm;
}

float BaseSky::GetDistanceToSpace(float Elevation,float h_km) const
{
	// float mean_alt=0;
	float distance_to_edge=0;
	// how far to the edge of the atmosphere?
	// RH^2 = d^2 +Rh^2 - 2 d Rh cos(90+e)
	// d=1/2 (-b + _/b^2-4ac )
	float RH=PlanetRadius+AtmosphereThickness;
	float Rh=PlanetRadius+h_km;
	float cosine=cos(pi/2.f+Elevation);
	float b=-2*Rh*cosine;
	float c=Rh*Rh-RH*RH;
	distance_to_edge=0.5f*(-b+sqrt(b*b-4*c));
	// this is the distance to the atmosphere.
	return distance_to_edge;
}

float BaseSky::GetOvercastAtAltitudeRange(float alt1_km,float alt2_km) const
{
	if(alt1_km>=OvercastBaseKm+OvercastRangeKm)
		return 0.f;
	if(alt2_km<=alt1_km)
		std::swap(alt1_km,alt2_km);
	if(alt2_km<=OvercastBaseKm)
		return Overcast;
	if(alt1_km==alt2_km)
		return GetOvercastAtAltitude(alt1_km);
	float diff_km=alt2_km-alt1_km;
	// 4 cases:
	if(alt1_km<OvercastBaseKm)
	{
	//		1 - alt1<base, alt2 in between:
		if(alt2_km<OvercastBaseKm+OvercastRangeKm)
		{
			float proportion=(OvercastBaseKm-alt1_km)/diff_km;
			return proportion*Overcast+(1.f-proportion)*GetOvercastAtAltitude(0.5f*(OvercastBaseKm+alt2_km));
		}
	//		1 - alt1<base, alt2>highest:
		else
		{
			float proportion1=(OvercastBaseKm-alt1_km)/diff_km;
			float proportion2=OvercastRangeKm/diff_km;
			return proportion1*Overcast+proportion2*GetOvercastAtAltitude(OvercastBaseKm+0.5f*OvercastRangeKm);
		}
	}
	//		3 - alt1 in between, alt2 in between
	else if(alt2_km<OvercastBaseKm+OvercastRangeKm)
	{
		return GetOvercastAtAltitude(0.5f*(alt1_km+alt2_km));
	}
	//		4 - alt1 in between, alt2 above.
	else
	{
		float proportion=(alt2_km-(OvercastBaseKm+OvercastRangeKm))/diff_km;
		return (1.f-proportion)*GetOvercastAtAltitude(0.5f*(alt1_km+OvercastBaseKm+OvercastRangeKm));
	}
}

float4 BaseSky::AzimuthElevationToDirection(float az,float el)
{
	float4 dir;
	dir.x=sin(az)*cos(el);
	dir.y=cos(az)*cos(el);
	dir.z=sin(el);
	dir.w=0;
	return dir;
}

void BaseSky::SetSunPosByDirection()
{
	float l=simul::sky::length(DirectionToSun);
	if(l)
		DirectionToSun=DirectionToSun/l;
	else
		DirectionToSun.Set(0,0,1.f,0);

	ElevationToSunRadians=asin(DirectionToSun.z);
	AzimuthToSunRadians=atan2(DirectionToSun.x,DirectionToSun.y);
	UpdateSunPosition();
}

void BaseSky::SetSunPosByAngle()
{
	DirectionToSun=AzimuthElevationToDirection(AzimuthToSunRadians,ElevationToSunRadians);
}

void BaseSky::SetMoonPosByAngle()
{
	DirectionToMoon=AzimuthElevationToDirection(AzimuthToMoonRadians,ElevationToMoonRadians);
}

const float4 &BaseSky::GetAmbientLight(float altitude_km) const
{
	static float4 ambient;
	float4 inscatter=GetIsotropicInscatterFactor(altitude_km,pi/2.f,0,1e5f);
	ambient=inscatter*GetInscatterAngularMultiplier(0,inscatter.w,altitude_km);
	return ambient;
}

void BaseSky::SetMoonPosByDirection()
{
	float l=length(DirectionToMoon);
	if(l)
		DirectionToMoon=DirectionToMoon/l;
	else
		DirectionToMoon.Set(0,0,1.f,0);
}

void BaseSky::HazeBaseHeightChanged()
{
	use_haze_base_km=HazeBaseHeightKm;
}

void BaseSky::HazeScaleHeightChanged()
{
	use_haze_scale_km=HazeScaleHeightKm;
}


float BaseSky::GetDistanceToEarth(float Elevation,float h_km) const
{
	if(h_km<0)
		return 0;
	float RH=PlanetRadius-1.f;
	float Rh=PlanetRadius-1.f+h_km;
	float cosine=cos(pi/2.f+Elevation);
	float b=-2*Rh*cosine;
	float c=Rh*Rh-RH*RH;
	float b24c=b*b-4*c;
	if(b24c<0)
		return -1.f;
	float dist=0.5f*(-b-sqrt(b24c));
	return dist;
}