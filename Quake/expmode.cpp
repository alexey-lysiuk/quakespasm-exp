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

#ifdef USE_LUA_SCRIPTING

static bool exp_framescope;

static void LS_EnsureFrameScope(lua_State* state)
{
	if (!exp_framescope)
		// TODO: Add faulty function name to error message
		luaL_error(state, "Calling ImGui function outside of frame scope");
}

uint32_t exp_windowscope;

static void LS_EnsureWindowScope(lua_State* state)
{
	if (exp_windowscope == 0)
		// TODO: Add faulty function name to error message
		luaL_error(state, "Calling ImGui function outside of window scope");
}

uint32_t exp_popupscope;

static void LS_EnsurePopupScope(lua_State* state)
{
	if (exp_popupscope == 0)
		// TODO: Add faulty function name to error message
		luaL_error(state, "Calling ImGui function outside of popup scope");
}

uint32_t exp_tablescope;

static void LS_EnsureTableScope(lua_State* state)
{
	if (exp_tablescope == 0)
		// TODO: Add faulty function name to error message
		luaL_error(state, "Calling ImGui function outside of table scope");
}

using ImGuiEndFunction = void(*)();

static ImVector<ImGuiEndFunction> ls_endfuncstack;

static void LS_AddToImGuiStack(ImGuiEndFunction endfunc)
{
	ls_endfuncstack.push_back(endfunc);
}

static void LS_RemoveFromImGuiStack(lua_State* state, ImGuiEndFunction endfunc)
{
	assert(!ls_endfuncstack.empty());

	if (ls_endfuncstack.back() != endfunc)
		luaL_error(state, "unexpected end function");  // TODO: improve error message

	ls_endfuncstack.pop_back();
	endfunc();
}

static void LS_ClearImGuiStack()
{
	while (!ls_endfuncstack.empty())
	{
		ls_endfuncstack.back()();
		ls_endfuncstack.pop_back();
	}
}

static int LS_global_imgui_GetMainViewport(lua_State* state)
{
	LS_EnsureFrameScope(state);

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	assert(viewport);

	// TODO: return userdata with fields accessible on demand instead of complete table
	lua_createtable(state, 0, 5);

	lua_pushnumber(state, viewport->Flags);
	lua_setfield(state, -2, "Flags");

	lua_createtable(state, 0, 2);
	lua_pushnumber(state, viewport->Pos.x);
	lua_setfield(state, -2, "x");
	lua_pushnumber(state, viewport->Pos.y);
	lua_setfield(state, -2, "y");
	lua_setfield(state, -2, "Pos");

	lua_createtable(state, 0, 2);
	lua_pushnumber(state, viewport->Size.x);
	lua_setfield(state, -2, "x");
	lua_pushnumber(state, viewport->Size.y);
	lua_setfield(state, -2, "y");
	lua_setfield(state, -2, "Size");

	lua_createtable(state, 0, 2);
	lua_pushnumber(state, viewport->WorkPos.x);
	lua_setfield(state, -2, "x");
	lua_pushnumber(state, viewport->WorkPos.y);
	lua_setfield(state, -2, "y");
	lua_setfield(state, -2, "WorkPos");

	lua_createtable(state, 0, 2);
	lua_pushnumber(state, viewport->WorkSize.x);
	lua_setfield(state, -2, "x");
	lua_pushnumber(state, viewport->WorkSize.y);
	lua_setfield(state, -2, "y");
	lua_setfield(state, -2, "WorkSize");

	return 1;
}

static int LS_global_imgui_SetClipboardText(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* text = luaL_checkstring(state, 1);
	assert(text);

	ImGui::SetClipboardText(text);
	return 0;
}

#ifndef NDEBUG
static int LS_global_imgui_ShowDemoWindow(lua_State* state)
{
	LS_EnsureFrameScope(state);

	bool openvalue;
	bool* openptr = &openvalue;

	if (lua_type(state, 1) == LUA_TNIL)
		openptr = nullptr;
	else
		openvalue = lua_toboolean(state, 1);

	ImGui::ShowDemoWindow(openptr);

	if (openptr == nullptr)
		return 0;  // p_open == nullptr, no return value

	lua_pushboolean(state, openvalue);
	return 1;
}
#endif // !NDEBUG

static void LS_EndWindowScope()
{
	assert(exp_windowscope > 0);
	--exp_windowscope;

	ImGui::End();
}

