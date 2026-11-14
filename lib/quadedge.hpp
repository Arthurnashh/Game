#ifndef _POSEIDON_LIB_QUADEDGE_HPP
#define _POSEIDON_LIB_QUADEDGE_HPP

#include "Es/Framework/debugLog.hpp"
#include "Es/Containers/array.hpp"
#include <cmath>

namespace Delaunay {

class Vector2d {
  public:
   float x, y;
   Vector2d():x(0),y(0) {}
   Vector2d(float a, float b):x(a),y(b) {}
   float norm() const { return sqrt(x*x + y*y); }
};

class Point2d : public RefCount {
  public:
   float x, y;
   Point2d() { x = 0; y = 0; }
   Point2d(float a, float b) { x = a; y = b; }
   Vector2d operator-(Point2d const& p) const { return Vector2d(x - p.x, y - p.y); }
   int operator==(Point2d const& p) const { return ((*this - p).norm() < 1e-6); }
};

class Line {
  private:
   float a, b, c;
  public:
   Line() {}
   Line(Point2d const& p, Point2d const& q) {
      Vector2d t = q - p;
      float len = t.norm();
      a =   t.y / len;
      b = - t.x / len;
      c = -(a*p.x + b*p.y);
   }
   float eval(Point2d const& p) const { return (a * p.x + b * p.y + c); }
};


class QuadEdge;

class Edge {
   friend class QuadEdge;
   friend void Splice(Edge*, Edge*);
  private:
   int index;  // index (0..3) in the containing QuadEdge
   Edge* next;
   Ref<Point2d> data;
   Edge& operator=(Edge const& rhs);  // don't copy
   Edge(Edge const& rhs);             // don't copy

  public:
   Edge()         { data = 0; }
   // Return the dual of the current edge, directed from its right to its left.
   Edge* Rot()    { return index<3 ? this+1 : this-3; }
   // Return the dual of the current edge, directed from its left to its right.
   Edge* invRot() { return index>0 ? this-1 : this+3; }
   // Return the edge from the destination to the origin of the current edge.
   Edge* Sym()    { return index<2 ? this+2 : this-2; }

   // Return the next ccw edge around (from) the origin of the current edge.
   Edge* Onext()  { return next; }
   // Return the next cw edge around (from) the origin of the current edge.
   Edge* Oprev()  { return Rot()->Onext()->Rot(); }
   // Return the next ccw edge around (into) the destination of the current edge.
   Edge* Dnext()  { return Sym()->Onext()->Sym(); }
   // Return the next cw edge around (into) the destination of the current edge.
   Edge* Dprev()  { return invRot()->Onext()->invRot(); }
   // Return the ccw edge around the left face following the current edge.
   Edge* Lnext()  { return invRot()->Onext()->Rot(); }
   // Return the ccw edge around the left face before the current edge.
   Edge* Lprev()  { return Onext()->Sym(); }
   // Return the edge around the right face ccw following the current edge.
   Edge* Rnext()  { return Rot()->Onext()->invRot(); }
   // Return the edge around the right face ccw before the current edge.
   Edge* Rprev()  { return Sym()->Onext(); }

   Ref<Point2d> Org() { return data; }
   Ref<Point2d> Dest(){ return Sym()->data; }
   Point2d const& Org2d() const { return *data; }
   Point2d const& Dest2d() const { return *(index<2 ? this+2 : this-2)->data; }
   void SetEndpoints(Ref<Point2d> origin, Ref<Point2d> dest) { data = origin; Sym()->data = dest; }

