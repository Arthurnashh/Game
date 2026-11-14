#ifdef _MSC_VER
#pragma once
#endif

#ifndef _A_STAR_HPP
#define _A_STAR_HPP

#define DIAG_A_STAR 0

#include <Es/Memory/normalNew.hpp>

/// node used for A* searching
template <class Field>
struct AStarNode
{
  /// searched field
  Field _field;
  /// node from which we have arrived
  AStarNode *_parent;
  /// cost to get from staring point to this node
  float _g;
  /// _g plus heuristics to get to the end point
  float _f;
  /// indication if the node is open
  int _open;

  AStarNode(const Field &field, AStarNode *parent, float g, float f)
    : _field(field)
  {
    _parent = parent;
    _g = g;
    _f = f;
    _open = 0;	// negative if not open, future use - index in OpenList 
  }

  // operator == for Field is required
  bool operator == (const AStarNode &with) const {return _field == with._field;}

#if DIAG_A_STAR >= 2
  RString GetDebugName() const {return Format("%s, g=%g, f=%g", cc_cast(_field.GetDebugName()), _f, _g);}
#endif

  USE_FAST_ALLOCATOR;
};
#include <Es/Memory/debugNew.hpp>

/// provide "NULL" base - by default there is no interface
class AStarNullBase {};

template <class Field>
struct AStarCostFunctionsDef
{
  typedef AStarNode<Field> Node;
  
  
  struct CostFunction
  {
    // by default we assume distance is the cost 
    float operator () (const Field &a, const Field &b) const {return a.Distance(b);};
  };
  
  struct HeuristicFunction
  {
    // by default we assume distance is the heuristics
    float operator () (const Field &a, const Field &b) const {return a.Distance(b);};
  };
  
  /// how should be heuristics combined with cost to create a total cost
  struct AddHeuristics
  {
    /// by default add them
    float operator () (const Field &curr, const Field &next, float g, float h) const {return g+h;}
  };

  /** having following functors as bases would allow Empty Base Optimization,
  however the code would be hard to read and the benefit is doubtful, as usually AStar does not exist in many instances
  */
  
};

template <class Field>
struct AStarCostFunctions: AStarCostFunctionsDef<Field>
{
};

/// this class makes sure any types added in AStarCostFunctionsDef are added as members as well
template <class CostFunctions>
struct AStarCostFunctionsWrapped: public CostFunctions
{
  typedef typename CostFunctions::CostFunction CostF;
  typedef typename CostFunctions::HeuristicFunction HeuristicF;
  typedef typename CostFunctions::AddHeuristics AddHeuristicsF;
  
  CostF _costFunction;
  HeuristicF _heuristicFunction;
  AddHeuristicsF _addHeuristicsFunction;
  
  /**
  we assume the caller constructs all classes he is aware of.
  The rest is handled by default construction.
  */
  AStarCostFunctionsWrapped(
    const CostF &cost/*=CostF()*/,
    const HeuristicF &heur/*=HeuristicF()*/,
    const AddHeuristicsF &add=AddHeuristicsF()
  )
  :_costFunction(cost),_heuristicFunction(heur),_addHeuristicsFunction(add)
  {}
};

#define ASTAR_TEMPLATE \
template < \
  class Field, \
  class CostFunctions, class ReadyFunction, class InterruptFunction, class EndFunction, \
  class Iterator, \
  class ClosedList, class OpenList, \
  class Params, class Base, class CostFunctionsWrapped \
>

#define ASTAR_ARGS \
  Field, CostFunctions, ReadyFunction, InterruptFunction, EndFunction, \
  Iterator, ClosedList, OpenList, Params, Base, CostFunctionsWrapped

/// A* template
template <
  class Field,
  class CostFunctions, class ReadyFunction, class InterruptFunction, class EndFunction,
  class Iterator,
  class ClosedList, class OpenList,
  class Params, class Base=AStarNullBase, class CostFunctionsWrapped=AStarCostFunctionsWrapped<CostFunctions>
>
class AStar: public Base, public CostFunctionsWrapped
{
protected:
  typedef AStarNode<Field> Node;

  bool _done;
  Node *_found;
  /// start may be usefull for diagnostics
  Field _start;
  /// destination is needed during the search
  Field _destination;

  /// list of nodes visited
  ClosedList _closed;
  /// list of all nodes which are not closed yet
  OpenList _open;
  ReadyFunction _readyFunction;
  InterruptFunction _interruptFunction;
  EndFunction _endFunction;
  
  using CostFunctionsWrapped::_costFunction;
  using CostFunctionsWrapped::_heuristicFunction;
  using CostFunctionsWrapped::_addHeuristicsFunction;

