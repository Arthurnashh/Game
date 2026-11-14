#ifndef _SHADERS_PS_DECL_H
#define _SHADERS_PS_DECL_H 1

//! Max. color taken from current texture
#define T0_MAXCOLOR   0
//! Diffuse color coeficient (combination of light and material)
#define PS_DIFFUSE    1
//! Back side diffuse color coeficient (combination of light and material)
#define PS_DIFFUSE_BACK    2
//! Specular color coeficient (combination of light and material)
#define PS_SPECULAR    3

//@{ environmental reflection colors
#define PS_GROUND_REFL_COLOR 4
#define PS_SKY_REFL_COLOR 5
//@}

//! Max. color taken from current texture
#define T0_AVGCOLOR   6

#define PS_WAVE_COLOR 7

// postprocess uses another family of shaders
#define POST_FX_PARAM 0
#define POST_SHADOWVIS_PARAM 0
#define POST_SHADOWVIS_RAMPMUL 1
#define POST_SHADOWVIS_RAMPADD 2

#endif