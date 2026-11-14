#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PACTEXT_HPP
#define _PACTEXT_HPP

class OggDecoder;


// basic structure for storing bitmap data
#include <Es/Types/memtype.h>
#include "types.hpp"
#include <El/QStream/qStream.hpp>
#include <El/Color/colors.hpp>
#include <El/Enum/enumNames.hpp>

#include <Es/Containers/boolArray.hpp>
#include <El/QStream/serializeBin.hpp>

#define PAC_FORMAT_ENUM(type,prefix,XX) \
  XX(type, prefix, P8) \
  XX(type, prefix, AI88) \
  XX(type, prefix, RGB565) \
  XX(type, prefix, ARGB1555) \
  XX(type, prefix, ARGB4444) \
  XX(type, prefix, ARGB8888) \
  XX(type, prefix, DXT1) \
  XX(type, prefix, DXT2) \
  XX(type, prefix, DXT3) \
  XX(type, prefix, DXT4) \
  XX(type, prefix, DXT5) \
  XX(type, prefix, Unknown)

/// surface format
#ifndef DECL_ENUM_PAC_FORMAT
#define DECL_ENUM_PAC_FORMAT
DECL_ENUM(PacFormat)
#endif
DECLARE_ENUM(PacFormat, Pac, PAC_FORMAT_ENUM)

static inline bool PacFormatIsDXT(PacFormat format)
{
  return format>=PacDXT1;
}

class PacPalette;

#if _RELEASE
  #define CHECKSUMS 0
#else
  #define CHECKSUMS 0
#endif

class PacLevelMem
{
  public:
  // short for texture dimensions and pitch should be enough
  short _w,_h; // note:
  short _pitch; // DDraw requires DWORD (or QWORD) alignement

  SizedEnum<PacFormat,char> _sFormat; // source (file) format
  SizedEnum<PacFormat,char> _dFormat; // destination (memory) format

  int _start; // where in the file this mipmap level start

  // note: _memData is often used as temporary pointer
  //void *_memData; // _data for one mipmap levels


  public:
  PacLevelMem();

  PackedColor GetPixel(const void *data, float u, float v) const;
  PackedColor GetPixelInt(const void *data, int u, int v) const;

  static void DecompressDXT1(void *dst, const void *src, int w, int h);
  static void DecompressDXT2(void *dst, const void *src, int w, int h);
  static void DecompressDXT4(void *dst, const void *src, int w, int h);

  void SerializeBin(SerializeBinStream &f);

  protected:

  // paa 16b formats loading
  int LoadPaaBin16( QIStream &in, void *mem, const PacPalette *pal ) const;
  int LoadPaaDXT( QIStream &in, void *mem, const PacPalette *pal, int texSize ) const;


  // pac formats loading
  int LoadPacARGB1555( QIStream &in, void *mem, const PacPalette *pal ) const;
  int LoadPacRGB565( QIStream &in, void *mem, const PacPalette *pal ) const;

  public:
  void Interpolate
  (
    void *data, void *withData,
    const PacLevelMem &with, float factor
  );

  //! fill data with a color
  bool FillMipmap(void *dta, PackedColor color) const;

  int LoadPac( QIStream &in, void *mem, const PacPalette *pal, int texSize ) const; // format set by init

  int LoadPaa( QIStream &in, void *mem, const PacPalette *pal, int texSize ) const; // format set by init

  void SetStart(int offset){_start=offset;}
  int Init(PacFormat sFormat); // no file init - used for JPG files
  int Init(QIStream &in, PacFormat sFormat);
  void SetDestFormat(PacFormat dFormat, int align);
  //int Skip( QIStream &in );
  PacFormat SrcFormat() const {return _sFormat.GetEnumValue();}
  PacFormat DstFormat() const {return _dFormat.GetEnumValue();}
  int Pitch() const {return _pitch;}
  int Size() const {Assert(_pitch > 0); return _pitch*_h;}
  bool TooLarge( int max=256 ) const {return _w>max || _h>max;}
  /// size estimation - no alignment considered
  /** Usefull when allocating memory for GetMipmapData */
  static int MipmapSize(PacFormat format, int w, int h);
};

