#include <algorithm>
#include "precomp.h"

#include "ColourSky.h"
#include "Simul/Math/Pi.h"
#include "TextureGenerator.h"
#include <math.h>
using namespace simul::sky;

static float saturate(float x)
{
	if(x<0)
		return 0;
	if(x>1.f)
		return 1.f;
	return x;
}

ColourSky::ColourSky():BaseSky()
	,Horizon(.8f,.9f,1.3f,0)
	,Zenith(.5f,.5f,.9f,0)
	,HazeColour(0,0,0,0)
	,opt_lookup_valid(false)
{
	Haze=1.f;
	MieEccentricity=0.7f;
	MakeOpticalLengthLookupTable();
}

ColourSky::~ColourSky()
{
}

unsigned int ColourSky::GetAtmosphericPropertiesChecksum() const
{
	static unsigned c=0;
	return c;
}

unsigned int ColourSky::GetAtmosphericStateChecksum() const
{
	static unsigned c=0;
	c=GetAtmosphericPropertiesChecksum();
	c++;
	return c;
}

float4 ColourSky::GetSkyColour(const float4 &dir,float altitude_km) const
{
	dir;
	altitude_km;
	return Zenith;
}


float4 ColourSky::GetColourLossFactor(const float4 &,const float4 &,bool,int) const
{
	return float4(1,1,1,1);
}

float4 ColourSky::GetInscatter(const float4 &,const float4 &,bool,int) const
{
	return Horizon;
}

float4 ColourSky::GetCoefficients(float alt_km,float elevation,float dist_km,bool earth_test,int steps) const
{
	alt_km;
	dist_km;
	earth_test;
	steps;
	float space_dist=GetDistanceToSpace(elevation,alt_km);
	if(dist_km>space_dist)
		dist_km=space_dist;
	float opt_len,vis,ozone_opt_len,haze_opt_len,haze_opt_len_horizon;
	LookupOpticalLengths(elevation,alt_km,opt_len,vis,ozone_opt_len,haze_opt_len);
	float opt_len_horizon,opt_len_zenith;
	static float horizon_elev=0.f*pi/180.f;
	LookupOpticalLengths(horizon_elev	,alt_km,opt_len_horizon,vis,ozone_opt_len,haze_opt_len_horizon);
	LookupOpticalLengths(pi/2.f			,alt_km,opt_len_zenith,vis,ozone_opt_len,haze_opt_len);
static float olh=50.f;
	opt_len_horizon=olh;
	float4 hor=Horizon+float4(1e-4f,1e-4f,1e-4f,1e-4f);
	float4 zen=Zenith+float4(1e-4f,1e-4f,1e-4f,1e-4f);
	float4 haz=Haze/50.f*HazeColour+float4(1e-4f,1e-4f,1e-4f,1e-4f);
static float pw=4.f;
	float interp=fabs((elevation)/(horizon_elev-pi/2.f));
	interp=1.f-interp;
	interp=pow(interp,pw);
	interp=::saturate(interp);

	//opt_len=std::min(opt_len,opt_len_horizon);

	float4 Zenith_coeff=ColourToScatteringCoefficients(zen,opt_len_zenith,pi/2.f);
	float4 Horizon_coeff=ColourToScatteringCoefficients(hor,opt_len_horizon,horizon_elev);
	float4 Haze_coeff=Haze*ColourToScatteringCoefficients(haz,haze_opt_len_horizon,horizon_elev);
	float4 coeff=lerp(interp,Zenith_coeff,Horizon_coeff)+Haze_coeff;
	return coeff;
}

float4 ColourSky::GetIsotropicColourLossFactor(float alt_km,float elevation,float min_dist_km,float max_dist_km,bool earth_test,int steps) const
{
	float4 coeff=GetCoefficients(alt_km,elevation,max_dist_km,earth_test,steps);
	float opt_len_to_max=GetOpticalLength(elevation,alt_km,max_dist_km);
	float opt_len_to_min=GetOpticalLength(elevation,alt_km,min_dist_km);
	float diff=opt_len_to_max-opt_len_to_min;
	return Exp(-coeff*diff);
}

