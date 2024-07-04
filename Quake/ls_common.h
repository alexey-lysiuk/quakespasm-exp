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

#include "frozen/string.h"
#include "lua.hpp"

lua_State* LS_GetState(void);

// Default message handler for lua_pcall() and xpcall()
int LS_ErrorHandler(lua_State* state);

void LS_LoadScript(lua_State* state, const char* filename);

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
	uint32_t type:8;
	uint32_t offset:24;
};

#define LS_DEFINE_MEMBER(TYPENAME, MEMBERNAME) \
	{ #MEMBERNAME, { uint8_t(LS_MemberTypeResolver<decltype(TYPENAME::MEMBERNAME)>::TYPE), offsetof(TYPENAME, MEMBERNAME) } }

template <typename T>
const LS_MemberDefinition& LS_GetMemberDefinition(lua_State* state, const char* nameoftype, const T& members)
{
	size_t length;
	const char* key = luaL_checklstring(state, 2, &length);
	assert(key);
	assert(length > 0);

	const frozen::string keystr(key, length);
	const auto valueit = members.find(keystr);

	if (members.end() == valueit)
		luaL_error(state, "unknown member '%s' of type '%s'", key, nameoftype);

	return valueit->second;
}

int LS_GetMemberValue(lua_State* state, const LS_TypelessUserDataType& type, const LS_MemberDefinition& member);

void LS_InitEdictType(lua_State* state);
void LS_PushEdictValue(lua_State* state, int edictindex);

void LS_InitProgsType(lua_State* state);

#endif // USE_LUA_SCRIPTING
