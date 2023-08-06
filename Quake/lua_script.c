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
		"dofile",
		"loadfile",
		NULL
	};

	for (const char** func = unsafefuncs; *func; ++func)
	{
		lua_pushnil(state);
		lua_setglobal(state, *func);
	}
}

static void LS_PrepareState(lua_State* state)
{
	LS_InitStandardLibraries(state);

	// Register scripting functions
	// TODO: ...
}

static qboolean LS_Verify(lua_State* state, const char* filename, int result)
{
	if (result == LUA_OK)
		return true;

	const char* error = lua_tostring(state, -1);
	assert(error);

	// Remove junk from [string "beginning of script..."]:line: message
	const char* nojunkerror = strstr(error, "...\"]:");
	Con_SafePrintf("Error while executing Lua script\n%s", filename);

	if (nojunkerror)
		Con_SafePrintf("%s\n", nojunkerror + 5);
	else
		Con_SafePrintf(":0: %s\n", error);

	return false;
}

static void LS_Exec(const char* script, const char* filename)
{
	// TODO: memory allocation via Hunk_Alloc(), see lua_Alloc
	lua_State* state = luaL_newstate();

	if (!state)
	{
		Con_SafePrintf("Failed to create lua state, out of memory?\n");
		return;
	}

	LS_PrepareState(state);

	if (LS_Verify(state, filename, luaL_loadstring(state, script)))
	{
		int argc = Cmd_Argc();

		for (int i = 2; i < argc; ++i)  // skip command and script name
			lua_pushstring(state, Cmd_Argv(i));

		LS_Verify(state, filename, lua_pcall(state, argc - 2, 0, 0));
	}

	lua_close(state);
}

static void LS_Exec_f(void)
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
		LS_Exec(script, filename);
	else
		Con_SafePrintf("Failed to load Lua script '%s'\n", filename);

	Hunk_FreeToLowMark(mark);
}

void LS_Init(void)
{
	Cmd_AddCommand("lua", LS_Exec_f);
}

#endif // USE_LUA_SCRIPTING
