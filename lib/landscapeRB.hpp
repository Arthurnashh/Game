#include "landscape.hpp"
//#include <Marina/src/marina.hpp>
#include "thingrb.hpp"

class LandscapeRB : public Landscape, public Geometry
{
protected:
  // temporary data.
  FindArray<Vector3> _vertex;
  AutoArray<MFace> _face;
  AutoArray<MEdge> _edge;

  bool IsTriviallyOver(const AABox& );
  void PrepareDataCD(const AABox& );
  void FindEdges();
  void CalcDirs();
  void FilterLandPairs(RBContactArray& finalContacts, CDContactArray& foundPairs,float scale, MeshGeometry& geom);
  void AdjustScaleOnDist(float &dist, float& scale, const Plane& sepPlane, MeshGeometry& cSecond);
public:
  LandscapeRB( Engine *engine, World *world, bool nets=false ) : Landscape( engine, world, nets), Geometry(EGT_LANDSCAPE, 0.02f,0) {_enclosingRectangle.Enlarge(10000000);};  //TODO later to it somehow better
  virtual ~LandscapeRB() {};

  IRBGeometry * Clone() const {return NULL;}; // never clone landscape geometry

public:
  virtual void CalculateEnclosingRectangle() {};

  virtual bool IsIntersection( IRBGeometry& cSecond, int simCycle);
  virtual bool ForceInterFree( IRBGeometry& cSecond, int simCycle); // return FALSE if bodies must be ignored.
  virtual void Scale(float scale) {};
  virtual void NormScale() {};

  virtual int FindContacts( RBContactArray& cContactPoints, IRBGeometry& cSecond, int simCycle); 

  int NFaces() const {return _face.Size();};
  virtual int NumVertices() const {return _vertex.Size();};
  int NEdges() const {return _edge.Size();};
  virtual Vector3Val GetVertex(int i) const {return _vertex[i];}; 
  MPlaneVal GetPlane(int i) const {return _face[i];};
  MFaceVal GetFace(int i) const {return _face[i];};
  MEdgeVal GetEdge(int i) const {return _edge[i];};

  void OnKickOffFromPhysEngine() {};
  
  ///////////////////////////
  // Debug
  void AddForce(Vector3Par pos, Vector3Par force, Color color = HWhite );
};