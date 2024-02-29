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

#pragma once

#ifdef USE_LUA_SCRIPTING

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

lua_State* LS_GetState(void);

// Default message handler for lua_pcall() and xpcall()
int LS_ErrorHandler(lua_State* state);

void LS_LoadScript(lua_State* state, const char* filename);

typedef struct
{
	union
	{
		struct { char ch[4]; };
		int fourcc;
	};
	size_t size;
} LS_UserDataType;

void* LS_CreateTypedUserData(lua_State* state, const LS_UserDataType* type);
void* LS_GetValueFromTypedUserData(lua_State* state, int index, const LS_UserDataType* type);

int LS_GetVectorComponent(lua_State* state, int index, int componentcount);

typedef float vec_t;
void LS_InitVec3Type(lua_State* state);
void LS_PushVec3Value(lua_State* state, const vec_t* value);
vec_t* LS_GetVec3Value(lua_State* state, int index);

void LS_InitEdictType(lua_State* state);
void LS_PushEdictValue(lua_State* state, int edictindex);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // USE_LUA_SCRIPTING
