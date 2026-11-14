#ifndef FPSHADERS_DECL_H
#define FPSHADERS_DECL_H

// dcl_position v0
// dcl_normal v3
// dcl_texture0 v7

// Input semantics
#define INPUT_POSITION      v0
#define INPUT_V_COLOR       v1 // typically only alpha is passed here
#define INPUT_NORMAL        v3
#define INPUT_TEXCOORD0     v7
#define INPUT_BLENDWEIGHT   v8
#define INPUT_BLENDINDICES  v9
#define INPUT_S             v10
#define INPUT_T             v11
#define INPUT_POSITIONA     v1 // shared
#define INPUT_BLENDWEIGHTA  v2
#define INPUT_BLENDINDICESA v3 // shared
#define INPUT_POSITIONB     v4
#define INPUT_BLENDWEIGHTB  v5
#define INPUT_BLENDINDICESB v6
#define INPUT_COLOR         v8 // could be merged with INPUT_V_COLOR?

#define INPUT_MAT_A         v12
#define INPUT_MAT_D         v13
#define INPUT_MAT_D_FORCED  v14
#define INPUT_MAT_S         v15

#define V_MAT_A             12
#define V_MAT_D             13
#define V_MAT_D_FORCED      14
#define V_MAT_S             15

// modulate color
#define MODULATE_COLOR r6
// skinned position
#define SKINNED_POSITION r7
// skinned normal
#define SKINNED_NORMAL r8
// transformed position
#define TRANSFORMED_POSITION r9
// diffuse accumulator
#define ACCUMULATOR_DIFFUSE r10
// specular accumulator
#define ACCUMULATOR_SPECULAR r11


// Bone beginning
#define BONES_START -96

// water animation bones share offsets with bones

// note: 10 more constants after WATER_DEPTH_LAST will be read ad depth data due to interpolation
// but their value will be ignored

enum
{
  WATER_DEPTH=BONES_START, // depth data
  WATER_DEPTH_LAST=-16, // WATER_DEPTH+81-1,
  WATER_DEPTHGRID_GRID_1_GRIDd2, // depth data grid
  WATER_PEAK_WHITE, // transfrom depth to peak visibility
  WATER_SEA_LEVEL, // average sea level
  WATER_PI_0p5_PI2_0p25,
  WATER_SIN_TAYLOR_3579,
  WATER_COS_TAYLOR_2468,
  WATER_WAVE_HEIGHT_X_Y_XDER_YDER,
  WATER_REFLECT_ADD_MUL_u_u,
  WATER_WAVEGEN, // "wavegen" mul and add coefs
};

// note: 10 more constants after LAND_SHADOW_LAST will be read ad depth data due to interpolation
// but their value will be ignored

enum
{
  LAND_SHADOW=BONES_START, // shadow data
  LAND_SHADOW_LAST=-16, // LAND_SHADOW+81-1,
  LAND_SHADOWGRID_GRID__1__GRIDd2, // depth data grid
};
// Constants offset
// Note that all the bones can lie between BONES_START and (CONSTANTS_OFFSET-1)
#define CONSTANTS_OFFSET 35

// Basic constants
#define CONSTANTS_0_1_765_0d5     (CONSTANTS_OFFSET + 0)

// Screenspace conversion constants
#define SCREENSPACE_SCALE         (CONSTANTS_OFFSET + 1)
#define SCREENSPACE_OFFSET        (CONSTANTS_OFFSET + 2)

// Tree crown shader
#define TREE_CROWN_CENTER         (CONSTANTS_OFFSET + 3)
#define TREE_CROWN_SIZE           (CONSTANTS_OFFSET + 4)

// World-View matrix
#define WORLD_VIEW_MATRIX_0       (CONSTANTS_OFFSET + 5)
#define WORLD_VIEW_MATRIX_1       (CONSTANTS_OFFSET + 6)
#define WORLD_VIEW_MATRIX_2       (CONSTANTS_OFFSET + 7)

// Projection matrix
#define PROJ_MATRIX_0             (CONSTANTS_OFFSET + 8)
#define PROJ_MATRIX_1             (CONSTANTS_OFFSET + 9)
#define PROJ_MATRIX_2             (CONSTANTS_OFFSET + 10)
#define PROJ_MATRIX_3             (CONSTANTS_OFFSET + 11)

