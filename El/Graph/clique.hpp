#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_CLIQUE_HPP
#define _EL_CLIQUE_HPP

#include <Es/essencepch.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Containers/array2D.hpp>

/// cliques of the graph for each vertex
typedef AutoArray< AutoArray<int> > Cliques;

/// graph represented as adjacency matrix
class AdjacencyMatrix
{
protected:
  Array2D<bool> _edge; 
  
public:
  AdjacencyMatrix() {};
  AdjacencyMatrix(int size)
  {
    _edge.Dim(size,size);
  }
  bool operator () (int row, int col) const {return _edge(col,row);}
  bool &operator () (int row, int col) {return _edge(col,row);}
  int Size() const {return _edge.GetXRange();}
  void Resize(int n) { _edge.Dim(n,n); }
  
  void FindCliques(AutoArray< AutoArray<int> > &cliques);
};


#endif
