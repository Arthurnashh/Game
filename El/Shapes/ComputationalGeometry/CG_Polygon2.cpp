#include ".\CG_Polygon2.h"
#include ".\CG_Triangle2.h"

// -------------------------------------------------------------------------- //

CG_Polygon2::CG_Polygon2()
: m_Vertices(0)
{
}

// -------------------------------------------------------------------------- //

CG_Polygon2::CG_Polygon2(const CG_Polygon2& other)
{
	m_Vertices = 0;
	for (unsigned int i = 0, cnt = other.VerticesCount(); i < cnt; ++i)
	{
		AddVertex(other.GetVertex(i));
	}
}

// -------------------------------------------------------------------------- //

CG_Polygon2::~CG_Polygon2()
{
	RemoveAllVertices();
}

// -------------------------------------------------------------------------- //

CG_Polygon2& CG_Polygon2::operator = (const CG_Polygon2& other)
{
	RemoveAllVertices();

	for (unsigned int i = 0, cnt = other.VerticesCount(); i < cnt; ++i)
	{
		AddVertex(other.GetVertex(i));
	}

	return *this;
}

// -------------------------------------------------------------------------- //

unsigned int CG_Polygon2::VerticesCount() const
{
	unsigned int count = 0;
	
	if (m_Vertices)
	{
		CG_Polygon2Vertex* currV = m_Vertices;

		do
		{
			count++;
			currV = currV->Next();
		}
		while (currV != m_Vertices);
	}

	return count;
}

// -------------------------------------------------------------------------- //

void CG_Polygon2::AddVertex(const CG_Polygon2Vertex& vertex)
{
	CG_Polygon2Vertex* newV = new CG_Polygon2Vertex(vertex);

	if (m_Vertices)
	{
		newV->Next(m_Vertices);
		newV->Prev(m_Vertices->Prev());
		m_Vertices->Prev(newV);
		newV->Prev()->Next(newV);
	}
	else
	{
		m_Vertices = newV;
		m_Vertices->Prev(newV);
		m_Vertices->Next(newV);
	}
}

// -------------------------------------------------------------------------- //

void CG_Polygon2::AddVertex(double x, double y)
{
	AddVertex(CG_Polygon2Vertex(x, y));
}

// -------------------------------------------------------------------------- //

void CG_Polygon2::AddVertex(double x, double y, unsigned int id)
{
	AddVertex(CG_Polygon2Vertex(x, y, id));
}

// -------------------------------------------------------------------------- //

CG_Polygon2Vertex* CG_Polygon2::FindVertex(const CG_Point2& point) const
{
	if (m_Vertices)
	{
		CG_Polygon2Vertex* currV = m_Vertices;
		do
		{
			if (*currV == point) return currV;
			currV = currV->Next();
		}
		while (currV != m_Vertices);
	}

	return 0;
}

// -------------------------------------------------------------------------- //

void CG_Polygon2::RemoveAllVertices()
{
	if (m_Vertices)
	{
		int count = VerticesCount();
		CG_Polygon2Vertex* currV = m_Vertices;
		CG_Polygon2Vertex* prevV = m_Vertices->Prev();
		CG_Polygon2Vertex* nextV = m_Vertices->Next();

		do
		{
			count--;
			delete currV;
			if (count > 0)
			{
				prevV->Next(nextV);
				currV = nextV;
				nextV = nextV->Next();
			}
		}
		while (count > 0);

		m_Vertices = 0;
	}
}

// -------------------------------------------------------------------------- //

const CG_Polygon2Vertex& CG_Polygon2::GetVertex(unsigned int index) const
{
	// what happens if m_Vertices == 0 ?

	CG_Polygon2Vertex* currV = m_Vertices;

	for (unsigned int i = 0; i < index; ++i)
	{
		currV = currV->Next();
	}

	return *currV;
}

// -------------------------------------------------------------------------- //

CG_Polygon2Vertex& CG_Polygon2::GetVertex(unsigned int index)
{
	// what happens if m_Vertices == 0 ?

	CG_Polygon2Vertex* currV = m_Vertices;

	for (unsigned int i = 0; i < index; ++i)
	{
		currV = currV->Next();
	}

	return *currV;
}

