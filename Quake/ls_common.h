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

struct LS_ImGuiMember
{
	size_t type:8;
	size_t offset:24;
};

template <typename T>
struct LS_ImGuiTypeHolder;

enum LS_ImGuiType
{
	ImMemberType_bool,
	ImMemberType_int,
	ImMemberType_unsigned,
	ImMemberType_ImGuiDir,
	ImMemberType_float,
	ImMemberType_ImVec2,
	ImMemberType_ImVec4,
};

#define LS_IMGUI_DEFINE_MEMBER_TYPE(TYPE) \
	template <> struct LS_ImGuiTypeHolder<TYPE> { static constexpr LS_ImGuiType IMGUI_MEMBER_TYPE = ImMemberType_##TYPE; }

LS_IMGUI_DEFINE_MEMBER_TYPE(bool);
LS_IMGUI_DEFINE_MEMBER_TYPE(int);
LS_IMGUI_DEFINE_MEMBER_TYPE(unsigned);
LS_IMGUI_DEFINE_MEMBER_TYPE(float);

#define LS_IMGUI_MEMBER(TYPENAME, MEMBERNAME) \
	{ #MEMBERNAME, { LS_ImGuiTypeHolder<decltype(TYPENAME::MEMBERNAME)>::IMGUI_MEMBER_TYPE, offsetof(TYPENAME, MEMBERNAME) } }

template <typename T>
const LS_ImGuiMember& LS_GetIndexMemberType(lua_State* state, const char* nameoftype, const T& members)
{
	size_t length;
	const char* key = luaL_checklstring(state, 2, &length);
	assert(key);
	assert(length > 0);

	const frozen::string keystr(key, length);
	const auto valueit = members.find(keystr);

	if (members.end() == valueit)
		luaL_error(state, "unknown member '%s' of %s", key, nameoftype);

	return valueit->second;
}

using CustomTypeHandler = bool(*)(lua_State*, const LS_TypelessUserDataType& type, const LS_ImGuiMember&);
int LS_ImGuiTypeOperatorIndex(lua_State* state, const LS_TypelessUserDataType& type, const LS_ImGuiMember& member, CustomTypeHandler hander = nullptr);

void LS_InitEdictType(lua_State* state);
void LS_PushEdictValue(lua_State* state, int edictindex);

void LS_InitProgsType(lua_State* state);

#endif // USE_LUA_SCRIPTING
