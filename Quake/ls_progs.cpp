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

const ddef_t* LS_GetProgsFieldDefinitionByIndex(int index);
const ddef_t* LS_GetProgsFieldDefinitionByOffset(int offset);
const ddef_t* LS_GetProgsGlobalDefinitionByOffset(int offset);
const ddef_t* LS_GetProgsGlobalDefinitionByIndex(int index);
const char* LS_GetProgsOpName(unsigned short op);
const char* LS_GetProgsString(int offset);
const char* LS_GetProgsTypeName(unsigned short type);
}

void LS_PushEdictFieldValue(lua_State* state, etype_t type, const eval_t* value);


static void LS_GlobalStringToBuffer(const int offset, luaL_Buffer& buffer, const bool withcontent = true)
{
	lua_State* state = buffer.L;
	size_t length;

	lua_pushinteger(state, offset);
	lua_tolstring(state, -1, &length);
	luaL_addvalue(&buffer);

	const ddef_t* definition = LS_GetProgsGlobalDefinitionByOffset(offset);

	if (definition)
	{
		const char* const name = LS_GetProgsString(definition->s_name);
		const size_t namelength = strlen(name);

		luaL_addchar(&buffer, '(');
		luaL_addlstring(&buffer, name, namelength);
		luaL_addchar(&buffer, ')');

		length += namelength + 2;  // with two round brackets

		if (withcontent)
		{
			if (offset >= progs->numglobals)
				luaL_error(state, "invalid global offset %d", offset);

			const eval_t* value = reinterpret_cast<const eval_t*>(&pr_globals[offset]);
			const int type = definition->type & ~DEF_SAVEGLOBAL;
			size_t valuelength;

			LS_PushEdictFieldValue(state, etype_t(type), value);
			lua_tolstring(state, -1, &valuelength);
			luaL_addvalue(&buffer);

			length += valuelength;
		}
	}
	else
	{
		luaL_addlstring(&buffer, "(?)", 3);
		length += 3;
	}

	do
	{
		luaL_addchar(&buffer, ' ');
		++length;
	}
	while (length < 20);
}


//
// Statements
//

static void LS_StatementToBuffer(const dstatement_t& statement, luaL_Buffer& buffer, const bool withbinary)
{
	if (withbinary)
	{
		char binbuf[32];
		const size_t binlen = q_snprintf(binbuf, sizeof binbuf, "%3u %5i %5i %5i  ",
			statement.op, statement.a, statement.b, statement.c);
		luaL_addlstring(&buffer, binbuf, binlen);
	}

	const char* const op = LS_GetProgsOpName(statement.op);
	const size_t oplength = strlen(op);

	luaL_addlstring(&buffer, op, oplength);
	luaL_addchar(&buffer, ' ');

	for (size_t i = oplength; i < 10; ++i)
		luaL_addchar(&buffer, ' ');

	switch (statement.op)
	{
	case OP_IF:
	case OP_IFNOT:
		LS_GlobalStringToBuffer(statement.a, buffer);
		luaL_addstring(&buffer, "branch ");

		lua_pushinteger(buffer.L, statement.b);
		luaL_addvalue(&buffer);
		break;

	case OP_GOTO:
		luaL_addstring(&buffer, "branch ");

		lua_pushinteger(buffer.L, statement.a);
		luaL_addvalue(&buffer);
		break;

	case OP_STORE_F:
	case OP_STORE_V:
	case OP_STORE_S:
	case OP_STORE_ENT:
	case OP_STORE_FLD:
	case OP_STORE_FNC:
		LS_GlobalStringToBuffer(statement.a, buffer);
		LS_GlobalStringToBuffer(statement.b, buffer, false);
		break;

	default:
		if (statement.a)
			LS_GlobalStringToBuffer(statement.a, buffer);
		if (statement.b)
			LS_GlobalStringToBuffer(statement.b, buffer);
		if (statement.c)
			LS_GlobalStringToBuffer(statement.c, buffer, false);
		break;
	}
}