// -------------------------------------------------------------------------- //

double CG_Polygon2::Area() const
{
	double area = 0;

	if (m_Vertices)
	{
		CG_Polygon2Vertex* firstV = m_Vertices;

		CG_Polygon2Vertex* currV = m_Vertices;
		CG_Polygon2Vertex* nextV = m_Vertices->Next();

		do
		{
			CG_Triangle2 t(*firstV, *currV, *nextV);
			area += t.SignedArea();

			currV = nextV;
			nextV = currV->Next();
		}
		while (currV != m_Vertices);
	}
	return area;
}

// -------------------------------------------------------------------------- //

const vector<CG_Triangle2>& CG_Polygon2::Triangulation() const
{
	return m_Triangulation;
}

// -------------------------------------------------------------------------- //

const vector<CG_Diagonal>& CG_Polygon2::Diagonals() const
{
	return m_Diagonals;
}

// -------------------------------------------------------------------------- //

bool CG_Polygon2::IsEdge(const CG_Segment2& segment) const
{
	if (m_Vertices)
	{
		CG_Polygon2Vertex* currV = m_Vertices;
		CG_Polygon2Vertex* nextV = m_Vertices->Next();

		do
		{
			if (*currV == segment.From() && *nextV == segment.To()) return true;
			currV = currV->Next();
			nextV = currV->Next();
		}
		while (currV != m_Vertices);
	}
	return false;
}

// -------------------------------------------------------------------------- //

bool CG_Polygon2::IsReversedEdge(const CG_Segment2& segment) const
{
	if (m_Vertices)
	{
		CG_Polygon2Vertex* currV = m_Vertices;
		CG_Polygon2Vertex* nextV = m_Vertices->Next();

		do
		{
			if (*currV == segment.To() && *nextV == segment.From()) return true;
			currV = currV->Next();
			nextV = currV->Next();
		}
		while (currV != m_Vertices);
	}
	return false;
}

// -------------------------------------------------------------------------- //

bool CG_Polygon2::IsDiagonal(const CG_Segment2& segment) const
{
	if (m_Vertices)
	{
		CG_Polygon2Vertex* currV = m_Vertices;
		CG_Polygon2Vertex* nextV;

		do
		{
			nextV = currV->Next();

			if ((segment.From() != *currV) && 
				(segment.From() != *nextV) &&
				(segment.To() != *currV) &&
				(segment.To() != *nextV))
			{
				CG_Segment2 edge(*currV, *nextV);
				if (segment.Intersects(edge, TOLERANCE))
				{
					return false;
				}
			}

			currV = currV->Next();
		}
		while (currV != m_Vertices);

		return true;
	}
	return false;
}

// -------------------------------------------------------------------------- //

bool CG_Polygon2::IsInternalDiagonal(const CG_Segment2& segment) const
{
	if (m_Vertices)
	{
		CG_Polygon2Vertex* fromV = FindVertex(segment.From());
		CG_Polygon2Vertex* toV   = FindVertex(segment.To());
		if (!fromV || !toV) return false;

		CG_Segment2 invSegment = segment.Inverted();
		return (InCone(*fromV, segment) && InCone(*toV, invSegment) && IsDiagonal(segment));
	}
	return false;
}

// -------------------------------------------------------------------------- //

bool CG_Polygon2::SetAsStartingEdge(const CG_Segment2& segment)
{
	if (m_Vertices)
	{
		CG_Polygon2Vertex* currV = m_Vertices;
		CG_Polygon2Vertex* nextV = m_Vertices->Next();

		do
		{
			if (*currV == segment.From() && *nextV == segment.To())
			{
				m_Vertices = currV;
				return true;
			}
			currV = currV->Next();
			nextV = currV->Next();
		}
		while (currV != m_Vertices);
	}

	return false;
}

// -------------------------------------------------------------------------- //

