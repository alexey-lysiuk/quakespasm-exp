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

#ifndef NDEBUG
static cvar_t ig_showdemo = {"imgui_showdemo", "0", CVAR_NONE};
#endif // !NDEBUG

qboolean ig_active;

static void IG_Activate()
{
	ig_active = true;

	if (key_dest == key_console && cls.state == ca_connected)
		Con_ToggleConsole_f();

	IN_Deactivate(true);
}

void IG_Init(SDL_Window* window, SDL_GLContext context)
{
	ImGui::CreateContext();

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

static qboolean ig_framestarted;

void IG_Update()
{
	if (ig_framestarted)
		return;

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();

	ImGui::NewFrame();

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

	GL_ClearBufferBindings();

	ImGui::Render();

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	ig_framestarted = false;
}

qboolean IG_ProcessEvent(const SDL_Event* event)
{
	return ImGui_ImplSDL2_ProcessEvent(event);
}

}

#endif // USE_IMGUI