  // current internal state (enable to interrupt Process inside iterator loop)
  Node *_best;
  Iterator _iterator;

public:
  AStar(
    const Field &start, const Field &destination,
    const CostFunctionsWrapped &costFunctions,
    const ReadyFunction &readyFunction, const InterruptFunction &interruptFunction,
    const EndFunction &endFunction
  )
    : CostFunctionsWrapped(costFunctions),
    _start(start),_destination(destination),
    _readyFunction(readyFunction), _interruptFunction(interruptFunction), _endFunction(endFunction)
  {
    _done = false;
    _found = NULL;
    _best = NULL;
    Init(start);
  }

  bool IsDone() const {return _done;}
  bool IsFound() const {return _found != NULL;}
  const Node *GetLastNode() const {return _found;}
  const Node *GetBestNode() const;
  const Node *GetBestNode(const void *context) const;
  int GetNodeCount() const {return _closed.NItems();}
  RString GetDiagText(bool cover) const {return Format("nodes %d",GetNodeCount());}

  //! Process maxIters iterations of A*
  /*!
  \return number of iterations really processed
  */
  int Process(void *context=NULL, bool diags=false);

  bool IsTargetReached(const Node *node, const void *context) const;
  
  //! Access to closed list used for diagnostics
  const ClosedList &GetClosedList() const {return _closed;}
  //! Access to open list used for diagnostics
  const OpenList &GetOpenList() const {return _open;}
  //! Access to currently expanding used for diagnostics
  const Node *GetBest() const {return _best;}
  /// Access to destination for consistency checks
  const Field &GetDestination() const {return _destination;}
  /// Access to destination for diagnostics purposes
  const Field &GetStart() const {return _start;}


  InterruptFunction &GetInterruptFunction() {return _interruptFunction;}

protected:
  void Init(const Field &start);

  void UpdateClosedList(Node *node, const Field &field, Node *parent, float g, float f);
};

ASTAR_TEMPLATE
const AStarNode<Field> *AStar<ASTAR_ARGS>::GetBestNode(const void *context) const
{
  typedef typename ClosedList::Iterator ClosedIterator;
  
  const Node *best = NULL;

  // search in closed list
  float minVal = FLT_MAX;
  for (ClosedIterator iterator(_closed); iterator; ++iterator)
  {
    const Node *node = *iterator;

    // CHANGED: find the nearest node in sense of distance rather than heuristic (this can include some unwanted parts)
    float val = node->_field.Distance(_destination, context);
    if (val < minVal)
    {
      best = node;
      minVal = val;
    }
  }
  return best;
}

ASTAR_TEMPLATE
const AStarNode<Field> *AStar<ASTAR_ARGS>::GetBestNode() const
{
  typedef typename ClosedList::Iterator ClosedIterator;
  
  const Node *best = NULL;

  // search in closed list
  float minVal = FLT_MAX;
  for (ClosedIterator iterator(_closed); iterator; ++iterator)
  {
    const Node *node = *iterator;

    // CHANGED: find the nearest node in sense of distance rather than heuristic (this can include some unwanted parts)
    float val = node->_field.Distance(_destination);
    if (val < minVal)
    {
      best = node;
      minVal = val;
    }
  }
  return best;
}

ASTAR_TEMPLATE
bool AStar<ASTAR_ARGS>::IsTargetReached(const Node *node, const void *context) const
{
  float dist2 = node->_field.Distance(_destination,context);
  return dist2<Square(2.0);
}

ASTAR_TEMPLATE
void AStar<ASTAR_ARGS>::Init(const Field &start)
{
  float h = _heuristicFunction(start, _destination);
  // enable to use custom (for example thread safe) allocation
  Node *node = _closed.AddNew(start, NULL, 0, h);
  /*
  Node *node = new Node(start, NULL, 0, h);
  _closed.Add(node);
  */
  _open.Add(node);
}

/// default implementation for terminating condition - terminate when destination is reached
template <class Field>
struct AStarEndDefault
{
  Field _destination;
  
  AStarEndDefault(const Field &destination):_destination(destination){}
  
  bool operator () (AStarNode<Field> *best) const
  {
    return best->_field == _destination;
  }
};