//
// Function parameters
//

struct LS_FunctionParameter
{
	int name;
	int type;
};

// Assigns name and type indices to function parameter by its index
static void LS_GetFunctionParameter(const dfunction_t* const function, const int paramindex, LS_FunctionParameter& parameter)
{
	assert(function);

	if (function->first_statement > 0)
	{
		const ddef_t* def = LS_GetProgsGlobalDefinitionByOffset(function->parm_start + paramindex);
		parameter.name = def ? def->s_name : 0;
		parameter.type = def ? def->type : (function->parm_size[paramindex] > 1 ? ev_vector : ev_bad);
	}
	else
	{
		parameter.name = 0; // TODO: set parameter name of built-in function
		parameter.type = function->parm_size[paramindex] > 1 ? ev_vector : ev_bad;
	}
}

// Fills array of function parameters
static int LS_GetFunctionParameters(const dfunction_t* const function, LS_FunctionParameter parameters[MAX_PARMS])
{
	assert(function);

	const int numparms = q_min(function->numparms, MAX_PARMS);

	for (int i = 0; i < numparms; ++i)
		LS_GetFunctionParameter(function, i, parameters[i]);

	return numparms;
}

// Adds function parameter type and name separated by space (if it's set) to given Lua buffer
static void LS_FunctionParameterToBuffer(const LS_FunctionParameter& parameter, luaL_Buffer& buffer)
{
	const char* const type = LS_GetProgsTypeName(parameter.type);
	luaL_addstring(&buffer, type);

	const char* const name = LS_GetProgsString(parameter.name);
	if (name[0] != '\0')
	{
		luaL_addchar(&buffer, ' ');
		luaL_addstring(&buffer, name);
	}
}

// Adds function parameters if any to given Lua buffer
static void LS_FunctionParametersToBuffer(const dfunction_t* const function, luaL_Buffer& buffer)
{
	LS_FunctionParameter parameters[MAX_PARMS];
	const int numparms = LS_GetFunctionParameters(function, parameters);

	for (int i = 0; i < numparms; ++i)
	{
		if (i > 0)
			luaL_addstring(&buffer, ", ");

		LS_FunctionParameterToBuffer(parameters[i], buffer);
	}
}

constexpr LS_UserDataType<LS_FunctionParameter> ls_functionparameter_type("function parameter");

// Pushes name of given 'function parameter' userdata
static int LS_value_functionparameter_name(lua_State* state)
{
	const LS_FunctionParameter& parameter = ls_functionparameter_type.GetValue(state, 1);
	const char* const name = LS_GetProgsString(parameter.name);

	lua_pushstring(state, name);
	return 1;
}

// Pushes type index of given 'function parameter' userdata
static int LS_value_functionparameter_type(lua_State* state)
{
	const LS_FunctionParameter& parameter = ls_functionparameter_type.GetValue(state, 1);
	lua_pushinteger(state, parameter.type);
	return 1;
}

constexpr LS_Member ls_functionparameter_members[] =
{
	{ "name", LS_value_functionparameter_name },
	{ "type", LS_value_functionparameter_type },
};

// Pushes method of 'function parameter' userdata by its name
static int LS_value_functionparameter_index(lua_State* state)
{
	return LS_GetMember(state, ls_functionparameter_type, ls_functionparameter_members, Q_COUNTOF(ls_functionparameter_members));
}

// Pushes string representation of given 'function parameter' userdata
static int LS_value_functionparameter_tostring(lua_State* state)
{
	const LS_FunctionParameter& parameter = ls_functionparameter_type.GetValue(state, 1);

	const char* const type = LS_GetProgsTypeName(parameter.type);
	const char* name = LS_GetProgsString(parameter.name);

	if (name[0] == '\0')
		lua_pushfstring(state, "unnamed function parameter of type '%s'", type);
	else
		lua_pushfstring(state, "function parameter '%s' of type '%s'", name, type);

	return 1;
}

