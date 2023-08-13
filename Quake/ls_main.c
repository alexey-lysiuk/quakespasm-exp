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

static const char* ls_axisnames[] = { "x", "y", "z" };

// Pushes string built from vec3_t value, i.e. from a table with 'x', 'y', 'z' fields
static int LS_Vec3ToString(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	vec3_t value;

	for (int i = 0; i < 3; ++i)
	{
		int type = lua_getfield(state, 1, ls_axisnames[i]);
		if (type != LUA_TNUMBER)
			luaL_error(state, "Bad value in vec3_t at index %d", i);

		value[i] = lua_tonumber(state, -1);
		lua_pop(state, 1);  // remove value
	}

	char buf[128];
	int length = q_snprintf(buf, sizeof buf, "%.1f %.1f %.1f", value[0], value[1], value[2]);

	lua_pushlstring(state, buf, length);
	return 1;
}

// Sets metatable for vec3_t table
static void LS_SetVec3MetaTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "__tostring", LS_Vec3ToString },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "vec3_t"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
}

// Pushes field value by its type and name
static void LS_PushFieldValue(lua_State* state, const char* name, etype_t type, const eval_t* value)
{
	assert(type != ev_bad);
	assert(name);
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
		lua_createtable(state, 0, 3);
		LS_SetVec3MetaTable(state);

		for (int i = 0; i < 3; ++i)
		{
			lua_pushnumber(state, value->vector[i]);
			lua_setfield(state, -2, ls_axisnames[i]);
		}
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

// Pushes number of edicts
static int LS_EdictsCount(lua_State* state)
{
	lua_pushinteger(state, sv.active ? sv.num_edicts : 0);
	return 1;
}

// Pushes value of edict field by its name
static int LS_EdictIndex(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TUSERDATA);
	luaL_checktype(state, 2, LUA_TSTRING);

	edict_t** edptr = lua_touserdata(state, 1);
	assert(edptr);

	edict_t* ed = *edptr;
	assert(ed);

	if (ed->free)
		lua_pushnil(state);  // TODO: default value instead of nil
	else
	{
		const char* name = luaL_checkstring(state, 2);
		etype_t type;
		const eval_t* value;

		if (ED_GetFieldByName(ed, name, &type, &value))
			LS_PushFieldValue(state, name, type, value);
		else
			lua_pushnil(state);  // TODO: default value instead of nil
	}

	return 1;
}

// Sets metatable for edict table
static void LS_SetEdictMetaTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_EdictIndex },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "edict"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
}

static int LS_ForEachEdict(lua_State* state)
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
		lua_geti(state, 1, i);  // get edict by index, [0..num_edicts)
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
// * edict userdata by its integer index, [0..num_edicts)
// * method of 'edicts' userdata by its name
static int LS_EdictsIndex(lua_State* state)
{
	int indextype = lua_type(state, 2);

	if (indextype == LUA_TSTRING)
	{
		const char* key = lua_tostring(state, 2);
		assert(key);

		if (strcmp(key, "foreach") == 0)
			lua_pushcfunction(state, LS_ForEachEdict);
		else
			luaL_error(state, "Unknown edicts key '%s'", key);
	}
	else if (indextype == LUA_TNUMBER)
	{
		// Check edict index, [0..num_edicts), for validity
		lua_Integer index = lua_tointeger(state, 2);

		if (sv.active && index >= 0 && index < sv.num_edicts)
		{
			// Create edict userdata, and assign edict_t* to it
			edict_t** edictptr = lua_newuserdatauv(state, sizeof(edict_t*), 0);
			assert(edictptr);
			*edictptr = EDICT_NUM(index);
			LS_SetEdictMetaTable(state);
		}
		else
			luaL_error(state, "Edicts index %i is out of range [0..%i)", index, sv.num_edicts);
	}
	else
		luaL_error(state, "Invalid type %i of edicts key", indextype);

	return 1;
}

static int LS_EdictsNewIndex(lua_State* state)
{
	// TODO: cache key to value mapping (?)
	return 0;
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

	// Remove "unsafe" functions from standard libraries
	static const char* unsafefuncs[] =
	{
		"getmetatable",
		"setmetatable",
		NULL
	};

	for (const char** func = unsafefuncs; *func; ++func)
	{
		lua_pushnil(state);
		lua_setglobal(state, *func);
	}
}

static int LS_LoadFileX(lua_State* state, const char* filename, const char* mode)
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

static int LS_DoFile(lua_State* state)
{
	const char* filename = luaL_optstring(state, 1, NULL);
	lua_settop(state, 1);

	if (LS_LoadFileX(state, filename, NULL) != LUA_OK)
		return lua_error(state);

	lua_call(state, 0, LUA_MULTRET);
	return lua_gettop(state) - 1;
}

static int LS_LoadFile(lua_State* state)
{
	const char* filename = luaL_optstring(state, 1, NULL);
	const char* mode = luaL_optstring(state, 2, NULL);
	int env = !lua_isnone(state, 3) ? 3 : 0;  // 'env' index or 0 if no 'env'
	int status = LS_LoadFileX(state, filename, mode);

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

static int LS_Print(lua_State* state)
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

static void LS_PrepareState(lua_State* state)
{
	LS_InitStandardLibraries(state);

	// Replace global functions
	lua_pushcfunction(state, LS_DoFile);
	lua_setglobal(state, "dofile");
	lua_pushcfunction(state, LS_LoadFile);
	lua_setglobal(state, "loadfile");
	lua_pushcfunction(state, LS_Print);
	lua_setglobal(state, "print");

	static const char* edictsname = "edicts";

	// Create and register 'edicts' global userdata
	lua_newuserdatauv(state, 0, 0);
	lua_pushvalue(state, -1);  // copy for lua_setmetatable()
	lua_setglobal(state, edictsname);

	// Create and set metatable for 'edicts' global userdata
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_EdictsIndex },
		{ "__len", LS_EdictsCount },
		{ "__newindex", LS_EdictsNewIndex },
		{ NULL, NULL }
	};

	luaL_newmetatable(state, edictsname);
	luaL_setfuncs(state, functions, 0);
	lua_setmetatable(state, -2);
}

static void* LS_Alloc(void* userdata, void* ptr, size_t oldsize, size_t newsize)
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
	int mark = Hunk_LowMark();
	int argc = Cmd_Argc();

	if (argc > 1)
	{
		lua_State* state = lua_newstate(LS_Alloc, NULL);
		assert(state);

		LS_PrepareState(state);

		int status = luaL_loadstring(state, Cmd_Args());

		if (status == LUA_OK)
			status = lua_pcall(state, 0, 0, 0);

		if (status != LUA_OK)
			Con_SafePrintf("Error while executing Lua script\n%s\n", lua_tostring(state, -1));

		lua_close(state);
	}
	else
		Con_SafePrintf("Running %s\n", LUA_RELEASE);

	Hunk_FreeToLowMark(mark);
}

void LS_Init(void)
{
	Cmd_AddCommand("lua", LS_Exec_f);
}

#endif // USE_LUA_SCRIPTING
