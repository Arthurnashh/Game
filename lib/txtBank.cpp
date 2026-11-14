#include "wpch.hpp"
#include "textbank.hpp"
#include "engine.hpp"
#include "world.hpp"
#include "paramFileExt.hpp"
#include <El/QStream/qbStream.hpp>
#include <El/Evaluator/expressImpl.hpp>
#include <Es/Strings/bString.hpp>
#include "global.hpp"
#ifdef _WIN32
#include <Es/Common/win.h>
#endif

#if _ENABLE_PHYSICS
#include <Physics/enf_worldphysics.h>
#endif

int AbstractTextBank::AnimatedNumber( const char *name )
{
  // procedural textures are never animated
  if (name[0]=='#') return -1;
  // animated name has structure: name.number.ext (e.g. buch.01.tga)
  const char *n=strrchr(name,'\\');
  if (!n)
  {
    // can be a web address
    n = strrchr(name, '/');
    if (!n)
    {
    LogF("Bad texture name '%s'",name);
    return -1;
  }
  }
  n++;
  const char *nn=strchr(n,'.');
  // no dot - cannot be animated
  if( !nn ) return -1;
  // check if there is another dot after this one
  nn++;
  const char *ext=strchr(nn,'.');
  if (!ext) return -1;
  // number is placed between nn and ext
  // scan number, detect error
  int num = 0;
  while (isdigit(*nn))
  {
    num = num*10+*nn-'0';
    nn++;
  }
  if (*nn!='.')
  {
    LogF("Texture name %s looked like animated",name);
    return -1;
  }
  return num;
}

int AbstractTextBank::AnimatedName( const char *name, char *prefix, char *postfix )
{
  *postfix=0;
  strcpy(prefix,name);
  char *n=strrchr(prefix,'\\');
  if (!n)
  {
    // can be a web address
    n = strrchr(prefix, '/');
  if (!n) return -1;
  }
  n++;
  char *nn=strchr(n,'.');
  if( !nn ) return -1;
  int ret=atoi(nn+1);
  char *ext=strchr(nn+1,'.');
  if( !ext ) return -1;
  strcpy(postfix,ext); // use postfix
  *++nn=0; // terminate prefix
  return ret;
}

const char *AnimatedTexture::Name() const
{
  if( Size()<=0 ) return "";
  Texture *tex = Get(0);
  if (!tex) return RStringB();
  return tex->GetName();
}

RStringB AnimatedTexture::GetName() const
{
  if( Size()<=0 ) return "";
  Texture *tex = Get(0);
  if (!tex) return RStringB();
  return tex->GetName();
}

size_t Texture::GetMemoryControlled() const
{
  return sizeof(*this);
}

#if _DEBUG
void Texture::OnUsed() const
{
  Assert(!IsInList());
}
#endif

void Texture::OnUnused() const
{
  DoAssert(RefCounter()==0);
  if (GetName().GetLength()>0 && GEngine && GEngine->TextBank())
  {
    GEngine->TextBank()->OnUnused(unconst_cast(this));
    //DestroyLinkChain();
  }
  else
  {
    Destroy();
  }
}

void AbstractTextBank::FlushBankFromCache(QFBank *bank)
{
  for (Texture *tex = _cache.Last(); tex; )
  {
    Texture *prev = _cache.Prev(tex);
    // check if texture is from given bank
    if (!bank || bank->Contains(tex->GetName()))
    {
      Assert(tex->RefCounter()==0);
      _cache.Delete(tex);
      tex->Destroy();
    }
    tex = prev;
  }
}

bool AbstractTextBank::FlushCached(RStringB name)
{
  for (Texture *tex = _cache.Last(); tex; tex = _cache.Prev(tex))
  {
    if (tex->GetName() == name)
    {
      Assert(tex->RefCounter()==0);
      _cache.Delete(tex);
      tex->Destroy();
      return true;
    }
  }
  return false;
}

Ref<Texture> AbstractTextBank::LoadCached(RStringB name)
{
  for (Texture *tex = _cache.Last(); tex; tex = _cache.Prev(tex))
  {
    if (tex->GetName() == name)
    {
      Assert(tex->RefCounter()==0);
      _cache.Delete(tex);
      return tex;
    }
  }
  return NULL;
}

