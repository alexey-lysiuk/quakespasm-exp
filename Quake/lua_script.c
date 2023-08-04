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

static int LUA_Echo(lua_State* state)
{
	const char* str = luaL_checkstring(state, 1);
	if (str)
		Con_SafePrintf("%s\n", str);
	return 0;
}

static qboolean LUA_MakeEdictTable(lua_State* state, int index)
{
	if (!sv.active || index < 0 || index >= sv.max_edicts)
	{
		lua_pushnil(state);
		return false;
	}

	edict_t* ed = EDICT_NUM(index);
	assert(ed);

	lua_createtable(state, 0, 0);

	for (int fi = 1; fi < progs->numfielddefs; ++fi)
	{
		etype_t type;
		const char* name;
		const eval_t* value;

		extern qboolean ED_GetFieldAt(edict_t* ed, size_t fieldindex, etype_t* type, const char** name, const eval_t** value);
		if (!ED_GetFieldAt(ed, fi, &type, &name, &value))
			continue;

		assert(type != ev_bad);
		assert(name);
		assert(value);

		dfunction_t* func;

		switch (type)
		{
		case ev_string:
			lua_pushstring(state, PR_GetString(value->string));
			lua_setfield(state, -2, name);
			break;

		case ev_function:
			func = pr_functions + value->function;
			lua_pushfstring(state, "%s()", PR_GetString(func->s_name));
			lua_setfield(state, -2, name);
			break;

		// TODO: other types

		default:
			break;
		}
	}

	return true;
}

static int LUA_Edict(lua_State* state)
{
	lua_Integer i = luaL_checkinteger(state, 1);
	LUA_MakeEdictTable(state, i);
	return 1;
}

static int LUA_GetNextEdict(lua_State* state)
{
	lua_Integer i = luaL_checkinteger(state, 2);
	i = luaL_intop(+, i, 1);
	lua_pushinteger(state, i);
	return LUA_MakeEdictTable(state, i) ? 2 : 1;
}

static int LUA_Edicts(lua_State* state)
{
	lua_pushcfunction(state, LUA_GetNextEdict);
	lua_createtable(state, 0, 4);
	lua_pushinteger(state, -1);
	return 3;
}

static void LUA_PrepareState(lua_State* state)
{
	// Available standard libraries
	static const luaL_Reg stdlibs[] =
	{
		{LUA_GNAME, luaopen_base},
		{LUA_STRLIBNAME, luaopen_string},
		{NULL, NULL}
	};

	for (const luaL_Reg* lib = stdlibs; lib->func; ++lib)
	{
		luaL_requiref(state, lib->name, lib->func, 1);
		lua_pop(state, 1);
	}

	// Remove "unsafe" functions from standard libraries
	static const char* unsafefuncs[] =
	{
		"dofile",
		"loadfile",
		NULL
	};

	for (const char** func = unsafefuncs; *func; ++func)
	{
		lua_pushnil(state);
		lua_setglobal(state, *func);
	}

	// Scripting functions
	lua_pushcfunction(state, LUA_Echo);
	lua_setglobal(state, "echo");

	lua_pushcfunction(state, LUA_Edict);
	lua_setglobal(state, "edict");

	lua_pushcfunction(state, LUA_Edicts);
	lua_setglobal(state, "edicts");

	// Script arguments
	int argc = Cmd_Argc() - 1;
	lua_createtable(state, argc, 0);

	for (int i = 0; i < argc; ++i)
	{
		lua_pushstring(state, Cmd_Argv(i + 1));
		lua_rawseti(state, -2, i);
	}

	lua_setglobal(state, "args");
}

static void LUA_Exec(const char* script, const char* filename)
{
	lua_State* state = luaL_newstate();

	if (state)
	{
		LUA_PrepareState(state);

		int result = luaL_dostring(state, script);
		int top = lua_gettop(state);

		if (result != LUA_OK)
		{
			const char* error = lua_tostring(state, top);
			Con_SafePrintf("Error while executing lua script '%s':\n%s\n", filename, error);
		}

		lua_pop(state, top);
		lua_close(state);
	}
	else
		Con_SafePrintf("Failed to create lua state, out of memory?\n");
}

static void LUA_Exec_f(void)
{
	if (Cmd_Argc() < 2)
	{
		Con_Printf("lua <filename>\n");
		return;
	}

	const char* filename = Cmd_Argv(1);
	int mark = Hunk_LowMark();
	const char* script = (const char *)COM_LoadHunkFile(filename, NULL);

	if (script)
		LUA_Exec(script, filename);
	else
		Con_SafePrintf("Failed to load lua script '%s'\n", filename);

	Hunk_FreeToLowMark(mark);
}

void LUA_Init(void)
{
	Cmd_AddCommand("lua", LUA_Exec_f);
}

#endif // USE_LUA_SCRIPTING
