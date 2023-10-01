/*
 * lua_script.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef USE_LUA_SCRIPTING

#include <assert.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "quakedef.h"
#include "tlsf.h"


qboolean ED_GetFieldByIndex(edict_t* ed, size_t fieldindex, const char** name, etype_t* type, const eval_t** value);
qboolean ED_GetFieldByName(edict_t* ed, const char* name, etype_t* type, const eval_t** value);
const char* ED_GetFieldNameByOffset(int offset);

static lua_State* ls_state;
static const char* ls_console_name = "console";

static tlsf_t ls_memory;
static const size_t ls_memorysize = 1024 * 1024;

typedef struct
{
	size_t usedbytes;
	size_t usedblocks;
	size_t freebytes;
	size_t freeblocks;
} LS_MemoryStats;


typedef union
{
	struct { char ch[4]; };
	int fourcc;
} LS_UserDataType;

static const LS_UserDataType ls_edict_type = { {{'e', 'd', 'c', 't'}} };
static const LS_UserDataType ls_vec3_type = { {{'v', 'e', 'c', '3'}} };

static void* LS_CreateTypedUserData(lua_State* state, LS_UserDataType type)
{
	size_t size;

	if (type.fourcc == ls_edict_type.fourcc)
		size = sizeof(int);  // edict index
	else if (type.fourcc == ls_vec3_type.fourcc)
		size = sizeof(vec3_t);
	else
	{
		assert(false);
		size = 0;
	}

	int* result = lua_newuserdatauv(state, sizeof type + size, 0);
	assert(result);

	*result = type.fourcc;
	result += 1;

	return result;
}

static void* LS_GetValueFromTypedUserData(lua_State* state, int index, LS_UserDataType type)
{
	luaL_checktype(state, index, LUA_TUSERDATA);

	int* result = lua_touserdata(state, index);
	assert(result);

	if (type.fourcc != *result)
	{
		char expected[5], actual[5];

		memcpy(expected, &type, 4);
		expected[4] = '\0';
		memcpy(actual, result, 4);
		actual[4] = '\0';

		luaL_error(state, "Invalid userdata type, expected '%s', got '%s'", expected, actual);
	}

	result += 1;

	return result;
}


//
// Expose vec3_t as 'vec3' userdata
//

// Converts 'vec3' component at given stack index to vec3_t integer index [0..2]
// On Lua side, valid numeric component indices are 1, 2, 3
static int LS_Vec3GetComponent(lua_State* state, int index)
{
	int comptype = lua_type(state, index);
	int component = -1;

	if (comptype == LUA_TSTRING)
	{
		const char* compstr = lua_tostring(state, 2);
		assert(compstr);

		char compchar = compstr[0];

		if (compchar != '\0' && compstr[1] == '\0')
			component = compchar - 'x';

		if (component < 0 || component > 2)
			luaL_error(state, "Invalid vec3 component '%s'", compstr);
	}
	else if (comptype == LUA_TNUMBER)
	{
		component = lua_tointeger(state, 2) - 1;  // on C side, indices start with 0

		if (component < 0 || component > 2)
			luaL_error(state, "vec3 component %d is out of range [1..3]", component + 1);  // on Lua side, indices start with 1
	}
	else
		luaL_error(state, "Invalid type %s of vec3 component", lua_typename(state, comptype));

	assert(component >= 0 && component <= 2);
	return component;
}

// Get value of 'vec3' from userdata at given index
static vec_t* LS_Vec3GetValue(lua_State* state, int index)
{
	vec3_t* value = LS_GetValueFromTypedUserData(state, index, ls_vec3_type);
	assert(value);

	return *value;
}

static void LS_PushVec3Value(lua_State* state, const vec_t* value);

// Pushes result of two 'vec3' values addition
static int LS_value_vec3_add(lua_State* state)
{
	vec_t* v1 = LS_Vec3GetValue(state, 1);
	vec_t* v2 = LS_Vec3GetValue(state, 2);

	vec3_t result;
	VectorAdd(v1, v2, result);

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes result of two 'vec3' values subtraction
static int LS_value_vec3_sub(lua_State* state)
{
	vec_t* v1 = LS_Vec3GetValue(state, 1);
	vec_t* v2 = LS_Vec3GetValue(state, 2);

	vec3_t result;
	VectorSubtract(v1, v2, result);

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes result of 'vec3' scale by a number (the second argument)
static int LS_value_vec3_mul(lua_State* state)
{
	vec_t* value = LS_Vec3GetValue(state, 1);
	lua_Number scale = luaL_checknumber(state, 2);

	vec3_t result;
	VectorScale(value, scale, result);

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes result of 'vec3' scale by a reciprocal of a number (the second argument)
static int LS_value_vec3_div(lua_State* state)
{
	vec_t* value = LS_Vec3GetValue(state, 1);
	lua_Number scale = 1.f / luaL_checknumber(state, 2);

	vec3_t result;
	VectorScale(value, scale, result);

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes result of 'vec3' inversion
static int LS_value_vec3_unm(lua_State* state)
{
	vec_t* value = LS_Vec3GetValue(state, 1);

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
	vec_t* v1 = LS_Vec3GetValue(state, 1);
	vec_t* v2 = LS_Vec3GetValue(state, 2);

	// TODO: compare with some epsilon
	lua_pushboolean(state, VectorCompare(v1, v2));
	return 1;
}

// Pushes value of 'vec3' component, indexed by integer [0..2] or string 'x', 'y', 'z'
static int LS_value_vec3_index(lua_State* state)
{
	vec_t* value = LS_Vec3GetValue(state, 1);
	int component = LS_Vec3GetComponent(state, 2);

	lua_pushnumber(state, value[component]);
	return 1;
}

// Sets new value of 'vec3_t' component, indexed by integer [0..2] or string 'x', 'y', 'z'
static int LS_value_vec3_newindex(lua_State* state)
{
	vec_t* value = LS_Vec3GetValue(state, 1);
	int component = LS_Vec3GetComponent(state, 2);

	lua_Number compvalue = luaL_checknumber(state, 3);
	value[component] = compvalue;

	return 0;
}

// Pushes string built from 'vec3' value
static int LS_value_vec3_tostring(lua_State* state)
{
	char buf[64];
	vec_t* value = LS_Vec3GetValue(state, 1);
	int length = q_snprintf(buf, sizeof buf, "%.1f %.1f %.1f", value[0], value[1], value[2]);

	lua_pushlstring(state, buf, length);
	return 1;
}

// Creates and pushes 'vec3' userdata built from vec3_t value
static void LS_PushVec3Value(lua_State* state, const vec_t* value)
{
	vec3_t* valueptr = LS_CreateTypedUserData(state, ls_vec3_type);
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
	vec_t* min = LS_Vec3GetValue(state, 1);
	vec_t* max = LS_Vec3GetValue(state, 2);

	vec3_t result;

	for (int i = 0; i < 3; ++i)
		result[i] = min[i] + (max[i] - min[i]) * 0.5f;

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes new 'vec3' userdata which value is a cross product of functions arguments
static int LS_global_vec3_cross(lua_State* state)
{
	vec_t* v1 = LS_Vec3GetValue(state, 1);
	vec_t* v2 = LS_Vec3GetValue(state, 2);

	vec3_t result;
	CrossProduct(v1, v2, result);

	LS_PushVec3Value(state, result);
	return 1;
}

// Pushes a number which value is a dot product of functions arguments
static int LS_global_vec3_dot(lua_State* state)
{
	vec_t* v1 = LS_Vec3GetValue(state, 1);
	vec_t* v2 = LS_Vec3GetValue(state, 2);

	vec_t result = DotProduct(v1, v2);

	lua_pushnumber(state, result);
	return 1;
}


//
// Expose edict_t as 'edict' userdata
//

// Pushes field value by its type and name
static void LS_PushEdictFieldValue(lua_State* state, etype_t type, const eval_t* value)
{
	assert(type != ev_bad);
	assert(value);

	switch (type)
	{
	case ev_void:
		lua_pushstring(state, "void");
		break;

	case ev_string:
		lua_pushstring(state, PR_GetString(value->string));
		break;

	case ev_float:
		lua_pushnumber(state, value->_float);
		break;

	case ev_vector:
		LS_PushVec3Value(state, value->vector);
		break;

	case ev_entity:
		lua_pushfstring(state, "entity %d", NUM_FOR_EDICT(PROG_TO_EDICT(value->edict)) + 1);  // on Lua side, indices start with 1
		break;

	case ev_field:
		lua_pushfstring(state, ".%s", ED_GetFieldNameByOffset(value->_int));
		break;

	case ev_function:
		lua_pushfstring(state, "%s()", PR_GetString((pr_functions + value->function)->s_name));
		break;

	case ev_pointer:
		lua_pushstring(state, "pointer");
		break;

	default:
		// Unknown type, e.g. some of FTE extensions
		lua_pushfstring(state, "bad type %d", type);
		break;
	}
}

// Gets pointer to edict_t from 'edict' userdata
static edict_t* LS_GetEdictFromUserData(lua_State* state)
{
	int* indexptr = LS_GetValueFromTypedUserData(state, 1, ls_edict_type);
	assert(indexptr);

	int index = *indexptr;
	return (index >= 0 && index < sv.num_edicts) ? EDICT_NUM(index) : NULL;
}

// Pushes value of edict field by its name
// or pushes a table with name, type, value by field's numerical index
static int LS_value_edict_index(lua_State* state)
{
	edict_t* ed = LS_GetEdictFromUserData(state);

	if (ed == NULL || ed->free)
	{
		lua_pushnil(state);  // TODO: default value instead of nil
		return 1;
	}

	const char* name;
	etype_t type;
	const eval_t* value;

	int indextype = lua_type(state, 2);

	if (indextype == LUA_TSTRING)
	{
		name = luaL_checkstring(state, 2);

		if (ED_GetFieldByName(ed, name, &type, &value))
			LS_PushEdictFieldValue(state, type, value);
		else
			lua_pushnil(state);
	}
	else if (indextype == LUA_TNUMBER)
	{
		// TODO: optimize numeric indexing to be O(n) instead of O(n^2)

		int fieldindex = lua_tointeger(state, 2);
		int fieldswithvalues = 0;  // a value before first valid index that starts with one on Lua side

		for (int i = 1; i < progs->numfielddefs; ++i)
		{
			const char* name;
			etype_t type;
			const eval_t* value;

			if (ED_GetFieldByIndex(ed, i, &name, &type, &value))
			{
				++fieldswithvalues;

				if (fieldindex == fieldswithvalues)
				{
					lua_createtable(state, 0, 2);

					lua_pushstring(state, name);
					lua_setfield(state, -2, "name");
					lua_pushnumber(state, type);
					lua_setfield(state, -2, "type");
					LS_PushEdictFieldValue(state, type, value);
					lua_setfield(state, -2, "value");

					break;
				}
			}
		}

		if (fieldindex > fieldswithvalues)
			lua_pushnil(state);  // no such index
	}
	else
		luaL_error(state, "Invalid type %s of edict index", lua_typename(state, indextype));

	return 1;
}

// Pushes next() function, table with edict's fields and values, initial value (nil)
static int LS_value_edict_pairs(lua_State* state)
{
	edict_t* ed = LS_GetEdictFromUserData(state);
	qboolean valid = ed != NULL;

	lua_getglobal(state, "next");
	lua_createtable(state, 0, valid ? 16 : 0);

	if (valid)
	{
		for (int i = 1; i < progs->numfielddefs; ++i)
		{
			const char* name;
			etype_t type;
			const eval_t* value;

			if (ED_GetFieldByIndex(ed, i, &name, &type, &value))
			{
				LS_PushEdictFieldValue(state, type, value);
				lua_setfield(state, -2, name);
			}
		}
	}

	lua_pushnil(state);
	return 3;
}

// Pushes string representation of given edict
static int LS_value_edict_tostring(lua_State* state)
{
	edict_t* ed = LS_GetEdictFromUserData(state);

	if (ed == NULL)
	{
		lua_pushstring(state, "invalid edict");
		return 1;
	}

	const char* description;

	if (ed->free)
		description = "<free>";
	else
	{
		description = PR_GetString(ed->v.classname);

		if (description[0] == '\0')
			description = PR_GetString(ed->v.model);

		if (description[0] == '\0')
			description = "<unnamed>";
	}

	int index = NUM_FOR_EDICT(ed) + 1;  // on Lua side, indices start with 1
	lua_pushfstring(state, "edict %d: %s", index, description);

	return 1;
}

// Sets metatable for edict table
static void LS_SetEdictMetaTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_value_edict_index },
		{ "__pairs", LS_value_edict_pairs },
		{ "__tostring", LS_value_edict_tostring },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "edict"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
}


//
// Expose sv.edicts as 'edicts' global table
//

// Pushes either
// * edict userdata by its integer index, [1..num_edicts]
// * method of 'edicts' userdata by its name
static int LS_global_edicts_index(lua_State* state)
{
	if (!sv.active)
	{
		lua_pushnil(state);
		return 1;
	}

	int indextype = lua_type(state, 2);

	if (indextype == LUA_TNUMBER)
	{
		// Check edict index, [1..num_edicts], for validity
		lua_Integer index = lua_tointeger(state, 2);

		if (index > 0 && index <= sv.num_edicts)
		{
			int* indexptr = LS_CreateTypedUserData(state, ls_edict_type);
			assert(indexptr);
			*indexptr = index - 1;  // on C side, indices start with 0
			LS_SetEdictMetaTable(state);
		}
		else
			lua_pushnil(state);
	}
	else
		luaL_error(state, "Invalid type %s of edicts key", lua_typename(state, indextype));

	return 1;
}

// Pushes number of edicts
static int LS_global_edicts_len(lua_State* state)
{
	lua_pushinteger(state, sv.active ? sv.num_edicts : 0);
	return 1;
}

static int LS_global_edicts_isfree(lua_State* state)
{
	qboolean isfree = true;

	if (sv.active)
	{
		int indextype = lua_type(state, 1);

		if (indextype == LUA_TNUMBER)
		{
			// Check edict index, [1..num_edicts], for validity
			lua_Integer index = lua_tointeger(state, 1);

			if (index > 0 && index <= sv.num_edicts)
			{
				edict_t* edict = EDICT_NUM(index - 1);  // on C side, indices start with zero
				assert(edict);

				isfree = edict->free;
			}
		}
		else if (indextype == LUA_TUSERDATA)
		{
			edict_t* edict = LS_GetEdictFromUserData(state);
			if (edict)
				isfree = edict->free;
		}
		else
			luaL_error(state, "Invalid type %s of edicts key", lua_typename(state, indextype));
	}

	lua_pushboolean(state, isfree);
	return 1;
}


//
// Expose 'player' global table with corresponding helper functions
//

static int LS_global_player_setpos(lua_State* state)
{
	vec_t* pos = LS_Vec3GetValue(state, 1);
	char anglesstr[64] = { '\0' };

	if (lua_type(state, 2) == LUA_TUSERDATA)
	{
		vec_t* angles = LS_Vec3GetValue(state, 2);
		q_snprintf(anglesstr, sizeof anglesstr, " %.02f %.02f %.02f", angles[0], angles[1], angles[2]);
	}

	char command[128];
	q_snprintf(command, sizeof command, "setpos %.02f %.02f %.02f%s;", pos[0], pos[1], pos[2], anglesstr);

	Cbuf_AddText(command);
	return 0;
}

static int LS_PlayerCheatCommand(lua_State* state, const char* command)
{
	const char* argstr = "";

	if (lua_isnumber(state, 1))
		argstr = lua_tonumber(state, 1) ? " 1" : " 0";
	else if (lua_isboolean(state, 1))
		argstr = lua_toboolean(state, 1) ? " 1" : " 0";

	char cmdbuf[64];
	q_snprintf(cmdbuf, sizeof cmdbuf, "%s%s;", command, argstr);

	Cbuf_AddText(cmdbuf);
	return 0;
}

static int LS_global_player_god(lua_State* state)
{
	return LS_PlayerCheatCommand(state, "god");
}

static int LS_global_player_notarget(lua_State* state)
{
	return LS_PlayerCheatCommand(state, "notarget");
}

static int LS_global_player_noclip(lua_State* state)
{
	return LS_PlayerCheatCommand(state, "noclip");
}


static void LS_InitStandardLibraries(lua_State* state)
{
	// Available standard libraries
	static const luaL_Reg stdlibs[] =
	{
		{ LUA_GNAME, luaopen_base },
		{ LUA_MATHLIBNAME, luaopen_math },
		{ LUA_TABLIBNAME, luaopen_table },
		{ LUA_STRLIBNAME, luaopen_string },
		{ NULL, NULL }
	};

	for (const luaL_Reg* lib = stdlibs; lib->func; ++lib)
	{
		luaL_requiref(state, lib->name, lib->func, 1);
		lua_pop(state, 1);
	}
}

static int LS_LoadFile(lua_State* state, const char* filename, const char* mode)
{
	if (filename == NULL)
	{
		lua_pushstring(state, "reading from stdin is not supported");
		return LUA_ERRFILE;
	}

	assert(mode != NULL && mode[0] == 't' && mode[1] == '\0');

	int handle;
	int length = COM_OpenFile(filename, &handle, NULL);

	if (handle == -1)
	{
		lua_pushfstring(state, "cannot open Lua script '%s'", filename);
		return LUA_ERRFILE;
	}

	char* script = tlsf_malloc(ls_memory, length);
	assert(script);

	int bytesread = Sys_FileRead(handle, script, length);
	COM_CloseFile(handle);

	int result;

	if (bytesread == length)
		result = luaL_loadbufferx(state, script, length, filename, mode);
	else
	{
		lua_pushfstring(state,
			"error while reading Lua script '%s', read %d bytes instead of %d",
			filename, bytesread, length);
		result = LUA_ERRFILE;
	}

	tlsf_free(ls_memory, script);

	return result;
}

static int LS_global_dofile(lua_State* state)
{
	const char* filename = luaL_optstring(state, 1, NULL);
	lua_settop(state, 1);

	if (LS_LoadFile(state, filename, "t") != LUA_OK)
		return lua_error(state);

	lua_call(state, 0, LUA_MULTRET);
	return lua_gettop(state) - 1;
}

static int LS_global_loadfile(lua_State* state)
{
	const char* filename = luaL_optstring(state, 1, NULL);
	int env = !lua_isnone(state, 3) ? 3 : 0;  // 'env' index or 0 if no 'env'
	int status = LS_LoadFile(state, filename, "t");

	if (status == LUA_OK)
	{
		if (env != 0)
		{
			// 'env' parameter?
			lua_pushvalue(state, env);  // environment for loaded function
			if (!lua_setupvalue(state, -2, 1))  // set it as 1st upvalue
				lua_pop(state, 1);  // remove 'env' if not used by previous call
		}

		return 1;
	}
	else
	{
		// error (message is on top of the stack)
		luaL_pushfail(state);
		lua_insert(state, -2);  // put before error message
		return 2;  // return fail plus error message
	}
}

static int LS_global_print(lua_State* state)
{
	enum PrintDummyEnum { MAX_LENGTH = 4096 };  // See MAXPRINTMSG
	char buf[MAX_LENGTH] = { '\0' };

	char* bufptr = buf;
	int remain = MAX_LENGTH;

	for (int i = 1, n = lua_gettop(state); i <= n; ++i)
	{
		const char* str = luaL_tolstring(state, i, NULL);
		int charscount = q_snprintf(bufptr, remain, "%s%s", (i == 1 ? "" : " "), str);

		bufptr += charscount;
		assert(bufptr <= &buf[MAX_LENGTH - 1]);

		remain -= charscount;
		assert(remain >= 0);

		lua_pop(state, 1);  // pop string value
	}

	Con_SafePrintf("%s\n", buf);

	return 0;
}
static int LS_global_panic(lua_State* state)
{
	const char* message = lua_tostring(state, -1);

	if (!message)
		message = "unknown error";

	Host_Error("%s", message);

	assert(false);
	return 0;
}

static void LS_global_warning(void* ud, const char *msg, int tocont)
{
	(void)ud;
	Con_SafePrintf("%s%s", msg, tocont ? "" : "\n");
}

static void LS_global_hook(lua_State* state, lua_Debug* ar)
{
	luaL_error(state, "infinite loop detected, aborting");
}

static void LS_ReportError(lua_State* state)
{
	Con_SafePrintf("Error while executing Lua script\n");

	const char* errormessage = lua_tostring(state, -1);
	if (errormessage)
		Con_SafePrintf("%s\n", errormessage);

	lua_pop(state, 1);  // remove error message
}

static void* LS_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	(void)ud;

	if (nsize == 0)
	{
		if (ptr != NULL)
			tlsf_free(ls_memory, ptr);

		return NULL;
	}
	else
		return tlsf_realloc(ls_memory, ptr, nsize);
}

static void LS_MemoryStatsCollector(void* pointer, size_t size, int isused, void* user)
{
	(void)pointer;

	LS_MemoryStats* stats = user;
	assert(stats);

	if (isused)
	{
		stats->usedbytes += size;
		++stats->usedblocks;
	}
	else
	{
		stats->freebytes += size;
		++stats->freeblocks;
	}
}

static int LS_global_printmemstats(lua_State* state)
{
	(void)state;

	LS_MemoryStats stats;
	memset(&stats, 0, sizeof stats);
	tlsf_walk_pool(tlsf_get_pool(ls_memory), LS_MemoryStatsCollector, &stats);

	size_t totalblocks = stats.usedblocks + stats.freeblocks;
	size_t totalbytes = stats.usedbytes + stats.freebytes + tlsf_size() +
		tlsf_pool_overhead() + tlsf_alloc_overhead() * (totalblocks - 1);

	Con_SafePrintf("Used : %zu bytes, %zu blocks\nFree : %zu bytes, %zu blocks\nTotal: %zu bytes, %zu blocks\n",
		stats.usedbytes, stats.usedblocks, stats.freebytes, stats.freeblocks, totalbytes, totalblocks);

	return 0;
}

static lua_CFunction ls_loadfunc;

// Calls original load() function with mode explicitly set to text
static int LS_global_load(lua_State* state)
{
	int argc = lua_gettop(state);

	switch (argc)
	{
	case 0:
		// don't care about erroneous call
		break;

	case 1:
		lua_pushnil(state);  // add second argument if it's missing
		// fall through

	default:
		lua_pushstring(state, "t");  // text mode only
		if (argc > 2)
			lua_replace(state, 3);  // replace mode if it was set
		break;
	}

	assert(ls_loadfunc != NULL);
	return ls_loadfunc(state);
}

static void LS_InitGlobalFunctions(lua_State* state)
{
	lua_atpanic(state, LS_global_panic);
	lua_setwarnf(state, LS_global_warning, NULL);
	lua_sethook(state, LS_global_hook, LUA_MASKCOUNT, 1 * 1024 * 1024);

	lua_pushglobaltable(state);

	// Save pointer to load() function
	lua_getfield(state, 1, "load");
	ls_loadfunc = lua_tocfunction(state, -1);
	lua_pop(state, 1);  // remove function

	static const luaL_Reg functions[] =
	{
		// Replaced global functions
		{ "dofile", LS_global_dofile },
		{ "load", LS_global_load },
		{ "loadfile", LS_global_loadfile },
		{ "print", LS_global_print },

		// Helper functions
		{ "printmemstats", LS_global_printmemstats },

		{ NULL, NULL }
	};

	luaL_setfuncs(state, functions, 0);
	lua_pop(state, 1);  // remove global table
}

static void LS_InitGlobalTables(lua_State* state)
{
	// Create and register 'vec3' table with helper functions for 'vec3' values
	{
		static const luaL_Reg functions[] =
		{
			{ "cross", LS_global_vec3_cross },
			{ "dot", LS_global_vec3_dot },
			{ "mid", LS_global_vec3_mid },
			{ "new", LS_global_vec3_new },
			{ NULL, NULL }
		};

		luaL_newlib(state, functions);
		lua_setglobal(state, "vec3");
	}

	// Create and register 'edicts' table
	{
		static const luaL_Reg edicts_metatable[] =
		{
			{ "__index", LS_global_edicts_index },
			{ "__len", LS_global_edicts_len },
			{ NULL, NULL }
		};

		lua_newtable(state);
		lua_pushvalue(state, -1);  // copy for lua_setmetatable()
		lua_setglobal(state, "edicts");

		lua_pushcfunction(state, LS_global_edicts_isfree);
		lua_setfield(state, -2, "isfree");

		luaL_newmetatable(state, "edicts");
		luaL_setfuncs(state, edicts_metatable, 0);
		lua_setmetatable(state, -2);

		lua_pop(state, 1);  // remove table
	}

	// Create and register 'player' table
	{
		static const luaL_Reg functions[] =
		{
			{ "god", LS_global_player_god },
			{ "noclip", LS_global_player_noclip },
			{ "notarget", LS_global_player_notarget },
			{ "setpos", LS_global_player_setpos },
			{ NULL, NULL }
		};

		luaL_newlib(state, functions);
		lua_setglobal(state, "player");
	}

	// Register namespace for console commands
	lua_createtable(state, 0, 16);
	lua_setglobal(state, ls_console_name);
}

static void LS_LoadEngineScripts(lua_State* state)
{
	static const char* scripts[] =
	{
		"scripts/edicts.lua",
		NULL
	};

	for (const char** scriptptr = scripts; *scriptptr != NULL; ++scriptptr)
	{
		lua_pushcfunction(state, LS_global_dofile);
		lua_pushstring(state, *scriptptr);

		if (lua_pcall(state, 1, 0, 0) != LUA_OK)
			LS_ReportError(state);
	}
}

static void LS_ResetState(void)
{
	if (ls_state == NULL)
		return;

	lua_close(ls_state);
	ls_state = NULL;

#ifndef NDEBUG
	// check memory pool
	LS_MemoryStats stats;
	memset(&stats, 0, sizeof stats);
	tlsf_walk_pool(tlsf_get_pool(ls_memory), LS_MemoryStatsCollector, &stats);

	assert(stats.usedbytes == 0);
	assert(stats.usedblocks == 0);
	assert(stats.freebytes + tlsf_size() + tlsf_pool_overhead() == ls_memorysize);
	assert(stats.freeblocks == 1);
#endif // !NDEBUG
}

static lua_State* LS_GetState(void)
{
	if (ls_state != NULL)
		return ls_state;

	if (ls_memory == NULL)
		ls_memory = tlsf_create_with_pool(malloc(ls_memorysize), ls_memorysize);

	lua_State* state = lua_newstate(LS_alloc, NULL);
	assert(state);

	lua_gc(state, LUA_GCSTOP);

	LS_InitStandardLibraries(state);
	LS_InitGlobalFunctions(state);
	LS_InitGlobalTables(state);
	LS_LoadEngineScripts(state);

	lua_gc(state, LUA_GCRESTART);
	lua_gc(state, LUA_GCCOLLECT);

	ls_state = state;
	return state;
}

typedef struct
{
	const char* script;
	size_t scriptlength;
	size_t partindex;
} LS_ReaderData;

static const char* LS_global_reader(lua_State* state, void* data, size_t* size)
{
	LS_ReaderData* readerdata = data;
	assert(readerdata);

	static const char prefix[] = "return ";
	static const size_t prefixsize = sizeof prefix - 1;

	static const char suffix[] = ";";
	static const size_t suffixsize = sizeof suffix - 1;

	const char* result;
	size_t resultsize;

	switch (readerdata->partindex)
	{
		case 0:
			result = prefix;
			resultsize = prefixsize;
			break;
		case 1:
			result = readerdata->script;
			resultsize = readerdata->scriptlength;
			break;
		case 2:
			result = suffix;
			resultsize = suffixsize;
			break;
		default:
			result = NULL;
			resultsize = 0;
			break;
	}

	++readerdata->partindex;
	*size = resultsize;

	return result;
}

static void LS_Exec_f(void)
{
	int argc = Cmd_Argc();

	if (argc > 1)
	{
		lua_State* state = LS_GetState();
		assert(state);
		assert(lua_gettop(state) == 0);

		const char* args = Cmd_Args();
		assert(args);

		const char* script = args;
		size_t scriptlength = strlen(args);
		qboolean removequotes = argc == 2 && scriptlength > 2 && args[0] == '"' && args[scriptlength - 1] == '"';

		if (removequotes)
		{
			// Special case of lua CCMD invocation with one argument wrapped with double quotes
			// Skip these quotes, and pass remaining sctring as script code
			scriptlength -= 2;
			script += 1;
		}

		static const char* scriptname = "script";
		static const char* scriptmode = "t";

		LS_ReaderData data = { script, scriptlength, 0 };
		int status = lua_load(state, LS_global_reader, &data, scriptname, scriptmode);

		if (status != LUA_OK)
		{
			lua_pop(state, 1);  // remove error message
			status = luaL_loadbufferx(state, script, scriptlength, scriptname, scriptmode);
		}

		if (status == LUA_OK)
			status = lua_pcall(state, 0, LUA_MULTRET, 0);

		if (status == LUA_OK)
		{
			int resultcount = lua_gettop(state);

			if (resultcount > 0)
			{
				luaL_checkstack(state, LUA_MINSTACK, "too many results to print");

				// Print all results
				lua_pushcfunction(state, LS_global_print);
				lua_insert(state, 1);
				lua_pcall(state, resultcount, 0, 0);

				lua_settop(state, 0);  // clear stack
			}
		}
		else
			LS_ReportError(state);

		assert(lua_gettop(state) == 0);
	}
	else
		Con_SafePrintf("Running %s\n", LUA_RELEASE);
}

void LS_Init(void)
{
	Cmd_AddCommand("lua", LS_Exec_f);
	Cmd_AddCommand("resetluastate", LS_ResetState);
}

void LS_Shutdown(void)
{
	LS_ResetState();

	free(ls_memory);
	ls_memory = NULL;
}

qboolean LS_ConsoleCommand(void)
{
	qboolean result = false;

	lua_State* state = LS_GetState();
	assert(state);
	assert(lua_gettop(state) == 0);

	lua_getglobal(state, ls_console_name);

	if (lua_getfield(state, -1, Cmd_Argv(0)) == LUA_TFUNCTION)
	{
		int argc = Cmd_Argc();

		if (lua_checkstack(state, argc))
		{
			for (int i = 1; i < argc; ++i)
				lua_pushstring(state, Cmd_Argv(i));

			if (lua_pcall(state, argc - 1, 0, 0) != LUA_OK)
				LS_ReportError(state);
		}
		else
			Con_SafePrintf("Too many arguments (%i) to call Lua script\n", argc - 1);

		result = true;  // command has been processed
	}
	else
		lua_pop(state, 1);  // remove nil

	lua_pop(state, 1);  // remove console namespace table
	assert(lua_gettop(state) == 0);

	return result;
}

const char *LS_GetNextCommand(const char *command)
{
	lua_State* state = LS_GetState();
	assert(state);
	assert(lua_gettop(state) == 0);

	lua_getglobal(state, ls_console_name);

	if (command == NULL)
		lua_pushnil(state);
	else
		lua_pushstring(state, command);

	const char* result = NULL;

	while (lua_next(state, -2) != 0)
	{
		if (lua_type(state, -1) == LUA_TFUNCTION && lua_type(state, -2) == LUA_TSTRING)
		{
			result = lua_tostring(state, -2);
			assert(result);

			lua_pop(state, 2);  // remove name and value
			break;
		}

		lua_pop(state, 1);  // remove value, keep name for next iteration
	}

	lua_pop(state, 1);  // remove console namespace table
	assert(lua_gettop(state) == 0);

	return result;
}

#endif // USE_LUA_SCRIPTING
