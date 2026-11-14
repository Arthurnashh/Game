#include "precomp.h"

#include "Sky.h"
#include "TextureGenerator.h"
#include "Simul/Math/Matrix4x4.h"
#include <math.h>
#include <algorithm>

#include <time.h>


#ifdef SKY_THREADS
#include <windows.h>
#include "Simul/Base/ReadWriteMutex.h"
#undef max
#undef min
#endif

#define pi (3.1415926536f)
using namespace simul::sky;

// Red: 650nm Green: 510nm Blue: 475nm
//static float red_wavelength=650.f;  630?
//static float green_wavelength=510.f; 530?
//static float blue_wavelength=475.f;450?

// Mars scale height 11km
// CO2 95 N 3 1.6 Ar 
// Atmo pressure 610 Pa at Datum altitude
// Dusty: tawny when seen from surface 1.5 microns across
// Temperatures: 130K to 293K

static float rayleigh_approx(float l)
{
	const float N=2.545e-12f;
	const float pn=0.0035f;
	const float n=1.003f;
	float result=0;
	
	result=pow(2.f*pi,3.f);
	result*=pow(n*n-1.f,2.f);
	result/=(3.f*N*pow(l,4.f));
	result*=(6.f+3.f*pn);
	result/=(6.f-7.f*pn);
	return result;
}
//spectral irradiance

static float saturate(float x)
{
	if(x<0)
		return 0;
	if(x>1.f)
		return 1.f;
	return x;
}

float Sky::HenyeyGreenstein(float g,float cos0)
{
	float g2=g*g;
#if 0
	return 3.f*(1.f-g2)*(1.f+cos0*cos0)/(8.f*pi*(2+g2)*pow(1.f+g2-2.f*g*cos0,1.5f));
	return pow(1.f-g,2.f)/(4.f*pi*pow(1.f+g2-2.f*g*cos0,1.5f));
#else
	//return (1.f-g2)/(4.f*pi*pow(1.f+g2-2.f*g*cos0,1.5f));
	return 0.5f*0.079577f+0.5f*(1.f-g2)/(4.f*pi*pow(1.f+g2-2.f*g*cos0,1.5f));
#endif
}

static float GetSunRefraction(float elevation,float alt_km)
{
	elevation;alt_km;return 0;
/*	if(elevation<-0.23565f)
		elevation=-0.23565f;

	// d(e+r)/de=1.0-10.246*.5*3.14159/180.0*exp(-10.246f*elevation);
	// =0 when
	// ln(1/(10.246*.5*3.14159/180.0))/(-10.246f)=elevation;
	// elevation=ln(1/(.08941))/-10.246
	//          =-0.23565
	return exp(-alt_km/4.f)*0.5f*3.14159f/180.f*exp(-10.246f*elevation);*/
}
/*
    Cumulus: Cumulus Congestus (larger, flat-bottomed clouds) and Cumulus Mediocris (smaller, puffy clouds).
    Stratus: Both solid stratus decks from a rainy day, and broken stratus decks.
    Cirrus: High, wispy clouds as a translucent plane.
    Cumulonimbus: Rainclouds.
*/

/*use the following formula for luminance from (r,g,b):

	l = .265*r + .670*g + .065*b

(This is in units of .00559 cd/m^2, but never mind that.)  Then, say you want
your color (hue,saturation) to be the same as (r,g,b)=(.5,.1,.05) -- a nice
brown.  The values you would use for your glow would be these divided by the
luminance as defined above, i.e. divided by (.256*.5 + .670*.1 + .065*.05),
which is .198.  Thus, you would use:

	skyfunc glow groundglow
	0
	0*
	4 2.52 .504 .252 0*/
static const float DEGREES = 3.1415926536f/180.f;
static const double DEGREES_D = 3.1415926536/180.0;


Sky::Sky():
	 BaseSky()
	,step_lengths(NULL)
	,step_centres(NULL)
	,LookupTableSize(64)
	,AutoIncrementDayOfTheYear(false)
	,Steps(0)
	,earth_test_scale(1.f)
#ifdef SKY_THREADS
	,sky_mutex(NULL)
	,dens_mutex(NULL)
#endif
	,AutoUpdateTables(true)
	,ThreadSafety(false)
	,AutomaticMoonPosition(true)
	,AutomaticSunPosition(true)
{
	InitializePropertiesDefinition();
	InitializePropertiesState();
#ifdef SKY_THREADS
	sky_mutex=new simul::base::ReadWriteMutex;
	dens_mutex=new simul::base::ReadWriteMutex;
#endif

	Starlight.Set(0.00001f,0.00001f,0.00001f,0);
	ColourWavelengthsNm.Set(630.f,540.f,450.f,0);
	//ColourWavelengthsNm.Set(650.f,510.f,475.f,0);
	InitializePropertiesDefinition();
	InitializePropertiesState();
	Timezone=0;
	// Chappuis absorption at
	Ozone.Set(.67f,.4f,0.09f,0);
	MieWavelengthExponent=0;//-1.3f;
	Mie.Set(0.0046f,0.0046f,0.0046f,0);
	Density=1.f;
	SetSteps(12);
	DayOfTheYear=183;
	Year=2010;
	Timezone=0.f;
	LatitudeRadians=LongitudeRadians=0.f;
	Haze=0.f;
	SetHaze(0.5f);
	OzoneLevel=.1f;
	OzoneLayerAltitude=22.f;
	sky_lookup_valid=false;
	density_lookup_valid=false;
	OzoneChanged();
	UpdateSunPosition();

	UpdateMoonPosition();
	CalcMieCoefficients();
	//Rayleigh.Set(0.00695f,0.0118f,0.0244f,0);
	CalcAutomaticRayleighCoefficients();
	rayleigh=Rayleigh;
	rayleigh*=Density;
}

Sky::~Sky()
{
#ifdef SKY_THREADS
	simul::base::ReadWriteMutex *sm=(simul::base::ReadWriteMutex *)sky_mutex;
	delete sm;
	sky_mutex=NULL;
	
	simul::base::ReadWriteMutex *dm=(simul::base::ReadWriteMutex *)dens_mutex;
	delete dm;
	dens_mutex=NULL;
#endif
	delete [] step_lengths;
	delete [] step_centres;
}

int Sky::DaysInTheYear(int y) const
{
	bool leap=false;
	if(y%400==0)
		leap=true;
	else if(y%100==0)
		leap=false;
	else if(y%4==0)
		leap=true;
	return 365+((leap==true)?1:0);
}

void Sky::GetDate(int &M,int &D) const
{
	return GetDate(M,D,DayOfTheYear);
}

