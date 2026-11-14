#ifndef _component_h_
#define _component_h_

#include <El/ParamFile/paramFile.hpp>
#include <El/Evaluator/express.hpp>
#include "polyplaneBlock.h"
#include "polyplaneBunch.h"
#include "polyplaneBlockBend.h"
#include "polyplaneBlockNorm.h"
#include "polyplaneBlockBend2.h"
#include "polyplaneBlockNorm2.h"
#include "polyplaneBlockSimple.h"
#include "polyplaneBlockSimple2.h"
//#include "polyplaneuniplane.h"
#include "vectorSpace.h"
#include "primitive.h"
#include "IPlantDB.h"

#define MAX_SLOT_COUNT 64
#define MAX_WALKTHROUGH_DEPTH 64
#define MAX_WALKTHROUGH_COUNT 65536

namespace TreeEngine
{


  // Structure of one vertex.
  struct SBranchVertex
  {
    // Vertex position.
    Vector3 Position;
    // Vertex normal.
    Vector3 Normal;
    // Diffuse color.
    D3DCOLOR Diffuse;
    // 1st set, 2-D.
    float U1, V1;
  };

  enum ETextureAdress {
    WRAP,
    MIRROR
  };

  //! High level parameters
  /*!
  Definition of this structure should stay outside the component module.?
  */
  /*
  struct SInputParameters {
  };

  struct SControlParameters {
  //! Texture length in vertical direction (V coordinate) already used by ancestors
  float _textureLength;
  //! Boolean value determines wheter child will be drawn as a polyplane
  int _renderAsPolyplane;
  //! Importancy of the branch beeing a bone.
  float _boneImportancy;
  };
  */

  //! Actual parameters
  /*!
  These parameters must be set for each child of the component. These
  parameters and constant parameters define the particular instance of the
  plant.
  */
  struct SActualParams
  {
    //! Position and orientation
    Matrix4 _origin;
    //! Random seed
    int _seed;
    //! Age
    float _age;
    //! Counter
    int _counter;
    //! Trunk
    float _trunk;
    //! Position of the diatropism origin
    Vector3 _diaOrigin;
    //! V offset of the texture corresponds to the current depth of the tree
    float _textureVOffset;
    //! Geometry spin relative to the child orientation
    float _geometrySpin;
    //! optional script parameters
    GameValue _optional;
    //! branch type
    int _branchTypeId;
    //! polyplane type
    int _polyplaneType;
    //! when true, polyplane is enforced (remain of geometry will be rendered to the polyplane)
    bool _forcePolyplane;

    SActualParams():_polyplaneType(PT_BLOCK),_branchTypeId(0),_forcePolyplane(false),_geometrySpin(0),
        _textureVOffset(0) {}
  };

  //! Internal parameters
  /*!
    These parameters are internal parameters, not visible to components or scripts
  */
  struct SInternalParams
  {
    bool _isRoot;
  };

  //! Constant parameters
  /*!
  These parameters remains unchanged through the creating of a whole
  structure. This structure contains such things like date, time,
  collision geometry, a.s.o.
  */
  struct SConstantParams
  {
    //! Detail to draw the plant with. Values are from interval <0, 1>
    float _detail;
    //! Hours. Proper values are from interval <0, 24)
    float _time;
    //! Months. Proper values are from interval <1, 12)
    float _month;
    //! Age of the plant
    float _plantAge;
  };

  //! Component child description
  /*!
  This structure determines an exact appearance of a whole subtree.
  Of course we must include the constant parameters.
  */
  struct SComponentChild
  {
    //! Index of the root component of the subtree
    int _componentIndex;
    //! Importancy of the branch - usually dependent on age and such staff
    float _importancy;
    //! Index of the slot of the primitive component
    int _slotIndex;
    //! Actual parameters of the subtree
    SActualParams _actualParams;
    //! Internal parameters
    SInternalParams _internalParams;
    ClassIsMovable(SComponentChild)
  };

    //! Buffer of component children
  class CComponentChildBuffer
  {
  private:
    //! Array of children
    AutoArray<SComponentChild> _children;
    //! Index of the newly added child
    int _newIntervalIndex;
  public:
    //! Constructor
    CComponentChildBuffer() {_newIntervalIndex = -1;}
    //! Add a child
    void AddChild(
      int componentIndex,
      float importancy,
      int slotIndex,
      const SActualParams &ap);
    //! Returns pointer to the specified child
    SComponentChild *GetChild(int i);
    //! Returns number of children
    int GetChildCount();
    //!{ Scope around child adding
    void BeginChildUpdate();
    void EndChildUpdate(const SInternalParams &ip);
    //!}