float4 ColourSky::ColourToScatteringCoefficients(float4 colour,float opt_thickness_km,float elevation) const
{
	float4 sunlight=SunIrradiance;
	colour=float4::Min(colour,sunlight);
	static float mult=0.0596831f;
	float sun_elev=asin(DirectionToSun.z);
	float cos0=cos(elevation-sun_elev);
	float4 output=colour/((1+cos0*cos0)*mult);
	if(sunlight.x<output.x)
		sunlight*=output.x/sunlight.x;
	if(sunlight.y<output.y)
		sunlight*=output.y/sunlight.y;
	if(sunlight.z<output.z)
		sunlight*=output.z/sunlight.z;
	// 1.f-colour/SunIrradiance=exp(-coeff*opt_thickness_km);
	float4 coeff=-Ln(1.f-.99f*output/sunlight)/opt_thickness_km;
	coeff.w=0;
	return coeff;
}

float4 ColourSky::GetIsotropicInscatterFactor(float alt_km,float elevation,float min_dist_km,float max_dist_km,bool earth_test,int steps) const
{
	float4 coeff=GetCoefficients(alt_km,elevation,max_dist_km,earth_test,steps);
	float opt_len_to_max=GetOpticalLength(elevation,alt_km,max_dist_km);
	float opt_len_to_min=GetOpticalLength(elevation,alt_km,min_dist_km);
	float diff=opt_len_to_max-opt_len_to_min;
	float4 colour=SunIrradiance*(1.f-Exp(-coeff*diff));
	float4 output=colour;
	output.w=Haze;
	return output;
}

float4 ColourSky::GetIsotropicInscatter(float,float,float,bool,int) const
{
	return Horizon;
}

static float HenyeyGreenstein(float g,float cos0)
{
	float g2=g*g;
	return 0.5f*0.079577f+0.5f*(1.f-g2)/(4.f*pi*pow(1.f+g2-2.f*g*cos0,1.5f));
}

float4 ColourSky::GetInscatterAngularMultiplier(float cos0,float mie_factor,float alt_km) const
{
	float4 Mie(0,0,0,0);
	float4 mie=Mie*mie_factor;
	float opt_len_zenith;
	float vis,ozone_opt_len,haze_opt_len;
	LookupOpticalLengths(pi/2.f,alt_km,opt_len_zenith,vis,ozone_opt_len,haze_opt_len);
	float4 rayleigh=ColourToScatteringCoefficients(Zenith,opt_len_zenith,pi/2.f);
	float4 extinction=mie+rayleigh+float4(1e-6f,1e-6f,1e-6f,0);
	extinction.w=1.f;
	float BetaRayleigh=0.0596831f*(1+cos0*cos0);			// rayleigh's phase function
	float BetaMie=HenyeyGreenstein(MieEccentricity,cos0);
	float4 BetaTotal=(BetaRayleigh*rayleigh+BetaMie*mie)/(extinction);
	return BetaTotal;
}

float ColourSky::GetOzoneAtAltitude(float) const
{
	return 0;
}

float4 ColourSky::GetLocalIrradiance(float) const
{
	return Sunlight*25.f;
}

float4 ColourSky::GetRayleighCoefficients() const
{
	return float4(0,0,0,1);
}

float4 ColourSky::GetMieCoefficients() const
{
	return float4(0,0,0,1);
}

float4 ColourSky::GetMieRayleighRatio() const
{
	return float4(1,1,1,1);
}

float4 ColourSky::GetSunlightFactor(float) const
{
	return float4(1.f,1.f,1.f,1);
}

float4 ColourSky::GetSunlightFactor(const float4 &) const
{
	return float4(1.f,1.f,1.f,1);
}

