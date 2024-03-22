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

static int LS_global_imgui_IsAnyItemHovered(lua_State* state)
{
	LS_EnsureFrameScope(state);

	lua_pushboolean(state, ImGui::IsAnyItemHovered());
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

static int LS_global_imgui_IsMouseReleased(lua_State* state)
{
	LS_EnsureFrameScope(state);

	const int flags = luaL_checkinteger(state, 1);
	const bool result = ImGui::IsMouseReleased(flags);

	lua_pushboolean(state, result);
	return 1;
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

static int LS_global_imgui_GetWindowContentRegionMax(lua_State* state)
{
	LS_EnsureWindowScope(state);

	LS_PushImVec(state, ImGui::GetWindowContentRegionMax());
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

static int LS_global_imgui_SameLine(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const float offset = luaL_optnumber(state, 1, 0.f);
	const float spacing = luaL_optnumber(state, 2, -1.f);

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

static int LS_global_imgui_BeginPopup(lua_State* state)
{
	LS_EnsureWindowScope(state);

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
	LS_EnsureWindowScope(state);

	const char* strid = luaL_checkstring(state, 1);
	const int flags = luaL_optinteger(state, 2, 0);

	ImGui::OpenPopup(strid, flags);
	return 0;
}

static int LS_global_imgui_BeginTable(lua_State* state)
{
	LS_EnsureWindowScope(state);

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
	const ImVec2 size = luaL_opt(state, LS_GetImVecValue<ImVec2>, 2, ImVec2());

	const bool result = ImGui::Button(label, size);
	lua_pushboolean(state, result);
	return 1;
}

static int LS_global_imgui_GetItemRectMax(lua_State* state)
{
	LS_EnsureWindowScope(state);

	return LS_PushImVec(state, ImGui::GetItemRectMax());
}

static int LS_global_imgui_GetItemRectMin(lua_State* state)
{
	LS_EnsureWindowScope(state);

	return LS_PushImVec(state, ImGui::GetItemRectMin());
}

static int LS_global_imgui_Indent(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const float indentw = luaL_optnumber(state, 1, 0.f);
	ImGui::Indent(indentw);
	return 0;
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

	const ImVec2 size = luaL_opt(state, LS_GetImVecValue<ImVec2>, 4, ImVec2());
	const int flags = luaL_optinteger(state, 5, 0);

	// TODO: Input text callback support
	const bool changed = ImGui::InputTextMultiline(label, buf, buffersize, size, flags);
	lua_pushboolean(state, changed);

	if (changed)
		lua_pushstring(state, buf);
	else
		lua_pushvalue(state, 2);

	return 2;
}

static int LS_global_imgui_IsItemHovered(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const int flags = luaL_optinteger(state, 1, 0);
	const bool hovered = ImGui::IsItemHovered(flags);

	lua_pushboolean(state, hovered);
	return 1;
}

static int LS_global_imgui_Selectable(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const char* const label = luaL_checkstring(state, 1);
	const bool selected = luaL_opt(state, lua_toboolean, 2, false);
	const int flags = luaL_optinteger(state, 3, 0);
	const ImVec2 size = luaL_opt(state, LS_GetImVecValue<ImVec2>, 4, ImVec2());

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

static int LS_global_imgui_SetTooltip(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const char* const text = luaL_checkstring(state, 1);
	ImGui::SetTooltip("%s", text);
	return 0;
}

static int LS_global_imgui_SmallButton(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const char* const label = luaL_checkstring(state, 1);
	const bool result = ImGui::SmallButton(label);

	lua_pushboolean(state, result);
	return 1;
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

static int LS_global_imgui_Unindent(lua_State* state)
{
	LS_EnsureWindowScope(state);

	const float indentw = luaL_optnumber(state, 1, 0.f);
	ImGui::Unindent(indentw);
	return 0;
}

static void LS_InitImGuiFuncs(lua_State* state)
{
	assert(lua_gettop(state) == 0);

	static const luaL_Reg functions[] =
	{
		{ "ImVec2", LS_global_imgui_ImVec2 },
		{ "ImVec4", LS_global_imgui_ImVec4 },

		{ "CalcTextSize", LS_global_imgui_CalcTextSize },
		{ "GetMainViewport", LS_global_imgui_GetMainViewport },
		{ "GetStyle", LS_global_imgui_GetStyle },
		{ "SetClipboardText", LS_global_imgui_SetClipboardText },
#ifndef NDEBUG
		{ "ShowDemoWindow", LS_global_imgui_ShowDemoWindow },
#endif // !NDEBUG

		{ "IsAnyItemHovered", LS_global_imgui_IsAnyItemHovered },
		{ "IsItemClicked", LS_global_imgui_IsItemClicked },
		{ "IsMouseReleased", LS_global_imgui_IsMouseReleased },

		{ "Begin", LS_global_imgui_Begin },
		{ "End", LS_global_imgui_End },
		{ "GetWindowContentRegionMax", LS_global_imgui_GetWindowContentRegionMax },
		{ "IsWindowFocused", LS_global_imgui_IsWindowFocused },
		{ "SameLine", LS_global_imgui_SameLine },
		{ "SetNextWindowFocus", LS_global_imgui_SetNextWindowFocus },
		{ "SetNextWindowPos", LS_global_imgui_SetNextWindowPos },
		{ "SetNextWindowSize", LS_global_imgui_SetNextWindowSize },

		{ "BeginPopup", LS_global_imgui_BeginPopup },
		{ "BeginPopupContextItem", LS_global_imgui_BeginPopupContextItem },
		{ "EndPopup", LS_global_imgui_EndPopup },
		{ "OpenPopup", LS_global_imgui_OpenPopup },

		{ "BeginTable", LS_global_imgui_BeginTable },
		{ "EndTable", LS_global_imgui_EndTable },
		{ "TableHeadersRow", LS_global_imgui_TableHeadersRow },
		{ "TableNextColumn", LS_global_imgui_TableNextColumn },
		{ "TableNextRow", LS_global_imgui_TableNextRow },
		{ "TableSetupColumn", LS_global_imgui_TableSetupColumn },

		{ "Button", LS_global_imgui_Button },
		{ "GetItemRectMax", LS_global_imgui_GetItemRectMax },
		{ "GetItemRectMin", LS_global_imgui_GetItemRectMin },
		{ "Indent", LS_global_imgui_Indent },
		{ "InputTextMultiline", LS_global_imgui_InputTextMultiline },
		{ "IsItemHovered", LS_global_imgui_IsItemHovered },
		{ "Selectable", LS_global_imgui_Selectable },
		{ "Separator", LS_global_imgui_Separator },
		{ "SeparatorText", LS_global_imgui_SeparatorText },
		{ "SetTooltip", LS_global_imgui_SetTooltip },
		{ "SmallButton", LS_global_imgui_SmallButton },
		{ "Spacing", LS_global_imgui_Spacing },
		{ "Text", LS_global_imgui_Text },
		{ "Unindent", LS_global_imgui_Unindent },

		// ...

		{ nullptr, nullptr }
	};

	luaL_newlib(state, functions);
}