TypeIsMovable(PacLevelMem)

const char *FormatName( PacFormat format );
PacFormat PacFormatFromDesc( int desc, bool &alpha );

bool IsWebSource(const char *filename);

class MipInfo
{
  public:
  Texture *_texture;
  int _level;

  MipInfo( Texture *texture, int level )
  :_texture(texture),_level(level)
  {}
  MipInfo()
  :_texture(NULL),_level(-1)
  {}

  bool IsOK() const {return _level>=0;}
  void SetNoTexture()
  {
    _texture = NULL;
    _level = 0;
  }
};

TypeIsBinary(MipInfo);


#define TRANSPARENT1_RGB 0xff00ff
#define TRANSPARENT2_RGB 0x00ffff

/// flags used to indicate clamping in UV directions
enum {TexClampU=1,TexClampV=2};

/// information about colors and attributes
class PacPalette
{
  //friend class PacLevel;

  public:
  //MemoryHeap *_heap;
  int _nColors; // palette size
  DWORD *_palette; // 24-bit RGB format
  //Pixel *_palPixel; // pixel-format palette (suitable for copy mode)

  Color _averageColor;
  PackedColor _averageColor32;
  //! color used to maximize dynamic range
  PackedColor _maxColor32;

  /// forced clamping in any given direction
  int _clampFlags;
  // this information is same for all mipmaps, but is also copied in mipmap header
  int _transparentColor;
  //! check if _maxColor32 was loaded
  bool _maxColorSet;
  bool _isAlpha,_isTransparent,_isAlphaNonOpaque;

  #if CHECKSUMS
    mutable bool _checksumValid;
    mutable int _checksum;
  #endif

  public:
  PacPalette();
  ~PacPalette();

  PacPalette(const PacPalette &src);
  PacPalette &operator=( const PacPalette &src);

  int Load( QIStream &in, int *startOffsets, int maxStartOffsets, PacFormat format );
  static int Skip( QIStream &in );

  ColorVal AverageColor() const {return _averageColor;}
  PackedColor AverageColor32() const {return _averageColor32;}
  PackedColor MaxColor32() const {return _maxColor32;}

  void SerializeBin(SerializeBinStream &f);

private:
  void Clone(const PacPalette &src);
};

//! Set of texture types
enum TextureType
{
  /// diffuse color, SRGB color space
  TT_Diffuse,
  /// diffuse color, linear color space
  TT_Diffuse_Linear,
  /// detail texture, linear color space
  TT_Detail,
  /// normal map
  TT_Normal,
  /// irradiance map
  TT_Irradiance,
  //! Map with random values from interval <0.5, 1>
  TT_RandomTest,
  /// tree crown calculation texture
  TT_TreeCrown,
  /// diffuse "macro" object color, SRGB color space
  TT_Macro,
  /// ambient shadow layer
  TT_AmbientShadow,
  /// amount of specular layer
  TT_Specular,
  /// dithering texture
  TT_Dither,
  /// amount of detail specular layer
  TT_DTSpecular,
  /// mask used for texture selection in the multi-shader
  TT_Mask,
  /// Thermal Image texture
  TT_TI,
  //! Number of texture types
#if _HELISIM_RENDER_TO_TEXTURE
  TT_R2T,
#endif
  NTextureType
};


/** TFAnizotropic means auto-detect anisotropy level based on PixelShaderID while loading the material **/

#define TEXFILTER_ENUM(type,prefix,XX) \
  XX(type, prefix, Point) \
  XX(type, prefix, Linear) \
  XX(type, prefix, Trilinear) \
  XX(type, prefix, Anizotropic) \
  XX(type, prefix, Anizotropic2) \
  XX(type, prefix, Anizotropic4) \
  XX(type, prefix, Anizotropic8) \
  XX(type, prefix, Anizotropic16) \