AnimatedTexture *AbstractTextBank::LoadAnimated(RStringB name, bool caseSensitive)
{
  // scan animated textures for a match
  int i;
  for( i=0; i<_animatedTextures.Size(); i++ )
  {
    AnimatedTexture *tex=_animatedTextures[i];
    if( tex && tex->GetName()==name ) return tex;
  }
  if (name[0]=='#')
    return NULL; // procedural textures are never animated
  // scan normal textures for a match
  char * prefix = new char[strlen(name) + 1];
  char * postfix = new char[strlen(name) + 1];;
  AnimatedName(name,prefix,postfix);
  // try to find texture with the same prefix
  int prefLen=strlen(prefix);
  for( i=0; i<_animatedTextures.Size(); i++ )
  {
    AnimatedTexture *tex=_animatedTextures[i];
    if( tex && !strncmp(tex->Name(),name,prefLen) )
    {
      Log("Double animated texture %s",(const char *)name);
      delete [] prefix;
      delete [] postfix;
      return tex;
    }
  }
  // load data
  AnimatedTexture *ret=new AnimatedTexture(this);
  int start=AnimatedNumber(name);
  if (start<0)
    return NULL;
  char * loadName = new char[prefLen + strlen(postfix) + 5];
  while (start>1)
  {
    sprintf(loadName,"%s%02d%s",prefix,start-1,postfix);
    if( !QFBankQueryFunctions::FileExists(loadName) ) break;
    start--;
  }
  for( i=start; i<=9999; i++ )
  {
    sprintf(loadName,"%s%02d%s",prefix,i,postfix);
    if( !QFBankQueryFunctions::FileExists(loadName) ) break;
    Ref<Texture> texture=Load(loadName);
    if (texture)
    {
      ret->Add(texture);
      texture->_inAnimation=ret;
    }
  }

  delete [] prefix;
  delete [] postfix;
  delete [] loadName;

  if( ret->Size()<=0 )
  {
    // create dummy animated texture - placeholder
    Ref<Texture> texture=Load(name);
    ret->Add(texture);
    //WarningMessage("Cannot load animated texture '%s'.",(const char *)name);
  }
  _animatedTextures.AddReusingNull(ret);
  return ret;
}

void AbstractTextBank::DeleteAnimated( AnimatedTexture *texture )
{
  int index=_animatedTextures.Find(texture);
  Assert( index>=0 );
  if( index>=0 ) _animatedTextures.Delete(index);
}

AnimatedTexture::AnimatedTexture( AbstractTextBank *bank )
:_bank(bank)
{
}

AnimatedTexture::~AnimatedTexture()
{
  int i,n=Size();
  #if !_RELEASE
  if( n>0 )
  {
    Log("Delete animated texture '%s'",Name());
  }
  #endif
  // remove reference from the bank
  //_bank->DeleteAnimated(this);
  for( i=0; i<n; i++ )
  {
    // unlink animation from all its textures
    Texture *texture=Get(i);
    // forget all textures
    if (texture) texture->_inAnimation=NULL;
    Set(i).Free();
  }
  Clear();
}

void AnimatedTexture::Remove( Texture *text )
{
  int i,n=Size();
  for( i=0; i<n; i++ )
  {
    if( (*this)[i]==text )
    {
      Delete(i);
      break;
    }
  }
}


Texture::Texture()
:_inAnimation(NULL)
{
  _someDataNeeded = false;
  _someDataWanted = false;
  _info = &SurfaceInfo::_default;
#if _ENABLE_PHYSICS
  _physMat = GLOB_ENGINE->TextBank()->GetPhysMaterial("material/default");
#endif
}

/*!
\patch 1.90 Date 11/7/2002 by Ondra
- Fixed: MP: Flying ammo crates or jumping vehicles
caused by inconsistent terrain roughness determination.
*/

void Texture::SetName(RStringB name, bool caseSensitive)
{
  #if _ENABLE_REPORT
    if (!caseSensitive && !name.IsLowCase())
    {
      RptF("Bad case in %s",(const char *)name);
      Fail("Bad case in texture name");
    }
  #endif
  _name = name;
  // use surface information to acquire roughness
  if (_name.GetLength()>0)
  {
    const SurfaceInfo &surface=GLOB_ENGINE->TextBank()->GetSurface(name);
    _info = &surface;
#if _ENABLE_PHYSICS
		_physMat = GLOB_ENGINE->TextBank()->GetPhysMaterial(name);
#endif
  }
  else
  {
    _info = &SurfaceInfo::_default;
#if _ENABLE_PHYSICS
		_physMat = GLOB_ENGINE->TextBank()->GetPhysMaterial("material/default");
#endif
  }
  ResetMipmap();
  // by default nothing is loaded
  _loadedTexDetail = 0;
  // by default assume 1x1
  _maxTexDetail = 1;
}

