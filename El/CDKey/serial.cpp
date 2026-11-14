//

#include <El/elementpch.hpp>

#include "serial.hpp"
#include <El/QStream/qStream.hpp>
#include <RSA/rsa.h>
#include <ctype.h>

static const char *UsableChars = "0123456789ABCDEFGHJKLMNPRSTVWXYZ";

// decode 120 bit number into / from CD KEY

__forceinline void EraseMemory(unsigned char *buffer, int size)
{
  volatile unsigned char *erase = buffer;
  for(int i=0; i<size; i++)
  {
    erase = 0;
  }
}

/// convert CD key from ASCII string to internal representation
static bool DecodeMsg(unsigned char *msg, const char *buffer)
{
	for (int i=0; i<3; i++)
	{
		unsigned __int64 value = 0;
		int offset = 0;
		for (int k=0; k<8; k++)
		{
			int c = *buffer++;
			while (c && !isalnum((int)c)) c = *buffer++;
			if (!c) return false;
			c = toupper(c);
			// FIX: there are no letters O and I in serial number
			// if users enters it, it must be misread number
			if (c=='O') c='0';
			if (c=='I') c='1';
			const char *pos = strchr(UsableChars, c);
			if (!pos) return false;
			c = pos - UsableChars;
			unsigned __int64 cc = c;
			value |= cc << offset;
			offset += 5;
		}
		offset = 4 * 8;
		for (int j=0; j<5; j++)
		{
			*msg++ = (byte)((value >> offset) & 0xff);
			offset -= 8;
		}
	}
	return true;
}

/// convert CD key from internal representation to ASCII string
static void EncodeMsg(const unsigned char *msg, char *buffer)
{
  int outChar = 0;
  for (int i=0; i<3; i++)
  {
    // move 5 bytes (40 bits) to __int64
    unsigned __int64 value = 0;
    for (int j=0; j<5; j++)
    {
      value <<= 8;
      value |= *msg++;
    }

    // convert each 5 bits to char
    int offset = 0;
    for (int j=0; j<8; j++)
    {
      unsigned __int64 cc = (value >> offset) & 0x1f;
      offset += 5;
      char c = UsableChars[cc];

      if (outChar % 5 == 4) *buffer++ = '-';
      *buffer++ = c;
      outChar++;
    }
  }
  *buffer = 0;
}

// create / output MPI number from / into buffer

MPI CreateMPI(const unsigned char *buffer, DWORD size)
{
	MPI val = MPI_NULL;
	mpi_limb_t a;

	int nbytes = size;
	int nlimbs = (size + BYTES_PER_MPI_LIMB - 1) / BYTES_PER_MPI_LIMB;
	val = mpi_alloc(nlimbs);

	buffer += size - 1;

	int i = BYTES_PER_MPI_LIMB - nbytes % BYTES_PER_MPI_LIMB;
	i %= BYTES_PER_MPI_LIMB;
	val->nbits = nbytes * 8;
	int j = val->nlimbs = nlimbs;
	val->sign = 0;
	for( ; j > 0; j-- )
	{
		a = 0;
		for(; i < BYTES_PER_MPI_LIMB; i++)
		{
			a <<= 8;
			a |= *buffer--;
		}
		i = 0;
		val->d[j-1] = a;
	}
	return val;
}

__forceinline void EraseMPI(MPI &val, DWORD size)
{
	int nlimbs = (size + BYTES_PER_MPI_LIMB - 1) / BYTES_PER_MPI_LIMB;

	int j = nlimbs;
	for( ; j > 0; j-- )
	{
		val->d[j-1] = 0;
	}
}

void OutputMPI(MPI &val, unsigned char *buffer, DWORD size)
{
	mpi_limb_t a;

	int nbytes = size;
	int nlimbs = (size + BYTES_PER_MPI_LIMB - 1) / BYTES_PER_MPI_LIMB;

	buffer += size - 1;

	int i = BYTES_PER_MPI_LIMB - nbytes % BYTES_PER_MPI_LIMB;
	i %= BYTES_PER_MPI_LIMB;
	int j = nlimbs;
	for( ; j > 0; j-- )
	{
		a = val->d[j-1];
		int offset = (BYTES_PER_MPI_LIMB - i - 1) * 8;
		for(; i < BYTES_PER_MPI_LIMB; i++)
		{
			*buffer-- = (a >> offset) & 0xff;
			offset -= 8;
		}
		i = 0;
	}
}

void Encrypt(QOStrStream &out,const unsigned char *publicKey, int keySize)
{
	int msgSize = out.pcount();
	if (msgSize == 0) return;

	int nChunks = (msgSize + keySize - 1) / keySize;
	int newSize = nChunks * keySize;
	while (out.pcount() < newSize) out.put(' ');

	RSA_public_key key;
	// public exponent
	key.e = CreateMPI(publicKey, 4);
	// modulus
	key.n = CreateMPI(publicKey + 4, keySize);

	unsigned char *ptr = (unsigned char *)out.str();
	for (int i=0; i<nChunks; i++)
	{
		// prepare input
		MPI input = CreateMPI(ptr, keySize);

		// prepare output
		MPI output = mpi_alloc(mpi_get_nlimbs(key.n));

		// encode
		publ(output, input, &key);
		OutputMPI(output, ptr, keySize);

		m_free(input->d); m_free(input);
		m_free(output->d); m_free(output);

		ptr += keySize;
	}

	m_free(key.e->d); m_free(key.e);
	m_free(key.n->d); m_free(key.n);
}


