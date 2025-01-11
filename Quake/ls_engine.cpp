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

extern cvar_t gl_polyoffset_factor, gl_polyoffset_units, r_showbboxes, r_fullbright, sv_traceentity;
}


//
// Expose 'host' global table
//

static int LS_global_host_entities(lua_State* state)
{
	if (sv.active && sv.worldmodel)
	{
		lua_pushstring(state, sv.worldmodel->entities);
		return 1;
	}

	return 0;
}

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

static int LS_global_host_gamedir(lua_State* state)
{
	const char* const gamedir = COM_SkipPath(com_gamedir);
	lua_pushstring(state, gamedir);
	return 1;
}

static int LS_global_host_realtime(lua_State* state)
{
	lua_pushnumber(state, realtime);
	return 1;
}

static int LS_global_host_realtimes(lua_State* state)
{
	lua_Number integer = floor(realtime);
	lua_pushnumber(state, integer);

	lua_Number fractional = realtime - integer;
	lua_pushnumber(state, fractional);

	return 2;
}

static void LS_InitHostTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "entities", LS_global_host_entities },
		{ "framecount", LS_global_host_framecount },
		{ "frametime", LS_global_host_frametime },
		{ "gamedir", LS_global_host_gamedir },
		{ "realtime", LS_global_host_realtime },
		{ "realtimes", LS_global_host_realtimes },
		{ NULL, NULL }
	};

	luaL_newlib(state, functions);
	lua_setglobal(state, "host");
}


//
// Expose 'player' global table
//

static int LS_global_player_setpos(lua_State* state)
{
	const LS_Vector3& pos = LS_GetVectorValue<3>(state, 1);
	char anglesstr[64] = { '\0' };

	if (lua_type(state, 2) == LUA_TUSERDATA)
	{
		const LS_Vector3& angles = LS_GetVectorValue<3>(state, 2);
		q_snprintf(anglesstr, sizeof anglesstr, " %.02f %.02f %.02f", angles[0], angles[1], angles[2]);
	}

	char command[128];
	q_snprintf(command, sizeof command, "setpos %.02f %.02f %.02f%s;", pos[0], pos[1], pos[2], anglesstr);

	Cbuf_AddText(command);
	return 0;
}

static int LS_global_player_traceentity(lua_State* state)
{
	// Order of string literals must match SV_TRACE_ENTITY_... definitions
	static const char* const kinds[] = { "solid", "trigger", "any" };
	const int kind = luaL_checkoption(state, 1, "any", kinds) + 1;

	if (const edict_t* const edict = SV_TraceEntity(kind))
	{
		LS_PushEdictValue(state, edict);
		return 1;
	}

	return 0;
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

static int LS_global_player_ghost(lua_State* state)
{
	return LS_PlayerCheatCommand(state, "ghost");
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

static void LS_InitPlayerTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "ghost", LS_global_player_ghost },
		{ "god", LS_global_player_god },
		{ "ingameentitytrace", LS_BoolCVarFunction<sv_traceentity> },
		{ "noclip", LS_global_player_noclip },
		{ "notarget", LS_global_player_notarget },
		{ "setpos", LS_global_player_setpos },
		{ "traceentity", LS_global_player_traceentity },
		{ NULL, NULL }
	};

	luaL_newlib(state, functions);
	lua_setglobal(state, "player");
}


//
// Expose 'render' global table
//

static void LS_InitRenderTable(lua_State* state)
{
	{
		static const luaL_Reg functions[] =
		{
			{ "boundingboxes", LS_BoolCVarFunction<r_showbboxes> },
			{ "fullbright", LS_BoolCVarFunction<r_fullbright> },
			{ nullptr, nullptr }
		};

		luaL_newlib(state, functions);
	}

	{
		static const luaL_Reg functions[] =
		{
			{ "factor", LS_NumberCVarFunction<gl_polyoffset_factor> },
			{ "units", LS_NumberCVarFunction<gl_polyoffset_units> },
			{ nullptr, nullptr }
		};

		luaL_newlib(state, functions);
		lua_setfield(state, -2, "polyoffset");
	}

	lua_setglobal(state, "render");
}


//
// Expose 'sound' value
//

constexpr LS_UserDataType<int> ls_sound_type("sound");

static const sfx_t* LS_GetSoundFromUserData(lua_State* state)
{
	int count;
	const sfx_t* const sounds = LS_GetSounds(&count);

	const int index = ls_sound_type.GetValue(state, 1);
	return (index >= 0 && index < count) ? &sounds[index] : nullptr;
}

static const sfxcache_t* LS_GetCachedSound(const sfx_t& sound)
{
	cache_user_t cacheid = sound.cache;
	void* cacheptr = Cache_Check(&cacheid);
	return static_cast<const sfxcache_t*>(cacheptr);
}

static const sfxcache_t* LS_GetCachedSoundFromUserData(lua_State* state)
{
	const sfx_t* const sound = LS_GetSoundFromUserData(state);
	return sound ? LS_GetCachedSound(*sound) : nullptr;
}

