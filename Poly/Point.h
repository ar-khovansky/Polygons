#pragma once


namespace poly {



class Point
{
public:
	Point() {}
	Point(double x, double y) : x(x), y(y) {}

	bool operator==(Point const &r) const { return x == r.x && y == r.y; }
	
	// Lexicographical order
	//
	bool operator<(Point const &r) const { return x < r.x || (x == r.x && y < r.y); }

// Fields
	double x, y;
};



} // namespace poly
