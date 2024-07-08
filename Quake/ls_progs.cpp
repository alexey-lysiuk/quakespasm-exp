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

ddef_t *ED_GlobalAtOfs(int ofs);
const char* PR_GetTypeString(unsigned short type);
const char* PR_SafeGetString(int offset);
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

template <void (*Func)(lua_State* state, const dfunction_t* function)>
static int LS_FunctionMethod(lua_State* state)
{
	return LS_CallFunctionMethod(state, Func);
}

// Pushes file of 'function' userdata
static void LS_PushFunctionFile(lua_State* state, const dfunction_t* function)
{
	lua_pushstring(state, PR_SafeGetString(function->s_file));
}

// Pushes name of 'function' userdata
static void LS_PushFunctionName(lua_State* state, const dfunction_t* function)
{
	lua_pushstring(state, PR_SafeGetString(function->s_name));
}

// Returns function return type
static lua_Integer LS_GetFunctionReturnType(const dfunction_t* function)
{
	const int first_statement = function->first_statement;
	lua_Integer returntype;

	if (first_statement > 0)
	{
		returntype = ev_void;

		for (int i = first_statement, ie = progs->numstatements; i < ie; ++i)
		{
			dstatement_t* statement = &pr_statements[i];

			if (statement->op == OP_RETURN)
			{
				const ddef_t* def = ED_GlobalAtOfs(statement->a);
				returntype = def ? def->type : ev_bad;
				break;
			}
			else if (statement->op == OP_DONE)
				break;
		}
	}
	else
	{
		// TODO: create list of return types for built-in functions
		returntype = ev_bad;
	}

	return returntype;
}

// Pushes return type of 'function' userdata
static void LS_PushFunctionReturnType(lua_State* state, const dfunction_t* function)
{
	const lua_Integer returntype = LS_GetFunctionReturnType(function);
	lua_pushinteger(state, returntype);
}

// Pushes method of 'function' userdata by its name
static int LS_value_function_index(lua_State* state)
{
	size_t length;
	const char* name = luaL_checklstring(state, 2, &length);
	assert(name);
	assert(length > 0);

	if (strncmp(name, "file", length) == 0)
		lua_pushcfunction(state, LS_FunctionMethod<LS_PushFunctionFile>);
	else if (strncmp(name, "name", length) == 0)
		lua_pushcfunction(state, LS_FunctionMethod<LS_PushFunctionName>);
	else if (strncmp(name, "returntype", length) == 0)
		lua_pushcfunction(state, LS_FunctionMethod<LS_PushFunctionReturnType>);
	else
		luaL_error(state, "unknown function '%s'", name);

	return 1;
}

// Pushes string representation of given 'function' userdata
static void LS_PushFunctionToString(lua_State* state, const dfunction_t* function)
{
	const char* const returntype = "???";  // TODO: return type
	const char* name = PR_SafeGetString(function->s_name);
	const char* args = "";  // TODO: arguments

	lua_pushfstring(state, "%s %s(%s)", returntype, name, args);
}

// Sets metatable for 'function' userdata
static void LS_SetFunctionMetaTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_value_function_index },
		{ "__tostring", LS_FunctionMethod<LS_PushFunctionToString> },
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

// Pushes name of type by its index
static int LS_global_progs_typename(lua_State* state)
{
	const lua_Integer typeindex = luaL_checkinteger(state, 1);
	const char* const nameoftype = PR_GetTypeString(typeindex);
	lua_pushstring(state, nameoftype);
	return 1;
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
		{ "typename", LS_global_progs_typename },
		{ "version", LS_global_progs_version },
		{ nullptr, nullptr }
	};

	luaL_newlib(state, progs_functions);
	lua_setglobal(state, "progs");

	LS_LoadScript(state, "scripts/progs.lua");
}

#endif // USE_LUA_SCRIPTING
