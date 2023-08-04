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
#include "lauxlib.h"

#include "quakedef.h"

static void LUA_Exec(void)
{
	if (!sv.active)
		return;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("lua <filename>\n");
		return;
	}

	const char* filename = Cmd_Argv(1);
	int mark = Hunk_LowMark();
	const char* script = (const char *)COM_LoadHunkFile(filename, NULL);

	if (script)
	{
		lua_State* state = luaL_newstate();

		if (state)
		{
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
	else
		Con_SafePrintf("Failed to load lua script '%s'\n", filename);

	Hunk_FreeToLowMark(mark);
}

void LUA_Init(void)
{
	Cmd_AddCommand("lua", LUA_Exec);
}

#endif // USE_LUA_SCRIPTING
