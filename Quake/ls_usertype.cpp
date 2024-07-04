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

#ifdef USE_LUA_SCRIPTING

#include <assert.h>

#include "ls_common.h"
#include "ls_vector.h"


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

int LS_ImGuiTypeOperatorIndex(lua_State* state, const LS_TypelessUserDataType& type, const LS_MemberDefinition& member)
{
	void* userdataptr = type.GetValuePtr(state, 1);
	assert(userdataptr);

	const uint8_t* bytes = *reinterpret_cast<const uint8_t**>(userdataptr);
	assert(bytes);

	const uint8_t* memberptr = bytes + member.offset;

	switch (LS_MemberType(member.type))
	{
	case LS_MemberType::Boolean:
		lua_pushboolean(state, *reinterpret_cast<const bool*>(memberptr));
		break;

	case LS_MemberType::Signed:
	case LS_MemberType::Unsigned:
		lua_pushinteger(state, *reinterpret_cast<const int*>(memberptr));
		break;

	case LS_MemberType::Float:
		lua_pushnumber(state, *reinterpret_cast<const float*>(memberptr));
		break;

	case LS_MemberType::Vector2:
		LS_PushVectorValue(state, (*reinterpret_cast<const LS_Vector2*>(memberptr)));
		break;

	case LS_MemberType::Vector3:
		LS_PushVectorValue(state, (*reinterpret_cast<const LS_Vector3*>(memberptr)));
		break;

	case LS_MemberType::Vector4:
		LS_PushVectorValue(state, (*reinterpret_cast<const LS_Vector4*>(memberptr)));
		break;

	default:
		assert(false);
		lua_pushnil(state);
		break;
	}

	return 1;
}

#endif // USE_LUA_SCRIPTING
