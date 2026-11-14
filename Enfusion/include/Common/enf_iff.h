//-----------------------------------------------------------------------------
// File: enf_iff.h
//
// Desc: IFF file format loader/writer
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_IFF_H
#define ENF_IFF_H

#include "enf_config.h"
#include "common/enf_inttypes.h"
#include "common/enf_streams.h"
#include "common/enf_mathlib.h"

namespace enf
{
/** @addtogroup Common
@{*/
class CInputStream;
class COutputStream;

class HunkID
{
public:
	static const uint	MAX_LENGTH = 4;

	HunkID()
	{
		memset(id, 0, sizeof(id));
	}

	HunkID(const char* name)
	{
		memcpy(id, name, sizeof(id));
	}

	HunkID operator=(const HunkID& other)
	{
		memcpy(id, other.id, sizeof(id));
		return *this;
	}

	bool operator==(const HunkID& other)
	{
		return (memcmp(id, other.id, sizeof(id)) == 0);
	}

	bool operator!=(const HunkID& other)
	{
		return (memcmp(id, other.id, sizeof(id)) != 0);
	}

	bool operator==(const char* hunkid)
	{
		return (memcmp(id, hunkid, sizeof(id)) == 0);
	}

	bool operator!=(const char* hunkid)
	{
		return (memcmp(id, hunkid, sizeof(id)) != 0);
	}

	char operator[](uint index)
	{
		enf_assert(index < MAX_LENGTH);
		return id[index];
	}

private:
	friend class IFF;
	friend class IFFInput;
	friend class IFFOutput;
	char id[MAX_LENGTH];
};

/*!
Interchange File Format wrapper. Uses
big endian for hunk lengths
*/
class PUBLIC_API IFF
{
protected:
	enum HunkType {HUNKTYPE_BIG = 1, HUNKTYPE_SMALL};

	struct Hunk
	{
		HunkID	ID;
		HunkType	Type;
		size_t	Position;
	};

	static const uint	MAX_HUNK_LEVELS = 16;
	Hunk	m_Hunks[MAX_HUNK_LEVELS];
	uint	m_iHunkLevel;
	bool	m_bOpen;

public:

	IFF();
	virtual ~IFF() {}

	bool IsOpen() const
	{
		return m_bOpen;
	}
	virtual void Seek(size_t len)=0;
	virtual size_t GetCurrentPosition() const=0;
	virtual void Close()=0;
};

class PUBLIC_API IFFInput: public IFF
{
protected:
	CInputStream*	m_pStream;
	size_t			m_iLength;

public:
	IFFInput():
		m_pStream(NULL),
		m_iLength(0)
	{
	}
	IFFInput(CInputStream* stream, HunkID& id);
	~IFFInput();

	CInputStream* GetStream() const
	{
		return m_pStream;
	}
	bool Open(CInputStream* stream, HunkID& id);
	void Close();

	/*!
	Utility method for IFF support
	Reads one big endian int
	*/
	int ReadInt();
	
	/*!
	Utility method for IFF support
	Reads one big endian short
	*/
	short	ReadShort();
	

	/*!
	Reads RAW data
	\param ptr
	Pointer to buffer
	\param len
	Length of data to be read
	\return
	Number of bytes read
	*/
	ENF_INLINE size_t Read(void *ptr, size_t len)
	{
		enf_assert(m_pStream != NULL);
		enf_assert2(m_iHunkLevel == 0 || (GetCurrentPosition() + len) <= m_Hunks[m_iHunkLevel - 1].Position, "Reading past the current hunk");
		return m_pStream->Read((unsigned char*)ptr, len);
	}
	/*!Reads one little endian int*/
	int ReadInt32();

	/*!Reads one little endian int*/
	uint ReadUInt32();

	/*!Reads one little endian short*/
	short ReadInt16();

	/*!Reads one little endian short*/
	ushort ReadUInt16();

	/*!Read one little endian float*/
	float ReadFloat();

	/*!Read one little endian AAB(6 floats)*/
	AAB ReadAAB();

	/*!Read one little endian Vector2(2 floats)*/
	Vector2 ReadVector2();

	/*!Read one little endian Vector3(3 floats)*/
	Vector3 ReadVector3();

	/*!Read one little endian Vector4(4 floats)*/
	Vector4 ReadVector4();

	/*!Reads string*/
	uint ReadString(char* buffer, uint maxlength);

	//!Reads string
	uint ReadString(DString& str);

	/*!
	Seeks at position in stream
	\param position
	Position in stream
	*/
	void Seek(size_t position);
	void Skip(size_t position);
	
	/*!
	Returns current position in stream
	\return
	Position in stream
	*/
	size_t GetCurrentPosition() const;

	//!Begins reading a hunk. Returns length of data, or -1, when we are at the end of hunk.
	size_t BeginHunk(HunkID& hunkid);
	//!End reading. Force position on end, otherwise asserts when we are not on the end. Returns true/false if we were at the end
	bool EndHunk(bool force = false);
	//!Are we at the end of scoped hunk?
	bool EndOfHunk() const;

	uint LoadHunk(void *ptr);
	uint LoadHunk(void **ptr, size_t *len);
	void UnloadHunk(void* ptr);
};

class PUBLIC_API IFFOutput: public IFF
{
protected:
	COutputStream*	m_pStream;

public:

	IFFOutput():
		m_pStream(NULL)
	{
	}
	IFFOutput(COutputStream* stream, const HunkID& id);
	~IFFOutput();

	COutputStream* GetStream() const
	{
		return m_pStream;
	}
	bool Create(COutputStream* stream, const HunkID& id);
	void Close();

	ENF_INLINE void Write(const void *ptr, size_t len)
	{
		enf_assert(m_pStream != NULL);
		m_pStream->Write((const unsigned char*)ptr, len);
	}

	void Write(int n);
	void Write(short n);

	void Seek(size_t len);
	size_t GetCurrentPosition() const;

	/*!Writes zero terminated string*/
	void WriteString(const char *ptr);

	/*!Writes one little endian int*/
	void WriteInt32(const int& value);

	/*!Writes one little endian int*/
	void WriteUInt32(const uint& value);

	/*!Writes one little endian short*/
	void WriteInt16(const short& value);

	/*!Writes one little endian short*/
	void WriteUInt16(const ushort& value);

	/*!Writes one little endian float*/
	void WriteFloat(const float& value);

	/*!Writes one little endian AAB(6 floats)*/
	void WriteAAB(const AAB& value);

	/*!Writes one little endian Vector2(2 floats)*/
	void WriteVector2(const Vector2& value);

	/*!Writes one little endian Vector3(3 floats)*/
	void WriteVector3(const Vector3& value);

	/*!Writes one little endian Vector4(4 floats)*/
	void WriteVector4(const Vector4& value);


	uint BeginHunk(const HunkID& id);
	uint BeginSubHunk(const HunkID& id);
	bool EndHunk();
};
//@}
}
#endif //ENF_IFF_H
