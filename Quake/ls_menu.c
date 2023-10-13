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


lua_State* LS_GetState(void);

static const char* ls_menu_name = "menu";
static const char* ls_menu_pagestack_name = "pagestack";


// // Pushes menu table if it exists and returns state, otherwise returns null
//static lua_State* LS_PushMenuTable(void)
//{
//	lua_State* state = LS_GetState();
//	assert(state);
//	assert(lua_gettop(state) == 0);
//
//	lua_getglobal(state, ls_menu_name);
//
//	if (lua_istable(state, 1))
//		return state;
//
//	lua_pop(state, 1);  // remove nil
//	return NULL;
//}

static void LS_PopMenuTable(lua_State* state)
{
	lua_pop(state, 1);  // remove console namespace table
	assert(lua_gettop(state) == 0);
}

static qboolean LS_PushMenuStackTable(lua_State* state)
{
	//lua_State* state = LS_PushMenuTable();

	if (lua_getglobal(state, ls_menu_name) != LUA_TTABLE)
	{
		lua_pop(state, 1);  // remove incorrect value
		return false;
	}

//	if (state == NULL)
//		return false;

	if (lua_getfield(state, -1, ls_menu_pagestack_name) != LUA_TTABLE)
	{
		lua_pop(state, 1);  // remove incorrect value
		return false;
	}

	lua_remove(state, -2);  // remove menu table
	return true;
}

static void LS_OpenMenu(lua_State* state)
{
	m_entersound = true;
	m_state = m_luascript;

	IN_Deactivate(modestate == MS_WINDOWED);
	key_dest = key_menu;
}

static void LS_CloseMenu(lua_State* state)
{
	if (m_state != m_luascript)
		return;

	IN_Activate();
	key_dest = key_game;
	m_state = m_none;
}

static int LS_global_menu_pushpage(lua_State* state)
{
	if (LS_PushMenuStackTable(state))
	{
		lua_len(state, -1);

		int menustacktop = lua_tointeger(state, -1);
		assert(menustacktop >= 0);
		lua_pop(state, 1);  // remove page stack size

		lua_pushvalue(state, 1);
		lua_seti(state, -2, menustacktop + 1);
	}

	if (m_state != m_luascript)
		LS_OpenMenu(state);

	return 0;
}

static int LS_global_menu_poppage(lua_State* state)
{
//	m_entersound = true;
//	m_state = m_luascript;
//
//	IN_Deactivate(modestate == MS_WINDOWED);
//	key_dest = key_menu;

	return 0;
}

void LS_InitMenuSystem(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "pushpage", LS_global_menu_pushpage },
		{ "poppage", LS_global_menu_poppage },
		{ NULL, NULL }
	};

	luaL_newlib(state, functions);
	lua_newtable(state);
	lua_setfield(state, -2, ls_menu_pagestack_name);
	lua_setglobal(state, ls_menu_name);
}

void LS_ShutdownMenuSystem(lua_State* state)
{
	LS_CloseMenu(state);
}


void M_LuaScript_Draw(void)
{
	lua_State* state = LS_GetState();

	if (!LS_PushMenuStackTable(state))
	{
		LS_CloseMenu(LS_GetState());
		return;
	}

	lua_len(state, -1);

	int menustacktop = lua_tointeger(state, -1);
	assert(menustacktop >= 0);
	lua_pop(state, 1);  // remove menu stack size

	if (menustacktop == 0)
	{
		LS_CloseMenu(LS_GetState());
		return;
	}

	if (lua_geti(state, -1, menustacktop) != LUA_TTABLE)
	{
		LS_CloseMenu(LS_GetState());
		return;
	}

	if (lua_getfield(state, -1, "drawfunc") != LUA_TFUNCTION)
	{
		LS_CloseMenu(LS_GetState());
		return;
	}

	lua_insert(state, 2);  // swap top menu with draw function

	if (lua_pcall(state, 1, 0, 0) != LUA_OK)
	{
		// TODO: report error
		//LS_ReportError(state);
		LS_CloseMenu(LS_GetState());
		return;
	}

	LS_PopMenuTable(state);
}

void M_LuaScript_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		LS_CloseMenu(LS_GetState());
		break;

		// TODO: lua script key
	}
}

#endif // USE_LUA_SCRIPTING
