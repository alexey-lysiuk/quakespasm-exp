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

extern "C"
{
#include "quakedef.h"
}


static const LS_UserDataType ls_vec3_type =
{
	{ {{'v', 'e', 'c', '3'}} },
	sizeof(int) /* fourcc */ + sizeof(vec3_t)
};


//
// Expose vec3_t as 'vec3' userdata
//

static int LS_Vec3GetComponent(lua_State* state, int index)
{
	return LS_GetVectorComponent(state, index, 3);
}

// Gets value of 'vec3' from userdata at given index
vec_t* LS_GetVec3Value(lua_State* state, int index)
{
	void* value = LS_GetValueFromTypedUserData(state, index, &ls_vec3_type);
	assert(value);

	return *static_cast<vec3_t*>(value);
}

template <typename T>
int LS_Vec3Func(lua_State* state, T* func);

template <>
int LS_Vec3Func(lua_State* state, void (*func)(vec3_t, vec3_t, vec3_t))
{
	vec_t* v1 = LS_GetVec3Value(state, 1);
	vec_t* v2 = LS_GetVec3Value(state, 2);

	vec3_t result;
	func(v1, v2, result);

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes result of two 'vec3' values addition
static int LS_value_vec3_add(lua_State* state)
{
	return LS_Vec3Func(state, _VectorAdd);
}

// Pushes result of two 'vec3' values subtraction
static int LS_value_vec3_sub(lua_State* state)
{
	return LS_Vec3Func(state, _VectorSubtract);
}

// Pushes result of 'vec3' scale by a number (the second argument)
static int LS_value_vec3_mul(lua_State* state)
{
	vec_t* value = LS_GetVec3Value(state, 1);
	lua_Number scale = luaL_checknumber(state, 2);

	vec3_t result;
	VectorScale(value, scale, result);

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes result of 'vec3' scale by a reciprocal of a number (the second argument)
static int LS_value_vec3_div(lua_State* state)
{
	vec_t* value = LS_GetVec3Value(state, 1);
	lua_Number scale = 1.f / luaL_checknumber(state, 2);

	vec3_t result;
	VectorScale(value, scale, result);

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes result of 'vec3' inversion
static int LS_value_vec3_unm(lua_State* state)
{
	vec_t* value = LS_GetVec3Value(state, 1);

	vec3_t result;
	VectorCopy(value, result);
	VectorInverse(result);

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes result of 'vec3' value concatenation with some other value
static int LS_value_vec3_concat(lua_State* state)
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

// Pushes result of two 'vec3' values comparison for equality
static int LS_value_vec3_eq(lua_State* state)
{
	vec_t* v1 = LS_GetVec3Value(state, 1);
	vec_t* v2 = LS_GetVec3Value(state, 2);

	// TODO: compare with some epsilon
	lua_pushboolean(state, VectorCompare(v1, v2));
	return 1;
}

// Pushes value of 'vec3' component, indexed by integer [0..2] or string 'x', 'y', 'z'
static int LS_value_vec3_index(lua_State* state)
{
	vec_t* value = LS_GetVec3Value(state, 1);
	int component = LS_Vec3GetComponent(state, 2);

	lua_pushnumber(state, value[component]);
	return 1;
}

// Sets new value of 'vec3_t' component, indexed by integer [0..2] or string 'x', 'y', 'z'
static int LS_value_vec3_newindex(lua_State* state)
{
	vec_t* value = LS_GetVec3Value(state, 1);
	int component = LS_Vec3GetComponent(state, 2);

	lua_Number compvalue = luaL_checknumber(state, 3);
	value[component] = compvalue;

	return 0;
}

// Pushes string built from 'vec3' value
static int LS_value_vec3_tostring(lua_State* state)
{
	char buf[64];
	vec_t* value = LS_GetVec3Value(state, 1);
	int length = q_snprintf(buf, sizeof buf, "%.1f %.1f %.1f", value[0], value[1], value[2]);

	lua_pushlstring(state, buf, length);
	return 1;
}

// Creates and pushes 'vec3' userdata built from vec3_t value
void LS_PushVec3Value(lua_State* state, const vec_t* value)
{
	vec3_t* valueptr = static_cast<vec3_t*>(LS_CreateTypedUserData(state, &ls_vec3_type));
	assert(valueptr);

	VectorCopy(value, *valueptr);

	// Create and set 'vec3_t' metatable
	static const luaL_Reg functions[] =
	{
		// Math functions
		{ "__add", LS_value_vec3_add },
		{ "__sub", LS_value_vec3_sub },
		{ "__mul", LS_value_vec3_mul },
		{ "__div", LS_value_vec3_div },
		{ "__unm", LS_value_vec3_unm },

		// Other functions
		{ "__concat", LS_value_vec3_concat },
		{ "__eq", LS_value_vec3_eq },
		{ "__index", LS_value_vec3_index },
		{ "__newindex", LS_value_vec3_newindex },
		{ "__tostring", LS_value_vec3_tostring },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "vec3"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
}


//
// Helper functions for 'vec3' values
//

// Pushes new 'vec3' userdata built from individual component values
static int LS_global_vec3_new(lua_State* state)
{
	vec3_t result;

	for (int i = 0; i < 3; ++i)
		result[i] = luaL_optnumber(state, i + 1, 0.f);

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes new 'vec3' userdata which value is a mid point of functions arguments
static int LS_global_vec3_mid(lua_State* state)
{
	vec_t* min = LS_GetVec3Value(state, 1);
	vec_t* max = LS_GetVec3Value(state, 2);

	vec3_t result;

	for (int i = 0; i < 3; ++i)
		result[i] = min[i] + (max[i] - min[i]) * 0.5f;

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes new 'vec3' userdata which value is a copy of its argument
static int LS_global_vec3_copy(lua_State* state)
{
	vec_t* result = LS_GetVec3Value(state, 1);
	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes new 'vec3' userdata which value is a cross product of functions arguments
static int LS_global_vec3_cross(lua_State* state)
{
	return LS_Vec3Func(state, CrossProduct);
}

// Pushes a number which value is a dot product of functions arguments
static int LS_global_vec3_dot(lua_State* state)
{
	vec_t* v1 = LS_GetVec3Value(state, 1);
	vec_t* v2 = LS_GetVec3Value(state, 2);

	vec_t result = DotProduct(v1, v2);

	lua_pushnumber(state, result);
	return 1;
}


// Creates 'vec3' table with helper functions for 'vec3' values
void LS_InitVec3Type(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "copy", LS_global_vec3_copy },
		{ "cross", LS_global_vec3_cross },
		{ "dot", LS_global_vec3_dot },
		{ "mid", LS_global_vec3_mid },
		{ "new", LS_global_vec3_new },
		{ NULL, NULL }
	};

	luaL_newlib(state, functions);
	lua_setglobal(state, "vec3");
}

#endif // USE_LUA_SCRIPTING
