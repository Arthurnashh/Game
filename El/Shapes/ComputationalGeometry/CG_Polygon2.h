#ifndef CG_POLYGON2_H
#define CG_POLYGON2_H

// -------------------------------------------------------------------------- //

#include <vector>

// -------------------------------------------------------------------------- //

#include ".\CG_Polygon2Vertex.h"
#include ".\CG_Triangle2.h"
#include ".\CG_Diagonal.h"

// -------------------------------------------------------------------------- //

using std::vector;

// -------------------------------------------------------------------------- //

typedef enum
{
	PolyTri_SimpleEarClipping
} PolygTriangulators;

// -------------------------------------------------------------------------- //

class CG_Polygon2
{
	// head of the circular list
	CG_Polygon2Vertex* m_Vertices;

	vector<CG_Triangle2> m_Triangulation;
	vector<CG_Diagonal>  m_Diagonals;

public:

	CG_Polygon2();
	CG_Polygon2(const CG_Polygon2& other);

	~CG_Polygon2();

	CG_Polygon2& operator = (const CG_Polygon2& other);

	// operations on vertices
	// -------------------------------------------------------------------------- //
	unsigned int VerticesCount() const;
	void AddVertex(const CG_Polygon2Vertex& vertex);
	void AddVertex(double x, double y);
	void AddVertex(double x, double y, unsigned int id);

	// returns a pointer to the vertex of this polygon which coincide with
	// the given point or NULL if no match is found
	CG_Polygon2Vertex* FindVertex(const CG_Point2& point) const;

	void RemoveAllVertices();

	const CG_Polygon2Vertex& GetVertex(unsigned int index) const;
	CG_Polygon2Vertex& GetVertex(unsigned int index);
	// -------------------------------------------------------------------------- //

	// polygons properties
	// -------------------------------------------------------------------------- //
	double Area() const;

	const vector<CG_Triangle2>& Triangulation() const;
	const vector<CG_Diagonal>& Diagonals() const;

	// -------------------------------------------------------------------------- //

	// returns true if the given segment coincides with an edge of this polygon
	// if the edge is segment.m_To - segment.m_From return false
	bool IsEdge(const CG_Segment2& segment) const;

	// returns true if the given segment coincides with a reversed edge of this polygon
	// if the edge is segment.m_From - segment.m_To return false
	bool IsReversedEdge(const CG_Segment2& segment) const;

	// returns true if the given segment is a proper internal or external
	// diagonal of this polygon
	bool IsDiagonal(const CG_Segment2& segment) const;

	// returns true if the given segment is a proper internal  diagonal 
	// of this polygon
	bool IsInternalDiagonal(const CG_Segment2& segment) const;

	// modifies the head of the circular list of vertices to point
	// to the segment.m_From endpoint
	// returns true if successfull
	bool SetAsStartingEdge(const CG_Segment2& segment);

	// triangulates this polygon using ear clipping.
	// Reference: Computational Geometry in C - Joseph O'Rourke
	// WARNING: the vertices MUST be in CCW order
	void SimpleEarClippingTriangulate();

	// if this polygon is non-convex, partitions it in a series of
	// convex subparts which are returned in a vector
	// WARNING: - the vertices MUST be in CCW order
	//          - the called triangulator must generate the triangulation,
	//            the diagonals and must update the convex field of every vertex
	vector<CG_Polygon2> PartitionHertelMehlhorn(PolygTriangulators triangulator);

	// merges this polygon and the given one and returns the resulting polygon
	// the two polygons MUST share an edge
	CG_Polygon2 Merge(const CG_Polygon2& other) const;

protected:
	// returns true if the given segment is strictly internal to this polygon
	// in the neighborhood of the given vertex
	// WARNING: vertex must coincide with segment.m_From
	bool InCone(const CG_Polygon2Vertex& vertex, const CG_Segment2& segment) const;

	// initializes the ear field of every vertex of this polygon
	void EarInit();

	// initializes the convex field of every vertex of this polygon
	void ConvexInit();
};

// -------------------------------------------------------------------------- //

#endif