    static void RegisterCommands(GameState &gState);
  };

  //! Structure to hold one external parameter
  struct SVariable {
    //! Name of the parameter
    RString _name;
    //! Value of the parameter
    RString _value;
    ClassIsMovable(SVariable)
  };



  class CBBoxCounter;

  //! Component of the tree hierarchy structure
  class CComponent : public RefCount {
  protected:
    //! Name of the texture associated with the component
    RString _textureName;
    //! Name of the texture associated with the component used for saving purposes
    RString _textureSaveName;
    //! Index of the texture associated with the component
    int _textureIndex;
    //! U adressing type
    ETextureAdress _adressU;
    //! V adressing type
    ETextureAdress _adressV;
    //! Copy of list of component names
    AutoArray<RString> _componentNames;
    //! Parameters defined outside the script itself
    AutoArray<SVariable> _parameters;
    /// Parameter DB (new style)
    IPlantControlDB *_paramDB;
    /// Component name
    RString _name;

    //! Executes the component - creates children
    /*!
    Objects and methods needed in this method:
    - some CPrimitive ancestor and its methods
    - AddChild() method
    */
    virtual int Execute(
      const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer &ccb) = 0;
    //! Registers actual and constant parameters for the script purposes
    void RegisterActualAndConstantParameters(
      const SActualParams &ap,
      const SConstantParams &cp);
  public:
    CComponent():_paramDB(0) {}    
    //! Initializes the component with concrete values
    virtual void Init(
      RString folder,
      const ParamEntry &treeEntry,
      const ParamEntry &subEntry,
      const AutoArray<RString> &names);

    virtual void InitWithDb(IPlantControlDB *paramDB,  const Array<RString> &names);

    ///Sets default texture index
    void SetTextureIndex(int index)
    {
      _textureIndex=index;
    }
    ///Sets name of current component
    void SetComponentName(RString name)
    {
      _name=name;
    }
    ///Gets name of the component
    RString GetComponentName() const
    {
      return _name;
    }

    ///Retrieves pointer to plant control database
    IPlantControlDB *GetParamDatabase() const {return _paramDB;}

    //! Returns the primitive associated with the component
    virtual CPrimitive *GetPrimitive() = 0;

    //! Determine whether a new polyplane should be generated for this component (omit the comparison to the current set)
    virtual bool ForceNewPolyplane() {return false;}

    //! Returns the index and name of a texture associated with the component
    void GetTextureIndexAndName(int &index, RString &name, RString &saveName);
    void GetTextureAdress(ETextureAdress &adressU, ETextureAdress &adressV);
    RString GetComponentName(int componentIndex);



    int GetChildren(const SActualParams &ap,
                    const SInternalParams &ip,
                    const SConstantParams &cp,
                    CComponentChildBuffer &ccb);

    //! Generates some graphics for the tip of the branch in order to close it
    void CloseBranchTip(
      CPrimitiveStream &psBranch,
      int sharedVertexIndex,
      int sharedVertexCount);

//     int DrawPrimitive(
//       CPrimitiveStream &psBranch,
//       int sharedVertexIndex,
//       int sharedVertexCount,
//       float detail,
//       SSlot *pSlot);
    //! Recursive procedure draws a whole subtree
    /*!
    \return 1 if succeeded, 0 if some error occured.
    */
    int DrawSubtree(
      const RefArray<CComponent> &components,
      const SActualParams &ap,
      const SConstantParams &cp,
      CPrimitiveStream &psBranch,
      int &walkthroughCount,
      int walkthroughDepth = 0,
      int sharedVertexIndex = 0,
      int sharedVertexCount = 0,
      int *pPolyplaneType = NULL,
      CBBoxCounter *pBBoxCounter = NULL);

