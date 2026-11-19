#include "wpch.hpp"
#include "wounds.hpp"
#include "animation.hpp"
#include "paramFileExt.hpp"
#include "fileLocator.hpp"

void WoundInfo::Load(LODShape *shape, ParamEntryVal cfg)
{
  ParamEntryPtr texCfg = &cfg;
  ParamEntryPtr matCfg;
  // if wound info is a class, it should contain tex and mat
  if (cfg.IsClass())
  {
    texCfg = &(cfg>>"tex");
    matCfg = &(cfg>>"mat");
  }
  
	// needs shape
	// should be called after InitShape
	// load wound info from config
	// cfg should be array of pairs
	
	{
	  ParamEntryPar tex = *texCfg;
	  if ((tex.GetSize()%3)!=0)
	  {
		  RptF("Array tex in %s not multiply of 3",(const char *)cfg.GetContext());
	  }
	  _tex.Realloc(tex.GetSize()/2);
	  for (int i=0; i<tex.GetSize()-1; i+=2)
	  {
		  RString oldName = GetDefaultName(tex[i+0], ".pac");
		  RString newName1 = GetDefaultName(tex[i+1], ".pac");
		  RString newName2 = GetDefaultName(tex[i+2], ".pac");
		  // search shape for texture
		  oldName.Lower();
		  // we need to load all textures, because the shape is not loaded
		  // this means we will often have selections for a textures
		  // which are not included in the shape at all
		  Ref<Texture> oldTex = GlobLoadTexture(oldName);
		  if (oldTex)
		  {
			  newName1.Lower();
			  newName2.Lower();
			  Ref<Texture> newTex = GlobLoadTexture(newName1);
			  Ref<Texture> newTex2 = GlobLoadTexture(newName2);
			  //
			  WoundTexPair wp;
			  wp._healthy = oldTex;
			  wp._wounded[0] = newTex;
			  wp._wounded[1] = newTex2;
			  _tex.Add(wp);
		  }
	  }
	  _tex.Compact();
	}
	if (matCfg)
	{
	  ParamEntryPar mat = *matCfg;
	  if ((mat.GetSize()%3)!=0)
	  {
		  RptF("Array mat in %s not multiply of 3",(const char *)cfg.GetContext());
	  }
	  _mat.Realloc(mat.GetSize()/3);
	  for (int i=0; i<mat.GetSize()-2; i+=3)
	  {
		  RString oldName = mat[i+0];
		  RString newName1 = mat[i+1];
		  RString newName2 = mat[i+2];
		  // search shape for texture
		  oldName.Lower();
		  // we need to load all textures, because the shape is not loaded
		  // this means we will often have selections for a textures
		  // which are not included in the shape at all
		  Ref<TexMaterial> oldTex = GTexMaterialBank.New(TexMaterialName(oldName));
		  if (oldTex)
		  {
			  newName1.Lower();
			  newName2.Lower();
			  WoundMatPair wp;
			  wp._healthy = oldTex;
			  wp._wounded[0] = GTexMaterialBank.New(TexMaterialName(newName1));
			  wp._wounded[1] = GTexMaterialBank.New(TexMaterialName(newName2));
			  _mat.Add(wp);
		  }
	  }
	  _mat.Compact();
	}
	else
	{
	  _mat.Clear();
	}
}

void WoundInfo::Unload()
{
	_tex.Clear();
	_mat.Clear();
}

template <>
void WoundTextureSelection<Texture>::SetTexture(AnimationContext &animContext, const Shape *shape, Texture *tex, float dist2) const
{
  if (dist2 >= 0 && tex)
  {
    saturateMax(dist2, Square(0.1f));
    // scan all sections, gather most detailed aot
    // TODO: this could be done in WoundTextureSelection init 
    AreaOverTex aot;
	  for (int f=0; f<_sections.NSections(); f++)
	  {
		  PolyProperties &sec = animContext.SetSection(shape, _sections.GetSection(f));
      // make sure the material is loaded before applying it
      aot.Min(sec.GetAreaOverTex());
    }
    // preload is enough, no need to prepare - prepare will be done in rendering
    if (!tex->PreloadAndCheckMipmapLevel(dist2, aot.Get(0),false))
    {
      return;
    }
  }
	for (int f=0; f<_sections.NSections(); f++)
	{
		PolyProperties &sec = animContext.SetSection(shape, _sections.GetSection(f));
		sec.SetTexture(tex);
	}
}

