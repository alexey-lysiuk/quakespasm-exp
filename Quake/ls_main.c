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


qboolean ED_GetFieldByIndex(edict_t* ed, size_t fieldindex, const char** name, etype_t* type, const eval_t** value);
qboolean ED_GetFieldByName(edict_t* ed, const char* name, etype_t* type, const eval_t** value);
const char* ED_GetFieldNameByOffset(int offset);


//
// Expose vec3_t as 'vec3' userdata
//

// Converts 'vec3' component at given stack index to vec3_t integer index [0..2]
// On Lua side, valid numeric component indices are 1, 2, 3
static int LS_Vec3GetComponent(lua_State* state, int index)
{
	int comptype = lua_type(state, index);
	int component;

	if (comptype == LUA_TSTRING)
	{
		const char* compstr = lua_tostring(state, 2);
		assert(compstr);

		if (strcmp(compstr, "x") == 0)
			component = 0;
		else if (strcmp(compstr, "y") == 0)
			component = 1;
		else if (strcmp(compstr, "z") == 0)
			component = 2;
		else
			luaL_error(state, "Invalid vec3_t component '%s'", compstr);
	}
	else if (comptype == LUA_TNUMBER)
		component = lua_tointeger(state, 2) - 1;  // on C side, indices start with 0
	else
		luaL_error(state, "Invalid type %d of vec3_t component", comptype);

	if (component < 0 || component >= 3)
		luaL_error(state, "vec3_t component %d is out of range [1..3]", component + 1);  // on Lua side, indices start with 1

	return component;
}

// Get value of 'vec3' from userdata at given index
static vec_t* LS_Vec3GetValue(lua_State* state, int index)
{
	luaL_checktype(state, index, LUA_TUSERDATA);

	vec3_t* value = lua_touserdata(state, index);
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
	vec3_t* valueptr = lua_newuserdatauv(state, sizeof(edict_t*), 0);
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

// Pushes helper function by its name
static int LS_global_vec3_index(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TUSERDATA);
	luaL_checkstring(state, 2);

	const char* key = lua_tostring(state, 2);
	assert(key);

	if (strcmp(key, "new") == 0)
		lua_pushcfunction(state, LS_global_vec3_new);
	else if (strcmp(key, "mid") == 0)
		lua_pushcfunction(state, LS_global_vec3_mid);
	else if (strcmp(key, "cross") == 0)
		lua_pushcfunction(state, LS_global_vec3_cross);
	else if (strcmp(key, "dot") == 0)
		lua_pushcfunction(state, LS_global_vec3_dot);
	else
		luaL_error(state, "Unknown vec3 function '%s'", key);

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
		lua_pushfstring(state, "entity %d", NUM_FOR_EDICT(PROG_TO_EDICT(value->edict)));
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
	luaL_checktype(state, 1, LUA_TUSERDATA);

	edict_t** edptr = lua_touserdata(state, 1);
	assert(edptr);

	return *edptr;
}

// Pushes value of edict field by its name
static int LS_value_edict_index(lua_State* state)
{
	edict_t* ed = LS_GetEdictFromUserData(state);
	assert(ed);

	if (ed->free)
		lua_pushnil(state);  // TODO: default value instead of nil
	else
	{
		const char* name = luaL_checkstring(state, 2);
		etype_t type;
		const eval_t* value;

		if (ED_GetFieldByName(ed, name, &type, &value))
			LS_PushEdictFieldValue(state, type, value);
		else
			lua_pushnil(state);  // TODO: default value instead of nil
	}

	return 1;
}

// Pushes string representation of given edict
static int LS_value_edict_tostring(lua_State* state)
{
	edict_t* ed = LS_GetEdictFromUserData(state);
	assert(ed);

	int index = NUM_FOR_EDICT(ed) + 1;  // on Lua side, indices start with 1
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

	lua_pushfstring(state, "edict %d: %s", index, description);
	return 1;
}

// Sets metatable for edict table
static void LS_SetEdictMetaTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_value_edict_index },
		{ "__tostring", LS_value_edict_tostring },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "edict"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
}


//
// Expose sv.edicts as 'edicts' global userdata
//

