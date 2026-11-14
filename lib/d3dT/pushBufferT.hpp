#ifndef _pushBufferT_hpp_
#define _pushBufferT_hpp_

#if !defined _RESISTANCE_SERVER && _ENABLE_DX10

#include "d3dDefsT.hpp"
#include "../engine.hpp"
#include "ShaderSources/common.h"

//! Vertex shader constant definition
#define VCS_CDEFS(type,name,regnum) const int name = regnum;
#define VCS_CDEFA(type,name,regnum,dim) VCS_CDEFS(type,name,regnum)
VSC_LIST(VCS_CDEFS,VCS_CDEFA,VCS_CDEFS)

//! Pixel shader constant definition
#define PSC_CDEFS(type,name,regnum) const int name = regnum;
#define PSC_CDEFA(type,name,regnum,dim) const int name = regnum;
PSC_LIST(PSC_CDEFS,PSC_CDEFA)

//! Declaration to help to avoid of including the engddT.hpp
class EngineDDT;
class TextureD3DT;

//! Abstract class representing one PB item
class PBItemT : public RefCount
{
protected:
  //! Engine
  static EngineDDT *_engine;
public:
  //! Setting of the engine pointer
  static void SetEngine(EngineDDT *engine) {_engine = engine;}
  //! Executes one PB item
  virtual void Do(const PBContext &ctx) const = NULL;
  //! Returns index of the next item in the pushbuffer
  virtual int Next() const = NULL;
  //! Function to retrieve the size of this PushBuffer item in bytes
  virtual size_t GetBufferSize() const = NULL;
};

//////////////////////////////////////////////////////////////////////////

//! Wrapping of the SetVertexShaderConstant command
class PBISetVertexShaderConstantT : public PBItemT
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  float _data[4];
public:
  //! Constructor
  PBISetVertexShaderConstantT(int startRegister, const float *data)
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
class PBISetVertexShaderConstant2T : public PBItemT
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  float _data[2][4];
public:
  //! Constructor
  PBISetVertexShaderConstant2T(int startRegister, const float *data)
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
class PBISetVertexShaderConstantBT : public PBItemT
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  bool _data;
public:
  //! Constructor
  PBISetVertexShaderConstantBT(int startRegister, const bool *data)
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
class PBISetVertexShaderConstantB2T : public PBItemT
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  bool _data[2];
public:
  //! Constructor
  PBISetVertexShaderConstantB2T(int startRegister, const bool *data)
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
class PBISetVertexShaderConstantIT : public PBItemT
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  int _data[4];
public:
  //! Constructor
  PBISetVertexShaderConstantIT(int startRegister, const int *data)
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

