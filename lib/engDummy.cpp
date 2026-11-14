#include "wpch.hpp"

#include "engine.hpp"
#include <El/QStream/qbStream.hpp>
#include <Es/Containers/listBidir.hpp>
#include "textbank.hpp"
#include "engDummy.hpp"

#define MAX_MIPMAPS 10

#if _USE_FCPHMANAGER
//! Global instance
FCPHManager GFCPHManager;
#endif

// some engine implementation is necessary
// we define dummy engine that provides no functionality

class TextureDummy: public Texture
{
  typedef Texture base;
  
  friend class TextBankDummy;
  //! texture source provider
  /*!
  \patch_internal 1.11 Date 7/31/2001 by Ondra
  - Fixed: TextureDummy converted from pac/paa only	to universal data source.
  */
  mutable SRef<ITextureSource> _src;

  //! Flag to determine the structure has been initialized already
  mutable bool _loaded;

  mutable int _w,_h;

  mutable int _nMipmaps;
  mutable PacLevelMem _mipmaps[MAX_MIPMAPS];

  //! Function to secure the class initialization, fast if it was already initialized
  int DoInit() const;

  void InitFromSource() const;
  
public:
  //! Constructor
  TextureDummy();
  void OnUnused() const;
  
  int AArea(int)const {DoInit(); return _w*_h;}
  int AWidth(int)const {DoInit(); return _w;}
  int AHeight(int)const {DoInit(); return _h;}
  int ANMipmaps(void)const {return 8;}
  int BestMipmap() const {return 0;}
  void ASetNMipmaps(int){}
  int ALoad(int,int){return 0;}
  Color GetPixel(int,float,float)const {return HBlack;}
  bool IsDynRangeCompressed() const {DoInit(); return _src && _src->GetMaxColor()!=PackedWhite;}
  QFileTime GetTimestamp() const {DoInit(); return _src ? _src->GetTimestamp() : 0;}

  //! Virtual method
  virtual PacFormat GetFormat() const {DoInit(); return _src ? _src->GetFormat() : PacUnknown;}
  
  bool IsTransparent() const {DoInit(); return _src && _src->IsTransparent();}
  bool IsAlpha() const {DoInit(); return _src && _src->IsAlpha();}
  bool IsAlphaNonOpaque() const {DoInit(); return _src && _src->IsAlphaNonOpaque();}
  int GetClamp() const {DoInit(); return _src ? _src->GetClamp() : 0;}
  void SetClamp(int clampFlags) {DoInit(); if (_src) _src->SetClamp(clampFlags);}
  Color GetColor() const
  {
    DoInit();
    return _src ? Color(safe_cast<Color>(_src->GetAverageColor()))*Color(safe_cast<Color>(_src->GetMaxColor())) : HBlack;
  }
  bool GetRect(int level, int x, int y, int w, int h, PacFormat format, void *data) const;
  bool RequestGetRect(int level, int x, int y, int w, int h) const;

  bool VerifyChecksum(const PacLevelMem *)const {return true;}

  //! Virtual method
  virtual TextureType Type() const {DoInit(); return _src ? _src->GetTextureType() : TT_Diffuse;}
  
  virtual TexFilter GetMaxFilter() const {DoInit(); return _src ? _src->GetMaxFilter() : TFAnizotropic16;}

  const class PacLevelMem &AMipmap(int)const {return *(PacLevelMem *)NULL;}
  class PacLevelMem &AMipmap(int){return *(PacLevelMem *)NULL;}
  bool VerifyChecksum(const class MipInfo &) const {return true;}

  int ProcessingThreadId() const {return -1;}
  void RequestDone(RequestContext *context, RequestResult result) {}
};

class TextBankDummy: public AbstractTextBank
{
  friend class TextureDummy;
  
  private:
  LLinkArray<TextureDummy> _texture;

  public:
  TextBankDummy();
  ~TextBankDummy();