Texture::~Texture()
{
  DoAssert(!IsInList());
  DoAssert(!IsInSList());
  if (IsInSList())
  {
    // how can we remove it from the list? The only way is to copy the whole list and skip this item
    AbstractTextBank *bank = GEngine->TextBank();
    RemoveFromSList();
    bank->_loadRequested.Delete(this);
  }
  // if it is locked, it is too late to unlock it
  // actually, if it is locked, it should not be destroyed
  // texture bank unlock all textures before destroying itself
  // unless whole bank is 
  if( _inAnimation )
  {
    // if the texture is the last in some animated texture, remove whole animation
    // reference to any texture of the animation means reference to whole animation
    // delete any other textures in animation
    delete _inAnimation,_inAnimation=NULL;
  }
}

int Texture::AnimationLength() const
{
  if( !_inAnimation ) return 1; // no animation
  return _inAnimation->Size();
}

Texture *Texture::GetAnimation( int i ) const
{
  if( !_inAnimation ) return const_cast<Texture *>(this); // no animation
  Assert( i>=0 && i<_inAnimation->Size() );
  return _inAnimation->Get(i);
}

bool Texture::RequestAnimation( int i, int level ) const
{
  if( !_inAnimation ) return true;
  Assert( i>=0 && i<_inAnimation->Size() );
  AbstractTextBank *bank = GEngine->TextBank();
  // first check the level requested
  Texture *texI = _inAnimation->Get(i);
  MipInfo mipI = bank->UseMipmap(texI,level+1,level);
  return mipI._level<=level;
}

Texture *Texture::GetAnimationLoaded( int i ) const
{
  if( !_inAnimation ) return const_cast<Texture *>(this); // no animation
  Assert( i>=0 && i<_inAnimation->Size() );
  AbstractTextBank *bank = GEngine->TextBank();
  // first check the level requested
  {
    Texture *texI = _inAnimation->Get(i);
    MipInfo mipI = bank->UseMipmapLoaded(texI);
    if (mipI._level>=0 && mipI._level<texI->ANMipmaps())
    {
      return texI;
    }
  }
  // desired level not loaded - search for another one
  int bestP = i;
  int bestDist = INT_MAX;
  for (int p=0; p<_inAnimation->Size(); p++)
  {
    Texture *texP = _inAnimation->Get(p);
    MipInfo mipP = bank->UseMipmapLoaded(texP);
    if (mipP._level<0 || mipP._level>=texP->ANMipmaps()) continue;
    int dist = abs(p-i);
    if (bestDist>dist)
    {
      bestP = p;
      bestDist = dist;
    }
  }
  return _inAnimation->Get(bestP);
}

Ref<Texture> GlobLoadTexture(RStringB name, bool caseSensitive)
{
  if (name.GetLength()<=0) return NULL;
  #if _ENABLE_REPORT
    if (!caseSensitive)
    {
    RString low = name;
    low.Lower();
    if (strcmp(low,name))
    {
      RptF("Bad case in %s",(const char *)name);
    }
    }
  #endif
  if( AbstractTextBank::AnimatedNumber(name)>=0 )
  {
    AnimatedTexture *animated=GlobLoadTextureAnimated(name, caseSensitive);
    return animated->Get(0);
  }
/*
  if (stricmp(name, "xfonts\\helveticanarrowb22-03.paa") == 0)
  {
    LogF("Here");
  }
*/
  return GLOB_ENGINE->TextBank()->Load(name, caseSensitive);
}
AnimatedTexture *GlobLoadTextureAnimated(RStringB name, bool caseSensitive)
{
  return GLOB_ENGINE->TextBank()->LoadAnimated(name, caseSensitive);
}
Ref<Texture> GlobLoadTexture(ITextureSource *source)
{
  return GLOB_ENGINE->TextBank()->CreateTexture(source);
}
Ref<Texture> GlobLoadTextureUI(RStringB name)
{
  RString lowCaseName = name;
  lowCaseName.Lower();
  Ref<Texture> texture = GlobLoadTexture(lowCaseName);
  if (texture) texture->SetUsageType(Texture::TexUI);
  return texture;
}

/*!
\patch_internal 1.43 Date 1/22/2002 by Ondra
- Fixed: Only class members of CfgSurfaces are loaded.
*/

AbstractTextBank::AbstractTextBank()
{
  _textureMipBias=0;
  _textureMipBiasMin=0;
  _textureMipBiasAOTCoef=1;
  RegisterFreeOnDemandMemory(safe_cast<MHInstanceTexCache *>(this));
}