float4 ColourSky::GetSunlightFactor(const float4 &,const float4 &) const
{
	return float4(1.f,1.f,1.f,1);
}

float4 ColourSky::GetApparentDirectionToSun() const
{
	return DirectionToSun;
}

void ColourSky::GetMoonPosition(float &,float &) const
{
}

void ColourSky::TimeStep(float dt)
{
	float daytime=Time+dt/60.f/60.f/24.f;
	SetTime(daytime);
}

// OpticalLengthInterface
float ColourSky::GetOpticalLength(float Elevation,float h_km,float distance_to_edge) const
{
	// at this altitude, trace along the elevation to the edge of the atmosphere.
	float total=0;

	// how far to the edge of the atmosphere?
	// RH^2 = d^2 +Rh^2 - 2 d Rh cos(90+e)
	// d=1/2 (-b + _/b^2-4ac )
	float RH=PlanetRadius+AtmosphereThickness;
	float Rh=PlanetRadius+h_km;
	float cosine=cos(pi/2.f+Elevation);
	float b=-2*Rh*cosine;
	float c=Rh*Rh-RH*RH;
	float dist=0.5f*(-b+sqrt(b*b-4*c));
	// this is the distance to the atmosphere.
	if(dist<distance_to_edge||distance_to_edge<0)
		distance_to_edge=dist;
	int Steps=6;
	float step=distance_to_edge/(float)Steps;
	float d=step/2.f;
	float mean_alt=0;
	for(int i=0;i<Steps;i++,d+=step)
	{
		float Ra=sqrt(Rh*Rh+d*d-2*Rh*d*cosine);
		float new_alt=Ra-PlanetRadius;
		float dens_here=GetDensityAtAltitude(new_alt);
		mean_alt+=new_alt*dens_here*step;
		total+=dens_here*step;
	}
	mean_alt/=total;
	mean_alt=std::max(0.f,mean_alt);
	mean_alt=std::min(AtmosphereThickness,mean_alt);
	total/=1.225f;//divide by dens at sea level to get an optical LENGTH.
	return std::max(0.f,total);
}
// OpticalLengthInterface
float ColourSky::GetOvercastFactor(float Elevation,float h_km,float distance_to_edge) const
{
	// at this altitude, trace along the elevation to the edge of the atmosphere.
	float total=0;
	// how far to the edge of the atmosphere?
	// RH^2 = d^2 +Rh^2 - 2 d Rh cos(90+e)
	// d=1/2 (-b + _/b^2-4ac )
	float RH=PlanetRadius+AtmosphereThickness;
	float Rh=PlanetRadius+h_km;
	float cosine=cos(pi/2.f+Elevation);
	float b=-2*Rh*cosine;
	float c=Rh*Rh-RH*RH;
	float dist=0.5f*(-b+sqrt(b*b-4*c));
	// this is the distance to the atmosphere.
	if(dist<distance_to_edge||distance_to_edge<0)
		distance_to_edge=dist;
	int Steps=6;
	float step=distance_to_edge/(float)Steps;
	float d=0/2.f;
	float dens_total=0;
	for(int i=0;i<Steps;i++,d+=step)
	{
		float Ra1=sqrt(Rh*Rh+d*d-2*Rh*d*cosine);
		float alt1=Ra1-PlanetRadius;
		float Ra2=sqrt(Rh*Rh+(d+step)*(d+step)-2*Rh*(d+step)*cosine);
		float alt2=Ra2-PlanetRadius;
		float dens_here=GetDensityAtAltitude(0.5f*(alt1+alt2));
		dens_total+=dens_here;
		total+=GetOvercastAtAltitudeRange(alt1,alt2)*dens_here;
	}
	total/=dens_total;
	return std::max(0.f,1.f-total);
}

