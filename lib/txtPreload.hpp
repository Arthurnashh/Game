#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TXT_PRELOAD_HPP
#define _TXT_PRELOAD_HPP

#ifndef DECL_ENUM_PRELOADED_TEXTURE
#define DECL_ENUM_PRELOADED_TEXTURE
DECL_ENUM(PreloadedTexture)
#endif
DEFINE_ENUM_BEG(PreloadedTexture)
	TextureWhite,
  TextureHalf,
	TextureBlack,
	TextureZero,
	TextureDefault,
	TextureLine,
  TextureLine3D,

	CursorStrategy,
	CursorStrategyMove,

	CursorAim, // we want to aim somewhere
	CursorWeapon, // current weapon aim
  CursorFreelook, //freelook cursor 
	CursorTarget, // selected target
  CursorKnowTarget, // know target
	CursorLocked, // target locked
  CursorGunnerLocked, //if commander has gun and has has locked target

	CursorOutArrow, // cursor out of screen

	Corner,
	DialogBackground, DialogTitle, DialogGroup,
	TrackTexture,TrackTextureFour,

	Compass000,Compass090,Compass180,Compass270,

  FlareEye,
	Flare0,FlareLast=Flare0+15,

	HeatHazeNoiseTexture,

	MaxPreloadedTexture
DEFINE_ENUM_END(PreloadedTexture)

#ifndef DECL_ENUM_PRELOADED_MATERIAL
#define DECL_ENUM_PRELOADED_MATERIAL
DECL_ENUM(PreloadedMaterial)
#endif
DEFINE_ENUM_BEG(PreloadedMaterial)
  ShadowMaterial,
  SpriteMaterial,
  SpriteMaterialAddBlend,
  NonTLMaterial,
  FlareMaterial,
  EmptyMaterial,
  MaxPreloadedMaterial
DEFINE_ENUM_END(PreloadedMaterial)

TexMaterial *GlobPreloadMaterial(PreloadedMaterial id);
//Texture *GlobPreloadTexture(PreloadedTexture id);

#endif
