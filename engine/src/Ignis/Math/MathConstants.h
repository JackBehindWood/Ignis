#pragma once

#include <limits>

namespace Ignis::Math
{
	constexpr double pi = 3.14159265358979323846;
	constexpr double e = 2.71828182845904523536;
	constexpr double epsilon = std::numeric_limits<double>::epsilon();
	constexpr double infinity = std::numeric_limits<double>::infinity();
}