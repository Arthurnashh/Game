#ifndef _primitive_h_
#define _primitive_h_

#include <El/Math/math3d.hpp>
#include <Es/Framework/appFrame.hpp>
#include <El/ParamFile/paramFile.hpp>
#include <El/Evaluator/express.hpp>
#include <projects/ObjektivLib/ObjectData.h>
#include "primitivestream.h"
#include "pointList.h"



namespace TreeEngine
{
    
    using namespace ObjektivLib;

    // Description of the structure of tree vertex
  //#define D3DFVF_PRIMITIVE_VERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1|D3DFVF_TEXCOORDSIZE2(0))

  //! Coefficient to be used to resize point list envelope to cover the whole circel geometry
  const float SquareCoef = 1.4142135624; // 2/sqrt(2) - 

  // Macro which defines number of sides of a cone according to a specified parameters
  #define CONE_EDGESNUM_MAX 12
  #define CONE_EDGESNUM_MIN 3
  #define CONE_SIDES_NUM(Detail,Width) CalcConeSideNum(Width,Detail)

  //! Type of axis
  enum AxisType
  {
    AT_Aside,
    AT_Up,
    AT_Direction,
    AT_Aside_Rev,
    AT_Up_Rev,
    AT_Direction_Rev,
  };

  //! Axis rotation parameters
  struct AxisRotation
  {
    AxisType _at;
    float _angle;
  };

  static inline Matrix3 GetSideChildOriginOrientation(Matrix3 sideChildOriginOrientation,
                                        const AxisRotation *ar,
                                        int arCount) 
  {
    for (int i = 0; i < arCount; i++)
    {
      const AxisRotation &axisRotation = ar[i];
      Matrix3 rot;
      bool reversed = false;
      switch (axisRotation._at)
      {
      case AT_Aside_Rev: reversed = true;
      case AT_Aside:
        rot.SetRotationX(axisRotation._angle);
        break;

      case AT_Up_Rev: reversed = true;
      case AT_Up:
        rot.SetRotationY(axisRotation._angle);
        break;

      case AT_Direction_Rev: reversed = true;
      case AT_Direction:
        rot.SetRotationZ(axisRotation._angle);
        break;

      default:
        Fail("Unknown axis type");
      }
      if (reversed)
          sideChildOriginOrientation = rot * sideChildOriginOrientation ;
      else
          sideChildOriginOrientation = sideChildOriginOrientation * rot;
    }
    return sideChildOriginOrientation ;
  }


  //! Max size of the AxisRotation array
  #define AXIS_ROTATION_COUNT_MAX 4

  //! Structure of the primitive vertex
  struct SPrimitiveVertex
  {
    // Vertex position
    Vector3 _position;
    // Vertex normal
    Vector3 _normal;
    // Diffuse color
    D3DCOLOR _diffuse;
    // 1st set, 2-D
    float _u0, _v0;
  };

  DWORD dwPrimitiveVertexDecl[];

  //! Retrieving of the vertex item
  #define PRIMITIVE_VERTEX(ps,i) (((SPrimitiveVertex*)((ps).GetVertices().Data()))[(i)])

  //! Primitive child description
  struct SPrimitiveChild {
    //! Origin
    Matrix4 _origin;
    //! Index of the slot or -1 if there is not any
    int _slotIndex;
    //! V offset of the texture corresponds to the current depth of the tree
    float _textureVOffset;
    //! Geometry spin relative to the child orientation
    float _geometrySpin;
  };

  //! Slot
  struct SSlot
  {
    //! Index of the shared vertex
    int _sharedVertexIndex;
    //! Number of shared vertices
    int _sharedVertexCount;
    //! Flag to determine we're in a root (initialized by heel component)
    bool _isRoot;
    //! Constructor
    SSlot()
    {
      _sharedVertexIndex = -1;
      _sharedVertexCount = 0;
      _isRoot = false;
    }
  };

  class UVTransform
  {
    float a11,a12,a21,a22,a31,a32;
  public:
    UVTransform(Matrix3 &mx): a11(mx(1,1)),a12(mx(2,1)),
      a21(mx(1,2)),a22(mx(2,2)),
      a31(mx(1,3)),a32(mx(2,3)) {}

    UVTransform(float x, float y, float cx, float cy):a11(cx),a22(cy),a12(0),a21(0),
      a31(x),a32(y) {}

    UVTransform():a11(1),a22(1),a12(0),a21(0),a31(0),a32(0) {}

    UVTransform(float a11,float a12, float a21, float a22, float a31, float a32):
    a11(a11),a12(a12),a21(a21),a22(a22),a32(a32),a31(a31) {}

    float GetU(float u,float v) const {return u*a11+v*a21+a31;}
    float GetV(float u,float v) const {return u*a12+v*a22+a32;}
    float GetUNT(float u,float v) const {return u*a11+v*a21;}
    float GetVNT(float u,float v) const {return u*a12+v*a22;}