void AbstractTextBank::Init()
{
  // load surface information
  // load first from param file
  if( Pars.GetEntryCount()>0 )
  {
    ParamEntryVal par=Pars>>"CfgSurfaces";
    int i;
    for( i=0; i<par.GetEntryCount(); i++ )
    {
      ParamEntryVal entry = par.GetEntry(i);
      if (entry.IsClass())
      {
        BString<256> name;
        strcpy(name,"#");
        strcat(name,entry.GetName());
        SurfaceInfo *info = new SurfaceInfo(name);
        ConstParamEntryPtr pattern = entry.FindEntry("pattern");
        if (pattern)
        {
          info->_pattern = *pattern;
        }
        else
        {
          info->_name=entry>>"files";
        }
        _surfaces.Add(info);
      }
    }
    _surfaces.Compact();
  }
}

SurfaceInfo *AbstractTextBank::NewSurfaceInfo(const char *name)
{
  // try to find the surface info first
  for (int i=0; i<_surfaces.Size(); i++)
  {
    SurfaceInfo *info = _surfaces[i];
    if (!strcmpi(info->_entryName,name)) return info;
  }
  SurfaceInfo *info = new SurfaceInfo(name);
  _surfaces.Add(info);
  return info;
}


#if _ENABLE_CHEATS
RString AbstractTextBank::GetStat(int statId, RString &statVal, RString &statVal2)
{
  statVal = RString();
  statVal2 = RString();
  return RString();
}
#endif
/*!
\patch 1.35 Date 12/7/2001 by Ondra
- Fixed: Random crash during MP demo mission restart.
*/

#if 0
void AbstractTextBank::UnlockAllTextures()
{
  // check if any animated textures may be released
  for (int i=0; i<_animatedTextures.Size(); i++)
  {
    AnimatedTexture *anim = _animatedTextures[i];
    if (!anim) continue;
    bool isUsed = false;
    for (int a=0; a<anim->Size(); a++)
    {
      // MP demo crash fix:
      Texture *animA = anim->Get(a);
      if (animA && animA->RefCounter()>1)
      {
        isUsed = true;
        break;
      }
    }
    if (!isUsed)
    {
      _animatedTextures.Delete(i--);
    }
  }
}
#endif

//! Wildcard pattern matching function.

/*!
'*' may be used only as last character of the pattern
'*' matches any string in name
'?' matches any single character, not zero
*/

static bool PatternMatch( const char *name, const char *pattern )
{
  for(;;)
  {
    if( *name!=*pattern )
    {
      if (*pattern=='*') return true;
      if (*name==0) return false;
      // question marks matches any character, but not zero
      if (*pattern!='?') return false;
    }
    if (*pattern == 0) return true;
    Assert(*name!=0);
    name++,pattern++;
  }
  /*NOTREACHED*/
}

//! Wildcard pattern matching function for surface texture names.

/*!
caution: nasty trick to fix error in older data
final ?????? (six question marks) can match with 6 or 0 characters
*/

static bool PatternMatchQM6( const char *name, const char *pattern )
{
  // try matching full pattern first
  bool match = PatternMatch(name,pattern);
  if (match)
  {
    #if 0 //_ENABLE_REPORT
      if (strchr(pattern,'?') || strchr(pattern,'*'))
      {
        LogF("PatternMatch: %s matches %s",name,pattern);
      }
    #endif
    return match;
  }
  // full pattern does not match
  // check if there is trailing ??????
  int patlen = strlen(pattern);
  int namlen = strlen(name);
  // quick rejection test:
  // if ?????? should be matching, length of pattern must be length of name + 6
  if (namlen+6!=patlen) return false;
  int qmCount = 0;
  int patend = patlen;
  while (patend>0 && pattern[patend-1]=='?')
  {
    qmCount++;
    patend--;
  }
  if (qmCount<6) return false;

  // create short pattern
  const int shortPatBufSize = 256;
  int shortPatLen = patlen-6;
  // if pattern would overflow short pattern buffer, terminate
  if (shortPatLen>=shortPatBufSize) return false;
  BString<shortPatBufSize> shortPat;
  strncpy(shortPat,pattern,shortPatLen);
  shortPat[shortPatLen]=0;
  match = PatternMatch(name,shortPat);
  #if 0 //_ENABLE_REPORT
    if (match)
    {
      LogF("PatternMatchQM6: %s matches %s (%s)",name,pattern,shortPat.cstr());
    }
  #endif
  return match;
}

int AbstractTextBank::FindPath(const char *path) const
{
  // pattern matching ('?')
  // note: wild-cards are not in name, but in _surface[i]
  int i;
  for( i=0; i<_surfaces.Size(); i++ )
  {
    if (_surfaces[i]->_pattern.GetLength()<=0) continue;
    if (Matches(path,_surfaces[i]->_pattern) ) return i;
  }
  return -1;
}