    void Walkthrough(
      const RefArray<CComponent> &Components,
      const SActualParams &ap,
      const SConstantParams &cp);
    //! Draws the component and a whole subtree
    int Draw(
      PDirect3DDevice &pD3DDevice,
      DWORD hVSColor,
      DWORD hPSColor,
      DWORD hVSNormal,
      DWORD hPSNormal,
      const RefArray<CComponent> &components,
      TVectorSpace<CPolyplane> &polyplaneSpace,
      const SActualParams &ap,
      const SConstantParams &cp,
      CPrimitiveStream &psBranch,
      CPrimitiveStream &psPolyplane,
      int sharedVertexIndex = 0,
      int sharedVertexCount = 0,
      int walkthroughDepth = 0,
      int walkthroughCount = 0,
      int disablePolyplanes = 0,
      CBBoxCounter *pBBoxCounter = NULL);
    // -----------------------------------
    // Methods used in the Execute command
    //! Returns a pseudorandom number relative to seed and base
    static float Rand(int seed);
    //! Returns a pseudorandom number - index to be considered as a next seed
    static int RandIndex(int seed);
    //! Returns a value close to "value" modified by specified parameters
    static float RandShake(int seed, float value, float interval);
    //! Returns a pseudorandom number with a gauss distribution
    static float RandGauss(int seed, float exp, float var, float interval);
    //! Returns value from curve at the index position
    static float GetLinearCurveValue(AutoArray<float> &aa, float index);
    //static float GetLinearNonUniformCurveValue(AutoArray<float> &aa, float index);
    static float GetQuadraticCurveValue(AutoArray<float> &aa, float index);
    //! Returns value from the curve specified by the aa at the index position
    /*!
    Note that first number in the array specifies the type of the curve.
    */
    static float GetCurveValue(AutoArray<float> &aa, float index);
    /*  


    static float GetQuadraticNonUniformCurveValue(AutoArray<float> &aa, float index);
    */

    //! Returns rounded number
    static float Round(float number);
    //! Returns index of specified name in the name array
    int GetIndexOfName(const RString &name);
    //! Prints specified text into debug window
    static void PrintNumber(const char *text, float num);
    // -----------------------------------
    static void RegisterCommands(GameState &gState);

    static const int ALotOfFaces=9999;
    virtual int EstimateFacesCount() const {return ALotOfFaces;}
  };

  // ------------------------------------------
  // Following code is here for script purposes

  #define TYPES_GAMECOMPONENTCHILDBUFFER(XX, Category) \
  XX("Primitive",GameComponentChildBuffer,CreateGameComponentChildBuffer,"@GameComponentChildBuffer","GameComponentChildBuffer","GameComponentChildBuffer",Category)\

  TYPES_GAMECOMPONENTCHILDBUFFER(DECLARE_TYPE,Category)



  typedef CComponentChildBuffer *GameComponentChildBufferType;

  class GameDataComponentChildBuffer: public GameData {
    typedef GameData base;

    GameComponentChildBufferType _value;

  public:
    GameDataComponentChildBuffer():_value(0) {}
    GameDataComponentChildBuffer(GameComponentChildBufferType value) : _value(value) {}
    ~GameDataComponentChildBuffer() {}

	  const GameType &GetType() const {return GameComponentChildBuffer;}
    GameComponentChildBufferType GetComponentChildBuffer() const {return _value;}

    RString GetText() const;
    bool IsEqualTo(const GameData *data) const;
    const char *GetTypeName() const {return "componentChildBuffer";}
    GameData *Clone() const {return new GameDataComponentChildBuffer(*this);}

    LSError Serialize(ParamArchive &ar);	

    USE_FAST_ALLOCATOR
  };

  #define TYPES_GAMECOMPONENT(XX, Category) \
  XX("Primitive",GameComponent,CreateGameComponent,"@GameComponent","GameComponent","GameComponent",Category)\

  TYPES_GAMECOMPONENT(DECLARE_TYPE,Category)

  typedef CComponent *GameComponentType;

  class GameDataComponent: public GameData {
    typedef GameData base;

    GameComponentType _value;

  public:
    GameDataComponent():_value(0) {}
    GameDataComponent(GameComponentType value) : _value(value) {}
    ~GameDataComponent() {}

	  const GameType &GetType() const {return GameComponent;}
    GameComponentType GetComponent() const {return _value;}

    RString GetText() const;
    bool IsEqualTo(const GameData *data) const;
    const char *GetTypeName() const {return "component";}
    GameData *Clone() const {return new GameDataComponent(*this);}

    LSError Serialize(ParamArchive &ar);	

    USE_FAST_ALLOCATOR
  };

  // ------------------------------------------

}
#endif