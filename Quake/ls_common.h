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

#include "lua.hpp"

extern "C"
{
#include "q_stdinc.h"
#include "cvar.h"
}

class LS_TempAllocatorBase
{
protected:
	static void* Alloc(size_t size);
	static void Free(void* pointer);
};

template <typename T>
class LS_TempAllocator : LS_TempAllocatorBase
{
public:
	using value_type = T;

	static T* allocate(const size_t count)
	{
		return static_cast<T*>(Alloc(sizeof(T) * count));
	}

	static void deallocate(T* pointer, size_t) noexcept
	{
		Free(pointer);
	}
};

template <typename T, typename U>
bool operator==(const LS_TempAllocator<T>&, const LS_TempAllocator<U>&)
{
	return true;
}

template <typename T, typename U>
bool operator!=(const LS_TempAllocator<T>&, const LS_TempAllocator<U>&)
{
	return false;
}

lua_State* LS_GetState(void);

// Default message handler for lua_pcall() and xpcall()
int LS_ErrorHandler(lua_State* state);

void LS_LoadScript(lua_State* state, const char* filename);

void LS_SetIndexTable(lua_State* state, const luaL_Reg* const functions);

class LS_TypelessUserDataType
{
public:
	constexpr LS_TypelessUserDataType(const char* const name, const size_t size)
	: name(name)
	, size(size)
	{
	}

	const char* const GetName() const { return name; }
	const size_t GetSize() const { return size; }

	void* NewPtr(lua_State* state) const;
	void* GetValuePtr(lua_State* state, int index) const;

protected:
	const char* name;
	size_t size;

	void SetMetaTable(lua_State* state, const luaL_Reg* members, const luaL_Reg* metafuncs) const;
};

template <typename T>
class LS_UserDataType : public LS_TypelessUserDataType
{
public:
	constexpr LS_UserDataType(const char* const name)
	: LS_TypelessUserDataType(name, sizeof(void*) + sizeof(T))
	{
	}

	T& New(lua_State* const state, const luaL_Reg* members = nullptr, const luaL_Reg* metafuncs = nullptr) const
	{
		T& result = *static_cast<T*>(NewPtr(state));

		if (members || metafuncs)
			SetMetaTable(state, members, metafuncs);

		return result;
	}

	T& GetValue(lua_State* const state, const int index) const
	{
		return *static_cast<T*>(GetValuePtr(state, index));
	}
};

template <cvar_t& cvar>
inline int LS_BoolCVarFunction(lua_State* state)
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

template <cvar_t& cvar>
inline int LS_NumberCVarFunction(lua_State* state)
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

void LS_InitEdictType(lua_State* state);
void LS_PushEdictValue(lua_State* state, int edictindex);
void LS_PushEdictValue(lua_State* state, const struct edict_s* edict);

void LS_InitProgsType(lua_State* state);
void LS_ResetProgsType();

#endif // USE_LUA_SCRIPTING
