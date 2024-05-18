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

static int LS_value_ImGuiStyle_index(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const LS_ImGuiMember member = LS_GetIndexMemberType(state, "ImGuiStyle", ls_imguistyle_members);
	return LS_ImGuiTypeOperatorIndex(state, ls_imguistyle_type, member);
}

static int LS_global_imgui_GetStyle(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const ImGuiStyle** styleptr = static_cast<const ImGuiStyle**>(LS_CreateTypedUserData(state, &ls_imguistyle_type));
	assert(styleptr && *styleptr);

	const ImGuiStyle* style = &ImGui::GetStyle();
	*styleptr = style;

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

	const ImVec2 pos = ImGui::GetWindowPos();
	LS_PushImVec(state, pos);
	return 1;
}

static int LS_global_imgui_GetWindowSize(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const ImVec2 size = ImGui::GetWindowSize();
	LS_PushImVec(state, size);
	return 1;
}

static int LS_global_imgui_GetWindowWidth(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const float width = ImGui::GetWindowWidth();
	lua_pushnumber(state, width);
	return 1;
}

static int LS_global_imgui_SetNextWindowPos(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const ImVec2 pos = LS_GetImVecValue<ImVec2>(state, 1);
	const int cond = luaL_optinteger(state, 2, 0);
	const ImVec2 pivot = luaL_opt(state, LS_GetImVecValue<ImVec2>, 3, ImVec2());

	ImGui::SetNextWindowPos(pos, cond, pivot);
	return 0;
}

static int LS_global_imgui_SetNextWindowSize(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const ImVec2 size = LS_GetImVecValue<ImVec2>(state, 1);
	const int cond = luaL_optinteger(state, 2, 0);

	ImGui::SetNextWindowSize(size, cond);
	return 0;
}

static int LS_global_imgui_SetNextWindowSizeConstraints(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const ImVec2 minsize = LS_GetImVecValue<ImVec2>(state, 1);
	const ImVec2 maxsize = LS_GetImVecValue<ImVec2>(state, 2);
	// TODO: add support for callback

	ImGui::SetNextWindowSizeConstraints(minsize, maxsize);
	return 0;
}

static int LS_global_imgui_SetNextWindowFocus(lua_State* state)
{
	LS_EnsureFrameScope(state);

	ImGui::SetNextWindowFocus();
	return 0;
}

static int LS_global_imgui_GetWindowContentRegionMax(lua_State* state)
{
	LS_EnsureFrameScope(state);

	LS_PushImVec(state, ImGui::GetWindowContentRegionMax());
	return 1;
}