//! Setting of the MaxColor via vertex shader constant
class PBISetVertexShaderConstantMaxColorT : public PBItemT
{
  bool _texture0MaxColorSeparate;
public:
  //! Constructor
  PBISetVertexShaderConstantMaxColorT(bool texture0MaxColorSeparate)
  {
    _texture0MaxColorSeparate = texture0MaxColorSeparate;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of the VS constants related to ML_Sun
class PBISetVertexShaderConstantMLSunT : public PBItemT
{
  Color _matAmbient;
  Color _matDiffuse;
  Color _matForcedDiffuse;
  Color _matSpecular;
  Color _matEmmisive;
  bool _vsIsAS;
public:
  //! Constructor
  PBISetVertexShaderConstantMLSunT(Color matAmbient, Color matDiffuse, Color matForcedDiffuse, Color matSpecular, Color matEmmisive, bool vsIsAS)
  {
    _matAmbient = matAmbient;
    _matDiffuse = matDiffuse;
    _matForcedDiffuse = matForcedDiffuse;
    _matSpecular = matSpecular;
    _matEmmisive = matEmmisive;
    _vsIsAS = vsIsAS;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of the VS constants related to ML_None
class PBISetVertexShaderConstantMLNoneT : public PBItemT
{
  Color _matEmmisive;
  bool _vsIsAS;
public:
  //! Constructor
  PBISetVertexShaderConstantMLNoneT(Color matEmmisive, bool vsIsAS)
  {
    _matEmmisive = matEmmisive;
    _vsIsAS = vsIsAS;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of the VS constant VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart
class PBISetVertexShaderConstantSAFRT : public PBItemT
{
  float _matPower;
  Color _matAmbient;
public:
  //! Constructor
  PBISetVertexShaderConstantSAFRT(float matPower, Color matAmbient)
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

//! Setting of the VS constant C_FREE_FREE_AFOGEND_RCPAFOGENDMINUSAFOGSTART
class PBISetVertexShaderConstantFFART : public PBItemT
{
public:
  //! Constructor
  PBISetVertexShaderConstantFFART() {}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of the VS constant VSC_LWSMatrix
class PBISetVertexShaderConstantLWSMT : public PBItemT
{
public:
  //! Constructor
  PBISetVertexShaderConstantLWSMT() {}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Wrapping of the SetRenderState command
class PBISetRenderStateT : public PBItemT
{
private:
  //! Type of the render state
  D3DRENDERSTATETYPE_OLD _state;
  //! Data to be set
  DWORD _data;
public:
  //! Constructor
  PBISetRenderStateT(D3DRENDERSTATETYPE_OLD state, DWORD data)
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
class PBISetSamplerStateT : public PBItemT
{
private:
  //! Sampler stage
  DWORD _stage;
  //! Sampler state
  D3DSAMPLERSTATETYPE_OLD _state;
  //! Data to be set
  DWORD _value;
public:
  //! Constructor
  PBISetSamplerStateT(DWORD stage, D3DSAMPLERSTATETYPE_OLD state, DWORD value)
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
class PBISetTexture0T : public PBItemT
{
private:
  //! Texture
  const TextureD3DT *_texture;
public:
  //! Constructor
  PBISetTexture0T(const TextureD3DT *texture)
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
class PBISetTextureT : public PBItemT
{
private:
  //! Sampler stage
  DWORD _stage;
  //! Texture
  const TextureD3DT *_texture;
public:
  //! Constructor
  PBISetTextureT(DWORD stage, const TextureD3DT *texture)
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

//! Setting of pixel shader
class PBISetPixelShaderT : public PBItemT
{
private:
  // Pixel shader
  ComRef<ID3D10PixelShader> _ps;
public:
  //! Constructor
  PBISetPixelShaderT(const ComRef<ID3D10PixelShader> &ps) {_ps = ps;}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);} 
};

//! Wrapping of the SetPixelShaderConstant command
class PBISetPixelShaderConstantT : public PBItemT
{
private:
  //! Index of the constant register
  int _startRegister;
  //! Data to be set
  float _data[4];
public:
  //! Constructor
  PBISetPixelShaderConstantT(int startRegister, const float *data)
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
class PBISetPixelShaderConstantDBST : public PBItemT
{
  bool _texture0MaxColorSeparate;
  Color _matDiffuse;
  Color _matForcedDiffuse;
  Color _matSpecular;
  float _matPower;
public:
  //! Constructor
  PBISetPixelShaderConstantDBST(bool texture0MaxColorSeparate, ColorVal matDiffuse, ColorVal matForcedDiffuse, ColorVal matSpecular, float matPower)
  {
    _texture0MaxColorSeparate = texture0MaxColorSeparate;
    _matDiffuse = matDiffuse;
    _matForcedDiffuse = matForcedDiffuse;
    _matSpecular = matSpecular;
    _matPower = matPower;
  }
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of Diffuse, diffuseBack and Specular pixel shader constants to zero
class PBISetPixelShaderConstantDBSZeroT : public PBItemT
{
public:
  //! Constructor
  PBISetPixelShaderConstantDBSZeroT() {}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of A_DFORCED_E PS constant with sun
class PBISetPixelShaderConstant_A_DFORCED_E_SunT : public PBItemT
{
  bool _texture0MaxColorSeparate;
  Color _matEmmisive;
  Color _matAmbient;
  Color _matForcedDiffuse;
public:
  //! Constructor
  PBISetPixelShaderConstant_A_DFORCED_E_SunT(bool texture0MaxColorSeparate, ColorVal matEmmisive, ColorVal matAmbient, ColorVal matForcedDiffuse)
  {
    _texture0MaxColorSeparate = texture0MaxColorSeparate;
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
class PBISetPixelShaderConstant_A_DFORCED_E_NoSunT : public PBItemT
{
  Color _matEmmisive;
public:
  //! Constructor
  PBISetPixelShaderConstant_A_DFORCED_E_NoSunT(ColorVal matEmmisive)
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

//! Setting of water PS constants
class PBISetPixelShaderConstantWaterT : public PBItemT
{
public:
  //! Constructor
  PBISetPixelShaderConstantWaterT() {}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of water simple PS constants
class PBISetPixelShaderConstantWaterSimpleT : public PBItemT
{
public:
  //! Constructor
  PBISetPixelShaderConstantWaterSimpleT() {}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of glass PS constants
class PBISetPixelShaderConstantGlassT : public PBItemT
{
  Color _matSpecular;
public:
  //! Constructor
  PBISetPixelShaderConstantGlassT(ColorVal matSpecular) {_matSpecular = matSpecular;}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of vertex shader
class PBISetVertexShaderT : public PBItemT
{
  VertexShaderInfo _vs;
public:
  //! Constructor
  PBISetVertexShaderT(const VertexShaderInfo &vs) {_vs = vs;}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Setting of vertex declaration
class PBISetVertexDeclarationT : public PBItemT
{
  ComRef<ID3D10InputLayout> _decl;
public:
  //! Constructor
  PBISetVertexDeclarationT(const ComRef<ID3D10InputLayout> &decl) {_decl = decl;}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Drawing
class PBIDrawIndexedPrimitiveT : public PBItemT
{
  int _beg;
  int _end;
public:
  //! Constructor
  PBIDrawIndexedPrimitiveT(int beg, int end)
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
class PBISetBiasT : public PBItemT
{
  int _bias;
public:
  //! Constructor
  PBISetBiasT(int bias) {_bias = bias;}
  //! Virtual method
  virtual void Do(const PBContext &ctx) const;
  //! Virtual method
  virtual int Next() const {return 1;}
  //! Virtual method
  virtual size_t GetBufferSize() const {return sizeof(*this);}
};

//! Pushbuffer for the D3DT engine
class PushBufferD3DT : public PushBuffer
{
private:
  //! List of pushbuffer items
  RefArray<PBItemT> _pbItemList;
public:
  //! Constructor
  PushBufferD3DT(int key);
  //! Virtual method
  virtual void Draw(const PBContext &ctx) const;
  //! _pbItemList wrapper
  RefArray<PBItemT> *GetPBItemList() {return &_pbItemList;}
  //! Virtual method
  virtual size_t GetBufferSize() const;
};

#endif

#endif