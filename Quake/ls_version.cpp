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

#if defined(SDL_FRAMEWORK) || defined(NO_SDL_CONFIG)
#include <SDL2/SDL.h>
#else
#include "SDL.h"
#endif // SDL_FRAMEWORK || NO_SDL_CONFIG

#ifdef USE_CODEC_FLAC
#include <FLAC/format.h>
#endif // USE_CODEC_FLAC

#ifdef USE_CODEC_MIKMOD
#include <mikmod.h>
#endif // USE_CODEC_MIKMOD

#ifdef USE_CODEC_MPG123
#include <mpg123.h>
#if MPG123_API_VERSION < 48
static const char *mpg123_distversion(unsigned int *major, unsigned int *minor, unsigned int *patch)
{
	return "pre-1.32.0";
}
#endif // MPG123_API_VERSION < 48
#endif // USE_CODEC_MPG123

#ifdef USE_CODEC_MAD
#include <mad.h>
#endif // USE_CODEC_MAD

#ifdef USE_CODEC_OPUS
#include <opusfile.h>
#endif // USE_CODEC_OPUS

#ifdef USE_CODEC_VORBIS
#include <vorbis/codec.h>
#endif // USE_CODEC_VORBIS

#ifdef USE_CODEC_XMP
#include <xmp.h>
#endif // USE_CODEC_XMP

#ifdef USE_IMGUI
#include "imgui.h"
#endif // USE_IMGUI

#include "ls_common.h"

#if __has_include("expversion.h")
#include "expversion.h"
#else
static const char* const expversion = "developement version";
#endif // has expversion header


int LS_global_expversion(lua_State* state)
{
	int results = 0;

	lua_pushstring(state, expversion);
	++results;

	lua_pushfstring(state, "SDL2 %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
	++results;

#ifdef USE_CODEC_FLAC
	lua_pushfstring(state, "FLAC %s", FLAC__VERSION_STRING);
	++results;
#endif // USE_CODEC_FLAC

#ifdef USE_CODEC_MPG123
	lua_pushfstring(state, "mpg123 %s", mpg123_distversion(nullptr, nullptr, nullptr));
	++results;
#endif // USE_CODEC_MPG123

#ifdef USE_CODEC_MAD
	lua_pushstring(state, "libmad " MAD_VERSION);
	++results;
#endif // USE_CODEC_MAD

#ifdef USE_CODEC_OPUS
	lua_pushstring(state, opus_get_version_string());
	++results;
#endif // USE_CODEC_OPUS

#ifdef USE_CODEC_VORBIS
	lua_pushstring(state, vorbis_version_string());
	++results;
#endif // USE_CODEC_VORBIS

#ifdef USE_CODEC_XMP
	lua_pushstring(state, "libxmp " XMP_VERSION);
	++results;
#endif // USE_CODEC_XMP

#ifdef USE_CODEC_MIKMOD
	lua_pushfstring(state, "MikMod %d.%d.%d", LIBMIKMOD_VERSION_MAJOR, LIBMIKMOD_VERSION_MINOR, LIBMIKMOD_REVISION);
	++results;
#endif // USE_CODEC_MIKMOD

#ifdef USE_CODEC_MODPLUG
	lua_pushstring(state, "ModPlug");
	++results;
#endif // USE_CODEC_MODPLUG

	lua_pushstring(state, LUA_RELEASE);
	++results;

#ifdef USE_IMGUI
	lua_pushstring(state, "ImGui " IMGUI_VERSION);
	++results;
#endif // USE_IMGUI

	return results;
}

#endif // USE_LUA_SCRIPTING