//! texture sampling
#ifndef DECL_ENUM_TEX_FILTER
#define DECL_ENUM_TEX_FILTER
DECL_ENUM(TexFilter)
#endif
DECLARE_ENUM(TexFilter,TF,TEXFILTER_ENUM)

//! Texture source interface
class ITextureSource
{
public:
  virtual ~ITextureSource() {}

  virtual  bool IgnoreLoadedLevels() const { return false;  };
  virtual  OggDecoder* getDecoder() const {  return NULL; };

#if _HELISIM_RENDER_TO_TEXTURE
  //! Get RT name identifier
  virtual RString GetR2TName() const { return "InValid"; }
  //! Render target aspect
  virtual float GetR2TAspect() const { return -1.0f; }
#endif

  //! Init data source
  /*! Must be called before any other methods.*/
  virtual bool Init(PacLevelMem *mips, int maxMips) = 0;
  //! Get mipmap count.
  virtual int GetMipmapCount() const = 0;
  //! Get source data format.
  virtual PacFormat GetFormat() const = 0;
  //! Get the type of the texture
  virtual TextureType GetTextureType() const = 0;
  //! Get max. filter having sense for given texture
  virtual TexFilter GetMaxFilter() const = 0;

  //! Check if texture should be alpha blended
  virtual bool IsAlpha() const = 0;
  //! Check if texture should be alpha blended and overall opacity is very low
  virtual bool IsAlphaNonOpaque() const = 0;
  //! Check if texture should be alpha tested
  virtual bool IsTransparent() const = 0;

  bool IsPlatformLittleEndian() const
  {
    #ifdef _M_PPC
    return false;
    #else
    return true;
    #endif
  }
  /// on some platforms (X360) we may have dual data - some little endian, some big endian
  virtual bool IsLittleEndian() const = 0;
  //! Get mipmap data.
  virtual bool GetMipmapData(void *mem, const PacLevelMem &mip, int level, int texSize) const = 0;

  //! Request background loading of the data.
  virtual bool RequestMipmapData(
    const PacLevelMem *mips, int levelMin, int levelMax, FileRequestPriority prior
  ) const = 0;

  //! Get average color (packed format)
  virtual PackedColor GetAverageColor() const = 0;
  //! Get max. dynamic range color (packed format)
  virtual PackedColor GetMaxColor() const = 0;

  /// get what detail is represented by the texture when best mipmap is loaded
  /** For some textures we assume infinite detail */
  virtual float GetMaxTexDetail(float maxTexDetail) const = 0;
  //! We know from some reason texture is alpha blended
  virtual void ForceAlpha() = 0;

  /// modification time, 0 if not applicaple/not available
  virtual QFileTime GetTimestamp() const = 0;
  virtual void FlushHandles() = 0;

  // @{ clamping interface
  virtual int GetClamp() const = 0;
  virtual void SetClamp(int clampFlags) = 0;
  //@}

  virtual void SerializeBin(SerializeBinStream &f) {}
};

class TextureSourcePacBase: public ITextureSource
{
protected:
  PacPalette _pal;
  int _mipmaps; //!< source file mipmap count
  PacFormat _format; //!< source file pixel format
  bool _littleEndian; //!< data endian format
  bool _isPaa;
  RStringB _name;
  //! Type of the texture the texture source represents
  TextureType _tt;

public:
  TextureSourcePacBase();
  ~TextureSourcePacBase();

  virtual TextureType GetTextureType() const {return _tt;};
  virtual TexFilter GetMaxFilter() const {return TFAnizotropic16;}

  //! Set name, fill out texture type
  void SetName(const char *name);
  const RStringB& GetName() const {return _name;}

  bool Init(PacLevelMem *mips, int maxMips);


  int GetMipmapCount() const;
  PacFormat GetFormat() const;
  bool GetMipmapData(void *mem, const PacLevelMem &mip, int level, int texSize) const;
  bool IsLittleEndian() const {return _littleEndian;}
  bool RequestMipmapData(
    const PacLevelMem *mips, int levelMin, int levelMax, FileRequestPriority prior
  ) const;

