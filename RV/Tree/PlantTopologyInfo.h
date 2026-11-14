#pragma once


namespace TreeEngine
{
  struct SComponentChild;
  class IPlantControlDB;
  class CPointList;

  struct TopologyWeights {
      ///factor for age difference
      float age_diffFact;
      ///factor for importanci difference
      float importancy_diffFact;
      ///factor for difference of components
      float component_diffFact;
      ///factor for difference of childs
      float child_diffFact;
      ///factor for position
      float position_diffFact;
      ///factor for direction
      float direction_diffFact;
      ///factor for sizecoef
      float sizecoef_diffFact;

      void LoadWeightsFromDB( IPlantControlDB &db);
  };

  class PlantTopologyInfo
  {
    struct NodeInfo
    {
      float importancy; ///<importance of current node
      float age;      ///<age of current node
      int componentIndex; ///<ID of component
      unsigned int numChilds;  ///<number of children followed this node
      unsigned int firstChild; ///<index of the first child in the node info array
      int branchTypeId;
      Vector3 position;
      Vector3 direction;
      ClassIsSimpleZeroed(PlantTopologyInfo::NodeInfo);      
    };

    AutoArray<NodeInfo> _nodeList;
    float _rotation;
    Vector3 _boundingMax;
    Vector3 _boundingMin;
    mutable float _sizeCoef;
    mutable float _length;

    
    

  public:
    PlantTopologyInfo():_rotation(0) {}
    ///Adds a root node and returns its index
    unsigned int AddRootNode();
    ///Sets information to node.
    void SetNodeInfo(unsigned int nodeIndex, const SComponentChild &node);
    ///Allocates space for children. This must be called  once per node.
    bool AllocateNodeChildren(unsigned int nodeIndex, unsigned int childCount);
    ///Returns index of child node
    unsigned int GetNodeChildId(unsigned int nodeIndex, unsigned int childIndex) const;
    ///Returns count of child nodes
    unsigned int GetNodeChildrenCount(unsigned int nodeIndex) const;

    unsigned int GetCountOfNodes() const {return _nodeList.Size();}    

    ///Calculates difference of two topologies.
    float GetDifferenceFactor(const PlantTopologyInfo &other, const TopologyWeights &weights, float ceilHint=FLT_MAX) const;

    ///Calculates difference of two sub-topologies
    /**
    * @param other other topology
    * @param thisNodeIndex index of first node on this topology
    * @param otherNodeIndex index of first node on other topology
    */
    float GetDifferenceFactor(const PlantTopologyInfo &other, const TopologyWeights &weights, int thisNodeIndex, int otherNodeIndex, float ceilHint, float normSize1, float normSize2) const;

    void FinalizeTopology(const Matrix4 &origin);
    float GetTotalVolume() const;
    float GetBoundingSize() const;
    float GetSizeCoeficient() const;
    float GetSubBranchLength(const Vector3 &startPos=Vector3(0,0,0), int branch=0) const;
    float GetTopologyLength() const;

    void dumpTopology(const char *filename, int nodeIndex, bool append);
    static float getSizeCoef(const CPointList &pointList, const Vector3 &origin);
  };


}