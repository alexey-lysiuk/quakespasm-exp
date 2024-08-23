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
#include <vector>

#include "ls_common.h"
#include "ls_progs_builtins.h"

extern "C"
{
#include "quakedef.h"

int LS_GetKnownStringCount();
const ddef_t* LS_GetProgsFieldDefinitionByIndex(int index);
const ddef_t* LS_GetProgsFieldDefinitionByOffset(int offset);
const ddef_t* LS_GetProgsGlobalDefinitionByOffset(int offset);
const ddef_t* LS_GetProgsGlobalDefinitionByIndex(int index);
const char* LS_GetProgsOpName(unsigned short op);
const char* LS_GetProgsString(int offset);
const char* LS_GetProgsTypeName(unsigned short type);
}

void LS_PushEdictFieldValue(lua_State* state, etype_t type, const eval_t* value);


// Pushes member value by calling a function with given name from index table
static int LS_CallIndexTableMember(lua_State* state)
{
	const int functableindex = lua_upvalueindex(1);
	luaL_checktype(state, functableindex, LUA_TTABLE);

	const char* name = luaL_checkstring(state, 2);
	const int type = lua_getfield(state, functableindex, name);

	if (type == LUA_TNIL)
		return 0;

	lua_rotate(state, 1, 2);  // move 'self' to argument position
	lua_call(state, 1, 1);

	return 1;
}

// Creates index table, and assigns __index metamethod that calls functions from this table to make member values
static void LS_SetIndexTable(lua_State* state, const luaL_Reg* const functions)
{
	assert(lua_type(state, -1) == LUA_TTABLE);

	lua_newtable(state);
	luaL_setfuncs(state, functions, 0);

	// Set member values returning functions as upvalue for __index metamethod
	lua_pushcclosure(state, LS_CallIndexTableMember, 1);
	lua_setfield(state, -2, "__index");
}


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
	if (luaL_newmetatable(state, "funcparam"))
	{
		lua_pushcfunction(state, LS_value_functionparameter_tostring);
		lua_setfield(state, -2, "__tostring");

		// Create table with functions to return member values
		static const luaL_Reg functions[] =
		{
			{ "name", LS_value_functionparameter_name },
			{ "type", LS_value_functionparameter_type },
			{ nullptr, nullptr }
		};

		LS_SetIndexTable(state, functions);
	}

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

static void LS_GetFunctionForParameters(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);
	lua_getfield(state, 1, "function");
	lua_copy(state, 3, 1);
	lua_settop(state, 1);
}

// Pushes 'function parameter' userdata by the given numerical index, [1..function->numparms]
static int LS_progs_functionparameters_index(lua_State* state)
{
	const int index = luaL_checkinteger(state, 2) - 1;

	LS_GetFunctionForParameters(state);

	if (dfunction_t* function = LS_GetFunctionFromUserData(state))
	{
		if (index < 0 || index >= function->numparms)
			return 0;

		LS_FunctionParameter& parameter = ls_functionparameter_type.New(state);
		LS_GetFunctionParameter(function, index, parameter);
		LS_SetFunctionParameterMetaTable(state);
	}
	else
		luaL_error(state, "invalid function");

	return 1;
}

// Pushes number of function parameters
static int LS_progs_functionparameters_len(lua_State* state)
{
	LS_GetFunctionForParameters(state);

	if (dfunction_t* function = LS_GetFunctionFromUserData(state))
		lua_pushinteger(state, function->numparms);
	else
		luaL_error(state, "invalid function");

	return 1;
}