// Sets metatable for 'function parameter' userdata
static void LS_SetFunctionParameterMetaTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_value_functionparameter_index },
		{ "__tostring", LS_value_functionparameter_tostring },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "funcparam"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
}


//
// Functions
//

constexpr LS_UserDataType<int> ls_function_type("function");

// Gets pointer to dfunction_t from 'function' userdata
static dfunction_t* LS_GetFunctionFromUserData(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	const int index = ls_function_type.GetValue(state, 1);
	return (index > 0 && index < progs->numfunctions) ? &pr_functions[index] : nullptr;
}

static int LS_GetFunctionMemberValue(lua_State* state, int (*getter)(lua_State* state, const dfunction_t* function))
{
	if (dfunction_t* function = LS_GetFunctionFromUserData(state))
		return getter(state, function);
	else
		luaL_error(state, "invalid function");

	return 0;
}

template <int (*Func)(lua_State* state, const dfunction_t* function)>
static int LS_FunctionMember(lua_State* state)
{
	return LS_GetFunctionMemberValue(state, Func);
}

template <int (*Func)(lua_State* state, const dfunction_t* function)>
static int LS_FunctionMethod(lua_State* state)
{
	lua_pushcfunction(state, LS_FunctionMember<Func>);
	return 1;
}

// Pushes file of 'function' userdata
static int LS_PushFunctionFile(lua_State* state, const dfunction_t* function)
{
	lua_pushstring(state, LS_GetProgsString(function->s_file));
	return 1;
}

// Pushes name of 'function' userdata
static int LS_PushFunctionName(lua_State* state, const dfunction_t* function)
{
	lua_pushstring(state, LS_GetProgsString(function->s_name));
	return 1;
}

static int LS_global_functionparameters_iterator(lua_State* state)
{
	const dfunction_t* function = LS_GetFunctionFromUserData(state);
	const lua_Integer index = luaL_checkinteger(state, 2);

	if (index < function->numparms)
	{
		lua_pushinteger(state, index + 1);

		LS_FunctionParameter& parameter = ls_functionparameter_type.New(state);
		LS_GetFunctionParameter(function, index, parameter);
		LS_SetFunctionParameterMetaTable(state);
		return 2;
	}

	lua_pushnil(state);
	return 1;
}