void Sky::GetDate(int &M,int &D,int day_of_the_year) const
{
	int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
	int d=day_of_the_year;
	int y=Year;
	if(DaysInTheYear(y)==366)
		days[1]=29;
	for(int m=0;m<12;m++)
	{
		if(d-days[m]<0)
		{
			M=(m+1);
			D=d+1;
			return;
		}
		d-=days[m];
	}
}
void Sky::SetDate(int M,int D)
{
	int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
	if(DaysInTheYear(Year)==366)
		days[1]=29;
	int d=0;
	for(int m=0;m<M-1;m++)
	{
		d+=days[m];
	}
	d+=D-1;
	SetDayOfTheYear(d);
}

/*static long date_to_julian(int y, int m, int d)
{
	return (1461 * (y + 4800 + (m - 14) / 12)) / 4 +
		(367 * (m - 2 - 12 * ((m - 14) / 12))) / 12 -
		(3 * ((y + 4900 + (m - 14) / 12) / 100)) / 4 +
		d - 32075;
}*/

void Sky::GetDateFromDayNumber(int D,int &year,int &month,int &day)
{
	//d = JD – 2451543.5 = MJD – 51543.0
	int jd=(int)((double)D + 2451544);//was 43.5!!
	long  L = jd + 68569;
	long  n = (4 * L) / 146097;
	long  i, j;

	L = L - (146097 * n + 3) / 4;
	i = (4000 * (L + 1)) / 1461001;
	L = L - (1461 * i) / 4 + 31;
	j = (80 * L) / 2447;
	day = L - (2447 * j) / 80;
	L = j / 11;
	month = j + 2 - (12 * L);
	year = 100 * (n - 49) + i + L;
}

int Sky::GetDayNumber(int year,int month,int day)
{
    int D = 367*year - (7*(year + ((month+9)/12)))/4 + (275*month)/9 + day - 730530;
	return D;
}

float Sky::GetDayNumber() const
{
	int M=0;
	int day=0;
	GetDate(M,day);
	int D=GetDayNumber(Year,M,day);
	float d=(float)D;
	d+=GetHourOfTheDay()/24.f;
	return d;
}

float Sky::GetDensityFactorAtAltitude(float h_km) const
{
	float h=h_km*1000.f;

	// constants:
	float R=8314.32f;
	float g0=9.80665f;
	float M=28.9644f;

	// values dependent on atmospheric layer
	float L0=-.0065f;
	float r0=1.f;
	float T0=288.15f;
	float h0=0;

	if(h_km<0)
		return r0;
	if(h_km>11.f&&h_km<=20.f)
	{
		L0=0;
		r0=GetDensityFactorAtAltitude(11.f);// should give 0.36391f;
		T0=216.65f;					// = 288.15-.0065*11000
		h0=11000.f;

		float r=r0*exp(-g0*M*(h-h0)/(R*T0));
		return r;
	}
	else if(h_km>20.f&&h_km<=32.f)
	{
		L0=.001f;
		r0=GetDensityFactorAtAltitude(20.f);// should give 0.36391f;
		T0=216.65f;					// = 288.15-.0065*11000
		h0=20000.f;
	}
	else if(h_km>32.f)
	{
		L0=.0028f;
		r0=GetDensityFactorAtAltitude(32.f);// should give 0.36391f;
		T0=228.65f;					// = 288.15-.0065*11000
		h0=32000.f;
	}
	else if(h_km>47.f)
	{
		L0=0;
		r0=GetDensityFactorAtAltitude(47.f);// should give 0.36391f;
		T0=270.65f;					// = 288.15-.0065*11000
		h0=47000.f;

		float r=r0 * exp(- g0*M*(h-h0)/(R*T0));
		return r;
	}
	else if(h_km>53.f)
	{
		return 0;
	}
	float fraction=T0/(T0+L0*(h-h0));
	float power=g0*M/(R*L0)+1.f;
	float r=r0*pow(fraction,power);
	if(r<0)
		r=0;
	static float hh=20,xx=0;
	if(h_km>hh)
		r+=xx;
	return r;
}

float Sky::GetDensityAtAltitude(float h_km) const
{
	return Density*GetDensityFactorAtAltitude(h_km);
}

float Sky::GetOpticalLength(float Elevation,float h_km) const
{
	float distance_to_edge=-1.f;
	return GetOpticalLength(Elevation,h_km,distance_to_edge);
}

float Sky::GetHazeFactorOpticalLength(float Elevation,float h_km) const
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

float Sky::GetOpticalLength(float Elevation,float h_km,float distance_to_edge) const
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

  if (total > 0)
  {
    mean_alt/=total;
    mean_alt=std::max(0.f,mean_alt);
    mean_alt=std::min(AtmosphereThickness,mean_alt);
    total/=1.225f;//divide by dens at sea level to get an optical LENGTH.
  }
	return std::max(0.f,total);
}

float Sky::GetOzoneOpticalLength(float Elevation,float h_km) const
{
	// at this altitude, trace along the elevation to the edge of the atmosphere.
	float total=0;
	// how far to the edge of the atmosphere?
	// RH^2 = d^2 +Rh^2 - 2 d Rh cos(90+e)
	// d=1/2 (-b + _/b^2-4ac )
	if(h_km>=AtmosphereThickness)
		return 0;
	float RH=PlanetRadius+AtmosphereThickness;
	float Rh=PlanetRadius+h_km;
	float cosine=cos(pi/2.f+Elevation);
	float b=-2*Rh*cosine;
	float c=Rh*Rh-RH*RH;
	float dist=0.5f*(-b+sqrt(b*b-4*c));
	// this is the distance to the atmosphere.
	float step=dist/(float)Steps;
	float d=step/2.f;
	for(int i=0;i<Steps;i++,d+=step)
	{
		float Ra=sqrt(Rh*Rh+d*d-2*Rh*d*cosine);
		float new_alt=Ra-PlanetRadius;
		float dens_here=GetOzoneAtAltitude(new_alt);
		total+=dens_here*step;
	}
	return total;
}

void Sky::RemakeSkyLookupTable() const
{
	sky_lookup_valid=false;
}

void Sky::InitLookupTables()
{
	MakeSkyLookupTable();
	MakeDensityLookupTable();
}

bool Sky::MakeSkyLookupTable() const
{
	if(sky_lookup_valid)
		return true;
	use_haze_scale_km=1.f;
#ifdef SKY_THREADS
	simul::base::ReadWriteMutex *sm=(simul::base::ReadWriteMutex *)sky_mutex;
	if(ThreadSafety)
		sm->lock_for_write();
	if(!sky_lookup_valid)
	{
#endif
		float4 *lookup_ptr=sky_lookup.Setup(LookupTableSize,LookupTableSize,false,false);
		use_haze_base_km=0;
		simul::sky::TextureGenerator::Make2DSkyLookupTexture(this,PlanetRadius,SunRadiusArcMinutes,LookupTableSize,LookupTableSize,lookup_ptr,
			simul::sky::TextureGenerator::RGBA,true,false,AtmosphereThickness,false);
		use_haze_base_km=HazeBaseHeightKm;
		sky_lookup_valid=true;
#ifdef SKY_THREADS
	}
	if(ThreadSafety)
		sm->unlock_from_write();
#endif
	use_haze_scale_km=HazeScaleHeightKm;
	return true;
}

