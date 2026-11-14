#ifndef _primitivestream_h_
#define _primitivestream_h_

#include <Es/Common/win.h>
#include <Es/essencepch.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Strings/rString.hpp>
#include <El/ParamFile/paramFile.hpp>
#include "TreeDXCommon.h"


namespace TreeEngine
{


  #define PRIMITIVESTAGE_COUNT 1024
  #define VB_SIZE 65536
  #define IB_SIZE (65536*4)

  typedef AutoArray<RString> StringArray;

  //! Set of primitives represented as a incices to vertex buffer.
  /*!
    Type of index is set to D3DFMT_INDEX16. Type of primitive is D3DPT_TRIANGLELIST.
  */
  class CPrimitiveStage {
  private:
    //! List of triangles associated with this stage. All of them will be drawn as a single drawindexedprimitive.
    AutoArray<unsigned long> _Indices;
    //! Minimum index of a vertex associated with this stage.
    UINT _MinVertexIndex;
    //! Maximum index of a vertex associated with this stage.
    UINT _MaxVertexIndex;
  public:
    //! Texture on stage 0
    //IDirect3DTexture8 *_pTextureS0;
    ComRef<IDirect3DTexture8> _pTextureS0;
    //! Name of the texture on stage 0
    RString _TS0Name;
    //! Texture on stage 1
    //IDirect3DTexture8 *_pTextureS1;
    ComRef<IDirect3DTexture8> _pTextureS1;
    //! Name of the texture on stage 1
    RString _TS1Name;
    //! U textue adressing type
    D3DTEXTUREADDRESS _adressU;
    //! V textue adressing type
    D3DTEXTUREADDRESS _adressV;
    //! true if texture has linear alpha
    bool _isAlpha;
    //! Constructor
    CPrimitiveStage();
    //! Clears this stage.
    void Clear(bool freeTextures);
    //! Add specified index. Array will be resized if nacessary.
    void AddIndex(const unsigned long &Index);
    //! Returns number of indices in the array.
    int Size() const;
    //! Returns pointer to stage data.
    const unsigned long *Data() const;
    //! Returns minimal vertex index associated with this stage.
    UINT GetMinVertexIndex();
    //! Returns size of the vertex block associated with this stage.
    UINT GetNumVertices();
    /// Sorts stage to display alpha faces better
    /**
     * @param vertices Array of vertices, that faces referces
     */
    void SortStage(Vector3 *vertices);
  };

  //! Stream of primitives to draw.
  /*!
    Type of vertex is specified by _VertexFVF and its size by _VertexSize.
  */
  class CPrimitiveStream : public RefCount {
  private:
    //! 3D Device.
    ComRef<IDirect3DDevice8> _pD3DDevice;
    //! Vertex buffer.
    ComRef<IDirect3DVertexBuffer8> _pVB;
    //! Index buffer.
    ComRef<IDirect3DIndexBuffer8> _pIB;
    //! List of primitive stages.
    CPrimitiveStage _PS[PRIMITIVESTAGE_COUNT];
    //! Array of vertices.
    AutoArray<BYTE> _Vertices;
    //! Size of one vertex in bytes.
    int _VertexSize;
    //! Either handle for the vertex shader or FVF code.
    //DWORD _VertexHandle;
    //! Handle for the pixel shader.
    //DWORD _PixelHandle;
    //! Positions of stages in index buffer. This is set by Prepare() method.
    UINT _StageIndexPos[PRIMITIVESTAGE_COUNT];
    //! Number of indices in all stages together.
    int _IndexCount;

    //
    DWORD _FVF;
  public:
    //! Constructor.
    CPrimitiveStream();
    //! Constructor.
    CPrimitiveStream(ComRef<IDirect3DDevice8> &pD3DDevice);
    //! Copy constructor.
    /*!
      This is very quick copy constructor. Both streams will share their VB and IB.
      \param PrimitiveStream Source stream.
    */
    CPrimitiveStream(CPrimitiveStream &PrimitiveStream);