  PackedColor GetAverageColor() const {return _pal.AverageColor32();}
  PackedColor GetMaxColor() const {return _pal.MaxColor32();}

  bool IsAlpha() const {return _pal._isAlpha;}
  bool IsAlphaNonOpaque() const {return _pal._isAlphaNonOpaque;}
  bool IsTransparent() const {return _pal._isTransparent;}
  int GetClamp() const {return _pal._clampFlags;}
  void SetClamp(int clampFlags) {_pal._clampFlags = clampFlags;}

  float GetMaxTexDetail(float maxTexDetail) const;

  /// hack used to fix textures that have alpha flag missing
  void ForceAlpha()
  {
    // no need to force alpha when max color is set
    // - in such case attributes are always correct
    if (_pal._isAlpha || _pal._maxColorSet) return;
    _pal._isAlpha = true;
  }

  virtual void SerializeBin(SerializeBinStream &f);

protected:
  /// some streams may need some preparation so that they can be used to load mipmaps (e.g. go async)
  virtual SRef<QIStream> OpenStream() const = 0;
  virtual void CloseStream(SRef<QIStream> stream) const = 0;

#if _ENABLE_TEXTURE_HEADERS
  bool InitFromHeaderMng(PacLevelMem *mips, int maxMips);
#endif
};

#if (_ENABLE_REPORT || _ENABLE_BULDOZER)  
  #define TEXSRC_RELOAD 1
#else
  #define TEXSRC_RELOAD 0
#endif

/// load texture from a fil
class TextureSourcePac: public TextureSourcePacBase
{
private:
  #if TEXSRC_RELOAD
  mutable QFileTime _timestamp;
  #endif

public:
  TextureSourcePac() {}
  TextureSourcePac(const TextureSourcePac &src):TextureSourcePacBase(src)
  {
#if TEXSRC_RELOAD
    _timestamp = src._timestamp;
#endif
  }
  TextureSourcePac &operator=( const TextureSourcePac &src)
  {
    TextureSourcePacBase::operator=(src);
#if TEXSRC_RELOAD
    _timestamp = src._timestamp;
#endif
    return *this;
  }

protected:
  SRef<QIStream> OpenStream() const;
  void CloseStream(SRef<QIStream> stream) const;
  #if TEXSRC_RELOAD
  virtual QFileTime GetTimestamp() const {return _timestamp;}
  #else
  virtual QFileTime GetTimestamp() const {return 0;}
  #endif
  virtual void FlushHandles();
};

class TextureSourcePacMem: public TextureSourcePacBase
{
private:
  SRef<QIStream> _stream;

public:
  TextureSourcePacMem(QIStream *stream) {_stream = stream;}

protected:
  SRef<QIStream> OpenStream() const {return _stream ? _stream->Clone() : NULL;}
  void CloseStream(SRef<QIStream> stream) const {}
  // no timestamps
  virtual QFileTime GetTimestamp() const {return 0;}
  virtual void FlushHandles() {}
};

//! all routines required to create texture
class ITextureSourceFactory
{
  public:
  //! quick check if texture source can exist
  virtual bool Check(const char *name) = 0;
  //! preload any files as necessary so that Create will be quick.
  /*!
  In typical implementation data source asks file server to preload any files as necessary.
  Calling this function before Create is optional.
  */
  virtual bool PreInit(
    const char *name,
    RequestableObject *obj=NULL, RequestableObject::RequestContext *ctx=NULL
  ) = 0;
  //! create texture source
  virtual ITextureSource *Create(const char *name, PacLevelMem *mips, int maxMips) = 0;

  /// determine texture type based on name only, no file access
  virtual TextureType GetTextureType(const char *name) = 0;
  /// determine max. filter based on name only, no file access
  virtual TexFilter GetMaxFilter(const char *name) = 0;
};

/// texture containing a simple procedural texture
/**
Helper class for easier implementation of simple procedural textures
*/
class TextureSourceSimple: public ITextureSource
{
  protected:
  int _nMipmaps; //!< source file mipmap count
  int _w,_h; //!< texture dimensions
  PacFormat _format; //!< source file pixel format

