#pragma once

#include "Point.h"


namespace poly {



/// Vector from origin.
//
class Vector : public Point
{
public:
	Vector(double x, double y) : Point(x, y) {}
	
	Vector(Point const &p1, Point const &p2)
		: Point(p2.x - p1.x, p2.y - p1.y) {}

	Vector operator-() const { return Vector(-x, -y); }
};



/// Multiplication with scalar.
//
inline Vector operator*(double a, Vector const &v)
{ return Vector(a*v.x, a*v.y); }



} // namespace poly
