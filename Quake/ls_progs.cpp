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

#include "ls_common.h"

extern "C"
{
#include "quakedef.h"
}


// Returns CRC of progdefs header (PROGHEADER_CRC)
static int LS_global_progs_crc(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	lua_pushinteger(state, progs->crc);
	return 1;
}

// Returns CRC of 'progs.dat' file
static int LS_global_progs_datcrc(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	lua_pushinteger(state, pr_crc);
	return 1;
}

// Returns progs version number (PROG_VERSION)
static int LS_global_progs_version(lua_State* state)
{
	if (progs == nullptr)
		return 0;

	lua_pushinteger(state, progs->version);
	return 1;

}

void LS_InitProgsType(lua_State* state)
{
	static const luaL_Reg progs_functions[] =
	{
		{ "crc", LS_global_progs_crc },
		{ "datcrc", LS_global_progs_datcrc },
		{ "version", LS_global_progs_version },
		{ nullptr, nullptr }
	};

	luaL_newlib(state, progs_functions);
	lua_setglobal(state, "progs");
}

#endif // USE_LUA_SCRIPTING
