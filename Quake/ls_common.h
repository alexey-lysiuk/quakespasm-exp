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

#include <cstring>

#include "lua.hpp"

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

lua_State* LS_GetState(void);

// Default message handler for lua_pcall() and xpcall()
int LS_ErrorHandler(lua_State* state);

void LS_LoadScript(lua_State* state, const char* filename);

struct LS_Member
{
	size_t length;
	const char* name;

	using Getter = int(*)(lua_State* state);
	Getter getter;

	constexpr LS_Member(size_t length, const char* name, Getter getter)
	: length(length)
	, name(name)
	, getter(getter)
	{
	}

	template <size_t N>
	constexpr LS_Member(const char (&name)[N], Getter getter)
	: LS_Member(N - 1, name, getter)
	{
		static_assert(N > 1);
	}

	bool operator<(const LS_Member& other) const
	{
		return (length < other.length)
			|| (length == other.length && strncmp(name, other.name, length) < 0);
	}
};

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
};

template <typename T>
class LS_UserDataType : public LS_TypelessUserDataType
{
public:
	constexpr LS_UserDataType(const char* const name)
	: LS_TypelessUserDataType(name, sizeof(void*) + sizeof(T))
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

int LS_GetMember(lua_State* state, const LS_TypelessUserDataType& type, const LS_Member* members, const size_t membercount);

void LS_InitEdictType(lua_State* state);
void LS_PushEdictValue(lua_State* state, int edictindex);

void LS_InitProgsType(lua_State* state);

#endif // USE_LUA_SCRIPTING
