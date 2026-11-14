#include "ps.h"

// we need a value which we can add 4x together with no overflow, and with some more headroom for error
#define MAX_HALF_4 (MAX_HALF/10/4)
// similar to MAX_HALF_4, but can be used when adding 16 values
#define MAX_HALF_16 (MAX_HALF_4/4)


//need to have VS constants in PS shader ;-) 
#define VCS_HLSLDEFS(type,name,regtype,regnum) type name : register(regtype##regnum);
#define VCS_HLSLDEFA(type,name,regtype,regnum,dim) type name[dim] : register(regtype##regnum);
VSC_LIST(VCS_HLSLDEFS,VCS_HLSLDEFA)


/*!
simple squares
*/
float sqr(float s)  { return s*s; }
float2 sqr2(float2 s)  { return s*s; }
float4 sqr4(float4 s)  { return s*s; }

/*!
struct to have screen world dirs in one place
*/
struct ScreenCornerWorldDir 
{
	float4 LeftTop;
	float4 RightTop;
	float4 LeftBottom;
	float4 RightBottom;
};


//-----------------------------------------------------------------------------
/*!
linear interpolation to acquire world dir of point
\param texCoord 	texture coordinates
\param dir				world dirs into each screen corner
*/
//-----------------------------------------------------------------------------
float3 GetPixelWorldDir(float2 texCoord, const ScreenCornerWorldDir dirs)
{
	float3 x1 = lerp(dirs.LeftTop, dirs.RightTop, texCoord.x);
	float3 x2 = lerp(dirs.LeftBottom, dirs.RightBottom, texCoord.x);

	return lerp(x1, x2, texCoord.y);
}

//-----------------------------------------------------------------------------
/*!
world position
*/
//-----------------------------------------------------------------------------
float3 GetPixelWorldPos (half2 texCoord, float depth, const ScreenCornerWorldDir dirs)
{
	return depth*GetPixelWorldDir(texCoord, dirs);
}


//-----------------------------------------------------------------------------
/*!
sample RT, written separately to simply test different sampling methods
\param samp 	sampler
\param uv 		uv
*/
//-----------------------------------------------------------------------------
half4 SampleRT(sampler2D samp, const half2 uv)
{
	return tex2Dlod(samp, uv.xyyy);
}


//-----------------------------------------------------------------------------
/*!
return normal vector in view space from other points
\param dirs 						world space directions to screen corners
\param sDepth						depth sampler 
\param centerDepth			depth in the pixel center
\param centerDir				world direction into pixel depth center, can be computed in VS to spare GPU time
\param centerUV					UV of pixel center
\param nextPixelOffset 	UV offset to next pixel
\param centerWPos				OUT, return world pos of pixel
*/
//-----------------------------------------------------------------------------
float3 GetNormalNeighbours(
	const ScreenCornerWorldDir dirs, 
	sampler2D sDepth,
	const float centerDepth, 
	const float3 centerDir, 
	const float2 centerUV, 
	const float2 nextPixelOffset, 
	out float3 centerWPos)
{
	float2 right  = centerUV + float2(nextPixelOffset.x, 0);
	float2 bottom = centerUV + float2(0, nextPixelOffset.y);

	float3 depths = centerDepth;
	depths.y = SampleRT(sDepth, right).x;
	depths.z = SampleRT(sDepth, bottom).x;
	
	centerWPos 				= depths.x*centerDir;
	float3 rightWPos  = depths.y*GetPixelWorldDir(right, dirs);
	float3 bottomWPos	= depths.z*GetPixelWorldDir(bottom, dirs);

	float3 dx = (rightWPos - centerWPos);
	float3 dy = (bottomWPos - centerWPos);

	return normalize(cross(dx, dy)).xzy;
}


// ===================================================================================
// Shader for universal blur
// ===================================================================================
#define PSBLUR_MAX_KERNEL_SIZE 11

sampler PSScene: register(s0);

static const int PSBlurKernelSize = 9;//: register(i0);
float4 PSBlurKernel[PSBLUR_MAX_KERNEL_SIZE]: register(c0);

float4 PSPostProcessGaussBlur(in float2 uv:TEXCOORD0): COLOR0
{
	float3 p = float4(0.0f, 0.0f, 0.0f, 0.0f);
	for(int s = 0; s < PSBlurKernelSize; s++)
	{
		p += tex2D( PSScene, float2(uv.x + PSBlurKernel[s].x, uv.y + PSBlurKernel[s].y)) * PSBlurKernel[s].a;
	}
	return float4(p,1);
}

sampler PSGamma: register(s1);
float4 PSSceneOffsets2[4]: register(c0);
float PSGammaBias: register(c4);

// input: HDR scene

float4 PSPostProcessBloomDownsample2(in half2 uv:TEXCOORD0): COLOR0
{
#if 1 //_VBS3 // Hotfix - input values are sometimes very wrong (infinite?) which break the whole downsizing and avg/max color determination
	float3 p =	(saturate(tex2D( PSScene, uv + PSSceneOffsets2[0] ).rgb) +
       			   saturate(tex2D( PSScene, uv + PSSceneOffsets2[1] ).rgb) +
       			   saturate(tex2D( PSScene, uv + PSSceneOffsets2[2] ).rgb) +
       			   saturate(tex2D( PSScene, uv + PSSceneOffsets2[3] ).rgb) ) * 0.25f;
#else
	float3 p =	(tex2D( PSScene, uv + PSSceneOffsets2[0] ).rgb +
       			tex2D( PSScene, uv + PSSceneOffsets2[1] ).rgb +
       			tex2D( PSScene, uv + PSSceneOffsets2[2] ).rgb +
       			tex2D( PSScene, uv + PSSceneOffsets2[3] ).rgb) * 0.25f;
#endif
  
  p = min(p,MAX_HALF_4);
  
  float aperture = ApertureControl(tex2D(sampler1,float2(0,0)).x);
  
  // the p will become p*aperture
  p *= aperture;
  
  // approximate perceived brightness
  half i = p.r*0.299+p.g*0.587+p.b*0.114;
  
  float overbrightF = saturate((i-PSGammaBias)*5);
 	return float4(min(p*overbrightF/aperture,MAX_HALF_4),1);
 	//return p * tex1D( PSGamma, imax + PSGammaBias);
}

float4 PSSceneOffsets4[4][4]: register(c0);

// input: output of previous bloom downsampling step

float4 PSPostProcessBloomDownsample4(in half2 uv:TEXCOORD0): COLOR0
{
	half3 p = float4(0.0f, 0.0f, 0.0f, 0.0f);

	for(int v = 0; v < 4; v++)
	{
		for(int h = 0; h < 4; h++)
		{
			p += tex2D( PSScene, uv + PSSceneOffsets4[v][h]);
		}
	}
	return half4(p * (1.0/16), 1);
}

sampler PSBlurredScene: register(s1);
float PSBlurScale: register(c0);
float PSImageScale: register(c1);

float4 PSPostProcessBloomCombine(in half2 uv:TEXCOORD0, in half2 uv2:TEXCOORD1): COLOR0
{
	return float4( tex2D(PSBlurredScene, uv).rgb * PSBlurScale + tex2D(PSScene, uv2).rgb * PSImageScale, 1);
}

// ===================================================================================
// Shaders designed for SB rendering
// ===================================================================================

#define VSM 0

float4 PSShadowBufferAlpha_Default(float4 tDiffuseMap:TEXCOORD0, float4 tPosition:TEXCOORD7) : COLOR0
{
  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);

  // Kill output in case of alfa
  clip(diffuseMap.a - 0.5);

  #if VSM
  // Write out the Z and Z^2 (for VSM)
  return float4(tPosition.z,tPosition.z*tPosition.z,1,1);
  #else
  return float4(tPosition.z,1,1,1);
  #endif
}

//////////////////////////////////////////////

half4 PSShadowBufferAlpha_nVidia(float4 tDiffuseMap:TEXCOORD0) : COLOR0
{
  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);

  // It doesn't matter what we write to color map
  return diffuseMap;
}

// ===================================================================================
// Shaders designed for depth map rendering
// ===================================================================================

float4 GetFinalDepth(float depth, float alpha)
{
	return float4(saturate(depth.xxx), alpha);
}


float4 PSInvDepthAlphaTest(float4 tDiffuseMap:TEXCOORD0, float4 tPosition:TEXCOORD7) : COLOR0
{
  // Kill output in case it is too close
  clip(tPosition.z - PSC_DepthClip.w);

  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);

  // Kill output in case of alfa
  clip(diffuseMap.a - 0.5);

  // Write out the inverse of Z  
  return GetFinalDepth(tPosition.z, 1);
}

//! pixel shader with no output
float4 PSEmpty() : COLOR0
{
  clip(-1); // kill them all!
  return 0;
}

//! pixel shader with no output (_Default variant)
float4 PSEmpty_Default() : COLOR0
{
  clip(-1); // kill them all!
  return 0;
}

//! pixel shader with no output (_nVidia variant)
float4 PSEmpty_nVidia() : COLOR0
{
  clip(-1); // kill them all!
  return 0;
}


// we are interested in primary texture alpha only, nothing else
float4 PSAlphaOnly(float2 tDiffuseMap:TEXCOORD0, float4 tPosition:TEXCOORD7) : COLOR0
{
  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);

  // Kill output, consider the alphaRef value
  clip(diffuseMap.a - PSC_X_X_X_AlphaRef.a);

  return GetFinalDepth(tPosition.z, diffuseMap.a);
}

float4 PSAlphaOnlyMod(float2 tDiffuseMap:TEXCOORD0, half4 alpha:TEXCOORDAMBIENT, float4 tPosition:TEXCOORD7) : COLOR0
{
  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);

  // Calculate the final alpha
  half finalAlpha = diffuseMap.a*alpha.a;

  // Kill output, consider the alphaRef value
  clip(finalAlpha - PSC_X_X_X_AlphaRef.a);

  return GetFinalDepth(tPosition.z, finalAlpha);
}

float4 PSAlphaOnlyModAToC(centroid float2 tDiffuseMap:TEXCOORD0, half4 alpha:TEXCOORDAMBIENT, float4 tPosition:TEXCOORD7) : COLOR0
{
  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);

  // Calculate the final alpha
  half finalAlpha = diffuseMap.a*alpha.a;
  //clip(finalAlpha-0.1);
  
  return GetFinalDepth(tPosition.z, finalAlpha);
}

float4 PSAlphaOnlyTree(float4 tDiffuseMap:TEXCOORD0, half4 alpha:TEXCOORDAMBIENT, float4 tPosition:TEXCOORD7) : COLOR0
{
  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);
  half4 normalMap = tex2D(sampler1, tDiffuseMap.wzyx);

  // GetColorDiffuseDiscreteNoise takes care of alpha computation and pixel discarding
  half4 color = GetColorDiffuseDiscreteNoise(diffuseMap,alpha.a,normalMap.a, false);
  return GetFinalDepth(tPosition.z, color.a);
}


float4 PSAlphaOnlyTreeAToC(float4 tDiffuseMap:TEXCOORD0, half4 alpha:TEXCOORDAMBIENT, float4 tPosition:TEXCOORD7) : COLOR0
{
  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);
  half4 normalMap = tex2D(sampler1, tDiffuseMap.wzyx);
  half4 color = GetColorDiffuseDiscreteNoise(diffuseMap,alpha.a,normalMap.a, true);
  //color.a = 0.95;
  //clip(color.a-0.5);
  return GetFinalDepth(tPosition.z, color.a);
}

float4 PSAlphaOnlyTreeAdv(float4 tDiffuseMap:TEXCOORD0, half4 alpha:TEXCOORDAMBIENT, float4 tPosition:TEXCOORD7) : COLOR0
{
    // Sample the diffuse map
    half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);
  #ifdef NEW_TREE_NOISE
    // GetColorDiffuseDiscreteNoiseAdv takes care of alpha computation and pixel discarding
    half4 color = GetColorDiffuseDiscreteNoiseAdv(diffuseMap,alpha.a, false);
  #else
    // sample NON map (for noise)
    half4 normalMap = tex2D(sampler1, tDiffuseMap.wzyx);
    // GetColorDiffuseDiscreteNoise takes care of alpha computation and pixel discarding
    half4 color = GetColorDiffuseDiscreteNoise(diffuseMap,alpha.a,normalMap.a, false);
  #endif

  return GetFinalDepth(tPosition.z, color.a);
}

float4 PSAlphaOnlyTreeAdvAToC(float4 tDiffuseMap:TEXCOORD0, half4 alpha:TEXCOORDAMBIENT, float4 tPosition:TEXCOORD7) : COLOR0
{
  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);
  // GetColorDiffuseDiscreteNoiseAdv takes care of alpha computation and pixel discarding
  half4 color = GetColorDiffuseDiscreteNoiseAdv(diffuseMap,alpha.a, true);

  return GetFinalDepth(tPosition.z, color.a);
}