// Pushes function parameters iterator, e.g., for i, p in func:parameters() do print(i, p) end
static int LS_PushFunctionParameters(lua_State* state, const dfunction_t* function)
{
	const lua_Integer index = luaL_optinteger(state, 2, 0);
	lua_pushcfunction(state, LS_global_functionparameters_iterator);

	int& funcindex = ls_function_type.New(state);
	funcindex = function - pr_functions;
	// No need to set metatable for 'function' userdata as its sole purpose is to get function index

	lua_pushinteger(state, index);  // initial value
	return 3;
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
				const ddef_t* def = LS_GetProgsGlobalDefinitionByOffset(statement->a);
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
static int LS_PushFunctionReturnType(lua_State* state, const dfunction_t* function)
{
	const lua_Integer returntype = LS_GetFunctionReturnType(function);
	lua_pushinteger(state, returntype);

	return 1;
}

// Pushes disassembly of 'function' userdata as a string
static int LS_PushFunctionDisassemble(lua_State* state, const dfunction_t* function)
{
	const bool withbinary = luaL_opt(state, lua_toboolean, 2, false);

	luaL_Buffer buffer;
	luaL_buffinitsize(state, &buffer, 4096);

	const lua_Integer returntype = LS_GetFunctionReturnType(function);
	const char* const returntypename = LS_GetProgsTypeName(returntype);
	luaL_addstring(&buffer, returntypename);
	luaL_addchar(&buffer, ' ');

	const char* const name = LS_GetProgsString(function->s_name);
	luaL_addstring(&buffer, name);
	luaL_addchar(&buffer, '(');
	LS_FunctionParametersToBuffer(function, buffer);
	luaL_addchar(&buffer, ')');

	const int fileindex = function->s_file;
	if (fileindex == 0)
		luaL_addstring(&buffer, ":\n");
	else
	{
		const char* const file = LS_GetProgsString(fileindex);
		luaL_addstring(&buffer, ": // ");
		luaL_addstring(&buffer, file);
		luaL_addchar(&buffer, '\n');
	}

	// Code disassembly
	const int first_statement = function->first_statement;

	if (first_statement > 0)
	{
		char addrbuf[16];

		for (int i = first_statement, ie = progs->numstatements; i < ie; ++i)
		{
			const size_t addrlen = q_snprintf(addrbuf, sizeof(addrbuf), "%06i: ", i);
			luaL_addlstring(&buffer, addrbuf, addrlen);

			const dstatement_t& statement = pr_statements[i];
			LS_StatementToBuffer(statement, buffer, withbinary);

			if (statement.op == OP_DONE)
				break;
			else
				luaL_addchar(&buffer, '\n');
		}
	}
	else
		luaL_addstring(&buffer, "<built-in>");

	luaL_pushresult(&buffer);
	return 1;
}

constexpr LS_Member ls_function_members[] =
{
	{ "file", LS_FunctionMember<LS_PushFunctionFile> },
	{ "name", LS_FunctionMember<LS_PushFunctionName> },
	{ "parameters", LS_FunctionMethod<LS_PushFunctionParameters> },
	{ "returntype", LS_FunctionMember<LS_PushFunctionReturnType> },
	{ "disassemble", LS_FunctionMethod<LS_PushFunctionDisassemble> },
};

// Pushes method of 'function' userdata by its name
static int LS_value_function_index(lua_State* state)
{
	return LS_GetMember(state, ls_function_type, ls_function_members, Q_COUNTOF(ls_function_members));
}

// Pushes string representation of given 'function' userdata
static int LS_PushFunctionToString(lua_State* state, const dfunction_t* function)
{
	const lua_Integer returntypeindex = LS_GetFunctionReturnType(function);
	const char* const returntype = LS_GetProgsTypeName(returntypeindex);
	const char* name = LS_GetProgsString(function->s_name);

	luaL_Buffer buf;
	luaL_buffinitsize(state, &buf, 256);
	luaL_addstring(&buf, returntype);
	luaL_addchar(&buf, ' ');
	luaL_addstring(&buf, name);
	luaL_addchar(&buf, '(');
	LS_FunctionParametersToBuffer(function, buf);
	luaL_addchar(&buf, ')');
	luaL_pushresult(&buf);

	return 1;
}

// Sets metatable for 'function' userdata
static void LS_SetFunctionMetaTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_value_function_index },
		{ "__tostring", LS_FunctionMember<LS_PushFunctionToString> },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "func"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
}

static int LS_progs_functions_index(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	int indextype = lua_type(state, 2);
	int index = 0;  // error function at index zero is not valid function

	if (indextype == LUA_TSTRING)
	{
		const char* name = lua_tostring(state, 2);

		for (int i = 1; i < progs->numfunctions; ++i)
		{
			if (strcmp(LS_GetProgsString(pr_functions[i].s_name), name) == 0)
			{
				index = i;
				break;
			}
		}
	}
	else if (indextype == LUA_TNUMBER)
		index = lua_tointeger(state, 2);
	else
		luaL_error(state, "invalid type '%s' of function index, need number or string", lua_typename(state, indextype));

	// Check function index, [1..progs->numfunctions], for validity
	if (index > 0 && index < progs->numfunctions)
	{
		ls_function_type.New(state) = index;
		LS_SetFunctionMetaTable(state);
		return 1;
	}

	return 0;
}

