#ifndef _D3D9BUFFERS_HPP
#define _D3D9BUFFERS_HPP

#if !_DISABLE_GUI

#include "d3d9defs.hpp"


//-----------------------------------------------------------------------------
/*!
represents vertex buffer used to store user geometry
*/
//-----------------------------------------------------------------------------
class UserVertexBuffer: public RefCount
{
private:
	ComRef<IDirect3DVertexBuffer9> m_pBuffer;	

	int m_iMaxCount;		///< maximal possible number of vertices in the buffer
	int m_iVertexSize;	///< size of one vertex in bytes
	int m_iFrameID;			///< when this buffer was last used
	int m_iCurWritePos;	///< current free write position in the buffer (in vertices, not bytes!)

public:
  /*!
	constructor
  \param maxCount			maximal number of vertices that can be stored in the buffer
	\param vertexSize		vertex size
  */
  UserVertexBuffer(int maxCount, int vertexSize): m_iMaxCount(maxCount), m_iVertexSize(vertexSize), m_iFrameID(-1), m_iCurWritePos(0)
	{
		Assert(m_iVertexSize > 0 && m_iMaxCount > 0);
	}

  // creates buffer
  void CreateBuffer();

	/*!
	fills vertex buffer with given data, returns starting vertex index
	\param data					src data
	\param numVertices	number of vertices
	*/	
	int FillVertices(void *data, int numVertices);

	/*!
	lock vertex buffer
	\param numVertices	vertices to lock
	\param offset				offset where the lock starts
	*/
	void* Lock(int numVertices, int& offset);

	/*!
	unlock vertex buffer
	*/
	void UnLock();

	/*!
  returns if buffer is busy, we mark the buffer busy in current and prev frame
	\param frameID			current frame ID
	*/
  bool IsBusy(int frameID) const 
	{
		return m_iFrameID >= frameID - 1;
	}

	/*!
  marks the buffer as used
	\param frameID			current frame ID
	*/
  void SetUsed(int frameID) 
	{
		m_iFrameID = frameID;
	}

  // returns how many free vertices are we able to write
  int GetNumFree() const 
	{
		return m_iMaxCount - m_iCurWritePos;
	}

	// returns vertex size
  int GetVertexSize() const 
	{
		return m_iVertexSize;
	}

	// maximum number of vertices
  int GetMaxCount() const 
	{
		return m_iMaxCount;
	}

	//current position to write
  int GetCurWritePos() const 
	{
		return m_iCurWritePos;
	}

  ComRef<IDirect3DVertexBuffer9> GetBuffer() 
	{
		return m_pBuffer;
	}
};


//-----------------------------------------------------------------------------
/*!
scope class to safe lock/unlock VBs
*/
//-----------------------------------------------------------------------------
class VBLockScope
{
private:
	void*							m_pData;
	UserVertexBuffer& m_Buffer;	

public:
	VBLockScope(UserVertexBuffer& buffer, int vertices, int &offset): m_Buffer(buffer), m_pData (NULL)
	{
		m_pData = buffer.Lock(vertices, offset);
	}

	void* GetData() const
	{
		return m_pData;
	}

	~VBLockScope()
	{
		if (m_pData)
			m_Buffer.UnLock();
	}
};


//-----------------------------------------------------------------------------
/*!
represents vertex buffer array used to user geometry
*/
//-----------------------------------------------------------------------------
class UserVertexBufferArray : public RefCount
{
private:
	AutoArray<Ref<UserVertexBuffer > > m_VertexBuffers;		///< array of vertex buffers
	int m_iVertexSize;
	int m_iMaxVertices;

public:
	// max. number of vertex buffers
	static const int MaxVertexBuffers = 6;

	UserVertexBufferArray(): m_iMaxVertices(0), m_iVertexSize(0)
	{}

	/*!
	initialize buffers
	\param maxVertices	number of vertices
	\param vertexSize		size of vertex
	*/
	void Init(int maxVertices, int vertexSize)
	{
		Assert(maxVertices > 0 && vertexSize > 0);

		Clear();
		m_iMaxVertices = maxVertices;
		m_iVertexSize  = vertexSize;
	}

	/*!
	return free vertex buffer 
	\param numVertices		number of vertices
	*/
	Ref<UserVertexBuffer > GetVertexBuffer(int numVertices);

	// clear buffers
	void Clear()
	{
		m_VertexBuffers.Clear();
	}

	//is valid
	bool IsValid() const
	{
		return m_iMaxVertices*m_iVertexSize > 0;
	}
};


#endif
#endif