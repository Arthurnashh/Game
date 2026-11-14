#if _ENABLE_COMPILED_SHADER_CACHE && !defined _RESISTANCE_SERVER && _ENABLE_DX10

#define D3D10CompileShader D3D10CompileShader_CachedT

HRESULT WINAPI D3D10CompileShader_CachedT(LPCSTR pSrcData,
                                          SIZE_T srcDataLen,
                                          LPCSTR pFileName,
                                          CONST D3D10_SHADER_MACRO *pDefines,
                                          LPD3D10INCLUDE pInclude,
                                          LPCSTR pFunctionName,
                                          LPCSTR pProfile,
                                          UINT flags,
                                          ID3D10Blob **ppShader,
                                          ID3D10Blob **ppErrorMsgs);

class ShaderCompileCacheT
{
  static void ShaderCacheInit(Array<const char *> &sources, const char *sectionName, RStringB customPath, bool forceCreate);
  static void ShaderCacheDone();
public:
  ShaderCompileCacheT(Array<const char *> &sources, const char *sectionName, RStringB customPath = RStringB(), bool forceCreate = false)
  {
    ShaderCacheInit(sources, sectionName, customPath, forceCreate);
  }

  ShaderCompileCacheT(const char *fpshader, const char *sectionName, RStringB customPath = RStringB(), bool forceCreate = false)
  {
    const char *sources=fpshader;
    ShaderCacheInit(Array<const char *>(sources), sectionName, customPath, forceCreate);
  }

  ~ShaderCompileCacheT()
  {
    ShaderCacheDone();
  }
};

#endif