    //! Full Copy constructor.
    /*!
    */
    CPrimitiveStream(CPrimitiveStream &PrimitiveStream, bool full);

    void Init(CPrimitiveStream &PrimitiveStream);
    //! Initialization.
    /*!
      \param VertexSize Size of single vertex.
      \param VertexHandle Either handle for the vertex shader or FVF code.
      \param PixelHandle Handle for the pixel shader.
      \param FVF FVF code which should correspond to VertexHandle.
    */
    void Init(int VertexSize, DWORD FVF);
    //! Copying of primitiveStream data
    //void CopyData(CPrimitiveStream &primitiveStream);
    //! Clears all vertices and indices.
    void Clear(bool freeTextures);
    //! Add single vertex to vertex buffer.
    /*!
      \param pData Pointer to vertex data.
      \return Index of new vertex in vertex buffer.
    */
    unsigned long AddVertex(void *pData);
    //! Retrieve the vertex data
    void GetVertex(int vertexIndex, void *pData) const;
    //! Return pointer to the vertex data at the beginning of the desired vertex
    void *GetVertexData(int vertexIndex);


    //! Add single index to index buffer at specified stage.
    /*!
      \param Stage Index of stage to add index to.
      \param Index Index to add.
    */
    void AddIndex(int Stage, unsigned long Index);
    //! Gets index of the new vertex in the vertex buffer.
    unsigned long GetNewVertexIndex();
    //! Registers textures on specified stage.
    void RegisterTextures(
      int Stage,
      ComRef<IDirect3DTexture8> &textureS0,
      RString TS0Name,
      ComRef<IDirect3DTexture8> &textureS1,
      RString TS1Name,
      D3DTEXTUREADDRESS adressU,
      D3DTEXTUREADDRESS adressV,
      bool alpha=false);
    void SetTextureContent(int stage,ComRef<IDirect3DTexture8> &textureS0, ComRef<IDirect3DTexture8> &textureS1);
    //! Prepares vertex and index buffers.
    /*!
      Besides, this method set up the _StageIndexPos array which will be used
      in Draw method.
    */
    void Prepare();
    //! Draws primitives on the surface.
    void Draw(BOOL SetTextures = TRUE);
    //! Returns array of vertices
    const AutoArray<BYTE> &GetVertices();
    //! Returns specified stage
    CPrimitiveStage *GetPrimitiveStage(int i);
    //! Returns number of indices
    int GetIndexCount();
    void AddNewTextureNames(StringArray &names);

    void CopyFrom(const CPrimitiveStream &other)
    {
      _Vertices=other._Vertices;
      for (int i=0;i<PRIMITIVESTAGE_COUNT;i++)
      {
        _StageIndexPos[i]=other._StageIndexPos[i];
          _PS[i]=other._PS[i];
      }
      _IndexCount=other._IndexCount;
    }

    void SortStages();


    int GetVertexSize() const {return _VertexSize;}    

    CPrimitiveStream& operator=(const CPrimitiveStream &PrimitiveStream)
    {
      this->~CPrimitiveStream();      //destruct current instance
      _pD3DDevice=PrimitiveStream._pD3DDevice;
      Init(PrimitiveStream._VertexSize,PrimitiveStream._FVF);
      _pD3DDevice=PrimitiveStream._pD3DDevice;
      CopyFrom(PrimitiveStream);
      return *this;
    }

    const CPrimitiveStage *GetPrimitiveStage(unsigned int stage) const
    {
      if (stage<PRIMITIVESTAGE_COUNT) return _PS+stage;
      else return 0;
    }

    const Array<BYTE> &GetVertices() const {return _Vertices;}


    AutoArray<Vector3> ExportPointsForTexture(IDirect3DTexture8 *texture) const;
    AutoArray<Vector3> ExportPointsForStage(int index) const;

    ComRef<IDirect3DTexture8> GetTextureAtIndex(int index,bool normalMap);

    bool IsStageUsed(int index) const;
  };

}

TypeIsSimple(TreeEngine::StringArray);

#endif