/// light version of STerrain - only values output by alpha vertex shader are used here
struct STerrainAlphaOnly
{
  float4 tShadowBuffer           : TEXCOORDSR; //5
  float2 vPos                    : VPOS;
//   half4 ambientColor             : TEXCOORDAMBIENT; //6** not for alpha
//   half4 specularColor            : TEXCOORDSPECULAR; //7** not for alpha
//   half3 tLightLocal              : COLOR0; // ** not for alpha
//   half4 tPSFog                   : COLOR1; // ** not for alpha
  float4 tSatAndMask_GrassMap    : TEXCOORD4;
  float4 tColorMap_NormalMap     : TEXCOORD0;
  float4 tDetailMap_SpecularMap  : TEXCOORD1;
  float4 grassAlpha              : TEXCOORD2;
//   half3 tEyeLocal                : TEXCOORD3; // // ** not for alpha
};

float4 PSTerrainGrassAlphaX(STerrainAlphaOnly input): COLOR0
{
  // extend real input to match the signature of the body
  // the values we are are not used anyway, therefore they should not matter
  STerrain inBody;
  inBody.vPos  = input.vPos;
  inBody.ambientColor = 0;
  inBody.specularColor = 0;
  inBody.tShadowBuffer = 0;
  inBody.tLightLocal = 0;
  inBody.tPSFog  = 0;
  inBody.tSatAndMask_GrassMap = input.tSatAndMask_GrassMap;
  inBody.tColorMap_NormalMap = input.tColorMap_NormalMap;
  inBody.tDetailMap_SpecularMap  = input.tDetailMap_SpecularMap;
  inBody.grassAlpha  = input.grassAlpha;
  inBody.tEyeLocal = 0;
  
  const bool layers[6]={true,true,true,true,true,true};
  // texkill is the only thing we are interested in
  PFTerrainBodyX(inBody, layers, true, true, false, true, false, 0, false);
  // besides of that, we also need to return projected z for depth map
  // we assume input.specularColor is receiving projected position instead of usual specular
  //return float4(1.0f / input.specularColor.zzz, 1);
  return GetFinalDepth(input.tShadowBuffer.z, 1);
}


// ===================================================================================
// Post process
// ===================================================================================

VSPP_OUTPUT VSPostProcess(
  in float3 vPosition : POSITION,
  in float2 vTexCoord : TEXCOORD
)
{
	VSPP_OUTPUT output;
	output.Position.xyzw = vPosition.xyzz;
	output.TexCoord = vTexCoord;
  return output;
}

/// pass input directly to output
float4 PSPostProcessCopy(VSPP_OUTPUT input) : COLOR
{
  return tex2D(sampler0, input.TexCoord);
}


float4 BicubicSourceSize : register(c0);	// resampled/filtered texture size
float4 BicubicTexelSize: register(c1);	// size of texels in format (texelX, 0, 0, texelY)

sampler s0 : register(s0);
sampler s1 : register(s1);

void VSPostProcessRescaleBicubic(
  float4 iPos : POSITION0, float2 iUV	: TEXCOORD0,
	out float4 oPos	: POSITION0, out float2 oUV	: TEXCOORD0
)
{
	oPos = iPos.xyzz; //*float4(2,2,1,1);
	oUV.xy = iUV.xy;
}

float4 PSPostProcessRescaleBicubic(float2 iUV : TEXCOORD0) : COLOR0
{
  iUV += BicubicTexelSize.xw*float2(-0.5,-0.5);
	// determination of source texture coordinates
	float2 coeffCoord = iUV.xy * BicubicSourceSize.xy - float2(0.5, 0.5);

	// offsets and weights from coefficient texture
	float3 hgX = tex1D(s1, coeffCoord.x).xyz;
	float3 hgY = tex1D(s1, coeffCoord.y).xyz;

	// sampling coordinates 
	float2 coordSource10 = iUV.xy + hgX.x * BicubicTexelSize.xy;
	float2 coordSource00 = iUV.xy - hgX.y * BicubicTexelSize.xy;

	float2 coordSource11 = coordSource10 + hgY.x * BicubicTexelSize.zw;
	float2 coordSource01 = coordSource00 + hgY.x * BicubicTexelSize.zw;

	coordSource10 = coordSource10 - hgY.y * BicubicTexelSize.zw;
	coordSource00 = coordSource00 - hgY.y * BicubicTexelSize.zw;

	// four linear source texture fetches
	float4 texSource00 = tex2D(s0, coordSource00);
	float4 texSource10 = tex2D(s0, coordSource10);
	float4 texSource01 = tex2D(s0, coordSource01);
	float4 texSource11 = tex2D(s0, coordSource11);

	// lin. interpolation along y
	texSource00 = lerp(texSource00, texSource01, hgY.z);
	texSource10 = lerp(texSource10, texSource11, hgY.z);

	// lin. interpolation along x
	texSource00 = lerp(texSource00, texSource10, hgX.z);

	return texSource00;
}


// Shader to present thermal image (compose it from blurred and original version)
// _VBS3_TI
float4 BlurFactor : register(c0);
float4 PSPostProcessThermalPresent(VSPP_OUTPUT input) : COLOR
{
  float4 c = lerp(tex2D(sampler0, input.TexCoord), tex2D(sampler1, input.TexCoord), BlurFactor.w);
  c.rgb = pow((c.rgb+0.055)/1.055, 2.4);
  return c;
}

// Thermal visualisation shader
// _VBS3_TI
float4 TempRangeMin01_TempRangeMax01_InvTempRangeDiff_TIMode : register(c0);
float4 PSPostProcessThermal(VSPP_OUTPUT input) : COLOR
{
  // Decode the temperature from the RGB into 01 range
  float3 rgbEncodedTemperature = tex2D(sampler0, input.TexCoord).rgb;
  float temperature01 = (rgbEncodedTemperature.r + rgbEncodedTemperature.g + rgbEncodedTemperature.b) * 1.0f / 3.0f;

  // Stretch the temperature into the range that should be displayed and display it
  float intensity = saturate((temperature01 - TempRangeMin01_TempRangeMax01_InvTempRangeDiff_TIMode.x) * TempRangeMin01_TempRangeMax01_InvTempRangeDiff_TIMode.z);

  // Do the color conversion, write the output
  return float4(tex2D(sampler1, float2(intensity + (0.5f/TIConversionDimensionW), TempRangeMin01_TempRangeMax01_InvTempRangeDiff_TIMode.w / TIConversionDimensionH + (0.5f/TIConversionDimensionH))).rgb, 1.0f);
}

/////////////////////////////////////////////////////////////////////////////////////////////

float invWidth : register(c0);
float4 PSPostProcessGaussianBlurH(VSPP_OUTPUT input) : COLOR
{
  const int gaussSize = 5;
  half gaussV[5] = { 0.1016,  0.25,   0.2969, 0.25,   0.1016};
  float gaussP[5] = {-4,      -2,      0,      2,      4};

  half4 result = half4(0, 0, 0, 0);
  for (int i = 0; i < gaussSize; i++)
  {
    result += (half4)tex2D(sampler0, input.TexCoord + float2(invWidth * gaussP[i], 0)) * gaussV[i];
  }
  return result;
}

