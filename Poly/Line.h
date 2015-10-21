#pragma once


namespace poly {

class Point;



class Line
{
public:
	Line(Point const &p1, Point const &p2);

	double A() const { return _A; }
	double B() const { return _B; }
	double C() const { return _C; }

protected:
	double _A, _B, _C;   ///< Coefficients of general line equation.
};



} // namespace poly