static int LS_value_sound_channelcount(lua_State* state)
{
	if (const sfxcache_t* const cachedsound = LS_GetCachedSoundFromUserData(state))
	{
		lua_pushinteger(state, cachedsound->stereo + 1);
		return 1;
	}

	return 0;
}

static int LS_value_sound_framerate(lua_State* state)
{
	if (const sfxcache_t* const cachedsound = LS_GetCachedSoundFromUserData(state))
	{
		lua_pushinteger(state, cachedsound->speed);
		return 1;
	}

	return 0;
}

static int LS_value_sound_framecount(lua_State* state)
{
	if (const sfxcache_t* const cachedsound = LS_GetCachedSoundFromUserData(state))
	{
		lua_pushinteger(state, cachedsound->length);
		return 1;
	}

	return 0;
}

static int LS_value_sound_loopstart(lua_State* state)
{
	if (const sfxcache_t* const cachedsound = LS_GetCachedSoundFromUserData(state))
	{
		lua_pushinteger(state, cachedsound->loopstart);
		return 1;
	}

	return 0;
}

static int LS_value_sound_name(lua_State* state)
{
	if (const sfx_t* const sound = LS_GetSoundFromUserData(state))
	{
		lua_pushstring(state, sound->name);
		return 1;
	}

	return 0;
}

static int LS_value_sound_samplesize(lua_State* state)
{
	if (const sfxcache_t* const cachedsound = LS_GetCachedSoundFromUserData(state))
	{
		lua_pushinteger(state, cachedsound->width);
		return 1;
	}

	return 0;
}

static int LS_GetSoundSize(const sfxcache_t& cachedsound)
{
	return cachedsound.length * cachedsound.width * (cachedsound.stereo + 1);
}

static int LS_value_sound_size(lua_State* state)
{
	if (const sfxcache_t* const cachedsound = LS_GetCachedSoundFromUserData(state))
	{
		const int soundsize = LS_GetSoundSize(*cachedsound);
		lua_pushinteger(state, soundsize);
		return 1;
	}

	return 0;
}

static int LS_value_sound_tostring(lua_State* state)
{
	const sfx_t* const sound = LS_GetSoundFromUserData(state);

	if (sound)
	{
		if (const sfxcache_t* const cachedsound = LS_GetCachedSound(*sound))
			lua_pushfstring(state, "sound %s (%d bytes)", sound->name, LS_GetSoundSize(*cachedsound));
		else
			lua_pushfstring(state, "sound %s (not loaded)", sound->name);
	}
	else
		lua_pushstring(state, "invalid sound");

	return 1;
}

static int LS_MakeSoundSilent(lua_State* state)
{
	if (const sfxcache_t* const cachedsound = LS_GetCachedSoundFromUserData(state))
	{
		byte* const sounddata = const_cast<sfxcache_t*>(cachedsound)->data;
		const size_t size = LS_GetSoundSize(*cachedsound);
		memset(sounddata, 0, size);

		lua_pushboolean(state, true);
		return 1;
	}

	return 0;
}

static int LS_value_sound_makesilent(lua_State* state)
{
	lua_pushcfunction(state, LS_MakeSoundSilent);
	return 1;
}


//
// Expose 'sounds' global table
//

static int LS_global_sounds_index(lua_State* state)
{
	int count;
	const sfx_t* const sounds = LS_GetSounds(&count);

	if (!sounds || count == 0)
		return 0;

	const int index = luaL_checkinteger(state, 2);

	if (index <= 0 || index > count)
		return 0;

	static const luaL_Reg members[] =
	{
		{ "channelcount", LS_value_sound_channelcount },
		{ "framerate", LS_value_sound_framerate },
		{ "framecount", LS_value_sound_framecount },
		{ "loopstart", LS_value_sound_loopstart },
		{ "makesilent", LS_value_sound_makesilent },
		{ "name", LS_value_sound_name },
		{ "samplesize", LS_value_sound_samplesize },
		{ "size", LS_value_sound_size },
		{ nullptr, nullptr }
	};

	static const luaL_Reg metafuncs[] =
	{
		{ "__tostring", LS_value_sound_tostring },
		{ nullptr, nullptr }
	};

	ls_sound_type.New(state, members, metafuncs) = index - 1;  // on C side, indices start with 0
	return 1;
}

static int LS_global_sounds_len(lua_State* state)
{
	int count;
	LS_GetSounds(&count);

	lua_pushinteger(state, count);
	return 1;
}

static int LS_global_sounds_playlocal(lua_State* state)
{
	const char* filename = luaL_checkstring(state, 1);
	assert(filename);

	S_LocalSound(filename);
	return 0;
}

static int LS_global_sounds_stopall(lua_State* state)
{
	S_StopAllSounds(true);
	return 0;
}

