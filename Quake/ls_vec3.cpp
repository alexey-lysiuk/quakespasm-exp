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
#include <utility>

#include "ls_common.h"

extern "C"
{
#include "quakedef.h"
}


// Converts vector type component at given stack index to integer index [0..componentcount)
// On Lua side, valid numeric component indix start with one, [1..componentcount]
int LS_GetVectorComponent(lua_State* state, int index, int componentcount)
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


template <size_t N>
class LS_Vector
{
public:
	using UserDataType = LS_UserDataType<LS_Vector<N>>;
	static const UserDataType& GetUserDataType();

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

//	LS_Vector<components>& operator=(const float (&newvalue)[components])
//	{
//		memcpy(value, newvalue, sizeof(float) * components);
//		return *this;
//	}

//	const float* data() const { return value; }
//	float* data() { return value; }

private:
	float value[N];
};

template <size_t N>
const LS_Vector<N> operator+(const LS_Vector<N>& left, const LS_Vector<N>& right)
{
	LS_Vector<N> result;

	for (size_t i = 0; i < N; ++i)
		result[i] = left[i] + right[i];

	return result;
}

template <size_t N>
const LS_Vector<N> operator-(const LS_Vector<N>& left, const LS_Vector<N>& right)
{
	LS_Vector<N> result;

	for (size_t i = 0; i < N; ++i)
		result[i] = left[i] - right[i];

	return result;
}

template <size_t N>
const bool operator==(const LS_Vector<N>& left, const LS_Vector<N>& right)
{
	// TODO: compare with some epsilon

	for (size_t i = 0; i < N; ++i)
		if (left[i] != right[i])
			return false;

	return true;
}


#define LS_DEFINE_VECTOR_TYPE(COMPONENTS) \
	using LS_Vector##COMPONENTS = LS_Vector<COMPONENTS>; \
	constexpr LS_UserDataType<LS_Vector##COMPONENTS> ls_vec##COMPONENTS##_type("vec" #COMPONENTS); \
	template <> const LS_Vector##COMPONENTS::UserDataType& LS_Vector##COMPONENTS::GetUserDataType() { return ls_vec##COMPONENTS##_type; }

LS_DEFINE_VECTOR_TYPE(2)
LS_DEFINE_VECTOR_TYPE(3)
LS_DEFINE_VECTOR_TYPE(4)


// TODO: remove this!
// Gets value of 'vec3' from userdata at given index
vec_t* LS_GetVec3Value(lua_State* state, int index)
{
	return &ls_vec3_type.GetValue(state, index)[0];
}


//
// Expose 'vec?' userdata
//

template <size_t N>
int LS_PushVectorValue(lua_State* state, const LS_Vector<N>& value);

template <size_t N, typename F>
int LS_VectorBinaryOperation(lua_State* state, F func)
{
	const auto& userdatatype = LS_Vector<N>::GetUserDataType();
	const LS_Vector<N>& left = userdatatype.GetValue(state, 1);
	const LS_Vector<N>& right = userdatatype.GetValue(state, 2);

	const LS_Vector<N> result = func(left, right);
	return LS_PushVectorValue(state, result);
}

// Pushes result of two 'vec?' values addition
template <size_t N>
static int LS_value_vector_add(lua_State* state)
{
	return LS_VectorBinaryOperation<N>(state, operator+<N>);
}

// Pushes result of two 'vec?' values subtraction
template <size_t N>
static int LS_value_vector_sub(lua_State* state)
{
	return LS_VectorBinaryOperation<N>(state, operator-<N>);
}

//// Pushes result of 'vec3' scale by a number (the second argument)
//static int LS_value_vec3_mul(lua_State* state)
//{
//	vec_t* value = LS_GetVec3Value(state, 1);
//	lua_Number scale = luaL_checknumber(state, 2);
//
//	vec3_t result;
//	VectorScale(value, scale, result);
//
//	LS_PushVec3Value(state, result);
//	return 1;
//}
//
//// Pushes result of 'vec3' scale by a reciprocal of a number (the second argument)
//static int LS_value_vec3_div(lua_State* state)
//{
//	vec_t* value = LS_GetVec3Value(state, 1);
//	lua_Number scale = 1.f / luaL_checknumber(state, 2);
//
//	vec3_t result;
//	VectorScale(value, scale, result);
//
//	LS_PushVec3Value(state, result);
//	return 1;
//}
//
//// Pushes result of 'vec3' inversion
//static int LS_value_vec3_unm(lua_State* state)
//{
//	vec_t* value = LS_GetVec3Value(state, 1);
//
//	vec3_t result;
//	VectorCopy(value, result);
//	VectorInverse(result);
//
//	LS_PushVec3Value(state, result);
//	return 1;
//}

// Pushes result of 'vec?' value concatenation with some other value
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

// Pushes result of two 'vec?' values comparison for equality
template <size_t N>
static int LS_value_vector_eq(lua_State* state)
{
	const auto& userdatatype = LS_Vector<N>::GetUserDataType();
	const LS_Vector<N>& left = userdatatype.GetValue(state, 1);
	const LS_Vector<N>& right = userdatatype.GetValue(state, 2);

	const bool equal = left == right;
	lua_pushboolean(state, equal);
	return 1;
}

