#pragma once

#include "Point.h"
#include "Vector.h"
#include "Line.h"


namespace poly {



/// Segment ordered from p1 to p2.
//
class Segment
{
public:
	Point p1, p2;

//
	Segment(Point const &p1, Point const &p2) : p1(p1), p2(p2) {}

	Vector toVector() const { return Vector(p1, p2); }
	Line toLine() const { return Line(p1, p2); }
};



} // namespace poly
