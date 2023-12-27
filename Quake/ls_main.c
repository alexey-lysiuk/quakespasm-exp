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
#include "tlsf.h"


static lua_State* ls_state;
static size_t ls_quota;
static const char* ls_console_name = "console";

static tlsf_t ls_memory;
static size_t ls_memorysize;

typedef struct
{
	size_t usedbytes;
	size_t usedblocks;
	size_t freebytes;
	size_t freeblocks;
} LS_MemoryStats;


void* LS_CreateTypedUserData(lua_State* state, const LS_UserDataType* type)
{
	int* result = lua_newuserdatauv(state, type->size, 0);
	assert(result);

	*result = type->fourcc;
	result += 1;

	return result;
}

void* LS_GetValueFromTypedUserData(lua_State* state, int index, const LS_UserDataType* type)
{
	luaL_checktype(state, index, LUA_TUSERDATA);

	int* result = lua_touserdata(state, index);
	assert(result);

	if (type->fourcc != *result)
	{
		char expected[5], actual[5];

		memcpy(expected, type, 4);
		expected[4] = '\0';
		memcpy(actual, result, 4);
		actual[4] = '\0';

		luaL_error(state, "Invalid userdata type, expected '%s', got '%s'", expected, actual);
	}

	result += 1;

	return result;
}


//
// Expose 'player' global table with corresponding helper functions
//

static int LS_global_player_setpos(lua_State* state)
{
	vec_t* pos = LS_GetVec3Value(state, 1);
	char anglesstr[64] = { '\0' };

	if (lua_type(state, 2) == LUA_TUSERDATA)
	{
		vec_t* angles = LS_GetVec3Value(state, 2);
		q_snprintf(anglesstr, sizeof anglesstr, " %.02f %.02f %.02f", angles[0], angles[1], angles[2]);
	}

	char command[128];
	q_snprintf(command, sizeof command, "setpos %.02f %.02f %.02f%s;", pos[0], pos[1], pos[2], anglesstr);

	Cbuf_AddText(command);
	return 0;
}

static int LS_global_player_traceentity(lua_State* state)
{
	edict_t* ed = SV_TraceEntity(SV_TRACE_ENTITY_ANY);

	if (ed == NULL)
		lua_pushnil(state);
	else
		LS_PushEdictValue(state, NUM_FOR_EDICT(ed));

	return 1;
}

static int LS_PlayerCheatCommand(lua_State* state, const char* command)
{
	const char* argstr = "";

	if (lua_isnumber(state, 1))
		argstr = lua_tonumber(state, 1) ? " 1" : " 0";
	else if (lua_isboolean(state, 1))
		argstr = lua_toboolean(state, 1) ? " 1" : " 0";

	char cmdbuf[64];
	q_snprintf(cmdbuf, sizeof cmdbuf, "%s%s;", command, argstr);

	Cbuf_AddText(cmdbuf);
	return 0;
}

static int LS_global_player_god(lua_State* state)
{
	return LS_PlayerCheatCommand(state, "god");
}

static int LS_global_player_notarget(lua_State* state)
{
	return LS_PlayerCheatCommand(state, "notarget");
}

static int LS_global_player_noclip(lua_State* state)
{
	return LS_PlayerCheatCommand(state, "noclip");
}


//
// Expose 'sound' global table with related functions
//

static int LS_global_sound_playlocal(lua_State* state)
{
	const char* filename = luaL_checkstring(state, 1);
	assert(filename);

	S_LocalSound(filename);
	return 0;
}


//
// Expose 'host' global table with related functions
//

static int LS_global_host_framecount(lua_State* state)
{
	lua_pushinteger(state, host_framecount);
	return 1;
}

static int LS_global_host_frametime(lua_State* state)
{
	lua_pushnumber(state, host_frametime);
	return 1;
}

static int LS_global_host_realtime(lua_State* state)
{
	lua_pushnumber(state, realtime);
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
}

