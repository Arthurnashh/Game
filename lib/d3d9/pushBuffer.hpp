#ifndef _pushBuffer_hpp_
#define _pushBuffer_hpp_

#include "d3d9Defs.hpp"
#include "../engine.hpp"
#include "ShaderSources_3_0/common.h"

//! Vertex shader constant definition
#define VCS_CDEFS(type,name,regnum) const int name = regnum;
#define VCS_CDEFA(type,name,regnum,dim) VCS_CDEFS(type,name,regnum)
VSC_LIST(VCS_CDEFS,VCS_CDEFA,VCS_CDEFS)

//! Pixel shader constant definition
#define PSC_CDEFS(type,name,regtype,regnum) const int name = regnum;
#define PSC_CDEFA(type,name,regtype,regnum,dim) const int name = regnum;
PSC_LIST(PSC_CDEFS,PSC_CDEFA)

//! Declaration to help to avoid of including the engdd9.hpp
class EngineDD9;
class TextureD3D9;

//! Abstract class representing one PB item
class PBItem : public RefCount
{
protected:
  //! Engine
  static EngineDD9 *_engine;
public:
  //! Setting of the engine pointer
  static void SetEngine(EngineDD9 *engine) {_engine = engine;}
  //! Executes one PB item
  virtual void Do(const PBContext &ctx) const = NULL;
  //! Returns index of the next item in the pushbuffer
  virtual int Next() const = NULL;
  //! Function to retrieve the size of this PushBuffer item in bytes
  virtual size_t GetBufferSize() const = NULL;
};

//////////////////////////////////////////////////////////////////////////

