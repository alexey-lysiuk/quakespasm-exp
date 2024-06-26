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

#include <cstdint>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
} // extern "C"

lua_State* LS_GetState(void);

// Default message handler for lua_pcall() and xpcall()
int LS_ErrorHandler(lua_State* state);

void LS_LoadScript(lua_State* state, const char* filename);

class LS_TypelessUserDataType
{
public:
	constexpr LS_TypelessUserDataType(const char (&fourcc)[5], const size_t size)
	: fourcc((fourcc[0] << 24) | (fourcc[1] << 16) | (fourcc[2] << 8) | fourcc[3])
	, size(uint32_t(size))
	{
	}

	void* NewPtr(lua_State* state) const;
	void* GetValuePtr(lua_State* state, int index) const;

protected:
	uint32_t fourcc;
	uint32_t size;
};

template <typename T>
class LS_UserDataType : public LS_TypelessUserDataType
{
public:
	constexpr explicit LS_UserDataType(const char (&fourcc)[5])
	: LS_TypelessUserDataType(fourcc, sizeof(fourcc) + sizeof(T))
	{
	}

	T& New(lua_State* const state) const
	{
		return *static_cast<T*>(NewPtr(state));
	}

	T& GetValue(lua_State* const state, const int index) const
	{
		return *static_cast<T*>(GetValuePtr(state, index));
	}
};

int LS_GetVectorComponent(lua_State* state, int index, int componentcount);

typedef float vec_t;
void LS_InitVec3Type(lua_State* state);
void LS_PushVec3Value(lua_State* state, const vec_t* value);
vec_t* LS_GetVec3Value(lua_State* state, int index);

void LS_InitEdictType(lua_State* state);
void LS_PushEdictValue(lua_State* state, int edictindex);

void LS_InitProgsType(lua_State* state);

#endif // USE_LUA_SCRIPTING