  int Find(RStringB name1);
  Ref<Texture> CreateTexture(ITextureSource *source);
  Ref<Texture> Load(RStringB, bool);
  MipInfo UseMipmap(Texture *tex,int level,int top, int priority=TexPriorityNormal){return MipInfo(tex,level);}
  MipInfo UseMipmapLoaded(Texture *tex){return MipInfo(tex,0);}

  void Compact() {}
  void ReleaseMipmap(void){}
  void Preload(void){}

  void GetTextureList(LLinkArray<Texture> &list)
  {
    list.Realloc(_texture.Size());
    list.Resize(_texture.Size());
    for (int i=0; i<_texture.Size(); i++)
    {
      list[i] = _texture[i].GetLink();
    }
  }

  void FlushTextures(bool deep, bool allowAutoDeep) {} // flush temporary data
  void FlushBank(QFBank *bank) {}
};

class EngineDummy: public Engine
{
private:
  TextBankDummy *_bank;

public:
  EngineDummy();
  ~EngineDummy();
	bool IsDummy() { return true; }
  int GetMaxBonesCount() const {return 0;};
  void SetSkinningType(int cb, ESkinningType skinningType, const Matrix4 *bones = NULL, int nBones = 0) {};
  RString GetDebugName() const {return "No";}
  void InitDraw(void){}
  void FinishDraw(int wantedDurationVSync, bool allBackBuffers, RStringB fileName){}
  void Pause(void){}
  void Restore(void){}
  void DrawPicture555(unsigned short *){}
  void FogColorChanged(const class Color &){}
  void LightChanged(const class Color &,const class Color &){}
  void NightEffectChanged(float){}


  bool SwitchRes(int x, int y, int w, int h, int bpp) {return true;} // switch to resolution nearest to w,h
  bool SwitchRefreshRate(int refresh) {return true;} // switch to resolution nearest to w,h
  bool SwitchWindowed(bool windowed, int w=0, int h=0, int bpp=0) {return true;}

  void ListResolutions(FindArray<ResolutionInfo> &ret) {}
  void ListRefreshRates(FindArray<int> &ret) {}

  bool ZBiasExclusion() const {return false;}

  int PixelSize() const {return 32;}
  int RefreshRate() const {return 0;}
  bool CanBeWindowed() const {return true;}
  bool IsWindowed() const {return true;}

  //void DrawTriangle(class TLVertexMesh &,int,int,int,const class PacLevelMem *,int){}
  //bool BeginMesh(class TLVertexMesh &,int){return true;}
  //void EndMesh(class TLVertexMesh &){}
  class AbstractTextBank *TextBank(void){return _bank;}
  //HWND GetWindowHandle(void)const {return NULL;}
  float ZShadowEpsilon(void)const {return 0;}
  float ZRoadEpsilon(void)const {return 0;}
  float ObjMipmapCoef(void)const {return 1;}
  float LandMipmapCoef(void)const {return 1;}
  bool ShadowsFirst(void)const {return false;}
  bool SortByShape(void)const {return false;}
  int GetBias(int cb){return 0;}
  void SetBias(int cb, int bias){}
  void GetZCoefs(int cb, float &zAdd, float &zMult) {zAdd=0, zMult=1;}
  int Width(void)const {return 160;}
  int Height(void)const {return 120;}
  int Width2D(void)const {return 80;}
  int Height2D(void)const {return 60;}
  int AFrameTime(void)const {return 0;}

  void PrepareTriangle(int cb, const class PacLevelMem *,const TexMaterial *,int){}
  void DrawPolygon( const VertexIndex *i, int n ){}
  void DrawSection(const class FaceArray &,Offset b,Offset e) {}

  void DrawDecal
  (
    int cb, Vector3Par pos, float rhw, float sizeX, float sizeY, PackedColor col,
    const MipInfo &mip, const TexMaterial *mat, int specFlags
  ){}
  void Draw2D(const class PacLevelMem *,class PackedColor,float,float,float,float,float,float,float,float){}