static int LS_global_imgui_Begin(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const name = luaL_checkstring(state, 1);
	assert(name);

	if (name[0] == '\0')
		luaL_error(state, "window name is required");

	bool openvalue;
	bool* openptr = &openvalue;

	if (lua_type(state, 2) == LUA_TNIL)
		openptr = nullptr;
	else
		openvalue = lua_toboolean(state, 2);

	const int flags = luaL_optinteger(state, 3, 0);

	const bool visible = ImGui::Begin(name, openptr, flags);
	lua_pushboolean(state, visible);

	LS_AddToImGuiStack(LS_EndWindowScope);
	++exp_windowscope;

	if (openptr == nullptr)
		return 1;  // p_open == nullptr, one return value

	lua_pushboolean(state, openvalue);
	return 2;
}

static int LS_global_imgui_End(lua_State* state)
{
	LS_EnsureWindowScope(state);

	LS_RemoveFromImGuiStack(state, LS_EndWindowScope);
	return 0;
}

static int LS_global_imgui_GetWindowContentRegionMax(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const ImVec2 region = ImGui::GetWindowContentRegionMax();
	lua_pushnumber(state, region.x);
	lua_pushnumber(state, region.y);
	return 2;
}

static int LS_global_imgui_SameLine(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const float offset = luaL_optnumber(state, 1, 0.f);
	const float spacing = luaL_optnumber(state, 2, 0.f);

	ImGui::SameLine(offset, spacing);
	return 0;
}

static int LS_global_imgui_SetNextWindowFocus(lua_State* state)
{
	LS_EnsureFrameScope(state);

	ImGui::SetNextWindowFocus();
	return 0;
}

static int LS_global_imgui_SetNextWindowPos(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const float posx = luaL_checknumber(state, 1);
	const float posy = luaL_checknumber(state, 2);
	const ImVec2 pos(posx, posy);

	const int cond = luaL_optinteger(state, 3, 0);

	const float pivotx = luaL_optnumber(state, 4, 0.f);
	const float pivoty = luaL_optnumber(state, 5, 0.f);
	const ImVec2 pivot(pivotx, pivoty);

	ImGui::SetNextWindowPos(pos, cond, pivot);
	return 0;
}

static int LS_global_imgui_SetNextWindowSize(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const float sizex = luaL_checknumber(state, 1);
	const float sizey = luaL_checknumber(state, 2);
	const ImVec2 size(sizex, sizey);

	const int cond = luaL_optinteger(state, 3, 0);

	ImGui::SetNextWindowSize(size, cond);
	return 0;
}

static void LS_EndPopupScope()
{
	assert(exp_popupscope > 0);
	--exp_popupscope;

	ImGui::EndPopup();
}

static int LS_global_imgui_BeginPopupContextItem(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const char* strid = luaL_optlstring(state, 1, nullptr, nullptr);
	const int flags = luaL_optinteger(state, 2, 1);

	const bool visible = ImGui::BeginPopupContextItem(strid, flags);
	lua_pushboolean(state, visible);

	if (visible)
	{
		LS_AddToImGuiStack(LS_EndPopupScope);
		++exp_popupscope;
	}

	return 1;
}

static int LS_global_imgui_EndPopup(lua_State* state)
{
	LS_EnsurePopupScope(state);

	LS_RemoveFromImGuiStack(state, LS_EndPopupScope);
	return 0;
}

static void LS_EndTableScope()
{
	assert(exp_tablescope > 0);
	--exp_tablescope;

	ImGui::EndTable();
}

static int LS_global_imgui_BeginTable(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const char* strid = luaL_checkstring(state, 1);
	const int columncount = luaL_checkinteger(state, 2);
	const int flags = luaL_optinteger(state, 3, 0);

	const float sizex = luaL_optnumber(state, 4, 0.f);
	const float sizey = luaL_optnumber(state, 5, 0.f);
	const ImVec2 outersize(sizex, sizey);
	const float innerwidth = luaL_optnumber(state, 6, 0.f);

	const bool visible = ImGui::BeginTable(strid, columncount, flags, outersize, innerwidth);
	lua_pushboolean(state, visible);

	if (visible)
	{
		LS_AddToImGuiStack(LS_EndTableScope);
		++exp_tablescope;
	}

	return 1;
}

static int LS_global_imgui_EndTable(lua_State* state)
{
	LS_EnsureTableScope(state);

	LS_RemoveFromImGuiStack(state, LS_EndTableScope);
	return 0;
}

static int LS_global_imgui_TableHeadersRow(lua_State* state)
{
	LS_EnsureTableScope(state);

	ImGui::TableHeadersRow();
	return 0;
}