static void LS_InitSoundsTable(lua_State* state)
{
	constexpr luaL_Reg metatable[] =
	{
		{ "__index", LS_global_sounds_index },
		{ "__len", LS_global_sounds_len },
		{ nullptr, nullptr }
	};

	lua_newtable(state);
	luaL_newmetatable(state, "sounds");
	luaL_setfuncs(state, metatable, 0);
	lua_setmetatable(state, -2);

	static const luaL_Reg functions[] =
	{
		{ "playlocal", LS_global_sounds_playlocal },
		{ "stopall", LS_global_sounds_stopall },
		{ NULL, NULL }
	};

	luaL_setfuncs(state, functions, 0);
	lua_setglobal(state, "sounds");
}


//
// Expose 'text' global table
//

static int LS_global_text_localize(lua_State* state)
{
	const char* key = luaL_checkstring(state, 1);
	const char* value = LOC_GetRawString(key);

	if (value)
		lua_pushstring(state, value);
	else
		lua_pushvalue(state, 1);  // Push key if localization wasn't found

	return 1;
}

static int LS_global_text_tint(lua_State* state)
{
	const char* string = luaL_checkstring(state, 1);
	size_t length = strlen(string);
	char* result = LS_tempalloc(state, length + 1);

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
	LS_tempfree(result);

	return 1;
}

static int LS_global_text_toascii(lua_State* state)
{
	// Convert argument to function
	lua_getglobal(state, "tostring");
	lua_insert(state, 1);  // swap argument and function
	lua_call(state, 1, 1);

	const char* string = luaL_checkstring(state, 1);
	size_t buffersize = strlen(string) + 1;
	char* result = LS_tempalloc(state, buffersize);

	q_strtoascii(string, result, buffersize);
	lua_pushstring(state, result);
	LS_tempfree(result);

	return 1;
}

static void LS_InitTextTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "localize", LS_global_text_localize },
		{ "tint", LS_global_text_tint },
		{ "toascii", LS_global_text_toascii },
		{ NULL, NULL }
	};

	luaL_newlib(state, functions);
	lua_setglobal(state, "text");
}


//
// Expose 'textures' global table
//

// Pushes texture table by the given texture
static void LS_PushTextureTable(lua_State* state, const gltexture_t* const texture)
{
	assert(texture);

	lua_newtable(state);
	lua_pushstring(state, "name");
	lua_pushstring(state, texture->name);
	lua_rawset(state, -3);
	lua_pushstring(state, "width");
	lua_pushinteger(state, texture->width);
	lua_rawset(state, -3);
	lua_pushstring(state, "height");
	lua_pushinteger(state, texture->height);
	lua_rawset(state, -3);
	lua_pushstring(state, "texnum");
	lua_pushinteger(state, texture->texnum);
	lua_rawset(state, -3);
}

// Pushes sequence of tables with information about loaded textures
static int LS_global_textures_list(lua_State* state)
{
	const gltexture_t* texture = LS_GetTextures();
	if (!texture)
		return 0;

	int count = 1;

	lua_newtable(state);

	for (; texture; ++count)
	{
		LS_PushTextureTable(state, texture);

		// Set texture table as sequence value
		lua_rawseti(state, -2, count);

		texture = texture->next;
	}

	// Texture created last appears at the first position in sequence of texture tables
	// Reverse its order so every addition/deletion of texture doesn't shift the sequence
	for (int i = 1, m = count / 2; i < m; ++i)
	{
		lua_rawgeti(state, -1, i);
		lua_rawgeti(state, -2, count - i);

		lua_rawseti(state, -3, i);
		lua_rawseti(state, -2, count - i);
	}

	return 1;
}

// Pushes texture table by its name
static int LS_global_textures_index(lua_State* state)
{
	const char* const name = luaL_checkstring(state, 2);

	for (const gltexture_t* texture = LS_GetTextures(); texture; texture = texture->next)
	{
		if (strcmp(texture->name, name) == 0)
		{
			LS_PushTextureTable(state, texture);
			return 1;
		}
	}

	return 0;
}

static void LS_InitTexturesTable(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "list", LS_global_textures_list },
		{ nullptr, nullptr }
	};

	luaL_newlib(state, functions);

	if (luaL_newmetatable(state, "textures"))
	{
		static const luaL_Reg functions[] =
		{
			{ "__index", LS_global_textures_index },
			{ nullptr, nullptr }
		};

		luaL_setfuncs(state, functions, 0);
	}

	lua_setmetatable(state, -2);
	lua_setglobal(state, "textures");
}


void LS_InitEngineTables(lua_State* state)
{
	LS_InitHostTable(state);
	LS_InitPlayerTable(state);
	LS_InitRenderTable(state);
	LS_InitSoundsTable(state);
	LS_InitTextTable(state);
	LS_InitTexturesTable(state);

	LS_LoadScript(state, "scripts/engine.lua");
}

#endif // USE_LUA_SCRIPTING
