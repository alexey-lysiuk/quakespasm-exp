/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers

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

#ifdef USE_IMGUI

#include <assert.h>

#if defined(SDL_FRAMEWORK) || defined(NO_SDL_CONFIG)
#include <SDL2/SDL.h>
#else
#include "SDL.h"
#endif

#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_sdl2.h"

extern "C"
{

#include "quakedef.h"
#include "ls_common.h"

static bool exp_active;
static char exp_justactived;

static SDL_EventFilter exp_eventfilter;
static void* exp_eventuserdata;

#ifdef USE_LUA_SCRIPTING

static const char* ls_expmode_name = "expmode";

static bool LS_CallQImGuiFunction(const char* const name)
{
	lua_State* state = LS_GetState();
	assert(state);
	assert(lua_gettop(state) == 0);

	bool result = false;

	if (lua_getglobal(state, ls_expmode_name) == LUA_TTABLE)
	{
		if (lua_getfield(state, -1, name) == LUA_TFUNCTION)
		{
			if (lua_pcall(state, 0, 1, 0) == LUA_OK)
			{
				result = lua_toboolean(state, -1);
				lua_pop(state, 1);  // remove result
			}
			else
				LS_ReportError(state);

			void ImClearStack();
			ImClearStack();
		}
		else
			lua_pop(state, 1);  // remove incorrect value for function to call
	}

	lua_pop(state, 1);  // remove expmode table
	assert(lua_gettop(state) == 0);

	return result;
}

void LS_InitImGuiModule(lua_State* state)
{
	void ImLoadBindings(lua_State* state);
	ImLoadBindings(state);

	// Register 'expmode' table
	lua_createtable(state, 0, 16);
	lua_setglobal(state, ls_expmode_name);

	LS_LoadScript(state, "scripts/expmode.lua");
}

#endif // USE_LUA_SCRIPTING

static void EXP_EnterMode()
{
	if (exp_active)
		return;

	if (cls.state != ca_connected)
	{
		exp_active = false;
		return;
	}

	exp_active = true;

	// Need to skip two frames because SDL_StopTextInput() is called during the next one after ImGui activation
	// Otherwise, text input will be unavailable. See EXP_Update() for SDL_StartTextInput() call
	exp_justactived = 2;

	// Close menu or console if opened
	if (key_dest == key_console)
		Con_ToggleConsole_f();
	else if (key_dest == key_menu)
		M_ToggleMenu_f();

	// Mimin menu mode behavior, e.g. pause game in single player
	key_dest = key_menu;

	// Disallow in-game input, and enable mouse cursor
	IN_Deactivate(true);

	// Clear key down state, needed when ImGui is opened via bound key press
	extern qboolean keydown[MAX_KEYS];
	memset(keydown, 0, sizeof keydown);

	// Remove event filter to allow mouse move events
	SDL_GetEventFilter(&exp_eventfilter, &exp_eventuserdata);
	SDL_SetEventFilter(nullptr, nullptr);

	// Enable contol of ImGui windows by all input devices
	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

#ifdef USE_LUA_SCRIPTING
	LS_CallExpModeFunction("onopen");
#endif // USE_LUA_SCRIPTING
}

static void EXP_ExitMode()
{
	if (!exp_active)
		return;

#ifdef USE_LUA_SCRIPTING
	LS_CallExpModeFunction("onclose");
#endif // USE_LUA_SCRIPTING

	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NoMouse;

	SDL_StopTextInput();
	SDL_SetEventFilter(exp_eventfilter, exp_eventuserdata);

	IN_Activate();

	if (cls.state == ca_connected)
		key_dest = key_game;

	exp_active = false;
}

void EXP_Init(SDL_Window* window, SDL_GLContext context)
{
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	const int configargindex = COM_CheckParm("-expmodeconfig");
	const char* const configpath = configargindex > 0
		? (configargindex < com_argc - 1 ? com_argv[configargindex + 1] : "")
		: "expmode.ini";

	io.ConfigFlags = ImGuiConfigFlags_NoMouse;
	io.IniFilename = configpath[0] == '\0' ? nullptr : configpath;

	ImGui_ImplSDL2_InitForOpenGL(window, context);
	ImGui_ImplOpenGL2_Init();

	Cmd_AddCommand("expmode", EXP_EnterMode);
}

void EXP_Shutdown()
{
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();

	ImGui::DestroyContext();
}

void EXP_Update()
{
	if (!exp_active)
		return;

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();

	ImGui::NewFrame();

	if (exp_justactived > 0)
	{
		--exp_justactived;

		if (exp_justactived == 0)
			SDL_StartTextInput();
	}

#ifdef USE_LUA_SCRIPTING
	if (!LS_CallExpModeFunction("onupdate"))
		EXP_ExitMode();
#endif // USE_LUA_SCRIPTING

	ImGui::Render();

	// Fade screen a bit
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(0.0f, 0.0f, 0.0f, 0.25f);

	glBegin(GL_QUADS);
	glVertex2f(0.0f, 0.0f);
	glVertex2f(glwidth, 0.0f);
	glVertex2f(glwidth, glheight);
	glVertex2f(0.0f, glheight);
	glEnd();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	GL_ClearBufferBindings();

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	glEnable(GL_ALPHA_TEST);
}

qboolean EXP_ProcessEvent(const SDL_Event* event)
{
	if (!exp_active)
		return false;

	assert(event);
	const Uint32 type = event->type;

	if (type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE)
	{
		EXP_ExitMode();
		return true;  // stop further event processing
	}

	const qboolean eventconsumed = ImGui_ImplSDL2_ProcessEvent(event)
		// Window and quit events should be processed by engine as well
		&& (type != SDL_WINDOWEVENT) && (type != SDL_QUIT);
	return eventconsumed;
}

} // extern "C"

#endif // USE_IMGUI
