#include "../wpch.hpp"
#if _ENABLE_COMPILED_SHADER_CACHE && !defined _RESISTANCE_SERVER && _ENABLE_DX10

//#include <malloc.h>
#include <El/QStream/qStream.hpp>
#include <El/QStream/QBStream.hpp>
#include <Es/Strings/bString.hpp>
#include "d3dDefsT.hpp"
#include <El/FileServer/fileServer.hpp>
#include "ShaderCompileCacheT.h"

///Change this value, if you want to invalidate cache from previous version
#define CACHE_ID "0003"



class ShaderCacheFileDescT
{
  QOFStream _outcache;
  QIFStreamB _incache;
  RString _sectionName;
  //! Custom path to place the file, temporary folder taken if not specified
  RString _customPath;

  bool _cached;

public:
  void Init(Array<const char *> &sources, const char *sectionName, RStringB customPath, bool forceCreate);

  RString GetCacheFilename();
  static bool PrepareValidTestBlock(Array<const char *> &sources, QOStrStream &testBlock);

  bool ReadFromCache(ID3D10Blob **buffer);
  bool WriteToCache(ID3D10Blob *buffer);
  bool IsCacheActive() {return _cached;}
  void DropCache()
  {
    _incache.close();
    _outcache.open(GetCacheFilename());
    _outcache.close();
    LogF("Inconsistent cache detected - cache has been invalidated.");
  }
  bool TestNextSection(Array<const char *> &sectionNames);
  bool CompareWithCache(QOStrStream &testBlock);
};

static SRef<ShaderCacheFileDescT> SCurrentShaderFileDesc;

RString ShaderCacheFileDescT::GetCacheFilename()
{
  RStringB path;
  if (_customPath.IsEmpty())
  {
    unsigned long needsz = GetTempPath(0,0);
    char *name = path.CreateBuffer(needsz);
    GetTempPath(needsz, name);
  }
  else
  {
    path = _customPath;
  }
  RString name = path + _sectionName + RStringB(".shdc");
  LogF("Path for cache of compiled shaders: %s", cc_cast(name));
  return name;
}

bool ShaderCacheFileDescT::PrepareValidTestBlock(Array<const char *> &sources, QOStrStream &testBlock)
{
  const char *header="BIShaderCache-id: "CACHE_ID;
  testBlock.write(header,strlen(header)+1);
  bool sourceFilesExist = true;
  for (int i=0;i<sources.Size();i++)
  {
    QFileTime ftm=QFBankQueryFunctions::TimeStamp(sources[i]);
    if (ftm == 0) sourceFilesExist = false;
    if (ftm&1) ftm++; // Suppress the first bit (it can vary depending on machine)
    testBlock.write(&ftm,sizeof(ftm));
  }
  unsigned long sdkx=10/*D3DX_VERSION*/;
  testBlock.write(&sdkx,sizeof(sdkx));
  unsigned long sdk=D3D10_SDK_VERSION;
  testBlock.write(&sdk,sizeof(sdk));
  unsigned long mark=0xAA55AA55;
  testBlock.write(&mark,sizeof(mark));
  return sourceFilesExist;
}

/*!
  \patch 5151 Date 3/29/2007 by Flyman
  - Fixed: Retail version doesn't try to compile shaders now unless being specifically asked to generate them
*/
void ShaderCacheFileDescT::Init(Array<const char *> &sources, const char *sectionName, RStringB customPath, bool forceCreate)
{
  _sectionName=sectionName;
  _customPath = customPath;
  QOStrStream testBlock;
  RString cachename=GetCacheFilename();
  bool sourceFilesExist = PrepareValidTestBlock(sources,testBlock);
  _incache.AutoOpen(cachename);

  // Determine whether we use retail version or not
#if _SUPER_RELEASE
  const bool retail = true;
#else
  const bool retail = false;
#endif

  // Enter the no-source code in 2 cases:
  // - sources are really not found
  // - retail version is being used and cache creation was not forced (that means we want to create the shader cache)
  if (!sourceFilesExist || (retail && !forceCreate))
  {
    DoAssert(!_incache.fail());
    int size=testBlock.pcount();
    char *cmpmem=(char *)alloca(size);
    int rd=_incache.read(cmpmem,size);
    if (rd != size) ErrorMessage("Shaders not valid (mismatch of exe and data?)");
    if (retail)
    {
      LogF("Using cache of compiled shaders. Retail version.");      
    }
    else
    {
      LogF("Using cache of compiled shaders. No sources present.");      
    }
    _cached=true;
  }
  else if (_incache.fail())
  {
    _cached=false;
    if (!customPath.IsEmpty()) CreatePath(customPath);
    _outcache.open(cachename);
    _outcache.write(testBlock.str(),testBlock.pcount());
    LogF("Creating cache of compiled shaders");
  }
  else
  {
    int size=testBlock.pcount();
    char *cmpmem=(char *)alloca(size);
    int rd=_incache.read(cmpmem,size);
    if (rd!=size || memcmp(cmpmem,testBlock.str(),size)!=0)
    {
      _cached=false;
      _incache.close();
      if (!customPath.IsEmpty()) CreatePath(customPath);
      _outcache.open(cachename);
      _outcache.write(testBlock.str(),testBlock.pcount());
      LogF("Note: Cache of compiled shaders was invalidated. Re-creating cache");
    }
    else
    {
      LogF("Using cache of compiled shaders. It will speed-up startup.");      
      _cached=true;
    }
  }
}