    UVTransform operator*(const UVTransform &other) const
    {
      return UVTransform(other.GetUNT(a11,a12),other.GetVNT(a11,a12),
        other.GetUNT(a21,a22),other.GetVNT(a21,a22),
        other.GetU(a31,a32),other.GetV(a31,a32));
    }
  };

  //! Primitive component
  class CPrimitive : public RefCount {
  private:
    //! Bending of a matrix according to a specified direction and force.
    /*!
      Matrix will be bended as if its direction vector would be affected by a force
      specified by ForceDirection and ForceIntensity. ForceDirection should be size 1.
      ForceIntensity specifies the maximum angle to bend in radians. With such force
      will be affected only matrices, whose direction vector is orthogonal to 
      ForceDirection.
      \param Matrix Matrix to bend.
      \param ForceDirection Direction of the force of size 1.
      \param ForceIntensity Intensity of the force in radians.
      \return Bended matrix.
    */
    Matrix3 BendMatrix3DirectionByForce(
      Matrix3Par matrix,
      Vector3Par forceDirection,
      float forceIntensity);
    //! Bending of a matrix according to specified direction up and force.
    Matrix3 AlignMatrix3UpByForce(
      Matrix3Par matrix,
      Vector3Par forceDirection,
      float forceIntensity);
  protected:
    //! Origin of the primitive
    Matrix4 _origin;
    //! Type of polyplane associated with this primitive
    int _polyplaneType;
    //! Draws a rhomb
    void DrawRhomb(
      CPrimitiveStream &PS,
      int Stage,
      D3DCOLOR Color,
      Matrix4Par A,
      float Width,                                          
      float Height,
      const Array<const UVTransform> &uvTransform,
      bool twosided,
      int *pTAccu = NULL);
    //! Draw heel (that means generate base vertices for trunk)
    int DrawHeel(CPrimitiveStream &ps,
                 float TextureTilingCircumferenceCoef,
                 D3DCOLOR color,
                 Matrix4Par b,
                 float radiusB,
                 float textureB,
                 int sideNumB);
    //! Draws a cone
    int DrawCone(
          CPrimitiveStream &PS,
          int Stage,
          float TextureTilingCircumferenceCoef,
          D3DCOLOR Color,
          Matrix4Par A,
          float RadiusA,
          float TextureA,
          int SideNumA,
          Matrix4Par B,
          float RadiusB,
          float TextureB,
          int SideNumB,
          int *pTAccu = NULL);
    //! Draws a cone with shared vertices
    int DrawConeShared(
          CPrimitiveStream &PS,
          int Stage,
          float TextureTilingCircumferenceCoef,
          D3DCOLOR Color,
          int SharedVertexIndex,
          int SharedSidesCount,
          bool isRoot,
          Matrix4Par B,
          float RadiusB,
          float TextureB,
          int SidesCountB,
          int *pTAccu = NULL);
    //! Draws a gutter
    int DrawGutter(
      CPrimitiveStream &PS,
      int Stage,
      float TextureTilingCircumferenceCoef,
      Matrix4Par A,
      float RadiusA,
      float depthA,
      float TextureA,
      int SideNumA,
      Matrix4Par B,
      float RadiusB,
      float depthB,
      float TextureB,
      int SideNumB,
      int *pTAccu = NULL);
    //! Draws a gutter with shared vertices
    int DrawGutterShared(
      CPrimitiveStream &ps,
      int stage,
      float TextureTilingCircumferenceCoef,
      int SharedVertexIndex,
      int SharedVertexNumber,
      Matrix4Par B,
      float RadiusB,
      float depthB,
      float TextureB,
      int SideNumB,
      bool extraEndPoint,
      int *pTAccu = NULL);
    //! Draws a star
    int DrawStar(
      CPrimitiveStream &ps,
      int stage,
      Matrix4Par a,
      float radiusA,
      float textureA,
      Matrix4Par b,
      float radiusB,
      float textureB,
      int plateNum,
      int *pTAccu = NULL);
    //! Draws a star with shared vertices
    int DrawStarShared(
      CPrimitiveStream &ps,
      int stage,
      int SharedVertexIndex,
      int SharedVertexNumber,
      Matrix4Par b,
      float radiusB,
      float textureB,
      int plateNum,
      int *pTAccu = NULL);
    void DrawFunnel(
      CPrimitiveStream &ps,
      int stage,
      Matrix4Par origin,
      float radius,
      float depth,
      int sideNum,
      int *pTAccu = NULL,
      bool twotextures = false);
    //! Draws a block directly to the object data
    static int DrawBlock(ObjectData *o, int &bodyId, Matrix4Par origin, float width, float height);
  public:
    //! Draws a sphere directly to the object data
    static int DrawSphere(ObjectData *o, int &bodyId, Matrix4Par origin, float sX, float sY, float sZ, int nParallels, int nMeridians);
    static int GetSphereTriangles(int nParallels, int nMeridians);