int AbstractTextBank::FindFileName(const char *name) const
{
  // pattern matching ('?')
  // note: wild-cards are not in name, but in _surface[i]
  int i;
  for( i=0; i<_surfaces.Size(); i++ )
  {
    if (_surfaces[i]->_name.GetLength()<=0) continue;
    if( PatternMatchQM6(name,_surfaces[i]->_name) ) return i;
  }
  return -1;
}


int TextureSampler::MipmapSize( PacFormat format, int w, int h )
{
  return PacLevelMem::MipmapSize(format,w,h);
}

TextureSampler::TextureSampler(const char *name)
{
  ITextureSourceFactory *factory = SelectTextureSourceFactory(name);
  if (!factory || !factory->Check(name))
  {
    WarningMessage("Unknown sampler texture type %s.",(const char *)name);
    return;
  }
  _src = factory->Create(name,_mipmaps,MaxMipmaps);
  if (!_src)
  {
    WarningMessage("Cannot load sampler texture %s.",(const char *)name);
    return;
  }
  // TODO: skip mipmaps which are too fine
  // get all mipmap data immediately
  for (int i=0; i<_src->GetMipmapCount(); i++)
  {
    // calculate mipmap size
    PacLevelMem &mip = _mipmaps[i];
    int size = MipmapSize(mip._sFormat,mip._w,mip._h);
    _data[i] = new char[size];
    mip._pitch = size/mip._h;
    mip._dFormat = mip._sFormat;
    if (!_src->GetMipmapData(_data[i],_mipmaps[i],i,0))
    {
      memset(_data[i],0,size);
    }
  }
  
}


Color TextureSampler::GetPixelBilinear(float u, float v, int level) const
{
  const PacLevelMem &mip = _mipmaps[level];
  const void *data = _data[level];
  
  int iU=toIntFloor(u*mip._w);
  int iV=toIntFloor(v*mip._h);
  iU &= mip._w-1;
  iV &= mip._h-1;
  
  PackedColor color = mip.GetPixelInt(data,iU,iV);
  return color;
}

Color TextureSampler::GetPixelPoint(float u, float v, int level) const
{
  const PacLevelMem &mip = _mipmaps[level];
  const void *data = _data[level];

  int iU=toIntFloor(u*mip._w);
  int iV=toIntFloor(v*mip._h);
  iU &= mip._w-1;
  iV &= mip._h-1;
  
  PackedColor color = mip.GetPixelInt(data,iU,iV);
  return color;
}

TextureSampler::~TextureSampler()
{
  for (int i=0; i<_src->GetMipmapCount(); i++)
  {
    delete[] _data[i];
    _data[i] = NULL;
  }
}

SurfaceInfo::SurfaceInfo()
{
  Init();
}

SurfaceInfo::SurfaceInfo(const char *name)
{
  _entryName = name;
  if (*name!='#')
  {
    GameDataNamespace globals(NULL, RString(), false); // TODO: parsing namespace if access to globals needed

    ParamFile surfFile;
    surfFile.ParseBinOrTxt(name, NULL, NULL, &globals);
    if (surfFile.GetEntryCount()==0)
    {
      RptF("Cannot load surface info %s",cc_cast(name));
      Init();
    }
    else
    {
      Load(surfFile);
    }
    #if SINFO_RELOAD
      _timestamp = QFBankQueryFunctions::TimeStamp(name);
    #endif
  }
  else
  {
    ParamEntryVal entry = Pars>>"CfgSurfaces">>(name+1);
    Load(entry);
    #if SINFO_RELOAD
      _timestamp = 0;
    #endif
  }
}

SurfaceInfo::SurfaceInfo(ParamEntryPar entry)
{
  /// caution: entry name is not fully qualified here, cannot be used for serialization
  Fail("Obsolete - name not supplied");
  _entryName = entry.GetName();
  Load(entry);
}

void SurfaceInfo::Init()
{
  _roughness = 0;
  _dustness = 0;
  _isWater = false;
  _bulletPenetrability = 0;
  _terrainClutterFactor = 0;
  #if SINFO_RELOAD
    _timestamp = 0;
  #endif
  _audibility = 1.0f; 
  for(int n = 0; n < VPCount; n++)
    _visibility[n] = 1.0f;
}

static const EnumName SurfaceSounds[]=
{
  EnumName(SurfSGrassness, "grass"),
  EnumName(SurfSRockness, "rock"),
  EnumName(SurfSSandness, "sand"),
  EnumName(SurfSMudness, "mud"),
  EnumName(SurfSGravelness, "gravel"),
  EnumName(SurfSAsphaltness, "asphalt")
};


