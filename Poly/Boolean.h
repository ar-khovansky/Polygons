#pragma once

#include "Polygon.h"

#include <vector>



namespace poly {



/*!
 * Boolean operations on polygons
 *
 * All operations can be passed both CW and CCW polygons.
 *
 * Polygons must be simple.
 *
 * Not supported:
 * - touching of polygons by edges - throws exception.
 * - touching vertex to edge and vertex to vertex - produces incorrect results.
 */


std::vector<Polygon> add(Polygon const &p1, Polygon const &p2);

std::vector<Polygon> intersect(Polygon const &p1, Polygon const &p2);

/// Subtract p2 from p1.
std::vector<Polygon> subtract(Polygon const &p1, Polygon const &p2);

std::vector<Polygon> xor(Polygon const &p1, Polygon const &p2);

/// Partition of p1 by p2.
std::vector<Polygon> partition(Polygon const &p1, Polygon const &p2);



} // namespace poly