float Sky::GetHazeOpticalLength(float Elevation,float h_km) const
{
	if(AutoUpdateTables)
		MakeSkyLookupTable();
#ifdef SKY_THREADS
	simul::base::ReadWriteMutex *sm=(simul::base::ReadWriteMutex *)sky_mutex;
	if(ThreadSafety)
		sm->lock_for_read();
#endif
	float x=::saturate(0.5f+0.5f*(Elevation*2.f/3.1415926536f));
	float y=::saturate((h_km-HazeBaseHeightKm)/(use_haze_scale_km*AtmosphereThickness));
	float4 lookup=sky_lookup.Get(x,y);
	float haze_opt_len=lookup.z*use_haze_scale_km;
	if(h_km>HazeBaseHeightKm/use_haze_scale_km)
		haze_opt_len*=exp(HazeBaseHeightKm/use_haze_scale_km);
#ifdef SKY_THREADS
	if(ThreadSafety)
		sm->unlock_from_read();
#endif
	return haze_opt_len;
}

void Sky::Lookup(float Elevation,float h_km,float &opt_len,float &vis,float &ozone_opt_len) const
{
	if(AutoUpdateTables)
		MakeSkyLookupTable();
#ifdef SKY_THREADS
	simul::base::ReadWriteMutex *sm=(simul::base::ReadWriteMutex *)sky_mutex;
	if(ThreadSafety)
		sm->lock_for_read();
#endif
	float x=::saturate(0.5f+0.5f*(Elevation*2.f/3.1415926536f));
	float y=::saturate(h_km/AtmosphereThickness);
	float4 lookup=sky_lookup.Get(x,y);
	opt_len=lookup.x;
	vis=lookup.y;
	ozone_opt_len=lookup.w;
#ifdef SKY_THREADS
	if(ThreadSafety)
		sm->unlock_from_read();
#endif
}

void Sky::MakeDensityLookupTable() const
{
	if(!density_lookup_valid)
	{
#ifdef SKY_THREADS
		simul::base::ReadWriteMutex *dm=(simul::base::ReadWriteMutex *)dens_mutex;
		if(ThreadSafety)
			dm->lock_for_write();
		if(!density_lookup_valid)
		{
#endif
			use_haze_base_km=0;
			float4 *lookup_ptr=density_lookup.Setup(LookupTableSize,false,true);
			simul::sky::TextureGenerator::Make1DDensityImage(this,LookupTableSize,lookup_ptr,
				simul::sky::TextureGenerator::RGBA,true);
			use_haze_base_km=HazeBaseHeightKm;
			density_lookup_valid=true;
#ifdef SKY_THREADS
		}
		if(ThreadSafety)
			dm->unlock_from_write();
#endif
	}
}

void Sky::LookupDensities(float h_km,float &dens,float &ozone,float &haze) const
{
	if(AutoUpdateTables)
		MakeDensityLookupTable();
#ifdef SKY_THREADS
	simul::base::ReadWriteMutex *dm=(simul::base::ReadWriteMutex *)dens_mutex;
	if(ThreadSafety)
		dm->lock_for_read();
#endif
	float x=::saturate(h_km/AtmosphereThickness);
	float4 lookup=density_lookup.Get(x);
#ifdef SKY_THREADS
	if(ThreadSafety)
		dm->unlock_from_read();
#endif
	dens=lookup.x;
	ozone=lookup.y;
	x=::saturate((h_km-HazeBaseHeightKm)/(use_haze_scale_km*AtmosphereThickness));
	lookup=density_lookup.Get(x);
	haze=lookup.z;
}

void Sky::GetSolarTime(float *solar_hours,float standard_hours,float meridian,
						float longitude,int DayOfTheYear)
{
	*solar_hours=standard_hours+.17f*sin(4*pi*(DayOfTheYear-80.f)/373.f)
		-0.129f*sin(2*pi*(DayOfTheYear-8.f)/355.f)+12.f*(meridian-longitude)/pi;
}

void Sky::GetSunPosition(float &azimuth,float &elevation,float time_of_day,float longitude,float time_zone)
{
	float meridian=2.f*pi*time_zone/24.f;
	while(meridian<0)
		meridian+=2.f*pi;
	float solar_hours;
	GetSolarTime(&solar_hours,time_of_day,meridian,longitude,DayOfTheYear);
	float declination=0.4093f*sin(2.f*pi*(DayOfTheYear-81.f)/368.f);
	GetSunPosition(azimuth,elevation,declination,solar_hours);
}

void Sky::CalcAutomaticRayleighCoefficients(float4 &r,const float4 &ColourWavelengthsNm)
{
	r.Set(rayleigh_approx(ColourWavelengthsNm.x),rayleigh_approx(ColourWavelengthsNm.y),rayleigh_approx(ColourWavelengthsNm.z),0);
}

void Sky::CalcAutomaticRayleighCoefficients()
{
	CalcAutomaticRayleighCoefficients(Rayleigh,ColourWavelengthsNm);
}

void Sky::ChangedRayleighCoefficients()
{
	rayleigh=Rayleigh;
	rayleigh*=Density;
	//RemakeSkyLookupTable();
}

void Sky::ChangedMieCoefficients()
{
	if(MieWavelengthExponent!=0)
		CalcMieCoefficients();
	//RemakeSkyLookupTable();
}

float Sky::GetHourOfTheDay() const
{
	return Time*24.f;
}

void Sky::SetHourOfTheDay(float h)
{
	Time=h/24.f;
	UpdateSunPosition();
}

void Sky::TimeStep(float dt)
{
	Time+=dt/60.f/60.f/24.f;
	TimeChanged();/*
	while(Time>1.f)
	{
		DayOfTheYear++;
		Time-=1.f;
		if(DayOfTheYear>DaysInTheYear(Year))
		{
			Year++;		
			DayOfTheYear=0;
		}
	}
	while(Time<0.f)
	{
		Time+=1.f;
		DayOfTheYear--;
		if(!DayOfTheYear)
		{
			Year--;
			DayOfTheYear=DaysInTheYear(Year);
		}
	}
	UpdateSunPosition();
	UpdateMoonPosition();*/
}

void Sky::TimeChanged()
{
	while(Time>1.f)
	{
		DayOfTheYear++;
		Time-=1.f;
		if(DayOfTheYear>DaysInTheYear(Year))
		{
			Year++;		
			DayOfTheYear=0;
		}
	}
	while(Time<0.f)
	{
		Time+=1.f;
		DayOfTheYear--;
		if(!DayOfTheYear)
		{
			Year--;
			DayOfTheYear=DaysInTheYear(Year);
		}
	}
	UpdateSunPosition();
	UpdateMoonPosition();
}

