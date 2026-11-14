#ifndef _SERIAL_HPP
#define _SERIAL_HPP

//#define SERIAL_KEY 7856
//#define SERIAL_KEY 5896
#define SERIAL_KEY 6985

inline unsigned long CheckCode(unsigned long inval)
{
	return (inval & 0xffff) ^ ((inval >> 12) & 0xffff) ^ ((inval >> 24) & 0xffff);
}

#define KEY_BITS						120
#define KEY_BYTES						KEY_BITS / 8

class CDKey
{
protected:
	unsigned char _buffer[KEY_BYTES];

public:
	CDKey();
	bool DecodeMsg(unsigned char *msg, const char *buffer);
  static void EncodeMsg(const unsigned char *msg, char *buffer);

	void Init(const unsigned char *cdKey, const unsigned char *publicKey);

	bool Check(int offset, const char *with);
	__int64 GetValue(int offset, int size);
};

#endif
