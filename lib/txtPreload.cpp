// Scene does all transformation, clipping and ligthing
#include "wpch.hpp"
#include "textbank.hpp"
#include "Shape/material.hpp"
#include "scene.hpp"
#include "txtPreload.hpp"
#include "engine.hpp"
#include "fileLocator.hpp"
#include "paramFileExt.hpp"

PreloadedTextures::PreloadedTextures()
{
}

PreloadedTextures::~PreloadedTextures()
{
	_data.Clear();
}

void PreloadedTextures::Preload( bool all )
{
	if (_data.Size()>=MaxPreloadedTexture) return;
	_data.Clear();
	_data.Resize(MaxPreloadedTexture);

  ParamEntryVal cfgCore = Pars >> "CfgCoreData";

	//#define LOAD bank->Load
	#define LOAD(XX) GlobLoadTexture(GetPictureName(XX))
	// actual texture preloading
	//AbstractTextBank *bank=GEngine->TextBank();
	_data[TextureDefault]=LOAD(GetPictureName(cfgCore >> "textureDefault"));
	DefaultTexture=_data[TextureDefault].GetRef();
	_data[TextureWhite]=NULL;
	_data[TextureHalf]=LOAD(GetPictureName(cfgCore >> "textureHalf"));
	_data[TextureBlack]=LOAD(GetPictureName(cfgCore >> "textureBlack"));
	_data[TextureZero]=LOAD(GetPictureName(cfgCore >> "textureZero"));
	_data[TextureLine] = LOAD(GetPictureName(cfgCore >> "textureLine")); // 8x8
  _data[TextureLine3D] = LOAD(GetPictureName(cfgCore >> "textureLine3D")); 
	// we want 4x4 mipmap level only
	if (_data[TextureLine])
	{
		_data[TextureLine]->SetMipmapRange(1,1);
    GEngine->TextBank()->MakePermanent(_data[TextureLine]);
	}

	if( !all ) return;

	_data[TrackTexture]=LOAD(GetPictureName(cfgCore >> "textureTrack"));
	_data[TrackTextureFour]=LOAD(GetPictureName(cfgCore >> "textureTrackFour"));

	_data[Corner]=GlobLoadTextureUI( GetPictureName(Pars>>"CfgInGameUI">>"imageCornerElement") );

	ParamEntryVal wrapper=Pars>>"CfgWrapperUI";
	_data[DialogBackground] = GlobLoadTextureUI(GetPictureName(wrapper>>"Background">>"texture"));
	_data[DialogTitle] = GlobLoadTextureUI(GetPictureName(wrapper>>"TitleBar">>"texture"));
	_data[DialogGroup] = GlobLoadTextureUI(GetPictureName(wrapper>>"GroupBox2">>"texture"));
	
	ParamEntryVal cursor=Pars>>"CfgInGameUI">>"Cursor";
	_data[CursorLocked]=GlobLoadTextureUI( GetPictureName(cursor>>"lock_target"));
	_data[CursorTarget]=GlobLoadTextureUI( GetPictureName(cursor>>"select_target"));
  _data[CursorKnowTarget]=GlobLoadTextureUI( GetPictureName(cursor>>"known_target"));
  _data[CursorGunnerLocked]=GlobLoadTextureUI( GetPictureName(cursor>>"gunner_lock"));
	_data[CursorAim]=GlobLoadTextureUI( GetPictureName(cursor>>"aim"));
	_data[CursorWeapon]=GlobLoadTextureUI( GetPictureName(cursor>>"weapon"));
  _data[CursorFreelook]=LOAD( GetPictureName(cursor>>"freelook"));

	_data[CursorStrategy]=GlobLoadTextureUI( GetPictureName(cursor>>"tactical") );
	_data[CursorStrategyMove]=GlobLoadTextureUI( GetPictureName(cursor>>"move") );

	_data[CursorOutArrow]=GlobLoadTextureUI( GetPictureName(cursor>>"outArrow") );

  ParamEntryVal compass = Pars >> "CfgInGameUI" >> "Compass";
	_data[Compass000]=GlobLoadTextureUI(GetPictureName(compass >> "texture0"));
	_data[Compass090]=GlobLoadTextureUI(GetPictureName(compass >> "texture90"));
	_data[Compass180]=GlobLoadTextureUI(GetPictureName(compass >> "texture180"));
	_data[Compass270]=GlobLoadTextureUI(GetPictureName(compass >> "texture270"));
	#define MAX_S(id) {if (_data[id]) _data[id]->SetUsageType(Texture::TexUI);}
	MAX_S(Compass000);
	MAX_S(Compass090);
	MAX_S(Compass180);
	MAX_S(Compass270);

	// load AI signs
	ParamEntryVal entry=Pars>>"CfgWorlds";

  RString mask = cfgCore >> "maskTextureFlare";
  mask.Lower();
  for (int s=0; s<16; s++)
	{
		RString buf = Format(mask, s);
		_data[Flare0 + s] = LOAD(buf);
	}

  RString flareEye = cfgCore >> "eyeFlare";
  flareEye.Lower();
	_data[FlareEye] = LOAD(flareEye);

	RString heatHazeNoise = cfgCore >> "heatHazeNoise";
	heatHazeNoise.Lower();
	_data[HeatHazeNoiseTexture] = LOAD(heatHazeNoise);
}