void Sky::AtmosphereThicknessChanged()
{
	RemakeSkyLookupTable();
}

void Sky::PlanetRadiusChanged()
{
	RemakeSkyLookupTable();
}

void Sky::SetMoonPosByDirection()
{
	BaseSky::SetMoonPosByDirection();
	AutomaticMoonPosition=false;
}

void Sky::StarlightChanged()
{
//	RemakeSkyLookupTable();>>???
}

const float4 &Sky::GetDirectionToLight() const
{
	return DirectionToSun;
}

void Sky::UpdateSunPosition()
{
	if(!AutomaticSunPosition)
	{
		float apparent_sun_elev=asin(DirectionToSun.z);
		ElevationToSunRadians=apparent_sun_elev-GetSunRefraction(apparent_sun_elev,0);
		for(int i=0;i<4;i++)
			ElevationToSunRadians=apparent_sun_elev-GetSunRefraction(ElevationToSunRadians,0);
		AzimuthToSunRadians=atan2(-DirectionToSun.y,DirectionToSun.x);
		return;
	}
	GetSunPosition(AzimuthToSunRadians,ElevationToSunRadians,24.f*Time,LongitudeRadians,Timezone);
	float apparent_sun_elev=ElevationToSunRadians+GetSunRefraction(ElevationToSunRadians,0.f);
	// This is the direction TO the sun.
	DirectionToSun.x=sin(AzimuthToSunRadians)*cos(apparent_sun_elev);
	DirectionToSun.y=cos(AzimuthToSunRadians)*cos(apparent_sun_elev);
	DirectionToSun.z=sin(apparent_sun_elev);
  DirectionToSun.w=0;
}

void Sky::UpdateMoonPosition()
{
	if(AutomaticMoonPosition)
	{
		GetMoonPosition(AzimuthToMoonRadians,ElevationToMoonRadians);
	}
	DirectionToMoon.x=sin(AzimuthToMoonRadians)*cos(ElevationToMoonRadians);
	DirectionToMoon.y=cos(AzimuthToMoonRadians)*cos(ElevationToMoonRadians);
	DirectionToMoon.z=sin(ElevationToMoonRadians);
  DirectionToMoon.w=0;
}
float4 Sky::GetApparentDirectionToSun() const
{
	return DirectionToSun;
}

float Sky::GetTimeInSeconds() const
{
	return (DayOfTheYear+Time)*24.f*60.f*60.f;
}

void Sky::HazeChanged()
{
	CalcMieCoefficients();
}

void Sky::CalcMieCoefficients()
{
#if 1
	// Aerosol Mie scattering, coefficient is -beta*wavelength^(alpha*m)
	float beta=0.04608f*(Haze+1.f)-0.04586f;			// Angstrom's turbidity coefficient quoted by Preetham
	if(MieWavelengthExponent!=0)
	{
		Mie.x=.1f*beta*pow(ColourWavelengthsNm.x/1000.f,MieWavelengthExponent);		// wavelength is converted from nm to micrometres
		Mie.y=.1f*beta*pow(ColourWavelengthsNm.y/1000.f,MieWavelengthExponent);
		Mie.z=.1f*beta*pow(ColourWavelengthsNm.z/1000.f,MieWavelengthExponent);
	}
	/*else
	{
		Mie.x=.01f*beta*pow(green_wavelength/1000.f,-1.3f);
		Mie.y=.01f*beta*pow(green_wavelength/1000.f,-1.3f);
		Mie.z=.01f*beta*pow(green_wavelength/1000.f,-1.3f);
	}*/
#else
	// From "A Practical Analytical Model for Daylight" by Preetham:
	double nu=4.0;
	double c=(0.6544*(1.f+Haze)-0.6510)*1e-16;
	Mie.x=(float)(0.434*c*pi*pow(2.0*pi/(ColourWavelengthsNm.x		*1e-9),nu-2)*0.68455);
	Mie.y=(float)(0.434*c*pi*pow(2.0*pi/(ColourWavelengthsNm.y	*1e-9),nu-2)*0.673323);
	Mie.z=(float)(0.434*c*pi*pow(2.0*pi/(ColourWavelengthsNm.z	*1e-9),nu-2)*0.6691485);
#endif
}

void Sky::OzoneChanged()
{
	// Chappuis absorption at Red: 650nm Green: 510nm Blue: 475nm
	// for 1cm of ozone, 0.97, 0.99, 0.995
	// which means exp (- b x) = 0.97 where x is 0.01 metres.
	// so - 0.01 b = ln(0.97)   -    or b=100 ln(0.97)
	// 30.46, 10.05, 5.01
	//Ozone.Set(.67f,.4f,0.09f,0);
	static float4 baseOzone(.06f,.04f,0.01f,0);
	Ozone=baseOzone;
	Ozone*=OzoneLevel;
}

float4 Sky::GetMieCoefficients() const
{
	return Mie;
}		

float4 Sky::GetMieRayleighRatio() const
{
	return Mie/rayleigh;
}		

float4 Sky::GetRayleighCoefficients() const
{
	return rayleigh;
}

float4 Sky::GetLocalIrradiance(float altitude_km) const
{
	float4 pos(0,0,altitude_km,0);
	return GetSunlightColour(pos);
}

float4 Sky::GetZenithColour(float altitude_km) const
{
	float pos[]={0,0,1.f};
	return GetSkyColour(pos,altitude_km);
}


float4 Sky::GetHorizonColour(float altitude_km) const
{
	float pos[]={1.f,0,0};
	return GetSkyColour(pos,altitude_km);
}

float Extinction(float e,float t)
{
	return exp(e*-t);
}

void Extinction(float4 &result,const float4 &ext,float dist)
{
	result.x=exp(ext.x*-dist);
	result.y=exp(ext.y*-dist);
	result.z=exp(ext.z*-dist);
}

float4 Sky::GetSunlightFactor(const float4 &Pos,const float4 &DirectionToLight) const
{
	float4 output;
	float4 NewPos=Pos;
	NewPos.z+=PlanetRadius;
	NewPos.w=0;
	float4 NewVertical=NewPos;
	NewVertical/=length(NewPos);
	float new_altitude=float4::dot(NewPos,NewVertical)-PlanetRadius;
	float new_elevation_cosine=float4::dot(NewVertical,DirectionToLight);
	float new_sun_elevation=pi/2-acos(new_elevation_cosine);
	float illuminated_length;
	float vis=1.f;
	float ozone_length=0;
	float haze_opt_len=0;
	Lookup(new_sun_elevation,new_altitude,illuminated_length,vis,ozone_length);
	haze_opt_len=GetHazeOpticalLength(new_sun_elevation,new_altitude);
	output.Set(vis,vis,vis,0);
	output*=Exp(-rayleigh*illuminated_length-Mie*(Haze*haze_opt_len)-Ozone*ozone_length);
	output.w=vis;
	return output;
}



