#include "psvs.h"
#include "common.h"


//! Constant definition
#define VCS_HLSLDEFS(type,name,regtype,regnum) type name : register(regtype##regnum);
#define VCS_HLSLDEFA(type,name,regtype,regnum,dim) type name[dim] : register(regtype##regnum);
VSC_LIST(VCS_HLSLDEFS,VCS_HLSLDEFA)

// 
// //! Macro to create cX register format
// #define REG(regnum) c##regnum
// 

//! Register designed for passing the diffuse color and shadow intensity in alpha to PS
#define TEXCOORD_AMBIENT TEXCOORD6
#define TEXCOORD_SPECULAR TEXCOORD7
#define TEXCOORD_DIFFUSE_SI TEXCOORD4

// ===================================================================================
// NonTL drawing
// ===================================================================================

struct VS_OUTPUT_NONTL
{
  float4 Position             : POSITION;
  float4 oShadowMap           : TEXCOORD5;
  float4 Ambient              : TEXCOORD_AMBIENT;
  float4 Specular             : TEXCOORD_SPECULAR;
  float4 Diffuse              : TEXCOORD_DIFFUSE_SI;
  float4 TexCoord01           : TEXCOORD0;
  float4 x_x_nDotU_nDotL      : TEXCOORD3;
  float4 tVector3AndFog       : COLOR0;
  float4 oFog                 : COLOR1;
};

VS_OUTPUT_NONTL VSNonTL(
  in float4 vPosition   : POSITION,
  in float4 vAmbient    : COLOR0,
  in float4 vSpecular   : COLOR1,
  in float2 vTexCoord0  : TEXCOORD0,
  in float2 vTexCoord1  : TEXCOORD1
)
{
	VS_OUTPUT_NONTL output;
	output.Position.xyz = vPosition.xyz;
	output.Position.w = 1.0f;
	output.Position = output.Position / vPosition.w;
	output.Ambient = vAmbient;
	output.Specular = vSpecular;
	output.Diffuse = float4(0, 0, 0, 0);
	output.tVector3AndFog = float4(0, 0, 0, 1);
  output.oFog = float4(0, 0, 1, 1);
	output.TexCoord01.xy = vTexCoord0.xy;
	output.TexCoord01.zw = vTexCoord1.xy;
  output.x_x_nDotU_nDotL = 0;
	output.oShadowMap = 0; // shadow maps not used, but PS has them on input
  return output;
}


// register space shader with view, projection and texgen matrices
/*** it should do no harm - postprocess effects restore everything */

float4 PPEdgeU[4] : register(c0);
float4 PPEdgeV[4] : register(c4);
float4 PPEdgePosOffset : register(c8);

VSPP4T_OUTPUT VSPostProcessCustomEdge4T(
  in float3 vPosition  : POSITION,
  in float  vEdgeIndex : BLENDINDICES
)
{
	VSPP4T_OUTPUT output;
	output.Position.xyzw = vPosition.xyzz+PPEdgePosOffset;
	output.TexCoord0.x = PPEdgeU[vEdgeIndex].x;
	output.TexCoord0.y = PPEdgeV[vEdgeIndex].x;
	output.TexCoord1.x = PPEdgeU[vEdgeIndex].y;
	output.TexCoord1.y = PPEdgeV[vEdgeIndex].y;
	output.TexCoord2.x = PPEdgeU[vEdgeIndex].z;
	output.TexCoord2.y = PPEdgeV[vEdgeIndex].z;
	output.TexCoord3.x = PPEdgeU[vEdgeIndex].w;
	output.TexCoord3.y = PPEdgeV[vEdgeIndex].w;
  return output;
}




//--------------------------------------------------------------------------------------
/* _  _          _     _  _             
  | || |___ __ _| |_  | || |__ _ ______ 
  | __ / -_) _` |  _| | __ / _` |_ / -_)
  |_||_\___\__,_|\__| |_||_\__,_/__\___|                                      
*/
//--------------------------------------------------------------------------------------


struct VSOutHeatHaze
{
  float4 Position   		: POSITION;
  float4 TexCoordsZPower: TEXCOORD0;
	float2 TexCoordsVPOS	: TEXCOORD1;
};


