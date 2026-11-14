#ifndef _polyplane_h_
#define _polyplane_h_

#include "primitivestream.h"
#include "pointList.h"
#include "TextureFactory.h"


namespace TreeEngine
{
  #ifdef PRERELEASE
  #undef FAILED
  #undef assert
  #undef ASSERT
  static inline bool TestFailed(bool res, const char *type, const char *text, const char *file, int line)
  {
    if (!res) LogF("%s: %s at %s line %d",type,text,file,line);
    return res;
  }
  #define FAILED(x) (!TestFailed((x)==0,"FAILED",#x,__FILE__,__LINE__))
  #define assert(x) (TestFailed((x),"assert",#x,__FILE__,__LINE__))
  #define ASSERT(x) (TestFailed((x),"ASSERT",#x,__FILE__,__LINE__))
  #endif


  #define POLYPLANE_TRIANGLE_COUNT 12

  // Polyplane Type
  #define PT_NONE 1
  #define PT_BLOCK 2
  #define PT_BLOCKBEND 3
  #define PT_BLOCKNORM 4
  #define PT_BLOCKSIMPLE 5
  #define PT_BLOCKSIMPLE2 6
  #define PT_BLOCKBEND2 7
  #define PT_BLOCKNORM2 8
  #define PT_UNIPLANE_X 9
  #define PT_UNIPLANE_Y 10
  #define PT_UNIPLANE_Z 11
  #define PT_NOTDEFINED 12
  #define PT_BUNCH 13


  struct SPolyplaneVertex {
    // Vertex Position.
    Vector3 Position;
    // Vertex Normal.
    Vector3 Normal;
    // Coordinates in the normal map and at the same time coordinates in the RGB map.
    float U1, V1;
    // S coordinate of the orientation of the vertex.
    Vector3 S;
    // T coordinate of the orientation of the vertex.
    Vector3 T;

    bool GenerateST(const SPolyplaneVertex &bvec, const SPolyplaneVertex &cvec);
    bool CalculateNormal(const SPolyplaneVertex &bvec, const SPolyplaneVertex &cvec);
    ClassIsMovable(SPolyplaneVertex);

  };
  //#define D3DFVF_POLYPLANEVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX3|D3DFVF_TEXCOORDSIZE2(0)|D3DFVF_TEXCOORDSIZE3(1)|D3DFVF_TEXCOORDSIZE3(2))

  DWORD dwPolyplaneDecl[];
  DWORD dwPolyplaneNormalDecl[];

  //! Resources associated with one plane of the polyplane
  struct PlaneResources
  {
    //! Color texture
    ComRef<IDirect3DTexture8> _p;
    //! Normal map
    ComRef<IDirect3DTexture8> _pNorm;
    //! 2D convex hull of the plane
    CPointList _hull;
    //! Release all the resources
    void Free()
    {
      _p.Free();
      _pNorm.Free();
      _hull.Clear();
    }
  };

  //! Polyplane
  /*!
    This abstract class represents a common object which aproximates
    a bunch of geometry primitives positioned close to each other.
  */
  class ITextureFactory;
  class CPolyplane : public RefCount
  {
  protected:

    ITextureFactory *_factory;

    int _textureSize;
    //! Importancy of the polyplane
    float _importancy;
    //! Coeficient of the size
    /*!
      Using this parameter we can scale the polyplane during drawing.
    */
    float _SizeCoef;
    ///Index of textures in stages;
    int _baseStage;
    ///Array of representatives
    /**
    Useful for debuging
    */
    mutable RepresentativeArray _ra;

