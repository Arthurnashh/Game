#include "../wpch.hpp"

#if _ENABLE_VBS && _ENABLE_ADDONS
#include <El/QStream/qbStream.hpp>

class FilebankEncryptionXOR1024: public IFilebankEncryption
{
	char _password;

	public:
	FilebankEncryptionXOR1024(char password);
	bool Decode( char *dst, long lensb, QIStream &in );
	void Encode( QOStream &out, QIStream &in);
};

FilebankEncryptionXOR1024::FilebankEncryptionXOR1024(char password)
:_password(password)
{
}

bool FilebankEncryptionXOR1024::Decode( char *dst, long lensb, QIStream &in )
{
	AutoArray<char> buffer;
  WriteAutoArrayChar writeBuffer(buffer);
  in.Process(writeBuffer);

	// decode 
	memcpy(dst,buffer.Data(),buffer.Size());
	for (int i=0; i<lensb; i++)
	{
		dst[i] ^= _password+i;
	}
	return true;
}

void FilebankEncryptionXOR1024::Encode( QOStream &out, QIStream &in)
{
	AutoArray<char> buffer;
  WriteAutoArrayChar writeBuffer(buffer);
  in.Process(writeBuffer);

	int i;
	for (i=0; i<buffer.Size(); i++)
	{
		out.put(buffer[i]^(_password+i));
	}
	const int mask = 1024-1;
	while ((i&mask)!=0)
	{
		out.put(0);
		i++;
	}
}

IFilebankEncryption *CreateEncryptXOR1024(const void *context)
{
	// note: context may be anything
	// get key that is necessary 
	// for asymetric encryption private key need to be constructed now
	// we assume fixed key for XOR encryption
	char key = 'a';
	return new FilebankEncryptionXOR1024(key);
}
#endif
