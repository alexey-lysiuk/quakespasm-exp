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
#include <set>
#include <vector>

#include "ls_common.h"
#include "ls_vector.h"

extern "C"
{
#include "quakedef.h"

qboolean LS_GetEdictFieldByIndex(const edict_t* ed, size_t fieldindex, const char** name, etype_t* type, const eval_t** value);
qboolean LS_GetEdictFieldByName(const edict_t* ed, const char* name, etype_t* type, const eval_t** value);
const char* LS_GetEdictFieldName(int offset);
const char* SV_GetEntityName(const edict_t* entity);
const ddef_t* LS_GetProgsGlobalDefinitionByIndex(int index);
const char* LS_GetProgsString(int offset);
} // extern "C"

constexpr LS_UserDataType<int> ls_edict_type("edict");

static int LS_GetEdictIndex(lua_State* state, const edict_t* edict)
{
	const int index = (reinterpret_cast<const byte*>(edict) - reinterpret_cast<const byte*>(sv.edicts)) / pr_edict_size;

	if (index < 0 || index >= sv.num_edicts)
		luaL_error(state, "invalid edict index %d for pointer %p", index, edict);

	return index;
}

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

void LS_PushEdictValue(lua_State* state, const edict_t* edict)
{
	LS_PushEdictValue(state, LS_GetEdictIndex(state, edict));
}