  public:
  TextureSourceSimple(
    PacFormat format, int w, int h, int nMipmaps
  );
  ~TextureSourceSimple();

  bool Init(PacLevelMem *mips, int maxMips);

  int GetMipmapCount() const {return _nMipmaps;}
  PacFormat GetFormat() const {return _format;}

  //@{ functions which still need to be implemented
  bool GetMipmapData(void *mem, const PacLevelMem &mip, int level, int texSize) const = 0;
  PackedColor GetAverageColor() const = 0;
  bool IsAlpha() const = 0;
  //@}

  /// procedural data are always native
  bool IsLittleEndian() const {return IsPlatformLittleEndian();}

  bool RequestMipmapData(
    const PacLevelMem *mips, int levelMin, int levelMax, FileRequestPriority prior
  ) const
  {
    // data are always ready
    return true;
  }

  /// no dynamic range compression
  PackedColor GetMaxColor() const {return PackedColor(0xffffffff);}

  float GetMaxTexDetail(float maxTexDetail) const {return FLT_MAX;}
  
  // no timestamps
  virtual QFileTime GetTimestamp() const {return 0;}
  virtual void FlushHandles() {}

  //bool IsAlpha() const {return _color.A()<1.0f;}
  bool IsTransparent() const {return false;}
  /// hack used to fix textures that have alpha flag missing
  void ForceAlpha(){}
};

/// texture containing frame from video codec

class TextureSourceVideo: public ITextureSource
{
protected:
  int _nMipmaps;          //!< source file mipmap count
  int _w,_h;              //!< texture dimensions
  PacFormat _format;      //!< source file pixel format
  TextureType _tt;
  
  RStringB _name;
  mutable QFileTime _timestamp;
                                    
  mutable bool _textureCreated;
  mutable Ref<OggDecoder> _decoder;
public:
  TextureSourceVideo();
  TextureSourceVideo(  PacFormat format, int w, int h, int nMipmaps );
  ~TextureSourceVideo();

  OggDecoder* getDecoder() const {  return _decoder.GetRef();  };

  bool Init(PacLevelMem *mips, int maxMips);
  void SetName(const char *name);
  
  int GetMipmapCount() const {return _nMipmaps;}
  PacFormat GetFormat() const {return _format;}

  bool GetMipmapData(void *mem, const PacLevelMem &mip, int level, int texSize) const;
  PackedColor GetAverageColor() const;
  bool IsAlpha() const;
  bool IsAlphaNonOpaque() const;

  /// procedural data are always native
  bool IsLittleEndian() const {return IsPlatformLittleEndian();}

  bool RequestMipmapData(
    const PacLevelMem *mips, int levelMin, int levelMax, FileRequestPriority prior
    ) const
  {
    // data are always ready
    return true;
  }

  /// no dynamic range compression
  PackedColor GetMaxColor() const {return PackedColor(0xffffffff);}
  bool IgnoreLoadedLevels() const;
  float GetMaxTexDetail(float maxTexDetail) const {return FLT_MAX;}
  QFileTime GetTimestamp() const;
  void FlushHandles() {}

  //bool IsAlpha() const {return _color.A()<1.0f;}
  bool IsTransparent() const {return false;}
  /// hack used to fix textures that have alpha flag missing
  void ForceAlpha(){}
  /** no clamping needed */
  int GetClamp() const {return 0;}
  void SetClamp(int clampFlags) {}

  void SetTimestamp(RStringB name);
  TextureType GetTextureType() const {return _tt;};
  TexFilter GetMaxFilter() const {return TFAnizotropic16;}
  
};




/// texture header (item for TextureHeaderManager)
struct TextureHeader
{
  TextureHeader();
  /**
  \param name file name of the texture, that will be saved in the header information
  \param src header read from the texture
  \param numMipmaps number of mipmaps
  \param mipmaps array of mipmaps (must have size numMipmaps)
  */
  TextureHeader(const RString& name, const TextureSourcePac& src, int numMipmaps, PacLevelMem* mipmaps, int textureFileSize);

