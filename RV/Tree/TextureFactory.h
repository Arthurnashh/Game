#ifndef _TREE_ENGINE_ITEXTUREFACTORY_
#define _TREE_ENGINE_ITEXTUREFACTORY_
#pragma once



namespace TreeEngine
{

  //! Parameters used during generation controlled by tgen file f.i.
  struct DrawParameters
  {
    //! Flag to determine the polyplane's color map should include lighting
    bool _enlighted;
    //! Will create normalmap
    /** Useful only with LindaEditTexFactory, because LindaGen will always create normalmap
    */
    bool _normalMap;

    bool _ignoreForcedPolyplanes;
    //!{ Parameters for clustering (normal map will be modified according to these parameters)
    float _clusterRepresentativesPerMeter3;
    float _clusterWeight;
    //!}
    //! Number of vertices used for the polyplane hull
    int _polyplaneHullVertices;
    //! Constructor

    DrawParameters()
    {
      _enlighted = false;
      _clusterRepresentativesPerMeter3 = 1.0f;
      _clusterWeight = 0.0f;
      _polyplaneHullVertices = 8;
      _normalMap = true;
      _ignoreForcedPolyplanes = false;
    }

    template<class Archive>
    void Serialize(Archive &arch) {
        
        arch("enlighted",_enlighted);
        if (arch.IsVer(0x202))
            arch("normalMap",_normalMap);
        if (arch.IsVer(0x203))
            arch("ignoreForcedPolyplanes",_ignoreForcedPolyplanes);
        arch("clusterRepresentativesPerMeter3",_clusterRepresentativesPerMeter3);
        arch("clusterWeight",_clusterWeight);
        arch("polyplaneHullVertices",_polyplaneHullVertices);
    }

  };
  extern DrawParameters DPDefault;


  class CPrimitiveStream;
  class ITextureFactory
  {
  public:

    ///Renders plane
    /**This function is called for each polyplane
    Implementation must create normal and color map from given values

    @param PSBranch "non-polyplanes" primitive stream - branches, etc
    @param PSPolyplane "polyplane" primitive stream - triplanes etc.
    @param colorMap reference to colorMap - store will store the resulting color map here
    @param normalMap reference to normalMap - store will store the resulting normal map here
    @param Camera camera matrix
    @param Width width of polyplane geometry
    @param Height height of polyplane geometry
    @param sizex recommended texture size X
    @param sizey recommended texture size Y
    @param enlighted polyplane should be enlighted
    @return S_OK if there was no error, otherwise result is DirectX error
    */
    virtual HRESULT RenderPlane( CPrimitiveStream &PSBranch,
      CPrimitiveStream &PSPolyplane,
      ComRef<IDirect3DTexture8> &colorMap,
      ComRef<IDirect3DTexture8> &normalMap,
      Matrix4Par Camera,
      float Width,
      float Height,
      unsigned long sizex,
      unsigned long sizey,
      const DrawParameters &dp)=0;

    ///Loads texture from file
    /**
    Every texture processed by TreeEngine is loaded by this function
    @param filename Pathname for texture directly from component definition
    @param map resulting texture
    @return S_OK is no error
    */
    virtual HRESULT LoadTexure( const char *filename, ComRef<IDirect3DTexture8> &map, bool linearAlpha)=0;

    ///Gets lockable level from texture
    /**
    Tree engine cannot assume, that textures are lockable. If tree engine needs read content
    of texture, it need to get lockable surface. This function returns lockable surface of level
    of given texture. It can be implemented by GetSurfaceLevel function, but if texture is not
    lockable, function should copy content of texture into the system memory and return it.
    @param src source texture
    @param lockable returned lockable level
    @param level level number
    @return S_OK is no error
    */
    virtual HRESULT GetLockableLevel(IDirect3DTexture8 *src, ComRef<IDirect3DSurface8> &lockable, int level)=0;

    ///Returns render device associated with this factory
    /**
    @note Reference count if incerased before pointer is returned. Don't forget 
    call Release when you don't needed this pointer more.
    */
    virtual IDirect3DDevice8 *GetRenderDevice()=0;

    ///Prepares device to rendering branches
    /**
    This function is called during preview. It should select right pixel and vertex shader 
    into device
    */
    virtual void PrepareBranchPreview()=0;
    ///Prepares device to rendering triplanes
    /**
    This function is called during preview. It should select right pixel and vertex shader 
    into device
    */
    virtual void PrepareTriplanePreview()=0;

    virtual void StartGenerating() {}
    virtual void StopGenerating() {}

    virtual void RenderLock(bool lock) {}

    virtual int GetRenderPlaneComplexFactor() {return 150;}
  };

