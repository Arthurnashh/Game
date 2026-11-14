#include "../wpch.hpp"

#if _ENABLE_VBS && _ENABLE_ADDONS
#include <El/QStream/qbStream.hpp>
#include <Es/Common/win.h>

#include <MARX/mpi.h>
#pragma comment(lib,"mpiwin32")

class FilebankEncryptionVBS1: public IFilebankEncryption
{
protected:
	static BYTE _key[64];
	static bool _keyPrepared;

public:
	FilebankEncryptionVBS1(const void *context = "");
	~FilebankEncryptionVBS1();

	bool Decode( char *dst, long lensb, QIStream &in );
	void Encode(QOStream &out, QIStream &in);
};

bool FilebankEncryptionVBS1::_keyPrepared = false;
BYTE FilebankEncryptionVBS1::_key[64];

static DWORD OpenDevice(bool network, int passwordBase)
{
	DWORD reserved = 0, result = 0;

	BYTE hwType[20];
	strcpy((char *)hwType, "CBU");
	BYTE port[30];

	if (network)
	{
		BYTE server[MPI_MAX_SERVER_NAME];
		strcpy((char *)server, "*");

		result = MPI_SetNetworkParameter((BYTE *)"SERVER", server);
		if (result != MPI_SUCCESS) return 0;

		strcpy((char *)port, "AUTONET");
	}
	else
	{
		strcpy((char *)port, "USB");
	}
	
	result = MPI_SetChannel(hwType, port, &reserved);
	if (result != MPI_SUCCESS) return 0;

	result = MPI_SubmitPassword((BYTE *)"PASSWORD_ID1", (BYTE *)"READ", passwordBase * 13);
	if (result != MPI_SUCCESS) return 0;
	
	DWORD handle = 0;
	result = MPI_SearchFirst(hwType, port, &handle);
	while (result == MPI_SUCCESS)
	{
		result = MPI_Open(handle);
		if (result == MPI_SUCCESS)
		{
			return handle;
		}
		
		result = MPI_SearchNext(hwType, port, &handle);
	}

	MPI_ErasePassword((BYTE *)"ERASE_ALL");
	return 0;
}

static void FillKey(DWORD *key, int passwordBase, int context)
{
	key[0] = 'VBS1';
	key[1] = '****';
	key[2] = 'marx';
	key[3] = '****';
	DWORD result = MPI_ReadID(1, &key[1]);

	
	if (context != 0)
	{
		key[3] = context;
	}
	else
	{
		// false try to read from device memory - send wrong password
		result = MPI_SubmitPassword((BYTE *)"PASSWORD_MEM1", (BYTE *)"WRITE", passwordBase * 17 - 3712);
		if (result == MPI_SUCCESS)
		{
			DWORD dummy = 128;
			result = MPI_ReadMem(MPI_MEMORY_NR1, 32, sizeof(dummy), (char *)&dummy);
			// MPI_ERR_PASSWORD (== 0x1022) returned
		}
		MPI_ErasePassword((BYTE *)"PASSWORD_MEM1");

		result = MPI_SubmitPassword((BYTE *)"PASSWORD_ID2", (BYTE *)"READ", 11 * passwordBase + (result >> 1) - 805);
		if (result == MPI_SUCCESS)
		{
			result = MPI_ReadID(2, &key[3]);
			MPI_ErasePassword((BYTE *)"PASSWORD_ID2");
		}
	}
}

#define H_PI ( 3.14159265358979323846f )

static float GetPasswordBase()
{
	static float base1 = 0;
	if (base1 == 0)
	{
		for (int i=0; i<8; i++) base1 = 10 * base1 + 1;
		base1 += 624000;
		base1 *= H_PI;
	}
	return base1;
}