template <>
void WoundTextureSelection<TexMaterial>::SetTexture(AnimationContext &animContext, const Shape *shape, TexMaterial *tex, float dist2) const
{
  if (dist2 >= 0 && tex)
  {
    saturateMax(dist2, Square(0.1f));
    // scan all sections, gather most detailed aot
    // TODO: this could be done in WoundTextureSelection init 
    AreaOverTex aot;
	  for (int f=0; f<_sections.NSections(); f++)
	  {
		  PolyProperties &sec = animContext.SetSection(shape, _sections.GetSection(f));
      // make sure the material is loaded before applying it
      aot.Min(sec.GetAreaOverTex());
    }
    if (!tex->PreloadData(aot, dist2))
    {
      return;
    }
  }
	for (int f=0; f<_sections.NSections(); f++)
	{
		PolyProperties &sec = animContext.SetSection(shape, _sections.GetSection(f));
		sec.SetMaterialExt(tex);
	}
}

WoundTextureSelections::WoundTextureSelections()
{
}

void WoundTextureSelections::DeinitLevel(LODShape *shape,int level)
{
  #if CHECK_INIT_SHAPE_WOUND
	  DoAssert(_initCount[level]>0);
	  _initCount[level]--;
  #endif
  _selectionTex[level].Clear();
  _selectionMat[level].Clear();
}

struct WoundTextureSelectionsBasicDef: public WoundTextureSelectionsDef
{
  Ref<WoundInfo> info;

  void InitLevel(WoundTextureSelections &anim, LODShape *shape, int level)
  {
    anim.InitLevelBasic(shape,level,*this);
  }
};
struct WoundTextureSelectionsNameDef: public WoundTextureSelectionsBasicDef
{
  RStringB name,altName;

  void InitLevel(WoundTextureSelections &anim, LODShape *shape, int level)
  {
    anim.InitLevelName(shape,level,*this);
  }
};

void WoundTextureSelections::InitLevelBasic(
  LODShape *shape, int level, const WoundTextureSelectionsBasicDef &def
)
{
  if (!def.info) return;
	#if CHECK_INIT_SHAPE_WOUND
	  DoAssert(_initCount[level]>=0);
	  _initCount[level]++;
	#endif
	ShapeUsed sShape = shape->Level(level);
	// scan whole shape for old textures from info
	AutoArray<int> sections;
	{// for each texture found create a WoundTextureSelection item
	  for (int t=0; t<def.info->_tex.Size(); t++)
	  {
		  const WoundTexPair &wp = def.info->_tex.Get(t);
		  // scan faces for wp._healthy
		  // sections-based wounds 
		  sections.Resize(0);
		  for (int s=0; s<sShape->NSections(); s++)
		  {
			  const ShapeSection &ss = sShape->GetSection(s);
			  if (ss.GetTexture()==wp._healthy)
			  {
				  // some texture found
				  // create corresponding WoundTextureSelection
				  sections.Add(s);
			  }
		  }
		  if (sections.Size()>0)
		  {
			  WoundTextureSelectionTex &ws = _selectionTex[level].Append();
			  ws.Init(sections,wp._wounded[0],wp._wounded[1],wp._healthy);
		  }
	  } // for (pairs)
	  _selectionTex[level].Compact();
	}
	{// for each material found create a WoundTextureSelection item
	  for (int t=0; t<def.info->_mat.Size(); t++)
	  {
		  const WoundMatPair &wp = def.info->_mat.Get(t);
		  // scan faces for wp._healthy
		  // sections-based wounds 
		  sections.Resize(0);
		  for (int s=0; s<sShape->NSections(); s++)
		  {
			  const ShapeSection &ss = sShape->GetSection(s);
			  if (ss.GetMaterialExt()==wp._healthy)
			  {
				  // some texture found
				  // create corresponding WoundTextureSelection
				  sections.Add(s);
			  }
		  }
		  if (sections.Size()>0)
		  {
			  WoundTextureSelectionMat &ws = _selectionMat[level].Append();
			  ws.Init(sections,wp._wounded[0],wp._wounded[1],wp._healthy);
		  }
	  } // for (pairs)
	  _selectionMat[level].Compact();
	}
}


