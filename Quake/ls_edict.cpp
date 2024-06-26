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

#include <cassert>

#include "ls_common.h"

extern "C"
{
#include "quakedef.h"

qboolean ED_GetFieldByIndex(edict_t* ed, size_t fieldindex, const char** name, etype_t* type, const eval_t** value);
qboolean ED_GetFieldByName(edict_t* ed, const char* name, etype_t* type, const eval_t** value);
const char* ED_GetFieldNameByOffset(int offset);
const char* SV_GetEntityName(edict_t* entity);
} // extern "C"

constexpr LS_UserDataType<int> ls_edict_type("edct");

//
// Expose edict_t as 'edict' userdata
//

static void LS_SetEdictMetaTable(lua_State* state);

// Creates and pushes 'edict' userdata by edict index, [0..sv.num_edicts)
void LS_PushEdictValue(lua_State* state, int edictindex)
{
	int& newvalue = ls_edict_type.New(state);
	newvalue = edictindex;

	LS_SetEdictMetaTable(state);
}

// Pushes field value by its type and name
static void LS_PushEdictFieldValue(lua_State* state, etype_t type, const eval_t* value)
{
	assert(type != ev_bad);
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
		LS_PushVec3Value(state, value->vector);
		break;

	case ev_entity:
		if (value->edict == 0)
			lua_pushnil(state);
		else
			LS_PushEdictValue(state, NUM_FOR_EDICT(PROG_TO_EDICT(value->edict)));
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

// Gets pointer to edict_t from 'edict' userdata
static edict_t* LS_GetEdictFromUserData(lua_State* state)
{
	const int index = ls_edict_type.GetValue(state, 1);
	return (index >= 0 && index < sv.num_edicts) ? EDICT_NUM(index) : NULL;
}

// Pushes result of comparison for equality of two edict values
static int LS_value_edict_eq(lua_State* state)
{
	const int left = ls_edict_type.GetValue(state, 1);
	const int right = ls_edict_type.GetValue(state, 2);

	lua_pushboolean(state, left == right);
	return 1;
}

// Pushes true if index of first edict is less than index of second edict, otherwise pushes false
static int LS_value_edict_lt(lua_State* state)
{
	const int left = ls_edict_type.GetValue(state, 1);
	const int right = ls_edict_type.GetValue(state, 2);

	lua_pushboolean(state, left < right);
	return 1;
}

// Pushes value of edict field by its name
// or pushes a table with name, type, value by field's numerical index
static int LS_value_edict_index(lua_State* state)
{
	edict_t* ed = LS_GetEdictFromUserData(state);

	if (ed == NULL || ed->free)
	{
		lua_pushnil(state);  // TODO: default value instead of nil
		return 1;
	}

	const char* name;
	etype_t type;
	const eval_t* value;

	int indextype = lua_type(state, 2);

	if (indextype == LUA_TSTRING)
	{
		name = luaL_checkstring(state, 2);

		if (ED_GetFieldByName(ed, name, &type, &value))
			LS_PushEdictFieldValue(state, type, value);
		else
			lua_pushnil(state);
	}
	else if (indextype == LUA_TNUMBER)
	{
		// TODO: optimize numeric indexing to be O(n) instead of O(n^2)

		int fieldindex = lua_tointeger(state, 2);
		int fieldswithvalues = 0;  // a value before first valid index that starts with one on Lua side

		for (int i = 1; i < progs->numfielddefs; ++i)
		{
			const char* name;
			etype_t type;
			const eval_t* value;

			if (ED_GetFieldByIndex(ed, i, &name, &type, &value))
			{
				++fieldswithvalues;

				if (fieldindex == fieldswithvalues)
				{
					lua_createtable(state, 0, 3);

					lua_pushstring(state, name);
					lua_setfield(state, -2, "name");
					lua_pushnumber(state, type);
					lua_setfield(state, -2, "type");
					LS_PushEdictFieldValue(state, type, value);
					lua_setfield(state, -2, "value");

					break;
				}
			}
		}

		if (fieldindex > fieldswithvalues)
			lua_pushnil(state);  // no such index
	}
	else
		luaL_error(state, "invalid type %s of edict index", lua_typename(state, indextype));

	return 1;
}

// Pushes next() function, table with edict's fields and values, initial value (nil)
static int LS_value_edict_pairs(lua_State* state)
{
	edict_t* ed = LS_GetEdictFromUserData(state);
	qboolean valid = ed != NULL;

	lua_getglobal(state, "next");
	lua_createtable(state, 0, valid ? 16 : 0);

	if (valid)
	{
		for (int i = 1; i < progs->numfielddefs; ++i)
		{
			const char* name;
			etype_t type;
			const eval_t* value;

			if (ED_GetFieldByIndex(ed, i, &name, &type, &value))
			{
				LS_PushEdictFieldValue(state, type, value);
				lua_setfield(state, -2, name);
			}
		}
	}

	lua_pushnil(state);
	return 3;
}

// Pushes string representation of given edict
static int LS_value_edict_tostring(lua_State* state)
{
	edict_t* ed = LS_GetEdictFromUserData(state);

	if (ed == NULL)
		lua_pushstring(state, "invalid edict");
	else
		lua_pushfstring(state, "edict %d: %s", NUM_FOR_EDICT(ed), SV_GetEntityName(ed));

	return 1;
}

// Sets metatable for edict table
static void LS_SetEdictMetaTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "__eq", LS_value_edict_eq },
		{ "__lt", LS_value_edict_lt },
		{ "__index", LS_value_edict_index },
		{ "__pairs", LS_value_edict_pairs },
		{ "__tostring", LS_value_edict_tostring },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "edict"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
}