// Position of the camera in world coordinates (object space)
#define CAMERA_POSITION           (CONSTANTS_OFFSET + 12)

// Fog coeficients
#define FOGEND__RCP_FOGEND_MINUS_FOGSTART__SHADOWVOLUME_OFFSET__SHADOWVOLUME_SIZE (CONSTANTS_OFFSET + 13)

// Texture transformation matrix 0
#define TEX_TRANSFORM_MATRIX0_0 (CONSTANTS_OFFSET + 14)
#define TEX_TRANSFORM_MATRIX0_1 (CONSTANTS_OFFSET + 15)

// Texture transformation matrix 1
#define TEX_TRANSFORM_MATRIX1_0 (CONSTANTS_OFFSET + 16)
#define TEX_TRANSFORM_MATRIX1_1 (CONSTANTS_OFFSET + 17)

// Texture transformation matrix 2
#define TEX_TRANSFORM_MATRIX2_0 (CONSTANTS_OFFSET + 18)
#define TEX_TRANSFORM_MATRIX2_1 (CONSTANTS_OFFSET + 19)

// ground reflection color used for directional light
#define LDIRECTION_GROUND (CONSTANTS_OFFSET + 20)

// Material
#define MAT_E             (CONSTANTS_OFFSET + 21)
//#define MAT_A
//#define MAT_D
//#define MAT_D_FORCED
//#define MAT_S

// AccomodateEye
#define ACCOMODATE_EYE              (CONSTANTS_OFFSET + 22)

// ML_Sun
#define LDIRECTION_A                (CONSTANTS_OFFSET + 23)
#define LDIRECTION_D                (CONSTANTS_OFFSET + 24)
#define LDIRECTION_TRANSFORMED_DIR  (CONSTANTS_OFFSET + 25)

// ML_Sky
#define LDIRECTIONSKY_SUNSKY_COLOR      (CONSTANTS_OFFSET + 26)
#define LDIRECTIONSKY_SKY_COLOR         (CONSTANTS_OFFSET + 27)

// ML_Stars
#define LDIRECTIONSTARS_TRANSFORMED_SUN_DIR__ALPHA_CORRECTION (CONSTANTS_OFFSET + 28)

// ML_SunObject
#define LDIRECTION_SUNOBJECT_COLOR (CONSTANTS_OFFSET + 29)

// ML_SunHaloObject
#define LDIRECTION_SUNHALOOBJECT_COLOR (CONSTANTS_OFFSET + 30)

// ML_MoonObject
#define LDIRECTION_MOONOBJECT_COLOR (CONSTANTS_OFFSET + 31)

// ML_MoonHaloObject
#define LDIRECTION_MOONHALOOBJECT_COLOR (CONSTANTS_OFFSET + 32)

// Offset to the matrices: (0.5 - SetMatricesOffset)
#define MATRICES_OFFSET_765 (CONSTANTS_OFFSET + 33)

// Position multiplier
#define POS_MUL (CONSTANTS_OFFSET + 34)

// Position offset
#define POS_ADD (CONSTANTS_OFFSET + 35)

#define CONSTANTS_SVOffset_lightsat_1020_0d5 (CONSTANTS_OFFSET + 36)

//@{ volumetric shadow shader constants
#define SHADOW_MUL__ADD__INTENSITY (CONSTANTS_OFFSET + 37)

#define SHADOWDIRECTION_TRANSFORMED_DIR  (CONSTANTS_OFFSET + 38)

#define SHADOW_OFFSET__SHADOW_SIZE  (CONSTANTS_OFFSET + 39)
//@}

#define SHADOW_MODULATE_DIFFUSE  (CONSTANTS_OFFSET + 40)

// ------------------
// Relative constants

// Base of the point and spot lights
#define POINTSPOT_BASE (CONSTANTS_OFFSET + 41)

// Common size of the point and spot lights
#define POINTSPOT_COMMON_SIZE 5

// max. number of lights - see EngineDDXB::maxPSLights

// Point light constants
#define LPOINT_TRANSFORMED_POS  0
#define LPOINT_A                1
#define LPOINT_D                2
#define LPOINT_ATTEN            3

