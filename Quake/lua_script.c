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


static const char* LUA_axisnames[] = { "x", "y", "z" };

static int LUA_Vec3String(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	vec3_t value;

	for (int i = 0; i < 3; ++i)
	{
		int type = lua_getfield(state, 1, LUA_axisnames[i]);
		if (type != LUA_TNUMBER)
			luaL_error(state, "Bad value in vec3_t at index %i", i);

		value[i] = lua_tonumber(state, 2);
		lua_pop(state, 1);
	}

	lua_pushfstring(state, "'%f %f %f'", value[0], value[1], value[2]);
	return 1;
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
			break;

		case ev_float:
			lua_pushnumber(state, value->_float);
			break;

		case ev_vector:
			lua_createtable(state, 0, 3);
			int vtindex = lua_gettop(state);

			for (int i = 0; i < 3; ++i)
			{
				lua_pushnumber(state, value->vector[i]);
				lua_setfield(state, -2, LUA_axisnames[i]);
			}

			lua_createtable(state, 0, 1);
			lua_pushcfunction(state, LUA_Vec3String);
			lua_setfield(state, -2, "__tostring");
			lua_setmetatable(state, vtindex);
			// TODO: __concat
			break;

		case ev_entity:
			lua_pushfstring(state, "entity %i", NUM_FOR_EDICT(PROG_TO_EDICT(value->edict)));
			break;

		case ev_field:
			lua_pushfstring(state, ".%s", "_TODO_");
			break;

		case ev_function:
			func = pr_functions + value->function;
			lua_pushfstring(state, "%s()", PR_GetString(func->s_name));
			break;

		case ev_pointer:
			lua_pushstring(state, "pointer");
			break;

		default:
			// Unknown type, e.g. some of FTE extensions
			lua_pushfstring(state, "bad type %i", type);
			break;
		}

		lua_setfield(state, -2, name);
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
		{ LUA_GNAME, luaopen_base },
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
	typedef struct
	{
		const char* name;
		lua_CFunction ptr;
	} ScriptFunction;

	static const ScriptFunction scriptfuncs[] =
	{
		{ "edict", LUA_Edict },
		{ "edicts", LUA_Edicts },
		{ NULL, NULL }
	};

	for (const ScriptFunction* func = scriptfuncs; func->name; ++func)
	{
		lua_pushcfunction(state, func->ptr);
		lua_setglobal(state, func->name);
	}

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

	if (!state)
	{
		Con_SafePrintf("Failed to create lua state, out of memory?\n");
		return;
	}

	LUA_PrepareState(state);

	int result = luaL_dostring(state, script);
	int top = lua_gettop(state);

	if (result != LUA_OK)
	{
		const char* error = lua_tostring(state, top);
		assert(error);

		// Remove junk from [string "beginning of script..."]:line: message
		const char* nojunkerror = strstr(error, "...\"]:");
		Con_SafePrintf("Error while executing Lua script\n%s", filename);

		if (nojunkerror)
			Con_SafePrintf("%s\n", nojunkerror + 5);
		else
			Con_SafePrintf(":0: %s\n", error);
	}

	lua_pop(state, top);
	lua_close(state);
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
