/*
Copyright (C) 2023-2025 Alexey Lysiuk

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
#include "ls_vector.h"

extern "C"
{
#include "quakedef.h"

const sfx_t* LS_GetSounds(int* count);
const gltexture_t* LS_GetTextures();

extern cvar_t gl_polyoffset_factor, gl_polyoffset_units, r_showbboxes, sv_traceentity;
}

#ifdef USE_TLSF
#include "tlsf/tlsf.h"
#endif // USE_TLSF


int LS_global_expversion(lua_State* state);

static lua_State* ls_state;
static size_t ls_quota;

struct LS_MemoryStats
{
	size_t usedbytes;
	size_t usedblocks;
	size_t freebytes;
	size_t freeblocks;
};


#ifdef USE_TLSF

static tlsf_t ls_memory;
static size_t ls_memorysize;

char* LS_tempalloc(lua_State* state, size_t size)
{
	void* result = tlsf_malloc(ls_memory, size);

	if (!result)
		luaL_error(state ? state : ls_state, "unable to allocate %I bytes", size);

	return static_cast<char*>(result);
}

void LS_tempfree(void* ptr)
{
	tlsf_free(ls_memory, ptr);
}

#else // !USE_TLSF

char* LS_tempalloc(lua_State* state, size_t size)
{
	return static_cast<char*>(malloc(size));
}

void LS_tempfree(void* ptr)
{
	free(ptr);
}

#endif // USE_TLSF


void* LS_TypelessUserDataType::NewPtr(lua_State* state) const
{
	const LS_TypelessUserDataType** result = static_cast<const LS_TypelessUserDataType**>(lua_newuserdatauv(state, size, 0));
	assert(result);

	*result = this;
	result += 1;

	return result;
}

void* LS_TypelessUserDataType::GetValuePtr(lua_State* state, int index) const
{
	luaL_checktype(state, index, LUA_TUSERDATA);

	const LS_TypelessUserDataType** result = static_cast<const LS_TypelessUserDataType**>(lua_touserdata(state, index));
	assert(result && *result);

	if (*result != this)
		luaL_error(state, "invalid userdata type, expected '%s', got '%s'", name, (*result)->name);

	result += 1;

	return result;
}

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
void LS_SetIndexTable(lua_State* state, const luaL_Reg* const functions)
{
	assert(functions);
	assert(lua_type(state, -1) == LUA_TTABLE);

	lua_newtable(state);
	luaL_setfuncs(state, functions, 0);

	// Set member values returning functions as upvalue for __index metamethod
	lua_pushcclosure(state, LS_CallIndexTableMember, 1);
	lua_setfield(state, -2, "__index");
}

// Creates metatable and index table, and assigns __index metamethod that calls functions from this table to make member values
void LS_TypelessUserDataType::SetMetaTable(lua_State* state, const luaL_Reg* members, const luaL_Reg* metafuncs) const
{
	assert(lua_gettop(state) > 0);

	if (luaL_newmetatable(state, name))
	{
		if (members)
			LS_SetIndexTable(state, members);

		if (metafuncs)
		{
			for (const luaL_Reg* metafunc = metafuncs; metafunc->name; ++metafunc)
			{
				lua_pushcfunction(state, metafunc->func);
				lua_setfield(state, -2, metafunc->name);
			}
		}
	}

	lua_setmetatable(state, -2);
}


int LS_BoolCVarFunction(lua_State* state, cvar_t& cvar)
{
	if (lua_gettop(state) >= 1)
	{
		const int value = lua_toboolean(state, 1);
		Cvar_SetValueQuick(&cvar, static_cast<float>(value));
		return 0;
	}

	lua_pushboolean(state, static_cast<int>(cvar.value));
	return 1;
}

int LS_NumberCVarFunction(lua_State* state, cvar_t& cvar)
{
	if (lua_gettop(state) >= 1)
	{
		const float value = luaL_checknumber(state, 1);
		Cvar_SetValueQuick(&cvar, value);
		return 0;
	}

	lua_pushnumber(state, cvar.value);
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

	// Load original 'os' library without global name creations,
	// collect pointers to safe functions, and expose them as 'os' library
	luaL_requiref(state, "os", luaopen_os, 0);

	luaL_Reg functions[] =
	{
		{ "clock", NULL },
		{ "date", NULL },
		{ "difftime", NULL },
		{ "time", NULL },
		{ NULL, NULL }
	};

	for (luaL_Reg* f = functions; f->name != NULL; ++f)
	{
		lua_pushstring(state, f->name);
		lua_rawget(state, -2);
		f->func = lua_tocfunction(state, -1);
		assert(f->func);
		lua_pop(state, 1);  // remove function
	}

	luaL_newlib(state, functions);
	lua_setglobal(state, "os");

	lua_pop(state, 1);  // remove original 'os' library
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

	char* script = nullptr;
	int bytesread = 0;

	if (length > 0)
	{
		script = LS_tempalloc(state, length);
		bytesread = Sys_FileRead(handle, script, length);
	}

	COM_CloseFile(handle);

	int result;

	if (bytesread == length)
		result = luaL_loadbufferx(state, script ? script : "", length, filename, mode);
	else
	{
		lua_pushfstring(state,
			"error while reading Lua script '%s', read %d bytes instead of %d",
			filename, bytesread, length);
		result = LUA_ERRFILE;
	}

	if (script)
		LS_tempfree(script);

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
	constexpr size_t BUFFER_LENGTH = 1023;
	char buffer[BUFFER_LENGTH + 1];  // additional char for null terminator
	buffer[BUFFER_LENGTH] = '\0';

	for (int i = 1, n = lua_gettop(state); i <= n; ++i)
	{
		size_t length;
		const char* str = luaL_tolstring(state, i, &length);
		assert(str);

		if (i > 1)
			Con_SafePrintf(" ");

		if (length > 0)
		{
			for (size_t ci = 0, ce = length / BUFFER_LENGTH; ci < ce; ++ci)
			{
				memcpy(buffer, &str[BUFFER_LENGTH * ci], BUFFER_LENGTH);
				Con_SafePrintf("%s", buffer);
			}

			const size_t charsleft = length % BUFFER_LENGTH;

			if (charsleft > 0)
			{
				memcpy(buffer, &str[length - charsleft], charsleft);
				buffer[charsleft] = '\0';
				Con_SafePrintf("%s", buffer);
			}
		}

		lua_pop(state, 1);  // pop string value
	}

	Con_SafePrintf("\n");

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

static int LS_global_stacktrace(lua_State* state)
{
	const char* message = NULL;
	const int top = lua_gettop(state);

	if (top > 0)
		message = lua_tostring(state, 1);

	luaL_traceback(state, state, message, 1);

	size_t length = 0;
	const char* traceback = lua_tolstring(state, top + 1, &length);

	assert(traceback);
	assert(length > 0);

	char* cleaned = LS_tempalloc(state, length);
	assert(cleaned);

	qboolean skipnextquote = false;
	size_t d = 0;

	for (size_t s = 0; s < length;)
	{
		char ch = traceback[s];

		if (ch == '[' && strncmp(&traceback[s], "[string \"", 9) == 0)
		{
			s += 9;
			skipnextquote = true;
			continue;
		}

		if (ch == '\t')
			ch = ' ';
		else if (skipnextquote && ch == '"' && traceback[s + 1] == ']')
		{
			s += 2;
			skipnextquote = false;
			continue;
		}

		cleaned[d] = ch;
		++s;
		++d;
	}

	lua_pushlstring(state, cleaned, d);
	LS_tempfree(cleaned);

	return 1;
}

int LS_ErrorHandler(lua_State* state)
{
	LS_global_stacktrace(state);
	Con_SafePrintf("%s\n", lua_tostring(state, -1));
	return 0;
}

#ifdef USE_TLSF

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

	LS_MemoryStats* stats = static_cast<LS_MemoryStats*>(user);
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

static int LS_global_memstats(lua_State* state)
{
	LS_MemoryStats stats;
	memset(&stats, 0, sizeof stats);
	tlsf_walk_pool(tlsf_get_pool(ls_memory), LS_MemoryStatsCollector, &stats);

	size_t totalblocks = stats.usedblocks + stats.freeblocks;
	size_t totalbytes = stats.usedbytes + stats.freebytes + tlsf_size() +
		tlsf_pool_overhead() + tlsf_alloc_overhead() * (totalblocks - 1);

	char buffer[1024];
	int length = q_snprintf(buffer, sizeof buffer, "Used : %zu bytes, %zu blocks\nFree : %zu bytes, %zu blocks\nTotal: %zu bytes, %zu blocks",
		stats.usedbytes, stats.usedblocks, stats.freebytes, stats.freeblocks, totalbytes, totalblocks);
	assert(length > 0);

	lua_pushlstring(state, buffer, length);
	return 1;
}

#else // !USE_TLSF

static int LS_global_memstats(lua_State* state)
{
	const int used = lua_gc(state, LUA_GCCOUNT) * 1024 + lua_gc(state, LUA_GCCOUNTB);
	lua_pushfstring(state, "Used: %d bytes", used);
	return 1;
}

#endif // USE_TLSF

static int LS_global_crc16(lua_State* state)
{
	size_t length;
	const char* const data = luaL_checklstring(state, 1, &length);

	const unsigned int short crc = CRC_Block(reinterpret_cast<const byte*>(data), length);
	lua_pushinteger(state, crc);
	return 1;
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
		{ "crc16", LS_global_crc16 },
		{ "dprint", LS_global_dprint },
		{ "expversion", LS_global_expversion },
		{ "memstats", LS_global_memstats },
		{ "stacktrace", LS_global_stacktrace },

		{ NULL, NULL }
	};

	luaL_setfuncs(state, functions, 0);
	lua_pop(state, 1);  // remove global table
}

static void LS_InitGlobalTables(lua_State* state)
{
	LS_InitEngineTables(state);
	LS_InitVectorType(state);
	LS_InitProgsType(state);
	LS_InitEdictType(state);

#ifndef NDEBUG
	LS_LoadScript(state, "scripts/debug.lua");
#endif // !NDEBUG

#ifdef USE_IMGUI
	void LS_InitExpMode(lua_State* state);
	LS_InitExpMode(state);
#endif // USE_IMGUI
}

void LS_LoadScript(lua_State* state, const char* filename)
{
	lua_pushcfunction(state, LS_ErrorHandler);
	lua_pushcfunction(state, LS_global_dofile);
	lua_pushstring(state, filename);

	if (lua_pcall(state, 1, 0, 1) != LUA_OK)
		lua_pop(state, 1);  // remove nil returned by error handler

	lua_pop(state, 1);  // remove error handler
}

static void LS_ResetState(void)
{
	if (ls_state == NULL)
		return;

	LS_ResetProgsType();

	lua_close(ls_state);
	ls_state = NULL;

#if defined USE_TLSF && !defined NDEBUG
	// check memory pool
	LS_MemoryStats stats;
	memset(&stats, 0, sizeof stats);
	tlsf_walk_pool(tlsf_get_pool(ls_memory), LS_MemoryStatsCollector, &stats);

	assert(stats.usedbytes == 0);
	assert(stats.usedblocks == 0);
	assert(stats.freebytes + tlsf_size() + tlsf_pool_overhead() == ls_memorysize);
	assert(stats.freeblocks == 1);
#endif // USE_TLSF && !NDEBUG
}

static int LS_CheckSizeArgument(const char* argname)
{
	int argindex = COM_CheckParm(argname);
	int result = 0;

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
#ifdef USE_TLSF
		if (ls_memory == NULL)
		{
			if (ls_memorysize == 0)
			{
				const int heapsize = LS_CheckSizeArgument("-luaheapsize");  // in kB
				ls_memorysize = CLAMP(1 * 1024, heapsize == 0 ? 16 * 1024 : heapsize, 256 * 1024) * 1024;
			}

			ls_memory = tlsf_create_with_pool(malloc(ls_memorysize), ls_memorysize);
		}

		lua_State* state = lua_newstate(LS_alloc, NULL);
#else
		lua_State* state = luaL_newstate();
#endif // USE_TLSF

		assert(state);

		lua_gc(state, LUA_GCSTOP);

		LS_InitStandardLibraries(state);
		LS_InitGlobalFunctions(state);
		LS_InitGlobalTables(state);

		lua_gc(state, LUA_GCRESTART);
		lua_gc(state, LUA_GCCOLLECT);

		ls_state = state;
	}

	if (ls_quota == 0)
	{
		const int quota = LS_CheckSizeArgument("-luaexecquota");  // in kilo-ops
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
	LS_ReaderData* readerdata = static_cast<LS_ReaderData*>(data);
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

		lua_pushcfunction(state, LS_ErrorHandler);

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

		static const char* scriptname = "console";
		static const char* scriptmode = "t";

		LS_ReaderData data = { script, scriptlength, 0 };
		int status = lua_load(state, LS_global_reader, &data, scriptname, scriptmode);

		if (status != LUA_OK)
		{
			lua_pop(state, 1);  // remove error message
			status = luaL_loadbufferx(state, script, scriptlength, scriptname, scriptmode);
		}

		if (status == LUA_OK)
		{
			status = lua_pcall(state, 0, LUA_MULTRET, 1);

			if (status == LUA_OK)
			{
				int resultcount = lua_gettop(state) - 1;  // exluding error handler

				if (resultcount > 0)
				{
					luaL_checkstack(state, LUA_MINSTACK, "too many results to print");

					// Print all results
					lua_pushcfunction(state, LS_global_print);
					lua_insert(state, 2);
					lua_pcall(state, resultcount, 0, 0);
				}
			}
			else
				lua_pop(state, 1);  // remove nil returned by error handler

			lua_pop(state, 1);  // remove error handler
		}
		else
			lua_pcall(state, 1, 0, 0);  // call error handler directly

		assert(lua_gettop(state) == 0);
	}
	else
		Con_SafePrintf("Running %s\n", LUA_RELEASE);
}

extern "C"
{

void LS_Init(void)
{
	Cmd_AddCommand("lua", LS_Exec_f);
	Cmd_AddCommand("resetlua", LS_ResetState);

	Cbuf_AddText("exec scripts/aliases/common.cfg\n");
}

void LS_Shutdown(void)
{
	if (ls_state != NULL)
		LS_ResetState();

#ifdef USE_TLSF
	free(ls_memory);
	ls_memory = NULL;
#endif // USE_TLSF
}

void LS_CleanStack()
{
	if (ls_state)
		lua_settop(ls_state, 0);
}

} // extern "C"

#endif // USE_LUA_SCRIPTING