ASTAR_TEMPLATE
int AStar<ASTAR_ARGS>::Process(void *context, bool diags)
{
  _interruptFunction.Start();
  int i = 0;

  // main loop, in each step expand single node from the open list
  while (true)
  {
    if (!_best) // otherwise, we are continuing with the interrupted task
    {
      // select best node from open list
      Node *best = NULL;
      if (!_open.HeapGetFirst(best))
      {
        // open list is empty, path does not exist
#if DIAG_A_STAR
        if (diags)
        {
          LogF("  A* - path not found");
        }
#endif
        _done = true;
        return i;
      }
      Assert(best);
#if DIAG_A_STAR >= 2
      if (diags)
      {
        LogF("  A* - best node in open list is %s", cc_cast(best->GetDebugName()));
      }
#endif

      // check if destination is reached
      if (_endFunction(best))
      {
        // path found
#if DIAG_A_STAR
        if (diags)
        {
          LogF("  A* - path found");
        }
#endif
        _done = true;
        _found = best;
        best->_open = -1;
        _open.HeapRemoveFirst();
        return i;
      }

      // check if the best field can be used
      bool ready = _readyFunction(best->_field);
      if (!ready)
      {
#if DIAG_A_STAR
        if (diags)
        {
          LogF("  A* - map not ready");
        }
#endif
        return i;
      }

      // check interrupt function here - inside Iterator constructor failure can occur
      _iterator = Iterator(best->_field, context);
      if (_interruptFunction()) return i;

      // we are ready to expand now
      best->_open = -1;
      _open.HeapRemoveFirst();
      _best = best;
    }

    DoAssert(_best);
    // expand all neighbors
    for (; _iterator; ++_iterator)
    {
      Field neighbour = _iterator;
      
      // ??? check for neighbour == best->_parent (performance testing needed)

      float g = _best->_g + _costFunction(_best->_field, neighbour);

      // check interrupt function here - inside _costFunction failure can occur
      if (_interruptFunction()) return i;

      if (g >= Params::LimitCost()) continue;
      
      float h = _heuristicFunction(neighbour, _destination);
      float f = _addHeuristicsFunction(_best->_field, neighbour, g, h);

      // check if neighbour was previously expanded
      Node *node = _closed[neighbour.GetKey()];
      if (!node)
      {
        // new node
        // enable to use custom (for example thread safe) allocation
        node = _closed.AddNew(neighbour, _best, g, f);
/*
        Node *node = new Node(neighbour, _best, g, f);
        _closed.Add(node);
*/
        _open.Add(node);
#if DIAG_A_STAR >= 2
        if (diags)
        {
          LogF("  A* - node added to open list %s", cc_cast(node->GetDebugName()));
        }
#endif
      }
      else if (f < node->_f)
      {
        // node is found and update is needed
        if (node->_open >= 0)
        {
          // found on open list
          node->_field = neighbour; // update additional info
          node->_parent = _best;
          node->_g = g;
          node->_f = f;
          _open.UpdateUp(node);
#if DIAG_A_STAR >= 2
          if (diags)
          {
            LogF("  A* - node in open list updated %s", cc_cast(node->GetDebugName()));
          }
#endif
        }
        else
        {
          // found on closed list
          // DO NOT PERFORM THIS
          // result path is better cca by 1%, spent time is double
          // UpdateClosedList(node, neighbour, best, g, f);
#if DIAG_A_STAR >= 2
          if (diags)
          {
            LogF("  A* - node in closed list will not be updated %s", cc_cast(node->GetDebugName()));
          }
#endif
        }
      }
    }
    i++;
    // inform the interrupt function (some are based on the number of iterations)
    _interruptFunction.Iteration();
    // select the next node from the open list
    _best = NULL;
  }
  Fail("Unreachable");
  return i;
}

#include <Es/Containers/staticArray.hpp>

ASTAR_TEMPLATE
void AStar<ASTAR_ARGS>::UpdateClosedList(Node *node, const Field &field, Node *parent, float g, float f)
{
  // update node
  node->_field = field; // update additional info
  node->_parent = parent;
  node->_g = g;
  node->_f = f;

  // add node to stack
  AUTO_STATIC_ARRAY(Node *, stack, 256);
  stack.Add(node);

  while (int n = stack.Size() > 0)
  {
    // pop node from stack
    Node *parent = stack[n - 1];
    stack.Delete(n - 1, 1);

    for (Iterator iterator(parent->_field); iterator; ++iterator)
    {
      // child node
      Field field = iterator;
      Node *kid = _closed[field.GetKey()];
      if (!kid) continue;

      float g = parent->_g + _costFunction(parent->_field, field);
      if (g < kid->_g)
      {
        // update child node
        kid->_field = field; // update additional info
        kid->_parent = parent;
        float h = kid->_f - kid->_g;
        kid->_g = g;
        kid->_f = g + h;

        if (kid->_open >= 0)
        {
          _open.UpdateUp(kid);
        }
        else
        {
          // add child node to stack
          stack.Add(kid);
        }
      }
    }
  }
}

#endif