void SurfaceInfo::Load(ParamEntryPar entry)
{
  // TODO: surface type given in config, like character
  _surfaceType = entry.GetName();

  _roughness=entry>>"rough";
  _dustness=entry>>"dust";
  _soundEnv=entry>>"soundEnviron";
  _impact = entry >> "impact";
  
  _audibility = entry.ReadValue("audibility", 1.0f);
  for(int n = 0; n < VPCount; n++)
    _visibility[n] = 1.0f;
  if(entry.CheckIfEntryExists("visible"))
  {
    ParamEntryVal visible = entry >> "visible";
    if(visible.IsClass())
    {
      for(int n = 0; n < visible.GetEntryCount(); n++)
      {
        ParamEntryVal type = visible.GetEntry(n);
        if(strcmpi(type.GetName(), "down") == 0)
          _visibility[VPDown] = type;
        if(strcmpi(type.GetName(), "prone") == 0)
          _visibility[VPProne] = type;
        if(strcmpi(type.GetName(), "kneel") == 0)
          _visibility[VPKneel] = type;
        if(strcmpi(type.GetName(), "stand") == 0)
          _visibility[VPStand] = type;
      }
    }
  }

  _isWater = entry>>"isWater" ;
  
#if _HELISIM_TERRAIN_RIVERS
  _isWater = _isWater || (strcmpi(_soundEnv, "water") == 0);
#endif

  ConstParamEntryPtr clutterFactor = entry.FindEntry("terrainClutterFactor");
  if (clutterFactor) _terrainClutterFactor = *clutterFactor;
  else _terrainClutterFactor = _isWater ? 1 : 0;

  ConstParamEntryPtr character = entry.FindEntry("character");
  if (character)
  {
    RStringB charName = *character;
    ParamEntryVal charDesc = Pars>>"CfgSurfaceCharacters">>charName;
    float sumProbability = 0;
    
    ParamEntryVal array = charDesc>>"probability";
    int nItems = array.GetSize();

    // load clutter names
    // if not found, assume default ones
    ConstParamEntryPtr ptr = charDesc.FindEntry("names");

    // limit now applies only to default names
    const int maxDefItems = 4;
    if (!ptr && nItems>maxDefItems)
    {
      RptF("Too many clutter types in %s (%d>%d)",cc_cast(charDesc.GetContext()),nItems,maxDefItems);
      nItems = 4;
    }
    _character.Realloc(nItems);
    _character.Resize(nItems);
    for (int i=0; i<nItems; i++)
    {
      float prop = (charDesc>>"probability")[i];
      sumProbability += prop;
      _character[i]._probabilityThold = sumProbability;
    }
    if (ptr)
    {
      if (ptr->GetSize()!=array.GetSize())
      {
        RptF(
          "Clutter names value count different in %s (%d!=%d)",
          cc_cast(charDesc.GetContext()),ptr->GetSize(),array.GetSize()
        );
      }
      for (int i=0; i<nItems; i++)
      {
        RString name = (*ptr)[i];
        name.Lower();
        _character[i]._name = name;
        _character[i]._landIndex = -1;
      }
    }
    else
    {
      RptF("Default clutter names TypeX used for %s",cc_cast(charDesc.GetContext()));
      static const RStringB clutterN[maxDefItems]=
      {
        RStringB("type1"),RStringB("type2"),RStringB("type3"),RStringB("type4"),
      };
      for (int i=0; i<nItems; i++)
      {
        _character[i]._name = clutterN[i];
        _character[i]._landIndex = -1;
      }
    }
    // trim any values with zero probability
  }
  else
  {
    Log("No character for %s",(const char *)entry.GetContext());
  }
  ConstParamEntryPtr heightMapEntry = entry.FindEntry("heightMap");
  if (heightMapEntry)
  {
    RString heightMapName = *heightMapEntry;
    heightMapName.Lower();
    _heightMap = new TextureSampler(heightMapName);
  }

  _bulletPenetrability = 0;
  //
  ConstParamEntryPtr bulletPenetrability = entry.FindEntry("bulletPenetrabilityWithThickness");
  if (bulletPenetrability)
  {//Arma ArrowHead bulletPenetrability
    // source is thickness in mm required to stop bullet with velocity 1000 m/s
    _bulletPenetrability = 1e6/float(*bulletPenetrability);
  }
  else
  {//Arma classic bulletPenetrability
    bulletPenetrability = entry.FindEntry("bulletPenetrability");
    if (bulletPenetrability)
    {
      // source is thickness in mm required to stop bullet with velocity 1000 m/s
      _bulletPenetrability = 1e6/float(*bulletPenetrability);
    }
  }

  _thickness = -1;
  ConstParamEntryPtr thickness = entry.FindEntry("thickness");
  if (thickness)
  {
    // source is thickness in mm
    _thickness = float(*thickness)*0.001f;
  }
  ConstParamEntryPtr hitSound = entry.FindEntry("soundHit");
  if (hitSound)
  {
    RStringB hitSoundStr = *hitSound;

    BString<64> lowCaseSound;
    strcpy(lowCaseSound,hitSoundStr);
    strlwr(lowCaseSound);
    if (strcmp(lowCaseSound, "soft_ground") == 0)
      _soundHit = SHGround;
    else if (strcmp(lowCaseSound, "hard_ground") == 0)
      _soundHit = SHGroundHard;
    else if (strcmp(lowCaseSound, "building") == 0)
      _soundHit = SHBuilding;
    else if (strcmp(lowCaseSound, "glass") == 0)
      _soundHit = SHGlass;
    else if (strcmp(lowCaseSound, "wood") == 0)
      _soundHit = SHWood;
    else if (strcmp(lowCaseSound, "foliage") == 0)
      _soundHit = SHFoliage;
    else if (strcmp(lowCaseSound, "body") == 0)
      _soundHit = SHMan;
    else if (strcmp(lowCaseSound, "iron") == 0)
      _soundHit = SHIron;
    else if (strcmp(lowCaseSound, "metal") == 0)
      _soundHit = SHArmor;
    else if (strcmp(lowCaseSound, "metal_plate") == 0)
      _soundHit = SHArmor;
    else if (strcmp(lowCaseSound, "glass_armored") == 0)
      _soundHit = SHGlassArmored;
    else if (strcmp(lowCaseSound, "plastic") == 0)
      _soundHit = SHPlastic;
    else if (strcmp(lowCaseSound, "concrete") == 0)
      _soundHit = SHConcrete;
    else if (strcmp(lowCaseSound, "rubber") == 0)
      _soundHit = SHRubber;
    else
    {
      RptF("Unknown hit sound type in %s",cc_cast(entry.GetContext(hitSound->GetName())));
      _soundHit = SHDefault;
    }
  }
  else
  {
    _soundHit = SHDefault;
  }

  _surfSounds.Realloc(lenof(SurfaceSounds));
  _surfSounds.Resize(lenof(SurfaceSounds));
  
  // set all to zero
  for (int i = 0; i < _surfSounds.Size(); i++) _surfSounds[i] = 0;
  const char *ccSoundEnv = cc_cast(_soundEnv);

  // list of all surfaces: road, gravel, wood, metal, rock, sand, hallway, grass, drygrass, forest, mud
  if (!strcmpi(ccSoundEnv, "road"))
  {
    _surfSounds[SurfSAsphaltness] = 1;    
  } 
  else if (!strcmpi(ccSoundEnv, "gravel"))
  {
    _surfSounds[SurfSGravelness] = 1;
  }
  else if (!strcmpi(ccSoundEnv, "rock"))
  {
    _surfSounds[SurfSRockness] = 1; 
  }
  else if (!strcmpi(ccSoundEnv, "sand"))
  {
    _surfSounds[SurfSSandness] = 1; 
  }
  else if (!strcmpi(ccSoundEnv, "hallway"))
  {
    _surfSounds[SurfSRockness] = 1;     
  }
  else if (!strcmpi(ccSoundEnv, "grass")) // cr_trava1, cr_trava2
  {
    _surfSounds[SurfSGrassness] = 1; 
  }
  else if (!strcmpi(ccSoundEnv, "drygrass"))
  {
    _surfSounds[SurfSGrassness] = 1; 
  }
  else if (!strcmpi(ccSoundEnv, "forest"))
  {
    _surfSounds[SurfSGrassness] = 1;    
  }
  else if (!strcmpi(ccSoundEnv, "mud"))
  {
    _surfSounds[SurfSMudness] = 1; 
  }
  else if (!strcmpi(ccSoundEnv, "wood"))
  {

  }
  else if (!strcmpi(ccSoundEnv, "metal"))
  {

  }
  else if (!strcmpi(ccSoundEnv, "water"))
  {

  }
  else if (!strcmpi(ccSoundEnv, "normalExt"))
  {

  }
  else if (!strcmpi(ccSoundEnv, "empty"))
  {

  }
}