static int LS_LoadFile(lua_State* state, const char* filename, const char* mode)
{
	if (filename == NULL)
	{
		lua_pushstring(state, "reading from stdin is not supported");
		return LUA_ERRFILE;
	}

	assert(mode != NULL && mode[0] == 't' && mode[1] == '\0');

	int handle;
	int length = COM_OpenFile(filename, &handle, NULL);

	if (handle == -1)
	{
		lua_pushfstring(state, "cannot open Lua script '%s'", filename);
		return LUA_ERRFILE;
	}

	char* script = tlsf_malloc(ls_memory, length);
	assert(script);

	int bytesread = Sys_FileRead(handle, script, length);
	COM_CloseFile(handle);

	int result;

	if (bytesread == length)
		result = luaL_loadbufferx(state, script, length, filename, mode);
	else
	{
		lua_pushfstring(state,
			"error while reading Lua script '%s', read %d bytes instead of %d",
			filename, bytesread, length);
		result = LUA_ERRFILE;
	}

	tlsf_free(ls_memory, script);

	return result;
}

static int LS_global_dofile(lua_State* state)
{
	const char* filename = luaL_optstring(state, 1, NULL);
	lua_settop(state, 1);

	if (LS_LoadFile(state, filename, "t") != LUA_OK)
		return lua_error(state);

	lua_call(state, 0, LUA_MULTRET);
	return lua_gettop(state) - 1;
}

static int LS_global_loadfile(lua_State* state)
{
	const char* filename = luaL_optstring(state, 1, NULL);
	int env = !lua_isnone(state, 3) ? 3 : 0;  // 'env' index or 0 if no 'env'
	int status = LS_LoadFile(state, filename, "t");

	if (status == LUA_OK)
	{
		if (env != 0)
		{
			// 'env' parameter?
			lua_pushvalue(state, env);  // environment for loaded function
			if (!lua_setupvalue(state, -2, 1))  // set it as 1st upvalue
				lua_pop(state, 1);  // remove 'env' if not used by previous call
		}

		return 1;
	}
	else
	{
		// error (message is on top of the stack)
		luaL_pushfail(state);
		lua_insert(state, -2);  // put before error message
		return 2;  // return fail plus error message
	}
}

static int LS_global_print(lua_State* state)
{
	enum PrintDummyEnum { MAX_LENGTH = 4096 };  // See MAXPRINTMSG
	char buf[MAX_LENGTH] = { '\0' };

	char* bufptr = buf;
	int remain = MAX_LENGTH;

	for (int i = 1, n = lua_gettop(state); i <= n; ++i)
	{
		const char* str = luaL_tolstring(state, i, NULL);
		int charscount = q_snprintf(bufptr, remain, "%s%s", (i == 1 ? "" : " "), str);

		bufptr += charscount;
		assert(bufptr <= &buf[MAX_LENGTH - 1]);

		remain -= charscount;
		assert(remain >= 0);

		lua_pop(state, 1);  // pop string value
	}

	Con_SafePrintf("%s\n", buf);

	return 0;
}
static int LS_global_panic(lua_State* state)
{
	const char* message = lua_tostring(state, -1);

	if (!message)
		message = "unknown error";

	Host_Error("%s", message);

	assert(false);
	return 0;
}

static void LS_global_warning(void* ud, const char *msg, int tocont)
{
	(void)ud;
	Con_SafePrintf("%s%s", msg, tocont ? "" : "\n");
}

static void LS_global_hook(lua_State* state, lua_Debug* ar)
{
	luaL_error(state, "infinite loop detected, aborting");
}

void LS_ReportError(lua_State* state)
{
	Con_SafePrintf("Error while executing Lua script\n");

	const char* errormessage = lua_tostring(state, -1);
	if (errormessage)
		Con_SafePrintf("%s\n", errormessage);

	lua_pop(state, 1);  // remove error message
}

static void* LS_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	(void)ud;

	if (nsize == 0)
	{
		if (ptr != NULL)
			tlsf_free(ls_memory, ptr);

		return NULL;
	}
	else
		return tlsf_realloc(ls_memory, ptr, nsize);
}

static void LS_MemoryStatsCollector(void* pointer, size_t size, int isused, void* user)
{
	(void)pointer;

	LS_MemoryStats* stats = user;
	assert(stats);

	if (isused)
	{
		stats->usedbytes += size;
		++stats->usedblocks;
	}
	else
	{
		stats->freebytes += size;
		++stats->freeblocks;
	}
}