void WoundTextureSelections::InitLevelName(
  LODShape *shape, int level, const WoundTextureSelectionsNameDef &def
)
{
  if (!def.info) return;
  #if CHECK_INIT_SHAPE_WOUND
	  Assert(_initCount[level]>=0);
	  _initCount[level]++;
  #endif
	AutoArray<int> sections;
  ShapeUsed sShape = shape->Level(level);
  int selI = shape->FindNamedSel(sShape,def.name,def.altName);
  if (selI<0) return;
  const NamedSelection &sel = sShape->NamedSel(selI);
  #if _ENABLE_REPORT
  sel.CheckSectionsNeeded(shape,def.name,def.altName);
  #endif
  {
    // scan this particular selection for old textures from info
    // for each texture found create a WoundTextureSelection item
    for (int t=0; t<def.info->_tex.Size(); t++)
    {
      const WoundTexPair &wp = def.info->_tex.Get(t);
      sections.Resize(0);
      for (int si=0; si<sel.NSections(); si++)
      {
        int s = sel.GetSection(si);
        const ShapeSection &ss = sShape->GetSection(s);
        // scan faces for wp._healthy
        if (ss.GetTexture()==wp._healthy)
        {
          // some texture found
          // create corresponding WoundTextureSelection
          sections.Add(s);
        }
      } // for (pairs)
      if (sections.Size()>0)
      {
        WoundTextureSelectionTex &ws = _selectionTex[level].Append();
        ws.Init(sections,wp._wounded[0],wp._wounded[1],wp._healthy);
      }
    }
    _selectionTex[level].Compact();
  }
  {
    // scan this particular selection for old textures from info
    // for each texture found create a WoundTextureSelection item
    for (int t=0; t<def.info->_mat.Size(); t++)
    {
      const WoundMatPair &wp = def.info->_mat.Get(t);
      sections.Resize(0);
      for (int si=0; si<sel.NSections(); si++)
      {
        int s = sel.GetSection(si);
        const ShapeSection &ss = sShape->GetSection(s);
        // scan faces for wp._healthy
        if (ss.GetMaterialExt()==wp._healthy)
        {
          // some texture found
          // create corresponding WoundTextureSelection
          sections.Add(s);
        }
      } // for (pairs)
      if (sections.Size()>0)
      {
        WoundTextureSelectionMat &ws = _selectionMat[level].Append();
        ws.Init(sections,wp._wounded[0],wp._wounded[1],wp._healthy);
      }
    }
    _selectionMat[level].Compact();
  }
}

void WoundTextureSelections::Init( LODShape *shape, Ref<WoundInfo> info )
{
  _selectionTex.Init(shape->NLevels());
  _selectionMat.Init(shape->NLevels());

  WoundTextureSelectionsBasicDef *def = new WoundTextureSelectionsBasicDef;
  def->info = info;
  _def = def;

  //AnimationSectionBase::CheckSection();
}

void WoundTextureSelections::Init(
	LODShape *shape, Ref<WoundInfo> info, const char *name, const char *altName
)
{
  if (name)
  {
    _selectionTex.Init(shape->NLevels());
    _selectionMat.Init(shape->NLevels());

    WoundTextureSelectionsNameDef *def = new WoundTextureSelectionsNameDef;
    def->info = info;
    def->name = name;
    def->altName = altName;
    _def = def;
  }
  else
  {
    Init(shape,info);
  }
}

void WoundTextureSelections::Unload()
{
	for (int i=0; i<_selectionTex.Length(); i++)
	{
		_selectionTex[i].Clear();
	}
	for (int i=0; i<_selectionMat.Length(); i++)
	{
		_selectionMat[i].Clear();
	}
}


void WoundTextureSelections::Apply(AnimationContext &animContext, LODShape *shape, int level, int woundLevel, float dist2) const
{
	#if CHECK_INIT_SHAPE_WOUND
	  DoAssert(_initCount[level]>0);
	#endif

  Assert(shape->IsLevelLocked(level));
  const Shape *sShape = shape->GetLevelLocked(level);
  {
	  const WoundTextureSelectionArray &array = _selectionTex[level];
	  for (int a=0; a<array.Size(); a++)
	  {
		  array[a].Apply(animContext, sShape, woundLevel - 1, dist2);
	  }
	}
	{
	  const WoundMaterialSelectionArray &array = _selectionMat[level];
	  for (int a=0; a<array.Size(); a++)
	  {
		  array[a].Apply(animContext, sShape, woundLevel - 1, dist2);
	  }
	}
}

void WoundTextureSelections::ApplyModified(
  AnimationContext &animContext, LODShape *shape, int level, int woundLevel,
	Texture *orig, Texture *origWounded, float dist2
) const
{
	#if CHECK_INIT_SHAPE_WOUND
	  DoAssert(_initCount[level]>0);
	#endif
	
  Assert(shape->IsLevelLocked(level));
  const Shape *sShape = shape->GetLevelLocked(level);
  {
	  const WoundTextureSelectionArray &array = _selectionTex[level];
	  for (int a=0; a<array.Size(); a++)
	  {
		  const WoundTextureSelectionTex &sel = array[a];
		  if (sel.GetOrigTexture()!=orig || !origWounded)
		  {
			  sel.Apply(animContext, sShape, woundLevel - 1, dist2);
		  }
		  else
		  {
			  sel.Apply(animContext, sShape, origWounded, dist2);
		  }
	  }
	}
	// material do not offer special overrides
	{
	  const WoundMaterialSelectionArray &array = _selectionMat[level];
	  for (int a=0; a<array.Size(); a++)
	  {
		  array[a].Apply(animContext, sShape, woundLevel - 1, dist2);
	  }
	}
}