float ColourSky::GetOpticalLength(float Elevation,float h_km) const
{
	float distance_to_edge=-1.f;
	return GetOpticalLength(Elevation,h_km,distance_to_edge);
}

float ColourSky::GetHazeFactorOpticalLength(float Elevation,float h_km) const
{
	// at this altitude, trace along the elevation to the edge of the atmosphere.
	float total=0;
	// how far to the edge of the atmosphere?
	// RH^2 = d^2 +Rh^2 - 2 d Rh cos(90+e)
	// d=1/2 (-b + _/b^2-4ac )
	float RH=PlanetRadius+AtmosphereThickness;
	float Rh=PlanetRadius+h_km;
	float cosine=cos(pi/2.f+Elevation);
	float b=-2*Rh*cosine;
	float c=Rh*Rh-RH*RH;
	float distance_to_edge=0.5f*(-b+sqrt(b*b-4*c));
	// this is the distance to the atmosphere.
	int Steps=6;
	float step=distance_to_edge/(float)Steps;
	float d=step/2.f;
	for(int i=0;i<Steps;i++,d+=step)
	{
		float Ra=sqrt(Rh*Rh+d*d-2*Rh*d*cosine);
		float new_alt=Ra-PlanetRadius;
		float haze_here=GetHazeFactorAtAltitude(new_alt);
		total+=haze_here*step;
	}
	return std::max(0.f,total);
}
float ColourSky::GetOzoneOpticalLength(float Elevation,float h_km) const
{
	Elevation;
	h_km;
	return 0;
}
// AtmosphereDensityInterface
float ColourSky::GetDensityFactorAtAltitude(float h_km)		const
{
	if(h_km<0)
		h_km=0;
	static float ff=32.f;
	return exp(-h_km/ff);
}

float ColourSky::GetDensityAtAltitude(float h_km)				const
{
	return 1.225f*GetDensityFactorAtAltitude(h_km);
}

bool ColourSky::MakeOpticalLengthLookupTable() const
{
	if(opt_lookup_valid)
		return true;
#ifdef SKY_THREADS
	simul::base::ReadWriteMutex *sm=(simul::base::ReadWriteMutex *)sky_mutex;
	if(ThreadSafety)
		sm->lock_for_write();
	if(!sky_lookup_valid)
	{
#endif
		float4 *lookup_ptr=optical_lengths_lookup.Setup(512,512,false,false);
		simul::sky::TextureGenerator::Make2DSkyLookupTexture(this,PlanetRadius,
			SunRadiusArcMinutes,512,512,lookup_ptr,
			simul::sky::TextureGenerator::RGBA,true,false,AtmosphereThickness,false);
		opt_lookup_valid=true;

#ifdef SKY_THREADS
	}
	if(ThreadSafety)
		sm->unlock_from_write();
#endif
	return true;
}

void ColourSky::LookupOpticalLengths(float Elevation,float h_km,float &opt_len,float &vis,float &ozone_opt_len,float &haze_opt_len) const
{
	MakeOpticalLengthLookupTable();
#ifdef SKY_THREADS
	simul::base::ReadWriteMutex *sm=(simul::base::ReadWriteMutex *)sky_mutex;
	if(ThreadSafety)
		sm->lock_for_read();
#endif
	float x=::saturate(.5f+.5f*(Elevation*2.f/3.1415926536f));
	float y=::saturate(h_km/AtmosphereThickness);
	float4 lookup=optical_lengths_lookup.Get(x,y);
	opt_len=lookup.x;
	vis=lookup.y;
	ozone_opt_len=lookup.w;
	haze_opt_len=lookup.z;
#ifdef SKY_THREADS
	if(ThreadSafety)
		sm->unlock_from_read();
#endif
}

void ColourSky::TimeChanged()
{
	ElevationToSunRadians=Time*2.f*pi-pi/2.f;
	AzimuthToSunRadians=-pi/2.f;
	SetSunPosByAngle();
}