// Spot light constants
#define LSPOT_TRANSFORMED_POS                                   0
#define LSPOT_TRANSFORMED_DIR                                   1
#define LSPOT_A                                                 2
#define LSPOT_D                                                 3
#define LSPOT__ATTEN__COS_PHI_HALF__COS_THETA_HALF__INV_CTHMCPH 4

// Point light
#define LPOINT(X) \
  add r1.xyz, -SKINNED_POSITION, c[POINTSPOT_BASE + X + LPOINT_TRANSFORMED_POS] \
  dp3 r1.w, r1, r1 \
  rsq r1.w, r1.w \
  mul r1.xyz, r1, r1.w \
  mul r1.w, r1.w, r1.w \
  dp3 r3.w, SKINNED_NORMAL, r1 \
  max r3.w, r3.w, c[CONSTANTS_0_1_765_0d5].x \
  mul r0, INPUT_MAT_D, c[POINTSPOT_BASE + X + LPOINT_D] \
  mul r1.xyz, r3.w, r0 \
  mad r1.xyz, INPUT_MAT_A, c[POINTSPOT_BASE + X + LPOINT_A], r1 \
  mad r1.xyz, INPUT_MAT_D_FORCED, c[POINTSPOT_BASE + X + LPOINT_D], r1 \
  mul r1.w, r1.w, c[POINTSPOT_BASE + X + LPOINT_ATTEN].x \
  min r1.w, r1.w, c[CONSTANTS_SVOffset_lightsat_1020_0d5].y \
  mad ACCUMULATOR_DIFFUSE.xyz, r1, r1.w, ACCUMULATOR_DIFFUSE

#define LSPOT(X) \
  add r3.xyz, -SKINNED_POSITION, c[POINTSPOT_BASE + X + LSPOT_TRANSFORMED_POS] \
  dp3 r1.w, r3, r3 \
  rsq r1.w, r1.w \
  mul r5.xyz, r3, r1.w \
  mul r3.w, r1.w, r1.w \
  dp3 r1.w, -c[POINTSPOT_BASE + X + LSPOT_TRANSFORMED_DIR], r5 \
  dp3 r4.w, SKINNED_NORMAL, r5 \
  add r5.w, -c[POINTSPOT_BASE + X + LSPOT__ATTEN__COS_PHI_HALF__COS_THETA_HALF__INV_CTHMCPH].y, r1.w \
  mul r5.w, r5.w, c[POINTSPOT_BASE + X + LSPOT__ATTEN__COS_PHI_HALF__COS_THETA_HALF__INV_CTHMCPH].w \
  sge r2.w, c[POINTSPOT_BASE + X + LSPOT__ATTEN__COS_PHI_HALF__COS_THETA_HALF__INV_CTHMCPH].y, r1.w \
  slt r1.w, c[POINTSPOT_BASE + X + LSPOT__ATTEN__COS_PHI_HALF__COS_THETA_HALF__INV_CTHMCPH].z, r1.w \
  mad r2.w, -r5.w, r2.w, r5.w \
  add r5.w, -r2.w, c[CONSTANTS_0_1_765_0d5].y \
  mad r1.w, r1.w, r5.w, r2.w \
  mul r3.w, r3.w, c[POINTSPOT_BASE + X + LSPOT__ATTEN__COS_PHI_HALF__COS_THETA_HALF__INV_CTHMCPH].x \
  max r4.w, r4.w, c[CONSTANTS_0_1_765_0d5].x \
  mul r0, INPUT_MAT_D, c[POINTSPOT_BASE + X + LSPOT_D] \
  mul r3.xyz, r4.w, r0 \
  mad r3.xyz, INPUT_MAT_A, c[POINTSPOT_BASE + X + LSPOT_A], r3 \
  mad r3.xyz, INPUT_MAT_D_FORCED, c[POINTSPOT_BASE + X + LSPOT_D], r3 \
  mul r1.w, r3.w, r1.w \
  min r1.w, r1.w, c[CONSTANTS_SVOffset_lightsat_1020_0d5].y \
  mad ACCUMULATOR_DIFFUSE.xyz, r1.w, r3.xyz, ACCUMULATOR_DIFFUSE

#endif
