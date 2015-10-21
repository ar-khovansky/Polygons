#include "Boolean.h"

#include "Functions.h"

#include "../Lib/Iterators.h"

#include <algorithm>
#include <memory>
#include <cassert>

#include <iomanip>



#define ENABLE_TRACE
#ifdef ENABLE_TRACE
#  include <windows.h>
#  include <sstream>
#  define TRACE(x)                           \
   do {  std::wstringstream s;  s << x;    \
         OutputDebugString(s.str().c_str()); \
      } while(0)
#else
#  define TRACE(x)
#endif



/*!
 * Implementation is based on algorithm used in PolyBoolean library:
 * M. V. Leonov and A. G. Nikitin.
 * An Efficient Algorithm for a Closed Set of Boolean Operations on Polygonal Regions in the Plane
 * http://www.complex-a5.ru/polyboolean/downloads/polybool_eng.pdf.
 *
 * The algorithm is not tied to any specific method for finding intersections. Here the
 * brute force is used.
 *
 * Original algorithm can handle degeneracies and holes. This implementation does not.
 *
 * Also this implementation does not support touching of different polygons by edges and vertices.
 */


namespace poly {

using namespace std;


/*!
 * Terms and abbreviations
 *
 * Term                      Abbreviation   Description
 * ---------------------------------------------------------
 * VertEdge                  ve             see below
 * Cross vertex descriptor   xvd            see original paper 
 * Cross polygon             xp             Original polygon with added cross vertices.
 *                                            It has format vector<VertEdge>.
 */



struct XVD;



/// Aggregation of vertex and edge next to it.
//
struct VertEdge
{
	Point const vertex;

	// The following is for cross vertices
	shared_ptr<list<XVD>> xvdList;
	list<XVD>::iterator xvdPrev, xvdNext;

	enum EdgeLabel { Inside, Outside };
	EdgeLabel edgeLabel;

	bool edgeMark;

//
	/// Construct usual vertex
	VertEdge(Point const &vertex)
		: vertex(vertex)
		, edgeMark(false)
	{}
	/// Construct cross vertex
	VertEdge(Point const &vertex,
	         shared_ptr<list<XVD>> const &xvdList)
		: vertex(vertex)
		, xvdList(xvdList)
		, edgeMark(false)
	{}

	bool isCrossVertex() const { return (bool)xvdList; }
};



/// Cross vertex descriptor
//
struct XVD
{
	list<VertEdge> &xp;
	list<VertEdge>::iterator ve;

	enum Order { Prev, Next } order;
	