  class BaseTextureFactory: public ITextureFactory
  {
  protected:
    //! VS handle for branch rendering.
    DWORD _hVSBranch;
    //! PS handle for branch rendering.
    DWORD _hPSBranch;
    //! VS handle for triplane rendering.
    DWORD _hVSTriplane;
    //! PS handle for triplane rendering.
    DWORD _hPSTriplane;
    //! VS handle for branch color map generation.
    DWORD _hVSBranchColor;
    //! PS handle for branch color map generation.
    DWORD _hPSBranchColor;
    //! VS handle for branch normal map generation.
    DWORD _hVSBranchNormal;
    //! PS handle for branch normal map generation.
    DWORD _hPSBranchNormal;
    //! VS handle for polyplane color map generation.
    DWORD _hVSPolyplaneColor;
    //! PS handle for polyplane color map generation.
    DWORD _hPSPolyplaneColor;
    //! VS handle for polyplane normal map generation.
    DWORD _hVSPolyplaneNormal;
    //! PS handle for polyplane normal map generation.
    DWORD _hPSPolyplaneNormal;
    //! 3D Device.
    PDirect3DDevice _pD3DDevice;

    ComRef<IDirect3DSurface8> _renderTarget;
    ComRef<IDirect3DSurface8> _depthBuffer;
  public:

    struct InitResult
    {
      RString errShader;

      InitResult() {}
      InitResult(const char *errShader):errShader(errShader)  {}

      operator bool() const {return errShader==RString(0);}
    };

    //! Initialization.
    InitResult Init(PDirect3DDevice &pD3DDevice, const char *shaderPath="Tree/Shaders/");

    ///Returns render device associated with this factory
    virtual IDirect3DDevice8 *GetRenderDevice()
    {
      _pD3DDevice->AddRef();
      return _pD3DDevice;
    }

    virtual void PrepareBranchPreview();
    virtual void PrepareTriplanePreview();

    ///Loads texture from file
    virtual HRESULT LoadTexure( const char *filename, ComRef<IDirect3DTexture8> &map,bool linearAlpha);

  protected:
    ///Prepares render target
    /**
    function allocates render target of given size;
    @param sizex size of render target. Function stores the final size of render target here
    @param sizey size of render target. Function stores the final size of render target here
    @return S_OK is no error
    */
    HRESULT PrepareRenderTarget(unsigned long &sizex, unsigned long &sizey);

    ///Prepares camera
    HRESULT PrepareCamera(Matrix4Par Camera, float Width, float Height);

    ///Rendering core for color map
    HRESULT RenderCoreColorMap (CPrimitiveStream &PSBranch,CPrimitiveStream &PSPolyplane, bool enlighted);
    ///Rendering core for normal map
    HRESULT RenderCoreNormalMap (CPrimitiveStream &PSBranch,CPrimitiveStream &PSPolyplane);

    virtual void StopGenerating() {_renderTarget << 0;_depthBuffer << 0;}
  };

  class LindaGenTexFactory: public BaseTextureFactory
  {
  protected:
    bool _discretizeAlpha;
    int _antialiasingDegree;
    int _maxTexSize;
    int _minTexSize;
  public:

    LindaGenTexFactory(bool discretizeAlpha,int antialiasingDegree, int minTexSize=0, int maxTexSize=2048):
        _discretizeAlpha(discretizeAlpha),
          _antialiasingDegree(antialiasingDegree),
          _maxTexSize(maxTexSize),
          _minTexSize(minTexSize)
        {}

    ///Renders plane
    virtual HRESULT RenderPlane( CPrimitiveStream &PSBranch,
      CPrimitiveStream &PSPolyplane,
      ComRef<IDirect3DTexture8> &colorMap,
      ComRef<IDirect3DTexture8> &normalMap,
      Matrix4Par Camera,
      float Width,
      float Height,
      unsigned long sizex,
      unsigned long sizey,
      const DrawParameters &dp);

    virtual void PreviewPolyplane(IDirect3DSurface8 *surface) {}

    ///Gets lockable level from texture
    virtual HRESULT GetLockableLevel(IDirect3DTexture8 *src, ComRef<IDirect3DSurface8> &lockable, int level);

    void SetMinTexSize(int minTexSize){_minTexSize=minTexSize;}
    void SetMaxTexSize(int maxTexSize){_maxTexSize=maxTexSize;}
    virtual int GetRenderPlaneComplexFactor() {return 250;}
    virtual HRESULT LoadTexure( const char *filename, ComRef<IDirect3DTexture8> &map, bool linearAlpha);

  };

  class LindaEditTexFactory: public BaseTextureFactory
  {
  public:
    virtual HRESULT RenderPlane( CPrimitiveStream &PSBranch,
      CPrimitiveStream &PSPolyplane,
      ComRef<IDirect3DTexture8> &colorMap,
      ComRef<IDirect3DTexture8> &normalMap,
      Matrix4Par Camera,
      float Width,
      float Height,
      unsigned long sizex,
      unsigned long sizey,
      const DrawParameters &dp);

    virtual HRESULT GetLockableLevel(IDirect3DTexture8 *src, ComRef<IDirect3DSurface8> &lockable, int level);

  };

};
#endif