static int LS_progs_functions_len(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	lua_pushinteger(state, progs->numfunctions - 1);  // without error function at index zero
	return 1;
}


//
// Field and global definitions
//

static int LS_PushDefinitionName(lua_State* state, const ddef_t* definition)
{
	const char* const name = LS_GetProgsString(definition->s_name);
	lua_pushstring(state, name);
	return 1;
}

static int LS_PushDefinitionType(lua_State* state, const ddef_t* definition)
{
	lua_pushinteger(state, definition->type & ~DEF_SAVEGLOBAL);
	return 1;
}

static int LS_PushDefinitionOffset(lua_State* state, const ddef_t* definition)
{
	lua_pushinteger(state, definition->ofs);
	return 1;
}

constexpr LS_UserDataType<int> ls_fielddefinition_type("field definition");

static int LS_GetFieldDefinitionMemberValue(lua_State* state, int (*getter)(lua_State* state, const ddef_t* definition))
{
	const int index = ls_fielddefinition_type.GetValue(state, 1);
	const ddef_t* definition = LS_GetProgsFieldDefinitionByIndex(index);

	if (definition)
		return getter(state, definition);
	else
		luaL_error(state, "invalid field definition");

	return 0;
}

template <int (*Func)(lua_State* state, const ddef_t* definition)>
static int LS_FieldDefinitionMember(lua_State* state)
{
	return LS_GetFieldDefinitionMemberValue(state, Func);
}

constexpr LS_Member ls_fielddefinition_members[] =
{
	{ "name", LS_FieldDefinitionMember<LS_PushDefinitionName> },
	{ "type", LS_FieldDefinitionMember<LS_PushDefinitionType> },
	{ "offset", LS_FieldDefinitionMember<LS_PushDefinitionOffset> },
};

// Pushes value by member name of given 'field definition' userdata
static int LS_value_fielddefinition_index(lua_State* state)
{
	return LS_GetMember(state, ls_fielddefinition_type, ls_fielddefinition_members, Q_COUNTOF(ls_fielddefinition_members));
}

// Pushes string representation of given field definition
static int LS_PushFieldDefinitionToString(lua_State* state, const ddef_t* definition)
{
	const char* const name = LS_GetProgsString(definition->s_name);
	const char* const type = LS_GetProgsTypeName(definition->type);
	lua_pushfstring(state, "field definition '%s' of type '%s' at offset %d", name, type, definition->ofs);
	return 1;
}

// Sets metatable for 'field definition' userdata
static void LS_SetFieldDefinitionMetaTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_value_fielddefinition_index },
		{ "__tostring", LS_FieldDefinitionMember<LS_PushFieldDefinitionToString> },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "fielddef"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
}

static int LS_global_fielddefinitions_iterator(lua_State* state)
{
	lua_Integer index = luaL_checkinteger(state, 2);
	index = luaL_intop(+, index, 1);

	if (index > 0 && index < progs->numfielddefs)
	{
		lua_pushinteger(state, index);

		int& newvalue = ls_fielddefinition_type.New(state);
		newvalue = index;
		LS_SetFieldDefinitionMetaTable(state);

		return 2;
	}

	lua_pushnil(state);
	return 1;
}

constexpr LS_UserDataType<int> ls_globaldefinition_type("global definition");

static int LS_GetGlobalDefinitionMemberValue(lua_State* state, int (*getter)(lua_State* state, const ddef_t* definition))
{
	const int index = ls_globaldefinition_type.GetValue(state, 1);
	const ddef_t* definition = LS_GetProgsGlobalDefinitionByIndex(index);

	if (definition)
		return getter(state, definition);
	else
		luaL_error(state, "invalid global definition");

	return 0;
}

template <int (*Func)(lua_State* state, const ddef_t* definition)>
static int LS_GlobalDefinitionMember(lua_State* state)
{
	return LS_GetGlobalDefinitionMemberValue(state, Func);
}