//// Pushes value of 'vec3' component, indexed by integer [0..2] or string 'x', 'y', 'z'
//static int LS_value_vec3_index(lua_State* state)
//{
//	vec_t* value = LS_GetVec3Value(state, 1);
//	int component = LS_Vec3GetComponent(state, 2);
//
//	lua_pushnumber(state, value[component]);
//	return 1;
//}
//
//// Sets new value of 'vec3_t' component, indexed by integer [0..2] or string 'x', 'y', 'z'
//static int LS_value_vec3_newindex(lua_State* state)
//{
//	vec_t* value = LS_GetVec3Value(state, 1);
//	int component = LS_Vec3GetComponent(state, 2);
//
//	lua_Number compvalue = luaL_checknumber(state, 3);
//	value[component] = compvalue;
//
//	return 0;
//}

// Pushes string built from 'vec?' value
template <size_t N>
static int LS_value_vector_tostring(lua_State* state)
{
	const auto& userdatatype = LS_Vector<N>::GetUserDataType();
	const LS_Vector<N>& value = userdatatype.GetValue(state, 1);

	luaL_Buffer buffer;
	luaL_buffinit(state, &buffer);

	for (size_t i = 0; i < N; ++i)
	{
		char numbuf[32];
		const int numlen = lua_number2str(numbuf, sizeof numbuf, value[i]);

		if (i > 0)
			luaL_addlstring(&buffer, " ", 1);

		luaL_addlstring(&buffer, numbuf, numlen);
	}

	luaL_pushresult(&buffer);
	return 1;
}

// Creates and pushes 'vec?' userdata built from vec3_t value
template <size_t N>
int LS_PushVectorValue(lua_State* state, const LS_Vector<N>& value)
{
	const auto& userdatatype = value.GetUserDataType();

	LS_Vector<N>& newvalue = userdatatype.New(state);
	newvalue = value;

	// Create and set 'vec3_t' metatable
	static const luaL_Reg functions[] =
	{
		// Math functions
		{ "__add", LS_value_vector_add<N> },
		{ "__sub", LS_value_vector_sub<N> },
//		{ "__mul", LS_value_vec3_mul },
//		{ "__div", LS_value_vec3_div },
//		{ "__unm", LS_value_vec3_unm },

		// Other functions
		{ "__concat", LS_value_vector_concat<N> },
		{ "__eq", LS_value_vector_eq<N> },
//		{ "__index", LS_value_vec3_index },
//		{ "__newindex", LS_value_vec3_newindex },
		{ "__tostring", LS_value_vector_tostring<N> },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, userdatatype.GetName()))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
	return 1;
}

// TODO: Remove this!
void LS_PushVec3Value(lua_State* state, const vec_t* value)
{
	LS_Vector3 tempvalue;
	tempvalue[0] = value[0];
	tempvalue[1] = value[1];
	tempvalue[2] = value[2];
	LS_PushVectorValue(state, tempvalue);
}


//
// Helper functions for 'vec3' values
//

// Pushes new 'vec?' userdata built from individual component values
template <size_t N>
static int LS_global_vector_new(lua_State* state)
{
	LS_Vector<N> result;

	for (size_t i = 0; i < N; ++i)
		result[i] = luaL_optnumber(state, i + 1, 0.f);

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

// Pushes new 'vec?' userdata which value is a mid point of functions arguments
template <size_t N>
static int LS_global_vector_mid(lua_State* state)
{
	return LS_VectorBinaryOperation<N>(state, LS_VectorMidPoint<N>);
}

//// Pushes new 'vec3' userdata which value is a copy of its argument
//static int LS_global_vec3_copy(lua_State* state)
//{
//	vec_t* result = LS_GetVec3Value(state, 1);
//	LS_PushVec3Value(state, result);
//	return 1;
//}
//
//// Pushes new 'vec3' userdata which value is a cross product of functions arguments
//static int LS_global_vec3_cross(lua_State* state)
//{
//	return LS_Vec3Func(state, CrossProduct);
//}
//
//// Pushes a number which value is a dot product of functions arguments
//static int LS_global_vec3_dot(lua_State* state)
//{
//	vec_t* v1 = LS_GetVec3Value(state, 1);
//	vec_t* v2 = LS_GetVec3Value(state, 2);
//
//	vec_t result = DotProduct(v1, v2);
//
//	lua_pushnumber(state, result);
//	return 1;
//}


// Creates 'vec?' table with helper functions for 'vec?' values

template <size_t N>
static void LS_InitVectorType(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
//		{ "copy", LS_global_vec3_copy },
//		{ "cross", LS_global_vec3_cross },
//		{ "dot", LS_global_vec3_dot },
		{ "mid", LS_global_vector_mid<N> },
		{ "new", LS_global_vector_new<N> },
		{ NULL, NULL }
	};

	const char* vectortypename = LS_Vector<N>::GetUserDataType().GetName();
	luaL_newlib(state, functions);
	lua_setglobal(state, vectortypename);
}

void LS_InitVectorType(lua_State* state)
{
	LS_InitVectorType<2>(state);
	LS_InitVectorType<3>(state);
	LS_InitVectorType<4>(state);
}

#endif // USE_LUA_SCRIPTING