static int LS_global_imgui_TableNextColumn(lua_State* state)
{
	LS_EnsureTableScope(state);

	const bool visible = ImGui::TableNextColumn();
	lua_pushboolean(state, visible);
	return 1;
}

static int LS_global_imgui_TableNextRow(lua_State* state)
{
	LS_EnsureTableScope(state);

	const int flags = luaL_optinteger(state, 1, 0);
	const float minrowheight = luaL_optnumber(state, 2, 0.f);

	ImGui::TableNextRow(flags, minrowheight);
	return 0;
}

static int LS_global_imgui_TableSetupColumn(lua_State* state)
{
	LS_EnsureTableScope(state);

	const char* label = luaL_checkstring(state, 1);
	const int flags = luaL_optinteger(state, 2, 0);
	const float initwidthorweight = luaL_optnumber(state, 3, 0.f);
	const ImGuiID userid = luaL_optinteger(state, 4, 0);

	ImGui::TableSetupColumn(label, flags, initwidthorweight, userid);
	return 0;
}

static ImVector<char> ls_inputtextbuffer;

static int LS_global_imgui_Button(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const char* const label = luaL_checkstring(state, 1);
	const float sizex = luaL_optnumber(state, 2, 0.f);
	const float sizey = luaL_optnumber(state, 3, 0.f);
	const ImVec2 size(sizex, sizey);

	const bool result = ImGui::Button(label, size);
	lua_pushboolean(state, result);
	return 1;
}

static int LS_PushImVec2(lua_State* state, const ImVec2& value)
{
	lua_createtable(state, 0, 2);
	lua_pushnumber(state, value.x);
	lua_setfield(state, -2, "x");
	lua_pushnumber(state, value.y);
	lua_setfield(state, -2, "y");
	return 1;
}

static int LS_global_imgui_GetItemRectMax(lua_State* state)
{
	LS_EnsureWindowScope(state);

	return LS_PushImVec2(state, ImGui::GetItemRectMax());
}

static int LS_global_imgui_GetItemRectMin(lua_State* state)
{
	LS_EnsureWindowScope(state);

	return LS_PushImVec2(state, ImGui::GetItemRectMin());
}

static int LS_global_imgui_InputTextMultiline(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const char* label = luaL_checkstring(state, 1);
	assert(label);

	size_t textlength = 0;
	const char* text = luaL_checklstring(state, 2, &textlength);
	assert(text);

	static constexpr lua_Integer BUFFER_SIZE_MIN = 1024;
	static constexpr lua_Integer BUFFER_SIZE_MAX = 1024 * 1024;
	const lua_Integer ibuffersize = luaL_checkinteger(state, 3);

	const size_t buffersize = CLAMP(BUFFER_SIZE_MIN, ibuffersize, BUFFER_SIZE_MAX);
	ls_inputtextbuffer.resize(buffersize);

	if (buffersize <= textlength)
	{
		// Text doesn't fit the buffer, cut it
		textlength = buffersize - 1;
		ls_inputtextbuffer[textlength] = '\0';
	}

	char* buf = &ls_inputtextbuffer[0];

	if (textlength > 0)
		strncpy(buf, text, textlength);
	else
		*buf = '\0';

	const float sizex = luaL_optnumber(state, 4, 0.f);
	const float sizey = luaL_optnumber(state, 5, 0.f);
	const ImVec2 size(sizex, sizey);

	const int flags = luaL_optinteger(state, 6, 0);

	// TODO: Input text callback support
	const bool changed = ImGui::InputTextMultiline(label, buf, buffersize, size, flags);
	lua_pushboolean(state, changed);

	if (changed)
		lua_pushstring(state, buf);
	else
		lua_pushvalue(state, 2);

	return 2;
}

static int LS_global_imgui_Selectable(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const char* const label = luaL_checkstring(state, 1);
	const bool selected = luaL_opt(state, lua_toboolean, 2, false);
	const int flags = luaL_optinteger(state, 3, 0);

	const float sizex = luaL_optnumber(state, 4, 0.f);
	const float sizey = luaL_optnumber(state, 5, 0.f);
	const ImVec2 size(sizex, sizey);

	const bool pressed = ImGui::Selectable(label, selected, flags, size);
	lua_pushboolean(state, pressed);
	return 1;
}

static int LS_global_imgui_Separator(lua_State* state)
{
	LS_EnsureWindowScope(state);

	ImGui::Separator();
	return 0;
}

