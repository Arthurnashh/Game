//-----------------------------------------------------------------------------
// File: enf_iff.cpp
//
// Desc: IFF file format loader/writer
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "enf_main.h"
#include "system/enf_vfilesystem.h"
#include "common/enf_iff.h"
#include "common/enf_mathlib.h"
#include "common/enf_mathlib2d.h"

namespace enf
{
//-----------------------------------------------------------------------
IFF::IFF(): m_bOpen(false)
{
}

//-----------------------------------------------------------------------
IFFInput::~IFFInput()
{
	Close();
}

//-----------------------------------------------------------------------
size_t IFFInput::GetCurrentPosition() const
{
	enf_assert(m_pStream != NULL);
	return m_pStream->GetCurrentPosition();
}

//-----------------------------------------------------------------------
int IFFInput::ReadInt()
{
	unsigned int n;
	Read((unsigned char*)&n, sizeof(int));
	return BE_INT(n);
}

//-----------------------------------------------------------------------
short IFFInput::ReadShort()
{
	short n;
	Read((unsigned char*)&n, sizeof(short));

	return BE_SHORT(n);
}

//-----------------------------------------------------------------------
int IFFInput::ReadInt32()
{
	int n;
	Read((unsigned char*)&n, sizeof(int));
	return LE_INT(n);
}

//-----------------------------------------------------------------------
uint IFFInput::ReadString(char* buffer, uint maxlength)
{
	uint len = ReadInt32();
	if(len == 0xffffffff)
	{
		buffer[0] = 0;
		return 0;
	}
	Read(buffer, enf_min(len, maxlength));
	if(maxlength < len)
		m_pStream->Seek(m_pStream->GetCurrentPosition() + len - maxlength);
	return len - 1;
}

//-----------------------------------------------------------------------
uint IFFInput::ReadString(DString& str)
{
	str.Clear();

	uint len = ReadInt32();
	if(len == 0xffffffff)
	{
		return 0;
	}
	if(len)
	{
		len -= 1;

		str.Reserve(len);
		str.SetSize(len);

		Read((char*)str.cstr(), len);
	}

	// skip '\0' at the end of string
	m_pStream->Seek(m_pStream->GetCurrentPosition() + 1);

	return len;
}

//-----------------------------------------------------------------------
uint IFFInput::ReadUInt32()
{
	uint n;
	Read((unsigned char*)&n, sizeof(int));
	return LE_INT(n);
}

//-----------------------------------------------------------------------
short IFFInput::ReadInt16()
{
	short n;
	Read((unsigned char*)&n, sizeof(ushort));
	return LE_SHORT(n);
}

//-----------------------------------------------------------------------
ushort IFFInput::ReadUInt16()
{
	ushort n;
	Read((unsigned char*)&n, sizeof(ushort));
	return LE_SHORT(n);
}

//-----------------------------------------------------------------------
float IFFInput::ReadFloat()
{
	float n;
	Read((unsigned char*)&n, sizeof(float));
	return LE_FLOAT(n);
}

//-----------------------------------------------------------------------
Vector2 IFFInput::ReadVector2()
{
	float n[2];
	Read((unsigned char*)&n, sizeof(n));

	n[0] = LE_FLOAT(n[0]);
	n[1] = LE_FLOAT(n[1]);

	return Vector2(n);
}


//-----------------------------------------------------------------------
Vector3 IFFInput::ReadVector3()
{
	float n[3];
	Read((unsigned char*)&n, sizeof(n));

	return Vector3(LE_FLOAT(n[0]),
					LE_FLOAT(n[1]),
					LE_FLOAT(n[2]));
}

//-----------------------------------------------------------------------
Vector4 IFFInput::ReadVector4()
{
	float n[4];
	Read((unsigned char*)&n, sizeof(n));

	return Vector4(LE_FLOAT(n[0]),
		LE_FLOAT(n[1]),
		LE_FLOAT(n[2]),
		LE_FLOAT(n[3]));
}


//-----------------------------------------------------------------------
AAB IFFInput::ReadAAB()
{
	float aab[6];
	Read((unsigned char*)&aab, sizeof(aab));

	return AAB(
		LE_FLOAT(aab[0]),
		LE_FLOAT(aab[1]),
		LE_FLOAT(aab[2]),
		LE_FLOAT(aab[3]),
		LE_FLOAT(aab[4]),
		LE_FLOAT(aab[5]));
}



//-----------------------------------------------------------------------
void IFFInput::UnloadHunk(void *ptr)
{
	MemoryManager::Free(ptr);
}

//-----------------------------------------------------------------------
uint IFFInput::LoadHunk(void* ptr)
{
	enf_assert(m_pStream != NULL);
	if(m_iHunkLevel)
		return Read((unsigned char*)ptr, m_Hunks[m_iHunkLevel - 1].Position - GetCurrentPosition());
	else
		return 0;
}

//-----------------------------------------------------------------------
uint IFFInput::LoadHunk(void **ptr,size_t *len)
{
	enf_assert(m_pStream != NULL);

	size_t hunkLength;
	if(m_iHunkLevel)
		hunkLength = m_Hunks[m_iHunkLevel - 1].Position - GetCurrentPosition();
	else
		hunkLength = 0;

	if(hunkLength == 0)
		return 0;

	void* cptr = MemoryManager::Alloc(hunkLength, __FILE__, __LINE__);

	if(cptr == NULL)
		return 0;

	*ptr = cptr;
	*len = hunkLength;
	return Read((unsigned char*)cptr, hunkLength);
}

//-----------------------------------------------------------------------
void IFFInput::Close()
{
	if(m_pStream)
	{
		m_pStream->Close();
		m_pStream = NULL;
	}
	m_iHunkLevel = 0;
	m_bOpen = false;
}

//-----------------------------------------------------------------------
size_t IFFInput::BeginHunk(HunkID& hunkid)
{
	enf_assert(m_pStream != NULL);
	enf_assert(m_iHunkLevel < MAX_HUNK_LEVELS);

	if(m_iHunkLevel > 0)
	{
		if(m_pStream->GetCurrentPosition() >= m_Hunks[m_iHunkLevel - 1].Position)
			return -1;
	}

	if(m_pStream->Read((unsigned char*)&hunkid, sizeof(HunkID)) != sizeof(HunkID))
		return -1;

	size_t hunkLength = ReadInt();

	m_Hunks[m_iHunkLevel].ID = hunkid;
	m_Hunks[m_iHunkLevel].Position = GetCurrentPosition() + hunkLength;
	m_Hunks[m_iHunkLevel].Type = HUNKTYPE_BIG;
	m_iHunkLevel++;
	return hunkLength;
}

//-----------------------------------------------------------------------
bool IFFInput::EndHunk(bool force)
{
	enf_assert(m_iHunkLevel > 0);
	m_iHunkLevel--;
	if(force)
	{
		if(GetCurrentPosition() != m_Hunks[m_iHunkLevel].Position)
		{
			//ensure we are at the end of hunk
			m_pStream->Seek(m_Hunks[m_iHunkLevel].Position);
			return false;
		}
	}
	else
	{
		enf_assert(GetCurrentPosition() == m_Hunks[m_iHunkLevel].Position);
	}
	return true;
}

//-----------------------------------------------------------------------
bool IFFInput::EndOfHunk() const
{
	enf_assert(m_iHunkLevel > 0);
	return m_Hunks[m_iHunkLevel - 1].Position == GetCurrentPosition();
}

//-----------------------------------------------------------------------
IFFInput::IFFInput(CInputStream* stream, HunkID& id):
	m_pStream(NULL)
{
	m_bOpen = Open(stream, id);
}

//-----------------------------------------------------------------------
bool IFFInput::Open(CInputStream* stream, HunkID& id)
{
	HunkID head;

	Close();

	if((m_pStream = stream) == NULL)
		return false;

	if(!m_pStream->IsOpen())
		return false;

	if(m_pStream->Read((unsigned char*)&head, sizeof(head)) != sizeof(head))
		return false;

	if(head != "FORM")
		return false;

	m_iLength = ReadInt();

	m_pStream->Read(id.id, sizeof(id.id));
	
	m_bOpen = true;
	return true;
}

//-----------------------------------------------------------------------
void IFFInput::Seek(size_t len)
{
	enf_assert(m_pStream != NULL);
	m_pStream->Seek(len);
}

//-----------------------------------------------------------------------
void IFFInput::Skip(size_t len)
{
	enf_assert(m_pStream != NULL);
	m_pStream->Skip(len);
}

//-----------------------------------------------------------------------
IFFOutput::~IFFOutput()
{
	Close();
}

//-----------------------------------------------------------------------
size_t IFFOutput::GetCurrentPosition() const
{
	enf_assert(m_pStream != NULL);
	return m_pStream->GetCurrentPosition();
}

//-----------------------------------------------------------------------
void IFFOutput::Write(short n)
{
	enf_assert(m_pStream != NULL);

	n = BE_SHORT(n);
	m_pStream->Write((const unsigned char*)&n, sizeof(short));
}

//-----------------------------------------------------------------------
void IFFOutput::Write(int in)
{
	unsigned int n;

	enf_assert(m_pStream != NULL);

	n = BE_INT(in);
	m_pStream->Write((const unsigned char*)&n, sizeof(int));
}

//-----------------------------------------------------------------------
void IFFOutput::WriteInt32(const int& value)
{
	enf_assert(m_pStream != NULL);

	int n = LE_INT(value);

	m_pStream->Write((const unsigned char*)&n, sizeof(int));
}

//-----------------------------------------------------------------------
void IFFOutput::WriteUInt32(const uint& value)
{
	enf_assert(m_pStream != NULL);

	uint n = LE_INT(value);

	m_pStream->Write((const unsigned char*)&n, sizeof(uint));
}

//-----------------------------------------------------------------------
void IFFOutput::WriteInt16(const short& value)
{
	enf_assert(m_pStream != NULL);

	short n = LE_SHORT(value);

	m_pStream->Write((const unsigned char*)&n, sizeof(short));
}

//-----------------------------------------------------------------------
void IFFOutput::WriteUInt16(const ushort& value)
{
	enf_assert(m_pStream != NULL);

	ushort n = LE_SHORT(value);

	m_pStream->Write((const unsigned char*)&n, sizeof(ushort));
}

//-----------------------------------------------------------------------
void IFFOutput::WriteFloat(const float& value)
{
	enf_assert(m_pStream != NULL);

	float n = LE_FLOAT(value);

	m_pStream->Write((const unsigned char*)&n, sizeof(float));
}


//-----------------------------------------------------------------------
void IFFOutput::WriteVector2(const Vector2& value)
{
	enf_assert(m_pStream != NULL);

	Vector2 n(
		LE_FLOAT(value.x),
		LE_FLOAT(value.y));

	m_pStream->Write((const unsigned char*)&n, sizeof(Vector2));
}



//-----------------------------------------------------------------------
void IFFOutput::WriteVector3(const Vector3& value)
{
	enf_assert(m_pStream != NULL);

	Vector3 n(
		LE_FLOAT(value.x),
		LE_FLOAT(value.y),
		LE_FLOAT(value.z));

	m_pStream->Write((const unsigned char*)&n, sizeof(Vector3));
}


//-----------------------------------------------------------------------
void IFFOutput::WriteVector4(const Vector4& value)
{
	enf_assert(m_pStream != NULL);

	Vector4 n(
		LE_FLOAT(value.x),
		LE_FLOAT(value.y),
		LE_FLOAT(value.z),
		LE_FLOAT(value.w));

	m_pStream->Write((const unsigned char*)&n, sizeof(Vector4));
}


//-----------------------------------------------------------------------
void IFFOutput::WriteAAB(const AAB& value)
{
	enf_assert(m_pStream != NULL);

	float n[6];

	n[0] = LE_FLOAT(value.GetMins().x);
	n[1] = LE_FLOAT(value.GetMins().y);
	n[2] = LE_FLOAT(value.GetMins().z);
	n[3] = LE_FLOAT(value.GetMaxs().x);
	n[4] = LE_FLOAT(value.GetMaxs().y);
	n[5] = LE_FLOAT(value.GetMaxs().z);

	m_pStream->Write((const unsigned char*)&n, sizeof(AAB));
}



//-----------------------------------------------------------------------
void IFFOutput::WriteString(const char *ptr)
{
	enf_assert(m_pStream != NULL);

	if(ptr == NULL)
	{
		WriteUInt32(0xffffffff);
	}
	else
	{
		uint len =	strlen(ptr) + 1;
		WriteUInt32(len);
		Write((const unsigned char*)ptr, len);
	}
}

//-----------------------------------------------------------------------
void IFFOutput::Seek(size_t len)
{
	enf_assert(m_pStream != NULL);
   m_pStream->Seek(len);
}

//-----------------------------------------------------------------------
void IFFOutput::Close()
{
	if(m_pStream)
	{
		if(m_iHunkLevel)
			EndHunk();
		m_pStream->Close();
		m_pStream = NULL;
	}
	m_iHunkLevel = 0;
	m_bOpen = false;
}

//-----------------------------------------------------------------------
bool IFFOutput::Create(COutputStream* stream, const HunkID& id)
{
	Close();

	m_pStream = stream;

	if(m_pStream == NULL)
		return false;

	if(!m_pStream->IsOpen())
		return false;

	if(!BeginHunk("FORM"))
		return false;

	Write(id.id, sizeof(id.id));

	m_bOpen = true;
	return true;
}

//-----------------------------------------------------------------------
IFFOutput::IFFOutput(COutputStream* stream, const HunkID& id) : IFF(), 	m_pStream(NULL)
{
	Create(stream, id);
}

//-----------------------------------------------------------------------
uint IFFOutput::BeginHunk(const HunkID& id)
{
	static const uint len = 0;

	enf_assert(m_pStream != NULL);
	Hunk* hunk = &m_Hunks[m_iHunkLevel++];
	m_pStream->Write((const unsigned char*)&id, HunkID::MAX_LENGTH);
	m_pStream->Write((const unsigned char*)&len, sizeof(len));
	hunk->Position = m_pStream->GetCurrentPosition();
	hunk->Type = HUNKTYPE_BIG;
	hunk->ID = id;
	return m_iHunkLevel;
}

//-----------------------------------------------------------------------
uint IFFOutput::BeginSubHunk(const HunkID& id)
{
	static const ushort len = 0;

	enf_assert(m_pStream != NULL);
	Hunk* hunk = &m_Hunks[m_iHunkLevel++];
	m_pStream->Write((const unsigned char*)&id, HunkID::MAX_LENGTH);
	m_pStream->Write((const unsigned char*)&len, sizeof(len));
	hunk->Position = m_pStream->GetCurrentPosition();
	hunk->Type = HUNKTYPE_SMALL;
	hunk->ID = id;
	return m_iHunkLevel;
}

//-----------------------------------------------------------------------
bool IFFOutput::EndHunk()
{

	if(m_iHunkLevel == 0)
		return false;

	size_t pos = m_pStream->GetCurrentPosition();
	Hunk* hu = &m_Hunks[--m_iHunkLevel];
	size_t len;

	if(hu->Type == HUNKTYPE_BIG)
	{
		m_pStream->Seek(hu->Position - sizeof(int));
		len = pos - hu->Position;
		Write((int)len);
	}
	else if(hu->Type == HUNKTYPE_SMALL)
	{
		m_pStream->Seek(hu->Position - sizeof(ushort));
		len = pos - hu->Position;
		enf_assert(len < 65536);
		Write((short)len);
	}
   m_pStream->Seek(pos);
   return true;
}

}
