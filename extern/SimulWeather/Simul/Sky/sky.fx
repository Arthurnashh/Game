//------------------------------------
// Un-Tweakable Variables
//------------------------------------

float4x4 modelViewProjection : WorldViewProjection;

//------------------------------------
// Tweakable Variables
//------------------------------------

// Eye position in model space
// Maya compatible annotations
float4 eyePosition : Position
<
	string object = "camera";
	string space = "world";
>;

float4 lightVector : Direction
<
> ={-.08f,-.995f,-.056f,0};

float hazeEccentricity=0.8;
float4 rayleigh= {0.00695f,0.0118f,0.0244f,0};
float4 mie={0.000565f,0.0007745f,0.00085f,0};
int steps=3;

float pi=3.1415926536;
float planetRadius=6378.0;
float atmosphereThickness=8.4;
float GetOpticalLength(float elevation,float altitude,float planet_rad,float atmosphere_thickness)
{
	if(altitude<0)
		altitude=0;
	float ac=planet_rad/(planet_rad+altitude);
	float horizon_angle=0.5*pi;
	//horizon_angle+=acos(ac);
	elevation=0.5*pi-elevation;
	elevation*=(0.5*pi)/horizon_angle;
	elevation=clamp(elevation,0,91*pi/180);
	float d=atmosphere_thickness/(cos(elevation)+0.15*pow(93.885-180/pi*elevation,-1.253));
	return d;
}
float4 GetSunlightColour(float4 LightVector,float4 Pos,float planet_radius,float atmosphere_thickness,float4 extinction)
{
	float sunlight=25;
	float4 output=float4(1,1,1,1);
	float4 NewVertical=Pos;
	NewVertical[1]+=planet_radius;
	float new_altitude=length(NewVertical)-planet_radius;
	NewVertical=normalize(NewVertical);
	float new_elevation_cosine=-dot(NewVertical,LightVector);
	float new_sun_elevation=0.5f*pi-acos(new_elevation_cosine);
	float illuminated_length=GetOpticalLength(new_sun_elevation,new_altitude,planet_radius,atmosphere_thickness);
	illuminated_length*=clamp((1-new_altitude/atmosphere_thickness),0.0,1.0);
	float new_optical_thickness=illuminated_length*exp(-0.18*new_altitude);
	output=exp(extinction*-new_optical_thickness);
	// How much of the sky above is in sunlight?
	
	float horizon_angle=acos(planet_radius/(planet_radius+max(new_altitude,0)));

	if(new_sun_elevation<horizon_angle)
	{
		float ce=cos(new_sun_elevation);
		float ch=cos(horizon_angle);
		float diff=(1-planet_radius/atmosphere_thickness*(1.0/ce-1.0/ch));
		float proportion=exp(0.1*diff);
		output*=clamp(proportion,0,1.0);
	}
	//output=float4(1,1,1,1);
	output*=sunlight;
	return output;
}
struct fragment
{
	float4 position : POSITION;
	float4 colour	: COLOR0;		// In-scattered Color
};
struct vertex
{
    float4 position : POSITION;
    float4 colour   : COLOR0;
};

fragment Sky_VS(
	vertex IN,
	uniform float4x4 ModelViewProjection,
	uniform float HazeEccentricity,
	uniform float4 LightVector,
	uniform float4 Rayleigh,
	uniform float4 Mie,
	uniform float planet_radius,
	uniform float atmosphere_thickness,
	uniform int Steps)
{
	float4 VertexDir=normalize(IN.position);
	VertexDir.w=0;
	LightVector.w=0;
	float elevation=asin(IN.position.y);
	fragment OUT;
//	OUT.position=mul(modelViewProjection,IN.position);
    OUT.position = mul( float4(IN.position.xyz , 1.0) , ModelViewProjection);
	float s=length(IN.position);
	float cos0=-dot((LightVector),VertexDir);
	float BetaRayleigh=0.0596831f*(1+cos0*cos0);
	float g=HazeEccentricity;
	float BetaMie=(1-g*g)/(4*pi)/pow(1+g*g-2*g*cos0,1.5);			// Mie's phase function
	float4 BetaTotal=(BetaRayleigh*Rayleigh+BetaMie*Mie)/(Rayleigh+Mie);

	float4 extinction=Rayleigh+Mie;
	float space_altitude=30;		// arbitrary
	float b=-2*planet_radius*cos(pi/2+elevation);
	float c=(planet_radius+space_altitude);
	c*=c;
	c-=planet_radius*planet_radius;
	float l=GetOpticalLength(elevation,0,planet_radius,atmosphere_thickness);
	float d=.5*(-b+sqrt(b*b+4*c));
	float distance_to_space=min(l,d);
	float one_step=distance_to_space/(float)Steps;
	float4 ext=exp(extinction*-one_step);
	float4 local_sun;
	OUT.colour=float4(0,0,0,0);
	for(int i=0;i<Steps;i++)
	{
		float dist=distance_to_space*(Steps-i-.5f)/Steps;
		float4 Pos=VertexDir;
		Pos*=dist;
		local_sun=GetSunlightColour(LightVector,Pos,planet_radius,atmosphere_thickness,extinction);
		local_sun.x*=BetaTotal.x*(1-ext.x);
		local_sun.y*=BetaTotal.y*(1-ext.y);
		local_sun.z*=BetaTotal.z*(1-ext.z);
		OUT.colour*=ext;
		OUT.colour+=local_sun;
	}	
	OUT.colour.a=1;
	return OUT;
}


//-----------------------------------
// Techniques
//------------------------------------

technique Sky
{
    pass p0
    {
        Zenable = true;
        ZWriteEnable = false;
        //CullMode = CW;
        VertexShader = compile vs_2_0 Sky_VS
        				(
        					modelViewProjection,
							hazeEccentricity,
							lightVector,
							rayleigh,
							mie,
							planetRadius,
							atmosphereThickness,
							steps
        				);
    }
}