float4 Sky::GetSkyColour(const float4 &vertexDir,const float4 &input,float altitude_km) const
{
	float4 VertexDir=vertexDir;
	unit(VertexDir);
	float4 view_pos_km(0,0,altitude_km,0);
	float4 pos_km=view_pos_km+VertexDir*1e5f;
	float4 colour=input;
	colour*=GetColourLossFactor(view_pos_km,pos_km,true);
	float4 inscatter=GetIsotropicInscatterFactor(view_pos_km,pos_km);
	float cos0=float4::dot(DirectionToSun,VertexDir);
	colour+=inscatter*GetInscatterAngularMultiplier(cos0,inscatter.w,altitude_km);
	return colour;
}
float4 Sky::GetSkyColour(const float4 &vertexDir,float altitude_km) const
{
	float4 output;
	zero(output);
	return GetSkyColour(vertexDir,output,altitude_km);
}

float4 Sky::GetIsotropicColourLossFactor(float view_alt_km,float elevation,float min_dist_km,float max_dist_km,bool earth_test,int steps) const
{
	bool use_table=(steps==0);
	if(!steps)
		steps=Steps;
	float space_dist=GetDistanceToSpace(elevation,view_alt_km);
	if(max_dist_km>space_dist)
		max_dist_km=space_dist;
	if(min_dist_km>max_dist_km)
		return float4(1.f,1.f,1.f,1.f);
	float4 loss,extinction;
	// We consider the distance from the viewer to the edge of the atmosphere.
	float diff_km=max_dist_km-min_dist_km;
	float step_length=diff_km/(float)Steps;
	float4 total_loss(1,1,1,1);
	if(max_dist_km<=min_dist_km)
		return total_loss;
	float sin_e=sin(elevation);
	float cos_e=cos(elevation);
	float4 HazeMie=Haze*Mie;
	float mie_factor=0.f;
	float total_dens_dist=0.f;
	float prev_alt_km=1e5f;
	float prev_dist=max_dist_km;
	float total_length=0;
	for(int i=0;i<steps;i++)
	{
		float dist;
		if(use_table)
		{
			step_length=diff_km*step_lengths[i];
			dist=min_dist_km+diff_km*step_centres[i];
		}
		else
		{
			float frac=(steps-i-.5f)/(float)steps;
			float frac1=(steps-i-1.f)/(float)steps;
			float frac2=(steps-i)/(float)steps;
			dist=min_dist_km+diff_km*frac*frac;
			step_length=diff_km*(frac2*frac2-frac1*frac1);
		}
		float y=PlanetRadius+view_alt_km+dist*sin_e;
		float x=dist*cos_e;
		float r=sqrt(x*x+y*y);
		float alt_km=r-PlanetRadius;
		float dens_factor,ozone_factor,haze_factor;
		LookupDensities(alt_km,dens_factor,ozone_factor,haze_factor);
		float4 light=GetLocalIrradiance(alt_km)+Starlight;
		light+=GetMoonIrradiance(alt_km);
		// this light is calculated in order to get the inscatter between the previous and the current
		// altitude.
		float overcast=GetOvercastAtAltitudeRange(alt_km,prev_alt_km);
		light*=(1.f-overcast);
		float dens_dist=dens_factor*step_length;
		total_dens_dist+=dens_dist*dens_factor;
		// light.w is sun visibility
		mie_factor+=light.w*step_length*Haze*haze_factor;
		float4 total_ext=dens_factor*rayleigh+haze_factor*HazeMie+ozone_factor*Ozone;
		Extinction(loss,total_ext,step_length);
		if(earth_test&&alt_km<0)
		{
			if(alt_km<-earth_test_scale)
				loss*=0.f;
			else
				loss*=(1.f+alt_km/earth_test_scale);
		}
		total_loss*=loss;
		prev_alt_km=alt_km;
		prev_dist=dist;
		total_length+=step_length;
	}
	if(total_dens_dist)
		mie_factor/=total_dens_dist;
	if(mie_factor>1.f)
		mie_factor=1.f;
	if(mie_factor<0.f)
		mie_factor=0.f;
	total_loss.w=1.f;
	return total_loss;
}

float4 Sky::GetIsotropicInscatterFactor(float view_alt_km,float elevation,float min_dist_km,float max_dist_km,bool earth_test,int steps) const
{
	bool use_table=(steps==0);
	if(!steps)
		steps=Steps;

	float space_dist=GetDistanceToSpace(elevation,view_alt_km);
	if(earth_test)
		if(elevation<0)
		{
			float dist_to_earth=GetDistanceToEarth(elevation,view_alt_km);
			if(dist_to_earth>=0)
				if(dist_to_earth<space_dist)
					space_dist=dist_to_earth;
		}
	if(max_dist_km>space_dist)
		max_dist_km=space_dist;
	if(min_dist_km>max_dist_km)
		return float4(0,0,0,0);
	float4 ext,extinction,ext2;
	// We consider the distance from the viewer to the edge of the atmosphere.
	float4 final(0,0,0,0);
	float sin_e=sin(elevation);
	float cos_e=cos(elevation);
	float mie_factor=0.f;
	float total_dens_dist=0.f;
	
	float diff_km=max_dist_km-min_dist_km;
	float step_length=diff_km/(float)steps;
	float prev_alt_km=AtmosphereThickness;
	float4 HazeMie=Haze*Mie;
	static float skylight_effect=2.f*pi;
	float4 Skylight(0,0,0,0);
	if(skylight_effect>0&&steps>1)
	{
		static float hh=1.f;
		float cos0=DirectionToSun.z;
		Skylight=GetIsotropicInscatterFactor(hh,pi/2.f,0,1e5f,false,1);
		Skylight*=GetInscatterAngularMultiplier(cos0,Skylight.w,hh);
		Skylight*=skylight_effect;
	}
	for(int i=0;i<steps;i++)
	{
		float dist;
		if(use_table)
		{
			step_length=diff_km*step_lengths[i];
			dist=min_dist_km+diff_km*step_centres[i];
		}
		else
		{
			float frac=(steps-i-.5f)/(float)steps;
			dist=min_dist_km+diff_km*frac*frac;
		}
		float y=PlanetRadius+view_alt_km+dist*sin_e;
		float x=dist*cos_e;
		float r=sqrt(x*x+y*y);
		float alt_km=r-PlanetRadius;
		float dens_factor,ozone_factor,haze_factor;
		LookupDensities(alt_km,dens_factor,ozone_factor,haze_factor);
		float4 light=GetLocalIrradiance(alt_km)+Starlight+Skylight;
		light+=GetMoonIrradiance(alt_km);
		// this light is calculated in order to get the inscatter between the previous and the current
		// altitude.
		float overcast=GetOvercastAtAltitudeRange(alt_km,prev_alt_km);
		light*=(1.f-overcast);
		float4 mie=haze_factor*HazeMie;
		float dens_dist=dens_factor*step_length;
		total_dens_dist+=dens_dist*dens_factor;
		// light.w is sun visibility
		mie_factor+=step_length*Haze*haze_factor;//light.w*
		float4 extinction=dens_factor*rayleigh+mie;
		Extinction(ext,extinction,step_length);
		float4 ozone_ext=Ozone*ozone_factor;
		float4 total_ext=extinction+ozone_ext;
		Extinction(ext2,total_ext,step_length);
		final*=ext2;
		light*=(1.f-ext);
		final+=light;
		prev_alt_km=alt_km;
	}

	if(total_dens_dist)
		mie_factor/=total_dens_dist;
	if(mie_factor>1.f)
		mie_factor=1.f;
	if(mie_factor<0.f)
		mie_factor=0.f;
	final.w=mie_factor;
	return final;
}

