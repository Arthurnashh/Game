#line 2 "W:\c\Poseidon\lib\d3dT\ShaderSources\PS.hlsl"

#include "psvs.h"
#include "common.h"

//!{ Samplers used by all the PS's
sampler2D samplers[16]          : register(s0);

#define sampler0                samplers[0]
#define sampler1                samplers[1]
#define sampler2                samplers[2]
#define sampler3                samplers[3]
#define sampler4                samplers[4]
#define sampler5                samplers[5]
#define sampler6                samplers[6]
#define sampler7                samplers[7]
#define sampler8                samplers[8]
#define sampler9                samplers[9]
#define sampler10               samplers[10]
#define sampler11               samplers[11]
#define sampler12               samplers[12]
#define sampler13               samplers[13]
#define sampler14               samplers[14]
#define samplerShadowMap        samplers[15]
#define samplerLightIntensity   samplers[15]
//!}

//! Constant definition
#define PSC_HLSLDEFS(type,name,regnum) const type name : register(c##regnum);
#define PSC_HLSLDEFA(type,name,regnum,dim) const type name[dim] : register(c##regnum);
PSC_LIST(PSC_HLSLDEFS,PSC_HLSLDEFA)

//!{ Shadow intensity retrieving
/*!
  (note there's a purpose in using X and W swizzles on shadowReduction parameter in this order -
  their default values 0 and 1 will perform no shadow reduction in situations the VS don't set the
  COLOR1) 
*/
#define SHCOLORCOEF 8
half GetShadowIntensity(half diffuse, half4 shadowReduction)
{
  return diffuse * max(1 - shadowReduction.x * SHCOLORCOEF, 0);
}

half GetShadowIntensityFast(half diffuse, half4 shadowReduction)
{
  return diffuse * shadowReduction.w;
}
//!}