CDKey::CDKey()
{
	memset(_key, 0, KEY_BYTES);
	memset(_message, 0, KEY_BYTES);
}

CDKey::~CDKey()
{
  // optimizer could remove the value cleanup as the value is not used any more
  // to prevent this, we need to mark it as volatile
  for (int i=0; i<sizeof(_key); i++) const_cast<volatile unsigned char *>(_key)[i] = 0;
  for (int i=0; i<sizeof(_message); i++) const_cast<volatile unsigned char *>(_message)[i] = 0;
}


bool CDKey::DecodeMsg(unsigned char *msg, const char *buffer)
{
	return ::DecodeMsg(msg, buffer);
}

void CDKey::Decrypt(
  unsigned char *buffer,
  const unsigned char *cdKey, const unsigned char *publicKey
)
{
  // hotfix - avoid usage of uninitialized key
  bool empty = true;
  for (int i=0; i<KEY_BYTES; i++)
  {
    if (cdKey[i]) {empty = false; break;}
  }
  if (empty)
  {
    for (int i=0; i<KEY_BYTES; i++) buffer[i] = 0;
    return;
  }

	RSA_public_key key;
	// public exponent
	key.e = CreateMPI(publicKey, 4);
	// modulus
	key.n = CreateMPI(publicKey + 4, KEY_BYTES);

	// prepare input
	MPI input = CreateMPI(cdKey, KEY_BYTES);

	// prepare output
	MPI output = mpi_alloc(mpi_get_nlimbs(key.n));

	// decode
	publ(output, input, &key);
	OutputMPI(output, buffer, KEY_BYTES);
  // secure all data
  EraseMPI(output, KEY_BYTES);
  EraseMPI(input, KEY_BYTES);
  EraseMPI(key.e, 4);
  EraseMPI(key.n, KEY_BYTES);

  m_free(input->d);	m_free(input);
	m_free(output->d); m_free(output);

	m_free(key.e->d); m_free(key.e);
	m_free(key.n->d); m_free(key.n);
}

void RSADecrypt( 
   unsigned char *outData, 
   const unsigned char *inpData, int inpDataSize,
   const unsigned char *publicKey, int keySize
)
{
  RSA_public_key key;
  // public exponent
  key.e = CreateMPI(publicKey, 4);
  // modulus
  key.n = CreateMPI(publicKey + 4, keySize);

  // prepare input
  MPI input = CreateMPI(inpData, inpDataSize);

  // prepare output
  MPI output = mpi_alloc(mpi_get_nlimbs(key.n));

  // decode
  publ(output, input, &key);
  OutputMPI(output, outData, keySize);
  // secure all data
  EraseMPI(output, keySize);
  EraseMPI(input, keySize);
  EraseMPI(key.e, 4);
  EraseMPI(key.n, keySize);

  m_free(input->d);	m_free(input);
  m_free(output->d); m_free(output);

  m_free(key.e->d); m_free(key.e);
  m_free(key.n->d); m_free(key.n);
}


void CDKey::Init(const unsigned char *cdKey, const unsigned char *publicKey)
{
  if (publicKey) memcpy(_key,publicKey,sizeof(_key));
  memcpy(_message,cdKey,sizeof(_message));
}

bool CDKey::Check(int offset, const char *with)
{
  unsigned char buffer[KEY_BYTES];
  Decrypt(buffer,_message,_key);
	bool ok = (memcmp(buffer + offset, with, strlen(with)) == 0);
  EraseMemory(buffer,KEY_BYTES);
  return ok;
}

__int64 CDKey::GetValue(int offset, int size)
{
  unsigned char buffer[KEY_BYTES];
  Decrypt(buffer,_message,_key);
	__int64 value = 0;
	const unsigned char *ptr = buffer + offset + size - 1;
	for (int i=0; i<size; i++)
	{
		value <<= 8;
		value |= *ptr--;
	}
  EraseMemory(buffer,KEY_BYTES);
	return value;
}

unsigned CDKey::GetValue32b(int offset, int size)
{
  unsigned char buffer[KEY_BYTES];
  Decrypt(buffer,_message,_key);
	unsigned value = 0;
	const unsigned char *ptr = buffer + offset + size - 1;
	for (int i=0; i<size; i++)
	{
		value <<= 8;
		value |= *ptr--;
	}
  EraseMemory(buffer,KEY_BYTES);
	return value;
}

void CDKey::GetTextMessage(char *buffer) const
{
  EncodeMsg(_message, buffer);
}