//
// Expose sv.edicts as 'edicts' global table
//

// Pushes edict userdata by its integer index, [1..num_edicts]
static int LS_global_edicts_index(lua_State* state)
{
	qboolean badindex = true;

	if (sv.active && lua_isnumber(state, 2))
	{
		// Check edict index, [1..num_edicts], for validity
		lua_Integer index = lua_tointeger(state, 2);

		if (index > 0 && index <= sv.num_edicts)
		{
			LS_PushEdictValue(state, index - 1);  // on C side, indices start with 0
			badindex = false;
		}
	}

	if (badindex)
		lua_pushnil(state);

	return 1;
}

// Pushes number of edicts
static int LS_global_edicts_len(lua_State* state)
{
	lua_pushinteger(state, sv.active ? sv.num_edicts : 0);
	return 1;
}

static edict_t* LS_GetEdictFromParameter(lua_State* state)
{
	edict_t* edict = NULL;

	if (sv.active)
	{
		int indextype = lua_type(state, 1);

		if (indextype == LUA_TNUMBER)
		{
			// Check edict index, [1..num_edicts], for validity
			lua_Integer index = lua_tointeger(state, 1);

			if (index > 0 && index <= sv.num_edicts)
				return EDICT_NUM(index - 1);  // on C side, indices start with zero
		}
		else if (indextype == LUA_TUSERDATA)
			return LS_GetEdictFromUserData(state);
		else
			luaL_error(state, "invalid type %s of edicts key", lua_typename(state, indextype));
	}

	return edict;
}

// Pushes boolean free state of edict passed index or by value
static int LS_global_edicts_isfree(lua_State* state)
{
	edict_t* edict = LS_GetEdictFromParameter(state);
	lua_pushboolean(state, edict ? edict->free : true);
	return 1;
}

// Pushes user-frendly name of edict passed index or by value
static int LS_global_edicts_getname(lua_State* state)
{
	edict_t* edict = LS_GetEdictFromParameter(state);

	if (edict)
		lua_pushstring(state, SV_GetEntityName(edict));
	else
		lua_pushnil(state);

	return 1;
}


// Edict reference collection

static const char* LS_references_GetString(int num)
{
	const char* result = PR_GetString(num);
	return result[0] == '\0' ? NULL : result;
}

static qboolean LS_references_StringsEqual(const char* string, int num)
{
	const char* other = PR_GetString(num);
	return strcmp(string, other) == 0;
}

static void LS_references_ConvertTable(lua_State* state, int index)
{
	lua_newtable(state);

	int destindex = lua_gettop(state);
	lua_Integer counter = 1;

	lua_pushnil(state);  // first key

	while (lua_next(state, index) != 0)
	{
		lua_pop(state, 1);  // remove value (zero)

		LS_PushEdictValue(state, lua_tointeger(state, -1));
		lua_rawseti(state, destindex, counter++);
	}

	// Replace initial table
	lua_remove(state, index);
	lua_insert(state, index);
}

static void LS_references_PushEdictIndex(lua_State* state, int tableindex, edict_t* edict)
{
	lua_pushinteger(state, NUM_FOR_EDICT(edict));
	lua_pushinteger(state, 0);
	lua_rawset(state, tableindex);
}

