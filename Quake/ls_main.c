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


qboolean ED_GetFieldByIndex(edict_t* ed, size_t fieldindex, const char** name, etype_t* type, const eval_t** value);
qboolean ED_GetFieldByName(edict_t* ed, const char* name, etype_t* type, const eval_t** value);
const char* ED_GetFieldNameByOffset(int offset);

static const char* ls_axisnames[] = { "x", "y", "z" };
static const char ls_edictindexname[] = "_luascripting_edictindex";
static const char ls_lazyevalname[] = "lazyeval";

static int LS_Vec3ToString(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	vec3_t value;

	for (int i = 0; i < 3; ++i)
	{
		int type = lua_getfield(state, 1, ls_axisnames[i]);
		if (type != LUA_TNUMBER)
			luaL_error(state, "Bad value in vec3_t at index %d", i);

		value[i] = lua_tonumber(state, -1);
		lua_pop(state, 1);  // remove value
	}

	char buf[128];
	int length = q_snprintf(buf, sizeof buf, "%.1f %.1f %.1f", value[0], value[1], value[2]);

	lua_pushlstring(state, buf, length);
	return 1;
}

static void LS_SetVec3MetaTable(lua_State* state)
{
	static const char vec3mtblname[] = "_luascripting_vec3metatable";
	lua_getglobal(state, vec3mtblname);
	int mtbltype = lua_type(state, -1);

	if (mtbltype == LUA_TNIL)
	{
		lua_pop(state, 1);  // remove 'nil'

		// Create metatable, and save it to global variable
		lua_createtable(state, 0, 1);
		lua_pushvalue(state, -1);  // copy of table for lua_setglobal()
		lua_setglobal(state, vec3mtblname);

		// Add function to metatable
		lua_pushcfunction(state, LS_Vec3ToString);
		lua_setfield(state, -2, "__tostring");
	}
	else if (mtbltype != LUA_TTABLE)
		luaL_error(state, "Broken vec3_t metatable");

	lua_setmetatable(state, -2);
}

static void LS_PushFieldValue(lua_State* state, etype_t type, const char* name, const eval_t* value)
{
	assert(type != ev_bad);
	assert(name);
	assert(value);

	switch (type)
	{
	case ev_void:
		lua_pushstring(state, "void");
		break;

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
}

// Pushes competle edict table with all fields set
static qboolean LS_BuildFullEdictTable(lua_State* state, int index)
{
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

		if (!ED_GetFieldByIndex(ed, fi, &name, &type, &value))
			continue;

		LS_PushFieldValue(state, type, name, value);
		lua_setfield(state, -2, name);
	}

	return true;
}

// Pushes number of edicts
static int LS_EdictsCount(lua_State* state)
{
	lua_pushinteger(state, sv.active ? sv.num_edicts : 0);
	return 1;
}

// Pushes value of edict field by its name
static int LS_EdictIndex(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);
	luaL_checktype(state, 2, LUA_TSTRING);

	// Fetch edict index, [0..num_edicts), from edict table
	lua_pushstring(state, ls_edictindexname);
	lua_rawget(state, 1);

	lua_Integer index = luaL_checkinteger(state, -1);
	lua_pop(state, 1);  // remove ls_edictindexname

	if (sv.active && index >= 0 && index < sv.num_edicts)
	{
		edict_t* ed = EDICT_NUM(index);
		assert(ed);

		if (!ed->free)
		{
			const char* name = luaL_checkstring(state, 2);
			etype_t type;
			const eval_t* value;

			if (ED_GetFieldByName(ed, name, &type, &value))
			{
				LS_PushFieldValue(state, type, name, value);

				// Add field and its value to edict table
				lua_pushvalue(state, 2);  // field name
				lua_pushvalue(state, -2);  // copy of field value for lua_rawset()
				lua_rawset(state, 1);

				return 1;
			}
		}
	}

	lua_pushnil(state);
	return 1;
}

static void LS_SetEdictMetaTable(lua_State* state)
{
	static const char edictmtbl[] = "_luascripting_edictmetatable";
	lua_getglobal(state, edictmtbl);

	// Check edict metatable type, and create it if needed
	int mtbltype = lua_type(state, -1);

	if (mtbltype == LUA_TNIL)
	{
		lua_pop(state, 1);  // remove 'nil'

		// Create metatable, and save it to global variable
		lua_createtable(state, 0, 1);
		lua_pushvalue(state, -1);  // copy of table for lua_setglobal()
		lua_setglobal(state, edictmtbl);

		// Add function to metatable
		lua_pushcfunction(state, LS_EdictIndex);
		lua_setfield(state, -2, "__index");
	}
	else if (mtbltype != LUA_TTABLE)
		luaL_error(state, "Broken edict metatable");

	lua_setmetatable(state, -2);
}

// Pushes edict table by its index, [0..num_edicts)
static int LS_EdictsIndex(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	// Check edict index, [0..num_edicts), for validity
	lua_Integer index = luaL_checkinteger(state, 2);

	if (!sv.active || index < 0 || index >= sv.num_edicts)
	{
		lua_pushnil(state);
		return 1;
	}

	// Fetch edict lazy evaluation status
	lua_pushstring(state, ls_lazyevalname);
	lua_rawget(state, 1);

	qboolean lazyeval = lua_toboolean(state, -1);

	// Create edict table
	lua_createtable(state, 0, 16);

	if (lazyeval)
	{
		LS_SetEdictMetaTable(state);

		// Set own edict index, [0..num_edicts), to edict table
		lua_pushnumber(state, index);
		lua_setfield(state, -2, ls_edictindexname);
	}
	else
		LS_BuildFullEdictTable(state, index);

	// Add this edict to 'edicts' global table
	lua_pushvalue(state, -1);
	lua_rawseti(state, 1, index);

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

	// Create and register 'edicts' global table
	lua_createtable(state, sv.active ? sv.num_edicts : 0, 1);
	lua_pushboolean(state, true);
	lua_setfield(state, -2, ls_lazyevalname);
	lua_pushvalue(state, -1);
	lua_setglobal(state, "edicts");

	// Create and set metatable for 'edicts' global table
	lua_createtable(state, 0, 3);
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
