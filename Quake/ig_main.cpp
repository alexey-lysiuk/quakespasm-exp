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
#if defined(USE_SDL2)
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif
#else
#include "SDL.h"
#endif

#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_sdl2.h"

#include "quakedef.h"


extern "C"
{

static bool ig_showdemo;

static void IG_ShowDemo()
{
	ig_showdemo = true;
}

void IG_Init()
{
	Cmd_AddCommand("ig_showdemo", IG_ShowDemo);
}

void IG_Shutdown()
{
	
}

static qboolean ig_framestarted;

void IG_Update()
{
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();

	ImGui::NewFrame();

	ig_framestarted = true;
}

void IG_Render()
{
	if (!ig_framestarted)
		return;

	if (ig_showdemo)
		igShowDemoWindow(&ig_showdemo);

	GL_ClearBufferBindings();

	ImGui::Render();

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	ig_framestarted = false;
}

void IG_ProcessEvent(const SDL_Event* event)
{
	ImGui_ImplSDL2_ProcessEvent(&event);
}

}

#endif // USE_IMGUI
