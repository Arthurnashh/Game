#ifndef _SERIAL_HPP
#define _SERIAL_HPP

/*
Obsolete very easy to hack serial key check.
//#define SERIAL_KEY 7856
//#define SERIAL_KEY 5896
#define SERIAL_KEY 6985
inline unsigned long CheckCode(unsigned long inval)
{
	return (inval & 0xffff) ^ ((inval >> 12) & 0xffff) ^ ((inval >> 24) & 0xffff);
}
*/

// when DECODE_ON_GET is non-zero, key is stored encrypted
// and is decrypted only when needed

#define DECODE_ON_GET 1

#define KEY_BITS						120
#define KEY_BYTES						KEY_BITS / 8

class CDKey
{
protected:
	unsigned char _key[KEY_BYTES+4];
	unsigned char _message[KEY_BYTES];

public:
	CDKey();
	bool DecodeMsg(unsigned char *msg, const char *buffer);

	void Decrypt(
    unsigned char *buffer,
    const unsigned char *cdKey, const unsigned char *publicKey
  );
	void Init(const unsigned char *cdKey, const unsigned char *publicKey);

	bool Check(int offset, const char *with);
  /// get value (64b)
	__int64 GetValue(int offset, int size);

  /// get value (no more than 32b needed)
	unsigned GetValue32b(int offset, int size);

  void GetTextMessage(char *buffer) const;
};

// outData computed by RSADecrypt are in little endian! (Probably should be reversed.)
void RSADecrypt(unsigned char *outData, const unsigned char *inpData, int inpDataSize, const unsigned char *publicKey, int keySize);

#endif
