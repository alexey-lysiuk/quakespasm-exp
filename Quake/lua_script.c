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


qboolean ED_GetFieldByIndex(edict_t* ed, size_t fieldindex, etype_t* type, const char** name, const eval_t** value);
const char* ED_GetFieldNameByOffset(int offset);
qboolean ED_GetFieldByName(edict_t* ed, const char* name, etype_t* type, const eval_t** value);

static const char* ls_axisnames[] = { "x", "y", "z" };

static int LS_Vec3ToString(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	vec3_t value;

	for (int i = 0; i < 3; ++i)
	{
		int type = lua_getfield(state, 1, ls_axisnames[i]);
		if (type != LUA_TNUMBER)
			luaL_error(state, "Bad value in vec3_t at index %d", i);

		value[i] = lua_tonumber(state, 2);
		lua_pop(state, 1);
	}

	lua_pushfstring(state, "'%f %f %f'", value[0], value[1], value[2]);
	return 1;
}

static void LS_SetVec3MetaTable(lua_State* state)
{
	static const char vec3mtblname[] = "__vec3mtbl";
	lua_getglobal(state, vec3mtblname);
	int mtbltype = lua_type(state, -1);

	if (mtbltype == LUA_TNIL)
	{
		lua_pop(state, 1);  // remove 'nil'
		lua_createtable(state, 0, 1);
		lua_pushvalue(state, -1);  // copy of table for lua_setglobal()
		lua_setglobal(state, vec3mtblname);
		lua_pushcfunction(state, LS_Vec3ToString);
		lua_setfield(state, -2, "__tostring");
	}
	else if (mtbltype != LUA_TTABLE)
		luaL_error(state, "Broken vec3_t metatable");

	lua_setmetatable(state, -2);
}

static qboolean LS_MakeEdictTable(lua_State* state, int index)
{
	if (!sv.active || index < 0 || index >= sv.num_edicts)
	{
		lua_pushnil(state);
		return false;
	}

	edict_t* ed = EDICT_NUM(index);
	assert(ed);

	lua_createtable(state, 0, 0);

	if (ed->free)
		return true;

	for (int fi = 1; fi < progs->numfielddefs; ++fi)
	{
		etype_t type;
		const char* name;
		const eval_t* value;

		if (!ED_GetFieldByIndex(ed, fi, &type, &name, &value))
			continue;

		assert(type != ev_bad);
		assert(name);
		assert(value);

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
			LS_SetVec3MetaTable(state);

			for (int i = 0; i < 3; ++i)
			{
				lua_pushnumber(state, value->vector[i]);
				lua_setfield(state, -2, ls_axisnames[i]);
			}

			break;

		case ev_entity:
			lua_pushfstring(state, "entity %d", NUM_FOR_EDICT(PROG_TO_EDICT(value->edict)));
			break;

		case ev_field:
			lua_pushfstring(state, ".%s", ED_GetFieldNameByOffset(value->_int));
			break;

		case ev_function:
			lua_pushfstring(state, "%s()", PR_GetString((pr_functions + value->function)->s_name));
			break;

		case ev_pointer:
			lua_pushstring(state, "pointer");
			break;

		default:
			// Unknown type, e.g. some of FTE extensions
			lua_pushfstring(state, "bad type %d", type);
			break;
		}

		lua_setfield(state, -2, name);
	}

	return true;
}

static int LS_Edict(lua_State* state)
{
	lua_Integer i = luaL_checkinteger(state, 1);
	LS_MakeEdictTable(state, i);
	return 1;
}

static int LS_GetNextEdict(lua_State* state)
{
	lua_Integer i = luaL_checkinteger(state, 2);
	i = luaL_intop(+, i, 1);
	lua_pushinteger(state, i);
	return LS_MakeEdictTable(state, i) ? 2 : 1;
}

static int LS_Edicts(lua_State* state)
{
	lua_pushcfunction(state, LS_GetNextEdict);
	lua_createtable(state, 0, 4);
	lua_pushinteger(state, -1);
	return 3;
}