void CG_Polygon2::SimpleEarClippingTriangulate()
{
	if (m_Vertices)
	{
		m_Diagonals.clear();
		m_Triangulation.clear();

		unsigned int n = VerticesCount();

		if (n < 3) return;

		EarInit();

		// a copy is needed because the following algorithm
		// is destructive (deletes vertices)
		CG_Polygon2 tempThis(*this);

		CG_Polygon2Vertex* currV;
		CG_Polygon2Vertex* prevV;	
		CG_Polygon2Vertex* nextV;	
		CG_Polygon2Vertex* prevprevV;	
		CG_Polygon2Vertex* nextnextV;	


		while (n > 3)
		{
			currV = tempThis.m_Vertices;
			
			// updates the convex state of the vertices (the polygon changes at
			// every iteration)
			tempThis.ConvexInit();
			do
			{
				if (currV->Ear())
				{
					prevV     = currV->Prev(); 
					nextV     = currV->Next(); 
					prevprevV = prevV->Prev(); 
					nextnextV = nextV->Next(); 

					// prevV - nextV is a diagonal
					m_Diagonals.push_back(CG_Diagonal(*prevV, *nextV, prevV->Convex(), nextV->Convex()));
					m_Triangulation.push_back(CG_Triangle2(*prevV, *currV, *nextV));

					CG_Segment2 prevEdge(*prevprevV, *nextV);
					CG_Segment2 nextEdge(*prevV, *nextnextV);
					prevV->Ear(IsInternalDiagonal(prevEdge));
					nextV->Ear(IsInternalDiagonal(nextEdge));

					prevV->Next(nextV);
					nextV->Prev(prevV);
					delete currV;
					tempThis.m_Vertices = nextV;
					n--;
					break;
				}
				currV = currV->Next();
			}
			while (currV != tempThis.m_Vertices);
		}
		m_Triangulation.push_back(CG_Triangle2(*tempThis.m_Vertices->Prev(), 
											   *tempThis.m_Vertices, 
											   *tempThis.m_Vertices->Next()));
	}
}

// -------------------------------------------------------------------------- //

vector<CG_Polygon2> CG_Polygon2::PartitionHertelMehlhorn(PolygTriangulators triangulator)
{
	vector<CG_Polygon2> polygons;

	if (m_Vertices)
	{
		switch(triangulator)
		{
		case PolyTri_SimpleEarClipping:
			{
				SimpleEarClippingTriangulate();
			}
			break;
		};

		// we begin filling the polygons list with all the triangles
		// of the triangulation
		for (size_t i = 0, cnt = m_Triangulation.size(); i < cnt; ++i)
		{
			CG_Triangle2 t = m_Triangulation[i];
			CG_Polygon2 poly;
			for (unsigned int j = 0; j < 3; ++j)
			{
				poly.AddVertex(t.Vertex(j));
			}
			polygons.push_back(poly);
		}

		// first checks diagonals, disabling the essential field
		// for the ones having both convex endpoints
		for (size_t i = 0, cnt = m_Diagonals.size(); i < cnt; ++i)
		{
			if (m_Diagonals[i].FromConvex() && m_Diagonals[i].ToConvex()) 
			{
				m_Diagonals[i].Essential(false);
			}
		}

		// now merges all polygons sharing the non-essential diagonals
		for (size_t i = 0, cnt1 = m_Diagonals.size(); i < cnt1; ++i)
		{
			if (!m_Diagonals[i].Essential())
			{
				int polyToMerge1Index = -1;
				int polyToMerge2Index = -1;
				for (size_t j = 0, cnt2 = polygons.size(); j < cnt2; ++j)
				{
					if (polygons[j].IsEdge(m_Diagonals[i]))
					{
						polyToMerge1Index = (int)j;
						break;
					}
				}
				for (size_t j = 0, cnt2 = polygons.size(); j < cnt2; ++j)
				{
					if (polygons[j].IsReversedEdge(m_Diagonals[i]))
					{
						polyToMerge2Index = (int)j;
						break;
					}
				}

				if (
					polyToMerge1Index == -1 || 
					polyToMerge2Index == -1 ||
					polyToMerge1Index == polyToMerge2Index 
				   )
				{
					/* something really bad has happened */ 
					polygons.clear();
					return polygons;
				}

				CG_Polygon2 polyToMerge1 = polygons[polyToMerge1Index];
				CG_Polygon2 polyToMerge2 = polygons[polyToMerge2Index];

				CG_Polygon2 mergedPoly = polyToMerge1.Merge(polyToMerge2);

				polygons[polyToMerge1Index] = mergedPoly;
				polygons.erase(polygons.begin() + polyToMerge2Index);
			}
		}
	}

	return polygons;
}