VSOutHeatHaze VSNonTLHeatHaze(in float4 vPosition : POSITION, in float3 vTexCoord0  : TEXCOORD0)
{
	VSOutHeatHaze output;

	output.Position.xyz = vPosition.xyz;
	output.Position.w = 1.0f;

	output.TexCoordsZPower.xy = vTexCoord0.xy;
	output.TexCoordsZPower.z  = vPosition.w;
	output.TexCoordsZPower.w  = vTexCoord0.z;

	output.TexCoordsVPOS = vPosition.xy*float2(0.5, -0.5) + 0.5;
	
  return output;
}


//--------------------------------------------------------------------------------------
/* ___         _ ___              
  / __|___  __| | _ \__ _ _  _ ___
 | (_ / _ \/ _` |   / _` | || (_-<
  \___\___/\__,_|_|_\__,_|\_, /__/
                          |__/    
*/
//--------------------------------------------------------------------------------------

#define LensCenter 		VSC_GodRaysCenterSize.xy
#define LensSize			VSC_GodRaysCenterSize.zw

#define CornerLD			VSC_GodRaysCornerPoints[0]
#define CornerRD			VSC_GodRaysCornerPoints[1]
#define CornerLU			VSC_GodRaysCornerPoints[2]
#define CornerRU			VSC_GodRaysCornerPoints[3]


struct VSGodraysOut
{
	float4 m_vPos 			: POSITION;
	float4 m_vTexCoord	: TEXCOORD0;
};

struct VSGodraysShrinkOut : VSGodraysOut
{
	float4 m_vTexCoord1	: TEXCOORD1;
};


/*!
for frustum projection
*/
float3 GetEyeRayWS(float2 screencoord)
{
	return 1;
	
	float3 y0  = lerp(CornerLD, CornerRD, screencoord.x);
	float3 y1  = lerp(CornerLU, CornerRU, screencoord.x);

	float3 pos = lerp(y0, y1, screencoord.y);
	
	return pos;
}


#define NextPixel VSC_GodRaysTexOffset

VSGodraysShrinkOut VSGodRaysShrink(in float2 LensCorner : POSITION)
{
	VSGodraysShrinkOut output;

	output.m_vPos = float4(LensCorner, 1, 1);
	
	float2 tex = LensCorner*0.5 + 0.5;
	tex.y = 1.0 - tex.y;
	output.m_vTexCoord	= tex.xyxy + float4(1.0, 1.0, 3.0, 1.0)*NextPixel;
	output.m_vTexCoord1	= tex.xyxy + float4(1.0, 3.0, 3.0, 3.0)*NextPixel;

	return output;
}


VSGodraysOut VSGodRays(in float2 LensCorner : POSITION)
{
	//compute position
	float2 clipSpace = LensCenter + LensCorner*LensSize;
	
	float4 position;
	position.xy = clipSpace;
	position.zw = float2(1, 1);
	
	//position is already in normalized projection space
	VSGodraysOut output;

	output.m_vPos 			= position;
	output.m_vTexCoord	= (LensCorner*0.5 + 0.5).xyxy + VSC_GodRaysTexOffset;
//	output.m_vTexCoord.z		= GetEyeRayWS(clipSpace.xy*0.5 + 0.5).z;
	
	return output;	
}

//--------------------------------------------------------------------------------------
/* ___      _      
  | _ \__ _(_)_ _  
  |   / _` | | ' \ 
  |_|_\__,_|_|_||_|
*/
//--------------------------------------------------------------------------------------

#define mView				VSC_ViewMatrix
#define mProjection	VSC_ProjMatrix

/*
moving and time
*/
#define Sizes					CV_RAIN_VERT
#define DropWidth 		Sizes.x
#define DropHeight		Sizes.y
#define DropTime			Sizes.z
#define RainRadius 		Sizes.w

/*
lighting parameters
*/
#define LumTex  			CV_RAIN_LUM_TEX
#define SunLumInt		  LumTex.x
#define Luminance			LumTex.y
#define RelTime				LumTex.z

/*
in world space
*/
#define Direction 		CV_RAIN_DIR
//#define DissolveOffs	Direction.w