const EnumName *GetEnumNames(SufraceSound dummy)
{
  return SurfaceSounds;
}

float SurfaceInfo::GetSurfSound(SufraceSound type) const
{
  Assert(_surfSounds.Size() > type);

  if (_surfSounds.Size() > type) { return _surfSounds[type]; }
  
  return 0.0f;
}

SurfaceInfo::~SurfaceInfo()
{
  _surfSounds.Clear();
}

SurfaceInfo SurfaceInfo::_default;

#if _ENABLE_PHYSICS
enf::PhysicsMaterial* AbstractTextBank::GetPhysMaterial(const char *name) const
{
	if(!GWorld || !GWorld->GetPhysicsManager())
		return 0;

	enf::PhysicsMaterial* mat = GWorld->GetPhysicsManager()->FindMaterial(name);
	if(mat)
		return mat;
	
	// get only pure name (without path or extension)
  const char *fName=strrchr(name,'\\');
  if( fName ) fName++;
  else fName=name;
  char sName[MAX_PATH];
  strcpy(sName,fName);
  char *ext=strrchr(sName,'.');
  if( ext ) *ext=0;
  // search
	mat = GWorld->GetPhysicsManager()->FindMaterial(sName);
	if(mat)
		return mat;

	return GWorld->GetPhysicsManager()->FindMaterial("material/default");

}
#endif