static int LS_global_imgui_SeparatorText(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const char* const label = luaL_checkstring(state, 1);
	ImGui::SeparatorText(label);
	return 0;
}

static int LS_global_imgui_Spacing(lua_State* state)
{
	LS_EnsureWindowScope(state);

	ImGui::Spacing();
	return 0;
}

static int LS_global_imgui_Text(lua_State* state)
{
	LS_EnsureWindowScope(state);

	// Format text is not supported for security reasons
	const char* const text = luaL_checkstring(state, 1);
	ImGui::TextUnformatted(text);
	return 0;
}

struct ImGuiEnumValue
{
	const char* name;
	int value;
};

static void LS_InitImGuiEnum(lua_State* state, const char* name, const ImGuiEnumValue* values, size_t valuecount)
{
	assert(lua_gettop(state) > 0);  // imgui table must be on top of the stack

	lua_pushstring(state, name);
	lua_createtable(state, 0, valuecount);

	for (size_t i = 0; i < valuecount; ++i)
	{
		const ImGuiEnumValue& value = values[i];
		lua_pushstring(state, value.name);
		lua_pushnumber(state, value.value);
		lua_rawset(state, -3);
	}

	lua_rawset(state, -3);  // add enum table to imgui table
}

static void LS_InitImGuiEnums(lua_State* state)
{
	assert(lua_gettop(state) == 1);

#define EXP_IMGUI_ENUM_BEGIN() \
	{ static const ImGuiEnumValue values[] = {

#define EXP_IMGUI_ENUM_VALUE(ENUMNAME, VALUENAME) \
	{ #VALUENAME, ImGui##ENUMNAME##_##VALUENAME },

#define EXP_IMGUI_ENUM_END(ENUMNAME) \
	}; LS_InitImGuiEnum(state, #ENUMNAME, values, Q_COUNTOF(values)); }

	EXP_IMGUI_ENUM_BEGIN()
	EXP_IMGUI_ENUM_VALUE(Cond, None)
	EXP_IMGUI_ENUM_VALUE(Cond, Always)
	EXP_IMGUI_ENUM_VALUE(Cond, Once)
	EXP_IMGUI_ENUM_VALUE(Cond, FirstUseEver)
	EXP_IMGUI_ENUM_VALUE(Cond, Appearing)
	EXP_IMGUI_ENUM_END(Cond)

	EXP_IMGUI_ENUM_BEGIN()
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, None)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, CharsDecimal)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, CharsHexadecimal)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, CharsUppercase)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, CharsNoBlank)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, AutoSelectAll)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, EnterReturnsTrue)
	// TODO: Input text callback support
	//EXP_IMGUI_ENUM_VALUE(InputTextFlags, CallbackCompletion)
	//EXP_IMGUI_ENUM_VALUE(InputTextFlags, CallbackHistory)
	//EXP_IMGUI_ENUM_VALUE(InputTextFlags, CallbackAlways)
	//EXP_IMGUI_ENUM_VALUE(InputTextFlags, CallbackCharFilter)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, AllowTabInput)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, CtrlEnterForNewLine)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, NoHorizontalScroll)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, AlwaysOverwrite)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, ReadOnly)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, Password)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, NoUndoRedo)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, CharsScientific)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, CallbackResize)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, CallbackEdit)
	EXP_IMGUI_ENUM_VALUE(InputTextFlags, EscapeClearsAll)
	EXP_IMGUI_ENUM_END(InputTextFlags)

	EXP_IMGUI_ENUM_BEGIN()
	EXP_IMGUI_ENUM_VALUE(SelectableFlags, None)
	EXP_IMGUI_ENUM_VALUE(SelectableFlags, DontClosePopups)
	EXP_IMGUI_ENUM_VALUE(SelectableFlags, SpanAllColumns)
	EXP_IMGUI_ENUM_VALUE(SelectableFlags, AllowDoubleClick)
	EXP_IMGUI_ENUM_VALUE(SelectableFlags, Disabled)
	EXP_IMGUI_ENUM_VALUE(SelectableFlags, AllowOverlap)
	EXP_IMGUI_ENUM_END(SelectableFlags)

	EXP_IMGUI_ENUM_BEGIN()
	EXP_IMGUI_ENUM_VALUE(TableFlags, None)
	EXP_IMGUI_ENUM_VALUE(TableFlags, Resizable)
	EXP_IMGUI_ENUM_VALUE(TableFlags, Reorderable)
	EXP_IMGUI_ENUM_VALUE(TableFlags, Hideable)
	EXP_IMGUI_ENUM_VALUE(TableFlags, Sortable)
	EXP_IMGUI_ENUM_VALUE(TableFlags, NoSavedSettings)
	EXP_IMGUI_ENUM_VALUE(TableFlags, ContextMenuInBody)
	EXP_IMGUI_ENUM_VALUE(TableFlags, RowBg)
	EXP_IMGUI_ENUM_VALUE(TableFlags, BordersInnerH)
	EXP_IMGUI_ENUM_VALUE(TableFlags, BordersOuterH)
	EXP_IMGUI_ENUM_VALUE(TableFlags, BordersInnerV)
	EXP_IMGUI_ENUM_VALUE(TableFlags, BordersOuterV)
	EXP_IMGUI_ENUM_VALUE(TableFlags, BordersH)
	EXP_IMGUI_ENUM_VALUE(TableFlags, BordersV)
	EXP_IMGUI_ENUM_VALUE(TableFlags, BordersInner)
	EXP_IMGUI_ENUM_VALUE(TableFlags, BordersOuter)
	EXP_IMGUI_ENUM_VALUE(TableFlags, Borders)
	EXP_IMGUI_ENUM_VALUE(TableFlags, NoBordersInBody)
	EXP_IMGUI_ENUM_VALUE(TableFlags, NoBordersInBodyUntilResize)
	EXP_IMGUI_ENUM_VALUE(TableFlags, SizingFixedFit)
	EXP_IMGUI_ENUM_VALUE(TableFlags, SizingFixedSame)
	EXP_IMGUI_ENUM_VALUE(TableFlags, SizingStretchProp)
	EXP_IMGUI_ENUM_VALUE(TableFlags, SizingStretchSame)
	EXP_IMGUI_ENUM_VALUE(TableFlags, NoHostExtendX)
	EXP_IMGUI_ENUM_VALUE(TableFlags, NoHostExtendY)
	EXP_IMGUI_ENUM_VALUE(TableFlags, NoKeepColumnsVisible)
	EXP_IMGUI_ENUM_VALUE(TableFlags, PreciseWidths)
	EXP_IMGUI_ENUM_VALUE(TableFlags, NoClip)
	EXP_IMGUI_ENUM_VALUE(TableFlags, PadOuterX)
	EXP_IMGUI_ENUM_VALUE(TableFlags, NoPadOuterX)
	EXP_IMGUI_ENUM_VALUE(TableFlags, NoPadInnerX)
	EXP_IMGUI_ENUM_VALUE(TableFlags, ScrollX)
	EXP_IMGUI_ENUM_VALUE(TableFlags, ScrollY)
	EXP_IMGUI_ENUM_VALUE(TableFlags, SortMulti)
	EXP_IMGUI_ENUM_VALUE(TableFlags, SortTristate)
	EXP_IMGUI_ENUM_VALUE(TableFlags, HighlightHoveredColumn)
	EXP_IMGUI_ENUM_END(TableFlags)

	EXP_IMGUI_ENUM_BEGIN()
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, None)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, Disabled)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, DefaultHide)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, DefaultSort)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, WidthStretch)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, WidthFixed)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, NoResize)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, NoReorder)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, NoHide)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, NoClip)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, NoSort)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, NoSortAscending)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, NoSortDescending)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, NoHeaderLabel)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, NoHeaderWidth)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, PreferSortAscending)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, PreferSortDescending)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, IndentEnable)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, IndentDisable)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, AngledHeader)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, IsEnabled)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, IsVisible)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, IsSorted)
	EXP_IMGUI_ENUM_VALUE(TableColumnFlags, IsHovered)
	EXP_IMGUI_ENUM_END(TableColumnFlags)

	EXP_IMGUI_ENUM_BEGIN()
	EXP_IMGUI_ENUM_VALUE(WindowFlags, None)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoTitleBar)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoResize)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoMove)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoScrollbar)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoScrollWithMouse)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoCollapse)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, AlwaysAutoResize)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoBackground)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoSavedSettings)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoMouseInputs)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, MenuBar)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, HorizontalScrollbar)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoFocusOnAppearing)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoBringToFrontOnFocus)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, AlwaysVerticalScrollbar)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, AlwaysHorizontalScrollbar)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoNavInputs)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoNavFocus)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, UnsavedDocument)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoDocking)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoNav)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoDecoration)
	EXP_IMGUI_ENUM_VALUE(WindowFlags, NoInputs)
	EXP_IMGUI_ENUM_END(WindowFlags)