float4 Sky::GetInscatterAngularMultiplier(float cos0,float mie_factor,float ) const
{
#if 1
	float4 mie=Mie*mie_factor;
	float4 extinction=mie+rayleigh;
	extinction.w=1.f;
	float BetaRayleigh=0.0596831f*(1+cos0*cos0);			// rayleigh's phase function
	float BetaMie=HenyeyGreenstein(MieEccentricity,cos0);
	float4 BetaTotal=(BetaRayleigh*rayleigh+BetaMie*mie)/(extinction);
#else
	float4 mie=Mie;
	float4 extinction=(Mie*mie_factor+rayleigh)/rayleigh;
	extinction.w=1.f;
	float BetaRayleigh=0.0596831f*(1+cos0*cos0);			// rayleigh's phase function
	float BetaMie=HenyeyGreenstein(MieEccentricity,cos0);
	float4 BetaTotal=(BetaRayleigh+BetaMie*mie_factor*mie_rayleigh_ratio)/(extinction);
#endif
	return BetaTotal;
}

float Sky::GetVisibilityDistance(float view_alt_km)
{
	float dens_factor,ozone_factor,haze_factor;
	float4 HazeMie=Haze*Mie;
	LookupDensities(view_alt_km,dens_factor,ozone_factor,haze_factor);
	float4 total_ext4=dens_factor*rayleigh+haze_factor*HazeMie+ozone_factor*Ozone;
	float4 loss4(0.1f,0.1f,0.1f,0.1f);
	float4 len4=-Ln(loss4)/total_ext4;
	float len=(len4.x+len4.y+len4.z)/3.f;
	return len;
}

float Sky::GetHazeForVisibilityDistance(float view_alt_km,float horiz_dist_km)
{
	float dens_factor,ozone_factor,haze_factor;
	float4 HazeMie=Haze*Mie;
	LookupDensities(view_alt_km,dens_factor,ozone_factor,haze_factor);
	float4 loss4(0.1f,0.1f,0.1f,0.1f);
	float4 len4(horiz_dist_km,horiz_dist_km,horiz_dist_km,0);
	float4 total_ext4=-Ln(loss4)/len4;
	//float4 total_ext4=dens_factor*rayleigh+haze_factor*HazeMie+ozone_factor*Ozone;
	float4 sub_ext4=dens_factor*rayleigh+ozone_factor*Ozone;
	float4 required_haze4=(total_ext4-sub_ext4)/(haze_factor*Mie);
	float required_haze=(required_haze4.x+required_haze4.y+required_haze4.z)/3.f;
	return required_haze;
}

float4 Sky::GetIsotropicInscatter(float view_alt_km,float elevation,float dist_km,bool earth_test,int steps) const
{
	float4 inscatter=GetIsotropicInscatterFactor(view_alt_km,elevation,0,dist_km,earth_test,steps);
	inscatter*=GetInscatterAngularMultiplier(0,1);
	return inscatter;
}
float4 Sky::GetColourLossFactor(const float4 &view_pos_km,const float4 &pos_km,bool earth_test,int steps) const
{
	float4 diff=pos_km-view_pos_km;
	float dist_km=length(diff);
	float elevation=atan2(diff.z,sqrt(diff.x*diff.x+diff.y*diff.y));
	return GetIsotropicColourLossFactor(view_pos_km.z,elevation,0,dist_km,earth_test,steps);
}

float4 Sky::GetIsotropicInscatterFactor(const float4 &view_pos_km,const float4 &pos_km,bool earth_test,int steps) const
{
	float4 diff=pos_km-view_pos_km;
	float dist_km=length(view_pos_km-pos_km);
	float elevation=atan2(diff.z,sqrt(diff.x*diff.x+diff.y*diff.y));
	return GetIsotropicInscatterFactor(view_pos_km.z,elevation,0,dist_km,earth_test,steps);
}

float4 Sky::GetInscatter(const float4 &view_pos_km,const float4 &pos_km,bool earth_test,int steps) const
{
	float4 inscatter=GetIsotropicInscatterFactor(view_pos_km,pos_km,earth_test,steps);
	// The following applies ALL THE WAY ALONG any given ray:
	float4 VertexDir=pos_km-view_pos_km;
	if(length(VertexDir)<1e-12f)
		return inscatter;
	unit(VertexDir);
	float cos0=float4::dot(DirectionToSun,VertexDir);
	inscatter*=GetInscatterAngularMultiplier(cos0,inscatter.w,view_pos_km.z);
	return inscatter;
}

// The ozone scattering downwards at this altitude:
float Sky::GetOzoneAtAltitude(float alt_km) const
{
	static float ozone_layer_thickness_km=10.f;
	static float base_ozone=0.1f;
	float sigma=ozone_layer_thickness_km/2.f;
	float delta=(alt_km-OzoneLayerAltitude)/sigma;
	float distribution=base_ozone+(1.f-base_ozone)*exp(-delta*delta);
	
	float delta2=(alt_km)/2.f;
	distribution+=.2f*exp(-delta2*delta2);

	// RVK: low-level ozone:
	//distribution*=exp(-Haze/10.f);

	return distribution;
}
void Sky::ApplyFade(float4 &output,const float4 &pos_km,const float4 &inp)
{
	float4 VertexPos=inp;
	VertexPos*=1e-3f;	// working in METRES for fade!
	float4 VertexDir=VertexPos-pos_km;
	unit(VertexDir);
	float4 sunlight_colour;
	sunlight_colour=GetLocalIrradiance(VertexPos.z)+Starlight;
	float distance=length(VertexPos);
//should have min(space_dist,distance) here???
	float cos0=float4::dot(DirectionToSun,VertexDir);					// cosine of the forward scattering angle
	float4 ext;
	float4 extinction;
	float BetaRayleigh=0.0596831f*(1+cos0*cos0);			// rayleigh's phase function
	float g=MieEccentricity;
	float BetaMie=HenyeyGreenstein(g,cos0);//(1.f-g)*(1.f-g)/(4*pi)/pow(1+g*g-2*g*cos0,3.f/2.f);
	float4 BetaTotal;

	float step_length=distance/(float)Steps;
	Extinction(ext,extinction,step_length);
	for(int i=0;i<Steps;i++)
	{
		float dist=1.f*distance*(Steps-i-.5f)/(float)Steps;
		float4 Pos=VertexDir;
		float4 local_sun;
		Pos*=dist;
		local_sun=GetLocalIrradiance(Pos.z)+Starlight;
		float4 mie=Mie*GetHazeAtAltitude(Pos.z);
		BetaTotal=(BetaRayleigh*rayleigh+BetaMie*mie)/(extinction);
		Extinction(ext,extinction,distance);
		local_sun.x*=BetaTotal.x*(1.f-ext.x);
		local_sun.y*=BetaTotal.y*(1.f-ext.y);
		local_sun.z*=BetaTotal.z*(1.f-ext.z);
		output*=ext;
		output+=local_sun;
	}
	output.w=1.f;
}