static int LS_global_edicts_foreach(lua_State* state)
{
	if (!sv.active)
		return 0;

	luaL_checktype(state, 1, LUA_TUSERDATA);
	luaL_checktype(state, 2, LUA_TFUNCTION);

	lua_Integer target = luaL_optinteger(state, 3, 0);
	lua_Integer current = 1;

	for (int i = 0; i < sv.num_edicts; ++i)
	{
		edict_t* ed = EDICT_NUM(i);

		if (ed->free)
			continue;

		lua_pushvalue(state, 2);  // iteration function to call
		lua_geti(state, 1, i + 1);  // get edict by index, [1..num_edicts]
		lua_pushinteger(state, current);
		lua_pushinteger(state, target);
		lua_call(state, 3, 1);

		int restype = lua_type(state, -1);

		if (restype == LUA_TNIL)
			break;
		else if (restype == LUA_TNUMBER)
		{
			current = lua_tointeger(state, -1);
			lua_pop(state, 1);  // remove result
		}
		else
			luaL_error(state, "Invalid type returned from edicts.foreach() iteration function");
	}

	return 0;
}

// Pushes either
// * edict userdata by its integer index, [1..num_edicts]
// * method of 'edicts' userdata by its name
static int LS_global_edicts_index(lua_State* state)
{
	if (!sv.active)
		luaL_error(state, "Game is not running");

	int indextype = lua_type(state, 2);

	if (indextype == LUA_TSTRING)
	{
		const char* key = lua_tostring(state, 2);
		assert(key);

		if (strcmp(key, "foreach") == 0)
			lua_pushcfunction(state, LS_global_edicts_foreach);
		else
			luaL_error(state, "Unknown edicts key '%s'", key);
	}
	else if (indextype == LUA_TNUMBER)
	{
		// Check edict index, [1..num_edicts], for validity
		lua_Integer index = lua_tointeger(state, 2);

		if (index > 0 && index <= sv.num_edicts)
		{
			// Create edict userdata, and assign edict_t* to it
			edict_t** edictptr = lua_newuserdatauv(state, sizeof(edict_t*), 0);
			assert(edictptr);
			*edictptr = EDICT_NUM(index - 1);
			LS_SetEdictMetaTable(state);
		}
		else
			lua_pushnil(state);
	}
	else
		luaL_error(state, "Invalid type %d of edicts key", indextype);

	return 1;
}

// Pushes number of edicts
static int LS_global_edicts_len(lua_State* state)
{
	lua_pushinteger(state, sv.active ? sv.num_edicts : 0);
	return 1;
}


//
// Expose 'player' userdata with corresponding helper functions
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

// Pushes player helper function by its name
static int LS_global_player_index(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TUSERDATA);
	luaL_checkstring(state, 2);

	const char* key = lua_tostring(state, 2);
	assert(key);

	if (strcmp(key, "setpos") == 0)
		lua_pushcfunction(state, LS_global_player_setpos);
	else if (strcmp(key, "god") == 0)
		lua_pushcfunction(state, LS_global_player_god);
	else if (strcmp(key, "notarget") == 0)
		lua_pushcfunction(state, LS_global_player_notarget);
	else if (strcmp(key, "noclip") == 0)
		lua_pushcfunction(state, LS_global_player_noclip);
	else
		luaL_error(state, "Unknown player function '%s'", key);

	return 1;
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

	int handle;
	int length = COM_OpenFile(filename, &handle, NULL);

	if (handle == -1)
	{
		lua_pushfstring(state, "cannot open Lua script '%s'", filename);
		return LUA_ERRFILE;
	}

	char* script = (char*)Hunk_AllocName(length, filename);
	assert(script);

	int bytesread = Sys_FileRead(handle, script, length);
	COM_CloseFile(handle);

	if (bytesread != length)
	{
		lua_pushfstring(state,
			"error while reading Lua script '%s', read %d bytes instead of %d",
			filename, bytesread, length);
		return LUA_ERRFILE;
	}

	return luaL_loadbufferx(state, script, length, filename, mode);
}

static int LS_global_dofile(lua_State* state)
{
	const char* filename = luaL_optstring(state, 1, NULL);
	lua_settop(state, 1);

	if (LS_LoadFile(state, filename, NULL) != LUA_OK)
		return lua_error(state);

	lua_call(state, 0, LUA_MULTRET);
	return lua_gettop(state) - 1;
}

