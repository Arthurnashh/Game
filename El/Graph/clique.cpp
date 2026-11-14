#include <Es/essencepch.hpp>
#include <El/Graph/clique.hpp>
#include <Es/Algorithms/qsort.hpp>

///clique comparition (should be qsorted before!)
bool CliqueEquals(AutoArray<int> &cl1, AutoArray<int> &cl2)
{
  if (cl1.Size()!=cl2.Size()) return false;
  for (int i=0, siz=cl1.Size(); i<siz; i++)
  {
    if (cl1[i]!=cl2[i]) return false;
  }
  return true;
}

///results of POSET comparison
enum ZoneComparitionResult
{
  ZONES_INCOMPARABLE,
  ZONES_LESS,
  ZONES_EQUAL,
  ZONES_GREATER
};

/// helper for finding the zones in the graph
class ZoneGraph : public AdjacencyMatrix
{
public:
  typedef AdjacencyMatrix base;
  ///Zone is the set of vertexes, which has the very same row in adjacency matrix
  class Zone
  {
  public:
    int size;    //number of vertexes inside zone
    int rowNum;  //AdjacencyMatrix index of row
    int zoneIx;  //number of zone in zones array

    Zone() {};
    Zone(int row, int siz) : rowNum(row), size(siz) {}

    bool Equivalent(Zone &zone, AdjacencyMatrix &adjMat);
    ClassIsMovable(Zone);
  };

  ///vertexZone[ix] contains number of zone containing vertex ix (ix==row in adjMat)
  AutoArray<int>  vertexZone;

  ZoneGraph(AdjacencyMatrix &adjmat);
  ///get max clique for each vertex of adjMat
  void GetCliques(Cliques &retClique);

private:
  AdjacencyMatrix &adjMat;
  ///zones by index
  AutoArray<Zone> zones;
};

ZoneGraph::ZoneGraph(AdjacencyMatrix &adjmat) 
: adjMat(adjmat)
{
  vertexZone.Resize(adjMat.Size());
  //get zones and vertex to zone incidence
  for (int i=0, siz=adjMat.Size(); i<siz; i++)
  {
    int ix=-1;
    Zone newZone(i,1);  //maybe it has been added for another vertex from this zone yet
    for (int j=0, zsiz=zones.Size(); j<zsiz; j++)
    {
      if (newZone.Equivalent(zones[j],adjMat))
      {
        zones[j].size++;
        vertexZone[i]=j;
        ix=j;
        break;
      }
    }
    if (ix<0) //add as new zone
    {
      vertexZone[i]=newZone.zoneIx=zones.Size();
      zones.Add(newZone);
    }
  }
  _edge.Dim(zones.Size(),zones.Size());

  for (int i=0, siz=Size(); i<siz; i++)
  {
    for (int j=i+1; j<siz; j++)
    {
      (*this)(i,j) = (*this)(j,i) = adjmat(zones[i].rowNum,zones[j].rowNum);
    }
  }
  for (int i=0, siz=Size(); i<siz; i++) (*this)(i,i)=true;
}

bool ZoneGraph::Zone::Equivalent(Zone &zone, AdjacencyMatrix &adjMat)
{
  int zone2=zone.rowNum;
  for (int i=0, siz=adjMat.Size(); i<siz; i++)
  {
    if (adjMat(rowNum,i)!=adjMat(zone2,i)) return false;
  }
  return true;
}

/// integer comparison used for QSort
static inline int CompareInt(const int *a1, const int *a2)
{
  return *a1-*a2;
}

void ZoneGraph::GetCliques(Cliques &retClique)
{
  //for each zone, find the maximal clique (clique not contained in any larger clique)
  Cliques zoneCliques;
  for (int i=0, siz=Size(); i<siz; i++)
  {
    AutoArray<int> newclique;
    newclique.Add(i);
    bool cliqueEnlarged=false;
    do 
    {
      for (int j=0; j<siz; j++)
      { //check, if there is still some vertex adjacent to all newclique vertices
        if (i==j) continue;
        bool addVertex=true;
        for (int k=0, clSiz=newclique.Size(); k<clSiz; k++)
        {
          if (!(*this)(j,newclique[k])) {addVertex=false; break;}
        }
        if (addVertex) newclique.Add(j);
      }
    } while (cliqueEnlarged);
    QSort(newclique, CompareInt);
    bool cliqueIsNew=true;
    for (int clNum=0, clSiz=zoneCliques.Size(); clNum<clSiz; clNum++)
    {
      if (CliqueEquals(newclique, zoneCliques[clNum])) { cliqueIsNew=false; break;}
    }
    if (cliqueIsNew) zoneCliques.Add(newclique);
  }
  //convert zoneCliques to retClique using vertexZone
  int vSiz = vertexZone.Size();
  for (int i=0, zonNum=zoneCliques.Size(); i<zonNum; i++)
  {
    AutoArray<int> clique;
    for (int j=0, siz=zoneCliques[i].Size(); j<siz; j++)
    {
      for (int k=0; k<vSiz; k++) 
        if (vertexZone[k]==zoneCliques[i][j]) clique.Add(k);
    }
    retClique.Add(clique);
  }
}

void AdjacencyMatrix::FindCliques(AutoArray< AutoArray<int> > &cliques)
{
  ZoneGraph zoneGraph(*this);
  zoneGraph.GetCliques(cliques);
}

#if EXAMPLE
int main()
{
  bool mat[13][13] = 
  {
  /*0*/ {1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,0,1,1,1,0,1,1,1,1,1,1},
  /*2*/ {1,0,1,0,1,0,1,0,1,0,0,0,0},
        {1,1,0,1,1,1,0,1,1,1,1,1,1},
  /*4*/ {1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,0,1,1,1,0,1,1,1,1,1,1},
  /*6*/ {1,0,1,0,1,0,1,0,1,0,0,0,0},
        {1,1,0,1,1,1,0,1,1,0,1,1,1},
  /*8*/ {1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,0,1,1,1,0,0,1,1,0,1,1},
  /*10*/{1,1,0,1,1,1,0,1,1,0,1,1,1},
        {1,1,0,1,1,1,0,1,1,0,1,1,0},
  /*12*/{1,1,0,1,1,1,0,1,1,0,1,0,1}
  };
  //create it
  AdjacencyMatrix adjMatrix(13);
  for (int i=0; i<13; i++)
    for (int j=0; j<13; j++)
      adjMatrix(i,j)=mat[i][j];

//////////////////////////////////////////////////////////////////////////
  ZoneGraph zoneGraph(adjMatrix);
  Cliques cliques;
  zoneGraph.GetCliques(cliques);
//////////////////////////////////////////////////////////////////////////
  //just print it for test
  for (int i=0; i<cliques.Size(); i++)
  {
    printf("Clique number %d is: ",i);
    for (int j=0; j<cliques[i].Size(); j++)
    {
      printf("%d, ", cliques[i][j]);
    }
    printf("\n");
  }

  return 0;
}
#endif