void PreloadedTextures::Clear()
{
	_data.Clear();
}

Texture *PreloadedTextures::New( RStringB name )
{
	// make texture permanent
	// assign new id
	// search data for the same texture
	for (int i=0; i<_data.Size(); i++)
	{
		Texture *dataI = _data[i];
		if ( dataI && dataI->GetName()==name)
		{
			return dataI;
		}
	}
	Ref<Texture> txt = GlobLoadTexture(name);
	_data.Add(txt);
	return txt;
}

Texture *PreloadedTextures::New( PreloadedTexture id )
{
	// predefined texture ids
	if (id>=_data.Size())
	{
		//LogF("Preloaded textures not initialized");
		return NULL;
	}
	return _data[id];
}

PreloadedTextures GPreloadedTextures;

Texture *GlobPreloadTexture( RStringB name )
{
	return GPreloadedTextures.New(name);
}


/// materials used directly by the engine (mostly synthetic)
class PreloadedMaterials
{
	RefArray<TexMaterial> _data;

	public:
  PreloadedMaterials();
  ~PreloadedMaterials();

	void Preload( bool all );
	void Clear();
	
	TexMaterial *New( PreloadedMaterial id ); // predefined texture ids
};

PreloadedMaterials GPreloadedMaterials;

PreloadedMaterials::PreloadedMaterials()
{
}
PreloadedMaterials::~PreloadedMaterials()
{
  _data.Clear();
}

void PreloadedMaterials::Preload( bool all )
{
  _data.Realloc(MaxPreloadedMaterial);
  _data.Resize(MaxPreloadedMaterial);

  // Shadow volume material
  _data[ShadowMaterial] = new TexMaterial;
  _data[ShadowMaterial]->SetVertexShaderID(VSShadowVolume);
  _data[ShadowMaterial]->SetPixelShaderID(PSWhite);
  _data[ShadowMaterial]->SetFogMode(FM_None);

  // Sprite material
  _data[SpriteMaterial] = new TexMaterial;
  _data[SpriteMaterial]->SetVertexShaderID(VSSprite);
  _data[SpriteMaterial]->SetPixelShaderID(PSSprite);
  _data[SpriteMaterial]->SetFogMode(FM_Alpha);
  _data[SpriteMaterial]->_diffuse = Color(0.15,0.15,0.15,1);
  _data[SpriteMaterial]->_forcedDiffuse = Color(0.45,0.45,0.45,1);

  // Sprite material with RFAddBlend
  _data[SpriteMaterialAddBlend] = new TexMaterial;
  _data[SpriteMaterialAddBlend]->SetVertexShaderID(VSSprite);
  _data[SpriteMaterialAddBlend]->SetPixelShaderID(PSSprite);
  _data[SpriteMaterialAddBlend]->SetFogMode(FM_Alpha);
  _data[SpriteMaterialAddBlend]->_diffuse = Color(0.15,0.15,0.15,1);
  _data[SpriteMaterialAddBlend]->_forcedDiffuse = Color(0.45,0.45,0.45,1);
  _data[SpriteMaterialAddBlend]->_emmisive = Color(0.5,0.5,0.5,0);
  _data[SpriteMaterialAddBlend]->SetRenderFlag(RFAddBlend, true);

  // NonTL material
  _data[NonTLMaterial] = new TexMaterial;
  _data[NonTLMaterial]->SetVertexShaderID(VSBasic);
  _data[NonTLMaterial]->SetPixelShaderID(PSNonTL);
  _data[NonTLMaterial]->SetMainLight(ML_None);
  _data[NonTLMaterial]->SetFogMode(FM_None);

  // Flare material
  _data[FlareMaterial] = new TexMaterial;
  _data[FlareMaterial]->SetVertexShaderID(VSBasic);
  _data[FlareMaterial]->SetPixelShaderID(PSNonTLFlare);
  _data[FlareMaterial]->SetFogMode(FM_None);
  _data[FlareMaterial]->SetRenderFlag(RFAddBlend, true);

  // Empty material
  _data[EmptyMaterial] = new TexMaterial;
  _data[EmptyMaterial]->SetVertexShaderID(VSBasic);
  _data[EmptyMaterial]->SetPixelShaderID(PSWhite);
  _data[EmptyMaterial]->SetMainLight(ML_None);
  _data[EmptyMaterial]->SetFogMode(FM_None);
}
void PreloadedMaterials::Clear()
{
  _data.Clear();
}

TexMaterial *PreloadedMaterials::New( PreloadedMaterial id )
{
	if (id>=_data.Size())
	{
		//LogF("Preloaded textures not initialized");
		return NULL;
	}
	return _data[id];
}

TexMaterial *GlobPreloadMaterial(PreloadedMaterial id)
{
  return GPreloadedMaterials.New(id);
}

void GlobPreloadedTexturesPreload(bool all)
{
  GPreloadedMaterials.Preload(all);
  GPreloadedTextures.Preload(all);
}

void GlobPreloadedTexturesClear()
{
  GPreloadedMaterials.Clear();
  GPreloadedTextures.Clear();
	GTexMaterialBank.ClearCache();
}


