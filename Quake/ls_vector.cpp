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

#ifdef USE_LUA_SCRIPTING

#include <cassert>

#include "ls_common.h"
#include "ls_vector.h"


template <size_t N>
const LS_UserDataType<LS_Vector<N>>& LS_GetVectorUserDataType();

#define LS_DEFINE_VECTOR_USER_DATA_TYPE(COMPONENTS) \
	constexpr LS_UserDataType<LS_Vector##COMPONENTS> ls_vec##COMPONENTS##_type("vec" #COMPONENTS); \
	template <> const LS_UserDataType<LS_Vector<COMPONENTS>>& LS_GetVectorUserDataType() { return ls_vec##COMPONENTS##_type; } \
	template <> LS_Vector<COMPONENTS>& LS_GetVectorValue(lua_State* state, int index) { return LS_GetVectorUserDataType<COMPONENTS>().GetValue(state, index); } \
	template int LS_PushVectorValue<COMPONENTS>(lua_State* state, const LS_Vector<COMPONENTS>& value);

LS_DEFINE_VECTOR_USER_DATA_TYPE(2)
LS_DEFINE_VECTOR_USER_DATA_TYPE(3)
LS_DEFINE_VECTOR_USER_DATA_TYPE(4)

#undef LS_DEFINE_VECTOR_USER_DATA_TYPE


// Converts vector type component at given stack index to integer index [0..componentcount)
// On Lua side, valid numeric component indix start with one, [1..componentcount]
static int LS_GetVectorComponent(lua_State* state, int index, int componentcount)
{
	assert(componentcount > 1 && componentcount < 5);

	const int comptype = lua_type(state, index);
	int component = -1;

	if (comptype == LUA_TSTRING)
	{
		const char* compstr = lua_tostring(state, 2);
		assert(compstr);

		char compchar = compstr[0];

		if (compchar != '\0' && compstr[1] == '\0')
			component = compchar - 'x';

		if (componentcount == 4 && component == -1)
			component = 3;  // 'w' -> [3]

		if (component < 0 || component >= componentcount)
			luaL_error(state, "invalid vector component '%s'", compstr);
	}
	else if (comptype == LUA_TNUMBER)
	{
		component = lua_tointeger(state, 2) - 1;  // on C side, indices start with 0

		if (component < 0 || component >= componentcount)
			luaL_error(state, "vector component %d is out of range [1..%d]", component + 1, componentcount);  // on Lua side, indices start with 1
	}
	else
		luaL_error(state, "invalid type %s of vector component", lua_typename(state, comptype));

	assert(component >= 0 && component <= componentcount);
	return component;
}


//
// Expose 'vecN' userdata
//

template <size_t N, typename F>
int LS_VectorBinaryOperation(lua_State* state, F func)
{
	const auto& userdatatype = LS_GetVectorUserDataType<N>();
	const LS_Vector<N>& left = userdatatype.GetValue(state, 1);
	const LS_Vector<N>& right = userdatatype.GetValue(state, 2);

	const LS_Vector<N> result = func(left, right);
	return LS_PushVectorValue(state, result);
}

// Pushes result of two 'vecN' values addition
template <size_t N>
static int LS_value_vector_add(lua_State* state)
{
	return LS_VectorBinaryOperation<N>(state, operator+<N>);
}

// Pushes result of two 'vecN' values subtraction
template <size_t N>
static int LS_value_vector_sub(lua_State* state)
{
	return LS_VectorBinaryOperation<N>(state, operator-<N>);
}

// Pushes result of 'vecN' scale by a number or a dot product of two vectors
template <size_t N>
static int LS_value_vector_mul(lua_State* state)
{
	const int lefttype = lua_type(state, 1);

	if (LUA_TNUMBER == lefttype)
	{
		const lua_Number left = lua_tonumber(state, 1);
		const LS_Vector<N>& right = LS_GetVectorValue<N>(state, 2);
		return LS_PushVectorValue(state, right * left);
	}
	else if (LUA_TUSERDATA == lefttype)
	{
		const LS_Vector<N>& left = LS_GetVectorValue<N>(state, 1);
		const int righttype = lua_type(state, 2);

		if (LUA_TNUMBER == righttype)
		{
			const lua_Number right = luaL_checknumber(state, 2);
			return LS_PushVectorValue(state, left * right);
		}
		else if (LUA_TUSERDATA == righttype)
		{
			const LS_Vector<N>& right = LS_GetVectorValue<N>(state, 2);
			lua_pushnumber(state, left * right);
			return 1;
		}
		else
			luaL_error(state, "invalid argument of type %s passed to vector multiplication, expected number or %s",
				lua_typename(state, righttype), LS_GetVectorUserDataType<N>().GetName());
	}
	else
		luaL_error(state, "invalid argument of type %s passed to vector multiplication, expected number or %s",
			lua_typename(state, lefttype), LS_GetVectorUserDataType<N>().GetName());

	return 0;
}

// Pushes result of 'vecN' scale by a reciprocal of a number (the second argument)
template <size_t N>
static int LS_value_vector_div(lua_State* state)
{
	const LS_Vector<N>& left = LS_GetVectorValue<N>(state, 1);
	const lua_Number right = luaL_checknumber(state, 2);

	const LS_Vector<N> result = left * (1.f / right);
	return LS_PushVectorValue(state, result);
}

// Pushes result of 'vecN' inversion
template <size_t N>
static int LS_value_vector_unm(lua_State* state)
{
	LS_Vector<N> result = LS_GetVectorValue<N>(state, 1);

	for (size_t i = 0; i < N; ++i)
		result[i] = -result[i];

	return LS_PushVectorValue(state, result);
}

// Pushes result of 'vecN' value concatenation with some other value
template <size_t N>
static int LS_value_vector_concat(lua_State* state)
{
	lua_getglobal(state, "tostring");
	lua_pushvalue(state, -1);  // copy function for the second call
	lua_pushvalue(state, 1);  // the first argument
	lua_call(state, 1, 1);

	lua_insert(state, 3);  // swap result and tostring() function
	lua_pushvalue(state, 2);  // the second argument
	lua_call(state, 1, 1);

	lua_concat(state, 2);
	return 1;
}

// Pushes result of two 'vecN' values comparison for equality
template <size_t N>
static int LS_value_vector_eq(lua_State* state)
{
	const auto& userdatatype = LS_GetVectorUserDataType<N>();
	const LS_Vector<N>& left = userdatatype.GetValue(state, 1);
	const LS_Vector<N>& right = userdatatype.GetValue(state, 2);

	const bool equal = left == right;
	lua_pushboolean(state, equal);
	return 1;
}

// Pushes value of 'vecN' component, indexed by integer [0..N-1] or string 'x', 'y' (, 'z' (, 'w'))
template <size_t N>
static int LS_value_vector_index(lua_State* state)
{
	const LS_Vector<N>& value = LS_GetVectorValue<N>(state, 1);
	const size_t component = LS_GetVectorComponent(state, 2, 3);

	lua_pushnumber(state, value[component]);
	return 1;
}

// Sets new value of 'vecN' component, indexed by integer [0..N-1] or string 'x', 'y' (, 'z' (, 'w'))
template <size_t N>
static int LS_value_vector_newindex(lua_State* state)
{
	LS_Vector<N>& value = LS_GetVectorValue<N>(state, 1);
	const size_t component = LS_GetVectorComponent(state, 2, 3);

	lua_Number compvalue = luaL_checknumber(state, 3);
	value[component] = compvalue;
	return 0;
}

// Pushes string built from 'vecN' value
template <size_t N>
static int LS_value_vector_tostring(lua_State* state)
{
	const LS_Vector<N>& value = LS_GetVectorValue<N>(state, 1);

	luaL_Buffer buffer;
	luaL_buffinit(state, &buffer);

	for (size_t i = 0; i < N; ++i)
	{
		if (i > 0)
			luaL_addlstring(&buffer, " ", 1);

		char numbuf[16];
		size_t numlen = l_sprintf(numbuf, sizeof numbuf, "%.1f", value[i]);

		if (numlen > sizeof numbuf)
		{
			lua_pushnumber(state, value[i]);
			luaL_addvalue(&buffer);
		}
		else
		{
			if (numlen >= 3 && numbuf[numlen - 1] == '0' && numbuf[numlen - 2] == '.')
				numlen -= 2;  // skip .0

			luaL_addlstring(&buffer, numbuf, numlen);
		}
	}

	luaL_pushresult(&buffer);
	return 1;
}

// Creates and pushes 'vecN' userdata built from LS_Vector value
template <size_t N>
int LS_PushVectorValue(lua_State* state, const LS_Vector<N>& value)
{
	static const luaL_Reg functions[] =
	{
		// Math functions
		{ "__add", LS_value_vector_add<N> },
		{ "__sub", LS_value_vector_sub<N> },
		{ "__mul", LS_value_vector_mul<N> },
		{ "__div", LS_value_vector_div<N> },
		{ "__unm", LS_value_vector_unm<N> },

		// Other functions
		{ "__concat", LS_value_vector_concat<N> },
		{ "__eq", LS_value_vector_eq<N> },
		{ "__index", LS_value_vector_index<N> },
		{ "__newindex", LS_value_vector_newindex<N> },
		{ "__tostring", LS_value_vector_tostring<N> },
		{ nullptr, nullptr }
	};

	const auto& userdatatype = LS_GetVectorUserDataType<N>();
	userdatatype.New(state, nullptr, functions) = value;

	return 1;
}


//
// Helper functions for 'vecN' values
//

// Pushes new 'vecN' userdata built from individual component values
template <size_t N>
static int LS_global_vector_new(lua_State* state)
{
	LS_Vector<N> result;

	for (size_t i = 0; i < N; ++i)
		result[i] = luaL_optnumber(state, int(i + 1), 0.f);

	return LS_PushVectorValue(state, result);
}

template <size_t N>
const LS_Vector<N> LS_VectorMidPoint(const LS_Vector<N>& min, const LS_Vector<N>& max)
{
	LS_Vector<N> result;

	for (size_t i = 0; i < N; ++i)
		result[i] = min[i] + (max[i] - min[i]) * 0.5f;

	return result;
}

// Pushes new 'vecN' userdata which value is a mid point of functions arguments
template <size_t N>
static int LS_global_vector_mid(lua_State* state)
{
	return LS_VectorBinaryOperation<N>(state, LS_VectorMidPoint<N>);
}

// Pushes new 'vecN' userdata which value is a copy of its argument
template <size_t N>
static int LS_global_vector_copy(lua_State* state)
{
	const LS_Vector<N>& value = LS_GetVectorValue<N>(state, 1);
	return LS_PushVectorValue(state, value);
}

// Pushes a number which value is a dot product of functions arguments
template <size_t N>
static int LS_global_vector_dot(lua_State* state)
{
	const LS_Vector<N>& left = LS_GetVectorValue<N>(state, 1);
	const LS_Vector<N>& right = LS_GetVectorValue<N>(state, 2);

	const float result = left * right;
	lua_pushnumber(state, result);
	return 1;
}


// Creates 'vecN' table with helper functions for 'vecN' values
template <size_t N>
static void LS_InitVectorType(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "copy", LS_global_vector_copy<N> },
		{ "dot", LS_global_vector_dot<N> },
		{ "mid", LS_global_vector_mid<N> },
		{ "new", LS_global_vector_new<N> },
		{ NULL, NULL }
	};

	luaL_newlib(state, functions);
	lua_setglobal(state, LS_GetVectorUserDataType<N>().GetName());
}

static LS_Vector3 LS_Vector3CrossProduct(const LS_Vector3& left, const LS_Vector3& right)
{
	LS_Vector3 result;

	result[0] = left[1] * right[2] - left[2] * right[1];
	result[1] = left[2] * right[0] - left[0] * right[2];
	result[2] = left[0] * right[1] - left[1] * right[0];

	return result;
}

// Pushes new 'vec3' userdata which value is a cross product of functions arguments
static int LS_value_vector3_cross(lua_State* state)
{
	return LS_VectorBinaryOperation<3>(state, LS_Vector3CrossProduct);
}

void LS_InitVectorType(lua_State* state)
{
	LS_InitVectorType<2>(state);
	LS_InitVectorType<3>(state);
	LS_InitVectorType<4>(state);

	lua_getglobal(state, "vec3");
	lua_pushstring(state, "cross");
	lua_pushcfunction(state, LS_value_vector3_cross);
	lua_rawset(state, -3);
	lua_pop(state, 1);  // remove 'vec3' table
}

#endif // USE_LUA_SCRIPTING