    void FillBackgroundWithAvgColor(ComRef<IDirect3DTexture8> &map, int level);
    void DisableAlpha(ComRef<IDirect3DTexture8> &map, int level);
    void Downsize(ComRef<IDirect3DTexture8> &mapSource, ComRef<IDirect3DTexture8> &mapDest);
    void CreateMipLevels(ComRef<IDirect3DTexture8> &map);
    void FixInvalidNormals(ComRef<IDirect3DTexture8> &map);
    void DiscretizeAlpha(ComRef<IDirect3DTexture8> &map);
    //! Renders stream into one plane. Renders both color and normal map, creates point list if required
    void RenderPlane(      
      CPrimitiveStream &PSBranch,
      CPrimitiveStream &PSPolyplane,
      ComRef<IDirect3DTexture8> &colorMap,
      ComRef<IDirect3DTexture8> &normalMap,
      const CPointList &pointList,
      Matrix4Par Camera,
      float Width,
      float Height,
      const DrawParameters &dp,
      CPointList *outPolygon);
    //! Draws one plane according to specified parameters.
    void DrawPlane(
      int Stage,
      CPrimitiveStream &PSTriplane,
      ComRef<IDirect3DTexture8> &colorMap,
      ComRef<IDirect3DTexture8> &normalMap,
      Matrix4Par Origin,
      float Width,
      float Height,
      float dirOffset,
      const CPointList *polygon);

    void SaveTexture(RString fileName, ComRef<IDirect3DTexture8> &texture, bool normalMap);

    void RegisterTextureToStage(CPrimitiveStream &PSStageInit, int offset, ComRef<IDirect3DTexture8> colorMap, ComRef<IDirect3DTexture8> normalMap);

    // Modify geometry normals according to pointList and specified coefficients. This is here to
    // create clustering as described in the article "Simplified Tree Lighting Using Aggregate
    // Normals" by "Scott Peterson and Lawrence Lee"
    void ModifyNormals(CPrimitiveStream &PSBranch, CPrimitiveStream &PSPolyplaneconst, const CPointList &pointList, float polyplaneVolume, const DrawParameters &dp) const;

  public:
    //! Constructor.
    CPolyplane();
    //! Destructor.
    ~CPolyplane();
    //! Initialization of polyplane
    virtual HRESULT Init(ITextureFactory *factory, int textureSize);
    //! Deinitialization of polyplane
    virtual HRESULT Done();
    //! Preparing of the polyplane
    virtual void Render(
      CPrimitiveStream &PSBranch,
      CPrimitiveStream &PSPolyplane,
      CPrimitiveStream &PSStageInit,
      Matrix4Par Origin,
      const CPointList &pointList,
      float SizeCoef,
      const DrawParameters &dp) = 0;
    //! Drawing of the polyplane
    virtual void Draw( CPrimitiveStream &PSTriplane,   Matrix4Par Origin,   float NewSizeCoef) = 0;
    //! Updating of the point list
    virtual void UpdatePointList(
      Matrix4Par Origin,
      float NewSizeCoef,
      CPointList &pointList) = 0;
    //! Saving of the polyplane to file
    virtual void Save(RString fileName, RString suffix) = 0;
    //! Returns the size of the texture
    void UpdateImportancy(float importancy) {_importancy = max(_importancy, importancy);};
    //! Returns the importancy
    float GetImportancy() {return _importancy;};
    //! Returns the size coeficient
    float GetSizeCoef() {return _SizeCoef;};   
    //! Returns number of used textures (maximum is 3 - by default)
    virtual int GetNumberOfTextures() {return 3;};
    //! Virtual method
    virtual int GetTextureSize() {return _textureSize;};
    
    virtual ComRef<IDirect3DTexture8> GetTexture(int index)=0;

    virtual ComRef<IDirect3DTexture8> GetNormalTexture(int index)=0;

    virtual int SetBaseStage(int baseStage)
    {
      _baseStage=baseStage;
      return GetNumberOfTextures();
    }

    const RepresentativeArray &GetRepresentatives() const {return _ra;}
    float CalcScale(float sizeCoef) {
        if (_SizeCoef == 0) 
            return 1;
        else
            return sizeCoef / _SizeCoef;
    }

  };

  HRESULT SaveSurfaceRGBAToTGA(ComRef<IDirect3DSurface8> rgba32Surface, const char *filename);


}
#endif