float Sky::GetSunAzimuth() const
{
	return AzimuthToSunRadians;
}

float Sky::GetSunElevation() const
{
	return ElevationToSunRadians;
}

static double PositiveAngleRange(double a)
{
	while(a>2.*pi)
		a-=2.*pi;
	while(a<0.f)
		a+=2.*pi;
	return a;
}

static float PositiveAngleRangeLargeRange(double a)
{
	// because a may be HUGE, we need a fast way to reduce it to the correct range.
	double a_revolutions=a/(2.0*pi);
	int int_revolutions=(int)a_revolutions;
	a-=(double)(int_revolutions*(2.0*pi));
	return (float)(PositiveAngleRange(a));
}

static float SymmetricAngleRange(float a)
{
	while(a>pi)
		a-=2*pi;
	while(a<-pi)
		a+=2*pi;
	return a;
}


float Sky::GetSiderealTimeAtGreenwich() const
{
	float T = (GetDayNumber()-2.f ) / 36525.f;

	float theta0 = 280.46061837f*DEGREES + 360.98564736629f*DEGREES*(GetDayNumber()-2.f)
		+ 0.000387933f*DEGREES*T*T - T*T*T*DEGREES/38710000.f; // degrees
	return theta0; //Radians;
}

float Sky::GetLocalSiderealTime() const
{
	float theta0=GetSiderealTimeAtGreenwich();
	float theta = theta0 + SymmetricAngleRange(LongitudeRadians);//(eastern longitudes positive, western negative)
	return theta;
}

// LatitudeRadians: l
// solar declination d
// solar time t (decimal hours)
void Sky::GetSunPosition(float &az,float &el,float decl,float solar_hours)
{
	//AscensionDeclinationToAzimuthElevation(declination,3.1415926536f*0/12.f,*azimuth,*elevation);
	double tau=3.1415926536*(1.0+solar_hours/12.0);
	double sinl=sin(LatitudeRadians);
	double cosl=cos(LatitudeRadians);
	double sind=sin(decl);
	double cosd=cos(decl);
	//el =(float)(asin(sin(LatitudeRadians)*sin(decl) + cos(LatitudeRadians)*cos(decl)*cos(tau)));
	//el =(float)(asin(sinl*sin(decl) + cos(LatitudeRadians)*cos(decl)*cos(tau)));
	//el =(float)(asin(sinl*sind + cos(LatitudeRadians)*cos(decl)*cos(tau)));
	//el =(float)(asin(sinl*sind + cosl*cos(decl)*cos(tau)));
	az =(float)(atan2( - sin(tau) , (cos((double)LatitudeRadians)*tan(decl) - sin((double)LatitudeRadians)*cos(tau))));
	//float tau=3.1415926536f*solar_hours/12.f;*/

	el=(float)asin(sinl*sind+cosl*cosd*cos(tau));

	double y=-cosd*sin(tau);
	double x=cosl*sind-sinl*cosd*cos(tau);
	az=(float)atan2(y,x);
}

void Sky::AscensionDeclinationToAzimuthElevation(double decl,double RA,float &az,float &el) const
{
	// Hour angle:
	double tau = GetLocalSiderealTime() - RA;
	el =(float)(asin(sin(LatitudeRadians)*sin(decl) + cos(LatitudeRadians)*cos(decl)*cos(tau)));
	az =(float)(atan2( - sin(tau) , (cos((double)LatitudeRadians)*tan(decl) - sin((double)LatitudeRadians)*cos(tau))));
}

const float *Sky::SiderealToEarthMatrix() const
{
	// Right ascension is the celestial equivalent of terrestrial longitude
	// Declination is like latitude.
	float ST=GetLocalSiderealTime();
	float L=LatitudeRadians;
	simul::math::Matrix4x4 st_mat(	-cos(ST),-sin(ST),0,0,
									-sin(ST),cos(ST),0,0,
									0,0,1.f,0,
									0,0,0,1.f);
	simul::math::Matrix4x4 L_mat(	1.f,0,0,0,
									0,sin(L),-cos(L),0,
									0,cos(L),sin(L),0,
									0,0,0,1.f);
	static simul::math::Matrix4x4 trans;
	simul::math::Multiply4x4(trans,L_mat,st_mat);
	return trans;
}

