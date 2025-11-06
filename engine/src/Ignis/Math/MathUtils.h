#pragma once

#include <cmath>
#include "MathConstants.h"

namespace Ignis::Math
{
	template <typename T>
	T abs(T value)
	{
		return value < 0 ? -value : value;
	}

	template <typename T>
	T min(T a, T b)
	{
		return a < b ? a : b;
	}

	template <typename T>
	T max(T a, T b)
	{
		return a > b ? a : b;
	}

	template <typename T>
	T sqrt(T value)
	{
		return std::sqrt(value);
	}

	template <typename T>
	T pow(T base, T exponent)
	{
		return std::pow(base, exponent);
	}

	template <typename T>
	T sin(T value)
	{
		return std::sin(value);
	}

	template <typename T>
	T cos(T value)
	{
		return std::cos(value);
	}

	template <typename T>
	T tan(T value)
	{
		return std::tan(value);
	}

	template <typename T>
	T asin(T value)
	{
		return std::asin(value);
	}

	template <typename T>
	T acos(T value)
	{
		return std::acos(value);
	}

	template <typename T>
	T atan(T value)
	{
		return std::atan(value);
	}

	template <typename T>
	T atan2(T y, T x)
	{
		return std::atan2(y, x);
	}

	template <typename T>
	T degrees(T radians)
	{
		return radians * 180 / pi;
	}

	template <typename T>
	T radians(T degrees)
	{
		return degrees * pi / 180;
	}

	template <typename T>
	T lerp(T a, T b, T t)
	{
		return a + (b - a) * t;
	}

	template <typename T>
	T clamp(T value, T min, T max)
	{
		return value < min ? min : value > max ? max : value;
	}

	template <typename T>
	T sign(T value)
	{
		return value < 0 ? -1 : value > 0 ? 1 : 0;
	}

	template <typename T>
	T round(T value)
	{
		return std::round(value);
	}

	template <typename T>
	T ceil(T value)
	{
		return std::ceil(value);
	}

	template <typename T>
	T floor(T value)
	{
		return std::floor(value);
	}

	template <typename T>
	T fract(T value)
	{
		return value - floor(value);
	}

	template <typename T>
	T mod(T value, T divisor)
	{
		return value % divisor;
	}

	template <typename T>
	T modf(T value, T* intpart)
	{
		return std::modf(value, intpart);
	}

	template <typename T>
	T fmod(T value, T divisor)
	{
		return std::fmod(value, divisor);
	}	
}