  public:
    //! Sets the origin of the primitive
    void SetOrigin(Matrix4Par origin);
    //! Applies a force with specified direction and intensity to the X vector
    void ApplyForceToX(Vector3Par direction, float intensity);
    //! Applies a force with specified direction and intensity to the Y vector
    void ApplyForceToY(Vector3Par direction, float intensity);
    //! Applies a force with specified direction and intensity to the Z vector
    void ApplyForceToZ(Vector3Par direction, float intensity);
    //! Applies gravitropism to the origin
    void ApplyGravitropism(float intensity);
    //! Applies geotropism to the origin
    void ApplyGeotropism(float intensity);
    //! Applies diatropism to the origin
    void ApplyDiatropism(Vector3Par diaOrigin, float intensity);
    void ApplyDiatropismX(Vector3Par diaOrigin, float intensity);
    void ApplyDiatropismY(Vector3Par diaOrigin, float intensity);
    void ApplyDiatropismZ(Vector3Par diaOrigin, float intensity);
    //! Applies bending according to a specified twist angle and bend angle
    void ApplyBending(float twistAngle, float bendAngle);
    //! Applies rotation according to a specified angle and X axis
    void ApplyRotationX(float angle);
    //! Applies rotation according to a specified angle and Y axis
    void ApplyRotationY(float angle);
    //! Applies rotation according to a specified angle and Z axis
    void ApplyRotationZ(float angle);
    //! Applies shift to the primitive
    void ApplyShift(Vector3Par shiftVector);
    //! Sets the way of drawing of the primitive
    void SetPolyplaneType(int polyplaneType);
    //! Returns polyplane type number
    int GetPolyplaneType();
    //! Draws the primitive, fills the slots
    virtual int Draw(
      CPrimitiveStream &ps,
      int sharedVertexIndex,
      int sharedVertexCount,
      bool isRoot,
      float detail,
      SSlot *pSlot) = 0;
    //! Add important points of the primitive to the point list
    virtual void UpdatePointList(CPointList &pointList) = 0;
    //! Draw the simplified version designed for special geometry purposes
    virtual int DrawBody(ObjectData *o, int &bodyId) {return 0;};

    static void RegisterCommands(GameState &gState);
  };

  // ------------------------------------------
  // Following code is here for script purposes

  #define TYPES_PRIMITIVE(XX, Category) \
  XX("Primitive",GamePrimitive,CreatePrimitive,"@Primitive","Primitive","Primitive",Category)\

  TYPES_PRIMITIVE(DECLARE_TYPE,Category)


  typedef CPrimitive *GamePrimitiveType;

  class GameDataPrimitive: public GameData {
	  typedef GameData base;

	  GamePrimitiveType _value;

	  public:
	  GameDataPrimitive():_value(0){}
	  GameDataPrimitive(GamePrimitiveType value):_value(value){}
	  ~GameDataPrimitive(){}

	  const GameType &GetType() const {return GamePrimitive;}
	  GamePrimitiveType GetPrimitive() const {return _value;}

	  RString GetText() const;
	  bool IsEqualTo(const GameData *data) const;
	  const char *GetTypeName() const {return "primitive";}
	  GameData *Clone() const {return new GameDataPrimitive(*this);}

	  LSError Serialize(ParamArchive &ar);	

	  USE_FAST_ALLOCATOR
  };

  CPrimitive *GetPrimitive(GameValuePar oper);

  // ------------------------------------------

  class CPrimitiveWithTexture: public CPrimitive {

  protected:
    //! Name of the associated texture
//    RString _textureName;
    //! Index of the associated texture
    int _textureIndex;
  public:
        virtual void SetTextureStage(int index) {_textureIndex = index;}
        virtual int GetTextureStage() const {return _textureIndex;}
  };

  class CPrimitiveConeType: public CPrimitiveWithTexture
  {
    int _minSides;
    int _maxSides;
    float _a0,  _a1, _a2;
  public:
    CPrimitiveConeType():_minSides(CONE_EDGESNUM_MIN),_maxSides(CONE_EDGESNUM_MAX),
      _a0(0),_a1(50),_a2(0) {}

      void SetConeParams(int sides) {_minSides=_maxSides=sides;}
      void SetConeParams(int mmin, int mmax, float a1)
      {
        _minSides=__min(mmin,mmax);
        _maxSides=__max(mmin,mmax);
        _a0=_a2=0;
        _a1=a1;
      }
      void SetConeParams(int mmin, int mmax, float a0, float a1, float a2=0.0f)
      {
        _minSides=__min(mmin,mmax);
        _maxSides=__max(mmin,mmax);
        _a0=a0;
        _a2=a2;
        _a1=a1;
      }    

    int CalcConeSideNum(float width, float detail)
    {
      float f=detail*width;
      int sd=toLargeInt(_a2*f*f+_a1*f+_a0);
      if (sd>_maxSides) sd=_maxSides;
      if (sd<_minSides) sd=_minSides;
      return sd;
    }
  };
}
#endif
