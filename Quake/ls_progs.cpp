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
		static const etype_t BUILTIN_RETURN_TYPES[] =
		{
			ev_bad,
			ev_void,     // void(entity e) makevectors = #1
			ev_void,     // void(entity e, vector o) setorigin = #2
			ev_void,     // void(entity e, string m) setmodel = #3
			ev_void,     // void(entity e, vector min, vector max) setsize = #4
			ev_void,     // void(entity e, vector min, vector max) setabssize = #5
			ev_void,     // void() break = #6
			ev_float,    // float() random = #7
			ev_void,     // void(entity e, float chan, string samp) sound = #8
			ev_vector,   // vector(vector v) normalize = #9
			ev_void,     // void(string e) error = #10
			ev_void,     // void(string e) objerror = #11
			ev_float,    // float(vector v) vlen = #12
			ev_float,    // float(vector v) vectoyaw = #13
			ev_entity,   // entity() spawn = #14
			ev_void,     // void(entity e) remove = #15
			ev_float,    // float(vector v1, vector v2, float tryents) traceline = #16
			ev_entity,   // entity() clientlist = #17
			ev_entity,   // entity(entity start, .string fld, string match) find = #18
			ev_void,     // void(string s) precache_sound = #19
			ev_void,     // void(string s) precache_model = #20
			ev_void,     // void(entity client, string s)stuffcmd = #21
			ev_entity,   // entity(vector org, float rad) findradius = #22
			ev_void,     // void(string s) bprint = #23
			ev_void,     // void(entity client, string s) sprint = #24
			ev_void,     // void(string s) dprint = #25
			ev_void,     // void(string s) ftos = #26
			ev_void,     // void(string s) vtos = #27
			ev_void,     // void() coredump = #28
			ev_void,     // void() traceon = #29
			ev_void,     // void() traceoff = #30
			ev_void,     // void(entity e) eprint = #31
			ev_float,    // float(float yaw, float dist) walkmove = #32
			ev_bad,      // #33 was removed
			ev_float,    // float(float yaw, float dist) droptofloor = #34
			ev_void,     // void(float style, string value) lightstyle = #35
			ev_float,    // float(float v) rint = #36
			ev_float,    // float(float v) floor = #37
			ev_float,    // float(float v) ceil = #38
			ev_bad,      // #39 was removed
			ev_float,    // float(entity e) checkbottom = #40
			ev_float,    // float(vector v) pointcontents = #41
			ev_bad,      // #42 was removed
			ev_float,    // float(float f) fabs = #43
			ev_vector,   // vector(entity e, float speed) aim = #44
			ev_float,    // float(string s) cvar = #45
			ev_void,     // void(string s) localcmd = #46
			ev_entity,   // entity(entity e) nextent = #47
			ev_void,     // void(vector o, vector d, float color, float count) particle = #48
			ev_void,     // void() ChangeYaw = #49
			ev_bad,      // #50 was removed
			ev_vector,   // vector(vector v) vectoangles = #51
			ev_void,     // void(float to, float f) WriteByte = #52
			ev_void,     // void(float to, float f) WriteChar = #53
			ev_void,     // void(float to, float f) WriteShort = #54
			ev_void,     // void(float to, float f) WriteLong = #55
			ev_void,     // void(float to, float f) WriteCoord = #56
			ev_void,     // void(float to, float f) WriteAngle = #57
			ev_void,     // void(float to, string s) WriteString = #58
			ev_void,     // void(float to, entity s) WriteEntity = #59
			ev_bad,      // #60 was removed
			ev_bad,      // #61 was removed
			ev_bad,      // #62 was removed
			ev_bad,      // #63 was removed
			ev_bad,      // #64 was removed
			ev_bad,      // #65 was removed
			ev_bad,      // #66 was removed
			ev_void,     // void(float step) movetogoal = #67
			ev_string,   // string(string s) precache_file = #68
			ev_void,     // void(entity e) makestatic = #69
			ev_void,     // void(string s) changelevel = #70
			ev_bad,      // #71 was removed
			ev_void,     // void(string var, string val) cvar_set = #72
			ev_void,     // void(entity client, string s) centerprint = #73
			ev_void,     // void(vector pos, string samp, float vol, float atten) ambientsound = #74
			ev_string,   // string(string s) precache_model2 = #75
			ev_string,   // string(string s) precache_sound2 = #76
			ev_string,   // string(string s) precache_file2 = #77
			ev_void,     // void(entity e) setspawnparms = #78

			// 2021 re-release, see PR_PatchRereleaseBuiltins()
			ev_float,    // float() finaleFinished = #79
			ev_void,     // void localsound (entity client, string sample) = #80
			ev_void,     // void draw_point (vector point, float colormap, float lifetime, float depthtest) = #81
			ev_void,     // void draw_line (vector start, vector end, float colormap, float lifetime, float depthtest) = #82
			ev_void,     // void draw_arrow (vector start, vector end, float colormap, float size, float lifetime, float depthtest) = #83
			ev_void,     // void draw_ray (vector start, vector direction, float length, float colormap, float size, float lifetime, float depthtest) = #84
			ev_void,     // void draw_circle (vector origin, float radius, float colormap, float lifetime, float depthtest) = #85
			ev_void,     // void draw_bounds (vector min, vector max, float colormap, float lifetime, float depthtest) = #86
			ev_void,     // void draw_worldtext (string s, vector origin, float size, float lifetime, float depthtest) = #87
			ev_void,     // void draw_sphere (vector origin, float radius, float colormap, float lifetime, float depthtest) = #88
			ev_void,     // void draw_cylinder (vector origin, float halfHeight, float radius, float colormap, float lifetime, float depthtest) = #89
			ev_float,    // float CheckPlayerEXFlags( entity playerEnt ) = #90
			ev_float,    // float walkpathtogoal( float movedist, vector goal ) = #91
			ev_float,    // float bot_movetopoint( entity bot, vector point ) = #92
			// same # as above, float bot_followentity( entity bot, entity goal ) = #92
		};

		const size_t builtin = -first_statement;

		switch (builtin)
		{
		case 0:
			// TODO: add return type detection by name for re-release progs
			returntype = ev_bad;
			break;

		case 99:
			returntype = ev_float;  // float checkextension( string s ) = #99
			break;

		case 401:
			returntype = ev_void;  // void setcolor( entity client, float color ) = #401
			break;

		default:
			returntype = (builtin < Q_COUNTOF(BUILTIN_RETURN_TYPES)) ? BUILTIN_RETURN_TYPES[builtin] : ev_bad;
			break;
		}
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
	const lua_Integer returntypeindex = LS_GetFunctionReturnType(function);
	const char* const returntype = PR_GetTypeString(returntypeindex);
	const char* name = PR_SafeGetString(function->s_name);

	luaL_Buffer buf;
	luaL_buffinit(state, &buf);
	luaL_addstring(&buf, returntype);
	luaL_addchar(&buf, ' ');
	luaL_addstring(&buf, name);
	luaL_addchar(&buf, '(');

	struct Parameter
	{
		int name;
		unsigned short type;
	};
	Parameter parameters[MAX_PARMS];

	const int first_statement = function->first_statement;
	const int numparms = q_min(function->numparms, MAX_PARMS);

	for (int i = 0; i < numparms; ++i)
	{
		Parameter param;

		if (first_statement > 0)
		{
			const ddef_t* def = ED_GlobalAtOfs(function->parm_start + i);
			param.name = def ? def->s_name : 0;
			param.type = def ? def->type : (function->parm_size[i] > 1 ? ev_vector : ev_bad);
		}
		else
		{
			param.name = 0 /* no name */;
			param.type = function->parm_size[i] > 1 ? ev_vector : ev_bad;
		}

		parameters[i] = param;
	}

	for (int i = 0; i < numparms; ++i)
	{
		if (i > 0)
			luaL_addstring(&buf, ", ");

		const char* type = PR_GetTypeString(parameters[i].type);
		luaL_addstring(&buf, type);

		const char* name = PR_SafeGetString(parameters[i].name);
		if (name[0] != '\0')
		{
			luaL_addchar(&buf, ' ');
			luaL_addstring(&buf, name);
		}
	}

	luaL_addchar(&buf, ')');
	luaL_pushresult(&buf);
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
