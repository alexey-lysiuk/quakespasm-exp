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

extern qboolean keydown[MAX_KEYS];
}


#ifdef USE_LUA_SCRIPTING

#include "ls_imgui.h"

static const char* ls_expmode_name = "expmode";

static bool LS_CallExpModeFunction(const char* const name)
{
	lua_State* state = LS_GetState();
	assert(state);
	assert(lua_gettop(state) == 0);

	bool result = false;

	lua_pushcfunction(state, LS_ErrorHandler);

	if (lua_getglobal(state, ls_expmode_name) == LUA_TTABLE)
	{
		if (lua_getfield(state, -1, name) == LUA_TFUNCTION)
		{
			if (lua_pcall(state, 0, 1, 1) == LUA_OK)
				result = lua_toboolean(state, -1);

			lua_pop(state, 1);  // remove result or nil returned by error handler

			LS_ClearImGuiStack();
		}
		else
			lua_pop(state, 1);  // remove incorrect value for function to call
	}

	lua_pop(state, 2);  // remove expmode table and error handler
	assert(lua_gettop(state) == 0);

	return result;
}

static void EXP_EnterMode();

static int LS_EnterMode(lua_State* state)
{
	EXP_EnterMode();
	return 0;
}

void LS_InitExpMode(lua_State* state)
{
	lua_gc(state, LUA_GCSTOP);

	LS_InitImGuiBindings(state);

	// Register 'expmode' table
	lua_createtable(state, 0, 16);
	lua_pushcfunction(state, LS_EnterMode);
	lua_setfield(state, -2, "enter");
	lua_setglobal(state, ls_expmode_name);

	LS_LoadScript(state, "scripts/expmode_base.lua");
	LS_LoadScript(state, "scripts/expmode_edicts.lua");
	LS_LoadScript(state, "scripts/expmode_progs.lua");
#ifndef NDEBUG
	LS_LoadScript(state, "scripts/expmode_debug.lua");
#endif // !NDEBUG

	lua_gc(state, LUA_GCRESTART);
	lua_gc(state, LUA_GCCOLLECT);
}

#endif // USE_LUA_SCRIPTING


static bool exp_active;

static SDL_EventFilter exp_eventfilter;
static void* exp_eventuserdata;

static SDL_Window* exp_window;
static SDL_GLContext exp_glcontext;
static bool exp_created;

static int exp_mouseposx = INT_MIN, exp_mouseposy = INT_MIN;

static void EXP_Create()
{
	assert(!exp_created);
	exp_created = true;

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	const int configargindex = COM_CheckParm("-expmodeconfig");
	const char* const configpath = configargindex > 0
		? (configargindex < com_argc - 1 ? com_argv[configargindex + 1] : "")
		: "expmode.ini";

	io.ConfigFlags = ImGuiConfigFlags_NoMouse;
	io.IniFilename = configpath[0] == '\0' ? nullptr : configpath;

	ImGui_ImplSDL2_InitForOpenGL(exp_window, exp_glcontext);
	ImGui_ImplOpenGL2_Init();
}

static void EXP_EnterMode()
{
	if (exp_active || cls.state != ca_connected)
		return;

	if (!exp_created)
		EXP_Create();

	exp_active = true;

	S_BlockSound();

	// Close menu or console if opened
	if (key_dest == key_console)
		Con_ToggleConsole_f();
	else if (key_dest == key_menu)
		M_ToggleMenu_f();

	// Mimic menu mode behavior, e.g. pause game in single player
	m_state = m_none;
	key_dest = key_menu;

	// Disallow in-game input, and enable mouse cursor
	IN_Deactivate(true);

	// Clear key down state, needed when ImGui is opened via bound key press
	memset(keydown, 0, sizeof keydown);

	// Remove event filter to allow mouse move events
	SDL_GetEventFilter(&exp_eventfilter, &exp_eventuserdata);
	SDL_SetEventFilter(nullptr, nullptr);

	if (exp_mouseposx != INT_MIN && exp_mouseposx != INT_MIN)
		SDL_WarpMouseInWindow(exp_window, exp_mouseposx, exp_mouseposy);

	// Enable contol of ImGui windows by all input devices
	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

#ifdef USE_LUA_SCRIPTING
	LS_CallExpModeFunction("onopen");
#endif // USE_LUA_SCRIPTING
}

extern "C"
{

static void EXP_ExitMode()
{
	if (!exp_active)
		return;

#ifdef USE_LUA_SCRIPTING
	LS_CallExpModeFunction("onclose");
#endif // USE_LUA_SCRIPTING

	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NoMouse;

	SDL_GetMouseState(&exp_mouseposx, &exp_mouseposy);

	SDL_StopTextInput();
	SDL_SetEventFilter(exp_eventfilter, exp_eventuserdata);

	IN_Activate();

	if (cls.state == ca_connected)
		key_dest = key_game;

	S_UnblockSound();

	exp_active = false;
}

void EXP_Init(SDL_Window* window, SDL_GLContext context)
{
	exp_window = window;
	exp_glcontext = context;

	Cbuf_AddText("exec scripts/aliases/expmode.cfg\n");
}

void EXP_Shutdown()
{
	if (!exp_created)
		return;

	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();

	ImGui::DestroyContext();
}

void EXP_Update()
{
	if (!exp_active)
		return;

	if (cls.state != ca_connected)
	{
		EXP_ExitMode();
		return;
	}

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();

	ImGui::NewFrame();

	if (!SDL_IsTextInputActive())
		SDL_StartTextInput();

#ifdef USE_LUA_SCRIPTING
	LS_MarkImGuiFrameStart();

	if (!LS_CallExpModeFunction("onupdate"))
		EXP_ExitMode();

	LS_MarkImGuiFrameEnd();
#endif // USE_LUA_SCRIPTING

	ImGui::Render();

	// Fade screen a bit
	GL_SetCanvas(CANVAS_DEFAULT);
	GL_ClearBufferBindings();

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

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	glEnable(GL_ALPHA_TEST);
}

qboolean EXP_ProcessEvent(const SDL_Event* event)
{
	if (!exp_active)
		return false;

	assert(event);
	const Uint32 type = event->type;

	if (type == SDL_KEYDOWN)
	{
		switch (event->key.keysym.sym)
		{
		case SDLK_ESCAPE:
			EXP_ExitMode();
			return true;  // stop further event processing

		case SDLK_PRINTSCREEN:
			void SCR_ScreenShot_f();
			SCR_ScreenShot_f();
			break;

		default:
			break;
		}
	}

	const qboolean eventconsumed = ImGui_ImplSDL2_ProcessEvent(event)
		// Window and quit events should be processed by engine as well
		&& (type != SDL_WINDOWEVENT) && (type != SDL_QUIT);
	return eventconsumed;
}

} // extern "C"

#endif // USE_IMGUI