void Sky::GetMoonPosition(float &az,float &el) const
{
//http://stjarnhimlen.se/comp/tutorial.html
	double d = GetDayNumber()+1.0;
    double N = 125.1228*DEGREES_D - 0.0529538083*DEGREES_D  * d;	// (Long asc. node)
    double i = 5.1454*DEGREES;									// (Inclination)
    double w = 318.0634*DEGREES_D + 0.1643573223*DEGREES_D  * d;	// (Arg. of perigee)
    double a = 60.2666;											// (Mean distance)
	double e = 0.054900;										// (Eccentricity)
    double Md = 115.3654*DEGREES_D + 13.0649929509*DEGREES_D * d;// (Mean anomaly)

	//double test1=PositiveAngleRange(16*pi-.113612);
	//double test2=PositiveAngleRangeLargeRange(16*pi-.113612);

	//double N_=PositiveAngleRange(N);
	N=PositiveAngleRangeLargeRange(N);
	i=(double)PositiveAngleRange(i);

	w=PositiveAngleRangeLargeRange(w);
	double M=PositiveAngleRangeLargeRange((double)Md);
	

	// Now compute E, the eccentric anomaly. We start with a first approximation (E0 and M in degrees):
	double E0 = M + e * sin(M) * (1.0 + e * cos(M));
	// Second approx:
	double E1 = E0 - (E0 - e * sin(E0) - M) / (1.0 - e * cos(E0));

	// Now we've computed E - the next step is to compute the Moon's distance and true anomaly. First we compute rectangular (x,y) coordinates in the plane of the lunar orbit:

	double x = a * (cos(E1) - e);
	double y = a * sqrt(1.0 - e*e) * sin(E1);

	// convert this to distance and true anomaly:
//    double r = sqrt( x*x + y*y );	// Earth radii
    double v = PositiveAngleRangeLargeRange(atan2(-y,-x));

	// Now we know the Moon's position in the plane of the lunar orbit.
	// To compute the Moon's position in ecliptic coordinates, we apply these formulae:

    double xeclip = ( cos(N) * cos(v+w) - sin(N) * sin(v+w) * cos(i) );
    double yeclip = ( sin(N) * cos(v+w) + cos(N) * sin(v+w) * cos(i) );
    double zeclip = sin(v+w) * sin(i);
	
	// ...In multiples of Earth radius.

	// Convert to ecliptic longitude, latitude, and distance:

	double r_horizontal=sqrt(xeclip*xeclip+yeclip*yeclip);
	double long_eclip=atan2(yeclip,xeclip);
	long_eclip=PositiveAngleRangeLargeRange(long_eclip);
	double lat_eclip =atan2(zeclip,r_horizontal);
	//double r_eclip=sqrt(xeclip*xeclip+yeclip*yeclip+zeclip*zeclip);
	// This could be a couple of degrees out due to perturbations.

	//Convert ecliptic latitude lat_eclip and longitude L to right ascension RA and declination delta

	// NOW calculate declination and right-ascension
   // obliquity of ecliptic:
    //double T		= (d-2.0)/36525.0;
    double eps	= 23.4393*DEGREES_D-3.563E-7*DEGREES_D* d;//(23.0 + 26.0/60.0 + 21.448/3600.0 - (46.815*T+ 0.00059*T*T- 0.001813*T*T*T)/3600.0)*pi/180.0;

	double X = cos(lat_eclip)*cos(long_eclip);
    double Y = cos(eps)*cos(lat_eclip)*sin(long_eclip) - sin(eps)*sin(lat_eclip);
    double Z =-sin(eps)*cos(lat_eclip)*sin(long_eclip) + cos(eps)*sin(lat_eclip);
    double R = sqrt(1.0-Z*Z);

    double decl	=atan2(Z,R);					// in radians
    double RA	=(24.0/pi)*atan2(Y,(X+R));		// in hours

	az=0;
	el=0;
	AscensionDeclinationToAzimuthElevation(decl,RA,az,el);
}

/*
void Sky::ColourTemperatureChanged()
{
	if(ColourTemperature)
	{
		double h=6.626e-34;
		double c=2.998e8;
		double k=1.38e-23;
		double wavelengths[] ={ColourWavelengthsNm.x,ColourWavelengthsNm.y,ColourWavelengthsNm.z};
		double radiance[3];
		double max_irr=0;
		for(int i=0;i<3;i++)
		{
			double lambda=wavelengths[i]/1000.0;// from nm to micrometres
			//double irr=2.0*pi*h*c*c/(pow(lambda,5.0)*exp(h*c/(k*lambda*ColourTemperature))-1.0);
			double irr=2.0*pi*c/(pow(lambda,5.0)/(k*lambda*ColourTemperature));
			radiance[i]=(float)irr;
			if(irr>max_irr)
				max_irr=irr;
		}
		if(max_irr)
		for(int i=0;i<3;i++)
			radiance[i]/=max_irr;
	}
}*/

unsigned Sky::GetAtmosphericPropertiesChecksum() const
{
	unsigned new_checksum=0;
	new_checksum+=*reinterpret_cast<const unsigned*>(&Density				);
	new_checksum+=*reinterpret_cast<const unsigned*>(&MieWavelengthExponent	);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Steps				);
	new_checksum+=*reinterpret_cast<const unsigned*>(&OzoneLevel		);
	new_checksum+=*reinterpret_cast<const unsigned*>(&ColourWavelengthsNm.x	);
	new_checksum+=*reinterpret_cast<const unsigned*>(&ColourWavelengthsNm.y	);
	new_checksum+=*reinterpret_cast<const unsigned*>(&ColourWavelengthsNm.z	);
	return new_checksum;
}

unsigned Sky::GetAtmosphericStateChecksum() const
{
	unsigned new_checksum=GetAtmosphericPropertiesChecksum();
	new_checksum+=*reinterpret_cast<const unsigned*>(&LatitudeRadians	);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Mie				);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Haze				);
	new_checksum+=*reinterpret_cast<const unsigned*>(&MieEccentricity		);
	new_checksum+=*reinterpret_cast<const unsigned*>(&Rayleigh		);
	new_checksum+=*reinterpret_cast<const unsigned*>(&HazeBaseHeightKm		);
	new_checksum+=*reinterpret_cast<const unsigned*>(&DirectionToSun.x	);
	new_checksum+=*reinterpret_cast<const unsigned*>(&DirectionToSun.y	);
	new_checksum+=*reinterpret_cast<const unsigned*>(&DirectionToSun.z	);
	new_checksum+=*reinterpret_cast<const unsigned*>(&SunIrradiance			);
	return new_checksum;
}

void Sky::StepsChanged()
{
	delete [] step_lengths;
	delete [] step_centres;
	step_lengths=new float[Steps];
	step_centres=new float[Steps];
	for(int i=0;i<Steps;i++)
	{
		float i1=1.f-(float)(i)/(float)Steps;
		i1*=i1;
		float i2=1.f-(float)(i+1)/(float)Steps;
		i2*=i2;
		step_lengths[i]=i1-i2;
	}
	for(int i=0;i<Steps;i++)
	{
		step_centres[i]=1.f-((float)i+0.5f)/(float)Steps;
		step_centres[i]*=step_centres[i];
	}
	RemakeSkyLookupTable();
}

void Sky::SetSunPosByDirection()
{
	float l=length(DirectionToSun);
	if(l)
		DirectionToSun=DirectionToSun/l;
	else
		DirectionToSun.Set(0,0,1.f,0);
	if(!AutomaticSunPosition)
	{
		ElevationToSunRadians=asin(DirectionToSun.z);
		ElevationToSunRadians=ElevationToSunRadians-GetSunRefraction(ElevationToSunRadians,0);
		for(int i=0;i<4;i++)
			ElevationToSunRadians=ElevationToSunRadians-GetSunRefraction(ElevationToSunRadians,0);
		AzimuthToSunRadians=atan2(DirectionToSun.x,DirectionToSun.y);
		UpdateSunPosition();
	}
}

void Sky::SetSunPosByAngle()
{
	if(!AutomaticSunPosition)
		BaseSky::SetSunPosByAngle();
}

void Sky::SetMoonPosByAngle()
{
	if(!AutomaticMoonPosition)
		BaseSky::SetMoonPosByAngle();
}
