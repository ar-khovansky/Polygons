#pragma once

#include <list>
#include <stdexcept>

#include "Poly/Poly.h"


////////////////////////////////////////////////////////////////////////////////////////////////////

/// Erase last n elements from container.
/*!
 * \throw length_error If n > container size.
 */
template <typename Container, typename Size>
void erase_n_back(Container &c, Size n)
{
	if ( n > c.size() )
		throw length_error("Container size exceeded");

	c.erase(prev(c.end(), n), c.end());
}


/// Truncate container to size n.
/*!
 * \throw length_error If n > container size.
 */
template <typename Container, typename Size>
void truncate(Container &c, Size n)
{
	if ( n > c.size() )
		return;

	erase_n_back(c, c.size() - n);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Action helpers - get iterators by indices

inline std::list<poly::Polygon>::iterator
polygonIteratorByIdx(std::list<poly::Polygon> &polygons, UINT polygonIdx) {
	ENSURE(polygonIdx <= polygons.size());
	return next(polygons.begin(), polygonIdx);
}

inline poly::Polygon & polygonByIdx(std::list<poly::Polygon> &polygons, UINT polygonIdx)
{ return *polygonIteratorByIdx(polygons, polygonIdx); }

static poly::Polygon::iterator vertexIteratorByIdx(poly::Polygon &polygon, UINT vertexIdx) {
	ENSURE(vertexIdx <= polygon.numVertices());
	return next(polygon.begin(), vertexIdx);
}

inline poly::Polygon::const_iterator
vertexIteratorByIdx(poly::Polygon const &polygon, UINT vertexIdx) {
	ENSURE(vertexIdx <= polygon.numVertices());
	return next(polygon.begin(), vertexIdx);
}

inline poly::Polygon::iterator
vertexIteratorByIdx(std::list<poly::Polygon> &polygons, UINT polygonIdx, UINT vertexIdx)
{ return vertexIteratorByIdx(polygonByIdx(polygons, polygonIdx), vertexIdx); }
