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

#include <algorithm>
#include <cstring>

#include "lua.hpp"

lua_State* LS_GetState(void);

// Default message handler for lua_pcall() and xpcall()
int LS_ErrorHandler(lua_State* state);

void LS_LoadScript(lua_State* state, const char* filename);

enum class LS_MemberType
{
	Invalid,
	Boolean,
	Signed,
	Unsigned,
	Float,
	Vector2,
	Vector3,
	Vector4,
};

template <typename T>
struct LS_MemberTypeResolver;

template <>
struct LS_MemberTypeResolver<bool>
{
	static constexpr LS_MemberType TYPE = LS_MemberType::Boolean;
};

template <>
struct LS_MemberTypeResolver<int>
{
	static constexpr LS_MemberType TYPE = LS_MemberType::Signed;
};

template <>
struct LS_MemberTypeResolver<unsigned>
{
	static constexpr LS_MemberType TYPE = LS_MemberType::Unsigned;
};

template <>
struct LS_MemberTypeResolver<float>
{
	static constexpr LS_MemberType TYPE = LS_MemberType::Float;
};

struct LS_MemberDefinition
{
	const char* name;
	LS_MemberType type;
	uint32_t offset;

	bool operator<(const LS_MemberDefinition& other) const
	{
		return strcmp(name, other.name);
	}
};

#define LS_DEFINE_MEMBER(TYPENAME, MEMBERNAME) \
	{ #MEMBERNAME, LS_MemberTypeResolver<decltype(TYPENAME::MEMBERNAME)>::TYPE, offsetof(TYPENAME, MEMBERNAME) }

class LS_TypelessUserDataType
{
public:
	const char* const GetName() const { return name; }
	const size_t GetSize() const { return size; }

	void* NewPtr(lua_State* state) const;
	void* GetValuePtr(lua_State* state, int index) const;

	int PushMemberValue(lua_State* state) const;

protected:
	const char* name;
	const LS_MemberDefinition* members;
	size_t size;
	size_t membercount;

	constexpr LS_TypelessUserDataType(const char* const name, const size_t size, const LS_MemberDefinition* const members = nullptr, const size_t membercount = 0)
	: name(name)
	, members(members)
	, size(size)
	, membercount(membercount)
	{
		assert((members == nullptr && membercount == 0) || (members != nullptr && membercount != 0));
	}
};

template <typename T>
class LS_UserDataType : public LS_TypelessUserDataType
{
public:
	constexpr LS_UserDataType(const char* const name, const LS_MemberDefinition* const members = nullptr, const size_t membercount = 0)
	: LS_TypelessUserDataType(name, sizeof(void*) + sizeof(T), members, membercount)
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

void LS_InitEdictType(lua_State* state);
void LS_PushEdictValue(lua_State* state, int edictindex);

void LS_InitProgsType(lua_State* state);

#endif // USE_LUA_SCRIPTING