/*
move to view space by multiplying by centralized view matrix
*/
#define RLightDir  		CV_RAIN_LIGHTDIR
//#define DissolveMul		RLightDir.w

#define RDirection 		CV_RAIN_RDIR
#define DistWidthMul	RDirection.w

#define EyeDir 				CV_RAIN_EYEDIR

#define REyePos    		CV_RAIN_REYE
#define InvRadius 		REyePos.w

#define Params 				CV_RAIN_PARAMS
#define RandMove 			Params.x
#define RandSpeed 		Params.y
#define RandGrid			Params.z

#define Params2				CV_RAIN_PARAMS2
#define HalfTexPixel	Params2.x
#define InvTexDrops		Params2.y
#define TexDrops			Params2.z

#define ShadowMapCenter CV_RAIN_RAINMAPPOS
#define ShadowMapScale	CV_RAIN_RAINMAPSCALE

#define RainMapTCScale			CV_RAIN_RAINMAPSCALE.xy
#define RainMapTCOffset			CV_RAIN_RAINMAPPOS.xy

#define RainMapDepthScale		CV_RAIN_RAINMAPSCALE.z
#define RainMapDepthOffset	CV_RAIN_RAINMAPPOS.z


//--------------------------------------------------------------------------------------
/*!
move rain drop
*/
//--------------------------------------------------------------------------------------
float3 MoveRainDrop(float3 npos, float rand01)
{
	// move by force direction and time
	npos = npos - REyePos.xyz;

	float randSpeed = 1 - RandSpeed*rand01;
	npos.y -= RelTime*randSpeed;
	
	return npos;
}


//--------------------------------------------------------------------------------------
/*!
wrap
*/
//--------------------------------------------------------------------------------------
float3 WrapToNormalizedBox(float3 npos, float3 direction)
{
	//wrap to bounding box
	npos = (npos + 1)*0.5;
	npos = frac (npos);
	npos = npos * 2 - 1;

	//shear the box according the direction of wind
	//npos.xz = npos.xz - float2(1, 1)*direction.xz*(npos.y - 1);
	npos.xz = npos.xz - float2(1, 1)*direction.xz*(npos.y);
	
	
	return npos;
}


//--------------------------------------------------------------------------------------
/*!
create geometry
*/
//--------------------------------------------------------------------------------------
float3 CreateGeometryW(float3 dirCenter, float3 wpos, float2 corners, float dist, float rand, float3 transDir, float sizeMul)
{
	float3 side = cross(dirCenter, transDir);

	float distMul   = saturate(dist*InvRadius)*DistWidthMul;
	float2 geomMul  = 1 + sizeMul + float2(distMul, distMul*0.4);
	
	float2 move = float2(corners.x*DropWidth, corners.y*DropHeight)*geomMul;
	
	float3 cpos = side*move.x + transDir*move.y;

	return wpos + cpos;
}


//--------------------------------------------------------------------------------------
/*!
output 
*/
//--------------------------------------------------------------------------------------
struct RainSnowIO
{
	float4 vPos 			: POSITION;
	float4 vTexCoordLumZ : TEXCOORD0;
};


sampler2D sRainNoise 			: register(s0);
sampler2D sRainHeightMap 	: register(s1);