static int LS_global_imgui_GetCursorPos(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const ImVec2 pos = ImGui::GetCursorPos();
	LS_PushImVec(state, pos);
	return 1;
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

static int LS_global_imgui_SeparatorText(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = luaL_checkstring(state, 1);
	ImGui::SeparatorText(label);
	return 0;
}

static int LS_global_imgui_Button(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = luaL_checkstring(state, 1);
	const ImVec2 size = luaL_opt(state, LS_GetImVecValue<ImVec2>, 2, ImVec2());

	const bool result = ImGui::Button(label, size);
	lua_pushboolean(state, result);
	return 1;
}

static int LS_global_imgui_SmallButton(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = luaL_checkstring(state, 1);
	const bool result = ImGui::SmallButton(label);

	lua_pushboolean(state, result);
	return 1;
}

static int LS_global_imgui_InputText(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* label = luaL_checkstring(state, 1);
	assert(label);

	LS_TextBuffer& textbuffer = LS_GetTextBufferValue(state, 2);
	const int flags = luaL_optinteger(state, 3, 0);

	// TODO: Input text callback support
	const bool changed = ImGui::InputText(label, textbuffer.data, textbuffer.size, flags);
	lua_pushboolean(state, changed);
	return 1;
}

static int LS_global_imgui_InputTextMultiline(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* label = luaL_checkstring(state, 1);
	assert(label);

	LS_TextBuffer& textbuffer = LS_GetTextBufferValue(state, 2);
	const ImVec2 size = luaL_opt(state, LS_GetImVecValue<ImVec2>, 3, ImVec2());
	const int flags = luaL_optinteger(state, 4, 0);

	// TODO: Input text callback support
	const bool changed = ImGui::InputTextMultiline(label, textbuffer.data, textbuffer.size, size, flags);
	lua_pushboolean(state, changed);
	return 1;
}

static int LS_global_imgui_Selectable(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const char* const label = luaL_checkstring(state, 1);
	const bool selected = luaL_opt(state, lua_toboolean, 2, false);
	const int flags = luaL_optinteger(state, 3, 0);
	const ImVec2 size = luaL_opt(state, LS_GetImVecValue<ImVec2>, 4, ImVec2());

	const bool pressed = ImGui::Selectable(label, selected, flags, size);
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

	const char* const label = luaL_checkstring(state, 1);
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

	const char* const label = luaL_checkstring(state, 1);
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

	const char* strid = luaL_checkstring(state, 1);
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

	const char* strid = luaL_checkstring(state, 1);
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

	const char* strid = luaL_checkstring(state, 1);
	const int columncount = luaL_checkinteger(state, 2);
	const int flags = luaL_optinteger(state, 3, 0);

	const ImVec2 outersize = luaL_opt(state, LS_GetImVecValue<ImVec2>, 4, ImVec2());
	const float innerwidth = luaL_optnumber(state, 5, 0.f);

	const bool visible = ImGui::BeginTable(strid, columncount, flags, outersize, innerwidth);
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

	const char* label = luaL_checkstring(state, 1);
	const int flags = luaL_optinteger(state, 2, 0);
	const float initwidthorweight = luaL_optnumber(state, 3, 0.f);
	const ImGuiID userid = luaL_optinteger(state, 4, 0);

	ImGui::TableSetupColumn(label, flags, initwidthorweight, userid);
	return 0;
}

static int LS_global_imgui_TableHeader(lua_State* state)
{
	LS_EnsureTableScope(state);

	const char* label = luaL_checkstring(state, 1);
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

	return LS_PushImVec(state, ImGui::GetItemRectMin());
}

static int LS_global_imgui_GetItemRectMax(lua_State* state)
{
	LS_EnsureFrameScope(state);

	return LS_PushImVec(state, ImGui::GetItemRectMax());
}

static int LS_value_ImGuiViewport_index(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const LS_ImGuiMember member = LS_GetIndexMemberType(state, "ImGuiViewport", ls_imguiviewport_members);
	return LS_ImGuiTypeOperatorIndex(state, ls_imguiviewport_type, member);
}

static int LS_global_imgui_GetMainViewport(lua_State* state)
{
	LS_EnsureFrameScope(state);

	ImGuiViewport** viewportptr = static_cast<ImGuiViewport**>(LS_CreateTypedUserData(state, &ls_imguiviewport_type));
	assert(viewportptr && *viewportptr);

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	assert(viewport);

	*viewportptr = viewport;

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

	const ImVec2 textsize = ImGui::CalcTextSize(text, text + length, hideafterhashes, wrapwidth);

	LS_PushImVec(state, textsize);
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
		{ "ImVec2", LS_global_imgui_ImVec2 },
		{ "ImVec4", LS_global_imgui_ImVec4 },
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

		// Styles
		// * StyleColorsDark
		// * StyleColorsLight
		// * StyleColorsClassic

		// Windows
		{ "Begin", LS_global_imgui_Begin },
		{ "End", LS_global_imgui_End },

		// Child Windows
		// * BeginChild
		// * BeginChild
		// * EndChild

		// Windows Utilities
		{ "IsWindowAppearing", LS_global_imgui_IsWindowAppearing },
		// * IsWindowCollapsed
		{ "IsWindowFocused", LS_global_imgui_IsWindowFocused },
		// * IsWindowHovered
		// * GetWindowDrawList
		{ "GetWindowPos", LS_global_imgui_GetWindowPos },
		{ "GetWindowSize", LS_global_imgui_GetWindowSize },
		{ "GetWindowWidth", LS_global_imgui_GetWindowWidth },
		// * GetWindowHeight

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
		// * SetWindowFontScale
		// * SetWindowPos
		// * SetWindowSize
		// * SetWindowCollapsed
		// * SetWindowFocus

		// Content region
		// * GetContentRegionAvail
		// * GetContentRegionMax
		// * GetWindowContentRegionMin
		{ "GetWindowContentRegionMax", LS_global_imgui_GetWindowContentRegionMax },

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

		// Parameters stacks
		// * PushFont
		// * PopFont
		// * PushStyleColor
		// * PushStyleColor
		// * PopStyleColor
		// * PushStyleVar
		// * PushStyleVar
		// * PopStyleVar
		// * PushTabStop
		// * PopTabStop
		// * PushButtonRepeat
		// * PopButtonRepeat

		// Parameters stacks
		// * PushItemWidth
		// * PopItemWidth
		// * SetNextItemWidth
		// * CalcItemWidth
		// * PushTextWrapPos
		// * PopTextWrapPos

		// Style read access
		// * GetFont
		// * GetFontSize
		// * GetFontTexUvWhitePixel
		// * GetColorU32
		// * GetColorU32
		// * GetColorU32
		// * GetStyleColorVec4

		// Layout cursor positioning
		// * GetCursorScreenPos
		// * SetCursorScreenPos
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
		{ "SeparatorText", LS_global_imgui_SeparatorText },

		// Widgets: Main
		{ "Button", LS_global_imgui_Button },
		{ "SmallButton", LS_global_imgui_SmallButton },
		// * InvisibleButton
		// * ArrowButton
		// * Checkbox
		// * CheckboxFlags
		// * CheckboxFlags
		// * RadioButton
		// * RadioButton
		// * ProgressBar
		// * Bullet

		// Widgets: Images
		// * Image
		// * ImageButton

		// Widgets: Combo Box
		// * BeginCombo
		// * EndCombo
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
		// * SliderFloat
		// * SliderFloat2
		// * SliderFloat3
		// * SliderFloat4
		// * SliderAngle
		// * SliderInt
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

		// Widgets: Selectables
		{ "Selectable", LS_global_imgui_Selectable },

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
		// * TableSetupScrollFreeze
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
		// * IsKeyDown
		// * IsKeyPressed
		// * IsKeyReleased
		// * IsKeyChordPressed
		// * GetKeyPressedAmount
		// * GetKeyName
		// * SetNextFrameWantCaptureKeyboard

		// Inputs Utilities: Mouse specific
		// * IsMouseDown
		// * IsMouseClicked
		{ "IsMouseReleased", LS_global_imgui_IsMouseReleased },
		// * IsMouseDoubleClicked
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

		{ nullptr, nullptr }
	};

	luaL_newlib(state, functions);
}
