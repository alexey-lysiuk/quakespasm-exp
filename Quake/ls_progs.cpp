/*

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

extern "C"
{
#include "quakedef.h"
}

constexpr LS_UserDataType<int> ls_function_type("function");

// Gets pointer to dfunction_t from 'function' userdata
static dfunction_t* LS_GetFunctionFromUserData(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	const int index = ls_function_type.GetValue(state, 1);
	return (index >= 0 && index < progs->numfunctions) ? &pr_functions[index] : nullptr;
}

static int LS_CallFunctionMethod(lua_State* state, void (*method)(lua_State* state, const dfunction_t* function))
{
	if (dfunction_t* function = LS_GetFunctionFromUserData(state))
		method(state, function);
	else
		luaL_error(state, "invalid function");

	return 1;
}

static void LS_PushFunctionFile(lua_State* state, const dfunction_t* function)
{
	lua_pushstring(state, PR_GetString(function->s_file));
}

// Pushes file of 'function' userdata
static int LS_value_function_file(lua_State* state)
{
	return LS_CallFunctionMethod(state, LS_PushFunctionFile);
}

static void LS_PushFunctionName(lua_State* state, const dfunction_t* function)
{
	lua_pushstring(state, PR_GetString(function->s_name));
}

// Pushes name of 'function' userdata
static int LS_value_function_name(lua_State* state)
{
	return LS_CallFunctionMethod(state, LS_PushFunctionName);
}

// Pushes method of 'function' userdata by its name
static int LS_value_function_index(lua_State* state)
{
	size_t length;
	const char* name = luaL_checklstring(state, 2, &length);
	assert(name);
	assert(length > 0);

	if (strncmp(name, "file", length) == 0)
		lua_pushcfunction(state, LS_value_function_file);
	else if (strncmp(name, "name", length) == 0)
		lua_pushcfunction(state, LS_value_function_name);
	else
		luaL_error(state, "unknown function '%s'", name);

	return 1;
}

// Pushes string representation of given 'function' userdata
static int LS_value_function_tostring(lua_State* state)
{
	if (dfunction_t* function = LS_GetFunctionFromUserData(state))
		lua_pushfstring(state, "%s()", PR_GetString(function->s_name));
	else
		lua_pushstring(state, "invalid function");

	return 1;
}

// Sets metatable for 'function' userdata
static void LS_SetFunctionMetaTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_value_function_index },
		{ "__tostring", LS_value_function_tostring },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "func"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
}

// Returns CRC of progdefs header (PROGHEADER_CRC)
static int LS_global_progs_crc(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	lua_pushinteger(state, progs->crc);
	return 1;
}

// Returns CRC of 'progs.dat' file
static int LS_global_progs_datcrc(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	lua_pushinteger(state, pr_crc);
	return 1;
}

static int LS_global_functions_iterator(lua_State* state)
{
	lua_Integer index = luaL_checkinteger(state, 2);
	index = luaL_intop(+, index, 1);

	if (index > 0 && index < progs->numfunctions)
	{
		lua_pushinteger(state, index);

		int& newvalue = ls_function_type.New(state);
		newvalue = index;
		LS_SetFunctionMetaTable(state);

		return 2;
	}

	lua_pushnil(state);
	return 1;
}

// Returns progs function iterator, e.g., for i, f in progs.functions() do print(i, f) end
static int LS_global_progs_functions(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	const lua_Integer index = luaL_optinteger(state, 1, 0);
	lua_pushcfunction(state, LS_global_functions_iterator);
	lua_pushnil(state);  // unused
	lua_pushinteger(state, index);  // initial value
	return 3;
}

// Returns progs version number (PROG_VERSION)
static int LS_global_progs_version(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	lua_pushinteger(state, progs->version);
	return 1;

}

void LS_InitProgsType(lua_State* state)
{
	static const luaL_Reg progs_functions[] =
	{
		{ "crc", LS_global_progs_crc },
		{ "datcrc", LS_global_progs_datcrc },
		{ "functions", LS_global_progs_functions },
		{ "version", LS_global_progs_version },
		{ nullptr, nullptr }
	};

	luaL_newlib(state, progs_functions);
	lua_setglobal(state, "progs");

	LS_LoadScript(state, "scripts/progs.lua");
}

#endif // USE_LUA_SCRIPTING