// Pushes table of progs function parameters
static int LS_PushFunctionParameters(lua_State* state, const dfunction_t* function)
{
	assert(function);

	lua_newtable(state);

	if (luaL_newmetatable(state, "function parameters"))
	{
		static const luaL_Reg functions[] =
		{
			{ "__index", LS_progs_functionparameters_index },
			{ "__len", LS_progs_functionparameters_len },
			{ nullptr, nullptr }
		};

		luaL_setfuncs(state, functions, 0);
	}

	lua_setmetatable(state, -2);

	// Set self as value for 'function' key
	lua_pushvalue(state, 1);
	lua_setfield(state, 2, "function");

	return 1;
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
	if (luaL_newmetatable(state, "func"))
	{
		lua_pushcfunction(state, LS_FunctionMember<LS_PushFunctionToString>);
		lua_setfield(state, -2, "__tostring");

		// Create table with functions to return member values
		static const luaL_Reg functions[] =
		{
			{ "disassemble", LS_FunctionMethod<LS_PushFunctionDisassemble> },
			{ "file", LS_FunctionMember<LS_PushFunctionFile> },
			{ "name", LS_FunctionMember<LS_PushFunctionName> },
			{ "parameters", LS_FunctionMember<LS_PushFunctionParameters> },
			{ "returntype", LS_FunctionMember<LS_PushFunctionReturnType> },
			{ nullptr, nullptr }
		};

		LS_SetIndexTable(state, functions);
	}

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
	const lua_Integer count = progs == nullptr ? 0 :
		progs->numfunctions - 1;  // without error function at index zero

	lua_pushinteger(state, count);
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
	if (luaL_newmetatable(state, "fielddef"))
	{
		lua_pushcfunction(state, LS_FieldDefinitionMember<LS_PushFieldDefinitionToString>);
		lua_setfield(state, -2, "__tostring");

		static const luaL_Reg functions[] =
		{
			{ "name", LS_FieldDefinitionMember<LS_PushDefinitionName> },
			{ "offset", LS_FieldDefinitionMember<LS_PushDefinitionOffset> },
			{ "type", LS_FieldDefinitionMember<LS_PushDefinitionType> },
			{ nullptr, nullptr }
		};

		LS_SetIndexTable(state, functions);
	}

	lua_setmetatable(state, -2);
}

// Pushes 'field definitions' userdata by the given numerical index, [1..progs->numfielddefs]
static int LS_progs_fielddefinitions_index(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	const int index = luaL_checkinteger(state, 2);

	if (index <= 0 || index >= progs->numfielddefs)
		return 0;

	ls_fielddefinition_type.New(state) = index;
	LS_SetFieldDefinitionMetaTable(state);

	return 1;
}

// Pushes number of progs field definitions
static int LS_progs_fielddefinitions_len(lua_State* state)
{
	const lua_Integer count = progs == nullptr ? 0 :
		progs->numfielddefs - 1;  // without unnamed void field at index zero

	lua_pushinteger(state, count);
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
	if (luaL_newmetatable(state, "globaldef"))
	{
		lua_pushcfunction(state, LS_GlobalDefinitionMember<LS_PushGlobalDefinitionToString>);
		lua_setfield(state, -2, "__tostring");

		static const luaL_Reg functions[] =
		{
			{ "name", LS_GlobalDefinitionMember<LS_PushDefinitionName> },
			{ "offset", LS_GlobalDefinitionMember<LS_PushDefinitionOffset> },
			{ "type", LS_GlobalDefinitionMember<LS_PushDefinitionType> },
			{ nullptr, nullptr }
		};

		LS_SetIndexTable(state, functions);
	}

	lua_setmetatable(state, -2);
}

// Pushes 'global definitions' userdata by the given numerical index, [1..progs->numglobaldefs]
static int LS_progs_globaldefinitions_index(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	const int index = luaL_checkinteger(state, 2);

	if (index <= 0 || index >= progs->numglobaldefs)
		return 0;

	ls_globaldefinition_type.New(state) = index;
	LS_SetGlobalDefinitionMetaTable(state);

	return 1;
}

// Pushes number of progs global definitions
static int LS_progs_globaldefinitions_len(lua_State* state)
{
	const lua_Integer count = progs == nullptr ? 0 :
		progs->numglobaldefs - 1;  // without unnamed void field at index zero

	lua_pushinteger(state, count);
	return 1;
}


//
// Global variables
//

// Pushes floating point value of global variable by its index, [1..progs->numglobals)
static int LS_progs_globalvariables_index(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	const int index = luaL_checkinteger(state, 2);

	if (index <= 0 || index >= progs->numglobals)
		return 0;

	assert(pr_globals);

	lua_pushnumber(state, pr_globals[index]);
	return 1;
}