   QuadEdge* Quadedge() { return (QuadEdge*)(this - index); }
};

class QuadEdge {
   friend Edge* MakeEdge();
   friend class Subdivision;
  private:
   Edge e[4];
   bool toDelete;
  public:
   QuadEdge() {
      e[0].index = 0; e[0].next = &e[0];
      e[1].index = 1; e[1].next = &e[3];
      e[2].index = 2; e[2].next = &e[2];
      e[3].index = 3; e[3].next = &e[1];
      toDelete = false;
   }
};

class Subdivision {
  private:
   Edge* startingEdge;
   Edge* Locate(Point2d const&);
  public:
   Subdivision(Point2d const&, Point2d const&, Point2d const&);
   ~Subdivision();
   float InsertSite(Point2d const&);  // returns distance to closest point
};



/*********************** Basic Topological Operators ************************/

Edge* MakeEdge()
{
   QuadEdge* q = new QuadEdge;
   return q->e;
}

// This operator affects the two edge rings around the origins of a and b,
// and, independently, the two edge rings around the left faces of a and b.
// In each case, (i) if the two rings are distinct, Splice will combine them into one;
// (ii) if the two are the same ring, Splice will break it into two separate pieces.
// Thus, Splice can be used both to attach the two edges together,
// and to break them apart. See Guibas and Stolfi (1985) p.96 for
// more details and illustrations.
void Splice(Edge* a, Edge* b)
{
   Edge* alpha = a->Onext()->Rot();
   Edge* beta  = b->Onext()->Rot();

   Edge* t1 = b->Onext();
   Edge* t2 = a->Onext();
   Edge* t3 = beta->Onext();
   Edge* t4 = alpha->Onext();

   a->next = t1;
   b->next = t2;
   alpha->next = t3;
   beta->next = t4;
}

void DeleteEdge(Edge* e)
{
   Splice(e, e->Oprev());
   Splice(e->Sym(), e->Sym()->Oprev());
   delete e->Quadedge();
}

/************* Topological Operations for Delaunay Diagrams *****************/

// Initialize a subdivision to the triangle defined by the points a, b, c.
Subdivision::Subdivision(Point2d const& a, Point2d const& b, Point2d const& c)
{
   Ref<Point2d> da = new Point2d(a), db = new Point2d(b), dc = new Point2d(c);
   Edge* ea = MakeEdge(); ea->SetEndpoints(da, db);
   Edge* eb = MakeEdge(); Splice(ea->Sym(), eb); eb->SetEndpoints(db, dc);
   Edge* ec = MakeEdge(); Splice(eb->Sym(), ec); ec->SetEndpoints(dc, da); Splice(ec->Sym(), ea);
   startingEdge = ea;
}

Subdivision::~Subdivision()
{
  AutoArray<QuadEdge *> destroyList;
  StackArray<QuadEdge *> stack;
  stack.Push(startingEdge->Quadedge());
  while ( !stack.Empty() )
  {
    QuadEdge *q = stack.Top();
    stack.Pop();
    if (q->toDelete) continue;
    q->toDelete = true;
    destroyList.Add(q);
    stack.Push(q->e[0].Onext()->Quadedge());
    stack.Push(q->e[1].Onext()->Quadedge());
    stack.Push(q->e[2].Onext()->Quadedge());
    stack.Push(q->e[3].Onext()->Quadedge());
  }
  for (int i=0; i<destroyList.Size(); i++)
  {
    delete destroyList[i];
  }
}

// Add a new edge e connecting the destination of a to the
// origin of b, in such a way that all three have the same
// left face after the connection is complete.
// Additionally, the data pointers of the new edge are set.
Edge* Connect(Edge* a, Edge* b)
{
   Edge* e = MakeEdge();
   Splice(e, a->Lnext());
   Splice(e->Sym(), b);
   e->SetEndpoints(a->Dest(), b->Org());
   return e;
}

// Essentially turns edge e counterclockwise inside its enclosing
// quadrilateral. The data pointers are modified accordingly.
void Swap(Edge* e)
{
   Edge* a = e->Oprev();
   Edge* b = e->Sym()->Oprev();
   Splice(e, a);
   Splice(e->Sym(), b);
   Splice(e, a->Lnext());
   Splice(e->Sym(), b->Lnext());
   e->SetEndpoints(a->Dest(), b->Dest());
}

/*************** Geometric Predicates for Delaunay Diagrams *****************/

// Returns twice the area of the oriented triangle (a, b, c),
// i.e. the area is positive if the triangle is oriented counterclockwise.
inline float TriArea(Point2d const& a, Point2d const& b, Point2d const& c)
{
   return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
}

// Returns true if the point d is inside the circle defined by the
// points a, b, c. See Guibas and Stolfi (1985) p.107.
bool InCircle(Point2d const& a, Point2d const& b, Point2d const& c, Point2d const& d)
{
   return (a.x*a.x + a.y*a.y) * TriArea(b, c, d) -
          (b.x*b.x + b.y*b.y) * TriArea(a, c, d) +
          (c.x*c.x + c.y*c.y) * TriArea(a, b, d) -
          (d.x*d.x + d.y*d.y) * TriArea(a, b, c) > 0;
}

// Is the point p to the right of the edge e?
bool RightOf(Point2d const& p, Edge* e)
{
   return TriArea(p, e->Dest2d(), e->Org2d()) > 0;
}

// A predicate that determines if the point x is on the edge e.
// The point is considered on if it is in the EPS-neighborhood of the edge.
bool OnEdge(Point2d const& x, Edge* e)
{
   float t1, t2, t3;
   t1 = (x - e->Org2d()).norm();
   t2 = (x - e->Dest2d()).norm();
   if (t1 < 1e-6 || t2 < 1e-6)
       return true;
   t3 = (e->Org2d() - e->Dest2d()).norm();
   if (t1 > t3 || t2 > t3)
       return false;
   Line line(e->Org2d(), e->Dest2d());
   return fabs(line.eval(x)) < 1e-6;
}

/************* An Incremental Algorithm for the Construction of Delaunay Diagrams *********************************/

// Returns an edge e, s.t. either x is on e, or e is an edge of
// a triangle containing x. The search starts from startingEdge
// and proceeds in the general direction of x. Based on the
// pseudocode in Guibas and Stolfi (1985) p.121.
Edge* Subdivision::Locate(Point2d const& x)
{
   Edge* e = startingEdge;

   while (true) {
      if (x == e->Org2d() || x == e->Dest2d())
          return e;
      else if (RightOf(x, e))
          e = e->Sym();
      else if (!RightOf(x, e->Onext()))
          e = e->Onext();
      else if (!RightOf(x, e->Dprev()))
          e = e->Dprev();
      else
          return e;
   }
}

// Inserts a new point into a subdivision representing a Delaunay
// triangulation, and fixes the affected edges so that the result
// is still a Delaunay triangulation. This is based on the
// pseudocode from Guibas and Stolfi (1985) p.120, with slight
// modifications and a bug fix.
// Returns the point in the old subdivision that's closest to the new site.
float Subdivision::InsertSite(Point2d const& x)
{
   float minDist = FLT_MAX;

   Edge* e = Locate(x);

   // Is point already in? Don't add it.
   if (x == e->Org2d())
      return 0.0f;
   else if (x == e->Dest2d())
      return 0.0f;
   else if (OnEdge(x, e)) {
      e = e->Oprev();
      DeleteEdge(e->Onext());
   }

   // Connect the new point to the vertices of the containing
   // triangle (or quadrilateral, if the new point fell on an
   // existing edge.)
   Edge* base = MakeEdge();
   base->SetEndpoints(e->Org(), new Point2d(x));
   Splice(base, e);
   startingEdge = base;

   do {
      base = Connect(e, base->Sym());
      e = base->Oprev();
   } while (e->Lnext() != startingEdge);

   // Examine suspect edges to ensure that the Delaunay condition is satisfied.
   do {
      Edge* t = e->Oprev();

      saturateMin(minDist, (x - e->Org2d()).norm());  // new candidate for closest point

      if (RightOf(t->Dest2d(), e) && InCircle(e->Org2d(), t->Dest2d(), e->Dest2d(), x)) {
         Swap(e);
         e = e->Oprev();
      }
      else if (e->Onext() == startingEdge)  // no more suspect edges
         return minDist;
      else  // pop a suspect edge
         e = e->Onext()->Lprev();
   } while (true);
}

}  // namespace Delaunay

#endif
