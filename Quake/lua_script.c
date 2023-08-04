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

static int LUA_GetNextEdict(lua_State* state)
{
	lua_Integer i = luaL_checkinteger(state, 2);
	i = luaL_intop(+, i, 1);
	lua_pushinteger(state, i);

	if (!sv.active || i >= sv.num_edicts)
	{
		lua_pushnil(state);
		return 1;
	}

	edict_t* ed = EDICT_NUM(i);

	lua_createtable(state, 0, 0);

	for (int fi = 1; fi < progs->numfielddefs; ++fi)
	{
		etype_t type;
		const char* name;
		const eval_t* value;

		extern qboolean ED_GetFieldAt(edict_t* ed, size_t fieldindex, etype_t* type, const char** name, const eval_t** value);
		if (!ED_GetFieldAt(ed, fi, &type, &name, &value))
			continue;

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

	return 2;
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
	// String library
	luaL_requiref(state, LUA_STRLIBNAME, luaopen_string, 1);
	lua_pop(state, 1);

	// Exposed functions
	lua_pushcfunction(state, LUA_Echo);
	lua_setglobal(state, "echo");

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
