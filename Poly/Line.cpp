#include "Line.h"

#include "Point.h"

using namespace poly;



Line::Line(Point const &p1, Point const &p2)
{
	if ( p1.y == p2.y ) {
		_A = 0 ;
		if ( p2.x > p1.x ) {
			_B = 1;
			_C = -p1.y;
		}
		else if ( p2.x == p1.x ) {
			_B = 0;
			_C = 0;
		}
		else {
			_B = -1;
			_C = p1.y;
		}
	}
	else if ( p2.x == p1.x ) {
		_B = 0;
		if ( p2.y > p1.y ) {
			_A = -1;
			_C = p1.x;
		}
		else if ( p2.y == p1.y ) {
			_A = 0;
			_C = 0;
		}
		else {
			_A = 1;
			_C = -p1.x;
		}
	}
	else {
		_A = p1.y - p2.y;
		_B = p2.x - p1.x;
		_C = -p1.x*_A - p1.y*_B;
	}
}
