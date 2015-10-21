#define _USE_MATH_DEFINES

#include "Functions.h"

#include "Segment.h"
#include "Polygon.h"

#include "../Lib/Iterators.h"

#include <cmath>
#include <float.h>


namespace poly {



double distanceSqr_Strict(Point const &p, Segment const &s)
{
	Vector const ptV(s.p1, p);
	Vector const segV = s.toVector();
	double const d1 = dotProduct(ptV, segV);
	if ( d1 < 0 )
		return DBL_MAX;
	double const d2 = dotProduct(segV, segV);
	if ( d1 > d2 )
		return DBL_MAX;
	return distanceSqr(p, s.toLine());
}



double distanceSqr(Point const &p, Polygon const &polygon)
{
	double dMin = DBL_MAX;

	for ( Point const &vertex : polygon ) {
		double const d = distanceSqr(p, vertex);
		if ( d == 0 )
			return 0;
		if ( d < dMin )
			dMin = d;
	}

	for ( auto it = polygon.edgeBegin(); it != polygon.edgeEnd(); ++it ) {
		double const d = distanceSqr_Strict(p, *it);
		if ( d == 0 )
			return 0;
		if ( d < dMin )
			dMin = d;
	}

	return dMin;
}



static bool areIntersecting_Segments_Crossing(Point const &p1, Point const &p2,
                                              Point const &p3, Point const &p4)
{
  int const i = (int) orientation(Vector(p1, p2), Vector(p1, p3));
  if ( i == 0 )
    return true;
  else
    return (int) orientation(Vector(p3, p4), Vector(p3, p2)) != -i;
}

static bool areIntersecting_Segments_Contained(Point const &p1, Point const &p2,
                                               Point const &p3, Point const &p4)
{
  int const i = (int) orientation(Vector(p1, p2), Vector(p1, p3));
  if ( i == 0 )
    return true;
  else
    return (int) orientation(Vector(p1, p2), Vector(p1, p4)) != i;
}


bool intersects(Segment const &s1, Segment const &s2)
{
	Point const &A1 = s1.p1;
	Point const &A2 = s1.p2;
	Point const &B1 = s2.p1;
	Point const &B2 = s2.p2;

	if ( A1 < A2 ) {
		if ( B1 < B2 ) {
			if ( A2 < B1 || B2 < A1 )
				return false;
			
			switch ( compare(A1, B1) ) {
				case -1:
					switch ( compare(A2, B1) ) {
						case -1:
							return false;
						case 0:
							return true;
						case 1:
							switch ( compare(A2, B2) ) {
								case -1:
									return areIntersecting_Segments_Crossing(A1, A2, B1, B2);
								case 0:
									return true;
								case 1:
									return areIntersecting_Segments_Contained(A1, A2, B1, B2);
							}
					}
				case 0:
					return true;
				case 1:
					switch ( compare(B2, A1) ) {
						case -1:
							return false;
						case 0:
							return true;
						case 1:
							switch ( compare(B2, A2) ) {
								case -1:
									return areIntersecting_Segments_Crossing(B1, B2, A1, A2);
								case 0:
									return true;
								case 1:
									return areIntersecting_Segments_Contained(B1, B2, A1, A2);
							}
					}
			}
		}
		else {
			if ( A2 < B2 || B1 < A1 )
				return false;
		
			switch ( compare(A1, B2) ) {
				case -1:
					switch ( compare(A2, B2) ) {
						case -1:
							return false;
						case 0:
							return true;
						case 1:
							switch ( compare(A2, B1) ) {
								case -1:
									return areIntersecting_Segments_Crossing(A1, A2, B2, B1);
								case 0:
									return true;
								case 1:
									return areIntersecting_Segments_Contained(A1, A2, B2, B1);
							}
					}
				case 0:
					return true;
				case 1:
					switch ( compare(B1, A1) ) {
						case -1:
							return false;
						case 0:
							return true;
						case 1:
							switch ( compare(B1, A2) ) {
								case -1:
									return areIntersecting_Segments_Crossing(B2, B1, A1, A2);
								case 0:
									return true;
								case 1:
									return areIntersecting_Segments_Contained(B2, B1, A1, A2);
							}
					}
			}
		}
	}
	else {
		if ( B1 < B2 ) {
			if ( A1 < B1 || B2 < A2 )
				return false;
			
			switch ( compare(A2, B1) ) {
				case -1:
					switch ( compare(A1, B1) ) {
						case -1:
							return false;
						case 0:
							return true;
						case 1:
							switch ( compare(A1, B2) ) {
								case -1:
									return areIntersecting_Segments_Crossing(A2, A1, B1, B2);
								case 0:
									return true;
								case 1:
									return areIntersecting_Segments_Contained(A2, A1, B1, B2);
							}
					}
				case 0:
					return true;
				case 1:
					switch ( compare(B2, A2) ) {
						case -1:
							return false;
						case 0:
							return true;
						case 1:
							switch ( compare(B2, A1) ) {
								case -1:
									return areIntersecting_Segments_Crossing(B1, B2, A2, A1);
								case 0:
									return true;
								case 1:
									return areIntersecting_Segments_Contained(B1, B2, A2, A1);
							}
					}
			}
		}
		else {
			if ( A1 < B2 || B1 < A2 )
				return false;
			
			switch ( compare(A2, B2) ) {
				case -1:
					switch ( compare(A1, B2) ) {
						case -1:
							return false;
						case 0:
							return true;
						case 1:
							switch ( compare(A1, B1) ) {
								case -1:
									return areIntersecting_Segments_Crossing(A2, A1, B2, B1);
								case 0:
									return true;
								case 1:
									return areIntersecting_Segments_Contained(A2, A1, B2, B1);
							}
					}
				case 0:
					return true;
				case 1:
					switch ( compare(B1, A2) ) {
						case -1:
							return false;
						case 0:
							return true;
						case 1:
							switch ( compare(B1, A1) ) {
								case -1:
									return areIntersecting_Segments_Crossing(B2, B1, A2, A1);
								case 0:
									return true;
								case 1:
									return areIntersecting_Segments_Contained(B2, B1, A2, A1);
							}
					}
			}
		}
	}
}



bool intersects(Polygon const &p1, Polygon const &p2)
{
	for ( auto seg1 = p1.edgeBegin(); seg1 != p1.edgeEnd(); ++seg1 ) {
		for ( auto seg2 = p2.edgeBegin(); seg2 != p2.edgeEnd(); ++seg2 ) {
			if ( intersects(*seg1, *seg2) )
				return true;
		}
	}

	return false;
}



double const SMALL_NUM = 0.00000001;



IntersectionShape intersect(Segment const s1, Segment const s2, Point &p1, Point &p2)
{
	// http://geomalgorithms.com/a05-_intersect-1.html
	
	Vector const u = s1.toVector();
	Vector const v = s2.toVector();
	Vector const w = s1.p1 - s2.p1;
	double const d = perpDotProduct(u, v);

	// test if they are parallel (includes either being a point)
	if ( fabs(d) < SMALL_NUM ) {           // parallel
		if ( perpDotProduct(u, w) != 0 || perpDotProduct(v, w) != 0 )
			return Isect_Empty;                    // they are NOT collinear
		
		// they are collinear or degenerate
		
		// check if they are degenerate  points
		double const du = dotProduct(u, u);
		double const dv = dotProduct(v, v);

		//if ( du == 0 && dv == 0) {            // both segments are points
		//	if (S1.P0 !=  S2.P0)         // they are distinct  points
		//		return 0;
		//	*I0 = S1.P0;                 // they are the same point
		//	return 1;
		//}
		//if (du==0) {                     // S1 is a single point
		//	if  (inSegment(S1.P0, S2) == 0)  // but is not in S2
		//		return 0;
		//	*I0 = S1.P0;
		//	return 1;
		//}
		//if (dv==0) {                     // S2 a single point
		//	if  (inSegment(S2.P0, S1) == 0)  // but is not in S1
		//		return 0;
		//	*I0 = S2.P0;
		//	return 1;
		//}

		// they are collinear segments - get  overlap (or not)

		double t0, t1;                    // endpoints of S1 in eqn for S2
		Vector const w2 = s1.p2 - s2.p1;
		if ( v.x != 0 ) {
			t0 = w.x / v.x;
			t1 = w2.x / v.x;
		}
		else {
			t0 = w.y / v.y;
			t1 = w2.y / v.y;
		}
		if ( t0 > t1 ) {                   // must have t0 smaller than t1
			double const t=t0; t0=t1; t1=t;    // swap if not
		}
		if ( t0 > 1 || t1 < 0 ) {
			return Isect_Empty;      // NO overlap
		}
		t0 = t0 < 0 ? 0 : t0;               // clip to min 0
		t1 = t1 > 1 ? 1 : t1;               // clip to max 1
		if ( t0 == t1 ) {                  // intersect is a point
			p1 = s2.p1 +  t0 * v;
			return Isect_Point;
		}

		// they overlap in a valid subsegment
		p1 = s2.p1 + t0 * v;
		p2 = s2.p1 + t1 * v;
		return Isect_Segment;
	}

	// the segments are skew and may intersect in a point
	// get the intersect parameter for S1
	double const sI = perpDotProduct(v, w) / d;
	if ( sI < 0 || sI > 1 )                // no intersect with S1
		return Isect_Empty;

	// get the intersect parameter for S2
	double const tI = perpDotProduct(u, w) / d;
	if ( tI < 0 || tI > 1 )                // no intersect with S2
		return Isect_Empty;

	p1 = s1.p1 + sI * u;                // compute S1 intersect point
	return Isect_Point;
}



bool inside(Point const &p, Polygon const &polygon)
{
	// Winding number algorithm
	
	int wn = 0;    // the  winding number counter

	// loop through all edges of the polygon
	for ( auto v = polygon.begin(); v != polygon.end(); ++v ) {   // edge from V[i] to  V[i+1]
		auto const v1 = next_cyclic(v, polygon);

		if ( v->y <= p.y ) {          // start y <= P.y
			if ( v1->y > p.y )      // an upward crossing
				if ( orientation(*v, *v1, p) == Left )  // P left of  edge
					++wn;            // have  a valid up intersect
		}
		else {                        // start y > P.y (no test needed)
			if ( v1->y <= p.y )     // a downward crossing
				if ( orientation(*v, *v1, p) == Right )  // P right of  edge
					--wn;            // have  a valid down intersect
		}
	}

	return wn != 0;
}



double polarAngle(Vector const &v)
{
	if ( v.x == 0 && v.y == 0 )
		return -1;
	
	if ( v.x == 0 )
		return (v.y > 0 ? M_PI_2 : 3*M_PI_2);
	
	double const theta = atan(v.y/v.x);
	if ( v.x > 0 )                                // 1 and 4 quadrants
		return (v.y >= 0 ? theta : 2*M_PI + theta);
	else                                          // 2 and 3 quadrants
		return M_PI + theta;
}



} // namespace poly