bool ShaderCacheFileDescT::TestNextSection(Array<const char *> &sectionNames)
{
  QOStrStream testBlock;
  for (int i=0;i<sectionNames.Size();i++)
  {
    testBlock.write(sectionNames[i],strlen(sectionNames[i])+1);
  }
  unsigned long magic=0x12345678;
  testBlock.write(&magic,sizeof(magic));
  int size=testBlock.pcount();
  const char *data=testBlock.str();
  if (IsCacheActive())
  {
    char *cmpmem=(char *)alloca(size);
    int rd=_incache.read(cmpmem,size);
    return rd==size && memcmp(cmpmem,data,size)==0;       
  }
  else
  {
    _outcache.write(data,size);
    return true;
  }
}


void ShaderCompileCacheT::ShaderCacheInit(Array<const char *> &sources, const char *sectionName, RStringB customPath, bool forceCreate)
{
  Log("ShaderCacheInit, was %p",SCurrentShaderFileDesc.GetRef());
  SCurrentShaderFileDesc=new ShaderCacheFileDescT;
  SCurrentShaderFileDesc->Init(sources, sectionName, customPath, forceCreate);
}

bool ShaderCacheFileDescT::ReadFromCache(ID3D10Blob **buffer)
{
  unsigned long sz;
  if (_incache.read(&sz,sizeof(sz))!=sizeof(sz)) return false;
  if (D3D10CreateBlob(sz,buffer)!=0) return false;
  if (_incache.read((*buffer)->GetBufferPointer(),sz)!=sz)
  {
    (*buffer)->Release();
    return false;
  }
  unsigned long magic;
  if (_incache.read(&magic,sizeof(magic))!=sizeof(magic) || magic!=0xAA00AA00)
  {
    (*buffer)->Release();
    return false;
  }

  return true;
}

bool ShaderCacheFileDescT::WriteToCache(ID3D10Blob *buffer)
{
  unsigned long sz = buffer->GetBufferSize();
  unsigned long magic=0xAA00AA00;
  _outcache.write(&sz,sizeof(sz));
  _outcache.write(buffer->GetBufferPointer(),sz);
  _outcache.write(&magic,sizeof(magic));
  return true;
}

void ShaderCompileCacheT::ShaderCacheDone()
{
  Log("ShaderCacheDone, was %p",SCurrentShaderFileDesc.GetRef());
  SCurrentShaderFileDesc=0;
}

#undef D3D10CompileShader
HRESULT WINAPI D3D10CompileShader_CachedT(LPCSTR pSrcData,
                                          SIZE_T srcDataLen,
                                          LPCSTR pFileName,
                                          CONST D3D10_SHADER_MACRO *pDefines,
                                          LPD3D10INCLUDE pInclude,
                                          LPCSTR pFunctionName,
                                          LPCSTR pProfile,
                                          UINT flags,
                                          ID3D10Blob **ppShader,
                                          ID3D10Blob **ppErrorMsgs)
{
  const char *sections[3];
  sections[0]="D3D10CompileShader";
  sections[1]=pFunctionName;
  sections[2]=pProfile;

  if (SCurrentShaderFileDesc.NotNull() && SCurrentShaderFileDesc->IsCacheActive())
  {
    if (SCurrentShaderFileDesc->TestNextSection(Array<const char *>(sections,lenof(sections))) && SCurrentShaderFileDesc->ReadFromCache(ppShader))
    {    
      Log("D3D10CompileShader: Cached %s",pFunctionName);
      ppErrorMsgs=0;
      return 0;
    }
    else
    {
      LogF("D3D10CompileShader: Cannot read from the cache. Cache dropped.");
      SCurrentShaderFileDesc->DropCache();
      SCurrentShaderFileDesc=0;
    }
  }

  HRESULT res=D3D10CompileShader(pSrcData, srcDataLen, pFileName, pDefines, pInclude, pFunctionName, pProfile, flags, ppShader, ppErrorMsgs);
  if (res==0)
  {
    //DWORD size = (*ppShader)->GetBufferSize();
    const DWORD *data = (const DWORD *)(*ppShader)->GetBufferPointer();
    //DWORD ssize = D3DXGetShaderSize(data);
    ComRef<ID3D10Blob> assembly;
    D3D10DisassembleShader(data, (*ppShader)->GetBufferSize(), false, pFunctionName, assembly.Init());
    const char *src = (const char *)assembly->GetBufferPointer();
    // disassembly ends with something like:
    // approximately 14 instruction slots used (1 texture, 13 arithmetic)
    static const char instrText[]="// Approximately ";
    const char *sizeStr = strstr(src,instrText);
    if (sizeStr)
    {
      sizeStr += sizeof(instrText)-1;
      BString<256> size;
      const char *endSize = strrchr(sizeStr,'\n');
      if (endSize)
      {
        strncpy(size,sizeStr,endSize-sizeStr);
        size[endSize-sizeStr] = 0;
      }
      else
      {
        strcpy(size,sizeStr);
      }
      LogF("D3D10CompileShader %s - %s",pFunctionName,cc_cast(size));
    }
    else
    {
      LogF("D3D10CompileShader %s - unable to parse disassembly");
    }

    if (SCurrentShaderFileDesc.NotNull() && !SCurrentShaderFileDesc->IsCacheActive())
    {
      SCurrentShaderFileDesc->TestNextSection(Array<const char *>(sections,lenof(sections)));
      SCurrentShaderFileDesc->WriteToCache(*ppShader);
    }
  }
  else
  {
    LogF("D3D10CompileShader %s failed (%x)",pFunctionName,res);
    if (pDefines)
    {
      LogF("Defines:");
      CONST D3D10_SHADER_MACRO *p = pDefines;
      while (p->Name != NULL)
      {
        LogF("%s - %s", p->Name, p->Definition);
        p++;
      }
    }
  }
  return res;
}
#endif