//! Wrapping of the SetVertexShaderConstant command
class PBISetVertexShaderConstant : public PBItem
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  float _data[4];
public:
  //! Constructor
  PBISetVertexShaderConstant(int startRegister, const float *data)
  {
    _startRegister = startRegister;
    memcpy(_data, data, sizeof(_data));
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Wrapping of the SetVertexShaderConstant command
class PBISetVertexShaderConstant2 : public PBItem
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  float _data[2][4];
public:
  //! Constructor
  PBISetVertexShaderConstant2(int startRegister, const float *data)
  {
    _startRegister = startRegister;
    memcpy(_data, data, sizeof(_data));
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Wrapping of the SetVertexShaderConstantB command
class PBISetVertexShaderConstantB : public PBItem
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  bool _data;
public:
  //! Constructor
  PBISetVertexShaderConstantB(int startRegister, const bool *data)
  {
    _startRegister = startRegister;
    _data = *data;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Wrapping of the SetVertexShaderConstantB command
class PBISetVertexShaderConstantB2 : public PBItem
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  bool _data[2];
public:
  //! Constructor
  PBISetVertexShaderConstantB2(int startRegister, const bool *data)
  {
    _startRegister = startRegister;
    _data[0] = data[0];
    _data[1] = data[1];
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Wrapping of the SetVertexShaderConstantB command
class PBISetVertexShaderConstantI : public PBItem
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  int _data[4];
public:
  //! Constructor
  PBISetVertexShaderConstantI(int startRegister, const int *data)
  {
    _startRegister = startRegister;
    memcpy(_data, data, sizeof(_data));
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of the VS constants related to ML_Sun
class PBISetVertexShaderConstantMLSun : public PBItem
{
  Color _matDiffuse;
  Color _matForcedDiffuse;
  Color _matSpecular;
public:
  //! Constructor
  PBISetVertexShaderConstantMLSun(Color matDiffuse, Color matForcedDiffuse, Color matSpecular)
  {
    _matDiffuse = matDiffuse;
    _matForcedDiffuse = matForcedDiffuse;
    _matSpecular = matSpecular;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of the VS constants related to ML_None
class PBISetVertexShaderConstantMLNone : public PBItem
{
public:
  //! Constructor
  PBISetVertexShaderConstantMLNone() {}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of the VS constant VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart
class PBISetVertexShaderConstantSAFR : public PBItem
{
  float _matPower;
  Color _matAmbient;
public:
  //! Constructor
  PBISetVertexShaderConstantSAFR(float matPower, Color matAmbient)
  {
    _matPower = matPower;
    _matAmbient = matAmbient;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of the VS constant VSC_AE and VSC_GE
class PBISetVertexShaderConstantLightValuesR : public PBItem
{
  Color _matEmmisive;
  Color _matAmbient;
  Color _matForcedDiffuse;
public:
  //! Constructor
  PBISetVertexShaderConstantLightValuesR(Color matEmmisive, Color matAmbient, Color matForcedDiffuse)
  {
    _matEmmisive = matEmmisive;
    _matAmbient = matAmbient;
    _matForcedDiffuse = matForcedDiffuse;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of the VS constant C_FREE_FREE_AFOGEND_RCPAFOGENDMINUSAFOGSTART
class PBISetVertexShaderConstantFFAR : public PBItem
{
public:
  //! Constructor
  PBISetVertexShaderConstantFFAR() {}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of the VS constant VSC_LWSMatrix
class PBISetVertexShaderConstantLWSM : public PBItem
{
public:
  //! Constructor
  PBISetVertexShaderConstantLWSM() {}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Wrapping of the SetRenderState command
class PBISetRenderState : public PBItem
{
private:
  //! Type of the render state
  D3DRENDERSTATETYPE _state;
  //! Data to be set
  DWORD _data;
public:
  //! Constructor
  PBISetRenderState(D3DRENDERSTATETYPE state, DWORD data)
  {
    _state = state;
    _data = data;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Wrapping of the SetSamplerState command
class PBISetSamplerState : public PBItem
{
private:
  //! Sampler stage
  DWORD _stage;
  //! Sampler state
  D3DSAMPLERSTATETYPE _state;
  //! Data to be set
  DWORD _value;
public:
  //! Constructor
  PBISetSamplerState(DWORD stage, D3DSAMPLERSTATETYPE state, DWORD value)
  {
    _stage = stage;
    _state = state;
    _value = value;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Wrapping of the SetTexture command
class PBISetTexture0 : public PBItem
{
private:
  //! Texture
  const TextureD3D9 *_texture;
public:
  //! Constructor
  PBISetTexture0(const TextureD3D9 *texture)
  {
    _texture = texture;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Wrapping of the SetTexture command
class PBISetTexture : public PBItem
{
private:
  //! Sampler stage
  DWORD _stage;
  //! Texture
  const TextureD3D9 *_texture;
public:
  //! Constructor
  PBISetTexture(DWORD stage, const TextureD3D9 *texture)
  {
    _stage = stage;
    _texture = texture;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Wrapping of the SetTexture command
class PBISetTextureSB : public PBItem
{
private:
  //! This value will help to decide whether to use SSSM or SB
  bool _alphaPrimitives;
public:
  //! Constructor
  PBISetTextureSB(bool alphaPrimitives)
  {
    _alphaPrimitives = alphaPrimitives;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of pixel shader
class PBISetPixelShader : public PBItem
{
private:
  // Pixel shader
  ComRef<IDirect3DPixelShader9> _ps;
public:
  //! Constructor
  PBISetPixelShader(IDirect3DPixelShader9 *ps) {_ps += ps;}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);} 
};

//! Wrapping of the SetPixelShaderConstant command
class PBISetPixelShaderConstant : public PBItem
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  float _data[4];
public:
  //! Constructor
  PBISetPixelShaderConstant(int startRegister, const float *data)
  {
    _startRegister = startRegister;
    memcpy(_data, data, sizeof(_data));
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Wrapping of the SetPixelShaderConstant command
class PBISetPixelShaderConstantI : public PBItem
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  int _data[4];
public:
  //! Constructor
  PBISetPixelShaderConstantI(int startRegister, const int *data)
  {
    _startRegister = startRegister;
    memcpy(_data, data, sizeof(_data));
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of Diffuse, diffuseBack and Specular pixel shader constants
class PBISetPixelShaderConstantDBS : public PBItem
{
  Color _matDiffuse;
  Color _matForcedDiffuse;
  Color _matSpecular;
  float _matPower;
  bool _useGroundBidir;
  float _specularCoef;
public:
  //! Constructor
  PBISetPixelShaderConstantDBS(ColorVal matDiffuse, ColorVal matForcedDiffuse, ColorVal matSpecular, float matPower, bool useGroundBidir, bool useNewSpecular);
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of Diffuse, diffuseBack and Specular pixel shader constants to zero
class PBISetPixelShaderConstantDBSZero : public PBItem
{
public:
  //! Constructor
  PBISetPixelShaderConstantDBSZero() {}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of A_DFORCED_E PS constant with sun
class PBISetPixelShaderConstant_A_DFORCED_E_Sun : public PBItem
{
  Color _matEmmisive;
  Color _matAmbient;
  Color _matForcedDiffuse;
public:
  //! Constructor
  PBISetPixelShaderConstant_A_DFORCED_E_Sun(ColorVal matEmmisive, ColorVal matAmbient, ColorVal matForcedDiffuse)
  {
    _matEmmisive = matEmmisive;
    _matAmbient = matAmbient;
    _matForcedDiffuse = matForcedDiffuse;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of A_DFORCED_E PS constant without sun
class PBISetPixelShaderConstant_A_DFORCED_E_NoSun : public PBItem
{
  Color _matEmmisive;
public:
  //! Constructor
  PBISetPixelShaderConstant_A_DFORCED_E_NoSun(ColorVal matEmmisive)
  {
    _matEmmisive = matEmmisive;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

/// setting of terrain layer constants
class PBISetPixelShaderConstantLayers : public PBItem
{
  bool _layers[6];
  /// size of the height field (nopx) texture used in given layer
  float _heigthTexSizeLog2[6];

  public:  
  PBISetPixelShaderConstantLayers(int layerMask, const TexMaterial *mat);
  
  virtual void Do(const PBContext &ctx) const;
  virtual int Next() const {return 1;}
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of water PS constants
class PBISetPixelShaderConstantWater : public PBItem
{
public:
  PBISetPixelShaderConstantWater() {}
  virtual void Do(const PBContext &ctx) const;
  virtual int Next() const {return 1;}
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};


//! Setting of water simple PS constants
class PBISetPixelShaderConstantWaterSimple : public PBItem
{
public:
  //! Constructor
  PBISetPixelShaderConstantWaterSimple() {}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of glass PS constants
class PBISetPixelShaderConstantGlass : public PBItem
{
  Color _matSpecular;
public:
  //! Constructor
  PBISetPixelShaderConstantGlass(ColorVal matSpecular) {_matSpecular = matSpecular;}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of PSSuper constants
class PBISetPixelShaderConstantSuper : public PBItem
{
  Color _matSpecular;
public:
  //! Constructor
  PBISetPixelShaderConstantSuper(ColorVal matSpecular) {_matSpecular = matSpecular;}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of PSTreePRT constants
class PBISetPixelShaderConstantTreePRT : public PBItem
{
  Color _matEmmisive;
  Color _matAmbient;
  Color _matDiffuse;
public:
  //! Constructor
  PBISetPixelShaderConstantTreePRT(ColorVal matEmmisive, ColorVal matAmbient, ColorVal matDiffuse)
  {
    _matEmmisive = matEmmisive;
    _matAmbient = matAmbient;
    _matDiffuse = matDiffuse;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of vertex shader
class PBISetVertexShader : public PBItem
{
  ComRef<IDirect3DVertexShader9> _vs;
public:
  //! Constructor
  PBISetVertexShader(IDirect3DVertexShader9 *vs) {_vs += vs;}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of vertex shader
class PBISetVertexDeclaration : public PBItem
{
  ComRef<IDirect3DVertexDeclaration9> _decl;
public:
  //! Constructor
  PBISetVertexDeclaration(IDirect3DVertexDeclaration9 *decl) {_decl += decl;}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Drawing
class PBIDrawIndexedPrimitive : public PBItem
{
  int _beg;
  int _end;
public:
  //! Constructor
  PBIDrawIndexedPrimitive(int beg, int end)
  {
    _beg = beg;
    _end = end;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of bias
class PBISetBias : public PBItem
{
  int _bias;
public:
  //! Constructor
  PBISetBias(int bias) {_bias = bias;}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Pushbuffer for the D3D9 engine
class PushBufferD3D9 : public PushBuffer
{
private:
  //! List of pushbuffer items
  RefArray<PBItem> _pbItemList;
public:
  //! Constructor
  PushBufferD3D9(int key);
  //! Virtual method
  virtual void Draw(const PBContext &ctx) const;
  //! _pbItemList wrapper
  RefArray<PBItem> *GetPBItemList() {return &_pbItemList;}
  //! Virtual method
  virtual size_t GetBufferSize() const;
};

#endif