FilebankEncryptionVBS1::FilebankEncryptionVBS1(const void *context)
{
	if (_keyPrepared) return;

	// false try to access device (no password submited)
	MPI_ErasePassword((BYTE *)"ERASE_ALL");
	BYTE hwType[20];
	strcpy((char *)hwType, "CBU");
	BYTE port[30];
	strcpy((char *)port, "AUTO");
	DWORD reserved = 256;
	DWORD result = MPI_SetChannel(hwType, port, &reserved);
	DWORD handle = 0;
	result = MPI_SearchFirst(hwType, port, &handle);
	// MPI_ERR_PASSWORD (== 0x1022) returned

	int passwordBase = toLargeInt(GetPasswordBase()) - (result >> 2) - 55;
	
	// initialize CRYPTO-BOX key
	handle = OpenDevice(false, passwordBase << 2);
	// if (handle == 0) handle = OpenDevice(true, passwordBase << 2);
	if (handle == 0) for (int i=0; i<15; i++)
	{
		handle = OpenDevice(true, passwordBase << 2);
		if (handle != 0) break;
		Sleep(2000);
	}
	if (handle == 0) return;

	// create key for Rijandel algorithm

	// create plain key
	DWORD key[4];
	FillKey(key, (passwordBase << 1) + 55000, context ? atoi((const char *)context) : 0);
	
	// encrypt key using RSA
	static BYTE RSAPublicKey[MPI_RSA_KEY_BUF_LEN(MPI_RSA_KEY_MODULUS_512)] =
	{
		0x00, 0x02, 0xfa, 0x6f, 0xc3, 0xa5, 0x9d, 0xbd, 0x07, 0x25, 0x58, 0xc3, 0x63, 0x24, 0x34, 0x25, 
		0xfe, 0x1b, 0xe9, 0xda, 0x87, 0x72, 0x68, 0xcc, 0xa7, 0xfb, 0x5c, 0xb3, 0xd3, 0x2c, 0xb6, 0x38, 
		0xcd, 0x43, 0x80, 0x7d, 0x0a, 0x8f, 0x33, 0xcb, 0x74, 0xfd, 0xed, 0xe7, 0x5f, 0x56, 0xf5, 0x93,
		0xa4, 0x74, 0x77, 0x85, 0x9a, 0x21, 0x38, 0xe4, 0x47, 0x4e, 0x0d, 0x57, 0x60, 0x48, 0x18, 0xf4, 
		0xc5, 0xe9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
		0x00, 0x01, 
	};
	result = MPI_SubmitEncryptionKey((BYTE *)"RSA_EXTERN_PUBLIC_KEY", sizeof(RSAPublicKey), RSAPublicKey, 0);
	if (result != MPI_SUCCESS) return;

	DWORD size = sizeof(_key);
	result = MPI_EncryptEx
	(
		(BYTE *)"RSA_ALGORITHM", (BYTE *)"RSA_EXTERN_PUBLIC_KEY", sizeof(key), (BYTE *)key, &size, _key
	);
	if (result != MPI_SUCCESS) return;

	MPI_EraseEncryptionKey((BYTE *)"RSA_EXTERN_PUBLIC_KEY");
	MPI_ErasePassword((BYTE *)"ERASE_ALL");
	MPI_Close(handle);

	_keyPrepared = true;
}

FilebankEncryptionVBS1::~FilebankEncryptionVBS1()
{
}

//#define ENCRYPTION_ALGORITHM (BYTE *)"RIJNDAEL_ALGORITHM"
//#define ENCRYPTION_KEY (BYTE *)"RIJNDAEL_SESSION_KEY"

#define ENCRYPTION_ALGORITHM (BYTE *)"RIJNDAEL_ALGORITHM"
#define ENCRYPTION_KEY (BYTE *)"RIJNDAEL_EXTERN_KEY"

bool FilebankEncryptionVBS1::Decode( char *dst, long lensb, QIStream &in )
{
	if (!_keyPrepared) return false;

	// decode 
	DWORD result = MPI_SubmitEncryptionKey(ENCRYPTION_KEY, 32, _key + 16, 0);
	if (result == MPI_SUCCESS)
	{
		DWORD size = lensb;
    AutoArray<char> buf;
    WriteAutoArrayChar writeBuf(buf);
    in.Process(writeBuf);

		result = MPI_DecryptEx
		(
			ENCRYPTION_ALGORITHM, ENCRYPTION_KEY, buf.Size(), (void *)buf.Data(), &size, dst
		);
		MPI_EraseEncryptionKey(ENCRYPTION_KEY);
	}
	return result == MPI_SUCCESS;
}


void FilebankEncryptionVBS1::Encode( QOStream &out, QIStream &in)
{
	if (!_keyPrepared) return;

	// copy data
	AutoArray<char> buffer;
  WriteAutoArrayChar writeBuffer(buffer);
  in.Process(writeBuffer);

	// encode
	DWORD result = MPI_SubmitEncryptionKey(ENCRYPTION_KEY, 32, _key + 16, 0);
	if (result != MPI_SUCCESS) return;	// !!! no data was writed

	result = MPI_EncryptEx
	(
		ENCRYPTION_ALGORITHM, ENCRYPTION_KEY, buffer.Size(), buffer.Data(), NULL, NULL
	);
	MPI_EraseEncryptionKey(ENCRYPTION_KEY);
	if (result != MPI_SUCCESS) return;	// !!! no data was writed

	out.write(buffer.Data(), buffer.Size());
}

IFilebankEncryption *CreateEncryptVBS1(const void *context)
{
	return new FilebankEncryptionVBS1(context);
}

#endif