// Pushes number of progs global variables
static int LS_progs_globalvariables_len(lua_State* state)
{
	const lua_Integer count = progs == nullptr ? 0 :
		progs->numglobals - 1;  // without null value at index zero (OFS_NULL)

	lua_pushinteger(state, count);
	return 1;
}

// Pushes integer value of global variable by its index, [1..progs->numglobals)
static int LS_progs_globalvariables_integer(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	const int index = luaL_checkinteger(state, 1);

	if (index <= 0 || index >= progs->numglobals)
		return 0;

	assert(pr_globals);
	const int* integerglobals = reinterpret_cast<const int*>(pr_globals);

	lua_pushinteger(state, integerglobals[index]);
	return 1;
}


//
// Strings
//

class LS_StringCache
{
public:
	const char* Get(size_t index, int* length, int* offset)
	{
		Update();

		if (offsets.empty() || index == 0)
			return nullptr;

		--index;  // on Lua side, indices start with one

		const size_t progscount = offsets.size() - 1;  // without offset to end of the last progs string
		const size_t knowncount = LS_GetKnownStringCount() - 1;  // without first empty string

		if (index < progscount)
		{
			const int progsoffset = offsets[index];

			if (length)
				*length = offsets[index + 1] - progsoffset - 1;

			if (offset)
				*offset = progsoffset;

			return &strings[progsoffset];
		}
		else
			index -= progscount;  // switch to known strings

		if (index < knowncount)
		{
			const int knownoffset = -int(index) - 2;  // known strings indices start with -1, skip first empty string as well
			const char* const string = LS_GetProgsString(knownoffset);

			if (length)
				*length = string ? strlen(string) : 0;

			if (offset)
				*offset = knownoffset;

			return string ? string : "";
		}

		return nullptr;  // index out of range, index >= progscount + knowncount
	}

	size_t Count()
	{
		Update();

		if (offsets.empty())
			return 0;

		return offsets.size() - 1  // without offset to end of the last string
			+ LS_GetKnownStringCount() - 1;  // without first empty string
	}

	void Reset()
	{
		OffsetList empty;
		offsets.swap(empty);

		strings = nullptr;
		endoffset = 0;
		crc = 0;
	}

private:
	using OffsetList = std::vector<int, LS_TempAllocator<int>>;
	OffsetList offsets;

	const char* strings = nullptr;
	int endoffset = 0;
	unsigned short crc = 0;

	void Update()
	{
		if (progs == nullptr)
		{
			Reset();
			return;
		}

		if (endoffset == progs->numstrings && crc == pr_crc)
			return;

		offsets.clear();

		strings = LS_GetProgsString(0);
		endoffset = progs->numstrings;
		crc = pr_crc;

		for (int offset = 0; offset < endoffset; ++offset)
		{
			if (strings[offset] == '\0')
				offsets.push_back(offset + 1);
		}
	}
};

static LS_StringCache ls_stringcache;

// Pushes string by the given numerical index starting with 1
static int LS_progs_strings_index(lua_State* state)
{
	const int index = luaL_checkinteger(state, 2);

	int length;
	const char* const string = ls_stringcache.Get(index, &length, nullptr);

	if (string == nullptr)
		return 0;

	lua_pushlstring(state, string, length);
	return 1;
}

// Pushes number of progs strings
static int LS_progs_strings_len(lua_State* state)
{
	const lua_Integer count = ls_stringcache.Count();
	lua_pushinteger(state, count);
	return 1;
}

// Pushes offset of progs strings by its index
static int LS_progs_strings_offset(lua_State* state)
{
	const int index = luaL_checkinteger(state, 1);

	int offset;
	const char* const string = ls_stringcache.Get(index, nullptr, &offset);

	lua_pushinteger(state, string ? offset : 0);
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
	constexpr luaL_Reg functions[] =
	{
		{ "__index", LS_progs_functions_index },
		{ "__len", LS_progs_functions_len },
		{ nullptr, nullptr }
	};

	lua_newtable(state);
	luaL_newmetatable(state, "functions");
	luaL_setfuncs(state, functions, 0);
	lua_setmetatable(state, -2);

	return 1;
}

