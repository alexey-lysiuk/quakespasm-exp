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

#ifndef NDEBUG
static bool ig_showdemo;
#endif // !NDEBUG

static bool ig_active;
static char ig_justactived;

static SDL_EventFilter ig_eventfilter;
static void* ig_eventuserdata;

#ifdef USE_LUA_SCRIPTING

static const char* ls_qimgui_name = "qimgui";

void LS_InitImGuiModule(lua_State* state)
{
	void ImLoadBindings(lua_State* state);
	ImLoadBindings(state);

	// Register tables for scripted ImGui windows
	lua_newtable(state);
	lua_pushvalue(state, -1);  // copy for lua_setfield()
	lua_setglobal(state, ls_qimgui_name);
	lua_createtable(state, 0, 16);
	lua_setfield(state, -2, "windows");
	lua_createtable(state, 0, 16);
	lua_setfield(state, -2, "tools");
	lua_pop(state, 1);  // remove qimgui global table

	LS_LoadScript(state, "scripts/qimgui.lua");
}

static qboolean LS_PushQImGuiTable(lua_State* state, const char* const name)
{
	assert(state != NULL);

	if (lua_getglobal(state, ls_qimgui_name) != LUA_TTABLE)
	{
		lua_pop(state, 1);  // remove incorrect value for qimgui table
		return false;
	}

	if (lua_getfield(state, -1, name) != LUA_TTABLE)
	{
		lua_pop(state, 2);  // remove qimgui table and incorrect named table value
		return false;
	}

	lua_remove(state, -2);  // remove qimgui table
	return true;
}

static void LS_CallQImGuiFunction(const char* const name)
{
	lua_State* state = LS_GetState();
	assert(state);
	assert(lua_gettop(state) == 0);

	if (lua_getglobal(state, ls_qimgui_name) == LUA_TTABLE)
	{
		if (lua_getfield(state, -1, name) == LUA_TFUNCTION)
		{
			if (lua_pcall(state, 0, 0, 0) != LUA_OK)
				LS_ReportError(state);

			void ImClearStack();
			ImClearStack();
		}
	}

	lua_pop(state, 1);  // remove tools table
	assert(lua_gettop(state) == 0);
}

#endif // USE_LUA_SCRIPTING

static void IG_Open()
{
	if (ig_active)
		return;

	if (cls.state != ca_connected)
	{
		ig_active = false;
		return;
	}

	ig_active = true;

	// Need to skip two frames because SDL_StopTextInput() is called during the next one after ImGui activation
	// Otherwise, text input will be unavailable. See IG_Update() for SDL_StartTextInput() call
	ig_justactived = 2;

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
	SDL_GetEventFilter(&ig_eventfilter, &ig_eventuserdata);
	SDL_SetEventFilter(nullptr, nullptr);

	// Enable contol of ImGui windows by all input devices
	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
}

static void IG_Close()
{
	if (!ig_active)
		return;

	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NoMouse;

	SDL_StopTextInput();
	SDL_SetEventFilter(ig_eventfilter, ig_eventuserdata);

	IN_Activate();

	if (cls.state == ca_connected)
		key_dest = key_game;

	ig_active = false;
}

void IG_Init(SDL_Window* window, SDL_GLContext context)
{
	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NoMouse;

	ImGui_ImplSDL2_InitForOpenGL(window, context);
	ImGui_ImplOpenGL2_Init();

	Cmd_AddCommand("imgui_open", IG_Open);
}

void IG_Shutdown()
{
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();

	ImGui::DestroyContext();
}

void IG_Update()
{
	if (!ig_active)
		return;

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();

	ImGui::NewFrame();

	if (ig_justactived > 0)
	{
		--ig_justactived;

		if (ig_justactived == 0)
			SDL_StartTextInput();
	}

	ImGui::SetNextWindowPos(ImVec2(), ImGuiCond_FirstUseEver);
	ImGui::Begin("ImGui", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);

#ifdef USE_LUA_SCRIPTING
	//LS_UpdateTools();
	LS_CallQImGuiFunction("updatetools");
#endif // USE_LUA_SCRIPTING

#ifndef NDEBUG
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Checkbox("Demo Window", &ig_showdemo);

	if (ig_showdemo)
		ImGui::ShowDemoWindow(&ig_showdemo);
#endif // !NDEBUG

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	if (ImGui::Button("Press ESC to exit"))
		IG_Close();
	ImGui::End();

#ifdef USE_LUA_SCRIPTING
	//LS_UpdateWindows();
	LS_CallQImGuiFunction("updatewindows");
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

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	GL_ClearBufferBindings();

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	glEnable(GL_ALPHA_TEST);
}

qboolean IG_ProcessEvent(const SDL_Event* event)
{
	if (!ig_active)
		return false;

	assert(event);

	if (event->type == SDL_KEYUP && event->key.keysym.sym == SDLK_ESCAPE)
	{
		IG_Close();
		return true;
	}

	return ImGui_ImplSDL2_ProcessEvent(event);
}

} // extern "C"

#endif // USE_IMGUI
