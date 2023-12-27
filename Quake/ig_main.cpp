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
static cvar_t ig_showdemo = {"imgui_showdemo", "0", CVAR_NONE};
#endif // !NDEBUG

static bool ig_active;
static char ig_justactived;
static bool ig_framestarted;

static SDL_EventFilter ig_eventfilter;
static void* ig_eventuserdata;

static void IG_Activate()
{
	if (ig_active || cls.state != ca_connected)
		return;

	ig_active = true;
	ig_justactived = 2;

	if (key_dest == key_console)
		Con_ToggleConsole_f();
	else if (key_dest == key_menu)
		M_ToggleMenu_f();

	IN_Deactivate(true);

	SDL_GetEventFilter(&ig_eventfilter, &ig_eventuserdata);
	SDL_SetEventFilter(nullptr, nullptr);

	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
}

static void IG_Deactivate()
{
	if (!ig_active)
		return;

	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NoMouse;

	SDL_StopTextInput();
	SDL_SetEventFilter(ig_eventfilter, ig_eventuserdata);

	IN_Activate();

	ig_active = false;
}

void IG_Init(SDL_Window* window, SDL_GLContext context)
{
	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NoMouse;

	ImGui_ImplSDL2_InitForOpenGL(window, context);
	ImGui_ImplOpenGL2_Init();

	Cmd_AddCommand("imgui_activate", IG_Activate);

#ifndef NDEBUG
	Cvar_RegisterVariable(&ig_showdemo);
#endif // !NDEBUG
}

void IG_Shutdown()
{
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();

	ImGui::DestroyContext();
}

#ifdef USE_LUA_SCRIPTING

static const char* ls_imgui_name = "imgui";
static const char* ls_widgets_name = "widgets";

void LS_InitImGuiModule(lua_State* state)
{
	void LoadImguiBindings(lua_State* state);
	LoadImguiBindings(state);

	// Register table for widgets
	lua_getglobal(state, ls_imgui_name);
	lua_createtable(state, 0, 16);
	lua_setfield(state, -2, ls_widgets_name);
	lua_pop(state, 1);  // remove imgui global table

	LS_LoadScript(state, "scripts/imgui.lua");
}

static void LS_UpdateWidgets()
{
	lua_State* state = LS_GetState();
	assert(state);
	assert(lua_gettop(state) == 0);

	if (lua_getglobal(state, ls_imgui_name) == LUA_TTABLE)
	{
		if (lua_getfield(state, -1, ls_widgets_name) == LUA_TTABLE)
		{
			if (lua_getfield(state, -1, "draw") == LUA_TFUNCTION)
				lua_pcall(state, 1, 0, 0);
			else
				lua_pushfstring(state, "%s.%s has no draw() function", ls_imgui_name, ls_widgets_name);

			lua_pop(state, 2);  // remove imgui and widgets tables
		}
		else
			lua_pop(state, 2);  // remove imgui table and incorrect widgets table value
	}
	else
		lua_pop(state, 1);  // remove incorrect value for imgui table

	assert(lua_gettop(state) == 0);
}

#endif // USE_LUA_SCRIPTING

static void IG_TracedEntityWindow()
{
	// TODO: redo this with Lua scripting
	extern char sv_tracedentityinfo[1024];

	if (sv_tracedentityinfo[0] != '\0')
	{
		ImGui::Begin("Traced entity");
		ImGui::Text("%s", sv_tracedentityinfo);
		ImGui::End();
	}
}

void IG_Update()
{
	if (ig_framestarted)
		return;

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();

	ImGui::NewFrame();

	if (ig_active)
	{
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
		ImGui::Begin("Close Hint", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
		ImGui::TextColored(ImVec4(.9f, 0.1f, 0.1f, 1.f), "Press ESC to exit ImGui mode");
		ImGui::End();
	}

#ifdef USE_LUA_SCRIPTING
	LS_UpdateWidgets();
#endif // USE_LUA_SCRIPTING

	IG_TracedEntityWindow();

	if (ig_justactived > 0)
	{
		--ig_justactived;

		if (ig_justactived == 0)
			SDL_StartTextInput();
	}

	ig_framestarted = true;
}

void IG_Render()
{
	if (!ig_framestarted)
		return;

#ifndef NDEBUG
	if (ig_showdemo.value)
	{
		bool showdemo = true;

		ImGui::ShowDemoWindow(&showdemo);

		if (!showdemo)
			Cvar_SetQuick(&ig_showdemo, "0");
	}
#endif // !NDEBUG

	ImGui::Render();

	GL_ClearBufferBindings();
	glDisable(GL_ALPHA_TEST);

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	glEnable(GL_ALPHA_TEST);

	ig_framestarted = false;
}

qboolean IG_ProcessEvent(const SDL_Event* event)
{
	if (!ig_active)
		return false;

	assert(event);

	if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE)
	{
		IG_Deactivate();
		return true;
	}

	return ImGui_ImplSDL2_ProcessEvent(event);
}

} // extern "C"

#endif // USE_IMGUI