//! Structure to hold separated components of light - GetLightX functions returns this
struct Light
{
  //! Light from the enviroment - roughly Ambient, DForced and Emmisive
  half3 indirect;
  //! Light from the main light source (light that disappears in a shadow) - roughly the diffuse light
  half3 direct;
  //! Specular light
  half3 specular;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Functions to calculate the light components

Light GetLight(half3 ambient, half3 diffuse, half3 specular)
{
  Light light;
  light.indirect = ambient;
  light.direct = diffuse;
  light.specular = specular;
  return light;
}

Light GetLightAS(half3 ambient, half3 diffuse, half3 specular, half4 ambientShadow)
{
  Light light;
  light.indirect = ambient + PSC_ADForcedE.rgb * ambientShadow.g;
  light.direct = diffuse;
  light.specular = specular;
  return light;
}

half4 GetIrradianceData(half3 lightLocal, half3 halfway,
  half4 normal4, sampler2D samplerIrradiance)
{
  // Get the values from the irradiance map
  half3 normal = normal4.rgb;
  normal = normal * 2 - 1;
  half u = dot(normal, lightLocal);
  half v = dot(normal, halfway);
  return tex2D(samplerIrradiance, half2(u, v));
}

Light GetLightIrradiance(half3 ambient, half4 irradiance, half specularCoef)
{
  // Fill out the light structure
  Light light;
  light.indirect = ambient;
  light.direct = PSC_Diffuse.rgb * irradiance.rgb;
  light.specular = PSC_Specular.rgb * irradiance.a * specularCoef;
  return light;
}

Light GetLightNormal(half3 ambient, half3 specular,
  half4 normal4, half3 lightLocal, out half coefDiffuse)
{
  // Get the normal
  half3 dxt5FixedNormal = half3(1 - normal4.a, 0, 0) + normal4.rgb;
  half3 normal = normalize(dxt5FixedNormal * 2 - 1);

  // Get the diffuse coefficient  
  half3 lightDir = lightLocal;
  coefDiffuse = saturate(dot(normal,lightDir));

  // Fill out the light structure
  Light light;
  light.indirect = ambient;
  light.direct = PSC_Diffuse * coefDiffuse;
  light.specular = specular;
  return light;
}

Light GetLightNormalAS(half3 ambient, half3 specular,
  half4 normal4, half3 lightLocal, half4 ambientShadow, out half coefDiffuse)
{
  // Get the normal
  half3 dxt5FixedNormal = half3(1 - normal4.a, 0, 0) + normal4.rgb;
  half3 normal = normalize(dxt5FixedNormal * 2 - 1);

  // Get the diffuse coefficient  
  half3 lightDir = lightLocal;
  coefDiffuse = saturate(dot(normal,lightDir));

  // Fill out the light structure
  Light light;
  light.indirect = ambient + PSC_ADForcedE.rgb * ambientShadow.g;
  light.direct = PSC_Diffuse * coefDiffuse;
  light.specular = specular;
  return light;
}

Light GetLightNormalSpecular(half3 ambient,
  half4 normal4, half3 lightLocal, half3 halfway,
  half4 specular, out half coefDiffuse)
{
  // Get the normal
  half3 dxt5FixedNormal = half3(1 - normal4.a, 0, 0) + normal4.rgb;
  half3 normal = normalize(dxt5FixedNormal * 2 - 1);

  // Get the diffuse and specular coefficients
  half4 litVector = lit(dot(normal, lightLocal), dot(normal, halfway), PSC_Specular.a * specular.b);
  coefDiffuse = litVector.y;
  half coefSpecular = litVector.z;
  coefSpecular = saturate(coefSpecular); // Specular power can be very high and this can probably lead to infinity

  // Fill out the light structure
  Light light;
  light.indirect = ambient;
  light.direct = PSC_Diffuse * coefDiffuse * specular.r;
  light.specular = PSC_Specular * coefSpecular * specular.g;
  return light;
}

Light GetLightNormalSpecularDI(half3 ambient,
  half4 normal4, half3 lightLocal, half3 halfway,
  half4 specular, out half coefDiffuse)
{
  // Get the normal
  half3 dxt5FixedNormal = half3(1 - normal4.a, 0, 0) + normal4.rgb;
  half3 normal = normalize(dxt5FixedNormal * 2 - 1);

  // Get the diffuse and specular coefficients
  half4 litVector = lit(dot(normal, lightLocal), dot(normal, halfway), PSC_Specular.a * specular.b);
  coefDiffuse = litVector.y;
  half coefSpecular = litVector.z;
  coefSpecular = saturate(coefSpecular); // Specular power can be very high and this can probably lead to infinity

  // Fill out the light structure
  Light light;
  light.indirect = ambient;
  light.direct = PSC_Diffuse * coefDiffuse * (1 - specular.g);
  light.specular = PSC_Specular * coefSpecular * specular.g;
  return light;
}

Light GetLightNormalSpecularAS(half3 ambient,
  half4 normal4, half3 lightLocal, half3 halfway,
  half4 specular, half4 ambientShadow, out half coefDiffuse)
{
  // Get the normal
  half3 dxt5FixedNormal = half3(1 - normal4.a, 0, 0) + normal4.rgb;
  half3 normal = normalize(dxt5FixedNormal * 2 - 1);

  // Get the diffuse and specular coefficients
  half4 litVector = lit(dot(normal, lightLocal), dot(normal, halfway), PSC_Specular.a * specular.b);
  coefDiffuse = litVector.y;
  half coefSpecular = litVector.z;
  coefSpecular = saturate(coefSpecular); // Specular power can be very high and this can probably lead to infinity

  // Fill out the light structure
  Light light;
  light.indirect = ambient + PSC_ADForcedE.rgb * ambientShadow.g;
  light.direct = PSC_Diffuse * coefDiffuse * specular.r;
  light.specular = PSC_Specular * coefSpecular * specular.g;
  return light;
}

Light GetLightNormalSpecularDIAS(half3 ambient,
  half4 normal4, half3 lightLocal, half3 halfway,
  half4 specular, half4 ambientShadow, out half coefDiffuse)
{
  // Get the normal
  half3 dxt5FixedNormal = half3(1 - normal4.a, 0, 0) + normal4.rgb;
  half3 normal = normalize(dxt5FixedNormal * 2 - 1);

  // Get the diffuse and specular coefficients
  half4 litVector = lit(dot(normal, lightLocal), dot(normal, halfway), PSC_Specular.a * specular.b);
  coefDiffuse = litVector.y;
  half coefSpecular = litVector.z;
  coefSpecular = saturate(coefSpecular); // Specular power can be very high and this can probably lead to infinity

  // Fill out the light structure
  Light light;
  light.indirect = ambient + PSC_ADForcedE.rgb * ambientShadow.g;
  light.direct = PSC_Diffuse * coefDiffuse * (1 - specular.g);
  light.specular = PSC_Specular * coefSpecular * specular.g;
  return light;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Functions to retrieve the color and alpha from textures

half4 GetColorDiffuse(half4 diffuse, half alphaMul, half alphaAdd)
{
  return half4(diffuse.rgb, diffuse.a * alphaMul + alphaAdd);
}

half4 GetColorDiffuseDXTADiscrete(half4 diffuse, half alphaMul)
{
  half dalpha = (diffuse.a * alphaMul) > 0.5;
  return half4((1 - diffuse.a) * PSC_T0AvgColor.rgb + diffuse.rgb, dalpha);
}

half4 GetColorDiffuseDiscreteNoise(half4 diffuse, half mul, half add)
{
  half dalpha = (diffuse.a*mul+add) > 1;
  clip(dalpha - 0.5);
  return half4(diffuse.rgb, dalpha);
}

half4 GetColorDiffuseDetail(half4 diffuse, half3 detail, half alphaMul, half alphaAdd)
{
  return half4(diffuse.rgb * detail * 2.0f, diffuse.a * alphaMul + alphaAdd);
}

half4 GetColorDiffuseMacroAS(half4 diffuse, half4 macro, half alphaMul, half alphaAdd)
{
  return half4(lerp(diffuse.rgb * PSC_MaxColor, macro.rgb, macro.a), diffuse.a * alphaMul + alphaAdd);
}

half4 GetColorDiffuseDetailMacroAS(half4 diffuse, half3 detail, half4 macro, half alphaMul, half alphaAdd)
{
  return half4(lerp(diffuse.rgb * PSC_MaxColor, macro.rgb, macro.a) * detail.rgb * 2, diffuse.a * alphaMul + alphaAdd);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Function to retrieve the final color of the shader

//! Strucure to hold output of the pixel fragment
struct FOut
{
  Light light;
  half4 color;
  half shadowIntensity;  // Shadow intensity of SV shadows (saturated normal dot light direction)
  half fog;              // Fog parameter
  half landShadow;       // Landscape shadow coefficient
};

half4 ApplyFog(half4 color, half fog)
{
  color.rgb = lerp(PSC_FogColor, color.rgb, fog);
  return color;
}

half4 GetFinalColor(FOut fout)
{
  return ApplyFog(
    half4(
      (fout.light.indirect + fout.light.direct * fout.landShadow) * fout.color.rgb + fout.light.specular * fout.landShadow,
      fout.color.a * (1 - fout.shadowIntensity * fout.landShadow * half(16.0f/255.0f))
    ),
    fout.fog
  );
}

half4 GetFinalColorNA(FOut fout)
{
  return ApplyFog(
    half4(
      (fout.light.indirect + fout.light.direct * fout.landShadow) * fout.color.rgb + fout.light.specular * fout.landShadow,
      fout.color.a
    ),
    fout.fog
  );
}

half4 GetFinalColorSR_Default(FOut fout, float4 tShadowBuffer)
{
  // Precomputed constants related to texture size
  const float texsize = 2048.0f;
  const float invTexsize = 1.0f / texsize;

  // Calculate the fractional part of the texel adressing  
  float2 dxy = frac(tShadowBuffer.xy * texsize);
  
  // Calculate weights for bilinear interpolation
  
  //float4 fxy = half4(dxy, half2(1,1)-dxy);

  float3 pnz = float3(+1,-1,0);
  float4 fxy = dxy.xyxy * pnz.xxyy + pnz.zzxx;

  // float4((1 - dxy.x) * (1 - dxy.y), (1 - dxy.x) * dxy.y, dxy.x * (1 - dxy.y), dxy.x * dxy.y)
  // float4(fxy.z       * fxy.w,       fxy.z       * fxy.y, fxy.x * fxy.w,       fxy.x * fxy.y)
  float4 weights = fxy.zzxx*fxy.wywy;
  
  // Get 4 shadow depths
  float4 shadowDepth = float4(
    tex2D(samplerShadowMap, tShadowBuffer.xy + float2(0,           0)).r,
    tex2D(samplerShadowMap, tShadowBuffer.xy + float2(0,           invTexsize)).r,
    tex2D(samplerShadowMap, tShadowBuffer.xy + float2(invTexsize,  0)).r,
    tex2D(samplerShadowMap, tShadowBuffer.xy + float2(invTexsize,  invTexsize)).r
  );

  // Get 4 shadow/not shadow values
  float4 sc = shadowDepth>=tShadowBuffer.z;
  
  // Use weights to get the shadow coefficient
  half shadowCoef = dot(sc, weights);
  const half shadowsZHalf = PSC_Shadow_Factor_ZHalf.w * 0.5f;
  half shadowDisappear = saturate((tShadowBuffer.w - shadowsZHalf) / shadowsZHalf);
  shadowCoef = shadowDisappear + shadowCoef - shadowCoef * shadowDisappear;

  // Involve land shadow to shadow coefficient
  shadowCoef *= fout.landShadow;

  // Control shadow intensity via PSC_Shadow_Factor_ZHalf.x
  XXX make sure the constant to be set
  half shadowCoefFinal = 1 + PSC_Shadow_Factor_ZHalf.x * (shadowCoef - 1);


//  float shadowDisappear = saturate((tShadowBuffer.w - 50) / 50);
//  float shadowCoef = (tex2D(samplerShadowMap, tShadowBuffer.xy).r < tShadowBuffer.z) ? shadowDisappear : 1;
  return ApplyFog(
    half4(
      (fout.light.indirect + fout.light.direct * shadowCoefFinal) * fout.color.rgb + fout.light.specular * shadowCoefFinal,
      fout.color.a * (1 - fout.shadowIntensity * half(16.0f/255.0f) * shadowCoef)
    ),
    fout.fog
  );
}

half4 GetFinalColorSR_nVidia(FOut fout, float4 tShadowBuffer)
{
  float4 nVidiaSTRQ = float4(tShadowBuffer.xyz, 1);
  const half shadowsZHalf = PSC_Shadow_Factor_ZHalf.w * 0.5f;
  half shadowDisappear = saturate((tShadowBuffer.w - shadowsZHalf) / shadowsZHalf);
  half shadowCoef = 1 - (1 - tex2D(samplerShadowMap, nVidiaSTRQ).r) * (1 - shadowDisappear);

  // Involve land shadow to shadow coefficient
  shadowCoef *= fout.landShadow;

  // Control shadow intensity via PSC_Shadow_Factor_ZHalf.x
  XXX make sure the constant to be set
  half shadowCoefFinal = 1 + PSC_Shadow_Factor_ZHalf.x * (shadowCoef - 1);

  return ApplyFog(
    half4(
      (fout.light.indirect + fout.light.direct * shadowCoefFinal) * fout.color.rgb + fout.light.specular * shadowCoefFinal,
      fout.color.a * (1 - fout.shadowIntensity * half(16.0f/255.0f) * shadowCoef)),
      fout.fog
    );
}

//! Macro to create the specified PS (and the version with shadow)
#define CREATEPSSN(name, structureName) \
half4 PS##name(structureName input) : COLOR \
{ \
  FOut fout = PF##name(input,false); \
  return GetFinalColor(fout); \
} \
half4 PSNA##name(structureName input) : COLOR \
{ \
  FOut fout = PF##name(input,false); \
  return GetFinalColorNA(fout); \
} \
half4 PSSR##name##_Default(structureName input) : COLOR \
{ \
  FOut fout = PF##name(input,true); \
  return GetFinalColorSR_Default(fout, input.tShadowBuffer); \
} \
half4 PSSR##name##_nVidia(structureName input) : COLOR \
{ \
  FOut fout = PF##name(input,true); \
  return GetFinalColorSR_nVidia(fout, input.tShadowBuffer); \
}

//! Macro to create the specified PS - no shadow variants
#define CREATEPSSN_NS(name, structureName) \
half4 PS##name(structureName input) : COLOR \
{ \
  FOut fout = PF##name(input,false); \
  return GetFinalColorNA(fout); \
}

#define CREATEPS(name) CREATEPSSN(name,S##name)

/////////////////////////////////////////////////////////////////////////////////////////////
// Pixel shaders

FOut PFDetail(SDetail input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  fout.light = GetLight(
    input.ambientColor,
    input.diffuseColor,
    half3(0, 0, 0));

  // Calculate the color
  fout.color = GetColorDiffuseDetail(
    tex2D(sampler0, input.tDiffuseMap_DetailMap.xy),
    tex2D(sampler1, input.tDiffuseMap_DetailMap.wzyx),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(input.diffuseColor.a, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(Detail)

//////////////////////////////////////////////

FOut PFDetailSpecularAlpha(SDetailSpecularAlpha input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  fout.light = GetLight(
    input.ambientColor,
    input.diffuseColor,
    input.specularColor_landShadow.rgb * 2.0f);

  // Calculate the color
  half3 s = input.specularColor_landShadow.rgb * 2.0f;
  half specIntensity = dot(s, s) * 2.0f;
  fout.color = GetColorDiffuseDetail(
    tex2D(sampler0, input.tDiffuseMap_DetailMap.xy),
    tex2D(sampler1, input.tDiffuseMap_DetailMap.wzyx),
    input.ambientColor.a, specIntensity);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(input.diffuseColor.a, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.specularColor_landShadow.a;

  return fout;
}

CREATEPS(DetailSpecularAlpha)

//////////////////////////////////////////////

FOut PFNormal(SNormal input, bool sbShadows)
{
  FOut fout;
  
//   fout.light.indirect = 0;
//   fout.light.direct = 1;
//   fout.light.specular = 0;
//   fout.landShadow = 1;
//   fout.shadowIntensity = 0;
//   fout.fog = 1;
//   //fout.color = float4(tex2D(sampler0, input.tDiffuseMap.xy).rgb, 1);
//   
// 
//   // Calculate the color
//   fout.color = GetColorDiffuse(
//     tex2D(sampler0, input.tDiffuseMap.xy),
//     input.ambientColor.a, 0);



  // Calculate the lights
  fout.light = GetLight(
    input.ambientColor,
    input.diffuseColor,
    half3(0, 0, 0));

  // Calculate the color
  fout.color = GetColorDiffuse(
    tex2D(sampler0, input.tDiffuseMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(input.diffuseColor.a, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;
  
  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(Normal)

FOut PFGrass(SGrass input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights
  fout.light = GetLight(
    input.ambientColor,
    input.diffuseColor,
    half3(0, 0, 0));

  // Calculate the color
  fout.color = GetColorDiffuse(
    tex2D(sampler0, input.tDiffuseMap.xy),
    input.ambientColor.a, 0);

  // perform alpha discretization
  fout.color.a = fout.color.a>0.5;
  
  // Calculate the shadow intesity
  
  if (sbShadows)
  {
    // ammount of lighting is transferred in input.landShadow.a only
    fout.shadowIntensity = GetShadowIntensity(1, input.tPSShadowReduction_Fog);
    // Calculate the land shadow
    fout.landShadow = input.landShadow.a;
  }
  else
  {
    // we can provide result value directly - separate direct/indirect not needed
    // set up color coefficients so that Final functions use the color we have already calculated
    fout.color.rgb = input.lightingColor*fout.color.rgb;
    
    fout.light.indirect = 0;
    fout.light.direct = 1;
    fout.landShadow = 1;

    fout.shadowIntensity = GetShadowIntensity(input.landShadow.a, input.tPSShadowReduction_Fog);
  }
  fout.fog = input.tPSShadowReduction_Fog.z;
  
  return fout;
}

CREATEPS(Grass)

//////////////////////////////////////////////

FOut PFNormalDXTA(SNormal input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights
  fout.light = GetLight(
    input.ambientColor,
    input.diffuseColor,
    half3(0, 0, 0));

  // Calculate the color
  fout.color = GetColorDiffuseDXTADiscrete(tex2D(sampler0, input.tDiffuseMap.xy), input.ambientColor.a);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(input.diffuseColor.a, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPSSN(NormalDXTA,SNormal)

//////////////////////////////////////////////

FOut PFSpecularAlpha(SSpecularAlpha input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights
  fout.light = GetLight(
    input.ambientColor,
    input.diffuseColor,
    input.specularColor_landShadow.rgb * 2.0f);

  // Calculate the color
  half3 s = input.specularColor_landShadow.rgb * 2.0f;
  half specIntensity = dot(s, s) * 2.0f;
  fout.color = GetColorDiffuse(
    tex2D(sampler0, input.tDiffuseMap.xy),
    input.ambientColor.a, specIntensity);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(input.diffuseColor.a, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.specularColor_landShadow.a;

  return fout;
}

CREATEPS(SpecularAlpha)

//////////////////////////////////////////////

FOut PFSpecularNormalMapGrass(SSpecularNormalMapGrass input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights
  {
    // Indirect light
    fout.light.indirect = input.ambientColor.rgb;

    // Calculate the direct light
    half4 normalMap = tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx);
    half frontCoef = saturate(dot(normalMap.rgb * 2 - 1, input.lightLocal_landShadow.rgb));
    half backCoef = saturate(dot(-(normalMap.rgb * 2 - 1), input.lightLocal_landShadow.rgb));
    fout.light.direct = frontCoef * PSC_Diffuse.rgb + backCoef * PSC_DiffuseBack.rgb;

    // No specural light
    fout.light.specular = half3(0, 0, 0);
  }

  // Calculate the color
  fout.color = GetColorDiffuse(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(1, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.lightLocal_landShadow.a;

  return fout;
}

CREATEPS(SpecularNormalMapGrass)

//////////////////////////////////////////////

FOut PFSpecularNormalMapThrough(SSpecularNormalMapThrough input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights and diffuse attenuation
  half da;
  half4 normalMap;
  {
    // perform ambient attenuation
    half4 ambientAtten = tex2D(sampler3, input.tDiffuseAtten_AmbientAtten.wzyx);
    fout.light.indirect = ambientAtten.rgb * input.ambientColor.rgb;
    
    // perform diffuse attenuation via rescaling diffuse result
    normalMap = tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx);
    half frontCoef = saturate(dot(normalMap.rgb * 2 - 1, input.lightLocal_landShadow.rgb));
    half backCoef = saturate(dot(-(normalMap.rgb * 2 - 1), input.lightLocal_landShadow.rgb));
    half diffuseAtten = tex2D(sampler2, input.tDiffuseAtten_AmbientAtten.xy).a;
    half lightCoef = backCoef * PSC_DiffuseBack.a + frontCoef;
    fout.light.direct = diffuseAtten * (lightCoef * PSC_Diffuse.rgb + input.diffuseColor.rgb) /*Including forceddiffuse - can be removed when using PS forced diffuse*/;
    da = diffuseAtten * frontCoef;
  
    // No specular light
    fout.light.specular = half3(0, 0, 0);
  }

  // Calculate the color
  fout.color = GetColorDiffuseDiscreteNoise(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    input.ambientColor.a,
    normalMap.a
  );
  
  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensityFast(da, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.lightLocal_landShadow.a;

  return fout;
}

CREATEPS(SpecularNormalMapThrough)

//////////////////////////////////////////////

FOut PFSpecularNormalMapThroughSimple(SSpecularNormalMapThrough input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights and diffuse attenuation
  half da;
  half4 normalMap;
  {
    // Ambient light
    fout.light.indirect = input.ambientColor.rgb;
    
    // perform diffuse attenuation via rescaling diffuse result
    normalMap = tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx);
    half frontCoef = saturate(dot(normalMap.rgb * 2 - 1, input.lightLocal_landShadow.rgb));
    half backCoef = saturate(dot(-(normalMap.rgb * 2 - 1), input.lightLocal_landShadow.rgb));
    half lightCoef = backCoef * PSC_DiffuseBack.a + frontCoef;
    fout.light.direct = (lightCoef * PSC_Diffuse.rgb + input.diffuseColor.rgb) /*Including forceddiffuse - can be removed when using PS forced diffuse*/;
    da = frontCoef;
  
    // No specular light
    fout.light.specular = half3(0, 0, 0);
  }

  // Calculate the color
  fout.color = GetColorDiffuseDiscreteNoise(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    input.ambientColor.a,
    normalMap.a
  );

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensityFast(da, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.lightLocal_landShadow.a;

  return fout;
}

CREATEPSSN(SpecularNormalMapThroughSimple,SSpecularNormalMapThrough)

//////////////////////////////////////////////

FOut PFSpecularNormalMapSpecularThrough(SSpecularNormalMapSpecularThrough input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights and diffuse attenuation
  half da;
  half4 normalMap;
  {
    // perform ambient attenuation
    half4 ambientAtten = tex2D(sampler3, input.tDiffuseAtten_AmbientAtten.wzyx);
    fout.light.indirect = ambientAtten.rgb * input.ambientColor.rgb;
    
    // perform diffuse attenuation via rescaling diffuse result
    normalMap = tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx);
    half3 normal = normalMap.rgb * 2 - 1;
    half frontBack = dot(normal, input.lightLocal_landShadow.rgb);
    half frontCoef = saturate(frontBack);
    half backCoef = saturate(-frontBack);
    half diffuseAtten = tex2D(sampler2, input.tDiffuseAtten_AmbientAtten.xy).a;
    half lightCoef = backCoef * PSC_DiffuseBack.a + frontCoef;
    fout.light.direct = diffuseAtten * (lightCoef * PSC_Diffuse.rgb + input.diffuseColor.rgb) /*Including forceddiffuse - can be removed when using PS forced diffuse*/;
    da = diffuseAtten * lightCoef;
    
    // Calculate specular light
    half4 litVector = lit(frontBack, dot(normal, input.tHalfway.xyz), PSC_Specular.a);
    half coefSpecular = litVector.z;
    coefSpecular = saturate(coefSpecular); // Specular power can be very high and this can probably lead to infinity
    fout.light.specular = PSC_Specular * coefSpecular * diffuseAtten;
  }

  // Calculate the color
  fout.color = GetColorDiffuseDiscreteNoise(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    input.ambientColor.a,
    normalMap.a
  );

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensityFast(da, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.lightLocal_landShadow.a;

  return fout;
}

CREATEPS(SpecularNormalMapSpecularThrough)

//////////////////////////////////////////////

FOut PFSpecularNormalMapSpecularThroughSimple(SSpecularNormalMapSpecularThrough input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights and diffuse attenuation
  half da;
  half4 normalMap;
  {
    // Ambient light
    fout.light.indirect = input.ambientColor.rgb;
    
    // perform diffuse attenuation via rescaling diffuse result
    normalMap = tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx);
    half3 normal = normalMap.rgb * 2 - 1;
    half frontBack = dot(normal, input.lightLocal_landShadow.rgb);
    half frontCoef = saturate(frontBack);
    half backCoef = saturate(-frontBack);
    half lightCoef = backCoef * PSC_DiffuseBack.a + frontCoef;
    fout.light.direct = (lightCoef * PSC_Diffuse.rgb + input.diffuseColor.rgb) /*Including forceddiffuse - can be removed when using PS forced diffuse*/;
    da = lightCoef;
    
    // Calculate specular light
    half4 litVector = lit(frontBack, dot(normal, input.tHalfway.xyz), PSC_Specular.a);
    half coefSpecular = litVector.z;
    coefSpecular = saturate(coefSpecular); // Specular power can be very high and this can probably lead to infinity
    fout.light.specular = PSC_Specular * coefSpecular;
  }

  // Calculate the color
  fout.color = GetColorDiffuseDiscreteNoise(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    input.ambientColor.a,
    normalMap.a
  );

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensityFast(da, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.lightLocal_landShadow.a;

  return fout;
}

CREATEPSSN(SpecularNormalMapSpecularThroughSimple,SSpecularNormalMapSpecularThrough)

//////////////////////////////////////////////

FOut PFNormalMapThroughLowEnd(SSpecularNormalMapThrough input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights and diffuse attenuation
  half da;
  {
    // Ambient light
    fout.light.indirect = input.ambientColor.rgb;
    
    // perform diffuse attenuation via rescaling diffuse result
    half3 normal = half3(0, 0, 1);
    half frontBack = dot(normal, input.lightLocal_landShadow.rgb);
    half frontCoef = saturate(frontBack);
    half backCoef = saturate(-frontBack);
    half lightCoef = backCoef * PSC_DiffuseBack.a + frontCoef;
    fout.light.direct = (lightCoef * PSC_Diffuse.rgb + input.diffuseColor.rgb) /*Including forceddiffuse - can be removed when using PS forced diffuse*/;
    da = lightCoef;
    
    // Calculate specular light
    fout.light.specular = half3(0, 0, 0);
  }

  // Calculate the color
  fout.color = GetColorDiffuseDiscreteNoise(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    input.ambientColor.a,
    0.5f);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensityFast(da, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.lightLocal_landShadow.a;

  return fout;
}

CREATEPSSN(NormalMapThroughLowEnd,SSpecularNormalMapThrough)

//////////////////////////////////////////////

FOut PFDetailMacroAS(SDetailMacroAS input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  fout.light = GetLightAS(
    input.ambientColor,
    input.diffuseColor,
    half3(0, 0, 0),
    tex2D(sampler3, input.tMacroMap_ShadowMap.wzyx));

  // Calculate the color
  fout.color = GetColorDiffuseDetailMacroAS(
    tex2D(sampler0, input.tDiffuseMap_DetailMap.xy),
    tex2D(sampler1, input.tDiffuseMap_DetailMap.wzyx),
    tex2D(sampler2, input.tMacroMap_ShadowMap.xy),
    input.ambientColor.a, 0);
    
  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(input.diffuseColor.a, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(DetailMacroAS)

//////////////////////////////////////////////

FOut PFDetailSpecularAlphaMacroAS(SDetailSpecularAlphaMacroAS input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  fout.light = GetLightAS(
    input.ambientColor,
    input.diffuseColor,
    input.specularColor_landShadow.rgb * 2.0f,
    tex2D(sampler3, input.tMacroMap_ShadowMap.wzyx));

  /*
    Not transcribed the handlig of alpha from the original shader:
    mov_x2 r1, v1
    dp3_x2 r1, r1, r1
  */

  // Calculate the color
  fout.color = GetColorDiffuseDetailMacroAS(
    tex2D(sampler0, input.tDiffuseMap_DetailMap.xy),
    tex2D(sampler1, input.tDiffuseMap_DetailMap.wzyx),
    tex2D(sampler2, input.tMacroMap_ShadowMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(input.diffuseColor.a, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.specularColor_landShadow.a;

  return fout;
}

CREATEPS(DetailSpecularAlphaMacroAS)

//////////////////////////////////////////////

FOut PFNormalMap(SNormalMap input, bool sbShadows)
{
  FOut fout;
  
  // Get the irradiance data
  half4 normal = tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx);
  half4 irradiance = GetIrradianceData(input.tLightLocalAndFog.xyz * 2 - 1, input.tHalfway, normal, sampler3);

  // Calculate the lights
  fout.light = GetLightIrradiance(
    input.ambientColor,
    irradiance,
    normal.a);

  // Calculate the color
  fout.color = GetColorDiffuse(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(irradiance.b, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(NormalMap)

//////////////////////////////////////////////

FOut PFNormalMapDetailMacroASSpecularMap(SNormalMapDetailMacroASSpecularMap input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  half coefDiffuse;
  fout.light = GetLightNormalSpecularAS(input.ambientColor,
    tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx), input.tLightLocalAndFog.xyz * 2 - 1, input.tHalfway,
    tex2D(sampler5, input.tShadowMap_SpecularMap.wzyx),
    tex2D(sampler4, input.tShadowMap_SpecularMap.xy),
    coefDiffuse);

  // Calculate the color
  fout.color = GetColorDiffuseDetailMacroAS(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    tex2D(sampler2, input.tDetailMap_MacroMap.xy),
    tex2D(sampler3, input.tDetailMap_MacroMap.wzyx),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(NormalMapDetailMacroASSpecularMap)

//////////////////////////////////////////////

FOut PFNormalMapDetailMacroASSpecularDIMap(SNormalMapDetailMacroASSpecularDIMap input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  half coefDiffuse;
  fout.light = GetLightNormalSpecularDIAS(input.ambientColor,
    tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx), input.tLightLocalAndFog.xyz * 2 - 1, input.tHalfway,
    tex2D(sampler5, input.tShadowMap_SpecularMap.wzyx),
    tex2D(sampler4, input.tShadowMap_SpecularMap.xy),
    coefDiffuse);

  // Calculate the color
  fout.color = GetColorDiffuseDetailMacroAS(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    tex2D(sampler2, input.tDetailMap_MacroMap.xy),
    tex2D(sampler3, input.tDetailMap_MacroMap.wzyx),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(NormalMapDetailMacroASSpecularDIMap)

//////////////////////////////////////////////

FOut PFNormalMapDetailSpecularMap(SNormalMapDetailSpecularMap input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  half coefDiffuse;
  fout.light = GetLightNormalSpecular(input.ambientColor,
    tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx), input.tLightLocalAndFog.xyz * 2 - 1, input.tHalfway,
    tex2D(sampler3, input.tDetailMap_SpecularMap.wzyx),
    coefDiffuse);

  // Calculate the color
  fout.color = GetColorDiffuseDetail(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    tex2D(sampler2, input.tDetailMap_SpecularMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(NormalMapDetailSpecularMap)

//////////////////////////////////////////////

FOut PFNormalMapDetailSpecularDIMap(SNormalMapDetailSpecularDIMap input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  half coefDiffuse;
  fout.light = GetLightNormalSpecularDI(input.ambientColor,
    tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx), input.tLightLocalAndFog.xyz * 2 - 1, input.tHalfway,
    tex2D(sampler3, input.tDetailMap_SpecularMap.wzyx),
    coefDiffuse);

  // Calculate the color
  fout.color = GetColorDiffuseDetail(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    tex2D(sampler2, input.tDetailMap_SpecularMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(NormalMapDetailSpecularDIMap)

//////////////////////////////////////////////

FOut PFNormalMapMacroAS(SNormalMapMacroAS input, bool sbShadows)
{
  FOut fout;
  
  // Get the irradiance data
  half4 normal = tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx);
  half4 irradiance = GetIrradianceData(input.tLightLocalAndFog.xyz * 2 - 1, input.tHalfway, normal, sampler3);

  // Calculate the lights
  fout.light = GetLightIrradiance(input.ambientColor, irradiance, normal.a);

  // Calculate the color
  fout.color = GetColorDiffuseMacroAS(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    tex2D(sampler2, input.tMacroMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(irradiance.b, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(NormalMapMacroAS)

//////////////////////////////////////////////

FOut PFNormalMapMacroASSpecularMap(SNormalMapMacroASSpecularMap input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights
  half coefDiffuse;
  fout.light = GetLightNormalSpecularAS(input.ambientColor,
    tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx), input.tLightLocalAndFog.xyz * 2 - 1, input.tHalfway,
    tex2D(sampler4, input.tSpecularMap.xy),
    tex2D(sampler3, input.tMacroMap_ShadowMap.wzyx),
    coefDiffuse);

  // Calculate the color
  fout.color = GetColorDiffuseMacroAS(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    tex2D(sampler2, input.tMacroMap_ShadowMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(NormalMapMacroASSpecularMap)

//////////////////////////////////////////////

FOut PFNormalMapMacroASSpecularDIMap(SNormalMapMacroASSpecularDIMap input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights
  half coefDiffuse;
  fout.light = GetLightNormalSpecularDIAS(input.ambientColor,
    tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx), input.tLightLocalAndFog.xyz * 2 - 1, input.tHalfway,
    tex2D(sampler4, input.tSpecularMap.xy),
    tex2D(sampler3, input.tMacroMap_ShadowMap.wzyx),
    coefDiffuse);

  // Calculate the color
  fout.color = GetColorDiffuseMacroAS(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    tex2D(sampler2, input.tMacroMap_ShadowMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(NormalMapMacroASSpecularDIMap)

//////////////////////////////////////////////

FOut PFNormalMapSpecularMap(SNormalMapSpecularMap input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  half coefDiffuse;
  fout.light = GetLightNormalSpecular(input.ambientColor,
    tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx), input.tLightLocalAndFog.xyz * 2 - 1, input.tHalfway,
    tex2D(sampler2, input.tSpecularMap.xy),
    coefDiffuse);

  // Calculate the color
  fout.color = GetColorDiffuse(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(NormalMapSpecularMap)

//////////////////////////////////////////////

FOut PFNormalMapSpecularDIMap(SNormalMapSpecularDIMap input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  half coefDiffuse;
  fout.light = GetLightNormalSpecularDI(input.ambientColor,
    tex2D(sampler1, input.tDiffuseMap_NormalMap.wzyx), input.tLightLocalAndFog.xyz * 2 - 1, input.tHalfway,
    tex2D(sampler2, input.tSpecularMap.xy),
    coefDiffuse);

  // Calculate the color
  fout.color = GetColorDiffuse(
    tex2D(sampler0, input.tDiffuseMap_NormalMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.landShadow.a;

  return fout;
}

CREATEPS(NormalMapSpecularDIMap)

//////////////////////////////////////////////

FOut PFSpecularNormalMapDiffuse(SSpecularNormalMapDiffuse input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  half coefDiffuse;
  fout.light = GetLightNormal(input.ambientColor, input.specularColor_landShadow.rgb,
    tex2D(sampler1, input.tColorMap_NormalMap.wzyx), input.tLightLocalAndFog.xyz * 2 - 1, coefDiffuse);

  // Calculate the color
  fout.color = GetColorDiffuseDetail(
    tex2D(sampler0, input.tColorMap_NormalMap.xy),
    tex2D(sampler2, input.tDetailMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.specularColor_landShadow.a;

  return fout;
}

CREATEPS(SpecularNormalMapDiffuse)

//////////////////////////////////////////////

FOut PFSpecularNormalMapDiffuseMacroAS(SSpecularNormalMapDiffuseMacroAS input, bool sbShadows)
{
  FOut fout;

  // Calculate the lights
  half coefDiffuse;
  fout.light = GetLightNormalAS(input.ambientColor, input.specularColor_landShadow.rgb,
    tex2D(sampler1, input.tColorMap_NormalMap.wzyx), input.tLightLocalAndFog.xyz * 2 - 1,
    tex2D(sampler4, input.tShadowMap.xy),
    coefDiffuse);

  // Calculate the color
  fout.color = GetColorDiffuseDetailMacroAS(
    tex2D(sampler0, input.tColorMap_NormalMap.xy),
    tex2D(sampler2, input.tDetailMap_MacroMap.xy),
    tex2D(sampler3, input.tDetailMap_MacroMap.wzyx),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = input.specularColor_landShadow.a;

  return fout;
}

CREATEPS(SpecularNormalMapDiffuseMacroAS)

//////////////////////////////////////////////

FOut PFTerrainX2(STerrain input, half3 primTexture, half4 normalMap, half3 detTexture, bool simple, bool grass, bool sbShadows)
{
  FOut fout;

  // Sample satelite and mask maps
  float2 satMask = input.tSatAndMask_GrassMap.xy;
  half3 satTexture   = tex2D(samplers[0],satMask);

  float2 noMap;
  if (!grass)
  {
    noMap = input.tDetailMap_SpecularMap.xy;
  }
  else
  {
    // grass layer needs a specific "normal map stage" mapping
    noMap = input.tSatAndMask_GrassMap.wz;
  }
  
  // lerp - detail map / satelitte map
  // distance is given in ambientColor.a
  // Get the light in local vertex space
  half3 lightDir = input.tLightLocalAndFog.xyz * 2 - 1;
  // Calculate the texel color
  half3 colorTexel;
  half diffuse;
  if (grass)
  {
    half perlin = tex2D(samplers[2+2],noMap).a;
    // grass is rendered for middle distance
    // no detail textures needed for the grass
    colorTexel = lerp(satTexture*primTexture*2,normalMap.rgb,input.grassAlpha.y);
    // normal map has different meaning - alpha used only for alpha testing
    diffuse = saturate(lightDir.z);
    clip(normalMap.a*input.grassAlpha.x+perlin-1);
  }
  else if (simple)
  {
    half detFactor = input.ambientColor.a;
    colorTexel = lerp(satTexture,detTexture,detFactor);
    diffuse = saturate(lightDir.z);
  }
  else
  {
    half detFactor = input.ambientColor.a;
    half3 detSatMap = lerp(satTexture*primTexture*2,detTexture,detFactor);
    colorTexel = detSatMap;

    // Get the normal from normal map
    // we know the normal map is DXT5 compressed (hq, vhq or nopx)
    half3 normal = (half3(1 - normalMap.a,0,0) + normalMap.rgb)*2-1;

    // Calculate the diffuse coefficient  
    diffuse = saturate(dot(normal, lightDir));
  }

  // Fill out the Light structure
  fout.light.indirect = input.ambientColor.rgb;
  fout.light.direct = diffuse * PSC_Diffuse;
  fout.light.specular = input.specularColor.rgb;

  // Calculate the color
  fout.color = half4(colorTexel, 1);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(diffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  // specularColor alpha contains depth under the shadow horizon
  // depth=0 -> shadow = 1
  // depth=1 -> shadow = 0
  fout.landShadow = saturate(0.5-input.specularColor.a*0.5)*0.8+0.2;

  return fout;
}

#define F_TERR(x) \
  primTexture = tex2D(samplers[x*3+2],coMap); \
  normalMap   = tex2D(samplers[x*3+3],noMap); \
  detTexture  = tex2D(samplers[x*3+4],dtMap);

#define N_TERR(x,m) \
  primTexture = lerp(primTexture, tex2D(samplers[x*3+2], coMap), maskTexture[m-1]); \
  normalMap   = lerp(normalMap,   tex2D(samplers[x*3+3], noMap), maskTexture[m-1]); \
  detTexture  = lerp(detTexture,  tex2D(samplers[x*3+4], dtMap), maskTexture[m-1]);

void LayerBlend1(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0)}
void LayerBlend2(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0)}
void LayerBlend3(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0);N_TERR(1,1)}
void LayerBlend4(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0)}
void LayerBlend5(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0);N_TERR(1,2)}
void LayerBlend6(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0);N_TERR(1,2)}
void LayerBlend7(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0);N_TERR(1,1);N_TERR(2,2)}
void LayerBlend8(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0)}
void LayerBlend9(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0);N_TERR(1,3)}
void LayerBlend10(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0);N_TERR(1,3)}
void LayerBlend11(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0);N_TERR(1,1);N_TERR(2,3)}
void LayerBlend12(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0);N_TERR(1,3)}
void LayerBlend13(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0);N_TERR(1,2);N_TERR(2,3)}
void LayerBlend14(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0);N_TERR(1,2);N_TERR(2,3)}
void LayerBlend15(in float2 coMap, in float2 noMap, in float2 dtMap, half4 maskTexture, out half3 primTexture, out half4 normalMap, out half3 detTexture) {F_TERR(0);N_TERR(1,1);N_TERR(2,2);N_TERR(3,3)}

#define TERRAINLAYER(x) \
FOut PFTerrain##x(STerrain input, bool sbShadows) \
{ \
  half3 primTexture; \
  half4 normalMap; \
  half3 detTexture; \
  float2 coMap = input.tColorMap_NormalMap.xy; \
  float2 dtMap = input.tDetailMap_SpecularMap.xy; \
  float2 noMap = dtMap; \
  float2 satMask = input.tSatAndMask_GrassMap.xy; \
  LayerBlend##x(coMap, noMap, dtMap, tex2D(samplers[1],satMask), primTexture, normalMap, detTexture); \
  return PFTerrainX2(input, primTexture, normalMap, detTexture, false, false, sbShadows); \
} \
FOut PFTerrainSimple##x(STerrain input, bool sbShadows) \
{ \
  half3 primTexture; \
  half4 normalMap; \
  half3 detTexture; \
  float2 coMap = input.tColorMap_NormalMap.xy; \
  float2 dtMap = input.tDetailMap_SpecularMap.xy; \
  float2 noMap = dtMap; \
  float2 satMask = input.tSatAndMask_GrassMap.xy; \
  LayerBlend##x(coMap, noMap, dtMap, tex2D(samplers[1],satMask), primTexture, normalMap, detTexture); \
  return PFTerrainX2(input, primTexture, normalMap, detTexture, true, false, sbShadows); \
} \
FOut PFTerrainGrass##x(STerrain input, bool sbShadows) \
{ \
  half3 primTexture; \
  half4 normalMap; \
  half3 detTexture; \
  float2 coMap = input.tColorMap_NormalMap.xy; \
  float2 dtMap = input.tDetailMap_SpecularMap.xy; \
  float2 noMap = input.tSatAndMask_GrassMap.wz; \
  float2 satMask = input.tSatAndMask_GrassMap.xy; \
  LayerBlend##x(coMap, noMap, dtMap, tex2D(samplers[1],satMask), primTexture, normalMap, detTexture); \
  return PFTerrainX2(input, primTexture, normalMap, detTexture, true, true, sbShadows); \
} \
CREATEPSSN(Terrain##x,STerrain) \
CREATEPSSN(TerrainSimple##x,STerrain)\
CREATEPSSN(TerrainGrass##x,STerrain)

///////////////////////////////////////////////////////////////////////////////

TERRAINLAYER(1)
TERRAINLAYER(2)
TERRAINLAYER(3)
TERRAINLAYER(4)
TERRAINLAYER(5)
TERRAINLAYER(6)
TERRAINLAYER(7)
TERRAINLAYER(8)
TERRAINLAYER(9)
TERRAINLAYER(10)
TERRAINLAYER(11)
TERRAINLAYER(12)
TERRAINLAYER(13)
TERRAINLAYER(14)
TERRAINLAYER(15)

//////////////////////////////////////////////
// very similar to PFTerrain and PFNormalMapDetailSpecularMap

FOut PFRoad(STerrain input, bool sbShadows)
{
  FOut fout;
  
  // Calculate the lights
  half coefDiffuse;
  fout.light = GetLightNormalSpecular(input.ambientColor,
    tex2D(sampler1, input.tColorMap_NormalMap.wzyx), input.tLightLocalAndFog.xyz * 2 - 1, input.tHalfway,
    tex2D(sampler3, input.tDetailMap_SpecularMap.wzyx),
    coefDiffuse);

  // Calculate the color
  fout.color = GetColorDiffuseDetail(
    tex2D(sampler0, input.tColorMap_NormalMap.xy),
    tex2D(sampler2, input.tDetailMap_SpecularMap.xy),
    input.ambientColor.a, 0);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  // specularColor alpha contains depth under the shadow horizon
  // depth=0 -> shadow = 1
  // depth=1 -> shadow = 0
  fout.landShadow = saturate(0.5-input.specularColor.a*0.5)*0.8+0.2;

  return fout;
}

CREATEPSSN(Road,STerrain)

//////////////////////////////////////////////

FOut PFRoad2Pass(STerrain input, bool sbShadows)
{
  FOut fout;
  
  // Kill output in case of alfa
  clip(tex2D(sampler0, input.tColorMap_NormalMap.xy).a - 0.5f);
  
  // Calculate the lights
  half coefDiffuse = max(dot(half3(0, 0, 1), input.tLightLocalAndFog.xyz * 2.0f - 1.0f), 0.0f);
  fout.light = GetLight(half3(0, 0, 0), half3(0, 0, 0), half3(0, 0, 0));

  // Calculate the color
  fout.color = half4(0, 0, 0, 1);

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(coefDiffuse, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  // specularColor alpha contains depth under the shadow horizon
  // depth=0 -> shadow = 1
  // depth=1 -> shadow = 0
  fout.landShadow = saturate(0.5-input.specularColor.a*0.5)*0.8+0.2;

  return fout;
}

CREATEPSSN(Road2Pass,STerrain)

//////////////////////////////////////////////

FOut PFShore(SShore input, bool sbShadows)
{
  // Get the normal
  half3 normal;
  {
    // Sample 3 normals from the source
    half4 normal1 = tex2D(sampler1, input.tNormalMap1_2.xy);
    half4 normal2 = tex2D(sampler1, input.tNormalMap1_2.zw);
    half4 normal3 = tex2D(sampler1, input.tNormalMap3.xy);
    
    // perform DXT5 swizzle and bx2 conversion
    half4 normalSrc = (normal1+normal2+normal3)*0.333f;
    normalSrc.x += (1-normalSrc.a);
    normalSrc.xy -= 0.5;
    // recalculate z based on xy
    normalSrc.z = 0.333;
    //normal = normalize(normalSrc.xyz);
    //normal = half3(0, 0, 1); // FLY
    
    // See label NORMAL_DISAPPEAR
    const half maxDisappear = 0.5f;
    normal = normalize(lerp(normalSrc.xyz, half3(0, 0, 1), maxDisappear));
  }
  
  // Get value from the reflection map
  half3 envColor;
  {
    // Get the LWS position
    float3 lwsPosition = float3(input.stn2lws0.w, input.stn2lws1.w, input.stn2lws2.w);
    
    // Get the LWS normal
    float3 lwsNormal;
    {
      lwsNormal.x = dot(input.stn2lws0, normal);
      lwsNormal.y = dot(input.stn2lws1, normal);
      lwsNormal.z = dot(input.stn2lws2, normal);
    }

    // Calculate the O vector (the reflection vector)
    float3 o;
    {
      float3 v = normalize(-lwsPosition.xyz);
      o = lwsNormal.xyz * dot(lwsNormal.xyz, v) * 2.0f - v;
    }

    // Modify O vector to favour bounding texels, multiply by constant to omit degenerated texels
    float oSize2 = dot(o.xz,o.xz);
    o.xz = o.xz * oSize2 * 0.95f;

    // Get color from the environmental map
    half3 envColorA = tex2D(sampler4, o.zx * float2(-0.5,+0.5) + 0.5);
    half3 envColorB = tex2D(sampler5, o.zx * float2(-0.5,+0.5) + 0.5);
    envColor = lerp(envColorB, envColorA, input.tFresnelView.w);
  }

  // Get the fresnel specular coefficient
  half4 fresnelSpecular;
  {
    // Get the specular halfway vector
    half3 specularHalfway = normalize(input.specularHalfway.rgb * 2.0f - 1.0f);
  
    // Get the sample coordinates and sample the reflect map
    half u = dot(normal, normalize(input.tFresnelView));
    half v = dot(normal, specularHalfway);
    fresnelSpecular = tex2D(sampler3, half2(u,v));
  }

  // Sum environmental color and sun specular - result is total reflected color
  half3 reflectedColor = envColor * PSC_GlassEnvColor + fresnelSpecular.rgb * PSC_Specular;
  
  // Calculate the water factor (1 - on wave, 0 - biggest distance from wave)
  half waveFactorA = saturate(input.tHeight.w + input.tHeight.x);
  half waveFactorB = saturate(input.tHeight.w + input.tHeight.y);

  // Calculate the water factor (almost discrete output depending on current wave)
  half waterFactorA = 1.0f - saturate((waveFactorA - 0.80f) * 10.0f);
  half waterFactorB = 1.0f - saturate((waveFactorB - 0.80f) * 10.0f);
  
  // Modify waterFactor - disappear water with height
  half waterFactor = saturate(saturate(1.2f - /*abs*/(input.tHeight.w * input.tHeight.w)) * max(waterFactorA, waterFactorB));

  FOut fout;

  // Calculate the lights
  fout.light.indirect = float3(1, 1, 1);
  fout.light.direct = float3(0, 0, 0);
  fout.light.specular = lerp(input.ambientColor.rgb, reflectedColor, fresnelSpecular.a) * waterFactor;

  // Calculate the color
  half alpha = max(input.ambientColor.a, fresnelSpecular.a);
  fout.color = half4(0, 0, 0, waterFactor * alpha);

  // Calculate the shadow intesity
  fout.shadowIntensity = 0;
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = 1;

  return fout;
}

CREATEPSSN_NS(Shore,SShore)

//////////////////////////////////////////////

FOut PFShoreFoam(SShore input, bool sbShadows)
{
  // Get the foam
  half foam;
  {
    half foam1 = tex2D(sampler2, input.tNormalMap1_2.xy * 8).x;
    half foam2 = tex2D(sampler2, input.tNormalMap1_2.zw * 8).x;
    half foam3 = tex2D(sampler2, input.tNormalMap3.xy * 8).x;
    foam = saturate(foam1 + foam2 + foam3 - 2.1875);
  }
  
  // Calculate the wave factor (1 - on wave, 0 - biggest distance from wave)
  half waveFactorA = saturate(input.tHeight.w + input.tHeight.x);
  half waveFactorB = saturate(input.tHeight.w + input.tHeight.y);

  // Calculate the water factor (almost discrete output depending on current wave)
  half waterFactorA = 1.0f - saturate((waveFactorA - 0.80f) * 10.0f);
  half waterFactorB = 1.0f - saturate((waveFactorB - 0.80f) * 10.0f);
  
  // Calculate the foam factor (sample the foam texture with distance to the top of the wave),
  half foamFactor;
  {
    // Calculate foam factors according to waveFactors
    // /*restrict it only to where water is drawn (*waterFactor)*/
    half foamFactorA = (pow(waveFactorA, 10) * 11 + input.tHeight.w * 0.7) * waterFactorA;
    half foamFactorB = (pow(waveFactorB, 10) * 11 + input.tHeight.w * 0.7) * waterFactorB;

    // Get the foam factor
    foamFactor = foam * max(foamFactorA, foamFactorB);

    // Modify foam factor - disappear foam that lies low
    half h = input.tHeight.w * 2.0f - 0.3f;
    foamFactor = saturate(pow(h, 4)) * saturate(foamFactor);
  }

  FOut fout;

  // Calculate the lights
  fout.light.indirect = float3(1, 1, 1);
  fout.light.direct = float3(0, 0, 0);
  fout.light.specular = float3(0, 0, 0);

  // Calculate the color
  fout.color = half4(PSC_WaveColor.rgb, foamFactor);

  // Calculate the shadow intesity
  fout.shadowIntensity = 0;
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = 1;

  return fout;
}

CREATEPSSN_NS(ShoreFoam,SShore)

//////////////////////////////////////////////

FOut PFShoreWet(SShoreWet input, bool sbShadows)
{
  // Calculate the wet factor (darkenning of shore disappearing with distance above water)
  half wetFactor = saturate(2.0f - abs(input.tHeight.w)) * 0.4f;

  // Fill out the FOut structure
  FOut fout;

  // Calculate the lights
  fout.light = GetLight(float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, 0));

  // Calculate the color
  fout.color = half4(0, 0, 0, wetFactor);

  // Calculate the shadow intesity
  fout.shadowIntensity = 0;
  fout.fog = input.tPSShadowReduction_Fog.z;

  // Calculate the land shadow
  fout.landShadow = 1;

  return fout;
}

CREATEPSSN_NS(ShoreWet,SShoreWet)

//////////////////////////////////////////////

FOut PFGlass(SGlass input, bool sbShadows)
{
  // Calculate the O vector (the reflection vector)
  half3 o;
  {
    half3 v = normalize(-input.lwsPosition.xyz);
    o = input.lwsNormal.xyz * dot(input.lwsNormal.xyz, v) * 2.0f - v;
  }
  
  // Get the color from the environmental map
  //half3 envColor = tex2D(sampler2, o.xy * 0.5 + 0.5);
  half3 envColor = tex2D(sampler2, o.xy * float2(1, -1) * 0.5 + 0.5) * PSC_GlassEnvColor.rgb;
  
  // Get the fresel reflection coefficient
  half fresnelCoef = tex2D(sampler1, float2(dot(input.lwsNormal.xyz, o), 0.5)).a;

  FOut fout;

  // Sample the diffuse color
  half4 diffuseColor = tex2D(sampler0, input.tDiffuseMap.xy);
  clip(diffuseColor.a - 1.0f/255.0f); // Clip output in case of 100% alpha

  // Calculate the lights
  fout.light.indirect = input.ambientColor;
  fout.light.direct = input.diffuseColor;
  half3 baseSpecular = (input.specularColor_landShadow.rgb + envColor * PSC_GlassMatSpecular) * fresnelCoef * 2.0f; // 2 is here because specular colors are halved
  fout.light.specular = baseSpecular * lerp(half3(1.0f, 1.0f, 1.0f), diffuseColor.rgb, diffuseColor.a);

  // Calculate the color
  fout.color = half4(
    diffuseColor.rgb * (1-fresnelCoef),
    lerp(diffuseColor.a * input.ambientColor.a, 1, fresnelCoef));

  // Calculate the shadow intesity
  fout.shadowIntensity = GetShadowIntensity(input.diffuseColor.a, input.tPSShadowReduction_Fog);
  fout.fog = input.tPSShadowReduction_Fog.z;
  
  // Calculate the land shadow
  fout.landShadow = input.specularColor_landShadow.a;

  return fout;
}

CREATEPS(Glass)

/////////////////////////////////////////////////////////////////////////////////////////////
// Special pixel shaders - no shadow versions required

half4 PSInterpolation(SNormal input) : COLOR
{
  half4 diffuseMapA = tex2D(sampler0, input.tDiffuseMap.xy);
  half4 diffuseMapB = tex2D(sampler1, input.tDiffuseMap.xy);

  // Interpolate between A and B
  half4 diffuse = lerp(diffuseMapB, diffuseMapA, input.ambientColor.a);

  // reconstruct colors from DXT5 compression
  // do it in such a way that even DXT1 textures work
  half3 diffuseFinal = half3(0, 1 - diffuse.a, 0) + diffuse;

  return half4((input.ambientColor.rgb /*+ input.diffuseColor*/) * diffuseFinal, 1);
}

//////////////////////////////////////////////

float4 PSWhite() : COLOR
{
  return float4(1, 1, 1, 1);
}

//////////////////////////////////////////////

/// simple pixel shader indended for UI and other 2D stuff
float4 PSNonTL(SNormal input) : COLOR
{
  half4 color = tex2D(sampler0, input.tDiffuseMap.xy);
  
  color.a = color.a*input.ambientColor.a;
  color.rgb = color.rgb*(input.ambientColor+input.diffuseColor);
  return color;
}

//////////////////////////////////////////////

float4 PSWhiteAlpha(
  float4 tDiffuseMap:TEXCOORD0) : COLOR
{
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);
  return half4(1, 1, 1, diffuseMap.a);
}

//////////////////////////////////////////////

float4 PSShadowBufferAlpha_Default(float4 tDiffuseMap:TEXCOORD0, float4 tPosition:TEXCOORD7) : COLOR0
{
  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);
  
  // Kill output in case of alfa
  clip(diffuseMap.a - 0.5);

  // Write out the Z  
  return float4(tPosition.zzz, 1);
}

//////////////////////////////////////////////

half4 PSShadowBufferAlpha_nVidia(float4 tDiffuseMap:TEXCOORD0) : COLOR0
{
  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);

  // It doesn't matter what we write to color map
  return diffuseMap;
}

//////////////////////////////////////////////

float4 PSInvDepthAlphaTest(float4 tDiffuseMap:TEXCOORD0, float4 tPosition:TEXCOORD7) : COLOR0
{
  // Kill output in case it is too close
  clip(tPosition.z - PSC_DepthClip.w);

  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);
  
  // Kill output in case of alfa
  clip(diffuseMap.a - 0.5);

  // Write out the inverse of Z  
  return float4(1.0f / tPosition.zzz, 1);
}

//////////////////////////////////////////////

half4 PSReflect(half4 color : TEXCOORDDIFFUSE, float4 tDiffuseMap : TEXCOORD0) : COLOR
{
  // TEXCOORDDIFFUSE.a should contain shadow-encoded amount of diffuse lighting
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);
  return half4(diffuseMap.rgb, color.a);
}

//////////////////////////////////////////////

half4 PSReflectNoShadow(float4 tDiffuseMap : TEXCOORD0) : COLOR
{
  half4 diffuseMap = tex2D(sampler0, tDiffuseMap.xy);
  return half4(diffuseMap.rgb, 1);
}

//////////////////////////////////////////////

float4 PSWater( float4 position                   : SV_POSITION,
                half3 specularHalfway             : COLOR0,
                half4 tPSShadowReduction_Fog      : COLOR1,
                half4 basicColor                  : TEXCOORDAMBIENT, 
                float4 stn2lws2                   : TEXCOORDSPECULAR,
                float4 stn2lws0                   : TEXCOORD4,
                float4 tWaveMap                   : TEXCOORD0,
                float4 tNormalMap1_2              : TEXCOORD1,
                half4 tFresnelView                : TEXCOORD2,
                float2 tNormalMap3                : TEXCOORD3,
                float4 stn2lws1                   : TEXCOORD5) : COLOR0
{
  //return float4(frac(tNormalMap1_2.z), frac(tNormalMap1_2.w), 0, 1);
  
  // z is vertical in local space (normal map)
  half4 normal1 = tex2D(sampler1,tNormalMap1_2.xy);
  half4 normal2 = tex2D(sampler1,tNormalMap1_2.zw);
  half4 normal3 = tex2D(sampler1,tNormalMap3.xy);
  
  // Foam
  half foam1 = tex2D(sampler2,tNormalMap1_2.xy).x;
  half foam2 = tex2D(sampler2,tNormalMap1_2.zw*4).x;
  half foam3 = tex2D(sampler2,tNormalMap3.xy*2).x;
  
//  // 1- is here so that if foam texture is missing, we get white foam
//  half foam = saturate(1-(foam1+foam2+foam3-1.5));
  half foam = saturate(foam1 + foam2 + foam3 - 2.1875);
  
  // x is sum of r + (1-a)
  /*
  // perform DXT5 nomap format 1-a->r swizzle and normalize 
  float4 sumNormals = normal1+normal2+normal3;
  sumNormals.x += (3-sumNormals.a);
  float3 normal = normalize(sumNormals.rgb-1.5);
  */
  
  // perform DXT5 swizzle and bx2 conversion
  half4 normalSrc = (normal1+normal2+normal3)*0.333f;
  normalSrc.x += (1-normalSrc.a);
  normalSrc.xy -= 0.5;
  // recalculate z based on xy
  //normalSrc.z = sqrt(1-dot(normalSrc.xy,normalSrc.xy));
  normalSrc.z = 0.333;
  //sqrt(1-dot(normalSrc.xy,normalSrc.xy));
  
  // Calculate normal disappearing factor (normal map disappears close to shore).
  // The same calculation is done on VS to disappear waves normal modification - see label NORMAL_DISAPPEAR
  const half maxDisappear = 0.5f;
  half normalDisappear = min(pow(tWaveMap.z, 10), maxDisappear);
  //normalDisappear = 0;

  half3 normal = normalize(lerp(normalSrc.xyz, half3(0, 0, 1), normalDisappear));
  
  // Get value from the reflection map
  half3 envColor;
  {
    // Get the LWS position
    float3 lwsPosition = float3(stn2lws0.w, stn2lws1.w, stn2lws2.w);
    
    // Get the LWS normal
    float3 lwsNormal;
    {
      lwsNormal.x = dot(stn2lws0, normal);
      lwsNormal.y = dot(stn2lws1, normal);
      lwsNormal.z = dot(stn2lws2, normal);
    }

    // Calculate the O vector (the reflection vector)
    float3 o;
    {
      float3 v = normalize(-lwsPosition.xyz);
      o = lwsNormal.xyz * dot(lwsNormal.xyz, v) * 2.0f - v;
    }

    // Modify O vector to favour bounding texels, multiply by constant to omit degenerated texels
    float oSize2 = dot(o.xz,o.xz);
    o.xz = o.xz * oSize2 * 0.95f;

    // Get color from the environmental map
    half3 envColorA = tex2D(sampler4, o.zx * float2(-0.5,+0.5) + 0.5);
    half3 envColorB = tex2D(sampler5, o.zx * float2(-0.5,+0.5) + 0.5);
    envColor = lerp(envColorB, envColorA, tFresnelView.w);
  }

  // Get the fresnel specular coefficient
  half4 fresnelSpecular;
  {
    // Get the specular halfway vector
    specularHalfway.rgb = normalize(specularHalfway.rgb * 2.0f - 1.0f);
  
    // Get the sample coordinates and sample the reflect map
    half u = dot(normal, normalize(tFresnelView.xyz));
    half v = dot(normal, specularHalfway);
    fresnelSpecular = tex2D(sampler3, half2(u,v)); // Reflect map
  }

  // Sum environmental color and sun specular - result is total reflected color
  half3 reflectedColor = envColor * PSC_GlassEnvColor + fresnelSpecular.rgb * PSC_Specular.rgb;
  
  half wave = tex2D(sampler0,tWaveMap.xy).x; // Wave map
  half3 bodyColor = basicColor;

  half3 color = lerp(bodyColor,reflectedColor,fresnelSpecular.a);
  
  
  half alpha = max(basicColor.a, fresnelSpecular.a);
  
  half4 base = half4(color,alpha);

  float4 result = base+half4(PSC_WaveColor*wave*foam,0.9*wave*foam);
  return ApplyFog(result, tPSShadowReduction_Fog.z);
}

//////////////////////////////////////////////

float4 PSWaterSimple( float4 basicColor:TEXCOORDAMBIENT, float3 reflectionHalfway:TEXCOORDSPECULAR,
                      float2 tWaveMap:TEXCOORD0, float2 tNormalMap1:TEXCOORD1,
                      float3 tFresnelView:TEXCOORD2,
                      float3 specularHalfway        : COLOR0,
                      half4 tPSShadowReduction_Fog  : COLOR1,
                      float2 tNormalMap2:TEXCOORD4, float2 tNormalMap3:TEXCOORD3) : COLOR0
{
  // z is vertical in local space (normal map)
  float4 normal1 = tex2D(sampler1,tNormalMap1);
  float4 normal2 = tex2D(sampler1,tNormalMap2);
  float4 normal3 = tex2D(sampler1,tNormalMap3);
  
  // x is sum of r + (1-a)
  /*
  // perform DXT5 nomap format 1-a->r swizzle and normalize 
  float4 sumNormals = normal1+normal2+normal3;
  sumNormals.x += (3-sumNormals.a);
  float3 normal = normalize(sumNormals.rgb-1.5);
  */
  
  // perform DXT5 swizzle and bx2 conversion
  float4 normalSrc = (normal1+normal2+normal3)*0.333f;
  normalSrc.x += (1-normalSrc.a);
  normalSrc.xy -= 0.5;
  // recalculate z based on xy
  normalSrc.z = sqrt(1-dot(normalSrc.xy,normalSrc.xy));

  float3 normal = normalSrc.xyz;
  
  reflectionHalfway = reflectionHalfway*2-1;
  
  specularHalfway = normalize(specularHalfway * 2 - 1);
  
  float u = dot(normal,tFresnelView);
  float v = dot(normal,specularHalfway);
  
  float4 fresnelSpecular = tex2D(sampler3,float2(u,v)); // Reflect map
  
  float cosReflectHalf = saturate(dot(normal,reflectionHalfway));
  
  // full ground reflection at value cos(45deg) = 0.70
  // we would like to know cos (2*alfa)
  // cos 2x = 2 cos^2(x) - 1
  float reflected = saturate(2*cosReflectHalf*cosReflectHalf-1);
  float3 reflectedColor = lerp(PSC_GroundReflColor,PSC_SkyReflColor,reflected) ;
  
  reflectedColor += fresnelSpecular.rgb*PSC_Specular.rgb;
  
  float wave = tex2D(sampler0,tWaveMap).x; // Wave map
  float3 bodyColor = basicColor;

  float3 color = lerp(bodyColor,reflectedColor,fresnelSpecular.a);
  
  
  float alpha = max(basicColor.a, fresnelSpecular.a);
  
  float4 base = float4(color,alpha);

  return ApplyFog(base, tPSShadowReduction_Fog.z);
}

//////////////////////////////////////////////

FOut PFCraterX(SCrater input, int nCraters, bool sbShadows)
{
  // Sample the diffuse map
  half4 diffuseMap = tex2D(sampler0, input.tDiffuseMap.xy);

//   // More sofistikated footprint - oval shape
//   float3 closestVector = input.originalPosition.xyz - PSC_Crater[0].xyz;
//   for (int i = 1; i < nCraters; i++)
//   {
//     float3 v = input.originalPosition.xyz - PSC_Crater[i].xyz;
//     if (dot(v*v, 1) < dot(closestVector*closestVector, 1))
//     {
//       closestVector = v;
//     }
//   }
//   float radius = 0.02f;
//   float3 direction = float3(0, 1, 0);
//   float stretch = dot(normalize(closestVector), direction);
//   float coef = radius * (1 + abs(stretch*stretch*stretch)) * rsqrt(dot(closestVector, closestVector)) - 1;
//   float icoef = (1 - saturate(coef));
//   half wetFactor = (1 - icoef * icoef * icoef * icoef) * diffuseMap.a;



  // Find the closest crater distance weighted by the crater size
  float distance2 = 100.0f;
  for (int i = 0; i < nCraters; i++)
  {
    float3 v = PSC_Crater[i].xyz - input.originalPosition.xyz;
    float d2 = dot(v*v, PSC_Crater[i].www);
    if (d2 < distance2)
    {
      distance2 = d2;
    }
  }

  // Calculate darkenning factor
  float craterCoef = 0.9f;
  half wetFactor = (1.0f - saturate(distance2 * distance2)) * diffuseMap.a * craterCoef;

  // Fill out the FOut structure
  FOut fout;

  // Calculate the lights (very simplified lighting for craters - input.originalPosition.w stores the diffuse coefficient)
  fout.light = GetLight(PSC_GlassEnvColor.rgb, PSC_GlassEnvColor.rgb * input.originalPosition.w, half3(0, 0, 0));

  // Calculate the color
  fout.color.rgb =  half3(1, 1, 1);
  fout.color.w = wetFactor * PSC_GlassEnvColor.a;

  // Calculate the shadow intesity
  fout.shadowIntensity = 0;

  // Calculate the land shadow
  fout.landShadow = 1;
  fout.fog = input.tPSShadowReduction_Fog.z;

  return fout;
}

#define CRATER(x) \
FOut PFCrater##x(SCrater input, bool sbShadows) \
{ \
  return PFCraterX(input, x, sbShadows); \
} \
CREATEPSSN(Crater##x,SCrater) \

CRATER(1)
CRATER(2)
CRATER(3)
CRATER(4)
CRATER(5)
CRATER(6)
CRATER(7)
CRATER(8)
CRATER(9)
CRATER(10)
CRATER(11)
CRATER(12)
CRATER(13)
CRATER(14)

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

half4 shadowLA : register(c6);
half4 shadowLD : register(c7);
half4 PSPostProcessStencilShadowsPri(VSPP_OUTPUT_SHADOWS input) : COLOR
{
  half4 c = tex2D(sampler0, input.TexCoord);
  half f = saturate((1-c.a) * (255.0f/16))* input.Color.a;
  return c*shadowLA/(shadowLD*f+shadowLA);
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

half Gauss(half2 sample)
{
  return 1.0f - sqrt(sample.x*sample.x + sample.y*sample.y)/* * 0.5*/;
}

// Blur radius for X and Y direction (values can be like this: float2(1/800 * 2, 1/600 * 2))
float3 blurRadius : register(c0);

// Inverse of the focal plane distance
float invFocalPlane : register(c1);

half4 PSPostProcessDOF(VSPP_OUTPUT input) : COLOR
{
//  if (input.TexCoord.y > 0.5)
//  {
//    return float4(tex2D(sampler0, input.TexCoord).rgb, 1);
//  }
//  else
  {
    //return float4(tex2D(sampler1, input.TexCoord).rgb, 1);
    
    // Poisson samples  
    const int samples = 4;
    const float2 poisson[4] = {float2(0.5, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5), float2(-0.5, 0.5)};

    // Common constants
    const half baseBlur = 0.0f;
    const half blurScale = blurRadius.z; // Coefficient to influence range of the blur effect
    const half smartBlurScale = 5.0f; // Coefficient to influence the blurrines with samples behind the center

    // Calculate the depth of the center
    float centerInvDepth = tex2D(sampler2, input.TexCoord).r;

    // Calculate the blurFactor
    half blurFactor = saturate(blurScale * abs(centerInvDepth - invFocalPlane.x) + baseBlur);

    // Fetch the color from Hi and Lo textures from the center
    half3 colorHi = tex2D(sampler0, input.TexCoord).rgb;
    half3 colorLo = tex2D(sampler1, input.TexCoord).rgb;

    // Mix Hi and Lo color
    half3 color = lerp(colorHi, colorLo, blurFactor);

    // Center is the only place where weight doesn't depend on blur factor
    half centerWeight = Gauss(float2(0, 0));

    // Create the center color
    half4 cOut = float4(color * centerWeight, centerWeight);

    // Go through the poisson samples and accumulate the color
    for (int i = 0; i < samples; i++)
    {
      // Texture coordinates for both Hi and Lo textures
      float2 coord = input.TexCoord + (poisson[i] * blurRadius.xy);

      // Calculate the depth of the sample
      float sampleInvDepth = tex2D(sampler2, coord).r;

      // Calculate the smart blur factor (if sample depth is closer than center depth, then the factor is 1,
      // else it slides linearly to 0 with some coefficient)
      // float smartBlurFactor = (sampleInvDepth >= centerInvDepth) ? 1.0f : 1.0f - (sampleDepth - centerDepth) * smartBlurScale;
      half smartBlurFactor = 1.0f - saturate(smartBlurScale * (centerInvDepth - sampleInvDepth));

      // Calculate the sample blur factor
      half blurFactor = saturate(blurScale * abs(sampleInvDepth - invFocalPlane) + baseBlur);

      // Fetch the color from Hi and Lo textures from the sample
      half3 colorHi = tex2D(sampler0, coord);
      half3 colorLo = tex2D(sampler1, coord);

      // Mix Hi and Lo color
      half3 color = lerp(colorHi, colorLo, blurFactor);

      // Weight of the sample depends on blurFactor and on weight of the sample according to it's distance
      // to center (with Gaussian values)
      half sampleWeight = blurFactor * Gauss(poisson[i]) * smartBlurFactor;

      // Add sample color with weight
      cOut.rgb += color * sampleWeight;
      cOut.a += sampleWeight;
    }

    // Normalize the color
    return half4(cOut.rgb / cOut.a, 1);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////

half4 PSPostProcessDistanceDOF(VSPP_OUTPUT input) : COLOR
{
  // Poisson samples  
  const int samples = 4;
  const float2 poisson[4] = {float2(0.5, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5), float2(-0.5, 0.5)};

  // Common constants
  const half baseBlur = 0.0f;
  const half blurScale = 1.0f; // Coefficient to influence range of the blur effect
  
  // Calculate the depth of the center
  float centerInvDepth = tex2D(sampler2, input.TexCoord).r;

  // Calculate the blurFactor
  //float blurFactor = saturate(blurScale * (invFocalPlane - centerInvDepth));
  half blurFactor = min(max(blurScale * (invFocalPlane - centerInvDepth), 0) + baseBlur, 1);

  // Fetch the color from Hi and Lo textures from the center
  half3 colorHi = tex2D(sampler0, input.TexCoord).rgb;
  half3 colorLo = tex2D(sampler1, input.TexCoord).rgb;

  // Mix Hi and Lo color
  half3 color = lerp(colorHi, colorLo, blurFactor);

  // Center is the only place where weight doesn't depend on blur factor
  half centerWeight = Gauss(float2(0, 0));

  // Create the center color
  half4 cOut = float4(color * centerWeight, centerWeight);

  // Go through the poisson samples and accumulate the color
  for (int i = 0; i < samples; i++)
  {
    // Texture coordinates for both Hi and Lo textures
    float2 coord = input.TexCoord + (poisson[i] * blurRadius);

    // Calculate the depth of the sample
    float sampleInvDepth = tex2D(sampler2, coord).r;

    // Calculate the sample blur factor
    //float blurFactor = saturate(blurScale * (invFocalPlane - sampleInvDepth));
    half blurFactor = min(max(blurScale * (invFocalPlane - sampleInvDepth), 0) + baseBlur, 1);

    // Fetch the color from Hi and Lo textures from the sample
    half3 colorHi = tex2D(sampler0, coord);
    half3 colorLo = tex2D(sampler1, coord);

    // Mix Hi and Lo color
    half3 color = lerp(colorHi, colorLo, blurFactor);

    // Weight of the sample depends on blurFactor and on weight of the sample according to it's distance
    // to center (with Gaussian values)
    half sampleWeight = blurFactor * Gauss(poisson[i]);

    // Add sample color with weight
    cOut.rgb += color * sampleWeight;
    cOut.a += sampleWeight;
  }

  // Normalize the color
  return half4(cOut.rgb / cOut.a, 1);
}

/////////////////////////////////////////////////////////////////////////////////////////////

/// calculate and downsample luminace
/**
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
  // note: this is daytime sensitivity, night time would be different (blue shift)
  half3 eyeSensitivityD4 = half3(0.299f/4, 0.587f/4, 0.114f/4);
  half luminance = dot(x1+x2+x3+x4,eyeSensitivityD4);
  return half4(luminance.xxx,1);
  // debugging - preserve colors from original image
  //return float4((x1+x2+x3+x4)*0.25,luminance);
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

float4 queryDot:register(c7);

/// shader to query any copoment or its combination
float4 PSPostProcessQuery(VSPP4T_OUTPUT input) : COLOR
{
  float value = DecodeApertureChange(tex2D(sampler0, input.TexCoord0).x);
  clip(value-queryDot.w);
  return float4(value.xxx,1);
}

/// RGB eye sensitivity, alpha not used
float4 rgbEyeCoef : register(c6);

/// x: multiplier - absolute level adjustment for rod/cell B/W effect simulation
/// y: addition - offset to allow disabling rods completely
float4 nightControl : register(c8);
/// x: ammount of glow
float4 glowHDRFactor : register(c7);

half3 ToneMapping(half3 c, half intensity, half3 maxAvgMin)
{
  // apply tone-mapping
  // tone mapping should never extend dynamic range
  // we also do not want to extend it too much, we want to get some overbright
  half oMax = min(max(maxAvgMin.r,1),2);
  // never needed to compress more than necessary
  // we might precalculate some of the following expression in some texture
  return c / (1+c*(oMax-1)/oMax);
  //return c / (1+c);
  //return c;
  //return maxAvgMin;
  //return c/max(maxAvgMin.r,0.01);
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

/// aperture (aperture value is computed on GPU)
float ApertureControl(float texValue)
{
  // sample aperture modification texture
  // value is stored as 0.5x, so that it can be higher than 1
  float apertureMod = DecodeApertureChange(texValue);
  return glowHDRFactor.x*apertureMod;
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
  
  float aperture = ApertureControl(tex2D(sampler2,float2(0,0)).x);
  half3 cAfterAperture = cOriginal*aperture;
  
  /*
  half4 cGlow = 0;
  
  for (half x=-1; x<=+1; x++)
  for (half y=-1; y<=+1; y++)
  {
    float ux = input.TexCoord0.x + x*4/1024;
    float vy = input.TexCoord0.y + y*4/768;
    
    cGlow = max(cGlow,tex2D(sampler1, float2(ux,vy)));
  }
  half3 glowAfterAperture = cGlow*aperture;
  half glowLum = saturate(dot(glowAfterAperture, rgbEyeCoef.rgb));
  half3 color = cAfterAperture + glowAfterAperture*saturate((glowLum-0.9)*1.6);
  */
  
  half3 color = cAfterAperture;
  // read min/max/avg, transform it into the new color space
  half3 maxAvgMin = tex2D(sampler3,float2(0,0))*aperture;
  return half4(ToneMapping(color, dot(color, rgbEyeCoef.rgb),maxAvgMin),cOriginal.a);
}

half4 PSPostProcessGlowNight(VSPP4T_OUTPUT input) : COLOR
{
  half4 cOriginal = tex2D(sampler0, input.TexCoord0);
  half4 cGlow = tex2D(sampler1, input.TexCoord1);
  half3 rawColor = cOriginal;
  //half3 rawColor = (cOriginal+cGlow);
  float aperture = ApertureControl(tex2D(sampler2,float2(0,0)).x);
  half3 afterAperture = rawColor*aperture;
  half3 maxAvgMin = tex2D(sampler3,float2(0,0))*aperture;
  return half4(NightBlueShift(afterAperture,maxAvgMin),cOriginal.a);
}

/// B&W, transformed to a single color

half4 PSPostProcessNVG(VSPP4T_OUTPUT input) : COLOR
{
  half4 cOriginal = tex2D(sampler0, input.TexCoord0);
  half4 cGlow = tex2D(sampler1, input.TexCoord1);
  half3 rawColor = cOriginal;
  //half3 rawColor = (cOriginal+cGlow);
  float aperture = ApertureControl(tex2D(sampler2,float2(0,0)).x);
  half3 afterAperture = rawColor*aperture;
  half3 maxAvgMin = tex2D(sampler3,float2(0,0))*aperture;

  half intensity = dot(afterAperture, rgbEyeCoef.rgb);
  
  // simulate different response in different RGB components
  //half3 color = half3(intensity*0.3, intensity*1, intensity*0.1); // yellowish
  half3 color = half3(intensity*0.15, intensity*1, intensity*0.2); // cyanish

  return half4(color,cOriginal.a);
}

//////////////////////////////////////////////

//! Function to return brightness of the color
half Brightness(half3 color)
{
  return dot(color, half3(0.299, 0.587, 0.114));
}

//! Simple pixel shader designed for flare drawing
float4 PSNonTLFlare(SNormal input) : COLOR
{
  // Get the brightness coefficient upon measured visibility of the light source
  half brightness = Brightness(tex2D(samplerLightIntensity, half2(0.5, 0.5)).rgb);
  half bCoef = saturate((brightness - 0.5) * 10.0);
  //half bCoef = (brightness > 0.1) ? 1 : 0;
  
  // Get the aperture value
  float aperture = ApertureControl(tex2D(sampler14,float2(0,0)).x);

  // Calculate original color and alpha
  half4 cOriginal = tex2D(sampler0, input.tDiffuseMap.xy);
  cOriginal.a = cOriginal.a*input.ambientColor.a*bCoef;
  cOriginal.rgb = cOriginal.rgb*(input.ambientColor+input.diffuseColor);
  
  // Include aperture
  half3 cAfterAperture = cOriginal.rgb * aperture;
  
  // read min/max/avg, transform it into the new color space
  half3 maxAvgMin = tex2D(sampler13,float2(0,0)) * aperture;
  return half4(ToneMapping(cAfterAperture, dot(cAfterAperture, rgbEyeCoef.rgb),maxAvgMin),cOriginal.a);
}