static int LS_EdictsCount(lua_State* state)
{
	lua_pushinteger(state, sv.active ? sv.num_edicts : 0);
	return 1;
}

static int LS_EdictIndex(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);
	luaL_checktype(state, 2, LUA_TSTRING);

	lua_pushvalue(state, 2);

	int valuetype = lua_rawget(state, 1);

	if (valuetype == LUA_TNIL)
	{
		const char* fieldname = luaL_checkstring(state, 2);

		//ED_GetFieldByName(

		//lua_pop(state, 1);  // remove 'nil'
		// TODO
	}

	//lua_pushnil(state);
	return 1;
}

static void LS_SetEdictMetaTable(lua_State* state)
{
	static const char edictmtbl[] = "__edictmtbl";
	lua_getglobal(state, edictmtbl);
	int mtbltype = lua_type(state, -1);

	if (mtbltype == LUA_TNIL)
	{
		lua_pop(state, 1);  // remove 'nil'
		lua_createtable(state, 0, 1);
		lua_pushvalue(state, -1);  // copy of table for lua_setglobal()
		lua_setglobal(state, edictmtbl);
		lua_pushcfunction(state, LS_EdictIndex);
		lua_setfield(state, -2, "__index");
	}
	else if (mtbltype != LUA_TTABLE)
		luaL_error(state, "Broken edict metatable");

	lua_setmetatable(state, -2);
}

static int LS_EdictsIndex(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	lua_Integer index = luaL_checkinteger(state, 2);

	if (!sv.active || index < 0 || index >= sv.num_edicts)
	{
		lua_pushnil(state);
		return 1;
	}

	int edicttype = lua_rawgeti(state, 1, index);

	if (edicttype == LUA_TNIL)
	{
		lua_pop(state, 1);  // remove 'nil'
		lua_createtable(state, 0, 0);
		lua_pushvalue(state, -1);  // copy for return value
		lua_rawseti(state, -2, index);
		LS_SetEdictMetaTable(state);
	}
	else if (edicttype != LUA_TTABLE)
		luaL_error(state, "Broken edict table at index %d", index);

	
//	lua_getglobal(state, ls_edictsname);
//
//	if (lua_type(state, -1) != LUA_TTABLE)
//		luaL_error(state, "Broken edicts global table");

//	lua_rawgeti(state, -1, index);
//	int edicttype = lua_type(state, -1);
//
//	if (edicttype == LUA_TNIL)
//	{
//		lua_pop(state, 1);  // remove 'nil'
//		lua_createtable(state, 0, 0);
//		lua_pushvalue(state, -1);  // copy for return value
//		lua_rawseti(state, -1, index);
//	}
//	else if (edicttype != LUA_TTABLE)
//		luaL_error(state, "Broken edicts global table");

	return 1;
}

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

//	// Register scripting functions
//	typedef struct
//	{
//		const char* name;
//		lua_CFunction ptr;
//	} ScriptFunction;
//
//	static const ScriptFunction scriptfuncs[] =
//	{
//		{ "edict", LUA_Edict },
//		{ "edicts", LUA_Edicts },
//		{ NULL, NULL }
//	};
//
//	for (const ScriptFunction* func = scriptfuncs; func->name; ++func)
//	{
//		lua_pushcfunction(state, func->ptr);
//		lua_setglobal(state, func->name);
//	}

	// Edicts table
	lua_createtable(state, 0, 0);
	lua_pushvalue(state, -1);
	lua_setglobal(state, "edicts");

	// Edicts metatable
	lua_createtable(state, 0, 2);
	lua_pushcfunction(state, LS_EdictsIndex);
	lua_setfield(state, -2, "__index");
	lua_pushcfunction(state, LS_EdictsCount);
	lua_setfield(state, -2, "__len");
	lua_setmetatable(state, -2);
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
