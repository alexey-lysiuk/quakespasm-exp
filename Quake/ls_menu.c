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

lua_State* LS_GetState(void);

static int LS_global_openmenu(lua_State* state)
{
	m_entersound = true;
	m_state = m_luascript;

	IN_Deactivate(modestate == MS_WINDOWED);
	key_dest = key_menu;

	return 0;
}

void LS_InitMenuFunctions(lua_State* state)
{
	lua_pushglobaltable(state);

	static const luaL_Reg functions[] =
	{
		{ "openmenu", LS_global_openmenu },
		{ NULL, NULL }
	};

	luaL_setfuncs(state, functions, 0);
	lua_pop(state, 1);  // remove global table
}

void M_LuaScript_Draw (void)
{
	M_Print(16, 4, "Lua script menu");

	// TODO: lua script draw
}

void M_LuaScript_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		IN_Activate();
		key_dest = key_game;
		m_state = m_none;
		break;

		// TODO: lua script key
	}
}

#endif // USE_LUA_SCRIPTING