	double polarAngle;

//
	XVD(list<VertEdge> &xp,
	    list<VertEdge>::iterator ve,
	    Order order,
	    double polarAngle)
		: xp(xp), ve(ve), order(order), polarAngle(polarAngle)
	{}
};



#ifdef ENABLE_TRACE
wostream & operator<<(wostream &s, Point const &p)
{ return s << setprecision(4) << "(" << p.x << ", " << p.y << ")"; }


static list<VertEdge> const *_xp1Ptr;

wostream & operator<<(wostream &s, VertEdge const &ve)
{
	s << ve.vertex;
	if ( ve.isCrossVertex() )
		s << ", " << (&ve.xvdNext->xp == _xp1Ptr ? "A" : "B") << ", cross";
	s << ", " << (ve.edgeLabel == VertEdge::Inside ? "inside" : "outside");
	if ( ve.edgeMark )
		s << ", mark";

	return s;
}
#endif



/// Find intersections between tho polygons.
/*!
 * Uses brute force method.
 * Does not support touching by edges and vertices.
 *
 * \pre Polygons must be counterclockwise.
 *
 * \param[in] p1    Polygon 1.
 * \param[in] p2    Polygon 2.
 * \param[out] xp1  Cross polygon 1.
 * \param[out] xp2  Cross polygon 2.
 *
 * \throw domain_error If touching be edges is detected.
 */
static void findIntersections(Polygon const &p1, Polygon const &p2,
                              list<VertEdge> &xp1_, list<VertEdge> &xp2_)
{
	// Predicate for ordering segment intersections by distance from first segment endpoint
	class SegmentPointLess {
	public:
		SegmentPointLess(Point const &initialPt) : initialPt(initialPt) {}
		bool operator()(VertEdge const &p1, VertEdge const &p2) const {
			return distanceSqr(initialPt, p1.vertex) < distanceSqr(initialPt, p2.vertex);
		}
	private:
		Point const initialPt;
	};


	list<VertEdge> xp1, xp2;
	
	// Iterators pointing to begins of original segments in xp2
	vector<list<VertEdge>::iterator> xp2OrigSegBegins;

	for ( auto seg1 = p1.edgeBegin(); seg1 != p1.edgeEnd(); ++seg1 ) {
		Point const &seg1Vertex1 = *seg1.vertex1Iterator();

		xp1.emplace_back(seg1Vertex1);
		auto const xp1CurSegBegin = --xp1.end();

		
		unsigned seg2Idx = 0;
		for ( auto seg2 = p2.edgeBegin(); seg2 != p2.edgeEnd(); ++seg2, ++seg2Idx ) {
			Point const &seg2Vertex1 = *seg2.vertex1Iterator();

			list<VertEdge>::iterator xp2CurSegBegin;
			
			if ( xp2OrigSegBegins.size() == seg2Idx ) {
				xp2.emplace_back(seg2Vertex1);
				xp2CurSegBegin = --xp2.end();
				xp2OrigSegBegins.push_back(xp2CurSegBegin);
			}
			else
				xp2CurSegBegin = xp2OrigSegBegins.at(seg2Idx);
			
			Point isectP1, isectP2;
			IntersectionShape const isectShape = intersect(*seg1, *seg2, isectP1, isectP2);

			if ( isectShape == Isect_Point ) {
				auto const xvdList = make_shared<list<XVD>>();

				{
					auto const lower =
						lower_bound(xp1CurSegBegin, xp1.end(), isectP1, SegmentPointLess(seg1Vertex1));
					xp1.emplace(lower,
					            isectP1, xvdList);
				}
				{
					auto const lower =
						lower_bound(xp2CurSegBegin, xp2.end(), isectP1, SegmentPointLess(seg2Vertex1));
					xp2.emplace(lower,
					            isectP1, xvdList);
				}
			}
			else if ( isectShape == Isect_Segment )
				//TODO
				throw domain_error("Touching edges are not supported");
		}

	}

	swap(xp1_, xp1);
	swap(xp2_, xp2);
}



/// Insert XVD in list preserving sorting
//
static list<XVD>::iterator insertSorted(list<XVD> &xvdList, XVD const &xvd)
{
	auto const lower = lower_bound(xvdList.begin(), xvdList.end(), xvd,
			[](XVD const &xvd1, XVD const &xvd2){
				return xvd1.polarAngle < xvd2.polarAngle;
			});
	return xvdList.insert(lower, xvd);
}



/// Fill connectivity lists from the given cross polygon's side.
//
static void fillConnectivityLists(list<VertEdge> &xp)
{
	for ( auto ve = xp.begin(); ve != xp.end(); ++ve ) {
		if ( ! ve->isCrossVertex() )
			continue;
		
		auto const vePrev = prev_cyclic(ve, xp);
		//ve->xvdList->emplace_back(&*ve, XVD::Prev,
		//                          polarAngle(Vector(ve->vertex, vePrev->vertex));
		ve->xvdPrev = insertSorted(*ve->xvdList,
		                           XVD(xp, ve,
		                               XVD::Prev,
															     polarAngle(Vector(ve->vertex, vePrev->vertex))));
		
		auto const veNext = next_cyclic(ve, xp);
		//ve->xvdList->emplace_back(&*ve, XVD::Next,
		//                          polarAngle(Vector(ve->vertex, veNext->vertex));
		ve->xvdNext = insertSorted(*ve->xvdList,
		                           XVD(xp, ve,
		                               XVD::Next,
		                               polarAngle(Vector(ve->vertex, veNext->vertex))));
	}
}



static Segment otherXVertNextEdge(VertEdge const &ve)
{
	for ( XVD const &xvd : *ve.xvdList ) {
		if ( &*xvd.ve != &ve )
			return Segment(xvd.ve->vertex, next_cyclic(xvd.ve, xvd.xp)->vertex);
	}

	assert(0);
	throw logic_error("");
}



/// Label edges of cross polygon
//
static void labelEdges(list<VertEdge> &xp)
{
	TRACE(__FUNCTION__ << endl);

	auto const firstXVert = find_if(xp.begin(), xp.end(),
	                                [](VertEdge const &ve){ return ve.isCrossVertex(); });
	
	auto ve = firstXVert;
	VertEdge::EdgeLabel lastLabel;
	do {
		auto ve2 = next_cyclic(ve, xp);
		
		if ( ve->isCrossVertex() ) {
			ve->edgeLabel = orientation(ve2->vertex, otherXVertNextEdge(*ve)) == Left ?
						          VertEdge::Inside : VertEdge::Outside;
			lastLabel = ve->edgeLabel;
		}
		else
			ve->edgeLabel = lastLabel;

		TRACE(*ve << endl);

		++ve;
		if ( ve == xp.end() )
			ve = xp.begin();
	}
	while ( ve != firstXVert );
}



enum Direction { Forward, Backward };

typedef bool (*EdgeRule)(VertEdge const &ve, bool contourA, Direction &dir);



static list<VertEdge>::iterator edgeCorrespondingTo(XVD const &xvd)
{
	return xvd.order == XVD::Next ? xvd.ve : prev_cyclic(xvd.ve, xvd.xp);
}



/// Select next cross vertex and direction
//
template<typename EdgeRule>
static bool jump(list<VertEdge> **xp,
                 bool &contourA,
                 list<VertEdge>::iterator &ve,
                 Direction &dir,
                 EdgeRule edgeRule)
{
	TRACE(__FUNCTION__ << "((" << *ve << "), " << dir << ") {" << endl);

	auto const xvdBegin = prev_cyclic((dir == Forward ? ve->xvdPrev : ve->xvdNext), *ve->xvdList);
	auto xvd = xvdBegin;
	do {
		bool const contourAXvd = (*xp == &xvd->xp ? contourA : ! contourA);
		auto const edgeXvd = edgeCorrespondingTo(*xvd);
		Direction newDir;
		if ( ! edgeXvd->edgeMark && edgeRule(*edgeXvd, contourAXvd, newDir) ) {
			*xp = &xvd->xp;
			contourA = contourAXvd;
			ve = xvd->ve;

			if ( xvd->order == XVD::Next && newDir == Forward ||
			     xvd->order == XVD::Prev && newDir == Backward )
			{
				dir = newDir;

				TRACE("} " << __FUNCTION__ << endl);
				TRACE("<<" << *ve << " | " << (contourA ? "A": "B") << " | " << dir << endl);
				return true;
			}
		}

		xvd = prev_cyclic(xvd, *ve->xvdList);
	}
	while ( xvd != xvdBegin );

	TRACE("} " << __FUNCTION__ << endl);
	TRACE("<<" << *ve << " | " << (contourA ? "A": "B") << " | " << dir << endl);
	return false;
}



template<typename EdgeRule>
static Polygon collectContour(list<VertEdge> &xp,
                              list<VertEdge>::iterator ve,
                              Direction dir,
                              EdgeRule edgeRule)
{
	TRACE(__FUNCTION__ << "((" << *ve << "), " << dir << ") {" << endl);

	list<Point> vertices;

	list<VertEdge> *pXP = &xp;
	bool contourA = true;
	auto edge = (dir == Forward ? ve : prev_cyclic(ve, *pXP));
	do {
		TRACE("+ " << ve->vertex << endl);
		vertices.push_back(ve->vertex);
		
		edge->edgeMark = true;

		ve = (dir == Forward ? next_cyclic(ve, *pXP) : prev_cyclic(ve, *pXP));
		
		TRACE(*ve << endl);

		if ( ve->isCrossVertex() ) {
			if ( ! jump(&pXP, contourA, ve, dir, edgeRule) )
				break;
			assert(contourA == (pXP == &xp));
		}

		edge = (dir == Forward ? ve : prev_cyclic(ve, *pXP));
	}
	while ( ! edge->edgeMark );

	TRACE("} " << __FUNCTION__ << endl);
	return vertices;
}



template<typename EdgeRule>
static void collectContours(list<VertEdge> &xp,
                            EdgeRule edgeRule,
                            vector<Polygon> &contours)
{
	TRACE(__FUNCTION__ << " {" << endl);

	for ( auto ve = xp.begin(); ve != xp.end(); ++ve ) {
		TRACE(*ve << endl);
		
		Direction dir;
		if ( ! ve->edgeMark && edgeRule(*ve, true, dir) ) {
			auto veStart = (dir == Forward ? ve : next_cyclic(ve, xp));
			Polygon const contour = collectContour(xp, veStart, dir, edgeRule);
			contours.push_back(contour);
		}
	}
	
	TRACE("} " << __FUNCTION__ << endl);
}



/// Do preparations common for all boolean operations, up to labeling edges.
//
static void prepareLabeledCrossPolygons(Polygon const &p1, Polygon const &p2,
                                        list<VertEdge> &xp1, list<VertEdge> &xp2)
{
	if ( ! (p1.isSimple() && p2.isSimple()) )
		throw domain_error("Self-intersecting polygon");
	
	Polygon const p1ccw = p1.toCcw();
	Polygon const p2ccw = p2.toCcw();
	
	findIntersections(p1ccw, p2ccw, xp1, xp2);

	TRACE("xp1:" << endl);
	for ( VertEdge const &ve : xp1 ) {
		TRACE(ve.vertex << (ve.isCrossVertex() ? ", cross" : "") << endl);
	}
	TRACE("xp2:" << endl);
	for ( VertEdge const &ve : xp2 ) {
		TRACE(ve.vertex << (ve.isCrossVertex() ? ", cross" : "") << endl);
	}
	
	_xp1Ptr = &xp1;
	
	fillConnectivityLists(xp1);
	fillConnectivityLists(xp2);

	labelEdges(xp1);
	labelEdges(xp2);
}



/// Clear edge marks
/*! Used for two-pass operations like partition.
 */
static void clearEdgeMarks(list<VertEdge> &xp)
{
	for ( auto &ve : xp )
		ve.edgeMark = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////


bool edgeRule_Add(VertEdge const &ve, bool contourA, Direction &dir)
{
	TRACE(__FUNCTION__"(" << ve << ")" << endl);

	if ( ve.edgeLabel == VertEdge::Outside ) {
		dir = Forward;
		TRACE("Forward" << endl);
		return true;
	}
	
	TRACE("false" << endl);
	return false;
}



vector<Polygon> add(Polygon const &p1, Polygon const &p2)
{
	TRACE(__FUNCTION__ << " {" << endl);

	list<VertEdge> xp1, xp2;
	
	prepareLabeledCrossPolygons(p1, p2, xp1, xp2);

	vector<Polygon> contours;
	collectContours(xp1, &edgeRule_Add, contours);
	collectContours(xp2, &edgeRule_Add, contours);

	if ( contours.size() > 1 )
		throw range_error("Resulting polygon contains holes");
	
	TRACE("} " << __FUNCTION__ << endl);
	return contours;
}

////////////////////////////////////////////////////////////////////////////////////////////////////


bool edgeRule_Intersect(VertEdge const &ve, bool contourA, Direction &dir)
{
	TRACE(__FUNCTION__"(" << ve << ")" << endl);

	if ( ve.edgeLabel == VertEdge::Inside ) {
		dir = Forward;
		TRACE("Forward" << endl);
		return true;
	}
	
	TRACE("false" << endl);
	return false;
}



vector<Polygon> intersect(Polygon const &p1, Polygon const &p2)
{
	TRACE(__FUNCTION__ << " {" << endl);

	list<VertEdge> xp1, xp2;
	
	prepareLabeledCrossPolygons(p1, p2, xp1, xp2);

	vector<Polygon> contours;
	collectContours(xp1, &edgeRule_Intersect, contours);
	collectContours(xp2, &edgeRule_Intersect, contours);
	
	TRACE("} " << __FUNCTION__ << endl);
	return contours;
}

////////////////////////////////////////////////////////////////////////////////////////////////////


bool edgeRule_Subtract(VertEdge const &ve, bool contourA, Direction &dir)
{
	TRACE(__FUNCTION__"(" << ve << ")" << endl);

	if ( contourA && ve.edgeLabel == VertEdge::Outside ) {
		dir = Forward;
		TRACE("Forward" << endl);
		return true;
	}
	else if ( ! contourA && ve.edgeLabel == VertEdge::Inside ) {
		dir = Backward;
		TRACE("Backward" << endl);
		return true;
	}

	TRACE("false" << endl);
	return false;
}



vector<Polygon> subtract(Polygon const &p1, Polygon const &p2)
{
	TRACE(__FUNCTION__ << " {" << endl);

	list<VertEdge> xp1, xp2;
	
	prepareLabeledCrossPolygons(p1, p2, xp1, xp2);

	vector<Polygon> contours;
	collectContours(xp1, &edgeRule_Subtract, contours);
	
	TRACE("} " << __FUNCTION__ << endl);
	return contours;
}

////////////////////////////////////////////////////////////////////////////////////////////////////


// Original XOR rule produces polygons with holes. We combine results of symmetrical
// subtractions instead.

//bool edgeRule_Xor(VertEdge const &ve, bool contourA, Direction &dir)
//{
//	TRACE(__FUNCTION__"(" << ve << ")" << endl);
//
//	if ( ve.edgeLabel == VertEdge::Outside ) {
//		dir = Forward;
//		TRACE("Forward" << endl);
//		return true;
//	}
//	else if ( ve.edgeLabel == VertEdge::Inside ) {
//		dir = Backward;
//		TRACE("Backward" << endl);
//		return true;
//	}
//	
//	TRACE("false" << endl);
//	return false;
//}



vector<Polygon> xor(Polygon const &p1, Polygon const &p2)
{
	TRACE(__FUNCTION__ << " {" << endl);

	list<VertEdge> xp1, xp2;
	
	prepareLabeledCrossPolygons(p1, p2, xp1, xp2);

	vector<Polygon> contours;
	collectContours(xp1, &edgeRule_Subtract, contours);
	collectContours(xp2, &edgeRule_Subtract, contours);
	
	TRACE("} " << __FUNCTION__ << endl);
	return contours;
}

////////////////////////////////////////////////////////////////////////////////////////////////////


vector<Polygon> partition(Polygon const &p1, Polygon const &p2)
{
	TRACE(__FUNCTION__ << " {" << endl);

	list<VertEdge> xp1, xp2;
	
	prepareLabeledCrossPolygons(p1, p2, xp1, xp2);

	vector<Polygon> contours;
	
	// Partition is a combination of (p1 & p2) and (p1 - p2)
	
	collectContours(xp1, &edgeRule_Intersect, contours);

	clearEdgeMarks(xp1);
	clearEdgeMarks(xp2);
	
	collectContours(xp1, &edgeRule_Subtract, contours);

	TRACE("} " << __FUNCTION__ << endl);
	return contours;
}



} // namespace poly