static int LS_global_loadfile(lua_State* state)
{
	const char* filename = luaL_optstring(state, 1, NULL);
	const char* mode = luaL_optstring(state, 2, NULL);
	int env = !lua_isnone(state, 3) ? 3 : 0;  // 'env' index or 0 if no 'env'
	int status = LS_LoadFile(state, filename, mode);

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

static int ls_hunk_lowmark;

static void LS_FreeMemory(void)
{
	assert(ls_hunk_lowmark > 0);
	Hunk_FreeToLowMark(ls_hunk_lowmark);
	ls_hunk_lowmark = 0;
}

static int LS_global_panic(lua_State* state)
{
	const char* message = lua_tostring(state, -1);

	if (!message)
		message = "unknown error";

	LS_FreeMemory();

	Host_Error("%s", message);

	assert(false);
	return 0;
}

static void LS_global_warning(void* ud, const char *msg, int tocont)
{
	(void)ud;
	Con_SafePrintf("%s%s", msg, tocont ? "" : "\n");
}

static void LS_CreateGlobalUserData(lua_State* state, const char* name, const luaL_Reg* metatable)
{
	lua_newuserdatauv(state, 0, 0);
	lua_pushvalue(state, -1);  // copy for lua_setmetatable()
	lua_setglobal(state, name);

	char metatablename[64];
	q_snprintf(metatablename, sizeof(metatablename), "global_%s", name);

	luaL_newmetatable(state, metatablename);
	luaL_setfuncs(state, metatable, 0);
	lua_setmetatable(state, -2);

	lua_pop(state, 1);  // remove userdata
}

static void LS_PrepareState(lua_State* state)
{
	LS_InitStandardLibraries(state);

	lua_atpanic(state, LS_global_panic);
	lua_setwarnf(state, LS_global_warning, NULL);

	// Replace global functions
	lua_pushcfunction(state, LS_global_dofile);
	lua_setglobal(state, "dofile");
	lua_pushcfunction(state, LS_global_loadfile);
	lua_setglobal(state, "loadfile");
	lua_pushcfunction(state, LS_global_print);
	lua_setglobal(state, "print");

	// Create and register 'vec3' global userdata with helper functions for 'vec3' values
	{
		static const luaL_Reg vec3_metatable[] =
		{
			{ "__index", LS_global_vec3_index },
			{ NULL, NULL }
		};
		LS_CreateGlobalUserData(state, "vec3", vec3_metatable);
	}

	// Create and register 'edicts' global userdata
	{
		static const luaL_Reg edicts_metatable[] =
		{
			{ "__index", LS_global_edicts_index },
			{ "__len", LS_global_edicts_len },
			{ NULL, NULL }
		};
		LS_CreateGlobalUserData(state, "edicts", edicts_metatable);
	}

	// Create and register 'player' global userdata
	{
		static const luaL_Reg player_metatable[] =
		{
			{ "__index", LS_global_player_index },
			{ NULL, NULL }
		};
		LS_CreateGlobalUserData(state, "player", player_metatable);
	}
}

static void* LS_global_alloc(void* userdata, void* ptr, size_t oldsize, size_t newsize)
{
	(void)userdata;

	if (newsize == 0)
	{
		// Free memory, this means "do nothing" for hunk memory
		return NULL;
	}
	else if (ptr == NULL)
	{
		// Allocate memory
		void* newptr = Hunk_Alloc(newsize);
		assert(newptr);
		return newptr;
	}
	else if (oldsize < newsize)
	{
		// Reallocate memory, do it only when new size is bigger than old
		void* newptr = Hunk_Alloc(newsize);
		assert(newptr);
		memcpy(newptr, ptr, oldsize);
		return newptr;
	}

	return ptr;
}

static void LS_Exec_f(void)
{
	ls_hunk_lowmark = Hunk_LowMark();

	int argc = Cmd_Argc();

	if (argc > 1)
	{
		lua_State* state = lua_newstate(LS_global_alloc, NULL);
		assert(state);

		LS_PrepareState(state);

		const char* args = Cmd_Args();
		assert(args);

		const char* script;
		size_t scriptlength = strlen(args);
		qboolean removequotes = argc == 2 && scriptlength > 2 && args[0] == '"' && args[scriptlength - 1] == '"';

		if (removequotes)
		{
			// Special case of lua CCMD invocation with one argument wrapped with double quotes
			// Remove these quotes, and pass remaining sctring as script code
			scriptlength -= 2;
			char* scriptcopy = Hunk_Alloc(scriptlength + 1);
			strncpy(scriptcopy, args + 1, scriptlength);
			scriptcopy[scriptlength] = '\0';
			script = scriptcopy;
		}
		else
			script = args;

		int status = luaL_loadbuffer(state, script, scriptlength, "script");

		if (status == LUA_OK)
			status = lua_pcall(state, 0, 0, 0);

		if (status != LUA_OK)
		{
			Con_SafePrintf("Error while executing Lua script\n");

			const char* errormessage = lua_tostring(state, -1);
			if (errormessage)
				Con_SafePrintf("%s\n", errormessage);
		}

		lua_close(state);
	}
	else
		Con_SafePrintf("Running %s\n", LUA_RELEASE);

	LS_FreeMemory();
}

void LS_Init(void)
{
	Cmd_AddCommand("lua", LS_Exec_f);
}

#endif // USE_LUA_SCRIPTING
