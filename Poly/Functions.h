#pragma once

#include "Point.h"
#include "Vector.h"
#include "Segment.h"
#include "Line.h"



namespace poly {

class Polygon;



inline Point operator+(Point const &p, Vector const &v)
	{ return Point(p.x + v.x, p.y + v.y); }

inline Point & operator+=(Point &p, Vector const &v)
	{ p.x += v.x; p.y += v.y; return p; }

inline Point & operator-=(Point &p, Vector const &v)
	{ p.x -= v.x; p.y -= v.y; return p; }

inline Vector operator-(Point const &p1, Point const &p2)
	{ return Vector(p2, p1); }



template <typename T>
int compare(T const &t1, T const &t2) { return t1 < t2 ? -1 : (t2 < t1 ? 1 : 0); }



inline double sqr(double d) { return d*d; }



inline double perpDotProduct(Vector const &v1, Vector const &v2)
{ return v1.x * v2.y - v1.y * v2.x; }



enum Orientation { Left = 1, Right = -1, Collinear = 0 };

/// Get orientation of v2 relative to v1.
//
inline Orientation orientation(Vector const &v1, Vector const &v2) {
	double const p = perpDotProduct(v1, v2);
	return p < 0 ? Left : (p > 0 ? Right : Collinear);
}

inline Orientation orientation(Point const &p0, Point const &p1, Point const &p2)
{ return orientation(Vector(p0, p1), Vector(p0, p2)); }

/// Get orientation of point relative to vector.
//
inline Orientation orientation(Point const &p, Segment const &s)
{ return orientation(s.toVector(), Vector(s.p1, p)); }



inline double dotProduct(Vector const &v1, Vector const &v2)
	{ return v1.x * v2.x + v1.y * v2.y; }



// Square of distance

inline double distanceSqr(Point const &p1, Point const &p2)
	{ return sqr(p1.x - p2.x) + sqr(p1.y - p2.y); }

/*! Return square of distance between point and segment, if projection of point to corresponging
 *  line lies within a segment. Otherwise return DBL_MAX.
 */
double distanceSqr_Strict(Point const &p, Segment const &s);

inline double distanceSqr(Point const &p, Line const &l) {
	double const A = l.A(), B = l.B();
	return sqr(A*p.x + B*p.y + l.C()) / (sqr(A) + sqr(B));
}

/// Distance from point to polygon.
/*! Polygon is interpreted as a contour, so point can lie inside.
 */
double distanceSqr(Point const &p, Polygon const &polygon);



// Intersection tests

bool intersects(Segment const &s1, Segment const &s2);

bool intersects(Polygon const &p1, Polygon const &p2);


enum IntersectionShape { Isect_Empty, Isect_Point, Isect_Segment };

/// Find intersection of two segments.
/*!
 * \pre Segments are not degenerate.
 *
 * \param[in]  s1  Segment 1.
 * \param[in]  s2  Segment 2.
 * \param[out] p1  Single or first intersection point, if exists.
 * \param[out] p2  Second intersection point, if exists.
 *
 * \return IntersectionShape.
 */
IntersectionShape intersect(Segment const s1, Segment const s2, Point &p1, Point &p2);



/// Test if point is inside polygon.
/*!
 * \pre Polygon can be self-intersecting.
 */
bool inside(Point const &p, Polygon const &polygon);



/// Angle from X axis to vector.
double polarAngle(Vector const &v);



} // namespace poly
