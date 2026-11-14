#line 2 "W:\c\Poseidon\lib\d3dT\ShaderSources\VSWater.hlsl"

#include "VS.h"

void VSWater(
  in float4 vPosition             : POSITION,
  in int2   iTex0                 : TEXCOORD0,
  out float4 oProjectedPosition   : SV_POSITION,
  out float3 vOutHalfwayLocal     : COLOR0,
  out float4 oShadowReduction_Fog : COLOR1,
  out float4 oOutAmbient          : TEXCOORD_AMBIENT,
  out float4 oOutSpecular         : TEXCOORD_SPECULAR,
  out float4 oOutDiffuse          : TEXCOORD_DIFFUSE_SI,
  out float4 vOutTexCoord0        : TEXCOORD0,
  out float4 vOutTexCoord1_2      : TEXCOORD1,
  out float4 vOutViewLocal        : TEXCOORD2,
  out float4 vOutTexCoord3        : TEXCOORD3,
  out float4 vOutSTN2LWS1         : TEXCOORD5)
{
  // Read floats from integers
  float3 vTex0 = float3(iTex0.xy, 1);

  // Do the vertex transformations
  TransformOutput to;
  AccomLights al;
  {
    to.skinnedMatrix0 = float4(1, 0, 0, 0);
    to.skinnedMatrix1 = float4(0, 1, 0, 0);
    to.skinnedMatrix2 = float4(0, 0, 1, 0);
    to.alpha_X_X_instanceShadow = VSC_Alpha_X_X_LandShadowIntensity;

    #define H_PI 3.1415926536
    const float4 PI_0p5_PI2_0p25={
      H_PI,0.5,H_PI*2,0.25
    };
    const float4 SIN_TAYLOR_3579={
      -1.0/(3*2),1.0/(5*4*3*2),-1.0/(7*6*5*4*3*2),+1.0/(9*8*7*6*5*4*3*2)
    };
    const float4 COS_TAYLOR_2468={
      -1.0/2,+1.0/(4*3*2),-1.0/(6*5*4*3*2),+1.0/(8*7*6*5*4*3*2)
    };

    float2 gridPos = vPosition.xz * VSC_WaveGrid.xx + VSC_WaveGrid.ww;
    gridPos = max(0,min(gridPos,VSC_WaveGrid.z));
    //float2 gridPos = vTex0.xy*VSC_WaveGrid.xx + VSC_WaveGrid.ww;
    float2 gridFrac = frac(gridPos);

    // remove the fractional part from y - this needs to be done for y, as it is multiplied
    gridPos -= gridFrac;

    float4 gridPosFrac = float4(gridFrac.x,gridFrac.y,1-gridFrac.x,1-gridFrac.y);

    // (1-xFrac)*(1-zFrac), xFrac*(1-zFrac), (1-xFrac)*zFrac, xFrac*zFrac
    float4 gridF = gridPosFrac.zxzx*gridPosFrac.wwyy;
    
    // xy contains: xFrac, zFrac
    // zw contains: 1-xFrac, 1-zFrac
    
    float gridAddr = VSC_WaveGrid.y*gridPos.y + gridPos.x;
    
    float4 gridSample;
    gridSample = gridF.x*VSC_WaterDepth[gridAddr]; // y00,y10 (yXZ)
    gridSample += gridF.y*VSC_WaterDepth[gridAddr+1]; // y00,y10 (yXZ)
    gridSample += gridF.z*VSC_WaterDepth[gridAddr+5]; // y01,y11
    gridSample += gridF.w*VSC_WaterDepth[gridAddr+5+1]; // y01,y11
    
    // landscape height on given position (world coord)
    float gridHeight = gridSample.x;
    
    float3 vNormal = float3(0,1,0);

    float3 t0 = float3(gridSample.yz,1);
    float2 tex0;
    tex0.x = dot(t0,VSC_TexTransform[0]._m00_m10_m30);
    tex0.y = dot(t0,VSC_TexTransform[0]._m01_m11_m31);
    
    // calculate phase offset for sin. argument - P(y)
    // valWAVEGEN
    const float4 waveGen=
    {
      // wave phase offset  - due to looping, when multiplied by 8, must give integer
      2.0/8,3.0/8,
      // wave offset amplitude - arbitrary
      0.2,0.12
    };
    // first version - P(y) = sin(y)
    float2 waveTime = tex0.yy * waveGen.xy;

    // x = x0*2*pi-pi
    
    float2 x = frac(waveTime)*PI_0p5_PI2_0p25.zz - PI_0p5_PI2_0p25.xx;

    // calculate 2x sin at once, using Taylor series
    float4 x24 = x.xyxy*x.xyxy; // r1.xy = r0^2.xy, r1.zw = r0^2.xy
    x24.zw = x24.xyxy * x24.xyxy; // r1.xy = r0^2.xy, r1.zw = r0^4.xy

    float4 x35 = x.xyxy * x24.xyzw; // r2.xy = r0^3.xy, r2.zw = r0^5.xy
    float4 x79 = x35.xyzw * x24.zwzw; // r3.xy = r0^7.xy, r2.zw = r0^9.xy

    float2 sinX = (
      x +
      x35.xy*SIN_TAYLOR_3579.x + 
      x35.zw*SIN_TAYLOR_3579.y +
      x79.xy*SIN_TAYLOR_3579.z +
      x79.zw*SIN_TAYLOR_3579.w
    );
    tex0.x += sinX.x*waveGen.z+sinX.y*waveGen.w;
    
    // wave top is where sin(x)==1 -> x = pi/2 -> x0 = 3/4;
    // white line is on u = 0.5 -> we seek 1/4

    float2 xWaveSin = frac(tex0);

    float4 xArg = xWaveSin.xyxy*PI_0p5_PI2_0p25.z - PI_0p5_PI2_0p25.x;

    // xArg.xy from -pi to +pi (x)

  // calculate 2x sin at once, using Taylor series

    float4 xArg24 = xArg*xArg; // r1.xy = r0^2.xy, r1.zw = r0^2.xy
    xArg24.zw = xArg24.xyxy*xArg24.xyxy; // r1.xy = r0^2.xy, r1.zw = r0^4.xy

    float4 xArg35 = xArg.xyxy*xArg24.xyzw; // r2.xy = r0^3.xy, r2.zw = r0^5.xy
    float4 xArg79 = xArg35*xArg24.zwzw; // r3.xy = r0^7.xy, r2.zw = r0^9.xy

    float2 waveSin = (
      xArg + 
      xArg35.xy*SIN_TAYLOR_3579.x +
      xArg35.zw*SIN_TAYLOR_3579.y +
      xArg79.xy*SIN_TAYLOR_3579.z +
      xArg79.zw*SIN_TAYLOR_3579.w
    );


    float4 xArg68 = xArg24.xyzw * xArg24.zwzw;
    float2 waveCos = (
      (float2)1 +
      xArg24.xy*COS_TAYLOR_2468.x +
      xArg24.zw*COS_TAYLOR_2468.y +
      xArg68.xy*COS_TAYLOR_2468.z +
      xArg68.zw*COS_TAYLOR_2468.w
    );

    // sin(A*x+B)' = A*cos(A*x+B)

    float waveSum = (waveSin.x+waveSin.y)*0.5;

    float2 waveDer = waveCos*VSC_WaveHeight.zw;
    float2 waveOff = waveSin*VSC_WaveHeight.xy;

    float waveHeight = waveOff.x + waveOff.y;

    to.position = vPosition;
    to.skinnedPosition = vPosition;

    to.skinnedPosition.y += waveHeight;

    // Get the depth not including the wave.
    // water depth on given position (height relative to gridHeight)
    float noWaveDepth = gridHeight -VSC_WaterSeaLevel.x;
    
    float onShoreDetect = saturate(VSC_WaterPeakWhite.x*noWaveDepth+VSC_WaterPeakWhite.y);
    float shoreNearDetect = saturate(VSC_WaterPeakWhite.z*noWaveDepth+VSC_WaterPeakWhite.w);
    float topDetect = saturate(waveSum);
    float deepWater = saturate(VSC_WaterSeaLevel.y * noWaveDepth + VSC_WaterSeaLevel.z);
    
    vOutTexCoord0.x = tex0.x-PI_0p5_PI2_0p25.w;
    vOutTexCoord0.y = shoreNearDetect*topDetect*0.25 + onShoreDetect;
    vOutTexCoord0.z = shoreNearDetect;
    vOutTexCoord0.w = deepWater;

    // Calculate the vector from vertex position to camera (normalized)
    float3 camDirectionNN = VSC_CameraPosition.xyz - to.skinnedPosition.xyz;
    float3 camDirection = normalize(camDirectionNN);

    // Calculate normal after wave animation
    float3 skinnedN;
    skinnedN.xz = -waveDer.xyy;
    skinnedN.y = 1;
    skinnedN = normalize(skinnedN);
    
    // Modify water normal
    {
      // Calculate normal disappearing coefficient - see label NORMAL_DISAPPEAR
      //half normalDisappear = pow(shoreNearDetect, 10);
      half normalDisappear = shoreNearDetect;
      
      // Make the normal more straight close to shore (to avoid visible change)
      skinnedN = lerp(skinnedN, float3(0, 1, 0), normalDisappear);

      const float baseInclination = 0.0f;
      
      // Modify water normal - don't modify close to shore
      skinnedN = ModifyWaterNormal(skinnedN, camDirection, baseInclination + (1.0f - baseInclination) * (1.0f - normalDisappear) * 0.8f /*normal inclination coefficient*/);
      //skinnedN = ModifyWaterNormal(skinnedN, camDirection, 0);
    }

    // Get S and T vectors based on normal  
    float3 skinnedS = normalize(float3(1,0,0)-skinnedN*skinnedN.x);
    float3 skinnedT = skinnedS.yzx*skinnedN.zxy - skinnedN.yzx*skinnedS.zxy;  // crossproduct

    // Fill out position and normal output variables
    to.skinnedNormal = skinnedN;
    to.transformedPosition = float4(mul(to.skinnedPosition, VSC_ViewMatrix), 1);
    to.projectedPosition = mul(VSC_ProjMatrix, to.transformedPosition);
    
    float3 modelS = -skinnedS;
    float3 modelT = -skinnedT;
    float3 modelN = skinnedN;

    vOutTexCoord1_2.x = dot(vTex0.xyz, VSC_TexTransform[1]._m00_m10_m30);
    vOutTexCoord1_2.y = dot(vTex0.xyz, VSC_TexTransform[1]._m01_m11_m31);

    vOutTexCoord1_2.z = dot(vTex0.xyz, VSC_TexTransform[2]._m00_m10_m30) * 2.0f;
    vOutTexCoord1_2.w = dot(vTex0.xyz, VSC_TexTransform[2]._m01_m11_m31) * 2.0f;

    vOutTexCoord3.x = dot(vTex0.xyz, VSC_TexTransform[3]._m00_m10_m30) * 4.0f;
    vOutTexCoord3.y = dot(vTex0.xyz, VSC_TexTransform[3]._m01_m11_m31) * 4.0f;
    vOutTexCoord3.zw = float2(0,1);
      
    // VSC_CameraPosition is in model space
    // camera to vertex direction in model space
    
    // Calculate the eye halfway vector

    // view direction for Fresnel term calculation
    // convert to local space
    float3 viewLocal;
    viewLocal.x = dot(modelS, camDirectionNN);
    viewLocal.y = dot(modelT, camDirectionNN);
    viewLocal.z = dot(modelN, camDirectionNN);
    
    vOutViewLocal = float4(viewLocal,1);
    
    float3 halfway = camDirection - VSC_LDirectionTransformedDir;
    
    
    float3 halfwayLocal;
    halfwayLocal.x = dot(modelS, halfway);
    halfwayLocal.y = dot(modelT, halfway);
    halfwayLocal.z = dot(modelN, halfway);
    
    vOutHalfwayLocal.xyz = (normalize(halfwayLocal) + 1) * 0.5;
    // vOutHalfwayLocal.w not initialized here

    // calculate reflection halfway vector and transform it into the tangent space

    // world/model space up is 0,1,0
    float3 up={0,1,0};
    
    float3 reflectHalfway = camDirection + up;

    // Transform it into local vertex space
    float3 reflectHalfwayLocal;
    reflectHalfwayLocal.x = dot(modelS, reflectHalfway);
    reflectHalfwayLocal.y = dot(modelT, reflectHalfway);
    reflectHalfwayLocal.z = dot(modelN, reflectHalfway);

    // Calculate alpha factor based on viewing angle
    float viewA = GetWaterViewA(skinnedN, camDirection);
    
    float shallowAlpha = VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.y;
    float deepAlpha = VSC_WaterSeaLevel.w;
    float waterA = (1-shoreNearDetect) * deepAlpha + shoreNearDetect*shallowAlpha;
    
    // calculate diffuse color
    float4 diffuse = saturate(dot(to.skinnedNormal, -(float3)VSC_LDirectionTransformedDir));
    
    diffuse *= deepWater;
    
    
    al.ambient.xyz = VSC_AE.rgb + VSC_LDirectionD.rgb * diffuse;
    al.ambient.w = waterA*viewA + 1-viewA;
    //vAmbientOut.w = 1;
    
    // LWS
    float4 vOutSTN2LWS0;
    float4 vOutSTN2LWS2;
    {  
      // Transform position to LWS
      float4 lwsPosition = float4(mul(to.skinnedPosition, VSC_LWSMatrix), VSC_LDirectionD.w);
      vOutSTN2LWS0.w = lwsPosition.x;
      vOutSTN2LWS1.w = lwsPosition.y;
      vOutSTN2LWS2.w = lwsPosition.z;
      vOutViewLocal.w = lwsPosition.w;
      
      // We want to get STN2LWS matrix wich is 9 dots of the second and first matrix
      vOutSTN2LWS0.x = dot(modelS, VSC_LWSMatrix._m00_m10_m20);
      vOutSTN2LWS1.x = dot(modelS, VSC_LWSMatrix._m01_m11_m21);
      vOutSTN2LWS2.x = dot(modelS, VSC_LWSMatrix._m02_m12_m22);
      vOutSTN2LWS0.y = dot(modelT, VSC_LWSMatrix._m00_m10_m20);
      vOutSTN2LWS1.y = dot(modelT, VSC_LWSMatrix._m01_m11_m21);
      vOutSTN2LWS2.y = dot(modelT, VSC_LWSMatrix._m02_m12_m22);
      vOutSTN2LWS0.z = dot(modelN, VSC_LWSMatrix._m00_m10_m20);
      vOutSTN2LWS1.z = dot(modelN, VSC_LWSMatrix._m01_m11_m21);
      vOutSTN2LWS2.z = dot(modelN, VSC_LWSMatrix._m02_m12_m22);
    }

    al.diffuse = vOutSTN2LWS0;
    al.specular = vOutSTN2LWS2;
    al.shadowReduction = float4(0, 0, 0, 1);
  }
  oProjectedPosition = to.projectedPosition;





  // Apply fog
  float oFog;
  VFog(to, oFog);

  // Include P&S lights
  VLPointSpotN(to, al);

  // Write lights to output
  VDoneLights(al, float4(1, 1, 1, 1), oOutAmbient, oOutSpecular, oOutDiffuse);
  oShadowReduction_Fog = al.shadowReduction;
  oShadowReduction_Fog.z = oFog;
}

