#pragma once

#include "Point.h"
#include "Segment.h"
#include "Functions.h" //TODO: only for "p += v"

#include <list>



namespace poly {



/// Polygon
/*!
 * Polygon is essentially a list of vertices, so begin()/end() return vertex iterators.
 * Iteration by edges is possible with edgeBegin()/edgeEnd().
 *
 * Polygon can have any number of vertices. It can be self-intersecting. Order can be clockwise
 * or counterclockwise. If this is not desirable, higher level logic should impose restrictions.
 *
 * The object can be constructed empty (with no vertices). This is only to allow some language
 * constructs.
 *
 * The class has move constructor ang move assignment operator with \c noexcept specification.
 */
class Polygon
{
public:
	typedef std::list<Point> VertexList;
	typedef VertexList::iterator       iterator;
	typedef VertexList::const_iterator const_iterator;

	/// Iterator by edges.
	//
	class ConstEdgeIterator {
	public:
		ConstEdgeIterator() : vertices(nullptr) {}

		ConstEdgeIterator & operator++() { ++vertex1; return *this; }

		Segment operator*() const { return Segment(*vertex1, *vertex2Iterator()); }

		Polygon::const_iterator vertex1Iterator() const { return vertex1; }
		Polygon::const_iterator vertex2Iterator() const;

		bool operator==(ConstEdgeIterator const &r) const { return vertex1 == r.vertex1; }
		bool operator!=(ConstEdgeIterator const &r) const { return !(*this == r); }

	protected:
		//friend ConstEdgeIterator Polygon::edgeBegin() const;
		//friend ConstEdgeIterator Polygon::edgeEnd() const;
		friend class Polygon;
		ConstEdgeIterator(VertexList::const_iterator vertex1, VertexList const &vertices)
			: vertex1(vertex1), vertices(&vertices) {}
	//Fields
		VertexList::const_iterator vertex1;
		VertexList const *vertices;
	};


public:
	Polygon() {}
	Polygon(std::list<Point> const &vertices);

	Polygon(Polygon &&r) _NOEXCEPT { vertices.swap(r.vertices); }

protected:
	Polygon(Polygon const &r) = default;

public:
	Polygon& operator=(Polygon &&r) _NOEXCEPT { vertices.swap(r.vertices); return *this; }

	unsigned int numVertices() const { return vertices.size(); }

	bool empty() const { return vertices.empty(); }

	iterator begin() { return vertices.begin(); }
	iterator end()   { return vertices.end(); }
	const_iterator begin() const { return vertices.begin(); }
	const_iterator end()   const { return vertices.end(); }

	Point & back() { return vertices.back(); }
	
	ConstEdgeIterator edgeBegin() const { return ConstEdgeIterator(vertices.begin(), vertices); }
	ConstEdgeIterator edgeEnd()   const { return ConstEdgeIterator(vertices.end(),   vertices); }

	void addVertex(Point const &vertex) { vertices.push_back(vertex); }
	void insertVertex(const_iterator at, Point const &vertex) { vertices.insert(at, vertex); }
	void removeVertex(const_iterator at) { vertices.erase(at); }
	
	/// Translate (move) by given vector
	void translate(Vector const &v) { for ( Point &p : vertices ) p += v; }

	/// Test if polygon is simple, i.e. has no self-intersections, including self-touches.
	bool isSimple() const;

	bool isCcw() const;    ///< Test if direction is counterclockwise.
	void makeCcw();        ///< Make direction counterclockwise.
	Polygon toCcw() const; ///< Return counterclockwise copy.

protected:
	VertexList vertices;
};



inline Polygon::const_iterator Polygon::ConstEdgeIterator::vertex2Iterator() const
{
	auto vertex2 = vertex1; ++vertex2;
	if ( vertex2 == vertices->end() )
		vertex2 = vertices->begin();
	return vertex2;
}



} // namespace poly