static int LS_global_printmemstats(lua_State* state)
{
	(void)state;

	LS_MemoryStats stats;
	memset(&stats, 0, sizeof stats);
	tlsf_walk_pool(tlsf_get_pool(ls_memory), LS_MemoryStatsCollector, &stats);

	size_t totalblocks = stats.usedblocks + stats.freeblocks;
	size_t totalbytes = stats.usedbytes + stats.freebytes + tlsf_size() +
		tlsf_pool_overhead() + tlsf_alloc_overhead() * (totalblocks - 1);

	Con_SafePrintf("Used : %zu bytes, %zu blocks\nFree : %zu bytes, %zu blocks\nTotal: %zu bytes, %zu blocks\n",
		stats.usedbytes, stats.usedblocks, stats.freebytes, stats.freeblocks, totalbytes, totalblocks);

	return 0;
}

static int LS_global_dprint(lua_State* state)
{
	if (developer.value)
	{
		int argcount = lua_gettop(state);

		lua_getglobal(state, "print");
		lua_insert(state, 1);
		lua_call(state, argcount, 0);
	}

	return 0;
}

static int LS_global_text_localize(lua_State* state)
{
	const char* key = luaL_checkstring(state, 1);
	const char* value = LOC_GetString(key);

	lua_pushstring(state, value);
	return 1;
}

static int LS_global_text_tint(lua_State* state)
{
	const char* string = luaL_checkstring(state, 1);
	size_t length = strlen(string);
	char* result = tlsf_malloc(ls_memory, length + 1);

	for (size_t i = 0; i < length; ++i)
	{
		unsigned char ch = string[i];

		if (ch > 0x20 && ch < 0x80)
			ch = ch | 0x80;
		else if (ch > 0xA0)
			ch = ch & ~0x80;

		result[i] = ch;
	}
	result[length] = 0;

	lua_pushstring(state, result);
	tlsf_free(ls_memory, result);

	return 1;
}

static lua_CFunction ls_loadfunc;

// Calls original load() function with mode explicitly set to text
static int LS_global_load(lua_State* state)
{
	int argc = lua_gettop(state);

	switch (argc)
	{
	case 0:
		// don't care about erroneous call
		break;

	case 1:
		lua_pushnil(state);  // add second argument if it's missing
		// fall through

	default:
		lua_pushstring(state, "t");  // text mode only
		if (argc > 2)
			lua_replace(state, 3);  // replace mode if it was set
		break;
	}

	assert(ls_loadfunc != NULL);
	return ls_loadfunc(state);
}

static void LS_InitGlobalFunctions(lua_State* state)
{
	lua_atpanic(state, LS_global_panic);
	lua_setwarnf(state, LS_global_warning, NULL);

	lua_pushglobaltable(state);

	// Save pointer to load() function
	lua_getfield(state, 1, "load");
	ls_loadfunc = lua_tocfunction(state, -1);
	lua_pop(state, 1);  // remove function

	static const luaL_Reg functions[] =
	{
		// Replaced global functions
		{ "dofile", LS_global_dofile },
		{ "load", LS_global_load },
		{ "loadfile", LS_global_loadfile },
		{ "print", LS_global_print },

		// Helper functions
		{ "printmemstats", LS_global_printmemstats },
		{ "dprint", LS_global_dprint },

		{ NULL, NULL }
	};

	luaL_setfuncs(state, functions, 0);
	lua_pop(state, 1);  // remove global table
}

static void LS_InitGlobalTables(lua_State* state)
{
	// Create and register 'player' table
	{
		static const luaL_Reg functions[] =
		{
			{ "god", LS_global_player_god },
			{ "noclip", LS_global_player_noclip },
			{ "notarget", LS_global_player_notarget },
			{ "setpos", LS_global_player_setpos },
			{ "traceentity", LS_global_player_traceentity },
			{ NULL, NULL }
		};

		luaL_newlib(state, functions);
		lua_setglobal(state, "player");
	}

	// Create and register 'sound' table
	{
		static const luaL_Reg functions[] =
		{
			{ "playlocal", LS_global_sound_playlocal },
			{ NULL, NULL }
		};

		luaL_newlib(state, functions);
		lua_setglobal(state, "sound");
	}

	// Create and register 'text' table
	{
		static const luaL_Reg functions[] =
		{
			{ "localize", LS_global_text_localize },
			{ "tint", LS_global_text_tint },
			{ NULL, NULL }
		};

		luaL_newlib(state, functions);
		lua_setglobal(state, "text");
	}

	// Create and register 'host' table
	{
		static const luaL_Reg functions[] =
		{
			{ "framecount", LS_global_host_framecount },
			{ "frametime", LS_global_host_frametime },
			{ "realtime", LS_global_host_realtime },
			{ NULL, NULL }
		};

		luaL_newlib(state, functions);
		lua_setglobal(state, "host");
	}

	// Register namespace for console commands
	lua_createtable(state, 0, 64);
	lua_setglobal(state, ls_console_name);

	LS_InitVec3Type(state);
	LS_InitEdictType(state);
}