  virtual void Clear(bool, bool, PackedColor, bool) {}
  void SetGamma(float) {}
  float GetGamma() const {return 1.0f;}
  float GetDefaultGamma() const {return 1.0f;}
  void PrepareTriangle(int cb, const MipInfo &, const TexMaterial *,int) {}
  void DrawPolygon(TLVertexTable &, const short *,int) {}
  void Draw2D(const Draw2DParsExt &, const Rect2DAbs &, const Rect2DAbs &) {}
  virtual void DrawLine(const TLVertexTable &mesh, int beg, int end, int orLine=0){}
  void DrawLinePrepare() {}
  void DrawLineDo(const Line2DAbs &, PackedColor, PackedColor, const Rect2DAbs &) {}
  void PrepareMesh(int cb, int spec) {}
  bool BeginMesh(int cb, TLVertexTable &, int) {return true;}
  void EndMesh(int cb, TLVertexTable &) {}
  float GetZCoef() const {return 1;}

  void DrawPolyPrepare
  (
    const MipInfo &mip, int specFlags
  ){}
  void DrawPolyDo
  (
    const Vertex2DPixel *vertices, int n, const Rect2DPixel &clipRect
  ){}
  void DrawPolyDo
  (
    const Vertex2DAbs *vertices, int n,
    const Rect2DAbs &clipRect
  ){}

  //! Virtual method
  virtual bool IsAsynchronousTextureLoadingSupported() const {return true;}

#if _ARMA3_SIMUL_WEATHER
  virtual SimulWeatherBase* CreateSimulWeather() {return 0;}
#endif
};

TextureDummy::TextureDummy()
{
  _loaded = false;
}

void TextureDummy::OnUnused() const
{
  if (GetName().GetLength()>0 && GEngine && GEngine->TextBank())
  {
    TextBankDummy *bank=static_cast<TextBankDummy *>(GEngine->TextBank());
    bank->_texture.Delete(this);
  }
  base::OnUnused();
}


static PacFormat BasicFormat( const char *name )
{
  const char *ext=strrchr(name,'.');
  if( ext && !strcmpi(ext,".paa") ) return PacARGB4444;
  else return PacARGB1555;
}

static PacFormat DstFormat( PacFormat srcFormat, int dxt )
{
  // memory representation
  switch (srcFormat)
  {
    case PacARGB1555:
    case PacRGB565:
    case PacARGB4444:
      return srcFormat;
    case PacP8:
      return PacARGB1555;
    case PacAI88:
      return srcFormat;
    case PacDXT1: case PacDXT2: case PacDXT3: case PacDXT4: case PacDXT5:
      return srcFormat;
    default:
      LogF("Unsupported source format %d",int(srcFormat));
      // TODO: fail
      return srcFormat;
  }
}

void TextureDummy::InitFromSource() const
{
  //in.seekg(0,QIOS::beg);
  // .paa should start with format marker

#if REPORT_ALLOC>=20
  LogF("Init texture %s",(const char *)Name());
#endif
  PacFormat format = _src->GetFormat();

  if
    (
    format==PacARGB4444 ||
    format==PacAI88 ||
    format==PacARGB8888
    )
  {
    _src->ForceAlpha();
  }


  int dxt = 0;
  PacFormat dFormat = DstFormat(format,dxt);

  int nMipmaps = _src->GetMipmapCount();
  int i;
  for( i=0; i<nMipmaps; i++ )
  {
    Assert(i < MAX_MIPMAPS);
    PacLevelMem &mip=_mipmaps[i];

    mip.SetDestFormat(dFormat,8);

    // do not load too small mip-maps
    if( mip._w<2 ) break;
    if( mip._h<2 ) break;
  }

  _nMipmaps=i;

  _w = _mipmaps[0]._w;
  _h = _mipmaps[0]._h;

  // Now we know the data were loaded
  _loaded = true;
}

int TextureDummy::DoInit() const
{
  // Return success, if the texture was loaded already
  if (_loaded) return 0;

  // parse texture header

  ITextureSourceFactory *factory = SelectTextureSourceFactory(Name());
  if (factory) _src = factory->Create(Name(),_mipmaps,MAX_MIPMAPS);

  if (_src)
  {

    InitFromSource();

    return 0;
  }	
  return -1;
}

