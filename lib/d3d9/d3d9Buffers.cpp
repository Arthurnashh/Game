#include "../wpch.hpp"

#include "../d3d9/engdd9.hpp"
#include "../d3d9/d3d9Buffers.hpp"

#if !_DISABLE_GUI

//-----------------------------------------------------------------------------
void UserVertexBuffer::CreateBuffer()
{
	m_pBuffer.Free();

	if (!GEngineDD)
		return;

	MyD3DDevice &dev = GEngineDD->GetDirect3DDevice();
	int byteSize = m_iMaxCount*m_iVertexSize;

	// make sure there is no extensive VRAM memory overhead
	GEngineDD->TextBankDD()->ReserveMemory(byteSize,3);

	PROFILE_SCOPE_DETAIL_EX(swCVBD,SW);
	do 
	{
		EngineDD9::CheckMemTypeScope scope(GEngineDD,"VBdynamic",byteSize);
		HRESULT err = dev->CreateVertexBuffer(byteSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, m_pBuffer.Init(), NULL);

		if (err != D3D_OK)
		{
			if (err == D3DERR_OUTOFVIDEOMEMORY)
			{
				GEngineDD->DDError9("UserVertexBuffer::Create - not enough VRAM memory", err);
				if (GEngineDD->TextBankDD()->ForcedReserveMemory(byteSize))
				{
					continue;
				}
			}

			LogF("UserVertexBuffer::Create - failed (%d)", err);
			if (GEngineDD->FreeOnDemandSystemMemoryLowLevel(byteSize)) 
				continue;

			RptF("UserVertexBuffer::Create - failed (%d)", err);
			GEngineDD->DDError9("UserVertexBuffer::Create", err);
			GEngineDD->RequestFlushMemoryCritical();
			return;
		}
	} while (0);

	DoAssert(m_pBuffer);
}


//-----------------------------------------------------------------------------
void* UserVertexBuffer::Lock(int numVertices, int& offset)
{
	offset = -1;

	if (!m_pBuffer)
		return NULL;

	DWORD lockFlags = D3DLOCK_NOOVERWRITE;
	if (numVertices > GetNumFree())
	{
		// not enough space - need to discard
		//if (m_iCurWritePos < numVertices)
		lockFlags = D3DLOCK_DISCARD;

		m_iCurWritePos = 0;
	}

	// lock
	int size = m_iVertexSize*numVertices;
	void *vbData = 0;
	HRESULT hr = GEngineDD->Lock(m_pBuffer, m_iCurWritePos*m_iVertexSize, size, &vbData, lockFlags);
	if (FAILED(hr) || !vbData)
	{
		GEngineDD->DDError9("UserVertexBuffer - VB Lock failed!", hr);
		return NULL;
	}

	offset = m_iCurWritePos;
	m_iCurWritePos += numVertices;

	return vbData;
}


//-----------------------------------------------------------------------------
void UserVertexBuffer::UnLock()
{
	HRESULT hr = m_pBuffer->Unlock();

	if (FAILED(hr))
	{
		GEngineDD->DDError9("UserVertexBuffer - VB Unlock failed!", hr);
	}
}


//-----------------------------------------------------------------------------
int UserVertexBuffer::FillVertices(void *data, int numVertices)
{
	PROFILE_SCOPE_EX(swVBFill, SW);

	if (!m_pBuffer || !data)
		return -1;

	int offset = 0;
	VBLockScope lock(*this, numVertices, offset);
	if (lock.GetData())
	{
		SetUsed(Glob.frameID);

		memcpy(lock.GetData(), data, m_iVertexSize*numVertices);
	}

	return offset;
}


//-----------------------------------------------------------------------------
Ref<UserVertexBuffer> UserVertexBufferArray::GetVertexBuffer(int numVertices)
{
	// try to find free VB
	for (int i = 0; i < m_VertexBuffers.Size(); ++i)
	{
		Ref<UserVertexBuffer> buffer = m_VertexBuffers[i];

		if (!buffer)
			continue;

		if (buffer->IsBusy(Glob.frameID))
		{
			// busy - lets check if there is space for our vertices
			if (buffer->GetNumFree() >= numVertices)     
			{
				return buffer; // we still have a space to write
			}
		}
		else
		{
			return buffer;
		}
	}

	// create new
	if (m_VertexBuffers.Size() < MaxVertexBuffers)
	{
		UserVertexBuffer* buffer = new UserVertexBuffer(m_iMaxVertices, m_iVertexSize);
		buffer->CreateBuffer();
		m_VertexBuffers.Add(buffer);
		return buffer;
	}
	else
	{
		LogF("SimulWeather - CloudRenderer - cannot find non-busy vertex buffer for cloud rendering. Clouds will not be rendered.");
		return NULL;
	}
}

#endif