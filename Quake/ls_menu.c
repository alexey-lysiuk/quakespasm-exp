/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifdef USE_LUA_SCRIPTING

#include <assert.h>

#include "ls_common.h"
#include "quakedef.h"


static const char* ls_menu_name = "menu";
static const char* ls_menu_pagestack_name = "pagestack";


static qboolean LS_PushMenuPageStackTable(lua_State* state)
{
	assert(state != NULL);

	if (lua_getglobal(state, ls_menu_name) != LUA_TTABLE)
	{
		lua_pop(state, 1);  // remove incorrect value for menu table
		return false;
	}

	if (lua_getfield(state, -1, ls_menu_pagestack_name) != LUA_TTABLE)
	{
		lua_pop(state, 2);  // remove menu table and incorrect page stack table value
		return false;
	}

	lua_remove(state, -2);  // remove menu table
	return true;
}

static qboolean LS_PushTopMenuPage(lua_State* state)
{
	assert(state != NULL);

	if (!LS_PushMenuPageStackTable(state))
		return false;

	lua_len(state, -1);

	int menustacktop = lua_tointeger(state, -1);
	assert(menustacktop >= 0);
	lua_pop(state, 1);  // remove page stack size

	if (menustacktop == 0)
	{
		// Page stack is empty, remove it and return
		lua_pop(state, 1);
		return false;
	}

	if (lua_geti(state, -1, menustacktop) != LUA_TTABLE)
	{
		lua_pop(state, 2);  // remove page stack and incorrect top page value
		return false;
	}

	lua_remove(state, -2);  // remove page stack table
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
	luaL_checktype(state, 1, LUA_TTABLE);

	if (LS_PushMenuPageStackTable(state))
	{
		lua_len(state, -1);

		int menustacktop = lua_tointeger(state, -1);
		assert(menustacktop >= 0);
		lua_pop(state, 1);  // remove page stack size

		lua_insert(state, -2);  // swap new menu page with page stack table
		lua_seti(state, -2, menustacktop + 1);
	}

	if (m_state != m_luascript)
		LS_OpenMenu(state);

	return 0;
}

static int LS_global_menu_poppage(lua_State* state)
{
	int menustacktop = 0;

	if (LS_PushMenuPageStackTable(state))
	{
		lua_len(state, -1);

		menustacktop = lua_tointeger(state, -1);
		assert(menustacktop >= 0);
		lua_pop(state, 1);  // remove page stack size

		if (menustacktop > 0)
		{
			lua_pushnil(state);
			lua_seti(state, -2, menustacktop);
		}
	}

	if (menustacktop == 1)
		LS_CloseMenu(state);

	return 0;
}

static int LS_MenuText(lua_State* state, void (*printfunc)(int x, int y, const char* text))
{
	if (m_state == m_luascript)
	{
		luaL_checktype(state, 1, LUA_TNUMBER);
		luaL_checktype(state, 2, LUA_TNUMBER);
		luaL_checktype(state, 3, LUA_TSTRING);

		int x = lua_tointeger(state, 1);
		int y = lua_tointeger(state, 2);
		const char* text = lua_tostring(state, 3);

		printfunc(x, y, text);
	}

	return 0;
}

static int LS_global_menu_text(lua_State* state)
{
	return LS_MenuText(state, M_PrintWhite);
}

static int LS_global_menu_tintedtext(lua_State* state)
{
	return LS_MenuText(state, M_Print);
}

void LS_InitMenuModule(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "pushpage", LS_global_menu_pushpage },
		{ "poppage", LS_global_menu_poppage },

		{ "text", LS_global_menu_text },
		{ "tintedtext", LS_global_menu_tintedtext },
		{ NULL, NULL }
	};

	luaL_newlib(state, functions);
	lua_newtable(state);
	lua_setfield(state, -2, ls_menu_pagestack_name);
	lua_setglobal(state, ls_menu_name);
}

void LS_ShutdownMenuModule(lua_State* state)
{
	LS_CloseMenu(state);
}


static void LS_CallMenuPageFunction(const char* funcname, int arg)
{
	assert(funcname != NULL);

	lua_State* state = LS_GetState();
	assert(lua_gettop(state) == 0);

	qboolean success = false;

	if (LS_PushTopMenuPage(state))
	{
		if (lua_getfield(state, -1, funcname) == LUA_TFUNCTION)
		{
			lua_insert(state, -2);  // swap top menu page with draw function
			lua_pushnumber(state, arg);
			success = lua_pcall(state, 2, 0, 0) == LUA_OK;
		}
		else
		{
			lua_pop(state, 2);  // remove top menu page and incorrect draw function value
			lua_pushfstring(state, "menu page has no %s() function", funcname);
		}
	}

	if (!success)
	{
		LS_ReportError(state);
		LS_CloseMenu(state);
	}

	assert(lua_gettop(state) == 0);
}

void M_LuaScript_Draw(void)
{
	LS_CallMenuPageFunction("ondraw", 0);
}

void M_LuaScript_Key(int keycode)
{
	LS_CallMenuPageFunction("onkeypress", keycode);
}

#endif // USE_LUA_SCRIPTING