void LS_LoadScript(lua_State* state, const char* filename)
{
	lua_pushcfunction(state, LS_global_dofile);
	lua_pushstring(state, filename);

	if (lua_pcall(state, 1, 0, 0) != LUA_OK)
		LS_ReportError(state);
}

static void LS_ResetState(void)
{
	if (ls_state == NULL)
		return;

	lua_close(ls_state);
	ls_state = NULL;

#ifndef NDEBUG
	// check memory pool
	LS_MemoryStats stats;
	memset(&stats, 0, sizeof stats);
	tlsf_walk_pool(tlsf_get_pool(ls_memory), LS_MemoryStatsCollector, &stats);

	assert(stats.usedbytes == 0);
	assert(stats.usedblocks == 0);
	assert(stats.freebytes + tlsf_size() + tlsf_pool_overhead() == ls_memorysize);
	assert(stats.freeblocks == 1);
#endif // !NDEBUG
}

static size_t LS_CheckSizeArgument(const char* argname)
{
	int argindex = COM_CheckParm(argname);
	size_t result = 0;

	if (argindex && argindex < com_argc - 1)
	{
		const char* sizearg = com_argv[argindex + 1];
		assert(sizearg);

		result = Q_atoi(sizearg);
	}

	return result;
}

lua_State* LS_GetState(void)
{
	if (ls_state == NULL)
	{
		if (ls_memory == NULL)
		{
			if (ls_memorysize == 0)
			{
				size_t heapsize = LS_CheckSizeArgument("-luaheapsize");  // in kB
				ls_memorysize = CLAMP(8 * 1024, heapsize, 64 * 1024) * 1024;
			}

			ls_memory = tlsf_create_with_pool(malloc(ls_memorysize), ls_memorysize);
		}

		lua_State* state = lua_newstate(LS_alloc, NULL);
		assert(state);

		lua_gc(state, LUA_GCSTOP);

		LS_InitStandardLibraries(state);
		LS_InitGlobalFunctions(state);
		LS_InitGlobalTables(state);
		LS_InitMenuModule(state);

#ifdef USE_IMGUI
		void LS_InitImGuiModule(lua_State* state);
		LS_InitImGuiModule(state);
#endif // USE_IMGUI

		lua_gc(state, LUA_GCRESTART);
		lua_gc(state, LUA_GCCOLLECT);

		ls_state = state;
	}

	if (ls_quota == 0)
	{
		size_t quota = LS_CheckSizeArgument("-luaexecquota");  // in kilo-ops
		ls_quota = CLAMP(4 * 1024, quota, 64 * 1024) * 1024;
	}

	lua_sethook(ls_state, LS_global_hook, LUA_MASKCOUNT, ls_quota);
	return ls_state;
}

typedef struct
{
	const char* script;
	size_t scriptlength;
	size_t partindex;
} LS_ReaderData;

static const char* LS_global_reader(lua_State* state, void* data, size_t* size)
{
	LS_ReaderData* readerdata = data;
	assert(readerdata);

	static const char prefix[] = "return ";
	static const size_t prefixsize = sizeof prefix - 1;

	static const char suffix[] = ";";
	static const size_t suffixsize = sizeof suffix - 1;

	const char* result;
	size_t resultsize;

	switch (readerdata->partindex)
	{
		case 0:
			result = prefix;
			resultsize = prefixsize;
			break;
		case 1:
			result = readerdata->script;
			resultsize = readerdata->scriptlength;
			break;
		case 2:
			result = suffix;
			resultsize = suffixsize;
			break;
		default:
			result = NULL;
			resultsize = 0;
			break;
	}

	++readerdata->partindex;
	*size = resultsize;

	return result;
}