const SurfaceInfo &AbstractTextBank::GetSurface(const char *name) const
{
  int index = FindPath(name);
  if (index>=0)
  {
    return *_surfaces[index];
  }
  // get only pure name (without path or extension)
  const char *fName=strrchr(name,'\\');
  if( fName ) fName++;
  else fName=name;
  char sName[MAX_PATH];
  strcpy(sName,fName);
  char *ext=strrchr(sName,'.');
  if( ext ) *ext=0;
  // search 
  index=FindFileName(sName);
  if( index<0 )
  {
    index=FindFileName("material/default");
    if( index<0 )
    {
      return SurfaceInfo::_default;
    }
  }
  return *_surfaces[index];
}

AbstractTextBank::~AbstractTextBank()
{
  ClearTexCache();
  _surfaces.Clear();
}

/*!
\patch_internal 1.28 Date 10/25/2001 by Ondra
- Fixed: crash in texture bank destruction during Abort (like in ErrorMessage).
*/

void AbstractTextBank::DeleteAllAnimated()
{
  // make sure any outstanding listed textures are flushed before Texture Bank destruction
  // Strange: this seems to be happenning frequently in Profile/Release build, but not in a Debug one.
  while(Ref<Texture> get = _loadRequested.Get())
  {
    get->RemoveFromSList();
    Log("Flushing listed texture %s",cc_cast(get->GetName()));
  }
  _loadRequested.Clear();
  
  // Assert( _animatedTextures.Size()==0 );
  _animatedTextures.Clear();

}

void AbstractTextBank::StartFrame()
{
}

void AbstractTextBank::FinishFrame()
{
}

void AbstractTextBank::PerformMipmapLoading()
{
}


float AbstractTextBank::PriorityTexCache() const
{
  return 0.005f;
}
RString AbstractTextBank::GetDebugNameTexCache() const
{
  return "TexCache";
}
size_t AbstractTextBank::MemoryControlledTexCache() const
{
  return _cache.MemoryControlled();
}
size_t AbstractTextBank::MemoryControlledRecentTexCache() const
{
  return _cache.MemoryControlledRecent();
}
size_t AbstractTextBank::FreeOneItemTexCache()
{
  Texture *last = _cache.First();
  if (!last) return 0;
  DoAssert(last->RefCounter()==0);
  size_t ret = _cache.Delete(last);
  last->Destroy();
  return ret;
}

void AbstractTextBank::ClearTexCache()
{
  for(;;)
  {
    Texture *last = _cache.First();
    if (!last) break;
    Assert(last->RefCounter()==0);
    _cache.Delete(last);
    last->Destroy();
  }
  // make sure AnimatedTextures are not kept when not necessary
  for (int i=0; i<_animatedTextures.Size(); i++)
  {
    AnimatedTexture *anim = _animatedTextures[i];
    if (!anim || anim->RefCounter()>1) continue;
    bool isUsed = false;
    // if there is no other reference to any of the Textures
    // than the one we own, the animated texture is unused
    for (int a=0; a<anim->Size(); a++)
    {
      Texture *animA = anim->Get(a);
      if (animA && animA->RefCounter()>1)
      {
        isUsed = true;
        break;
      }
    }
    if (!isUsed)
    {
      _animatedTextures[i] = NULL;
    }
  }
}

void AbstractTextBank::OnUnused(Texture *tex)
{
  DoAssert(tex->RefCounter()==0);
  _cache.Add(tex);
}

void AbstractTextBank::MemoryControlledFrameTexCache()
{
  _cache.Frame();
}

void AbstractTextBank::SetTextureMipBias(float bias)
{
  _textureMipBias=bias;
  _textureMipBiasAOTCoef = Texture::AOTCoefFromMipmapLevel(_textureMipBias);
}


LLink<Texture> DefaultTexture;