bool TextureDummy::GetRect(
  int level, int x, int y, int w, int h, PacFormat format, void *data
) const
{
  //Assert(_initialized);

  // open texture file
  if (!_src) return false;

  PacLevelMem mip=_mipmaps[level];

  // only whole surface is currently supported
  DoAssert(x==0);
  DoAssert(y==0);
  DoAssert(w==mip._w);
  DoAssert(h==mip._h);
  // override default destination format
  mip._dFormat = format;

  int dataSize = PacLevelMem::MipmapSize(format,w,h);
  
  mip._pitch = dataSize/h;
  
  _src->GetMipmapData(data,mip,level,0);

  return true;
  
}

bool TextureDummy::RequestGetRect(int level, int x, int y, int w, int h) const
{
  //if (!_initialized) return false;
  // open texture file
  if (!_src) return false;

  // only whole surface is currently supported
  FileRequestPriority prior = FileRequestPriority(10);
  return _src->RequestMipmapData(_mipmaps,level,level,prior);
}


TextBankDummy::TextBankDummy()
{
}

TextBankDummy::~TextBankDummy()
{
  ClearTexCache();
  DeleteAllAnimated();
}

int TextBankDummy::Find( RStringB name1)
{
  int i;
  for( i=0; i<_texture.Size(); i++ )
  {
    TextureDummy *texture=_texture[i];
    if( texture )
    {
      if( texture->GetName()!=name1 ) continue;
      return i;
    }
  }
  return -1;
}

Ref<Texture> TextBankDummy::CreateTexture(ITextureSource *source)
{
  TextureDummy *texture=new TextureDummy;
  texture->_src = source;
  texture->InitFromSource();
  return texture;
}

Ref<Texture> TextBankDummy::Load(RStringB name, bool caseSensitive)
{
  int index=Find(name);
  if( index>=0 ) return (Texture*)_texture[index];

  Ref<Texture> texture = LoadCached(name);
  if (!texture)
  {
    texture = new TextureDummy;
    texture->SetName(name);
  }

  int iFree = _texture.FindKey(NULL);
  if (iFree<0) iFree=_texture.Add();
  _texture[iFree] = static_cast<TextureDummy *>(texture.GetRef());

  return texture;
}

EngineDummy::EngineDummy()
{
  _bank=new TextBankDummy();
}

EngineDummy::~EngineDummy()
{
  _fonts.Clear();
  if( _bank ) delete _bank,_bank=NULL;
}

Engine *CreateEngineDummy()
{
  return new EngineDummy();
}

#if _USE_FCPHMANAGER

TypeIsSimple(HANDLE);
TypeIsSimple(FCProcessHandles*);
void FCPHManager::LimitNumberOfRunProcesses()
{
  // Repeat until we reach the limit
  int pNum;
  while ((pNum = Size()) >= _maxProcesses)
  {
    // Construct array of handles
    AutoArray<HANDLE> handle;
    AutoArray<FCProcessHandles*> handleItem;
    handle.Realloc(pNum);
    handleItem.Realloc(pNum);
    FCProcessHandles *item = First();
    while (item)
    {
      handle.Add(item->_handle);
      handleItem.Add(item);
      item = Next(item);
    }

    // Wait for one process to finish
    int finishedObject = WaitForMultipleObjects(handle.Size(), handle.Data(), false, INFINITE) - WAIT_OBJECT_0;

    // Remove the process from the list
    if ((finishedObject >= 0) && (finishedObject < pNum))
    {
      Delete(handleItem[finishedObject]);
      delete handleItem[finishedObject];
    }
    else
    {
      // Finish just to prevent the possible infinite loop
      Fail("Error: strange value was returned while calling WaitForMultipleObjects");
      break;
    }
  }
}

void FCPHManager::AddItem(RStringB fileName, HANDLE handle)
{
  // Create new item and add it into list
  Add(new FCProcessHandles(fileName, handle));

  // Wait while we have more or equal processes than _maxProcesses (that means, if 
  // _maxProcesses == 1, wait until we finish the just created process)
  LimitNumberOfRunProcesses();
}

#endif
