#include "Dx11ShaderCache.hpp"

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace
{
const uint32_t CacheVersion = 2;
const uint64_t MaximumCacheFileSize = 64ULL * 1024ULL * 1024ULL;
const size_t MaximumMacroCount = 4096;
volatile LONG CacheTemporaryFileSequence = 0;

#pragma pack(push, 1)
struct CacheHeader
{
  char magic[8];
  uint32_t version;
  uint32_t flags;
  uint32_t compilerVersion;
  uint32_t entryPointSize;
  uint32_t profileSize;
  uint32_t sourceNameSize;
  uint64_t sourceSize;
  uint64_t macroDataSize;
  uint64_t bytecodeSize;
  uint64_t keyHash;
  uint64_t bytecodeHash;
};
#pragma pack(pop)

uint64_t HashBytes(uint64_t hash, const void *data, size_t size)
{
  const unsigned char *bytes = static_cast<const unsigned char *>(data);
  for (size_t i = 0; i < size; ++i)
  {
    hash ^= bytes[i];
    hash *= 1099511628211ULL;
  }
  return hash;
}

uint64_t EntryKey(const char *source, SIZE_T sourceLength,
  const char *entryPoint, const char *profile, const char *sourceName,
  const std::string &macroData, UINT flags)
{
  uint64_t hash = 14695981039346656037ULL;
  hash = HashBytes(hash, &CacheVersion, sizeof(CacheVersion));
  const uint32_t compilerVersion = D3D_COMPILER_VERSION;
  hash = HashBytes(hash, &compilerVersion, sizeof(compilerVersion));
  hash = HashBytes(hash, &flags, sizeof(flags));
  hash = HashBytes(hash, entryPoint, strlen(entryPoint) + 1);
  hash = HashBytes(hash, profile, strlen(profile) + 1);
  if (sourceName)
    hash = HashBytes(hash, sourceName, strlen(sourceName) + 1);
  const uint8_t hasSourceName = sourceName ? 1 : 0;
  hash = HashBytes(hash, &hasSourceName, sizeof(hasSourceName));
  if (!macroData.empty())
    hash = HashBytes(hash, macroData.data(), macroData.size());
  return HashBytes(hash, source, sourceLength);
}

bool SerializeMacros(const D3D_SHADER_MACRO *defines, std::string &data)
{
  data.clear();
  if (!defines) return true;

  for (size_t i = 0; i < MaximumMacroCount; ++i)
  {
    const char *name = defines[i].Name;
    const char *definition = defines[i].Definition;
    if (!name && !definition) return true;
    if (!name) return false;
    const size_t nameLength = strlen(name);
    const size_t definitionLength = definition ? strlen(definition) : 0;
    if (nameLength > MaximumCacheFileSize ||
        definitionLength > MaximumCacheFileSize - nameLength ||
        nameLength + definitionLength + 2 > MaximumCacheFileSize - data.size())
      return false;
    data.append(name, nameLength + 1);
    if (definition)
      data.append(definition, definitionLength);
    data.push_back('\0');
  }
  return false; // The macro list must be terminated within the safety limit.
}

bool CacheDirectory(std::string &directory)
{
  char tempPath[MAX_PATH];
  DWORD length = GetTempPathA(MAX_PATH, tempPath);
  if (length == 0 || length >= MAX_PATH)
    return false;

  directory.assign(tempPath, length);
  if (!directory.empty() && directory[directory.size() - 1] != '\\')
    directory += '\\';
  directory += "DayZ-D3D11-ShaderCache-v2";
  if (!CreateDirectoryA(directory.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
    return false;
  return true;
}

bool MakeCachePath(const char *source, SIZE_T sourceLength,
  const char *entryPoint, const char *profile, const char *sourceName,
  const std::string &macroData, UINT flags,
  std::string &path, uint64_t &key)
{
  std::string directory;
  if (!CacheDirectory(directory))
    return false;
  key = EntryKey(source, sourceLength, entryPoint, profile, sourceName,
    macroData, flags);
  char fileName[32];
  _snprintf_s(fileName, sizeof(fileName), _TRUNCATE, "%016llx.bin",
    static_cast<unsigned long long>(key));
  path = directory + "\\" + fileName;
  return true;
}

bool ReadCache(const std::string &path, const char *source, SIZE_T sourceLength,
  const char *entryPoint, const char *profile, const char *sourceName,
  const std::string &macroData, UINT flags, uint64_t key,
  ID3DBlob **bytecode)
{
  std::ifstream input(path.c_str(), std::ios::binary);
  if (!input)
    return false;

  input.seekg(0, std::ios::end);
  const std::streamoff fileSize = input.tellg();
  input.seekg(0, std::ios::beg);
  if (fileSize < static_cast<std::streamoff>(sizeof(CacheHeader)) ||
      static_cast<uint64_t>(fileSize) > MaximumCacheFileSize)
    return false;

  CacheHeader header = {};
  input.read(reinterpret_cast<char *>(&header), sizeof(header));
  const size_t entrySize = strlen(entryPoint) + 1;
  const size_t profileSize = strlen(profile) + 1;
  const size_t sourceNameSize = sourceName ? strlen(sourceName) + 1 : 0;
  if (!input || memcmp(header.magic, "D11SHD02", 8) != 0 ||
      header.version != CacheVersion || header.flags != flags ||
      header.compilerVersion != D3D_COMPILER_VERSION ||
      header.sourceSize != static_cast<uint64_t>(sourceLength) ||
      header.entryPointSize != entrySize || header.profileSize != profileSize ||
      header.sourceNameSize != sourceNameSize ||
      header.macroDataSize != static_cast<uint64_t>(macroData.size()) ||
      header.keyHash != key || header.sourceSize > MaximumCacheFileSize ||
      header.bytecodeSize == 0 ||
      header.bytecodeSize > MaximumCacheFileSize)
    return false;

  const uint64_t expectedSize = sizeof(CacheHeader) + header.sourceSize +
    header.entryPointSize + header.profileSize + header.sourceNameSize +
    header.macroDataSize + header.bytecodeSize;
  if (expectedSize != static_cast<uint64_t>(fileSize))
    return false;

  std::vector<char> metadata;
  const uint64_t metadataSize = header.sourceSize + header.entryPointSize +
    header.profileSize + header.sourceNameSize + header.macroDataSize;
  if (metadataSize > MaximumCacheFileSize || metadataSize > static_cast<uint64_t>((std::numeric_limits<size_t>::max)()))
    return false;
  metadata.resize(static_cast<size_t>(metadataSize));
  if (!metadata.empty())
    input.read(&metadata[0], static_cast<std::streamsize>(metadata.size()));
  if (!input) return false;
  size_t offset = 0;
  if (sourceLength && memcmp(&metadata[offset], source, sourceLength) != 0)
    return false;
  offset += static_cast<size_t>(header.sourceSize);
  if (memcmp(&metadata[offset], entryPoint, entrySize) != 0)
    return false;
  offset += entrySize;
  if (memcmp(&metadata[offset], profile, profileSize) != 0)
    return false;
  offset += profileSize;
  if (sourceNameSize && memcmp(&metadata[offset], sourceName, sourceNameSize) != 0)
    return false;
  offset += sourceNameSize;
  if (!macroData.empty() && memcmp(&metadata[offset], macroData.data(), macroData.size()) != 0)
    return false;

  ID3DBlob *blob = NULL;
  if (FAILED(D3DCreateBlob(static_cast<SIZE_T>(header.bytecodeSize), &blob)))
    return false;
  input.read(static_cast<char *>(blob->GetBufferPointer()),
    static_cast<std::streamsize>(header.bytecodeSize));
  if (!input || HashBytes(14695981039346656037ULL, blob->GetBufferPointer(),
      static_cast<size_t>(header.bytecodeSize)) != header.bytecodeHash)
  {
    blob->Release();
    return false;
  }
  *bytecode = blob;
  return true;
}

void WriteCache(const std::string &path, const char *source, SIZE_T sourceLength,
  const char *entryPoint, const char *profile, const char *sourceName,
  const std::string &macroData, UINT flags, uint64_t key,
  ID3DBlob *bytecode)
{
  if (!bytecode || sourceLength > MaximumCacheFileSize ||
      bytecode->GetBufferSize() > MaximumCacheFileSize)
    return;

  CacheHeader header = {};
  memcpy(header.magic, "D11SHD02", 8);
  header.version = CacheVersion;
  header.flags = flags;
  header.compilerVersion = D3D_COMPILER_VERSION;
  header.entryPointSize = static_cast<uint32_t>(strlen(entryPoint) + 1);
  header.profileSize = static_cast<uint32_t>(strlen(profile) + 1);
  header.sourceNameSize = sourceName ? static_cast<uint32_t>(strlen(sourceName) + 1) : 0;
  header.sourceSize = static_cast<uint64_t>(sourceLength);
  header.macroDataSize = static_cast<uint64_t>(macroData.size());
  header.bytecodeSize = static_cast<uint64_t>(bytecode->GetBufferSize());
  header.keyHash = key;
  header.bytecodeHash = HashBytes(14695981039346656037ULL,
    bytecode->GetBufferPointer(), bytecode->GetBufferSize());

  char suffix[64];
  _snprintf_s(suffix, sizeof(suffix), _TRUNCATE, ".%lu.%lu.%ld.tmp",
    GetCurrentProcessId(), GetCurrentThreadId(),
    InterlockedIncrement(&CacheTemporaryFileSequence));
  const std::string temporaryPath = path + suffix;
  HANDLE file = CreateFileA(temporaryPath.c_str(), GENERIC_WRITE, 0, NULL,
    CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL);
  if (file == INVALID_HANDLE_VALUE)
    return;

  const char *entry = entryPoint;
  const char *target = profile;
  const char *sourceFile = sourceName;
  const void *parts[] = { &header, source, entry, target, sourceFile,
    macroData.empty() ? NULL : macroData.data(), bytecode->GetBufferPointer() };
  const uint64_t sizes[] = { sizeof(header), sourceLength, header.entryPointSize,
    header.profileSize, header.sourceNameSize, header.macroDataSize,
    header.bytecodeSize };
  bool written = true;
  for (size_t i = 0; i < sizeof(parts) / sizeof(parts[0]); ++i)
  {
    const char *cursor = static_cast<const char *>(parts[i]);
    uint64_t remaining = sizes[i];
    if (!remaining) continue;
    if (!cursor)
    {
      written = false;
      break;
    }
    while (remaining)
    {
      const DWORD chunk = remaining > 0x7ffff000ULL ? 0x7ffff000UL : static_cast<DWORD>(remaining);
      DWORD bytesWritten = 0;
      if (!WriteFile(file, cursor, chunk, &bytesWritten, NULL) || bytesWritten != chunk)
      {
        written = false;
        break;
      }
      cursor += bytesWritten;
      remaining -= bytesWritten;
    }
    if (!written) break;
  }
  if (written) written = FlushFileBuffers(file) != FALSE;
  CloseHandle(file);
  if (written)
    MoveFileExA(temporaryPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
  else
    DeleteFileA(temporaryPath.c_str());
}
}

HRESULT Dx11ShaderCache::Compile(const char *source, SIZE_T sourceLength,
  const char *entryPoint, const char *profile, UINT flags,
  ID3DBlob **bytecode, ID3DBlob **diagnostics,
  const D3D_SHADER_MACRO *defines, ID3DInclude *include,
  const char *sourceName)
{
  if (bytecode) *bytecode = NULL;
  if (diagnostics) *diagnostics = NULL;
  if (!source || !entryPoint || !profile || !bytecode)
    return E_INVALIDARG;

  std::string macroData;
  if (!SerializeMacros(defines, macroData))
    return E_INVALIDARG;

  std::string cachePath;
  uint64_t key = 0;
  // An arbitrary include handler can return mutable or context-dependent data.
  // Do not serve stale bytecode from the cache when includes are involved.
  if (!include && MakeCachePath(source, sourceLength, entryPoint, profile,
      sourceName, macroData, flags, cachePath, key) &&
      ReadCache(cachePath, source, sourceLength, entryPoint, profile,
        sourceName, macroData, flags, key, bytecode))
    return S_OK;

  ID3DBlob *compiled = NULL;
  ID3DBlob *messages = NULL;
  HRESULT hr = D3DCompile(source, sourceLength, sourceName, defines, include,
    entryPoint, profile, flags, 0, &compiled, &messages);
  if (diagnostics)
    *diagnostics = messages;
  else if (messages)
    messages->Release();

  if (FAILED(hr))
  {
    if (compiled) compiled->Release();
    return hr;
  }

  *bytecode = compiled;
  if (!cachePath.empty())
    WriteCache(cachePath, source, sourceLength, entryPoint, profile,
      sourceName, macroData, flags, key, compiled);
  return S_OK;
}