constexpr LS_Member ls_globaldefinition_members[] =
{
	{ "name", LS_GlobalDefinitionMember<LS_PushDefinitionName> },
	{ "type", LS_GlobalDefinitionMember<LS_PushDefinitionType> },
	{ "offset", LS_GlobalDefinitionMember<LS_PushDefinitionOffset> },
};

// Pushes value by member name of given 'global definition' userdata
static int LS_value_globaldefinition_index(lua_State* state)
{
	return LS_GetMember(state, ls_globaldefinition_type, ls_globaldefinition_members, Q_COUNTOF(ls_globaldefinition_members));
}

// Pushes string representation of given global definition
static int LS_PushGlobalDefinitionToString(lua_State* state, const ddef_t* definition)
{
	const char* const name = LS_GetProgsString(definition->s_name);
	const char* const type = LS_GetProgsTypeName(definition->type);
	lua_pushfstring(state, "global definition '%s' of type '%s' at offset %d", name, type, definition->ofs);
	return 1;
}

// Sets metatable for 'global definition' userdata
static void LS_SetGlobalDefinitionMetaTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_value_globaldefinition_index },
		{ "__tostring", LS_GlobalDefinitionMember<LS_PushGlobalDefinitionToString> },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "globaldef"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
}

static int LS_global_globaldefinitions_iterator(lua_State* state)
{
	lua_Integer index = luaL_checkinteger(state, 2);
	index = luaL_intop(+, index, 1);

	if (index > 0 && index < progs->numglobaldefs)
	{
		lua_pushinteger(state, index);

		int& newvalue = ls_globaldefinition_type.New(state);
		newvalue = index;
		LS_SetGlobalDefinitionMetaTable(state);

		return 2;
	}

	lua_pushnil(state);
	return 1;
}


//
// Global 'progs' table
//

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

// Returns table of progs functions
static int LS_global_progs_functions(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	lua_newtable(state);
	lua_pushvalue(state, -1);  // copy for lua_setmetatable()

	if (luaL_newmetatable(state, "functions"))
	{
		static const luaL_Reg functions[] =
		{
			{ "__index", LS_progs_functions_index },
			{ "__len", LS_progs_functions_len },
			{ nullptr, nullptr }
		};

		luaL_setfuncs(state, functions, 0);
	}

	lua_setmetatable(state, -2);

	return 1;
}

// Returns progs field definitions iterator, e.g., for i, f in progs.fielddefinitions() do print(i, f) end
static int LS_global_progs_fielddefinitions(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	const lua_Integer index = luaL_optinteger(state, 1, 0);
	lua_pushcfunction(state, LS_global_fielddefinitions_iterator);
	lua_pushnil(state);  // unused
	lua_pushinteger(state, index);  // initial value
	return 3;
}

// Returns progs global definitions iterator, e.g., for i, g in progs.globaldefinitions() do print(i, g) end
static int LS_global_progs_globaldefinitions(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	const lua_Integer index = luaL_optinteger(state, 1, 0);
	lua_pushcfunction(state, LS_global_globaldefinitions_iterator);
	lua_pushnil(state);  // unused
	lua_pushinteger(state, index);  // initial value
	return 3;
}

// Pushes name of type by its index
static int LS_global_progs_typename(lua_State* state)
{
	const lua_Integer typeindex = luaL_checkinteger(state, 1);
	const char* const nameoftype = LS_GetProgsTypeName(typeindex);
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
		{ "fielddefinitions", LS_global_progs_fielddefinitions },
		{ "functions", LS_global_progs_functions },
		{ "globaldefinitions", LS_global_progs_globaldefinitions },
		{ "typename", LS_global_progs_typename },
		{ "version", LS_global_progs_version },
		{ nullptr, nullptr }
	};

	luaL_newlib(state, progs_functions);
	lua_setglobal(state, "progs");

	LS_LoadScript(state, "scripts/progs.lua");
}

#endif // USE_LUA_SCRIPTING