// Pushes field value by its type and name
void LS_PushEdictFieldValue(lua_State* state, etype_t type, const eval_t* value)
{
	assert(type != ev_bad);
	assert(value);

	switch (type)
	{
	case ev_void:
		lua_pushstring(state, "void");
		break;

	case ev_string:
		lua_pushstring(state, LS_GetProgsString(value->string));
		break;

	case ev_float:
		lua_pushnumber(state, value->_float);
		break;

	case ev_vector:
		LS_PushVectorValue(state, LS_Vector3(value->vector));
		break;

	case ev_entity:
		if (value->edict == 0)
			lua_pushnil(state);
		else
			LS_PushEdictValue(state, PROG_TO_EDICT(value->edict));
		break;

	case ev_field:
		lua_pushfstring(state, ".%s", LS_GetEdictFieldName(value->_int));
		break;

	case ev_function:
		lua_pushfstring(state, "%s()", LS_GetProgsString((pr_functions + value->function)->s_name));
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

		if (LS_GetEdictFieldByName(ed, name, &type, &value))
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

			if (LS_GetEdictFieldByIndex(ed, i, &name, &type, &value))
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

			if (LS_GetEdictFieldByIndex(ed, i, &name, &type, &value))
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
		lua_pushfstring(state, "edict %d: %s", LS_GetEdictIndex(state, ed), SV_GetEntityName(ed));

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

static int LS_FindProgsFunction(const char* const functionname, int& functionindex)
{
	if (!progs)
		return -1;

	if (functionindex >= progs->numfunctions)
		functionindex = -1;

	const auto MatchFunction = [functionname](const int index)
	{
		const char* name = LS_GetProgsString(pr_functions[index].s_name);
		return strcmp(name, functionname) == 0;
	};

	if (functionindex > 0 && MatchFunction(functionindex))
		return functionindex;

	for (int i = 1, e = progs->numfunctions; i < e; ++i)
	{
		if (MatchFunction(i))
		{
			functionindex = i;
			return i;
		}
	}

	return -1;
}

static bool LS_DoDamage(int function, edict_t* target)
{
	const float health = target->v.health;

	if (health <= 0.0f && target->v.takedamage <= 0.0f)
		return false;

	// void T_Damage(entity target, entity inflictor, entity attacker, float damage)
	int* globals_iptr = (int*)pr_globals;
	edict_t* inflictor = svs.clients[0].edict;
	globals_iptr[OFS_PARM0] = EDICT_TO_PROG(target);     // target
	globals_iptr[OFS_PARM1] = EDICT_TO_PROG(inflictor);  // inflictor
	globals_iptr[OFS_PARM2] = globals_iptr[OFS_PARM1];   // attacker
	pr_globals[OFS_PARM3] = health + 1.0f;               // damage

	// Fill remaining four vec3 parameters with zeroes to handle
	// T_Damage() function with extra arguments, e.g. Copper 1.30
	memset(&pr_globals[OFS_PARM4], 0, sizeof(float) * 3 * 4);

	PR_ExecuteProgram(function);
	return true;
}

// Removes edict passed index or by value
static int LS_global_edicts_destroy(lua_State* state)
{
	edict_t* edict = LS_GetEdictFromParameter(state);

	// Skip free edict, worldspawn, player
	if (!edict || edict->free || edict == sv.edicts || edict == svs.clients[0].edict)
		return 0;

	static int cachedfunction = -1;
	const int function = LS_FindProgsFunction("T_Damage", cachedfunction);

	if (function == -1)
		return 0;

	LS_DoDamage(function, edict);
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

// Removes edict passed index or by value
static int LS_global_edicts_remove(lua_State* state)
{
	edict_t* edict = LS_GetEdictFromParameter(state);

	// Skip free edict, worldspawn, player
	if (edict && !edict->free && edict != sv.edicts && edict != svs.clients[0].edict)
	{
		static int cachedfunction = -1;
		const int function = LS_FindProgsFunction("SUB_UseTargets", cachedfunction);

		if (function > 0)
		{
			const ddef_t* definition = nullptr;

			for (int i = 1, e = progs->numglobaldefs; i < e; ++i)
			{
				definition = LS_GetProgsGlobalDefinitionByIndex(i);
				if (!definition)
					continue;

				const char* const definitionname = LS_GetProgsString(definition->s_name);

				if (strcmp(definitionname, "activator") == 0)
					break;
			}

			if (definition)
			{
				// Set target as self
				int* const selfptr = reinterpret_cast<int*>(pr_globals + 28); // TODO
				const int prevself = *selfptr;
				*selfptr = EDICT_TO_PROG(edict);

				// Set player as activator
				int* const activatorptr = reinterpret_cast<int*>(pr_globals + definition->ofs);
				const int prevactivator = *activatorptr;
				*activatorptr = EDICT_TO_PROG(svs.clients[0].edict);

				PR_ExecuteProgram(function);

				// Restore self and activator
				*selfptr = prevself;
				*activatorptr = prevactivator;
			}
		}

		ED_Free(edict);

		lua_pushboolean(state, true);
		return 1;
	}

	return 0;
}


// Edict reference collection

static const char* LS_references_GetString(int num)
{
	const char* result = LS_GetProgsString(num);
	return result[0] == '\0' ? NULL : result;
}

static qboolean LS_references_StringsEqual(const char* string, int num)
{
	const char* other = LS_GetProgsString(num);
	return strcmp(string, other) == 0;
}

// Push two tables:
// * The first one contains list of edicts referenced by edict passed as argument (outgoing references)
// * The second one contains list of edicts that refer edict passed as argument (incoming references)
static int LS_global_edicts_references(lua_State* state)
{
	const edict_t* const edict = LS_GetEdictFromParameter(state);

	if (!edict)
		return 0;

	if (edict->free)
	{
		// return two empty tables
		lua_newtable(state);
		lua_newtable(state);
		return 2;
	}

	using ReferenceSet = std::set<int, std::less<int>, LS_TempAllocator<int>>;
	ReferenceSet outgoing;  // Other edicts referenced by the given edict (outgoing references)
	ReferenceSet incoming;  // The given edict is referenced by other edicts (incoming references)

	enum ReferenceKind { Outgoing, Incoming };

	const auto AddReference = [&state, &outgoing, &incoming](ReferenceKind kind, const edict_t* const edict)
	{
		ReferenceSet& references = kind == Outgoing ? outgoing : incoming;
		references.insert(LS_GetEdictIndex(state, edict));
	};

	using TargetList = std::vector<const char*, LS_TempAllocator<const char*>>;
	TargetList targetnames;
	TargetList targets;
	TargetList killtargets;

	const auto IsTarget = [](const char* const name)
	{
		return strncmp(name, "target", 6) == 0
			// Do not match 'targetname' field name
			&& (name[6] == '\0' || (name[6] >= '0' && name[6] <= '9'));
	};

	const auto IsTargetName = [](const char* const name)
	{
		return strncmp(name, "targetname", 10) == 0;
	};

	const auto IsKillTarget = [](const char* const name)
	{
		return strncmp(name, "killtarget", 10) == 0;
	};

	for (int f = 1; f < progs->numfielddefs; ++f)
	{
		const char* name;
		etype_t type;
		const eval_t* value;
		
		if (LS_GetEdictFieldByIndex(edict, f, &name, &type, &value))
		{
			if (type == ev_entity)
			{
				const edict_t* const refedict = PROG_TO_EDICT(value->edict);

				if (!refedict->free && refedict != edict)
				{
					const bool owner = strcmp(name, "owner") == 0;
					AddReference(owner ? Incoming : Outgoing, refedict);
				}
			}
			else if (type == ev_string)
			{
				if (IsTargetName(name))
					targetnames.push_back(LS_references_GetString(value->string));
				else if (IsTarget(name))
					targets.push_back(LS_references_GetString(value->string));
				else if (IsKillTarget(name))
					killtargets.push_back(LS_references_GetString(value->string));
			}
		}
	}

	for (int e = 0; e < sv.num_edicts; ++e)
	{
		const edict_t* const probe = EDICT_NUM(e);
		assert(probe);

		if (probe->free || probe == edict)
			continue;

		for (int f = 1; f < progs->numfielddefs; ++f)
		{
			const char* name;
			etype_t type;
			const eval_t* value;

			if (!LS_GetEdictFieldByIndex(probe, f, &name, &type, &value))
				continue;

			if (type == ev_entity && EDICT_TO_PROG(edict) == value->edict)
			{
				const bool owner = strcmp(name, "owner") == 0;
				AddReference(owner ? Outgoing : Incoming, probe);
			}
			else if (type == ev_string)
			{
				if (!targetnames.empty() && (IsTarget(name) || IsKillTarget(name)))
				{
					for (const char* const targetname : targetnames)
					{
						if (LS_references_StringsEqual(targetname, value->string))
						{
							AddReference(Incoming, probe);
							break;
						}
					}
				}
				else if (!targets.empty() && IsTargetName(name))
				{
					for (const char* const target : targets)
					{
						if (LS_references_StringsEqual(target, value->string))
						{
							AddReference(Outgoing, probe);
							break;
						}
					}
				}
				else if (!killtargets.empty() && IsTargetName(name))
				{
					for (const char* const killtarget : killtargets)
					{
						if (LS_references_StringsEqual(killtarget, value->string))
						{
							AddReference(Outgoing, probe);
							break;
						}
					}
				}
			}
		}
	}

	const auto ConvertToTable = [state](const ReferenceSet& references)
	{
		lua_newtable(state);

		lua_Integer counter = 1;

		for (const int reference : references)
		{
			LS_PushEdictValue(state, reference);
			lua_rawseti(state, -2, counter++);
		}
	};

	ConvertToTable(outgoing);
	ConvertToTable(incoming);

	return 2;
}


// Creates 'edicts' table with helper functions for 'edict' values
void LS_InitEdictType(lua_State* state)
{
	constexpr luaL_Reg metatable[] =
	{
		{ "__index", LS_global_edicts_index },
		{ "__len", LS_global_edicts_len },
		{ nullptr, nullptr }
	};

	lua_newtable(state);
	luaL_newmetatable(state, "edicts");
	luaL_setfuncs(state, metatable, 0);
	lua_setmetatable(state, -2);

	constexpr luaL_Reg functions[] =
	{
		{ "destroy", LS_global_edicts_destroy },
		{ "getname", LS_global_edicts_getname },
		{ "isfree", LS_global_edicts_isfree },
		{ "references", LS_global_edicts_references },
		{ "remove", LS_global_edicts_remove },
		{ NULL, NULL }
	};

	luaL_setfuncs(state, functions, 0);
	lua_setglobal(state, "edicts");

	LS_LoadScript(state, "scripts/edicts.lua");
}

#endif // USE_LUA_SCRIPTING
