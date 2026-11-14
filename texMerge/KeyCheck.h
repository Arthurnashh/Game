#ifndef _KEY_CHECK_H
#define _KEY_CHECK_H



class CDKey;

class KeyCheckClass
{
  CDKey *_cdKeyInstance;

private:
  KeyCheckClass(const KeyCheckClass &other);
  KeyCheckClass &operator=(const KeyCheckClass &other);
public:
  KeyCheckClass();
  ~KeyCheckClass();

  ///Verifies serial number
  bool VerifyKey();
  ///Gets key from registry
  /**
  @param keyPath Registry path
  @param outCDKey pointer to buffer, that receives key
  @param bufferSize Size of buffer, specify size of buffer in bytes. If buffer is 
    not large enough, function returns false, and bufferSize contains count of bytes needed.
  @return true, if no error, and key has been 
  */
  static bool GetKeyFromRegistry(const char *keyPath, unsigned char *outCDKey, size_t &bufferSize);

};
#endif
