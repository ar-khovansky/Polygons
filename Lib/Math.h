#pragma once

#include <float.h>



template<typename T>
int iRound(T t) { return int(t + _copysign(0.5, t)); }