// -------------------------------------------------------------------------- //

CG_Polygon2 CG_Polygon2::Merge(const CG_Polygon2& other) const
{
	// copy required to preserve the const of the input polygons
	CG_Polygon2 tempThis  = *this;
	CG_Polygon2 tempOther = other;
	CG_Polygon2 newPoly;

	if (tempThis.m_Vertices)
	{
		CG_Polygon2Vertex* currV = tempThis.m_Vertices;
		CG_Polygon2Vertex* nextV = tempThis.m_Vertices->Next();

		do
		{
			CG_Segment2 edge(*currV, *nextV);
			if (tempOther.IsReversedEdge(edge))
			{
				CG_Segment2 invEdge = edge.Inverted();
				if (tempThis.SetAsStartingEdge(edge))
				{
					if (tempOther.SetAsStartingEdge(invEdge))
					{
						for (unsigned int i = 1, cnt = tempThis.VerticesCount(); i < cnt ; ++i)
						{
							newPoly.AddVertex(tempThis.GetVertex(i));

						}
						newPoly.AddVertex(tempThis.GetVertex(0));

						for (unsigned int i = 2, cnt = tempOther.VerticesCount(); i < cnt ; ++i)
						{
							newPoly.AddVertex(tempOther.GetVertex(i));
						}
					}
				}
				// we are done
				break;
			}

			currV = currV->Next();
			nextV = currV->Next();
		}
		while (currV != tempThis.m_Vertices);
	}

	return newPoly;
}

// -------------------------------------------------------------------------- //

bool CG_Polygon2::InCone(const CG_Polygon2Vertex& vertex, const CG_Segment2& segment) const
{
	if (m_Vertices)
	{
		CG_Polygon2Vertex* prevV = vertex.Prev();	
		CG_Polygon2Vertex* nextV = vertex.Next();	

		CG_Segment2 edge(vertex, *nextV);
		CG_Segment2 invSegment = segment.Inverted();

		// if vertex is convex
		if (edge.IsLeftOn(*prevV, TOLERANCE))
		{
			return (segment.IsLeft(*prevV, TOLERANCE) && invSegment.IsLeft(*nextV, TOLERANCE));
		}

		// vertex is reflex
		return !(segment.IsLeftOn(*nextV, TOLERANCE) && invSegment.IsLeftOn(*prevV, TOLERANCE));
	}
	return false;
}

// -------------------------------------------------------------------------- //

void CG_Polygon2::EarInit()
{
	if (m_Vertices)
	{
		CG_Polygon2Vertex* currV = m_Vertices;
		CG_Polygon2Vertex* prevV = m_Vertices->Prev();
		CG_Polygon2Vertex* nextV = m_Vertices->Next();

		do
		{
			CG_Segment2 chord(*prevV, *nextV);
			currV->Ear(IsInternalDiagonal(chord));
			currV = currV->Next();
			prevV = currV->Prev();
			nextV = currV->Next();
		}
		while (currV != m_Vertices);
	}
}

// -------------------------------------------------------------------------- //

void CG_Polygon2::ConvexInit()
{
	if (m_Vertices)
	{
		CG_Polygon2Vertex* currV = m_Vertices;
		CG_Polygon2Vertex* prevV = m_Vertices->Prev();
		CG_Polygon2Vertex* nextV = m_Vertices->Next();

		do
		{
			CG_Segment2 edge(*currV, *nextV);
			currV->Convex(edge.IsLeftOn(*prevV, TOLERANCE));
			currV = currV->Next();
			prevV = currV->Prev();
			nextV = currV->Next();
		}
		while (currV != m_Vertices);
	}
}

// -------------------------------------------------------------------------- //
