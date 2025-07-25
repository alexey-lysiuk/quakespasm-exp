/*
Copyright (C) 2023-2025 Alexey Lysiuk

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

static int LS_value_ImGuiStyle_index(lua_State* state)
{
	LS_EnsureFrameScope(state);

	return LS_ImGuiTypeOperatorIndex(state, ls_imguistyle_type, ls_imguistyle_members, Q_COUNTOF(ls_imguistyle_members));
}

static int LS_global_imgui_GetStyle(lua_State* state)
{
	LS_EnsureFrameScope(state);

	ImGuiStyle*& style = ls_imguistyle_type.New(state);
	style = &ImGui::GetStyle();

	// Create and set 'ImGuiStyle' metatable
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_value_ImGuiStyle_index },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "ImGuiStyle"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
	return 1;
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

static int LS_global_imgui_GetVersion(lua_State* state)
{
	const char* const version = ImGui::GetVersion();
	lua_pushstring(state, version);
	return 1;
}

static int LS_global_imgui_Begin(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const name = LS_CheckImGuiName(state);
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
	++ls_windowscope;

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

static int LS_global_imgui_BeginChild(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const name = LS_CheckImGuiName(state);
	const LS_Vector2 size = luaL_opt(state, LS_GetVectorValue<2>, 2, LS_Vector2::Zero());
	const int childflags = luaL_optinteger(state, 3, 0);
	const int windowflags = luaL_optinteger(state, 4, 0);

	const bool visible = ImGui::BeginChild(name, ToImVec2(size), childflags, windowflags);
	lua_pushboolean(state, visible);

	LS_AddToImGuiStack(LS_EndChildWindowScope);
	++ls_childwindowscope;

	return 1;
}

static int LS_global_imgui_EndChild(lua_State* state)
{
	LS_EnsureChildWindowScope(state);

	LS_RemoveFromImGuiStack(state, LS_EndChildWindowScope);
	return 0;
}

static int LS_global_imgui_IsWindowAppearing(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const bool appearing = ImGui::IsWindowAppearing();
	lua_pushboolean(state, appearing);
	return 1;
}

static int LS_global_imgui_IsWindowFocused(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const int flags = luaL_optinteger(state, 1, 0);
	const bool focused = ImGui::IsWindowFocused(flags);

	lua_pushboolean(state, focused);
	return 1;
}

static int LS_global_imgui_GetWindowPos(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const LS_Vector2 pos = FromImVec2(ImGui::GetWindowPos());
	return LS_PushVectorValue(state, pos);
}

static int LS_global_imgui_GetWindowSize(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const LS_Vector2 size = FromImVec2(ImGui::GetWindowSize());
	return LS_PushVectorValue(state, size);
}

static int LS_global_imgui_GetWindowWidth(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const float width = ImGui::GetWindowWidth();
	lua_pushnumber(state, width);
	return 1;
}

static int LS_global_imgui_GetWindowHeight(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const float height = ImGui::GetWindowHeight();
	lua_pushnumber(state, height);
	return 1;
}

static int LS_global_imgui_SetNextWindowPos(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const LS_Vector2 pos = LS_GetVectorValue<2>(state, 1);
	const int cond = luaL_optinteger(state, 2, 0);
	const LS_Vector2 pivot = luaL_opt(state, LS_GetVectorValue<2>, 3, LS_Vector2::Zero());

	ImGui::SetNextWindowPos(ToImVec2(pos), cond, ToImVec2(pivot));
	return 0;
}

static int LS_global_imgui_SetNextWindowSize(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const LS_Vector2 size = LS_GetVectorValue<2>(state, 1);
	const int cond = luaL_optinteger(state, 2, 0);

	ImGui::SetNextWindowSize(ToImVec2(size), cond);
	return 0;
}

static int LS_global_imgui_SetNextWindowSizeConstraints(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const LS_Vector2 minsize = LS_GetVectorValue<2>(state, 1);
	const LS_Vector2 maxsize = LS_GetVectorValue<2>(state, 2);
	// TODO: add support for callback

	ImGui::SetNextWindowSizeConstraints(ToImVec2(minsize), ToImVec2(maxsize));
	return 0;
}

static int LS_global_imgui_SetNextWindowFocus(lua_State* state)
{
	LS_EnsureFrameScope(state);

	ImGui::SetNextWindowFocus();
	return 0;
}

static int LS_global_imgui_SetNextItemWidth(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const float width = luaL_checknumber(state, 1);
	ImGui::SetNextItemWidth(width);
	return 0;
}

static int LS_global_imgui_GetCursorScreenPos(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const LS_Vector2 pos = FromImVec2(ImGui::GetCursorScreenPos());
	return LS_PushVectorValue(state, pos);
}

static int LS_global_imgui_SetCursorScreenPos(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const LS_Vector2 pos = LS_GetVectorValue<2>(state, 1);
	ImGui::SetCursorPos(ToImVec2(pos));
	return 0;
}

static int LS_global_imgui_GetContentRegionAvail(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const LS_Vector2 pos = FromImVec2(ImGui::GetContentRegionAvail());
	return LS_PushVectorValue(state, pos);
}

static int LS_global_imgui_GetCursorPos(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const LS_Vector2 pos = FromImVec2(ImGui::GetCursorPos());
	return LS_PushVectorValue(state, pos);
}

static int LS_global_imgui_GetCursorPosX(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const float posx = ImGui::GetCursorPosX();
	lua_pushnumber(state, posx);
	return 1;
}

static int LS_global_imgui_GetCursorPosY(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const float posy = ImGui::GetCursorPosY();
	lua_pushnumber(state, posy);
	return 1;
}

static int LS_global_imgui_Separator(lua_State* state)
{
	LS_EnsureFrameScope(state);

	ImGui::Separator();
	return 0;
}

static int LS_global_imgui_SameLine(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const float offset = luaL_optnumber(state, 1, 0.f);
	const float spacing = luaL_optnumber(state, 2, -1.f);

	ImGui::SameLine(offset, spacing);
	return 0;
}

static int LS_global_imgui_Spacing(lua_State* state)
{
	LS_EnsureFrameScope(state);

	ImGui::Spacing();
	return 0;
}

static int LS_global_imgui_Indent(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const float indentw = luaL_optnumber(state, 1, 0.f);
	ImGui::Indent(indentw);
	return 0;
}

static int LS_global_imgui_Unindent(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const float indentw = luaL_optnumber(state, 1, 0.f);
	ImGui::Unindent(indentw);
	return 0;
}

static int LS_global_imgui_AlignTextToFramePadding(lua_State* state)
{
	LS_EnsureFrameScope(state);

	ImGui::AlignTextToFramePadding();
	return 0;
}

static int LS_global_imgui_Text(lua_State* state)
{
	LS_EnsureFrameScope(state);

	// Format text is not supported for security reasons
	const char* const text = luaL_checkstring(state, 1);
	ImGui::TextUnformatted(text);
	return 0;
}

static int LS_global_imgui_BulletText(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const text = luaL_checkstring(state, 1);
	ImGui::BulletText("%s", text);
	return 0;
}

static int LS_global_imgui_SeparatorText(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = LS_CheckImGuiName(state);
	ImGui::SeparatorText(label);
	return 0;
}

static int LS_global_imgui_Button(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = LS_CheckImGuiName(state);
	const LS_Vector2 size = luaL_opt(state, LS_GetVectorValue<2>, 2, LS_Vector2::Zero());

	const bool result = ImGui::Button(label, ToImVec2(size));
	lua_pushboolean(state, result);
	return 1;
}

static int LS_global_imgui_Checkbox(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = LS_CheckImGuiName(state);
	luaL_checktype(state, 2, LUA_TBOOLEAN);
	bool value = lua_toboolean(state, 2);

	const bool pressed = ImGui::Checkbox(label, &value);
	lua_pushboolean(state, pressed);
	lua_pushboolean(state, value);
	return 2;
}

static int LS_global_imgui_Image(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const ImTextureID texID = luaL_checkinteger(state, 1);
	const LS_Vector2 size = luaL_opt(state, LS_GetVectorValue<2>, 2, LS_Vector2::Zero());
	const LS_Vector2 uv0 = luaL_opt(state, LS_GetVectorValue<2>, 3, LS_Vector2::Zero());
	const LS_Vector2 uv1 = luaL_opt(state, LS_GetVectorValue<2>, 4, LS_Vector2::One());

	ImGui::Image(texID, ToImVec2(size), ToImVec2(uv0), ToImVec2(uv1));
	return 0;
}

static int LS_global_imgui_ImageWithBg(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const ImTextureID texID = luaL_checkinteger(state, 1);
	const LS_Vector2 size = luaL_opt(state, LS_GetVectorValue<2>, 2, LS_Vector2::Zero());
	const LS_Vector2 uv0 = luaL_opt(state, LS_GetVectorValue<2>, 3, LS_Vector2::Zero());
	const LS_Vector2 uv1 = luaL_opt(state, LS_GetVectorValue<2>, 4, LS_Vector2::One());
	const LS_Vector4 tintColor = luaL_opt(state, LS_GetVectorValue<4>, 5, LS_Vector4::One());
	const LS_Vector4 borderColor = luaL_opt(state, LS_GetVectorValue<4>, 6, LS_Vector4::Zero());

	ImGui::ImageWithBg(texID, ToImVec2(size), ToImVec2(uv0), ToImVec2(uv1), ToImVec4(tintColor), ToImVec4(borderColor));
	return 0;
}

static int LS_global_imgui_BeginCombo(lua_State* state)
{
	LS_EnsureFrameScope(state);

	if (ls_comboscope)
		luaL_error(state, "calling BeginCombo() twice");

	const char* const label = LS_CheckImGuiName(state);
	const char* const preview_value = luaL_checkstring(state, 2);
	const ImGuiComboFlags flags = luaL_optinteger(state, 3, 0);

	const bool opened = ImGui::BeginCombo(label, preview_value, flags);

	if (opened)
	{
		LS_AddToImGuiStack(LS_EndComboScope);
		ls_comboscope = true;
	}

	lua_pushboolean(state, opened);
	return 1;
}

static int LS_global_imgui_EndCombo(lua_State* state)
{
	if (!ls_comboscope)
		luaL_error(state, "calling EndCombo() without BeginCombo()");

	LS_RemoveFromImGuiStack(state, LS_EndComboScope);
	return 1;
}

static int LS_global_imgui_SliderFloat(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = LS_CheckImGuiName(state);
	float value = luaL_checknumber(state, 2);
	const float minvalue = luaL_checknumber(state, 3);
	const float maxvalue = luaL_checknumber(state, 4);
	const char* const format = "%.2f";
	// TODO: Support for format needs a validation to prohibit things like "%" or "%n"
	// const char* const format = VALIDATE(luaL_optstring(state, 5, "%.3f"));
	const int flags = luaL_optinteger(state, 5, 0);

	const bool changed = ImGui::SliderFloat(label, &value, minvalue, maxvalue, format, flags);
	lua_pushboolean(state, changed);

	if (changed)
		lua_pushnumber(state, value);

	return changed ? 2 : 1;
}

static int LS_global_imgui_SliderInt(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = LS_CheckImGuiName(state);
	int value = luaL_checkinteger(state, 2);
	const float minvalue = luaL_checkinteger(state, 3);
	const float maxvalue = luaL_checkinteger(state, 4);
	const char* const format = "%d";
	// TODO: Support for format needs a validation to prohibit things like "%" or "%n"
	// const char* const format = VALIDATE(luaL_optstring(state, 5, "%d"));
	const int flags = luaL_optinteger(state, 5, 0);

	const bool changed = ImGui::SliderInt(label, &value, minvalue, maxvalue, format, flags);
	lua_pushboolean(state, changed);

	if (changed)
		lua_pushinteger(state, value);

	return changed ? 2 : 1;
}

static int LS_global_imgui_SmallButton(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = LS_CheckImGuiName(state);
	const bool result = ImGui::SmallButton(label);

	lua_pushboolean(state, result);
	return 1;
}

static int LS_global_imgui_InputText(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = LS_CheckImGuiName(state);
	LS_TextBuffer& textbuffer = ls_imguitextbuffer_type.GetValue(state, 2);
	const int flags = luaL_optinteger(state, 3, 0);

	// TODO: Input text callback support
	const bool changed = ImGui::InputText(label, textbuffer.data, textbuffer.size, flags);
	lua_pushboolean(state, changed);
	return 1;
}

static int LS_global_imgui_InputTextMultiline(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = LS_CheckImGuiName(state);
	LS_TextBuffer& textbuffer = ls_imguitextbuffer_type.GetValue(state, 2);
	const LS_Vector2 size = luaL_opt(state, LS_GetVectorValue<2>, 3, LS_Vector2::Zero());
	const int flags = luaL_optinteger(state, 4, 0);

	// TODO: Input text callback support
	const bool changed = ImGui::InputTextMultiline(label, textbuffer.data, textbuffer.size, ToImVec2(size), flags);
	lua_pushboolean(state, changed);
	return 1;
}

static int LS_global_imgui_Selectable(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = LS_CheckImGuiName(state);
	const bool selected = luaL_opt(state, lua_toboolean, 2, false);
	const int flags = luaL_optinteger(state, 3, 0);
	const LS_Vector2 size = luaL_opt(state, LS_GetVectorValue<2>, 4, LS_Vector2::Zero());

	const bool pressed = ImGui::Selectable(label, selected, flags, ToImVec2(size));
	lua_pushboolean(state, pressed);
	return 1;
}

static int LS_global_imgui_SetTooltip(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const text = luaL_checkstring(state, 1);
	ImGui::SetTooltip("%s", text);
	return 0;
}

static int LS_global_imgui_BeginMenuBar(lua_State* state)
{
	LS_EnsureFrameScope(state);

	if (ls_menubarscope)
		luaL_error(state, "calling BeginMenuBar() twice");

	const bool opened = ImGui::BeginMenuBar();

	if (opened)
	{
		LS_AddToImGuiStack(LS_EndMenuBarScope);
		ls_menubarscope = true;
	}

	lua_pushboolean(state, opened);
	return 1;
}

static int LS_global_imgui_EndMenuBar(lua_State* state)
{
	if (!ls_menubarscope)
		luaL_error(state, "calling EndMenuBar() without BeginMenuBar()");

	LS_RemoveFromImGuiStack(state, LS_EndMenuBarScope);
	return 1;
}

static int LS_global_imgui_BeginMainMenuBar(lua_State* state)
{
	LS_EnsureFrameScope(state);

	if (ls_mainmenubarscope)
		luaL_error(state, "calling BeginMainMenuBar() twice");

	const bool opened = ImGui::BeginMainMenuBar();

	if (opened)
	{
		LS_AddToImGuiStack(LS_EndMainMenuBarScope);
		ls_mainmenubarscope = true;
	}

	lua_pushboolean(state, opened);
	return 1;
}

static int LS_global_imgui_EndMainMenuBar(lua_State* state)
{
	if (!ls_mainmenubarscope)
		luaL_error(state, "calling EndMainMenuBar() without BeginMainMenuBar()");

	LS_RemoveFromImGuiStack(state, LS_EndMainMenuBarScope);
	return 1;
}

static int LS_global_imgui_BeginMenu(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = LS_CheckImGuiName(state);
	const bool enabled = luaL_opt(state, lua_toboolean, 2, true);
	const bool opened = ImGui::BeginMenu(label, enabled);

	if (opened)
	{
		LS_AddToImGuiStack(LS_EndMenuScope);
		++ls_menuscope;
	}

	lua_pushboolean(state, opened);
	return 1;
}

static int LS_global_imgui_EndMenu(lua_State* state)
{
	if (ls_menuscope == 0)
		luaL_error(state, "calling EndMenu() without BeginMenu()");

	LS_RemoveFromImGuiStack(state, LS_EndMenuScope);
	return 0;
}

static int LS_global_imgui_MenuItem(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = LS_CheckImGuiName(state);
	const char* const shortcut = luaL_optstring(state, 2, nullptr);
	const bool selected = luaL_opt(state, lua_toboolean, 3, false);
	const bool enabled = luaL_opt(state, lua_toboolean, 4, true);
	const bool pressed = ImGui::MenuItem(label, shortcut, selected, enabled);

	lua_pushboolean(state, pressed);
	return 1;
}

static int LS_global_imgui_BeginPopup(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const strid = LS_CheckImGuiName(state);
	const int flags = luaL_optinteger(state, 2, 0);
	const bool visible = ImGui::BeginPopup(strid, flags);

	lua_pushboolean(state, visible);
	lua_pushboolean(state, visible);

	if (visible)
	{
		LS_AddToImGuiStack(LS_EndPopupScope);
		++ls_popupscope;
	}

	return 1;
}

static int LS_global_imgui_EndPopup(lua_State* state)
{
	LS_EnsurePopupScope(state);

	LS_RemoveFromImGuiStack(state, LS_EndPopupScope);
	return 0;
}

static int LS_global_imgui_OpenPopup(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* strid = LS_CheckImGuiName(state);
	const int flags = luaL_optinteger(state, 2, 0);

	ImGui::OpenPopup(strid, flags);
	return 0;
}

static int LS_global_imgui_BeginPopupContextItem(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* strid = luaL_optlstring(state, 1, nullptr, nullptr);
	const int flags = luaL_optinteger(state, 2, 1);

	const bool visible = ImGui::BeginPopupContextItem(strid, flags);
	lua_pushboolean(state, visible);

	if (visible)
	{
		LS_AddToImGuiStack(LS_EndPopupScope);
		++ls_popupscope;
	}

	return 1;
}


static int LS_global_imgui_BeginTable(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* strid = LS_CheckImGuiName(state);
	const int columncount = luaL_checkinteger(state, 2);
	const int flags = luaL_optinteger(state, 3, 0);

	const LS_Vector2 outersize = luaL_opt(state, LS_GetVectorValue<2>, 4, LS_Vector2::Zero());
	const float innerwidth = luaL_optnumber(state, 5, 0.f);

	const bool visible = ImGui::BeginTable(strid, columncount, flags, ToImVec2(outersize), innerwidth);
	lua_pushboolean(state, visible);

	if (visible)
	{
		LS_AddToImGuiStack(LS_EndTableScope);
		++ls_tablescope;
	}

	return 1;
}

static int LS_global_imgui_EndTable(lua_State* state)
{
	LS_EnsureTableScope(state);

	LS_RemoveFromImGuiStack(state, LS_EndTableScope);
	return 0;
}

static int LS_global_imgui_TableNextRow(lua_State* state)
{
	LS_EnsureTableScope(state);

	const int flags = luaL_optinteger(state, 1, 0);
	const float minrowheight = luaL_optnumber(state, 2, 0.f);

	ImGui::TableNextRow(flags, minrowheight);
	return 0;
}

static int LS_global_imgui_TableNextColumn(lua_State* state)
{
	LS_EnsureTableScope(state);

	const bool visible = ImGui::TableNextColumn();
	lua_pushboolean(state, visible);
	return 1;
}

static int LS_global_imgui_TableSetColumnIndex(lua_State* state)
{
	LS_EnsureTableScope(state);

	const int column = luaL_checkinteger(state, 1);
	const bool visible = ImGui::TableSetColumnIndex(column);

	lua_pushboolean(state, visible);
	return 1;
}

static int LS_global_imgui_TableSetupColumn(lua_State* state)
{
	LS_EnsureTableScope(state);

	const char* const label = LS_CheckImGuiName(state);
	const int flags = luaL_optinteger(state, 2, 0);
	const float initwidthorweight = luaL_optnumber(state, 3, 0.f);
	const ImGuiID userid = luaL_optinteger(state, 4, 0);

	ImGui::TableSetupColumn(label, flags, initwidthorweight, userid);
	return 0;
}

static int LS_global_imgui_TableSetupScrollFreeze(lua_State* state)
{
	LS_EnsureTableScope(state);

	const int columns = luaL_checkinteger(state, 1);
	const int rows = luaL_checkinteger(state, 2);

	ImGui::TableSetupScrollFreeze(columns, rows);
	return 0;
}

static int LS_global_imgui_TableHeader(lua_State* state)
{
	LS_EnsureTableScope(state);

	const char* const label = LS_CheckImGuiName(state);
	ImGui::TableHeader(label);
	return 0;
}

static int LS_global_imgui_TableHeadersRow(lua_State* state)
{
	LS_EnsureTableScope(state);

	ImGui::TableHeadersRow();
	return 0;
}

static int LS_global_imgui_TableGetColumnFlags(lua_State* state)
{
	LS_EnsureTableScope(state);

	const int columnindex = luaL_optinteger(state, 1, -1);
	const ImGuiTableColumnFlags flags = ImGui::TableGetColumnFlags(columnindex);

	lua_pushinteger(state, flags);
	return 1;
}

static int LS_global_imgui_GetColumnWidth(lua_State* state)
{
	LS_EnsureTableScope(state);

	const int columnindex = luaL_optinteger(state, 1, -1);
	const float width = ImGui::GetColumnWidth(columnindex);

	lua_pushnumber(state, width);
	return 1;
}

static int LS_global_imgui_SetItemDefaultFocus(lua_State* state)
{
	LS_EnsureFrameScope(state);

	ImGui::SetItemDefaultFocus();
	return 0;
}

static int LS_global_imgui_SetKeyboardFocusHere(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const int offset = luaL_optinteger(state, 1, 0);
	ImGui::SetKeyboardFocusHere(offset);
	return 0;
}

static int LS_global_imgui_IsItemHovered(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const int flags = luaL_optinteger(state, 1, 0);
	const bool hovered = ImGui::IsItemHovered(flags);

	lua_pushboolean(state, hovered);
	return 1;
}

static int LS_global_imgui_IsItemClicked(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const int button = luaL_checkinteger(state, 1);
	const bool clicked = ImGui::IsItemClicked(button);

	lua_pushboolean(state, clicked);
	return 1;
}

static int LS_global_imgui_IsAnyItemHovered(lua_State* state)
{
	LS_EnsureFrameScope(state);

	lua_pushboolean(state, ImGui::IsAnyItemHovered());
	return 1;
}

static int LS_global_imgui_GetItemRectMin(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const LS_Vector2 rect = FromImVec2(ImGui::GetItemRectMin());
	return LS_PushVectorValue(state, rect);
}

static int LS_global_imgui_GetItemRectMax(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const LS_Vector2 rect = FromImVec2(ImGui::GetItemRectMax());
	return LS_PushVectorValue(state, rect);
}

static int LS_value_ImGuiViewport_index(lua_State* state)
{
	LS_EnsureFrameScope(state);

	return LS_ImGuiTypeOperatorIndex(state, ls_imguiviewport_type, ls_imguiviewport_members, Q_COUNTOF(ls_imguiviewport_members));
}

static int LS_global_imgui_GetMainViewport(lua_State* state)
{
	LS_EnsureFrameScope(state);

	ImGuiViewport*& viewport = ls_imguiviewport_type.New(state);
	viewport = ImGui::GetMainViewport();

	// Create and set 'ImGuiViewport' metatable
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_value_ImGuiViewport_index },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "ImGuiViewport"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
	return 1;
}

static int LS_global_imgui_CalcTextSize(lua_State* state)
{
	LS_EnsureFrameScope(state);

	size_t length;
	const char* text = luaL_checklstring(state, 1, &length);

	const bool hideafterhashes = luaL_opt(state, lua_toboolean, 2, false);
	const float wrapwidth = luaL_optnumber(state, 3, -1.f);

	const LS_Vector2 textsize = FromImVec2(ImGui::CalcTextSize(text, text + length, hideafterhashes, wrapwidth));
	return LS_PushVectorValue(state, textsize);
}

static int LS_global_imgui_IsKeyDown(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const ImGuiKey key { luaL_checkinteger(state, 1) };
	const bool isdown = ImGui::IsKeyDown(key);
	lua_pushboolean(state, isdown);
	return 1;
}

static int LS_global_imgui_IsKeyPressed(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const ImGuiKey key { luaL_checkinteger(state, 1) };
	const bool repeat = luaL_opt(state, lua_toboolean, 2, true);

	const bool ispressed = ImGui::IsKeyPressed(key, repeat);
	lua_pushboolean(state, ispressed);
	return 1;
}

static int LS_global_imgui_IsKeyReleased(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const ImGuiKey key { luaL_checkinteger(state, 1) };
	const bool isreleased = ImGui::IsKeyReleased(key);
	lua_pushboolean(state, isreleased);
	return 1;
}

static int LS_global_imgui_IsMouseReleased(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const int flags = luaL_checkinteger(state, 1);
	const bool result = ImGui::IsMouseReleased(flags);

	lua_pushboolean(state, result);
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

static void LS_InitImGuiFuncs(lua_State* state)
{
	assert(lua_gettop(state) == 0);

	static const luaL_Reg functions[] =
	{
		{ "TextBuffer", LS_global_imgui_TextBuffer },

		// Context creation and access
		// * should not be exposed

		// Main
		{ "GetStyle", LS_global_imgui_GetStyle },
		// * should not be exposed

		// Demo, Debug, Information
#ifndef NDEBUG
		{ "ShowDemoWindow", LS_global_imgui_ShowDemoWindow },
		// * should not be exposed
#endif // !NDEBUG
		{ "GetVersion", LS_global_imgui_GetVersion },

		// Styles
		// * StyleColorsDark
		// * StyleColorsLight
		// * StyleColorsClassic

		// Windows
		{ "Begin", LS_global_imgui_Begin },
		{ "End", LS_global_imgui_End },

		// Child Windows
		{ "BeginChild", LS_global_imgui_BeginChild },
		{ "EndChild", LS_global_imgui_EndChild },

		// Windows Utilities
		{ "IsWindowAppearing", LS_global_imgui_IsWindowAppearing },
		// * IsWindowCollapsed
		{ "IsWindowFocused", LS_global_imgui_IsWindowFocused },
		// * IsWindowHovered
		// * GetWindowDrawList
		{ "GetWindowPos", LS_global_imgui_GetWindowPos },
		{ "GetWindowSize", LS_global_imgui_GetWindowSize },
		{ "GetWindowWidth", LS_global_imgui_GetWindowWidth },
		{ "GetWindowHeight", LS_global_imgui_GetWindowHeight },

		// Window manipulation
		{ "SetNextWindowPos", LS_global_imgui_SetNextWindowPos },
		{ "SetNextWindowSize", LS_global_imgui_SetNextWindowSize },
		{ "SetNextWindowSizeConstraints", LS_global_imgui_SetNextWindowSizeConstraints },
		// * SetNextWindowContentSize
		// * SetNextWindowCollapsed
		{ "SetNextWindowFocus", LS_global_imgui_SetNextWindowFocus },
		// * SetNextWindowScroll
		// * SetNextWindowBgAlpha
		// * SetWindowPos
		// * SetWindowSize
		// * SetWindowCollapsed
		// * SetWindowFocus
		// * SetWindowPos
		// * SetWindowSize
		// * SetWindowCollapsed
		// * SetWindowFocus

		// Windows Scrolling
		// * GetScrollX
		// * GetScrollY
		// * SetScrollX
		// * SetScrollY
		// * GetScrollMaxX
		// * GetScrollMaxY
		// * SetScrollHereX
		// * SetScrollHereY
		// * SetScrollFromPosX
		// * SetScrollFromPosY

		// * PushFont
		// * PopFont
		// * GetFont
		// * GetFontSize
		// * GetFontBaked

		// Parameters stacks (shared)
		// * PushStyleColor
		// * PushStyleColor
		// * PopStyleColor
		// * PushStyleVar
		// * PushStyleVar
		// * PushStyleVarX
		// * PushStyleVarY
		// * PopStyleVar
		// * PushItemFlag
		// * PopItemFlag

		// Parameters stacks (current window)
		// * PushItemWidth
		// * PopItemWidth
		{ "SetNextItemWidth", LS_global_imgui_SetNextItemWidth },
		// * CalcItemWidth
		// * PushTextWrapPos
		// * PopTextWrapPos

		// Style read access
		// * GetFontTexUvWhitePixel
		// * GetColorU32
		// * GetColorU32
		// * GetColorU32
		// * GetStyleColorVec4

		// Layout cursor positioning
		{ "GetCursorScreenPos", LS_global_imgui_GetCursorScreenPos },
		{ "SetCursorScreenPos", LS_global_imgui_SetCursorScreenPos },
		{ "GetContentRegionAvail", LS_global_imgui_GetContentRegionAvail },
		{ "GetCursorPos", LS_global_imgui_GetCursorPos },
		{ "GetCursorPosX", LS_global_imgui_GetCursorPosX },
		{ "GetCursorPosY", LS_global_imgui_GetCursorPosY },
		// * SetCursorPos
		// * SetCursorPosX
		// * SetCursorPosY
		// * GetCursorStartPos

		// Other layout functions
		{ "Separator", LS_global_imgui_Separator },
		{ "SameLine", LS_global_imgui_SameLine },
		// * NewLine
		{ "Spacing", LS_global_imgui_Spacing },
		// * Dummy
		{ "Indent", LS_global_imgui_Indent },
		{ "Unindent", LS_global_imgui_Unindent },
		// * BeginGroup
		// * EndGroup
		{ "AlignTextToFramePadding", LS_global_imgui_AlignTextToFramePadding },
		// * GetTextLineHeight
		// * GetTextLineHeightWithSpacing
		// * GetFrameHeight
		// * GetFrameHeightWithSpacing

		// ID stack/scopes
		// * PushID
		// * PopID
		// * GetID

		// Widgets: Text
		{ "Text", LS_global_imgui_Text },
		// * TextColored
		// * TextDisabled
		// * TextWrapped
		// * LabelText
		// * BulletText
		{ "BulletText", LS_global_imgui_BulletText },
		{ "SeparatorText", LS_global_imgui_SeparatorText },

		// Widgets: Main
		{ "Button", LS_global_imgui_Button },
		{ "SmallButton", LS_global_imgui_SmallButton },
		// * InvisibleButton
		// * ArrowButton
		{ "Checkbox", LS_global_imgui_Checkbox },
		// * CheckboxFlags
		// * RadioButton
		// * RadioButton
		// * ProgressBar
		// * Bullet
		// * TextLink
		// * TextLinkOpenURL

		// Widgets: Images
		{ "Image", LS_global_imgui_Image },
		{ "ImageWithBg", LS_global_imgui_ImageWithBg },
		// * ImageButton

		// Widgets: Combo Box
		{ "BeginCombo", LS_global_imgui_BeginCombo },
		{ "EndCombo", LS_global_imgui_EndCombo },
		// * Combo

		// Widgets: Drag Sliders
		// * DragFloat
		// * DragFloat2
		// * DragFloat3
		// * DragFloat4
		// * DragFloatRange2
		// * DragInt
		// * DragInt2
		// * DragInt3
		// * DragInt4
		// * DragIntRange2
		// * DragScalar
		// * DragScalarN

		// Widgets: Regular Sliders
		{ "SliderFloat", LS_global_imgui_SliderFloat },
		// * SliderFloat2
		// * SliderFloat3
		// * SliderFloat4
		// * SliderAngle
		{ "SliderInt", LS_global_imgui_SliderInt },
		// * SliderInt2
		// * SliderInt3
		// * SliderInt4
		// * SliderScalar
		// * SliderScalarN
		// * VSliderFloat
		// * VSliderInt
		// * VSliderScalar

		// Widgets: Input with Keyboard
		{ "InputText", LS_global_imgui_InputText },
		{ "InputTextMultiline", LS_global_imgui_InputTextMultiline },
		// * InputTextWithHint
		// * InputFloat
		// * InputFloat2
		// * InputFloat3
		// * InputFloat4
		// * InputInt
		// * InputInt2
		// * InputInt3
		// * InputInt4
		// * InputDouble
		// * InputScalar
		// * InputScalarN

		// Widgets: Color Editor/Picker
		// * ColorEdit3
		// * ColorEdit4
		// * ColorPicker3
		// * ColorPicker4
		// * ColorButton
		// * SetColorEditOptions

		// Widgets: Trees
		// * TreeNode
		// * TreeNodeEx
		// * TreePush
		// * TreePop
		// * GetTreeNodeToLabelSpacing
		// * CollapsingHeader
		// * SetNextItemOpen
		// * SetNextItemStorageID

		// Widgets: Selectables
		{ "Selectable", LS_global_imgui_Selectable },

		// Multi-selection system for Selectable(), Checkbox(), TreeNode() functions
		// * BeginMultiSelect
		// * EndMultiSelect
		// * SetNextItemSelectionUserData
		// * IsItemToggledSelection

		// Widgets: List Boxes
		// * BeginListBox
		// * EndListBox
		// * ListBox

		// Widgets: Data Plotting
		// * PlotLines
		// * PlotHistogram

		// Widgets: Value
		// * Value

		// Widgets: Menus
		{ "BeginMenuBar", LS_global_imgui_BeginMenuBar },
		{ "EndMenuBar", LS_global_imgui_EndMenuBar },
		{ "BeginMainMenuBar", LS_global_imgui_BeginMainMenuBar },
		{ "EndMainMenuBar", LS_global_imgui_EndMainMenuBar },
		{ "BeginMenu", LS_global_imgui_BeginMenu },
		{ "EndMenu", LS_global_imgui_EndMenu },
		{ "MenuItem", LS_global_imgui_MenuItem },

		// Tooltips
		// * BeginTooltip
		// * EndTooltip
		{ "SetTooltip", LS_global_imgui_SetTooltip },

		// Tooltips: helpers for showing a tooltip when hovering an item
		// * BeginItemTooltip
		// * SetItemTooltip

		// Popups, Modals
		{ "BeginPopup", LS_global_imgui_BeginPopup },
		// * BeginPopupModal
		{ "EndPopup", LS_global_imgui_EndPopup },

		// Popups: open/close functions
		{ "OpenPopup", LS_global_imgui_OpenPopup },
		// * OpenPopupOnItemClick
		// * CloseCurrentPopup

		// Popups: open+begin combined functions helpers
		{ "BeginPopupContextItem", LS_global_imgui_BeginPopupContextItem },
		// * BeginPopupContextWindow
		// * BeginPopupContextVoid

		// Popups: query functions
		// * IsPopupOpen

		// Tables
		{ "BeginTable", LS_global_imgui_BeginTable },
		{ "EndTable", LS_global_imgui_EndTable },
		{ "TableNextRow", LS_global_imgui_TableNextRow },
		{ "TableNextColumn", LS_global_imgui_TableNextColumn },
		{ "TableSetColumnIndex", LS_global_imgui_TableSetColumnIndex },

		// Tables: Headers & Columns declaration
		{ "TableSetupColumn", LS_global_imgui_TableSetupColumn },
		{ "TableSetupScrollFreeze", LS_global_imgui_TableSetupScrollFreeze },
		{ "TableHeader", LS_global_imgui_TableHeader },
		{ "TableHeadersRow", LS_global_imgui_TableHeadersRow },
		// * TableAngledHeadersRow

		// Tables: Sorting & Miscellaneous functions
		// * TableGetSortSpecs
		// * TableGetColumnCount
		// * TableGetColumnIndex
		// * TableGetRowIndex
		// * TableGetColumnName
		{ "TableGetColumnFlags", LS_global_imgui_TableGetColumnFlags },
		// * TableSetColumnEnabled
		// * TableGetHoveredColumn
		// * TableSetBgColor

		// Legacy Columns API
		// * Columns
		// * NextColumn
		// * GetColumnIndex
		{ "GetColumnWidth", LS_global_imgui_GetColumnWidth },
		// * SetColumnWidth
		// * GetColumnOffset
		// * SetColumnOffset
		// * GetColumnsCount

		// Tab Bars, Tabs
		// * BeginTabBar
		// * EndTabBar
		// * BeginTabItem
		// * EndTabItem
		// * TabItemButton
		// * SetTabItemClosed

		// Logging/Capture
		// * should not be exposed

		// Drag and Drop
		// * BeginDragDropSource
		// * SetDragDropPayload
		// * EndDragDropSource
		// * BeginDragDropTarget
		// * AcceptDragDropPayload
		// * EndDragDropTarget
		// * GetDragDropPayload

		// Disabling [BETA API]
		// * BeginDisabled
		// * EndDisabled

		// Clipping
		// * PushClipRect
		// * PopClipRect

		// Focus, Activation
		{ "SetItemDefaultFocus", LS_global_imgui_SetItemDefaultFocus },
		{ "SetKeyboardFocusHere", LS_global_imgui_SetKeyboardFocusHere },

		// Keyboard/Gamepad Navigation
		// * SetNavCursorVisible

		// Overlapping mode
		// * SetNextItemAllowOverlap

		// Item/Widgets Utilities and Query Functions
		{ "IsItemHovered", LS_global_imgui_IsItemHovered },
		// * IsItemActive
		// * IsItemFocused
		{ "IsItemClicked", LS_global_imgui_IsItemClicked },
		// * IsItemVisible
		// * IsItemEdited
		// * IsItemActivated
		// * IsItemDeactivated
		// * IsItemDeactivatedAfterEdit
		// * IsItemToggledOpen
		{ "IsAnyItemHovered", LS_global_imgui_IsAnyItemHovered },
		// * IsAnyItemActive
		// * IsAnyItemFocused
		// * GetItemID
		{ "GetItemRectMin", LS_global_imgui_GetItemRectMin },
		{ "GetItemRectMax", LS_global_imgui_GetItemRectMax },
		// * GetItemRectSize

		// Viewports
		{ "GetMainViewport", LS_global_imgui_GetMainViewport },

		// Background/Foreground Draw Lists
		// * GetBackgroundDrawList
		// * GetForegroundDrawList

		// Miscellaneous Utilities
		// * IsRectVisible
		// * IsRectVisible
		// * GetTime
		// * GetFrameCount
		// * GetDrawListSharedData
		// * GetStyleColorName
		// * SetStateStorage
		// * GetStateStorage

		// Text Utilities
		{ "CalcTextSize", LS_global_imgui_CalcTextSize },

		// Color Utilities
		// * ColorConvertU32ToFloat4
		// * ColorConvertFloat4ToU32
		// * ColorConvertRGBtoHSV
		// * ColorConvertHSVtoRGB

		// Inputs Utilities: Keyboard/Mouse/Gamepad
		{ "IsKeyDown", LS_global_imgui_IsKeyDown },
		{ "IsKeyPressed", LS_global_imgui_IsKeyPressed },
		{ "IsKeyReleased", LS_global_imgui_IsKeyReleased },
		// * IsKeyChordPressed
		// * GetKeyPressedAmount
		// * GetKeyName
		// * SetNextFrameWantCaptureKeyboard

		// Inputs Utilities: Shortcut Testing & Routing
		// * Shortcut
		// * SetNextItemShortcut

		// Inputs Utilities: Key/Input Ownership
		// * SetItemKeyOwner

		// Inputs Utilities: Mouse specific
		// * IsMouseDown
		// * IsMouseClicked
		{ "IsMouseReleased", LS_global_imgui_IsMouseReleased },
		// * IsMouseDoubleClicked
		// * IsMouseReleasedWithDelay
		// * GetMouseClickedCount
		// * IsMouseHoveringRect
		// * IsMousePosValid
		// * IsAnyMouseDown
		// * GetMousePos
		// * GetMousePosOnOpeningCurrentPopup
		// * IsMouseDragging
		// * GetMouseDragDelta
		// * ResetMouseDragDelta
		// * GetMouseCursor
		// * SetMouseCursor
		// * SetNextFrameWantCaptureMouse

		// Clipboard Utilities
		// * GetClipboardText
		{ "SetClipboardText", LS_global_imgui_SetClipboardText },

		// Settings/.Ini Utilities
		// * should not be exposed

		// Debug Utilities
		// * should not be exposed

		// Memory Allocators
		// * should not be exposed

		// Color Text Editor
		{ "ColorTextEdit", LS_global_imgui_ColorTextEdit },

		{ nullptr, nullptr }
	};

	luaL_newlib(state, functions);
}
