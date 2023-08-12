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
static const char ls_edictindexname[] = "_luascripting_edictindex";
static const char ls_lazyevalname[] = "lazyeval";

// Creates metatable (if doesn't exist) and sets it for value on top of the stack
static void LS_SetMetaTable(lua_State* state, const char* metatablename, const luaL_Reg* functions)
{
	int type = luaL_getmetatable(state, metatablename);

	if (type == LUA_TNIL)
	{
		// Create metatable
		lua_pop(state, 1);  // remove 'nil'
		luaL_newmetatable(state, metatablename);

		// Add function(s) to metatable
		for (const luaL_Reg* entry = functions; entry->func; ++entry)
		{
			lua_pushcfunction(state, entry->func);
			lua_setfield(state, -2, entry->name);
		}
	}
	else if (type != LUA_TTABLE)
		luaL_error(state, "Broken '%s' metatable", metatablename);

	lua_setmetatable(state, -2);
}

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

	LS_SetMetaTable(state, "vec3_t", functions);
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

// Pushes complete edict table with all fields set
static qboolean LS_BuildFullEdictTable(lua_State* state, int index)
{
	edict_t* ed = EDICT_NUM(index);
	assert(ed);

	lua_createtable(state, 0, 0);

	if (ed->free)
		return true;

	for (int fi = 1; fi < progs->numfielddefs; ++fi)
	{
		etype_t type;
		const char* name;
		const eval_t* value;

		if (!ED_GetFieldByIndex(ed, fi, &name, &type, &value))
			continue;

		LS_PushFieldValue(state, name, type, value);
		lua_setfield(state, -2, name);
	}

	return true;
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
	luaL_checktype(state, 1, LUA_TTABLE);
	luaL_checktype(state, 2, LUA_TSTRING);

	// Fetch edict index, [0..num_edicts), from edict table
	lua_pushstring(state, ls_edictindexname);
	lua_rawget(state, 1);

	lua_Integer index = luaL_checkinteger(state, -1);
	lua_pop(state, 1);  // remove ls_edictindexname

	if (sv.active && index >= 0 && index < sv.num_edicts)
	{
		edict_t* ed = EDICT_NUM(index);
		assert(ed);

		if (!ed->free)
		{
			const char* name = luaL_checkstring(state, 2);
			etype_t type;
			const eval_t* value;

			if (ED_GetFieldByName(ed, name, &type, &value))
			{
				LS_PushFieldValue(state, name, type, value);

				// Add field and its value to edict table
				lua_pushvalue(state, 2);  // field name
				lua_pushvalue(state, -2);  // copy of field value for lua_rawset()
				lua_rawset(state, 1);

				return 1;
			}
		}
	}

	lua_pushnil(state);
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

	LS_SetMetaTable(state, "edict", functions);
}

// Pushes edict table by its index, [0..num_edicts)
static int LS_EdictsIndex(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	// Check edict index, [0..num_edicts), for validity
	lua_Integer index = luaL_checkinteger(state, 2);

	if (!sv.active || index < 0 || index >= sv.num_edicts)
	{
		lua_pushnil(state);
		return 1;
	}

	// Fetch edict lazy evaluation status
	lua_pushstring(state, ls_lazyevalname);
	lua_rawget(state, 1);

	qboolean lazyeval = lua_toboolean(state, -1);

	// Create edict table
	lua_createtable(state, 0, 16);

	if (lazyeval)
	{
		LS_SetEdictMetaTable(state);

		// Set own edict index, [0..num_edicts), to edict table
		lua_pushnumber(state, index);
		lua_setfield(state, -2, ls_edictindexname);
	}
	else
		LS_BuildFullEdictTable(state, index);

	// Add this edict to 'edicts' global table
	lua_pushvalue(state, -1);
	lua_rawseti(state, 1, index);

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

	// Create and register 'edicts' global table
	lua_createtable(state, sv.active ? sv.num_edicts : 0, 1);
	lua_pushboolean(state, true);
	lua_setfield(state, -2, ls_lazyevalname);
	lua_pushvalue(state, -1);
	lua_setglobal(state, "edicts");

	// Create and set metatable for 'edicts' global table
	lua_createtable(state, 0, 3);
	lua_pushcfunction(state, LS_EdictsIndex);
	lua_setfield(state, -2, "__index");
	lua_pushcfunction(state, LS_EdictsCount);
	lua_setfield(state, -2, "__len");
	lua_setmetatable(state, -2);
}

static void LS_Exec_f(void)
{
	int mark = Hunk_LowMark();
	int argc = Cmd_Argc();

	if (argc > 1)
	{
		// TODO: memory allocation via Hunk_Alloc(), see lua_Alloc
		lua_State* state = luaL_newstate();

		if (!state)
		{
			Con_SafePrintf("Failed to create lua state, out of memory?\n");
			return;
		}

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