#undef EXP_IMGUI_ENUM_END
#undef EXP_IMGUI_ENUM_VALUE
#undef EXP_IMGUI_ENUM_BEGIN
}

static void LS_InitImGuiBindings(lua_State* state)
{
	static const luaL_Reg functions[] =
	{
		{ "GetMainViewport", LS_global_imgui_GetMainViewport },
		{ "SetClipboardText", LS_global_imgui_SetClipboardText },
#ifndef NDEBUG
		{ "ShowDemoWindow", LS_global_imgui_ShowDemoWindow },
#endif // !NDEBUG

		{ "Begin", LS_global_imgui_Begin },
		{ "End", LS_global_imgui_End },
		{ "GetWindowContentRegionMax", LS_global_imgui_GetWindowContentRegionMax },
		{ "SameLine", LS_global_imgui_SameLine },
		{ "SetNextWindowFocus", LS_global_imgui_SetNextWindowFocus },
		{ "SetNextWindowPos", LS_global_imgui_SetNextWindowPos },
		{ "SetNextWindowSize", LS_global_imgui_SetNextWindowSize },

		{ "BeginPopupContextItem", LS_global_imgui_BeginPopupContextItem },
		{ "EndPopup", LS_global_imgui_EndPopup },

		{ "BeginTable", LS_global_imgui_BeginTable },
		{ "EndTable", LS_global_imgui_EndTable },
		{ "TableHeadersRow", LS_global_imgui_TableHeadersRow },
		{ "TableNextColumn", LS_global_imgui_TableNextColumn },
		{ "TableNextRow", LS_global_imgui_TableNextRow },
		{ "TableSetupColumn", LS_global_imgui_TableSetupColumn },

		{ "Button", LS_global_imgui_Button },
		{ "GetItemRectMax", LS_global_imgui_GetItemRectMax },
		{ "GetItemRectMin", LS_global_imgui_GetItemRectMin },
		{ "InputTextMultiline", LS_global_imgui_InputTextMultiline },
		{ "Selectable", LS_global_imgui_Selectable },
		{ "Separator", LS_global_imgui_Separator },
		{ "SeparatorText", LS_global_imgui_SeparatorText },
		{ "Spacing", LS_global_imgui_Spacing },
		{ "Text", LS_global_imgui_Text },

		// ...

		{ nullptr, nullptr }
	};

	luaL_newlib(state, functions);
	lua_pushvalue(state, 1);  // copy of imgui table for addition of enums
	lua_setglobal(state, "imgui");
	LS_InitImGuiEnums(state);
	lua_pop(state, 1);  // remove imgui table
}


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

