/*

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#pragma once

#ifdef USE_LUA_SCRIPTING

#include <cassert>
#include <cstring>

template <size_t N>
class LS_Vector
{
	static_assert(N >= 2 && N <= 4);

public:
	LS_Vector() = default;
	LS_Vector(const LS_Vector&) = default;
	LS_Vector(LS_Vector&&) = default;
	LS_Vector& operator=(const LS_Vector&) = default;
	LS_Vector& operator=(LS_Vector&&) = default;

	explicit LS_Vector(const float* const array)
	{
		memcpy(value, array, sizeof value);
	}

	LS_Vector& operator=(const float* const array)
	{
		memcpy(value, array, sizeof value);
		return *this;
	}

	const float operator[](const size_t component) const
	{
		assert(component < N);
		return value[component];
	}

	float& operator[](const size_t component)
	{
		assert(component < N);
		return value[component];
	}

	static LS_Vector Zero()
	{
		LS_Vector result;
		memset(result.value, 0, sizeof result.value);
		return result;
	}

private:
	float value[N];
};

template <size_t N>
inline const LS_Vector<N> operator+(const LS_Vector<N>& left, const LS_Vector<N>& right)
{
	LS_Vector<N> result;

	for (size_t i = 0; i < N; ++i)
		result[i] = left[i] + right[i];

	return result;
}

template <size_t N>
inline const LS_Vector<N> operator-(const LS_Vector<N>& left, const LS_Vector<N>& right)
{
	LS_Vector<N> result;

	for (size_t i = 0; i < N; ++i)
		result[i] = left[i] - right[i];

	return result;
}

template <size_t N>
inline float operator*(const LS_Vector<N>& left, const LS_Vector<N>& right)
{
	float result = 0.f;

	for (size_t i = 0; i < N; ++i)
		result += left[i] * right[i];

	return result;
}

template <size_t N>
inline const LS_Vector<N> operator*(const LS_Vector<N>& left, const float right)
{
	LS_Vector<N> result;

	for (size_t i = 0; i < N; ++i)
		result[i] = left[i] * right;

	return result;
}

template <size_t N>
inline const bool operator==(const LS_Vector<N>& left, const LS_Vector<N>& right)
{
	// TODO: compare with some epsilon?

	for (size_t i = 0; i < N; ++i)
		if (left[i] != right[i])
			return false;

	return true;
}

using LS_Vector2 = LS_Vector<2>;
using LS_Vector3 = LS_Vector<3>;
using LS_Vector4 = LS_Vector<4>;


struct lua_State;

template <size_t N>
LS_Vector<N>& LS_GetVectorValue(lua_State* state, int index);

template <size_t N>
int LS_PushVectorValue(lua_State* state, const LS_Vector<N>& value);

void LS_InitVectorType(lua_State* state);

#endif // USE_LUA_SCRIPTING