float invHeight : register(c0);
float4 PSPostProcessGaussianBlurV(VSPP_OUTPUT input) : COLOR
{
  const int gaussSize = 5;
  half gaussV[5] = { 0.1016,  0.25,   0.2969, 0.25,   0.1016};
  float gaussP[5] = {-4,      -2,      0,      2,      4};

  half4 result = half4(0, 0, 0, 0);
  for (int i = 0; i < gaussSize; i++)
  {
    result += (half4)tex2D(sampler0, input.TexCoord + float2(0, invHeight * gaussP[i])) * gaussV[i];
  }
  return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/*  ___ ___ ___ __  __ 
   / __/ __/ __|  \/  |
   \__ \__ \__ \ |\/| |
   |___/___/___/_|  |_|                  
*/
/////////////////////////////////////////////////////////////////////////////////////////////


float4 FOV : register(c0);
float4 SBTSize_invSBTSize_X_AlphaMult : register(c1);
float4x3 ShadowmapMatrix : register(c20);
float4 MaxDepth : register(c23);
float4 ShadowmapLayerBorder : register(c40);

half4 PSPostProcessSSSM_Default(VSPP_OUTPUT input) : COLOR
{
  // Retrieve the transformed position
  float4 transformedPosition;
  {
    // Z is what is written in the depth map (depth map contains 1/z - this is done to make DOF effects easy)
    // TODO: shadows processed in more passes - prefer optimization here over DOF
		transformedPosition.z = ShadowmapLayerBorder.y*tex2D(sampler0, input.TexCoord).r;

    // X and Y depends on the Z and FOV
    {
      // Get the screenspace XY in <-1,1>
      float2 screenspaceXY;
      screenspaceXY.x = input.TexCoord.x * 2.0f - 1.0f;
      //screenspaceXY.y = (1.0f - input.TexCoord.y) * 2.0f - 1.0f;
      screenspaceXY.y = 1.0f - input.TexCoord.y * 2.0f;

      // Get the transformed position XY
      transformedPosition.xy = screenspaceXY * FOV.xy * transformedPosition.z;
    }

    // W is supposed to be 1
    transformedPosition.w = 1.0;
  }

  // Calculate the shadow coefficient
  half shadowCoef = 1;
  half blurCoef = 0;
  if ((transformedPosition.z >= ShadowmapLayerBorder.w) && (transformedPosition.z < ShadowmapLayerBorder.x))
  {
    // Convert point into the shadow map
    float4 shadowMap;
    shadowMap.xyz = mul(transformedPosition, ShadowmapMatrix);
    shadowMap.w = transformedPosition.z;

		#if VSM
			// VSM  - simply fetch z and z^2 filtered from the map
			float4 vsm = tex2Dlod(samplerShadowMap, shadowMap.xyzw);
			
			float  fAvgZ  = vsm.r; // Filtered z
			float  fAvgZ2 = vsm.g; // Filtered z-squared

			// Standard shadow map comparison
			shadowCoef = ( shadowMap.z <= fAvgZ );
			

			// Use variance shadow mapping to compute the maximum probability that the
			// pixel is in shadow
			float variance = ( fAvgZ2 ) - ( fAvgZ * fAvgZ );
			
			const float epsilonVSM = 0.000020f;
			variance       = min( 1.0f, max( 0.0f, variance + epsilonVSM ) );
			
			float mean     = fAvgZ;
			float d        = shadowMap.z - mean;
			float p_max    = variance / ( variance + d*d );

			// To combat light-bleeding, experiment with raising p_max to some power
			// (Try values from 0.1 to 100.0, if you like.)
			shadowCoef = max(shadowCoef,pow( p_max, 2.0f ));

			// special case - shadow map is empty (farthers possible)
			if (fAvgZ>=1) 
				shadowCoef = 1;
    
		#else

			// Precomputed constants related to texture size
			const float texsize = PSC_SBTSize_invSBTSize_X_AlphaMult.x;
			const float invTexsize = PSC_SBTSize_invSBTSize_X_AlphaMult.y;

			// Calculate the fractional part of the texel addressing  
			float2 dxy = frac(shadowMap.xy * texsize);

			// Calculate weights for bilinear interpolation

			//float4 fxy = half4(dxy, half2(1,1)-dxy);

			float3 pnz = float3(+1,-1,0);
			float4 fxy = dxy.xyxy * pnz.xxyy + pnz.zzxx;

			// float4((1 - dxy.x) * (1 - dxy.y), (1 - dxy.x) * dxy.y, dxy.x * (1 - dxy.y), dxy.x * dxy.y)
			// float4(fxy.z       * fxy.w,       fxy.z       * fxy.y, fxy.x * fxy.w,       fxy.x * fxy.y)
			float4 weights = fxy.zzxx*fxy.wywy;
			// Get 4 shadow depths
			float4 shadowDepth = float4(
				tex2Dlod(samplerShadowMap, shadowMap.xyzw).r,
				tex2Dlod(samplerShadowMap, shadowMap.xyzw + float4(0,           invTexsize,0,0)).r,
				tex2Dlod(samplerShadowMap, shadowMap.xyzw + float4(invTexsize,  0,0,0)).r,
				tex2Dlod(samplerShadowMap, shadowMap.xyzw + float4(invTexsize,  invTexsize,0,0)).r
			);

			// where 1 is contained in the shadow map, we want to handle it as a practical infinity
			// any large number should do
			shadowDepth += (shadowDepth>=1)*1000;

			// Get 4 shadow/not shadow values
			float4 sc = shadowDepth>=shadowMap.z;

			// Use weights to get the shadow coefficient
			shadowCoef = dot(sc, weights);

		#endif

		#ifdef SSSMBLUR
			// Get the blur coefficient
			const float maxDiff = 0.1; // 10 cm difference decides
			const float invW = 1.0/1174.0;
			const float invH = 1.0/881.0;
			bool disableBlur = false;
			if (abs(1.0f / tex2D(sampler0, input.TexCoord + float2( invW * 5,  invH * 5)).r - transformedPosition.z) > maxDiff) disableBlur = true;
			if (abs(1.0f / tex2D(sampler0, input.TexCoord + float2(-invW * 5,  invH * 5)).r - transformedPosition.z) > maxDiff) disableBlur = true;
			if (abs(1.0f / tex2D(sampler0, input.TexCoord + float2(-invW * 5, -invH * 5)).r - transformedPosition.z) > maxDiff) disableBlur = true;
			if (abs(1.0f / tex2D(sampler0, input.TexCoord + float2( invW * 5, -invH * 5)).r - transformedPosition.z) > maxDiff) disableBlur = true;
			if (!disableBlur)
			{
				blurCoef = saturate(/*(1.0/10.0) **/ MaxDepth.w * dot(max(shadowMap.zzzz - shadowDepth, 0), 0.25) /*/ transformedPosition.z*/);
			}
		#endif
  
  }
  else
  {
    clip(-1);
  }

  // Write shadow value to output
  return half4(shadowCoef, blurCoef, 0, 1);
}

half4 PSPostProcessSSSM_nVidia(VSPP_OUTPUT input) : COLOR
{
#if _SM2
  return half4(1, 1, 0, 1);
#endif

  // Retrieve the transformed position
  float4 transformedPosition;
  {
    // Z is what is written in the depth map
    transformedPosition.z = 1.0f / tex2D(sampler0, input.TexCoord).r;

    // X and Y depends on the Z and FOV
    {
      // Get the screenspace XY in <-1,1>
      float2 screenspaceXY;
      screenspaceXY.x = input.TexCoord.x * 2.0f - 1.0f;
      screenspaceXY.y = (1.0f - input.TexCoord.y) * 2.0f - 1.0f;

      // Get the transformed position XY
      transformedPosition.xy = screenspaceXY * FOV.xy * transformedPosition.z;
    }

    // W is supposed to be 1
    transformedPosition.w = 1.0;
  }

  // Calculate shadow coefficient, sample the particular layer
  half shadowCoef = 1;
  if ((transformedPosition.z >= ShadowmapLayerBorder.w) && (transformedPosition.z < ShadowmapLayerBorder.x))
  {
    float3 shadowMap = mul(transformedPosition, ShadowmapMatrix);
    float4 nVidiaSTRQ = float4(shadowMap, 1);
    shadowCoef = tex2Dlod(samplerShadowMap, nVidiaSTRQ).r;
  }
  else
  {
#ifdef _XBOX
    return tex2D(sampler1, input.TexCoord);
#else
    clip(-1);
#endif
  }
#ifdef _XBOX
  shadowCoef *= tex2D(sampler1, input.TexCoord).r;
#endif

  // Return the shadow coefficient
  return half4(shadowCoef, 0, 0, 1);

//   // Visualise chessboard
//   float2 rounded = round(transformedPosition.xz);
//   return half4(fmod(rounded.x + rounded.y, 2) < 0.5, 0, 0, 1);
}

/////////////////////////////////////////////////////////////////////////////////////////////

half4 PSPostProcessSSSMStencil(VSPP_OUTPUT input) : COLOR
{
#ifdef SSSMBLUR
  return half4(0, 1, 0, 1);
#else
  return half4(0, 0, 0, 1);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////

float4 PSPostProcessSSSMBH(VSPP_OUTPUT input) : COLOR
{
  const int gaussSize = 5;
  half gaussV[5] = { 0.1016,  0.25,   0.2969, 0.25,   0.1016};
  float gaussP[5] = {-4,      -2,      0,      2,      4};

  half4 result = half4(0, 0, 0, 1);
  for (int i = 0; i < gaussSize; i++)
  {
    half4 original = (half4)tex2D(sampler0, input.TexCoord + float2(invWidth * gaussP[i], 0));
    result.r += original.r * gaussV[i];
    if (i == 0)
    {
      result.g = original.g;
    }
    else
    {
      result.g = min(result.g, original.g);
    }
  }
  return result;
}

float4 PSPostProcessSSSMBV(VSPP_OUTPUT input) : COLOR
{
  const int gaussSize = 5;
  half gaussV[5] = { 0.1016,  0.25,   0.2969, 0.25,   0.1016};
  float gaussP[5] = {-4,      -2,      0,      2,      4};

  half4 result = half4(0, 0, 0, 1);
  for (int i = 0; i < gaussSize; i++)
  {
    half4 original = (half4)tex2D(sampler0, input.TexCoord + float2(0, invHeight * gaussP[i]));
    result.r += original.r * gaussV[i];
    if (i == 0)
    {
      result.g = original.g;
    }
    else
    {
      result.g = min(result.g, original.g);
    }
  }
  return result;
}

float4 PSPostProcessSSSMBFinalBlur(VSPP_OUTPUT input) : COLOR
{
  //return tex2D(sampler0, input.TexCoord);
  float4 sssmOriginal = tex2D(sampler0, input.TexCoord);
  float4 sssmBlurred = tex2D(sampler1, input.TexCoord);
  return lerp(sssmOriginal, sssmBlurred, sssmOriginal.g) * float4(2, 1, 1, 1);
}

/////////////////////////////////////////////////////////////////////////////////////////////
/*  ___   ___  ___ 
   |   \ / _ \| __|
   | |) | (_) | _| 
   |___/ \___/|_|  
*/
/////////////////////////////////////////////////////////////////////////////////////////////


half Gauss(half2 sample)
{
  const float value = 1.0f - sqrt(0.5f);
  return value;

  //this is original computation, x and y are now always +-0.5 so doesn't need to compute it 
  //return 1.0f - sqrt(sample.x*sample.x + sample.y*sample.y)/* * 0.5*/;
}

// Blur radius for X and Y direction (values can be like this: float2(1/800 * 2, 1/600 * 2)), 
// in z is blur factor and in w is screen edge blur factor
float4 blurRadius : register(c0);
#define BlurScale			blurRadius.z
#define MaxFocalPlane blurRadius.w

// Inverse of the focal plane distance
float3 PlaneXBlur : register(c1);
#define FocalPlane		PlaneXBlur.x
#define XBlurMul			PlaneXBlur.y
#define XBlurPower		PlaneXBlur.z

static const int DOFSampleNum	 		= 4;
static const half2 DOFSamples[DOFSampleNum]	= 
{
	float2( 0.5,  0.5), 
	float2( 0.5, -0.5), 
	float2(-0.5, -0.5), 
	float2(-0.5,  0.5)
};


half4 PSPostProcessDOF(VSPP_OUTPUT input) : COLOR
{
	//additional blur in x, screen01 is 1 at screen vertical edges and 0 in center
	half2 screen01_2 = sqr2((abs(input.TexCoord.xy - 0.5f))*2.0);
	half edgeBlur    = sqrt(screen01_2.x + screen01_2.y)*XBlurMul;

  //calculate the depth of the center
  half centerDepth = tex2D(sampler2, input.TexCoord).r;

  // Calculate the blurFactor
  half blurFactor = 0.5f*(edgeBlur + saturate(BlurScale * abs(min(centerDepth, MaxFocalPlane) - FocalPlane.x)));

  // Fetch the color from Hi and Lo textures from the center;
  half3 colorLo = tex2D(sampler1, input.TexCoord).rgb;
  half3 colorHi = tex2D(sampler0, input.TexCoord).rgb;
  half3 color 	= lerp(colorHi, colorLo, blurFactor);

  // Create the center color
  half4 cOut = float4(color, 1.0);

  for (int i = 0; i < DOFSampleNum; i++)
  {
    half2 coord = input.TexCoord + (DOFSamples[i] * blurRadius.xy);

    // Calculate the depth of the sample
    half sampleDepth = tex2D(sampler2, coord).r;

		const half smartBlurScale = 0.1; // Coefficient to influence the blurrines with samples behind the center
    half smartBlurFactor = 1.0f - saturate(smartBlurScale * (sampleDepth - centerDepth));

    half blurFactor = 0.5f*(edgeBlur + saturate(BlurScale * abs(sampleDepth - FocalPlane)));

    // Fetch the color from Hi and Lo textures from the sample
    half3 colorHi = tex2D(sampler0, coord);
    half3 colorLo = tex2D(sampler1, coord);

    // Mix Hi and Lo color
    half3 color = lerp(colorHi, colorLo, blurFactor);

    // Weight of the sample depends on blurFactor and on weight of the sample according to it's distance
    // to center (with Gaussian values)
    half sampleWeight = blurFactor * Gauss(DOFSamples[i]) * smartBlurFactor;

    // Add sample color with weight
    cOut.rgb += color * sampleWeight;
    cOut.a += sampleWeight;
  }

  // Normalize the color
  return half4(cOut.rgb / cOut.a, 1);
}

/////////////////////////////////////////////////////////////////////////////////////////////

half4 PSPostProcessDistanceDOF(VSPP_OUTPUT input) : COLOR
{
  const int samples = 4;
  const float2 poisson[4] = {float2(0.5, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5), float2(-0.5, 0.5)};
	
  // Calculate the depth of the center
  half centerDepth = tex2D(sampler2, input.TexCoord).r;
  half blurFactor = saturate(BlurScale * (FocalPlane - centerDepth));

	half3 cOut = tex2D(sampler0, input.TexCoord).rgb;
	half localWeight = blurFactor*(1.0f - sqrt(0.5f));
	
	//quite a lot of scene is unaffected thus this could help
	[branch] if (blurFactor > 0.01)
  {
		half3 colorLo = tex2Dlod(sampler1, input.TexCoord.xyyy).rgb;
		cOut = lerp(cOut, colorLo, blurFactor);

		// Go through the poisson samples and accumulate the color
		[unroll] for (int i = 0; i < DOFSampleNum; i++)
		{
			float2 coord = input.TexCoord + (DOFSamples[i] * blurRadius);

			// Fetch the color from Hi and Lo textures from the sample
			half3 colorHi = tex2Dlod(sampler0, coord.xyyy);
			half3 colorLo = tex2Dlod(sampler1, coord.xyyy);
  
			cOut += lerp(colorHi, colorLo, blurFactor)*localWeight;
		}
		
		cOut /= 4*localWeight + 1;
	}

	return half4(cOut, 1);
}


/////////////////////////////////////////////////////////////////////////////////////////////
/* ___ ___   _   ___  
	/ __/ __| /_\ / _ \ 
	\__ \__ \/ _ \ (_) |
	|___/___/_/ \_\___/                      
*/
/////////////////////////////////////////////////////////////////////////////////////////////

/*!
pixel offsets
*/
//const float4 ssaoPixelOffsets : register(c191);
#define ssaoPixelOffsets				VSC_SSAOPar1
#define ssaoHalfScreenOffset		VSC_SSAOPar1.xy
#define ssaoHalfScreenOffsetX		VSC_SSAOPar1.x
#define ssaoHalfScreenOffsetY		VSC_SSAOPar1.y
#define ssaoHalfNoiseOffset		  VSC_SSAOPar1.zw
#define ssaoHalfDepthOffset		  VSC_SSAOPar1.zw

/*!
pixel scale
*/
//const float2 ssaoNoisePixelScale	: register(FREESPACE_START_REGISTER);
#define ssaoNoisePixelScale		  VSC_SSAOPar2.xy



/*!
SSAO vertex shader
*/
struct VS_SSAO 
{
	float4 vPos 			: POSITION;
	float4 vTexCoord	: TEXCOORD0;
	float3 vDir 			: TEXCOORD1;
};

//-----------------------------------------------------------------------------
VS_SSAO VSPostProcessSSAO(in float3 vPosition : POSITION, in float2 uv : TEXCOORD)
{
	VS_SSAO output;

	//move quad to exactly match the MSAA samples, 2xhalf screen because the clip space is <-1, 1>
	output.vPos 				= float4(vPosition.xy - 2*float2(ssaoHalfScreenOffsetX, -ssaoHalfScreenOffsetY), 0, 1);
	output.vTexCoord.xy	= uv;
	output.vTexCoord.zw	= uv*ssaoNoisePixelScale + ssaoHalfNoiseOffset;
	output.vDir 				= VSC_SSAOCorners[(int) vPosition.z];
	
	return output;
}


/*!
size of kernel to both sides
*/
#define SSAO_KERNEL_SIZE  2

/*!
blur offsets
*/
#define ssaoBlurPixelScreenOffset		VSC_SSAOPar3.xy
#define ssaoBlurPixelScreenOffsetX 	VSC_SSAOPar3.x
#define ssaoBlurPixelScreenOffsetY	VSC_SSAOPar3.y
#define ssaoBlurHalfScreenOffset		VSC_SSAOPar3.zw
#define depthBlurPixelScreenOffset	VSC_SSAOPar4.xy
#define depthBlurHalfScreenOffset		VSC_SSAOPar4.zw


struct PS_SSAOBlur
{
	float4 vDepthUV[SSAO_KERNEL_SIZE]	: TEXCOORD0;
	float4 vSSAOUV[SSAO_KERNEL_SIZE]	: TEXCOORD4;
	float4 vCenterSSAODepth						: TEXCOORD7;
};

struct VS_SSAOBlur : PS_SSAOBlur
{
	float4 vPos : POSITION;
};


//-----------------------------------------------------------------------------
VS_SSAOBlur VSPostProcessSSAOBlur(in float3 vPosition : POSITION, in float2 uv : TEXCOORD)
{
	VS_SSAOBlur output;

	output.vPos	= float4(vPosition.xy - float2(ssaoBlurPixelScreenOffsetX, -ssaoBlurPixelScreenOffsetY), 0, 1);

	output.vCenterSSAODepth.xy = uv + ssaoBlurHalfScreenOffset;
	output.vCenterSSAODepth.zw = uv + depthBlurHalfScreenOffset;
	
	int index = -SSAO_KERNEL_SIZE - 1;
	for (int i = 0; i < SSAO_KERNEL_SIZE; i++)
	{
		index++;
		output.vSSAOUV[i].xy	= uv + index*ssaoBlurPixelScreenOffset + ssaoBlurHalfScreenOffset;
		output.vDepthUV[i].xy	= uv + index*depthBlurPixelScreenOffset + depthBlurHalfScreenOffset;
		
		if (i == SSAO_KERNEL_SIZE/2)
			index++;
			
		index++;
		output.vSSAOUV[i].zw	= uv + index*ssaoBlurPixelScreenOffset + ssaoBlurHalfScreenOffset;
		output.vDepthUV[i].zw	= uv + index*depthBlurPixelScreenOffset + depthBlurHalfScreenOffset;
	}
	
	return output;
}


/*!
samplers binding
*/
#define ssaoScreen       sampler0
#define ssaoSSAO         sampler1
#define ssaoRandTexture  sampler2



/*!
to convert to screenpos
*/
const float4 ssaoPixelOffset	: register(c0);

/*!
to help with distance division
*/
const float4 ssaoDistDivParams	: register(c1);
#define ssaoDepthTexMul		      ssaoDistDivParams.xy
#define ssaoDeltaRadius		      ssaoDistDivParams.z
#define ssaoFar					    	  ssaoDistDivParams.w

const float4 ssaoAngleDist			: register(c2);
#define ssaoAngleK					  	ssaoAngleDist.x
#define ssaoAngleQ		   				ssaoAngleDist.y
#define ssaoMaxDistConst	   	 	ssaoAngleDist.z
#define ssaoOneConst						ssaoAngleDist.w

const float3 ssaoImageBlur		  : register(c3);
#define ssaoImageBias 			  	ssaoImageBlur.x
#define ssaoImageMul		 				ssaoImageBlur.y
#define ssaoMaxBlurDistConst   	ssaoImageBlur.z

const float4 ssaoFogParams		: register(c4);
#define ssaoFogEnd            ssaoFogParams.x
#define ssaoInvFogDelta       ssaoFogParams.y
#define ssaoFogExp            ssaoFogParams.z


/*!
rays
*/

//if using different length, it produces more random results, but looks more natural
#define SSAO_DIFF_LENGTH 	


//number of passes, pass is SSAO_MAX_RAYS rays sampling
#ifndef SSAO_PASSES
	#define SSAO_PASSES 		6
#endif

#define SSAO_MAX_RAYS		4

//0.5 multiplicator to have it more random, without it, the length of all rays are linearly converging to zero
#define ssaoBias  		(0.5*(1 - 1/SSAO_PASSES))
#define ssaoMul				((1 - ssaoBias)/SSAO_MAX_RAYS)

static const half2 ssao_ray[SSAO_MAX_RAYS] = 
{
	#ifdef SSAO_DIFF_LENGTH			
		half2(-1.0f,   0.0f)*(ssaoBias + 1*ssaoMul),
		half2( 0.0f,   1.0f)*(ssaoBias + 2*ssaoMul),
		half2( 1.0f,   0.0f)*(ssaoBias + 3*ssaoMul),
		half2( 0.0f,  -1.0f)*(ssaoBias + 4*ssaoMul)
	#else             
		half2(-1.0f,  0.0f),
		half2( 0.0f,  1.0f),
		half2( 1.0f,  0.0f),
		half2( 0.0f, -1.0f)
	#endif   
};


//-----------------------------------------------------------------------------
/*!
get random ray 
*/
//-----------------------------------------------------------------------------
half2 ssaoGetRandRay(int index, const half4 rotmatrix)
{
	return half2(dot(ssao_ray[index], rotmatrix.xy), dot(ssao_ray[index], rotmatrix.zw));
}


//-----------------------------------------------------------------------------
/*!
fog (VFogAlpha)
*/
//-----------------------------------------------------------------------------
float SSAOFog(float d)
{
  //linear -> return 1 - saturate(1.0 * (ssaoFogEnd - d) * ssaoInvFogDelta);
  return 1 - exp(d * ssaoFogExp);
}


ScreenCornerWorldDir SSAOWorldDirs	: register(c5);

//-----------------------------------------------------------------------------
/*!
SSAO with normal computation
*/
//-----------------------------------------------------------------------------
half4 PSPostProcessSSAO(const float4 Pixel : TEXCOORD0, const float3 vDir : TEXCOORD1) : COLOR
{
	#define CenterPosition 		Pixel.xy
	#define NoisePosition		  Pixel.zw

	//sample depth
	half centerDepth = SampleRT(ssaoScreen, CenterPosition).x;

	float3 viewCenterPos, worldNormal = GetNormalNeighbours(SSAOWorldDirs, ssaoScreen, centerDepth, vDir, CenterPosition, ssaoPixelOffset.xy, viewCenterPos);	
	worldNormal = normalize(worldNormal.xzy);

	//pixel scale according the distance with 1/z function, Z is now linear from 0 (near plane) until 1 !
	half2 xyScale = ssaoDepthTexMul/(centerDepth*ssaoDeltaRadius + ssaoFar);

	half2 next = float2(16.0/128.0, 0.0);
	half4 sum = 0;

	[unroll] for (int currPass = 0; currPass < SSAO_PASSES; currPass++)
	{
		//random ray
		half2 rotmatrix  = SampleRT(ssaoRandTexture, NoisePosition + next*currPass)*2 - 1;

		//this converges more linearly into smaller jitter values, more correct but worse result (sharper ssao)
		//half2 passScale  = xyScale*(currPass + 1)/SSAO_PASSES;
		half2 passScale  = xyScale;

		half4 cosAngle4, dist4;
		[unroll] for (int i = 0; i < SSAO_MAX_RAYS; i++)
		{
			//random ray and uv offset, in vector version this is faster than in scalar version
			//do not rotate first pass as this is faster
			half2 ray  = currPass == 0 ? ssao_ray[i] : reflect(ssao_ray[i], rotmatrix.xy);			
			half2 offs = ray.xy*passScale;

			//compute world position (in camera space) of a point
			half2 uv				= CenterPosition + offs;
			half currDepth	= SampleRT(ssaoScreen, uv).x;
			half3 currPos  	= GetPixelWorldPos(uv, currDepth, SSAOWorldDirs);
			
			//vector to point
			half3 viewVecToPixel = currPos - viewCenterPos;

			dist4[i]	 		= length(viewVecToPixel);// + 1e-6; division by zero shouldn't happen
			half3 normVec = viewVecToPixel/dist4[i];
			cosAngle4[i]  = dot(normVec, worldNormal);
		}

		half4 angleAtt4 = saturate(cosAngle4*ssaoAngleK + ssaoAngleQ);			
		half4 distA 	  =	ssaoMaxDistConst*dist4 + ssaoOneConst;
		half4 distAtt4	= 1/distA;
		
		sum += saturate(angleAtt4*distAtt4); 
	
		//this makes bigger jitter but looks better
		xyScale *= (1 - 1.0/SSAO_PASSES);
	}
	
	sum.x = dot(1, sum);

	half intensity  = ssaoImageMul*centerDepth + ssaoImageBias;
	half finalValue = 1 - intensity*sum.x/(SSAO_MAX_RAYS*SSAO_PASSES);

	//final influence according the distance
	half infl = LinearFunc(centerDepth.x, 1.0, 0.8);

	return lerp(finalValue, 1, saturate(infl + SSAOFog(centerDepth.x)));
}


//-----------------------------------------------------------------------------
/*!
blur
*/
//-----------------------------------------------------------------------------
half4 SSAOBlur(PS_SSAOBlur input, bool vertical, bool depthtest)
{
	half centerDepth = depthtest ? SampleRT (ssaoScreen, input.vCenterSSAODepth.zw).x  /*- 0.05/250/*+ ssaoMinDistance*/ : 0;
	half centerSSAO  = SampleRT(ssaoSSAO, input.vCenterSSAODepth.xy).x;

	half sum = centerSSAO;
	half weightsum = 1;
	half weight = 1;

	[unroll] for (int i = 0; i < SSAO_KERNEL_SIZE*2; i++)
	{
		int index = i/2;
		half2 uv  = i%2 ? input.vSSAOUV[index].zw : input.vSSAOUV[index].xy;
		
    half ssao = SampleRT(ssaoSSAO, uv).x;
		
		//bilateral filtering according the depth
    if (depthtest)
   	{
      half2 uvDepth = i%2 ? input.vDepthUV[index].zw : input.vDepthUV[index].xy;
      half depth    = SampleRT(ssaoScreen, uvDepth).x;
			
      half zDiff = abs(depth - centerDepth);
			weight = saturate(1/(1 + ssaoMaxBlurDistConst*zDiff));
    }
		
		weightsum += weight;
		sum 		  += weight*ssao;
	}
	
	return (half4) saturate(sum/weightsum);		
}


//-----------------------------------------------------------------------------
/*!
vertical blur
*/
//-----------------------------------------------------------------------------
half4 PSPostProcessSSAOBlurVert(PS_SSAOBlur input) : COLOR
{
	return SSAOBlur(input, true, false);
}

half4 PSPostProcessSSAOBlurVertDepth(PS_SSAOBlur input) : COLOR
{
	return SSAOBlur(input, true, true);
}

//-----------------------------------------------------------------------------
/*!
horizontal blur
*/
//-----------------------------------------------------------------------------
half4 PSPostProcessSSAOBlurHorz(PS_SSAOBlur input) : COLOR
{
  return SSAOBlur(input, false, false);
}

half4 PSPostProcessSSAOBlurHorzDepth(PS_SSAOBlur input) : COLOR
{
  return SSAOBlur(input, false, true);
}


//-----------------------------------------------------------------------------
/*!
final, just return, blend is done using alpha blend with:
normal:	src = 0; 				dest = destcolor*srccolor
debug:  src = srccolor; dest = 0
*/
//-----------------------------------------------------------------------------
half4 PSPostProcessSSAOFinal(const float4 Pixel : TEXCOORD0) : COLOR
{
  return (half4) tex2D(ssaoSSAO, Pixel.xy).x;
}

half4 PSPostProcessSSAODebug(const float4 Pixel : TEXCOORD0) : COLOR
{
  return half4 (tex2D(ssaoSSAO, Pixel.xy).xyz*0.5, 1);
}



/*!
SSAO downsampling
*/
struct VS_SSAODown
{
	float4 m_vPos 			: POSITION;
	float4 m_vTex1Tex2	: TEXCOORD0;
	float4 m_vTex3Tex4	: TEXCOORD1;
};

VS_SSAODown VSPostProcessSSAODown(in float3 vPosition : POSITION, in float2 uv : TEXCOORD)
{
	VS_SSAODown output;

	output.m_vPos 			  = vPosition.xyzz;

  float2 uv2            = uv + ssaoHalfScreenOffset;
	output.m_vTex1Tex2.xy = uv2;
	output.m_vTex1Tex2.zw	= uv2 + float2(ssaoPixelOffsets.z , 0.0);
	output.m_vTex3Tex4.xy = uv2 + float2(0.0, ssaoPixelOffsets.w);
	output.m_vTex3Tex4.zw = uv2 + float2(ssaoPixelOffsets.z , ssaoPixelOffsets.w);
	
	return output;
}


half4 GetAverage(sampler2D samp, half4 coord12, half4 coord34)
{
	return 0.25*(
		tex2Dlod(samp, coord12.xyyy) + 
		tex2Dlod(samp, coord12.zwww) + 
		tex2Dlod(samp, coord34.xyyy) + 
		tex2Dlod(samp, coord34.zwww));
}

/*!
SSAO downsample pixel shader 
*/
half4 PSPostProcessSSAODown(const float4 tex1tex2 : TEXCOORD0, const float4 tex3tex4 : TEXCOORD1) : COLOR
{
	return GetAverage(ssaoScreen, tex1tex2, tex3tex4);
}


/////////////////////////////////////////////////////////////////////////////////////////////
/* ___         _ ___              
  / __|___  __| | _ \__ _ _  _ ___
 | (_ / _ \/ _` |   / _` | || (_-<
  \___\___/\__,_|_|_\__,_|\_, /__/
                        |__/    
*/
/////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------
/*!
mask, just output kernel texture
*/
half4 PSPostProcessGodRaysMask(in half2 vTexCoord : TEXCOORD0) : COLOR
{
  return tex2D (sampler0, vTexCoord);
}


//-------------------------------------------------------------------------------------------
/*!
mask, just output kernel texture
*/
half4 PSPostProcessGodRaysShrink(in float4 coord12 : TEXCOORD0, in float4 coord34 : TEXCOORD1) : COLOR
{
  return GetAverage(sampler0, coord12, coord34);
}

const float4 graysDiletatePixels[5]		: register(c0);


//-------------------------------------------------------------------------------------------
/*!
diletation of mask
*/
half4 PSPostProcessGodRaysDiletate(in float2 vTexCoord : TEXCOORD0) : COLOR
{
	half maxVal = 0;
	[unroll] for (int i = 0; i < 9; i++)
	{
		half2 coordOffset = ((i%2 == 0) ? graysDiletatePixels[i/2].xy : graysDiletatePixels[i/2].zw);
	
		maxVal = max(maxVal, tex2Dlod(sampler0, (vTexCoord + coordOffset).xyyy).x);
	}

  return (half4) maxVal;
}

//-------------------------------------------------------------------------------------------
/*!
mask, just output kernel texture
*/
half4 PSPostProcessGodRaysMaskNoHor(VSPP_OUTPUT input) : COLOR
{
  return 1;//tex2D (sampler0, input.TexCoord);
}

//-------------------------------------------------------------------------------------------
const float3 graysSunPosition		: register(c0);
#define graysInvHeight					graysSunPosition.z

const float2  graysSteps				: register(c1);

const float4 graysTexScale			: register(c2);
#define BlurScreenToTexScale 		graysTexScale.xy
#define BlurScreenToTexOffset		graysTexScale.zw


/*!
radial blur
*/
half4 PSPostProcessGodRaysBlur(in half2 vTexCoord : TEXCOORD0, in half2 ScreenPos : VPOS) : COLOR
{
	half2 actPos  	= ScreenPos.xy*BlurScreenToTexScale + BlurScreenToTexOffset;
	half2 rayToSun  = graysSunPosition.xy - ScreenPos.xy;

	half2 rayStep 	= rayToSun*graysSteps; 
	
	float occluded	= 0.0;

	const int NumSteps = 8;	
	[unroll] for (int j = 0; j < NumSteps/4; j++)
	{
		half4 res;
		
		[unroll] for (int i = 0; i < 4; i++, actPos += rayStep)
		{
			res[i] = tex2Dlod(sampler0, actPos.xyyy);
		}
		
		occluded += dot(res, 1.0/NumSteps);
	}

	return (half4) occluded;
}

//-------------------------------------------------------------------------------------------
const float4 GRParams 		: register(c1);
//#define graysIntensity	GRParams.x  //inside graysColor
#define graysOverBurn	GRParams.y
#define graysOverBurnK	GRParams.z
#define graysOverBurnQ	GRParams.w


const float3 graysColor		: register(c2);

/*!
final computation
*/
half4 PSPostProcessGodRaysFinal(in half2 vTexCoord : TEXCOORD0, in half2 ScreenPos : VPOS) : COLOR
{
	half4 screen = tex2Dlod(sampler1, vTexCoord.xyyy);
  //half mask  	 = tex2Dlod(sampler0, vTexCoord.xyyy).r;
	half3 mask  	 = tex2Dlod(sampler0, vTexCoord.xyyy);

	half linFallOff = saturate(graysInvHeight*distance(graysSunPosition, ScreenPos));
	half overBurn = graysOverBurn*(saturate(linFallOff*linFallOff*graysOverBurnK + graysOverBurnQ)); 
	
//  return half4(screen*0.2 + overBurn);
	
	half3 godRays = graysColor*mask*(1 + overBurn);
	
  return half4(screen + godRays*saturate(1 - screen.rgb*screen.rgb), screen.a);
}


/////////////////////////////////////////////////////////////////////////////////////////////
/* ___      _      
  | _ \__ _(_)_ _  
  |   / _` | | ' \ 
  |_|_\__,_|_|_||_|
*/
/////////////////////////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------------------------
sampler2D sRainMap 										: register(s0);

const float4 rainMapDiletatePixels[5]	: register(c0);

//TODO: almost similar like in godrays, make it together
half4 PSPostProcessRainMap(VSPP_OUTPUT input) : COLOR
{
//	return (half4) tex2Dlod(sRainMap, input.TexCoord.xyyy).x;

	half minVal = tex2Dlod(sRainMap, (input.TexCoord.xy + rainMapDiletatePixels[4].xy).xyyy).x;

	half4 coord;
	coord = rainMapDiletatePixels[0] + input.TexCoord.xyxy;
	minVal = min(minVal, tex2Dlod(sRainMap, coord.xyyy).x);
	minVal = min(minVal, tex2Dlod(sRainMap, coord.zwww).x);	

	coord = rainMapDiletatePixels[1] + input.TexCoord.xyxy;
	minVal = min(minVal, tex2Dlod(sRainMap, coord.xyyy).x);
	minVal = min(minVal, tex2Dlod(sRainMap, coord.zwww).x);	

	coord = rainMapDiletatePixels[2] + input.TexCoord.xyxy;
	minVal = min(minVal, tex2Dlod(sRainMap, coord.xyyy).x);
	minVal = min(minVal, tex2Dlod(sRainMap, coord.zwww).x);	

	coord = rainMapDiletatePixels[3] + input.TexCoord.xyxy;
	minVal = min(minVal, tex2Dlod(sRainMap, coord.xyyy).x);
	minVal = min(minVal, tex2Dlod(sRainMap, coord.zwww).x);	

	return (half4) minVal;
}


//-------------------------------------------------------------------------------------------
sampler2D sRainTexture 			: register(s0);
sampler2D sScreen 					: register(s2);

float4 RainColor 						: register(c0);
#define RainIntensity 			RainColor.w

float4 RainScreenScaleOffs	: register(c1);
#define RainScreenScale			RainScreenScaleOffs.xy
#define RainScreenOffset		RainScreenScaleOffs.zw

float4 RainRefractOffs			: register(c2);

half4 PSPostProcessRain(in half4 vTexCoord : TEXCOORD0, in half2 ScreenPos : VPOS) : COLOR
{
	half4 tex    = tex2D(sRainTexture, vTexCoord.xy);
	half refract = (tex.xy - 0.5)*RainRefractOffs;
	half opacity = tex.w;
	
	//refraction sampling
	half2 uv = ScreenPos*RainScreenScale + RainScreenOffset;
	half4 pixel = tex2Dlod(sScreen, (uv + refract).xyyy);
	
	//luminance of pixel, but not eye comaptible ! blue sky makes problems
	half lum = dot(pixel.rgb, 0.33);
	
	//make a little brighter at dark colors and darker at bright colors
	half newLum = max(0.2*lum, 10*LinearFunc(lum, 0, 0.5));

	//a little to gray, not full color
	half3 color = /*RainColor.rgb*/(pixel.rgb*0.4 + lum*0.6)*newLum*vTexCoord.z;

	return half4(color, opacity);
}


float4 RainEyePosNear			: register(c3);
#define RainEyePos 				RainEyePosNear.xyz
#define RainFarPlane 			RainEyePosNear.w

ScreenCornerWorldDir RainWorldDirs	: register(c4);

sampler2D sRainLinDepth 	: register(s0);


//-------------------------------------------------------------------------------------------
half4 PSPostProcessRainLayer(in float2 vTexCoord : TEXCOORD0, in float3 vDir : TEXCOORD1, in half2 ScreenPos : VPOS): COLOR0
{
	float depth = tex2Dlod(sRainLinDepth, vTexCoord.xyyy);
	
	[BRANCH] if (depth >= 1)
		return 0;
		
	float3 viewPos, worldNormal = GetNormalNeighbours(RainWorldDirs, sRainLinDepth, depth, vDir, vTexCoord, RainScreenScale, viewPos);	
	float3 worldPos = viewPos + RainEyePos;

	half x = frac(worldPos.x) < 0.05 ? 1 : abs(worldNormal.x);
	half z = frac(worldPos.y) < 0.05 ? 1 : abs(worldNormal.z);
	half y = frac(worldPos.z) < 0.05 ? 1 : abs(worldNormal.y);

	return half4(x, y, z, 0.25);
}


/////////////////////////////////////////////////////////////////////////////////////////////
/*  ___      _              ___ _                      
   |   \ ___| |__ _  _ __ _/ __| |_  __ _ _ __  ___ ___
   | |) / -_) '_ \ || / _` \__ \ ' \/ _` | '_ \/ -_|_-<
   |___/\___|_.__/\_,_\__, |___/_||_\__,_| .__/\___/__/
                    |___/              |_|            
*/
/////////////////////////////////////////////////////////////////////////////////////////////


float4 cColor: register(c0);

half4 PSEnfDebugShape(): COLOR0
{
	return cColor;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/*   ___ _ _       ___          _      
    | __(_) |_ __ / __|_ _ __ _(_)_ _  
    | _|| | | '  \ (_ | '_/ _` | | ' \ 
    |_| |_|_|_|_|_\___|_| \__,_|_|_||_|                                  
*/
/////////////////////////////////////////////////////////////////////////////////////////////

/*!
to convert noise texture to pixel size 
*/
const float4 ScreenNoise 		: register(c0);
#define ScreenNoiseScale		ScreenNoise.xy
#define ScreenNoiseOffset		ScreenNoise.zw

/*!
parameters of effect
*/
const float4 Params				: register(c1);
#define Intensity         Params.x
#define Sharpness         Params.y
#define GrainSize         Params.z


/*!
parameters of effect, intensity line equation
*/
const float4 Params2			: register(c2);
#define LineK             Params2.x
#define LineQ             Params2.y


/*!
compute noise
\param noise	noise from texture in <0, 1>
*/
half3 ComputeFilmGrain(half3 noise, half3 screensample)
{
  //noise is uniformly distributed from <0, 1>, convert to <-1.0, 1.0>
  noise = (noise - 0.5)*2.0;

  //make power of absolute value
  half3 pownoise = pow (abs(noise), Sharpness);

  //back to <-1.0, 1.0>
  noise = sign(noise)*pownoise;

  //apply intensity 
  noise *= Intensity;

  //noise more visible in black, make quadtratic function
  //half3 blackmul  = saturate(BlackOffset - screensample);
  //noise *= blackmul*blackmul;

  //now, try to use linear interpolation, but the line is user specific
  const half3 lumcoef = half3(0.299, 0.587, 0.114);
  half lum            = dot(lumcoef, screensample);
  half lumintensity   = saturate(lum*LineK + LineQ);

  noise *= lumintensity;

  return noise;
}


/*!
color noise
*/
half4 PSPostProcessFilmGrainColor(VSPP_OUTPUT input) : COLOR
{
  half4 screensample 	= tex2D (sampler0, input.TexCoord);
  half3 noise 			= tex2Dlod (sampler1, float4(input.TexCoord*ScreenNoiseScale + ScreenNoiseOffset, 0, GrainSize));

  noise = ComputeFilmGrain(noise, screensample.rgb);
  return half4(screensample.rgb + noise, screensample.a);
}


/*!
monochromatic noise
*/
half4 PSPostProcessFilmGrainMono(VSPP_OUTPUT input) : COLOR
{
  half4 screensample 	= tex2D (sampler0, input.TexCoord);
  half3 noise 			= tex2Dlod (sampler1, float4(input.TexCoord*ScreenNoiseScale + ScreenNoiseOffset, 0, GrainSize));

  // .xxx is important, the compiler optimize the code and the pow function is called just once
  //for one channel instead for each of them -> about 7 instructions less
  noise = ComputeFilmGrain(noise, screensample.rgb).xxx;

  return half4(screensample.rgb + noise, screensample.a);
}


/////////////////////////////////////////////////////////////////////////////////////////////
/*   _  _ ___  ___ 
    | || |   \| _ \
    | __ | |) |   /
    |_||_|___/|_|_\
*/
/////////////////////////////////////////////////////////////////////////////////////////////

/// calculate and downsample luminace
/**
input: HDR image (RGB respresents color)
output: luminance in all channels
*/
half4 PSPostProcessDownSampleLuminance(VSPP4T_OUTPUT input) : COLOR
{
  // note: PS 2.0 have 16 texture samplers
  // using this and bilinear filtering we could perform 8x8 downsampling in one pass
  // this would require some offset calculation in the shader
  half3 x1 = tex2D(sampler0, input.TexCoord0);
  half3 x2 = tex2D(sampler1, input.TexCoord1);
  half3 x3 = tex2D(sampler2, input.TexCoord2);
  half3 x4 = tex2D(sampler3, input.TexCoord3);
#if 1 //_VBS3 // Hotfix - input values are sometimes very wrong (infinite?) which break the whole downsizing and avg/max color determination
  x1 = saturate(x1);
  x2 = saturate(x2);
  x3 = saturate(x3);
  x4 = saturate(x4);
#endif
  // note: this is daytime sensitivity, night time would be different (blue shift)
  half3 eyeSensitivityD4 = half3(0.299f/4, 0.587f/4, 0.114f/4);
  half luminance = dot(x1+x2+x3+x4,eyeSensitivityD4);
  // make sure the result fits in the half range
  luminance = min(luminance,MAX_HALF_4);
  return half4(luminance.xxx,1);
  // debugging - preserve colors from original image
  //return float4((x1+x2+x3+x4)*0.25,luminance);
}

/// calculate and downsample luminace, where source is encoded temperature
/**
output: luminance in all channels
*/
half4 PSPostProcessDownSampleThermalLuminance(VSPP4T_OUTPUT input) : COLOR
{
  // Get encoded temperature and scale it to <0,1> interval
  half x1 = dot(tex2D(sampler0, input.TexCoord0).rgb, 1) * 1.0f / 3.0f;
  half x2 = dot(tex2D(sampler1, input.TexCoord1).rgb, 1) * 1.0f / 3.0f;
  half x3 = dot(tex2D(sampler2, input.TexCoord2).rgb, 1) * 1.0f / 3.0f;
  half x4 = dot(tex2D(sampler3, input.TexCoord3).rgb, 1) * 1.0f / 3.0f;

  // Calculate the average temperature and return it
  half luminance = (x1 + x2 + x3 + x4) * 1.0f/4.0f;
  luminance = min(luminance,MAX_HALF_4);
  return half4(luminance.xxx,1);
}

/// luminance downsampling shader
/**
input: average in all components (only r really used)
output: average in all components
*/

half4 PSPostProcessDownSampleAvgLuminance(VSPP4T_OUTPUT input) : COLOR
{
  half4 x1 = tex2D(sampler0, input.TexCoord0);
  half4 x2 = tex2D(sampler1, input.TexCoord1);
  half4 x3 = tex2D(sampler2, input.TexCoord2);
  half4 x4 = tex2D(sampler3, input.TexCoord3);
  half luminance = (x1.r+x2.r+x3.r+x4.r)*0.25;
  return half4(luminance.xxx,1);
  // debugging - preserve colors from original image
  //return half4((x1.rgb+x2.rgb+x3.rgb+x4.rgb)*0.25,luminance);
}

/// luminance gathering shader
/**
Downsample for average, min and max luminance in r,g,b
Assume input has the same structure (avg,min,max) in r,g,b

Point sampling should be used here - min/max cannot be interpolated
*/

half4 PSPostProcessDownSampleMaxAvgMinLuminance(VSPP4T_OUTPUT input) : COLOR
{
  half3 x1 = tex2D(sampler0, input.TexCoord0);
  half3 x2 = tex2D(sampler1, input.TexCoord1);
  half3 x3 = tex2D(sampler2, input.TexCoord2);
  half3 x4 = tex2D(sampler3, input.TexCoord3);
  return half4(
    max(max(x1.r,x2.r),max(x3.r,x4.r)),
    (x1.g+x2.g+x3.g+x4.g)*0.25,
    min(min(x1.b,x2.b),min(x3.b,x4.b)),
    1
  );
}

float AssumedLuminance(float3 maxAvgMin)
{
  // use mostly maximum to avoid overexposure
  // let average value have some influence as well
  // desiredLuminance tells us where we want the max. luminace to be
  // average luminace should probably be something like half of this
  // The result must not be 0, because it will be used as a divisor
  // - that's why epsilon is introduced
  float maxFactor = 0.9;
  float epsilon = 0.0001f;
  return max(maxAvgMin.r*maxFactor+maxAvgMin.g*2*(1-maxFactor), epsilon);
}

/*
  To transform from last frame space to this frame space we need:
    relValueLastFrame*eyeAccomOld/eyeAccom

/// time used to time-dependend eye adaptation
*/
float4 AssumedLuminancePars1: register(c8);
float4 AssumedLuminancePars2: register(c9);

/// calculate "assumed" luminance value based on avg/min/max
/**
  constant: deltaT - frameTime
  constant: eyeAccomOld/eyeAccom - convert from last frame to this frame

  sampler0: 1x1 texture - current avg/min/max in this frame space
  sampler1: 1x1 texture - aperture used for the last frame in last frame space
  
  To get absolute value we need to use relValue*eyeAccom
  
  This shader works in full precision.
*/

float4 PSPostProcessAssumedLuminance(VSPP4T_OUTPUT input) : COLOR
{
  float eyeAccomLastToThis = AssumedLuminancePars1[0];
  //float deltaT = AssumedLuminancePars1[1];
  float desiredLuminance = AssumedLuminancePars1[2];
  
  float eyeAccomMin = AssumedLuminancePars2[0];
  float eyeAccomMax = AssumedLuminancePars2[1];
  float minRatio = AssumedLuminancePars2[2];
  float maxRatio = AssumedLuminancePars2[3];
  
  float3 maxAvgMin = tex2D(sampler0, input.TexCoord0);
  
  // check for any possible overflow
  float maxAperture = 1e3;
  // when any overflow is detected, truncate
  if (!(maxAvgMin.r<maxAperture)) maxAvgMin.r = maxAperture;
  if (!(maxAvgMin.g<maxAperture)) maxAvgMin.g = maxAperture;
  if (!(maxAvgMin.b<maxAperture)) maxAvgMin.b = maxAperture;
  
  float assumedLuminance = AssumedLuminance(maxAvgMin);
  
  // sample previous aperture texture
  float newEyeAccom = desiredLuminance/assumedLuminance;
  if (eyeAccomLastToThis>0)
  {
    // last aperture valid - update it
    // caculate desired apreture (in current render target space)
    // this is desired aperture = desired luminance / measure luminance
    // precision is critical here - partial precision not enough

    float lastFrameEyeAccom = DecodeApertureChange(tex2D(sampler1, float2(0,0)).x)*eyeAccomLastToThis;
    float ratio = newEyeAccom/lastFrameEyeAccom;

    float logRatio = abs(log2(ratio));
    // for small ratios make the change very slow
    if (logRatio<1)
    {
      logRatio = lerp(0,logRatio,logRatio);
    }
    float maxChange = exp2(logRatio);
    ratio = max(-maxChange,min(ratio,maxChange));
    // perform change based on deltaT and lastFrameAperture here
    ratio = max(minRatio,min(ratio,maxRatio));

    newEyeAccom = lastFrameEyeAccom*ratio;
  }
  // saturate into a valid range
  newEyeAccom = max(eyeAccomMin,min(newEyeAccom,eyeAccomMax));

  // if there is any NAN or infinity, make sure we truncate it to prevent spreading into following frames

  return float4(EncodeApertureChange(newEyeAccom).xxx,1);
}

/// Simplified version of PSPostProcessAssumedLuminance
/**
if there is no float render target support,
we cannot calculate change on the GPU
*/
half4 PSPostProcessAssumedLuminanceDirect(VSPP4T_OUTPUT input) : COLOR
{
  half desiredLuminance = AssumedLuminancePars1[2];
  
  half eyeAccomMin = AssumedLuminancePars2[0];
  half eyeAccomMax = AssumedLuminancePars2[1];
  
  half3 maxAvgMin = tex2D(sampler0, input.TexCoord0);
  
  half assumedLuminance = AssumedLuminance(maxAvgMin);
  
  // sample previous aperture texture
  half newEyeAccom = desiredLuminance/assumedLuminance;
  // saturate into a valid range
  newEyeAccom = max(eyeAccomMin,min(newEyeAccom,eyeAccomMax));
  return half4(EncodeApertureChange(newEyeAccom).xxx,1);
}

/// apply night blue shift + black and white vision
half3 NightBlueShift(half3 c, half3 maxAvgMin)
{
  half4 intensity;
  intensity.a = dot(c, rgbEyeCoef.rgb);
  intensity.rgb = c;
  
  half4 factor = saturate(intensity*nightControl.x + nightControl.y);
  // players (and Holywood) like the night blue - let them have it (to certain extent)
  half3 color = half3(intensity.a*0.95, intensity.a*0.95, intensity.a*1.05)*(1-factor.a) + c*factor.rgb;
  return ToneMapping(color,intensity.a,maxAvgMin);
}

/**
textures:
  sampler 0 - source image
  sampler 1 - glow image
  sampler 2 - aperture modificator (1x1)
  sampler 3 - 1x1 texture - current avg/min/max in this frame space
*/

half4 PSPostProcessGlow(VSPP4T_OUTPUT input) : COLOR
{
  half4 cOriginal = tex2D(sampler0, input.TexCoord0);
  half4 cGlow = tex2D(sampler1, input.TexCoord1);
  
  float aperture = ApertureControl(tex2D(sampler2,float2(0,0)).x);
  
  // read min/max/avg, transform it into the new color space
  half3 maxAvgMin = tex2D(sampler3,float2(0,0))*aperture;


  //half3 colorWGlow = max(cGlow.rgb, cOriginal.rgb);
  half3 colorWGlow = cGlow.rgb+ cOriginal.rgb*saturate(1-cGlow.rgb);
  //half3 colorWGlow = cGlow.rgb+ cOriginal.rgb*saturate(1-(cGlow.rgb-cOriginal.rgb));
  half3 color = colorWGlow.rgb*aperture;
 
  return half4(ToneMapping(color, dot(color, rgbEyeCoef.rgb),maxAvgMin),cOriginal.a);
}

half4 PSPostProcessGlowNight(VSPP4T_OUTPUT input) : COLOR
{
  half4 cOriginal = tex2D(sampler0, input.TexCoord0);
  half4 cGlow = tex2D(sampler1, input.TexCoord1);
  float aperture = ApertureControl(tex2D(sampler2,float2(0,0)).x);
  half3 maxAvgMin = tex2D(sampler3,float2(0,0))*aperture;
  return half4(NightBlueShift(max(cOriginal,cGlow)*aperture,maxAvgMin),cOriginal.a);
}

/// B&W, transformed to a single color

half4 PSPostProcessNVG(VSPP4T_OUTPUT input) : COLOR
{
  half4 cOriginal = tex2D(sampler0, input.TexCoord0);
  half4 cGlow = tex2D(sampler1, input.TexCoord1);

  float aperture = ApertureControl(tex2D(sampler2,float2(0,0)).x);
  half3 afterAperture = max(cOriginal.rgb,cGlow.rgb)*aperture;
  half3 maxAvgMin = tex2D(sampler3,float2(0,0))*aperture;

  half intensity = dot(afterAperture, rgbEyeCoef.rgb);
  
  // simulate different response in different RGB components
  //half3 color = half3(intensity*0.3, intensity*1, intensity*0.1); // yellowish
  half3 color = half3(intensity*0.15, intensity*1, intensity*0.2); // cyanish

  // used by _VBS3_TI
  //half3 color = cOriginal.rgb;

  return half4(color,cOriginal.a);
}


////////////////////////////////////////////////////////////////////////////////////////////////////

// generic vertex shader output structure
struct VSPP_OUT
{
  float4 pos : POSITION;
  float4 tc0 : TEXCOORD0;
  float4 tc1 : TEXCOORD1;
  float4 tc2 : TEXCOORD2;
  float4 tc3 : TEXCOORD3;
  float4 tc4 : TEXCOORD4;
  float4 tc5 : TEXCOORD5;
  float4 tc6 : TEXCOORD6;
};

// vertex shader data for RotBlur PP effect
float4 mA0 : register(c0);
float4 mA1 : register(c1);
float4 mA2 : register(c2);
float4 mB0 : register(c3);
float4 mB1 : register(c4);
float4 mB2 : register(c5);
// projection data for aspect correction
float4 projConsts : register(c6);

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
float4x4 GetRMatrix( float t )
{
  float4 Speeds = float4( 1.55, 0, 1.55, 1 );
   float4x4 mA = (float4x4)0;
   float4x4 mB = (float4x4)0;
   mA[0].x = 1; mA[1].y = 1; mA[2].z = 1; mA[3].w = 1;
   mB[0].x = 1; mB[1].y = 1; mB[2].z = 1; mB[3].w = 1;

   float angleA = 0.0;
   float angleB = 0.0;
   angleA = Speeds.x * t;
   angleB = Speeds.z * cos( t * Speeds.y );
   angleB = 1.57;
   
   float sinA = sin( angleA );
   float cosA = cos( angleA );
   float sinB = sin( angleB );
   float cosB = cos( angleB );
   
   mA[0].x = cosA;
   mA[2].z = cosA;
   mA[0].z = sinA;
   mA[2].x = -sinA;
   
   mB[1].y = cosB;
   mB[2].z = cosB;
   mB[1].z = sinB;
   mB[2].y = -sinB;
   
   mA = mul( mA, mB );
   return mA;
}
*/
/*
//////////////////////////////////////////////////////////////////////////////////////////
float4x4 MirrorMat( float4x4 m )
{
   float4x4 n;
   n[0][0] = m[0][0]; n[0][1] = m[1][0]; n[0][2] = m[2][0]; n[0][3] = m[3][0];
   n[1][0] = m[0][1]; n[1][1] = m[1][1]; n[1][2] = m[2][1]; n[1][3] = m[3][1];
   n[2][0] = m[0][2]; n[2][1] = m[1][2]; n[2][2] = m[2][2]; n[2][3] = m[3][2];
   n[3][0] = m[0][3]; n[3][1] = m[1][3]; n[3][2] = m[2][3]; n[3][3] = m[3][3];
   return n;
}
*/
float3x3 MirrorMat3( float3x3 m )
{
   float3x3 n;
   n[0][0] = m[0][0]; n[0][1] = m[1][0]; n[0][2] = m[2][0];
   n[1][0] = m[0][1]; n[1][1] = m[1][1]; n[1][2] = m[2][1];
   n[2][0] = m[0][2]; n[2][1] = m[1][2]; n[2][2] = m[2][2];
   return n;
}

/*
//////////////////////////////////////////////////////////////////////////////////////////
float3x3 GetRotMatrix( float x, float y, float z )
{
   float3x3 mA = (float3x3)0;
   float3x3 mB = (float3x3)0;
   mA[0].x = 1; mA[1].y = 1; mA[2].z = 1; //mA[3].w = 1;
   mB[0].x = 1; mB[1].y = 1; mB[2].z = 1; //mB[3].w = 1;

   float angleA = -y;
   float angleB = x;
   
   float sinA = sin( angleA );
   float cosA = cos( angleA );
   float sinB = sin( angleB );
   float cosB = cos( angleB );
   
   mA[0].x = cosA;
   mA[2].z = cosA;
   mA[0].z = sinA;
   mA[2].x = -sinA;
   
   mB[1].y = cosB;
   mB[2].z = cosB;
   mB[1].z = sinB;
   mB[2].y = -sinB;
   
   mA = mul( mB, mA );
   return mA;
}*/


//==================================================================================================
// POST PROCESS RotBlur
//==================================================================================================
VSPP_OUT VsPpRotBlur(
  in float3 inPos : POSITION,
  in float2 inTc0 : TEXCOORD
)
{
	VSPP_OUT o = (VSPP_OUT)0;
  half3 pos = inPos.xyz;
  half depthThreshold = mA0.w;
  half blurPower = mA1.w;
 
  // ------------------------------------------------------------------------------
  // description of modification can be found in file postProccess.cpp at line 3844
  // ------------------------------------------------------------------------------
  
  float3x3 AinvB;
  AinvB[0] = mA0.xyz;
  AinvB[1] = mA1.xyz;
  AinvB[2] = mA2.xyz;

  pos.xy *= projConsts.xy;  
  
  float3 dir = pos - mul(pos, AinvB);
    
  dir.y *= projConsts.z; // aspect correction
    
  o.pos.xyzw = float4( inPos.xyzz );
  o.tc0.xy = inTc0.xy;
  o.tc0.z = depthThreshold;
  o.tc1.xy = half2( dir.x, -dir.y ) * blurPower;
  
  //o.tc1.zw = half2( mA0.w, mA1.w );
  
  return o;
}

//==================================================================================================
float4 PsPpRotBlur( VSPP_OUT i ) : COLOR
{
  const int STEPS = 7;
 
  half depthTreshold = i.tc0.z; // depthThreshold
  half2 dir0 = i.tc1.xy;
	
  half2 tcP = i.tc0.xy;
  half2 tcN = i.tc0.xy;
  
  half d = 1;
	
	half3 accu = tex2D( sampler0, i.tc0.xy ).rgb;

	half zDiff = tex2D( sampler1, i.tc0.xy ).r - depthTreshold;
	if (zDiff > 0)
	{
		half depthMul = LinearFunc(zDiff, 2/250.0, 0);
		for( int i = 0; i < STEPS; i++ )
		{
			tcP += dir0*depthMul;
			if( tex2D( sampler1, tcP ).r > depthTreshold )
			{
				accu += tex2D( sampler0, tcP ).rgb;
				d += 1;
			}
			
			tcN -= dir0*depthMul;
			if( tex2D( sampler1, tcN ).r > depthTreshold )
			{
				accu += tex2D( sampler0, tcN ).rgb;
				d += 1;
			}
		}
	}

  return half4(accu.rgb / d, 1);
}

float4 radialBlurPars : register(c0); // xy... blur scale, zw... blur offset

//==================================================================================================
float4 PsPpRadialBlur( VSPP_OUTPUT i ) : COLOR
{
  const int STEPS = 7;
  half4 final;
  half2 tc = i.TexCoord.xy;
  half2 dir = tc - 0.5;
  dir.xy = -radialBlurPars.xy * saturate( abs(dir.xy) - radialBlurPars.zw ) * sign( dir.xy );
  dir.y *= projConsts.z; // aspect correction
  
  half3 accum = 0;
  tc.xy = (3 * dir.xy) + tc.xy;   // symmetrical sampling move +3 samples above, then move to -3 samples
  for (int i = 0; i < STEPS; i++)
  {
	accum += tex2D(sampler0, tc.xy).rgb;
	tc.xy -= dir.xy;
  }
  accum /= STEPS;  
  return half4(accum.rgb, 1);
}

//==================================================================================================
VSPP_OUT VsPpColors(
  in float3 inPos : POSITION,
  in float2 inTc0 : TEXCOORD
)
{
  VSPP_OUT o = (VSPP_OUT)0;
  half4 pos = half4( inPos.xy, 1.0, 1.0 );
  o.pos.xyzw = pos;
  o.tc0.xy = inTc0;  
  return o;
}

float4 ppColorsPars : register(c0); // x ..brightness, y ...contrast, z ...offset ( default: 1, 1, 0 )
float4 lerpColor : register(c2);
float4 colorizeColor : register(c3); // a ...saturation
float4 rgbWeights : register(c4); // 0.299, 0.587, 0.114

//==================================================================================================
float4 PsPpColors( VSPP_OUT i ) : COLOR
{
  half4 final;
  half3 src = tex2D( sampler0, i.tc0.xy ).rgb;
// CONTRAST + OFFSET
  src.rgb = saturate( src.rgb * ppColorsPars.y + ppColorsPars.z );
  // 
  half mono = src.r * rgbWeights.r + src.g * rgbWeights.g + src.b * rgbWeights.b;
  half3 mono3 = mono * colorizeColor.rgb;
// SATURATION / COLORIZATION
  final.rgb = lerp( mono3.rgb, src.rgb, colorizeColor.a );
// BRIGHTNESS
  half brightness = ppColorsPars.x;
  final.rgb *= saturate( brightness );
  final.rgb = lerp( final.rgb, half3( 1.0, 1.0, 1.0 ), saturate( brightness - 1.0 ) );
// COLOR BLEND
  final.rgb = lerp( final.rgb, lerpColor.rgb, lerpColor.a );
  
  final.a = 1;
  return final;
}


float4 chromAbberPars : register(c0); // xy... abberation power

//==================================================================================================
float4 PsPpChromAber( VSPP_OUTPUT i ) : COLOR
{
  // color masks for samples
  const half3 mask1 = half3( 0.8, 0.2, 0.0 );
  const half3 mask2 = half3( 0.2, 0.6, 0.2 );
  const half3 mask3 = half3( 0.0, 0.2, 0.8 );
  //
  half4 final;
  half2 tc = i.TexCoord.xy;
  half2 dir = tc - 0.5;
  dir.xy *= chromAbberPars.xy;
  //dir.y *= projConsts.z; // aspect correction
  //dir.x *= projConsts.x; // aspect correction
  
  final.rgb = tex2D( sampler0, tc - dir ).rgb * mask1.rgb;
  final.rgb += tex2D( sampler0, tc ).rgb * mask2.rgb;
  final.rgb += tex2D( sampler0, tc + dir ).rgb * mask3.rgb;
  final.a = 1;
  return final;
}

//==================================================================================================
float4 clrInversPars: register(c0); // r,g,b ... color inversion value

float4 PsPpClrInversion(VSPP_OUTPUT i): COLOR0
{
	float3 clr = abs(clrInversPars.xyz - tex2D(sampler0, i.TexCoord.xy).xyz);
	return float4(saturate(clr.xyz), 1);
}

//==================================================================================================

float4 ppWetDistortPars : register(c0); // time, blur, powerTop, powerBottom
float4 ppWDSpeeds : register(c1); // speeds
float4 ppWDAmps : register(c2); // amplitudes
float4 ppWDPhases : register(c3); // randX, randY, posX, posY
//
float4 ppWDAberration : register(c4);

//==================================================================================================
VSPP_OUT VsPpWetDistort(
  in float3 inPos : POSITION,
  in float2 inTc0 : TEXCOORD
)
{
	VSPP_OUT o = (VSPP_OUT)0;
  half4 pos = half4( inPos.xy, 1.0, 1.0 );
  half2 tc;
  tc.x = 0.5 * inPos.x + 0.5;
  tc.y = -0.5 * inPos.y + 0.5;
  
  o.pos.xyzw = pos;
	o.tc0.xy = tc;
  
  float t = ppWetDistortPars.x;
  float a0 = sin( t * ppWDSpeeds.x + inTc0.x * ppWDPhases.x + inPos.x * ppWDPhases.z ) * ppWDAmps.x;
  float a1 = sin( t * ppWDSpeeds.y + inTc0.y * ppWDPhases.x + inPos.x * ppWDPhases.z ) * ppWDAmps.y;
  float a2 = sin( t * ppWDSpeeds.z + inPos.z * ppWDPhases.y + inPos.y * ppWDPhases.w ) * ppWDAmps.z;
  float a3 = sin( t * ppWDSpeeds.w + inTc0.x * ppWDPhases.y + inPos.y * ppWDPhases.w ) * ppWDAmps.w;
  half gPhase = lerp( ppWetDistortPars.z, ppWetDistortPars.w, tc.y );
  o.tc0.w = ppWetDistortPars.y * gPhase;
  o.tc1.x = ( a0 + a1 ) * gPhase;
  o.tc1.y = ( a2 + a3 ) * gPhase;
  o.tc1.zw = inPos.xy;
  o.tc2.xyz = half3( inTc0.x, inTc0.y, inPos.z );
  o.tc2.w = t;
  
  return o;
}

//==================================================================================================
float4 PsPpWetDistort( VSPP_OUT i ) : COLOR
{
  half4 final;
  half2 tc = i.tc0.xy;
  half2 dtc = i.tc1.xy;
  
  half3 src;
  src.r = tex2D( sampler0, tc + dtc * ppWDAberration.x ).r;
  src.g = tex2D( sampler0, tc + dtc * ppWDAberration.y ).g;
  src.b = tex2D( sampler0, tc + dtc * ppWDAberration.z ).b;
  
  half3 blur;
  blur.r = tex2D( sampler1, tc + dtc * ppWDAberration.x ).r;
  blur.g = tex2D( sampler1, tc + dtc * ppWDAberration.y ).g;
  blur.b = tex2D( sampler1, tc + dtc * ppWDAberration.z ).b;
  
  final.rgb = lerp( src.rgb, blur.rgb, i.tc0.w );
  final.a = 1;
  return final;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// DYNAMIC BLUR
////////////////////////////////////////////////////////////////////////////////////////////////////

float4 ppDynBlurPars0 : register(c0);
float4 ppDynBlurPars1 : register(c1);
float4 ppDynBlurWeights[6] : register(c2);

//==================================================================================================
VSPP_OUT VsPpDynamicBlur(
  in float3 inPos : POSITION,
  in float2 inTc0 : TEXCOORD
)
{
	VSPP_OUT o = (VSPP_OUT)0;
  float4 pos = half4( inPos.xy, 1.0, 1.0 );
  float2 tc = inTc0.xy;
  tc.y *= ppDynBlurPars0.y;
  o.tc0.zw = tc;
  tc += ppDynBlurPars1.zw;
  float2 dir = ppDynBlurPars1.xy;
  
  o.pos.xyzw = pos;
	o.tc0.xy = tc;
  o.tc1.xy = tc + dir;
  o.tc1.zw = tc - dir;
  o.tc2.xy = o.tc1.xy + dir;
  o.tc2.zw = o.tc1.zw - dir;
  o.tc3.xy = o.tc2.xy + dir;
  o.tc3.zw = o.tc2.zw - dir;
  o.tc4.xy = o.tc3.xy + dir;
  o.tc4.zw = o.tc3.zw - dir;
  o.tc5.xy = o.tc4.xy + dir;
  o.tc5.zw = o.tc4.zw - dir;
  o.tc6.xyzw = float4( 1, ppDynBlurPars0.z, 1, ppDynBlurPars0.z );
  return o;
}

//==================================================================================================
float4 PsPpDynamicBlur( VSPP_OUT i ) : COLOR
{
  half4 final;
  float4 lim = i.tc6.xyzw;
  float4 tc0 = min( i.tc0, lim );
  float4 tc1 = min( i.tc1, lim );
  float4 tc2 = min( i.tc2, lim );
  float4 tc3 = min( i.tc3, lim );
  float4 tc4 = min( i.tc4, lim );
  float4 tc5 = min( i.tc5, lim );
  final = tex2D( sampler0, tc0.xy ) * ppDynBlurWeights[0].x;
  final += tex2D( sampler0, tc1.xy ) * ppDynBlurWeights[1].x;
  final += tex2D( sampler0, tc1.zw ) * ppDynBlurWeights[1].x;
  final += tex2D( sampler0, tc2.xy ) * ppDynBlurWeights[2].x;
  final += tex2D( sampler0, tc2.zw ) * ppDynBlurWeights[2].x;
  final += tex2D( sampler0, tc3.xy ) * ppDynBlurWeights[3].x;
  final += tex2D( sampler0, tc3.zw ) * ppDynBlurWeights[3].x;
  final += tex2D( sampler0, tc4.xy ) * ppDynBlurWeights[4].x;
  final += tex2D( sampler0, tc4.zw ) * ppDynBlurWeights[4].x;
  final += tex2D( sampler0, tc5.xy ) * ppDynBlurWeights[5].x;
  final += tex2D( sampler0, tc5.zw ) * ppDynBlurWeights[5].x;
  return final;
}

//==================================================================================================
VSPP_OUT VsPpDynamicBlurFinal(
  in float3 inPos : POSITION,
  in float2 inTc0 : TEXCOORD
)
{
	VSPP_OUT o = (VSPP_OUT)0;
  float4 pos = half4( inPos.xy, 1.0, 1.0 );
  float2 tc = inTc0.xy;
  half weight0 = saturate( ppDynBlurPars0.x );
  float2 aspect = float2( 0, ppDynBlurPars0.y );
  o.pos.xyzw = pos;
	o.tc0.xy = tc + ppDynBlurPars0.zw;
  o.tc1.xy = tc + ppDynBlurPars1.xy * aspect + ppDynBlurPars1.zw;
  o.tc2.xy = tc - ppDynBlurPars1.xy * aspect + ppDynBlurPars1.zw;
  o.tc3.xy = tc + ppDynBlurPars1.yx * aspect + ppDynBlurPars1.zw;
  o.tc4.xy = tc - ppDynBlurPars1.yx * aspect + ppDynBlurPars1.zw;
  o.tc0.w = weight0;
  return o;
}

//==================================================================================================
float4 PsPpDynamicBlurFinal( VSPP_OUT i ) : COLOR
{
  half4 final;
  half weight = i.tc0.w;
  half4 src0 = tex2D( sampler0, i.tc0.xy );
  half4 src1 = tex2D( sampler1, i.tc1.xy );
  src1 += tex2D( sampler1, i.tc2.xy );
  src1 += tex2D( sampler1, i.tc3.xy );
  src1 += tex2D( sampler1, i.tc4.xy );
  src1 *= 0.25;
  final.rgb = lerp( src0.rgb, src1.rgb, saturate( weight ) );
  final.a = 1;
  return final;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FXAA post process
////////////////////////////////////////////////////////////////////////////////////////////////////

#if (FXAA_SHADER_SOURCE == 1)

	// http://developer.nvidia.com/nvidia-graphics-sdk-11-direct3d, Image-Based Anti-Aliasing

	#define FXAA_HLSL_3		1
	#define FXAA_PRESET		5
	
	#define FXAA_DEBUG_PASSTHROUGH	0
	#define FXAA_DEBUG_HORZVERT		0
	#define FXAA_DEBUG_PAIR			0
	#define FXAA_DEBUG_NEGPOS		0
	#define FXAA_DEBUG_OFFSET		0	
	
	#include "FxaaShader.h"
	
#endif

#if (FXAA_SHADER_SOURCE == 2)
	
	// http://timothylottes.blogspot.com/2011/07/fxaa-311-released.html
	
	#define FXAA_PC					1
	#define FXAA_HLSL_3				1
	#define FXAA_GREEN_AS_LUMA		1
	//#define FXAA_QUALITY__PRESET	39

	#include "Fxaa3_11.h"
	
#endif

sampler2D FxaaSampler0: register(s0);

uniform float4 VsFxaa: register(c0);
uniform float4 PsFxaa: register(c1);
uniform float4 PsFxaaQuality: register(c2); // x = fxaaQualitySubpix, y = fxaaQualityEdgeThreshold, z = fxaaQualityEdgeThresholdMin

void VsPpFxaa3_11(in float3 iPos: POSITION0, in float2 itc0: TEXCOORD0, 
	out float4 oPos: POSITION0, out float2 otc0: TEXCOORD0)
{
	oPos = float4(iPos.xy, 1.0, 1.0);
	otc0.xy = itc0.xy + VsFxaa.xy;
}

float4 PsPpFxaa3_11(in float2 pos: TEXCOORD0): COLOR 
{	
	float4 clr = (float4) 0;
	
#if (FXAA_SHADER_SOURCE == 1)
	clr.rgb = FxaaPixelShader(pos.xy, FxaaSampler0, PsFxaa).rgb;
	clr.a = 1;
#endif		
	
#if (FXAA_SHADER_SOURCE == 2)

	clr = FxaaPixelShader(
		pos.xy,						// FxaaFloat2 pos
		0,							// FxaaFloat4 fxaaConsolePosPos
		FxaaSampler0, 
		FxaaSampler0,
		FxaaSampler0,
		PsFxaa.xy,					// FxaaFloat2 fxaaQualityRcpFrame
		0,							// FxaaFloat4 fxaaConsoleRcpFrameOpt
		0,							// FxaaFloat4 fxaaConsoleRcpFrameOpt2
		0,							// FxaaFloat4 fxaaConsole360RcpFrameOpt2
		PsFxaaQuality.x,			// FxaaFloat fxaaQualitySubpix
		PsFxaaQuality.y,			// FxaaFloat fxaaQualityEdgeThreshold
		PsFxaaQuality.z,			// FxaaFloat fxaaQualityEdgeThresholdMin
		0,							// FxaaFloat fxaaConsoleEdgeSharpness
		0,							// FxaaFloat fxaaConsoleEdgeThreshold
		0,							// FxaaFloat fxaaConsoleEdgeThresholdMin
		0							// fxaaConsole360ConstDir
	);
#endif
	
#if (FXAA_SHADER_DEBUG == 1) 
	if (PsFxaaQuality.w > 0.5) 
	{
		clr = clr - tex2D(FxaaSampler0, pos.xy);
	}
#endif
		
	return clr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Modified Sharpen complex v2 from Media Player Classic Home Cinema
////////////////////////////////////////////////////////////////////////////////////////////////////

#define CoefGauss 2
#define CoefClr (1 + CoefGauss)
#define Sharpen_val0 2
#define Sharpen_val1 ((Sharpen_val0 - 1) / 8.0)

sampler2D SharpenSampler0: register(s0);
uniform float4 PsSFilter: register(c1);	// x = 1 / width, y = 1 / height, offset * (1 / width), offset * (1/ hight)
uniform float4 PsSFilter1: register(c2); // SharpenEdge

float4 PsSharpenFilter(in float2 tex: TEXCOORD0): COLOR
{
	float3 clr = tex2D(SharpenSampler0, tex).rgb;

	// calculate image floue (filtre gaussien)
	float3 c1 = tex2D(SharpenSampler0, tex + float2(-PsSFilter.z, -PsSFilter.w)).rgb;
	float3 c2 = tex2D(SharpenSampler0, tex + float2(  		   0, -PsSFilter.w)).rgb;
	float3 c3 = tex2D(SharpenSampler0, tex + float2( PsSFilter.z,-PsSFilter.w)).rgb;
	float3 c4 = tex2D(SharpenSampler0, tex + float2(-PsSFilter.z, 0 )).rgb;
	float3 c5 = tex2D(SharpenSampler0, tex + float2( PsSFilter.z, 0 )).rgb;
	float3 c6 = tex2D(SharpenSampler0, tex + float2(-PsSFilter.z, PsSFilter.w)).rgb;
	float3 c7 = tex2D(SharpenSampler0, tex + float2(  		   0, PsSFilter.w)).rgb;
	float3 c8 = tex2D(SharpenSampler0, tex + float2( PsSFilter.z, PsSFilter.w)).rgb;

	// gaussian filter
	//   [ 1, 2, 1 ]
	//   [ 2, 4, 2 ]
	//   [ 1, 2, 1 ]
	
	// 1 / (1+2+1+2+4+2+1+2+1) = 1 / 16 = .0625
	float3 clrGauss = (c1+c3+c6+c8+ 2*(c2+c4+c5+c7) + 4*clr) * 0.0625;

	// substract gauss - image
	float3 cori = CoefClr*clr - CoefGauss*clrGauss;

	// edge detection
	//   [ c1,  c2, c3 ]
	//   [ c4, clr, c5 ]
	//   [ c6,  c7, c8 ]
	c1 = tex2D(SharpenSampler0, tex + float2(-PsSFilter.x, -PsSFilter.y)).rgb;
	c2 = tex2D(SharpenSampler0, tex + float2(			0, -PsSFilter.y)).rgb;
	c3 = tex2D(SharpenSampler0, tex + float2( PsSFilter.x, -PsSFilter.y)).rgb;
	c4 = tex2D(SharpenSampler0, tex + float2(-PsSFilter.x, 0)).rgb;
	c5 = tex2D(SharpenSampler0, tex + float2( PsSFilter.x, 0)).rgb;
	c6 = tex2D(SharpenSampler0, tex + float2(-PsSFilter.x, PsSFilter.y)).rgb;
	c7 = tex2D(SharpenSampler0, tex + float2(			0, PsSFilter.y)).rgb;
	c8 = tex2D(SharpenSampler0, tex + float2( PsSFilter.x, PsSFilter.y)).rgb;

	// gradient horizontal
	// 		sobel,		  robinson, 	  kirsch
	//   [ -1, 0 ,1 ]	[ -1, 1 ,1 ]	[ -5, 3 ,3 ]
	//   [ -2, 0, 2 ]	[ -1,-2 ,1 ]	[ -5, 0 ,3 ]
	//   [ -1, 0 ,1 ]	[ -1, 1 ,1 ]	[ -5, 3 ,3 ]
	
	float3 delta1 = (c3 + 2*c5 + c8)-(c1 + 2*c4 + c6);	
	//float3 delta1 = (c3 + c5 + c8 + c2 + c7) - (c1 + c4 + c6 + 2*clr);

	// gradient vertical
	//   [ -1,-2,-1 ] [-1, -1, -1]
	//   [  0, 0, 0 ] [ 1, -2,  1]
	//   [  1, 2, 1 ] [ 1,  1,  1]
	float3 delta2 = (c6 + 2*c7 + c8)-(c1 + 2*c2 + c3);
	//float3 delta2 = (c4 + c5 + c6 + c7 + c8) - (c1 + c2 + c3 + 2*clr);
	
	if (sqrt(dot(delta1, delta1) + dot(delta2, delta2)) > PsSFilter1.x) {	
		return float4(clr * Sharpen_val0 - (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8) * Sharpen_val1, 1);
	} else {
		return float4(cori.rgb, 1.0);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  SMAA
////////////////////////////////////////////////////////////////////////////////////////////////////

uniform float2 SMAA_PIXEL_SIZE : register(c1);
#define SMAA_HLSL_3 1
#include "PPAA\SMAA.h"

uniform sampler2D ColorGammaTex: register(s0);
uniform sampler2D DepthTex: register(s0);

uniform sampler2D EdgesTex: register(s0);
uniform sampler2D AreaTex: register(s1);
uniform sampler2D SearchTex: register(s2);

uniform sampler2D ColorTex: register(s0);
uniform sampler2D BlendTex: register(s1);

/**
 * Vertex shader function wrappers
 */
void DX9_SMAAEdgeDetectionVS(inout float4 position : POSITION, inout float2 texcoord : TEXCOORD0, out float4 offset[3] : TEXCOORD1) 
{
	SMAAEdgeDetectionVS(position, position, texcoord, offset);
}

void DX9_SMAABlendingWeightCalculationVS(inout float4 position : POSITION, inout float2 texcoord : TEXCOORD0, out float2 pixcoord : TEXCOORD1, out float4 offset[3] : TEXCOORD2) 
{
	SMAABlendingWeightCalculationVS(position, position, texcoord, pixcoord, offset);
}

void DX9_SMAANeighborhoodBlendingVS(inout float4 position : POSITION, inout float2 texcoord : TEXCOORD0, out float4 offset[2] : TEXCOORD1) 
{
	SMAANeighborhoodBlendingVS(position, position, texcoord, offset);
}

/**
 * Pixel shader function wrappers
 */	
float4 DX9_SMAALumaEdgeDetectionPS(float4 position : SV_POSITION, float2 texcoord : TEXCOORD0, float4 offset[3] : TEXCOORD1) : COLOR 
{
	return SMAALumaEdgeDetectionPS(texcoord, offset, ColorGammaTex);
}

float4 DX9_SMAAColorEdgeDetectionPS(float4 position : SV_POSITION, float2 texcoord : TEXCOORD0, float4 offset[3] : TEXCOORD1) : COLOR 
{
	return SMAAColorEdgeDetectionPS(texcoord, offset, ColorGammaTex);
}

float4 DX9_SMAADepthEdgeDetectionPS(float4 position : SV_POSITION, float2 texcoord : TEXCOORD0, float4 offset[3] : TEXCOORD1) : COLOR 
{
	return SMAADepthEdgeDetectionPS(texcoord, offset, DepthTex);
}

float4 DX9_SMAABlendingWeightCalculationPS(float4 position : SV_POSITION, float2 texcoord : TEXCOORD0, float2 pixcoord : TEXCOORD1, float4 offset[3] : TEXCOORD2) : COLOR 
{
	return SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset, EdgesTex, AreaTex, SearchTex, 0);
}

float4 DX9_SMAANeighborhoodBlendingPS(float4 position : SV_POSITION, float2 texcoord : TEXCOORD0, float4 offset[2] : TEXCOORD1) : COLOR 
{
	//return abs(tex2D(ColorTex, texcoord) - SMAANeighborhoodBlendingPS(texcoord, offset, ColorTex, BlendTex));
	return SMAANeighborhoodBlendingPS(texcoord, offset, ColorTex, BlendTex);
}