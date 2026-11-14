// CD Key parameters

#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#include <El/CDKey/serial.hpp>
#include <Es/Strings/rString.hpp>
#include "KeyCheck.h"

//#define ConfigApp "Software\\Codemasters\\Operation Flashpoint"
//#define ConfigApp "Software\\Bohemia Interactive Studio\\FSMEditor"

const bool CDKeyRegistryUser = true;
const char CDKeyRegistryPath[] = "Software\\Bohemia Interactive Studio";
const unsigned char CDKeyPublicKey[] =
{
  0x11, 0x00, 0x00, 0x00,
    0x2F, 0xAD, 0xE2, 0xFA, 0x1A, 0xA2, 0x60, 0x95, 0xEB, 0x81, 0x76, 0xDB, 0xCC, 0xE3, 0x50
};
const char CDKeyFixed[] = "BIS Internal.";

const int CDKeyIdLength = (sizeof(CDKeyPublicKey) / sizeof(*CDKeyPublicKey) - 4) - strlen(CDKeyFixed);


bool KeyCheckClass::GetKeyFromRegistry(const char *path, unsigned char *cdkey, size_t &bufferSize)
{
  memset(cdkey, 0, KEY_BYTES);
  HKEY root = CDKeyRegistryUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
  HKEY key;
  bool succ;
  DWORD type;
  if (::RegOpenKeyEx ( root, path, 0, KEY_READ, &key ) != ERROR_SUCCESS ) {  bufferSize=0;return false;}
  succ=::RegQueryValueEx(key, "KEY", 0, &type, cdkey, reinterpret_cast<DWORD *>(&bufferSize))==ERROR_SUCCESS;
  ::RegCloseKey(key);
  return succ;
}

bool KeyCheckClass::VerifyKey()
{
  // no Win32 function may be called here
  // as it would make protection impossible

  unsigned char cdkey[KEY_BYTES];
  size_t sz=sizeof(cdkey);
  if (!GetKeyFromRegistry(CDKeyRegistryPath, cdkey,sz))
  {
    return false;
  }

  _cdKeyInstance->Init(cdkey, CDKeyPublicKey);

  return _cdKeyInstance->Check(CDKeyIdLength, CDKeyFixed);  //this function generates memory leaks
}


KeyCheckClass::KeyCheckClass()
{
  _cdKeyInstance=new CDKey;
}

KeyCheckClass::~KeyCheckClass()
{
  delete _cdKeyInstance;
}