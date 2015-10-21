#include "Polygon.h"

#include "../Lib/Iterators.h"

#include <algorithm>


using namespace poly;
using namespace std;



Polygon::Polygon(list<Point> const &vertices)
{
	if ( vertices.empty() )
		throw invalid_argument("No vertices");

	this->vertices = vertices;
}



bool Polygon::isSimple() const
{
	if ( numVertices() < 3 )
		return false;
	
	if ( numVertices() == 3 )
		return true;
	
	unsigned n_3 = numVertices() - 3;
	
	ConstEdgeIterator itCur = edgeBegin();

	ConstEdgeIterator itTestStart = itCur; ++ ++itTestStart;
	for ( unsigned i = 0; i <= n_3; ++i, ++itCur, ++itTestStart ) {
		ConstEdgeIterator it = itTestStart;
		for ( unsigned i1 = 0; i1 <= n_3 - max((unsigned)1, i); ++i1, ++it )
			if ( intersects(*itCur, *it) )
				return false;
	}
	return true;
}



bool Polygon::isCcw() const
{
	auto const v = min_element(vertices.begin(), vertices.end());
	return orientation(*prev_cyclic(v, vertices), *v, *next_cyclic(v, vertices)) == Left;
}


void Polygon::makeCcw()
{
	if ( ! isCcw() )
		vertices.reverse();
}


Polygon Polygon::toCcw() const
{
	Polygon ccw = *this;
	ccw.makeCcw();
	return ccw;
}
