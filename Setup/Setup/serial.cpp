// CDKeys.cpp : Defines the entry point for the console application.
//

#include "wpch.hpp"
#include "serial.hpp"
#include "rsa\rsa.h"

#include <ctype.h>

static const char *UsableChars = "0123456789ABCDEFGHJKLMNPRSTVWXYZ";

// decode 120 bit number into / from CD KEY

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

static MPI CreateMPI(const unsigned char *buffer, DWORD size)
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

CDKey::CDKey()
{
	memset(_buffer, 0, KEY_BYTES);
}

bool CDKey::DecodeMsg(unsigned char *msg, const char *buffer)
{
	return ::DecodeMsg(msg, buffer);
}

void CDKey::EncodeMsg(const unsigned char *msg, char *buffer) 
{
  return ::EncodeMsg(msg, buffer);
}

void CDKey::Init(const unsigned char *cdKey, const unsigned char *publicKey)
{
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
	OutputMPI(output, _buffer, KEY_BYTES);

	m_free(input);
	m_free(output);

	m_free(key.e);
	m_free(key.n);
}

bool CDKey::Check(int offset, const char *with)
{
	return memcmp(_buffer + offset, with, strlen(with)) == 0;
}

__int64 CDKey::GetValue(int offset, int size)
{
	__int64 value = 0;
	unsigned char *ptr = _buffer + offset + size - 1;
	for (int i=0; i<size; i++)
	{
		value <<= 8;
		value |= *ptr--;
	}
	return value;
}