// Returns table of progs field definitions
static int LS_global_progs_fielddefinitions(lua_State* state)
{
	constexpr luaL_Reg functions[] =
	{
		{ "__index", LS_progs_fielddefinitions_index },
		{ "__len", LS_progs_fielddefinitions_len },
		{ nullptr, nullptr }
	};

	lua_newtable(state);
	luaL_newmetatable(state, "field definitions");
	luaL_setfuncs(state, functions, 0);
	lua_setmetatable(state, -2);

	return 1;
}

// Returns table of progs global definitions
static int LS_global_progs_globaldefinitions(lua_State* state)
{
	constexpr luaL_Reg functions[] =
	{
		{ "__index", LS_progs_globaldefinitions_index },
		{ "__len", LS_progs_globaldefinitions_len },
		{ nullptr, nullptr }
	};

	lua_newtable(state);
	luaL_newmetatable(state, "global definitions");
	luaL_setfuncs(state, functions, 0);
	lua_setmetatable(state, -2);

	return 1;
}

// Returns table of progs global variables
static int LS_global_progs_globalvariables(lua_State* state)
{
	constexpr luaL_Reg functions[] =
	{
		{ "__index", LS_progs_globalvariables_index },
		{ "__len", LS_progs_globalvariables_len },
		{ nullptr, nullptr }
	};

	lua_newtable(state);
	luaL_newmetatable(state, "global variables");
	luaL_setfuncs(state, functions, 0);
	lua_setmetatable(state, -2);

	lua_pushcfunction(state, LS_progs_globalvariables_integer);
	lua_setfield(state, -2, "integer");
	return 1;
}

// Pushes name of type by its index
static int LS_global_progs_typename(lua_State* state)
{
	const lua_Integer typeindex = luaL_checkinteger(state, 1);
	const char* const nameoftype = LS_GetProgsTypeName(typeindex);
	lua_pushstring(state, nameoftype);
	return 1;
}

// Pushes table of progs strings
static int LS_global_progs_strings(lua_State* state)
{
	constexpr luaL_Reg functions[] =
	{
		{ "__index", LS_progs_strings_index },
		{ "__len", LS_progs_strings_len },
		{ nullptr, nullptr }
	};

	lua_newtable(state);
	luaL_newmetatable(state, "strings");
	luaL_setfuncs(state, functions, 0);
	lua_setmetatable(state, -2);

	lua_pushcfunction(state, LS_progs_strings_offset);
	lua_setfield(state, -2, "offset");
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
	lua_newtable(state);

	// Metatable
	{
		constexpr luaL_Reg functions[] =
		{
			{ "crc", LS_global_progs_crc },
			{ "datcrc", LS_global_progs_datcrc },
			{ "version", LS_global_progs_version },
			{ nullptr, nullptr }
		};

		luaL_newmetatable(state, "progs");
		LS_SetIndexTable(state, functions);
		lua_setmetatable(state, -2);
	}

	// Fields
	{
		constexpr luaL_Reg fields[] =
		{
			{ "fielddefinitions", LS_global_progs_fielddefinitions },
			{ "functions", LS_global_progs_functions },
			{ "globaldefinitions", LS_global_progs_globaldefinitions },
			{ "globalvariables", LS_global_progs_globalvariables },
			{ "strings", LS_global_progs_strings },
		};

		for (const luaL_Reg& field : fields)
		{
			field.func(state);
			lua_setfield(state, -2, field.name);
		}
	}

	// Functions
	{
		constexpr luaL_Reg functions[] =
		{
			{ "typename", LS_global_progs_typename },
			{ nullptr, nullptr }
		};

		luaL_setfuncs(state, functions, 0);
	}

	lua_setglobal(state, "progs");

	LS_LoadScript(state, "scripts/progs.lua");
}

void LS_ResetProgsType()
{
	ls_stringcache.Reset();
}

#endif // USE_LUA_SCRIPTING