static void LS_Exec_f(void)
{
	int argc = Cmd_Argc();

	if (argc > 1)
	{
		lua_State* state = LS_GetState();
		assert(state);
		assert(lua_gettop(state) == 0);

		const char* args = Cmd_Args();
		assert(args);

		const char* script = args;
		size_t scriptlength = strlen(args);
		qboolean removequotes = argc == 2 && scriptlength > 2 && args[0] == '"' && args[scriptlength - 1] == '"';

		if (removequotes)
		{
			// Special case of lua CCMD invocation with one argument wrapped with double quotes
			// Skip these quotes, and pass remaining sctring as script code
			scriptlength -= 2;
			script += 1;
		}

		static const char* scriptname = "script";
		static const char* scriptmode = "t";

		LS_ReaderData data = { script, scriptlength, 0 };
		int status = lua_load(state, LS_global_reader, &data, scriptname, scriptmode);

		if (status != LUA_OK)
		{
			lua_pop(state, 1);  // remove error message
			status = luaL_loadbufferx(state, script, scriptlength, scriptname, scriptmode);
		}

		if (status == LUA_OK)
			status = lua_pcall(state, 0, LUA_MULTRET, 0);

		if (status == LUA_OK)
		{
			int resultcount = lua_gettop(state);

			if (resultcount > 0)
			{
				luaL_checkstack(state, LUA_MINSTACK, "too many results to print");

				// Print all results
				lua_pushcfunction(state, LS_global_print);
				lua_insert(state, 1);
				lua_pcall(state, resultcount, 0, 0);

				lua_settop(state, 0);  // clear stack
			}
		}
		else
			LS_ReportError(state);

		assert(lua_gettop(state) == 0);
	}
	else
		Con_SafePrintf("Running %s\n", LUA_RELEASE);
}

void LS_Init(void)
{
	Cmd_AddCommand("lua", LS_Exec_f);
	Cmd_AddCommand("resetluastate", LS_ResetState);
}

void LS_Shutdown(void)
{
	if (ls_state != NULL)
	{
		LS_ShutdownMenuModule(ls_state);
		LS_ResetState();
	}

	free(ls_memory);
	ls_memory = NULL;
}

// Pushes console table if it exists and returns state, otherwise returns null
static lua_State* LS_PushConsoleTable(void)
{
	lua_State* state = LS_GetState();
	assert(state);
	assert(lua_gettop(state) == 0);

	lua_getglobal(state, ls_console_name);

	if (lua_istable(state, 1))
		return state;

	lua_pop(state, 1);  // remove nil
	return NULL;
}

static void LS_PopConsoleTable(lua_State* state)
{
	lua_pop(state, 1);  // remove console namespace table
	assert(lua_gettop(state) == 0);
}

qboolean LS_ConsoleCommand(void)
{
	lua_State* state = LS_PushConsoleTable();
	qboolean result = false;

	if (state != NULL)
	{
		if (lua_getfield(state, -1, Cmd_Argv(0)) == LUA_TFUNCTION)
		{
			int argc = Cmd_Argc();

			if (lua_checkstack(state, argc))
			{
				for (int i = 1; i < argc; ++i)
					lua_pushstring(state, Cmd_Argv(i));

				if (lua_pcall(state, argc - 1, 0, 0) != LUA_OK)
					LS_ReportError(state);
			}
			else
				Con_SafePrintf("Too many arguments (%i) to call Lua script\n", argc - 1);

			result = true;  // command has been processed
		}
		else
			lua_pop(state, 1);  // remove nil

		LS_PopConsoleTable(state);
	}

	return result;
}

const char *LS_GetNextCommand(const char *command)
{
	lua_State* state = LS_PushConsoleTable();
	const char* result = NULL;

	if (state != NULL)
	{
		if (command == NULL)
			lua_pushnil(state);
		else
			lua_pushstring(state, command);

		while (lua_next(state, -2) != 0)
		{
			if (lua_type(state, -1) == LUA_TFUNCTION && lua_type(state, -2) == LUA_TSTRING)
			{
				result = lua_tostring(state, -2);
				assert(result);

				lua_pop(state, 2);  // remove name and value
				break;
			}

			lua_pop(state, 1);  // remove value, keep name for next iteration
		}

		LS_PopConsoleTable(state);
	}

	return result;
}

#endif // USE_LUA_SCRIPTING