// Push two tables:
// * The first one contains list of edicts referenced by edict passed as argument (outgoing references)
// * The second one contains list of edicts that refer edict passed as argument (incoming references)
static int LS_global_edicts_references(lua_State* state)
{
	edict_t* edict = LS_GetEdictFromParameter(state);

	if (!edict)
		return 0;

	lua_settop(state, 0);

	// These tables maps edict indices to dummy value (zero)
	lua_newtable(state);  // [1] this edict references other edicts (outgoing references)
	lua_newtable(state);  // [2] this edict is referenced by other edicts (incoming references)

	if (edict->free)
		return 2;  // return two empty tables

	const char* targetname = NULL;
	const char* target = NULL;
	const char* killtarget = NULL;

	for (int f = 1; f < progs->numfielddefs; ++f)
	{
		const char* name;
		etype_t type;
		const eval_t* value;
		
		if (ED_GetFieldByIndex(edict, f, &name, &type, &value))
		{
			if (type == ev_entity)
			{
				edict_t* refedict = PROG_TO_EDICT(value->edict);

				if (!refedict->free && refedict != edict)
				{
					qboolean owner = strcmp(name, "owner") == 0;
					LS_references_PushEdictIndex(state, owner ? 2 : 1, refedict);
				}
			}
			else if (type == ev_string)
			{
				if (strcmp(name, "targetname") == 0)
					targetname = LS_references_GetString(value->string);
				else if (strcmp(name, "target") == 0)
					target = LS_references_GetString(value->string);
				else if (strcmp(name, "killtarget") == 0)
					killtarget = LS_references_GetString(value->string);
			}
		}
	}

	for (int e = 0; e < sv.num_edicts; ++e)
	{
		edict_t* probe = EDICT_NUM(e);
		assert(probe);

		if (probe->free || probe == edict)
			continue;

		for (int f = 1; f < progs->numfielddefs; ++f)
		{
			const char* name;
			etype_t type;
			const eval_t* value;

			if (ED_GetFieldByIndex(probe, f, &name, &type, &value))
			{
				if (type == ev_entity && EDICT_TO_PROG(edict) == value->edict)
				{
					qboolean owner = strcmp(name, "owner") == 0;
					LS_references_PushEdictIndex(state, owner ? 1 : 2, probe);
				}
				else if (type == ev_string)
				{
					if (targetname && (strcmp(name, "target") == 0 || strcmp(name, "killtarget") == 0) && LS_references_StringsEqual(targetname, value->string))
						LS_references_PushEdictIndex(state, 2, probe);
					else if (target && strcmp(name, "targetname") == 0 && LS_references_StringsEqual(target, value->string))
						LS_references_PushEdictIndex(state, 1, probe);
					else if (killtarget && strcmp(name, "targetname") == 0 && LS_references_StringsEqual(killtarget, value->string))
						LS_references_PushEdictIndex(state, 1, probe);
				}
			}
		}
	}

	// Convert tables with edict indices as keys to sequences of edicts
	LS_references_ConvertTable(state, 1);
	LS_references_ConvertTable(state, 2);

	// Sort reference tables by edict index
	lua_getglobal(state, "table");
	lua_pushstring(state, "sort");
	lua_rawget(state, 3);
	lua_remove(state, 3);  // remove 'table' global table
	lua_pushvalue(state, 3);  // copy of table.sort() for the second call
	lua_pushvalue(state, 1);  // 'references' table
	lua_call(state, 1, 0);
	lua_pushvalue(state, 2);  // 'referenced by' table
	lua_call(state, 1, 0);

	return 2;
}


// Creates 'edicts' table with helper functions for 'edict' values
void LS_InitEdictType(lua_State* state)
{
	static const luaL_Reg edicts_metatable[] =
	{
		{ "__index", LS_global_edicts_index },
		{ "__len", LS_global_edicts_len },
		{ NULL, NULL }
	};

	lua_newtable(state);
	lua_pushvalue(state, -1);  // copy for lua_setmetatable()
	lua_setglobal(state, "edicts");

	static const luaL_Reg edicts_functions[] =
	{
		{ "references", LS_global_edicts_references },
		{ "getname", LS_global_edicts_getname },
		{ "isfree", LS_global_edicts_isfree },
		{ NULL, NULL }
	};

	luaL_setfuncs(state, edicts_functions, 0);

	luaL_newmetatable(state, "edicts");
	luaL_setfuncs(state, edicts_metatable, 0);
	lua_setmetatable(state, -2);

	lua_pop(state, 1);  // remove table

	LS_LoadScript(state, "scripts/edicts.lua");
}

#endif // USE_LUA_SCRIPTING