//--------------------------------------------------------------------------------------
/*!
vertex shader, create rain drop from center, later, use geometry shader to extract info
*/
//--------------------------------------------------------------------------------------
/*
RainSnowIO VSPostProcessRain(float3	vPos : POSITION, float4 vDir : NORMAL)
{
	#define RandVal01	(vDir.z/256)
	#define Corners 	vDir.xy
*/
RainSnowIO VSPostProcessRain(float4	vPos : POSITION, float2 vDir : NORMAL)
{
	#define RandVal01	vPos.w/256.0
	#define Corners 	vDir.xy

	
	RainSnowIO output = (RainSnowIO) 0;

	float3 normPos = vPos;
	float3 center  = MoveRainDrop(normPos, RandVal01);

	//compute direction of rain
	float3 direction  = Direction.xyz;

	if (RandMove > 0.0001)
	{
		float noise1 = tex2Dlod(sRainNoise, RelTime*0.005 + RandGrid*normPos.xzzz).rgb;
		float noise2 = tex2Dlod(sRainNoise, RelTime*0.004 + RandGrid*normPos.zxxx).rgb;
		float noise  = noise1 + noise2 - 1.0;
		
		direction = direction + RandMove*noise;
	}
	float3 rdirection = mul(direction, (float3x3) mView);
	
	
	//normalize drop position and recompute to camera relative world position
	float3 normCenter = WrapToNormalizedBox(center, direction);
	float3 relWPos = normCenter*RainRadius;
	
	float geomAlpha = 1.0;

	//--------------
	//sample height map of surounding objects -> transform of relWPos into rain map, depth buffer is linear in <0..1>
	//testing only the center of drop, in case of problems, we can test start and end points too
	float2 rainMapTC   = relWPos.xz*RainMapTCScale + RainMapTCOffset;

	float depthSample  = tex2Dlod(sRainHeightMap, rainMapTC.xyyy).r;
	float relObjHeight = DropHeight + RainMapDepthOffset - depthSample*RainMapDepthScale; //dropHeight should be added to RainMapDepthOffset on CPU side
	
	if (relObjHeight > relWPos.y)
	{
		output.vPos = float4(4,4,-2,1); //out of screen 
		return output;
	}
	
	//--------------
	//move to world position
	float3 vpos = mul(relWPos, (float3x3) mView);
	//TODO: some frustum test here possible but take care e.g. on ATI three/five tv system

	//create geometry in world position
	float distToCenter = length(vpos);
	float3 dirToCenter = vpos/distToCenter;

	//scattering simulation -> when looking through drop (eye to center DOT fall dir), make it more visible
	float dirScatter = saturate(5*(abs(dot (rdirection, dirToCenter)) - 0.8));
	dirScatter *= dirScatter;
	
	float3 vertex = CreateGeometryW(dirToCenter, vpos, Corners, distToCenter, RandVal01, rdirection, dirScatter);

	//projection
	float4 opos = mul(mProjection, float4(vertex, 1));
	output.vPos = opos;

	#ifdef REFRACT
		output.vZW.xy = opos.zw;	//needed for soft depth test
	#endif

	//texcoords
	float randOffs = ((int) (RandVal01*TexDrops))*InvTexDrops;
	output.vTexCoordLumZ.xy = Corners + 0.5;
	output.vTexCoordLumZ.x  = output.vTexCoordLumZ.x*InvTexDrops + randOffs;
	
	//lum, when looking to light from eyepos through the drop, make more lighter. Loop here through more lights possible
	float toSun01 = 0.5 + 0.5*dot(RLightDir.xyz, dirToCenter);
	float sunPower = pow(toSun01, 16);
	output.vTexCoordLumZ.z = Luminance + SunLumInt*sunPower;

	//alpha modification
	output.vTexCoordLumZ.w = 1 + dirScatter*2 + sunPower*2;

	return output;
}


//--------------------------------------------------------------------------------------
/*!
rain layer
*/
//--------------------------------------------------------------------------------------
struct RainLayerOutput
{
	float4 vPosition	: POSITION;
	float2 vTexCoord 	: TEXCOORD0;
	float3 vDir				: TEXCOORD1;
};


RainLayerOutput VSPostProcessRainLayer(in float3 vPosition : POSITION, in float2 vTexCoord : TEXCOORD)
{
	RainLayerOutput output;
	
	output.vPosition	= float4(vPosition.xy, 1, 1);
	output.vTexCoord	= vTexCoord;// + VSC_RainLayer_Offset;
	output.vDir 			= VSC_RainLayer_Corners[(int) vPosition.z];
	
  return output;
}



//--------------------------------------------------------------------------------------
/*!
enf shape stuff
*/
//--------------------------------------------------------------------------------------
float4 VSEnfDebugShape(float4 vPos: POSITION) : POSITION
{
	float3 vpos = mul(vPos, VSC_EnfShapeWorldMatrix);
	float3 wpos = mul(float4(vpos, 1), VSC_EnfShapeViewMatrix);
	float4 ppos = mul(VSC_EnfShapeProjMatrix, float4(wpos, 1));

	#ifdef PERSP
		ppos.zw += VSC_EnfShapeBias.zw * (1/ppos.z);
	#endif

	return ppos;
}

