
struct D3DXCompileShader_Result
{
  ComRef<ID3DXBuffer> shader;
  ComRef<ID3DXBuffer> errorMsgs;
};

struct LazyLoadFile
{
  virtual const char *Load(int &size) const = 0;
};

HRESULT WINAPI D3DXCompileShader_Cached(
  const LazyLoadFile &src, CONST D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude,
  LPCSTR pFunctionName, LPCSTR pProfile, DWORD Flags,
  D3DXCompileShader_Result &res
                                 );


class ShaderCompileCache
{
  static void ShaderCacheInit(const Array<RString> &sources, const char *sectionName, RStringB customPath, bool forceCreate);
  static void ShaderCacheDone();
public:
  ShaderCompileCache(const Array<RString> &sources, const char *sectionName, RStringB customPath = RStringB(), bool forceCreate = false)
  {
    ShaderCacheInit(sources, sectionName, customPath, forceCreate);
  }

  ShaderCompileCache(RString fpshader, const char *sectionName, RStringB customPath = RStringB(), bool forceCreate = false)
  {
    ShaderCacheInit(Array<RString>(fpshader), sectionName, customPath, forceCreate);
  }

  ~ShaderCompileCache()
  {
    ShaderCacheDone();
  }
};