  TextureSourcePac _src;
  AutoArray<PacLevelMem> _mipmaps;

  /// if this header is for file in PBO (used for patching control), no need to save!
  bool _isFromPBO;
  /// file size of the original texture
  int _textureFileSize;
  /// if the header is usable (if size of real file is different from _textureFileSize or texture is patched, then it is not usable). no need to save!
  bool _isUsable;

  void SerializeBin(SerializeBinStream &f);
  const char *GetKey() const {return _src.GetName().GetKey();}
};

TypeIsMovable(TextureHeader);

/// class managing texture headers
class TextureHeaderManager
{
public:
  /// default file name for texture headers file
  static const char * const DefaultFileName;

  typedef MapStringToClass<TextureHeader, AutoArray<TextureHeader> > THeadersMap;

  /** adds header to manager
  \param name file name of the texture, that will be saved in the header information
  \param src header read from the texture
  \param numMipmaps number of mipmaps
  \param mipmaps array of mipmaps (must have size numMipmaps)
  \param textureFileSize
  */
  bool AddHeader(const RString& name, const TextureSourcePac& src, int numMipmaps, PacLevelMem* mipmaps, int textureFileSize);

  /** reads header from given texture and adds header to manager
  \param fileName file name of the texture on the disc
  \param textureName file name of the texture, that will be saved in the header information
  */
  bool AddHeaderFromFile(const RString& fileName, const RString& textureName);

  /// saves headers to given file
  bool SaveToFile(const RString& file);
  /** load headers from given file
  \param file file name
  \param append if we should append headers from file to already existing headers in the manager (otherwise manager will be cleared before adding)
  \partam isFromPBO if the file is from PBO
  \param prefix - prefix that will be added to the names of all textures
  */
  bool LoadFromFile(const RString& file, bool append, bool isFromPBO, const RString& prefix);

  /// returns if header with given name is in the manager
  bool HasHeader(const RString& fileName) const;
  /// returns header for given texture (or NULL if not found)
  const TextureHeader* GetHeader(const RString& fileName) const;

  /// returns number of headers
  size_t GetCount() const {return _headersMap.NItems();}

  /// tests if the headers are correct (test size and patching) and sets _isBad flag, if header should not be used
  bool CheckHeadersAreCorrect();
private:
  class TextureHeadersTestContext
  {
  public:
    TextureHeadersTestContext(): _numOK(0), _numFailed(0), _numPatched(0) {}
    int _numOK;
    int _numFailed;
    int _numPatched;
  };

  /// callback function for CheckHeadersSize
  static void CheckHeaderIsCorrect(TextureHeader &header, THeadersMap *headersMap, void *context);

  /// \param isFromPBO - needed only when loading - if the file we are loading is from PBO
  /// \param prefix - needed only when loading - prefix that will be added to the names of all textures
  void SerializeBin(SerializeBinStream &f, bool isFromPBO, const RString& prefix);

  /// map of the headers
  THeadersMap _headersMap;
};

#if _ENABLE_TEXTURE_HEADERS
extern TextureHeaderManager GTextureHeaderManager;
extern bool GUseTextureHeaders;
#endif


//! create texture source based on identifier (file name)
ITextureSourceFactory *SelectTextureSourceFactory(const char *name);

/// create single pixel colored ARGB8888 texture
ITextureSource *CreateColorTextureSource(ColorVal color);
//! Create random texture for alpha testing purposes
ITextureSource *CreateRandomTestTextureSource();
/// load Pac texture from a stream
ITextureSource *CreatePacMemTextureSource(QIStream *stream, RString name);
/// create a dithered pattern texture
ITextureSource *CreateDitherTextureSource(int dim, int min, int max);
/// create a perlin noise texture
ITextureSource *CreatePerlinNoise(int x, int y, int nMipmaps, float xScale, float yScale, float min, float max);

#if _HELISIM_RENDER_TO_TEXTURE
ITextureSource *CreateTextureSourceR2T(PacFormat format, int w, int h, int nMipmaps, RString name, float aspect);
#endif

#endif