static void LS_InitExpMode()
{
	lua_State* state = LS_GetState();
	assert(state);

	const bool initialized = lua_getglobal(state, ls_expmode_name) != LUA_TNIL;
	lua_pop(state, 1);  // remove 'expmode' table or nil

	if (initialized)
		return;

	LS_InitImGuiBindings(state);

	// Register 'expmode' table
	lua_createtable(state, 0, 16);
	lua_setglobal(state, ls_expmode_name);

	LS_LoadScript(state, "scripts/expmode.lua");
}

#endif // USE_LUA_SCRIPTING


static bool exp_active;
static char exp_justactived;

static SDL_EventFilter exp_eventfilter;
static void* exp_eventuserdata;

static SDL_Window* exp_window;
static SDL_GLContext exp_glcontext;
static bool exp_created;

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
	if (exp_active || cls.state != ca_connected || cl.intermission)
		return;

	if (!exp_created)
		EXP_Create();

	exp_active = true;

	// Need to skip two frames because SDL_StopTextInput() is called during the next one after ImGui activation
	// Otherwise, text input will be unavailable. See EXP_Update() for SDL_StartTextInput() call
	exp_justactived = 2;

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
	extern qboolean keydown[MAX_KEYS];
	memset(keydown, 0, sizeof keydown);

	// Remove event filter to allow mouse move events
	SDL_GetEventFilter(&exp_eventfilter, &exp_eventuserdata);
	SDL_SetEventFilter(nullptr, nullptr);

	// Enable contol of ImGui windows by all input devices
	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

#ifdef USE_LUA_SCRIPTING
	LS_InitExpMode();
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
	exp_window = window;
	exp_glcontext = context;

	Cmd_AddCommand("expmode", EXP_EnterMode);
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
	assert(!exp_framescope);
	exp_framescope = true;

	if (!LS_CallExpModeFunction("onupdate"))
		EXP_ExitMode();

	exp_framescope = false;
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
