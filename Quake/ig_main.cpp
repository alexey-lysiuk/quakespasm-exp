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

	if (key_dest == key_console)
		Con_ToggleConsole_f();
	else if (key_dest == key_menu)
		M_ToggleMenu_f();

	key_dest = key_menu;

	IN_Deactivate(true);

	SDL_GetEventFilter(&ig_eventfilter, &ig_eventuserdata);
	SDL_SetEventFilter(nullptr, nullptr);

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

void IG_Update()
{
	if (!ig_active || ig_framestarted)
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

	ImGui::SetNextWindowPos(ImVec2(), ImGuiCond_FirstUseEver);
	ImGui::Begin("ImGui", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	if (ImGui::Button("Press ESC to exit"))
		IG_Close();
	ImGui::End();

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

	ig_framestarted = false;
}

qboolean IG_ProcessEvent(const SDL_Event* event)
{
	if (!ig_active)
		return false;

	assert(event);

	if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE)
	{
		IG_Close();
		return true;
	}

	return ImGui_ImplSDL2_ProcessEvent(event);
}

} // extern "C"

#endif // USE_IMGUI
