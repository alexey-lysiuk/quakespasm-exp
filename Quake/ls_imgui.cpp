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

#if defined USE_LUA_SCRIPTING && defined USE_IMGUI

#include "ls_imgui.h"

// TODO: Find better place for this WITHOUT altering ImGui source code
#define IM_VEC4_CLASS_EXTRA \
	float& operator[] (size_t idx) { IM_ASSERT(idx >= 0 || idx <= 3); return reinterpret_cast<float*>(this)[idx]; } \
	float operator[] (size_t idx) const { return (*const_cast<ImVec4*>(this))[idx]; } \

#include "imgui.h"

extern "C"
{
#include "quakedef.h"
#include "common.h"
}

#include "frozen/string.h"
#include "frozen/unordered_map.h"


template<typename T>
const char* LS_GetImVecUserDataName();

template<typename T>
const LS_UserDataType* LS_GetImVecUserDataType();

template<typename T>
int LS_GetImVecComponentCount();

// Gets value of 'ImVec?' from userdata at given index
template<typename T>
static T& LS_GetImVecValue(lua_State* state, int index)
{
	T* value = static_cast<T*>(LS_GetValueFromTypedUserData(state, index, LS_GetImVecUserDataType<T>()));
	assert(value);

	return *value;
}

// Pushes value of 'ImVec?' component, indexed by integer [0..?] or string 'x', 'y', ...
template<typename T>
static int LS_value_ImVec_index(lua_State* state)
{
	const T& value = LS_GetImVecValue<T>(state, 1);
	int component = LS_GetVectorComponent(state, 2, LS_GetImVecComponentCount<T>());

	lua_pushnumber(state, value[component]);
	return 1;
}

// Sets new value of 'ImVec?' component, indexed by integer [0..?] or string 'x', 'y', ...
template<typename T>
static int LS_value_ImVec_newindex(lua_State* state)
{
	T& value = LS_GetImVecValue<T>(state, 1);
	int component = LS_GetVectorComponent(state, 2, LS_GetImVecComponentCount<T>());

	lua_Number compvalue = luaL_checknumber(state, 3);
	value[component] = compvalue;

	return 0;
}

// Pushes string built from 'ImVec?' value
template<typename T>
static int LS_value_ImVec_tostring(lua_State* state);

// Creates and pushes 'ImVec?' userdata built from ImVec? value
template<typename T>
static int LS_PushImVec(lua_State* state, const T& value)
{
	T* valueptr = static_cast<T*>(LS_CreateTypedUserData(state, LS_GetImVecUserDataType<T>()));
	assert(valueptr);

	*valueptr = value;

	// Create and set 'ImVec?' metatable
	static const luaL_Reg functions[] =
	{
		{ "__index", LS_value_ImVec_index<T> },
		{ "__newindex", LS_value_ImVec_newindex<T> },
		{ "__tostring", LS_value_ImVec_tostring<T> },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, LS_GetImVecUserDataName<T>()))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
	return 1;
}

#define LS_DEFINE_IMVEC_TYPE(COMPONENTS) \
	static const LS_UserDataType ls_imvec##COMPONENTS##_type = { { {{'i', 'm', 'v', '0' + COMPONENTS}} }, sizeof(int) /* fourcc */ + sizeof(ImVec##COMPONENTS) }; \
	template<> const char* LS_GetImVecUserDataName<ImVec##COMPONENTS>() { return "ImVec"#COMPONENTS; } \
	template<> const LS_UserDataType* LS_GetImVecUserDataType<ImVec##COMPONENTS>() { return &ls_imvec##COMPONENTS##_type; } \
	template<> int LS_GetImVecComponentCount<ImVec##COMPONENTS>() { return COMPONENTS; }

LS_DEFINE_IMVEC_TYPE(2)
LS_DEFINE_IMVEC_TYPE(4)

#undef LS_DEFINE_IMVEC_TYPE


template <>
int LS_value_ImVec_tostring<ImVec2>(lua_State* state)
{
	const ImVec2& value = LS_GetImVecValue<ImVec2>(state, 1);
	lua_pushfstring(state, "%f %f", value.x, value.y);
	return 1;
}

static int LS_global_imgui_ImVec2(lua_State* state)
{
	const float x = luaL_optnumber(state, 1, 0.f);
	const float y = luaL_optnumber(state, 2, 0.f);

	LS_PushImVec(state, ImVec2(x, y));
	return 1;
}


static int LS_global_imgui_ImVec4(lua_State* state)
{
	const float x = luaL_optnumber(state, 1, 0.f);
	const float y = luaL_optnumber(state, 2, 0.f);
	const float z = luaL_optnumber(state, 3, 0.f);
	const float w = luaL_optnumber(state, 4, 0.f);

	LS_PushImVec(state, ImVec4(x, y, z, w));
	return 1;
}

template <>
int LS_value_ImVec_tostring<ImVec4>(lua_State* state)
{
	const ImVec4& value = LS_GetImVecValue<ImVec4>(state, 1);
	lua_pushfstring(state, "%f %f %f %f", value.x, value.y, value.z, value.w);
	return 1;
}


enum LS_ImGuiType
{
	ImMemberType_bool,
	ImMemberType_int,
	ImMemberType_unsigned,
	ImMemberType_float,
	ImMemberType_ImVec2,
	ImMemberType_ImVec4,
};

struct LS_ImGuiMember
{
	size_t type:8;
	size_t offset:24;
};

template <typename T>
struct LS_ImGuiTypeHolder;

#define LS_IMGUI_DEFINE_MEMBER_TYPE(TYPE) \
	template <> struct LS_ImGuiTypeHolder<TYPE> { static constexpr LS_ImGuiType IMGUI_MEMBER_TYPE = ImMemberType_##TYPE; }

LS_IMGUI_DEFINE_MEMBER_TYPE(bool);
LS_IMGUI_DEFINE_MEMBER_TYPE(int);
LS_IMGUI_DEFINE_MEMBER_TYPE(unsigned);
LS_IMGUI_DEFINE_MEMBER_TYPE(float);
LS_IMGUI_DEFINE_MEMBER_TYPE(ImVec2);
LS_IMGUI_DEFINE_MEMBER_TYPE(ImVec4);

#undef LS_IMGUI_DEFINE_MEMBER_TYPE

template <typename T>
static const LS_ImGuiMember& LS_GetIndexMemberType(lua_State* state, const char* nameoftype, const T& members)
{
	size_t length;
	const char* key = luaL_checklstring(state, 2, &length);
	assert(key);
	assert(length > 0);

	const frozen::string keystr(key, length);
	const auto valueit = members.find(keystr);

	if (members.end() == valueit)
		luaL_error(state, "unknown member '%s' of %s", key, nameoftype);

	return valueit->second;
}

static int LS_ImGuiTypeOperatorIndex(lua_State* state, const LS_UserDataType& type, const LS_ImGuiMember& member)
{
	void* userdataptr = LS_GetValueFromTypedUserData(state, 1, &type);
	assert(userdataptr);

	const uint8_t* bytes = *reinterpret_cast<const uint8_t**>(userdataptr);
	assert(bytes);

	const uint8_t* memberptr = bytes + member.offset;

	switch (member.type)
	{
	case ImMemberType_bool:
		lua_pushboolean(state, *reinterpret_cast<const bool*>(memberptr));
		break;

	case ImMemberType_int:
	case ImMemberType_unsigned:
		lua_pushinteger(state, *reinterpret_cast<const int*>(memberptr));
		break;

	case ImMemberType_float:
		lua_pushnumber(state, *reinterpret_cast<const float*>(memberptr));
		break;

	case ImMemberType_ImVec2:
		LS_PushImVec(state, *reinterpret_cast<const ImVec2*>(memberptr));
		break;

	case ImMemberType_ImVec4:
		LS_PushImVec(state, *reinterpret_cast<const ImVec4*>(memberptr));
		break;

	default:
		assert(false);
		lua_pushnil(state);
		break;
	}

	return 1;
}

#define LS_IMGUI_MEMBER(TYPENAME, MEMBERNAME) \
	{ #MEMBERNAME, { LS_ImGuiTypeHolder<decltype(TYPENAME::MEMBERNAME)>::IMGUI_MEMBER_TYPE, offsetof(TYPENAME, MEMBERNAME) } }


static const LS_UserDataType ls_imguiviewport_type =
{
	{ {{'i', 'm', 'v', 'p'}} },
	sizeof(int) /* fourcc */ + sizeof(void*)
};

constexpr frozen::unordered_map<frozen::string, LS_ImGuiMember, 6> ls_imguiviewport_members =
{
#define LS_IMGUI_VIEWPORT_MEMBER(NAME) LS_IMGUI_MEMBER(ImGuiViewport, NAME)

	LS_IMGUI_VIEWPORT_MEMBER(ID),
	LS_IMGUI_VIEWPORT_MEMBER(Flags),
	LS_IMGUI_VIEWPORT_MEMBER(Pos),
	LS_IMGUI_VIEWPORT_MEMBER(Size),
	LS_IMGUI_VIEWPORT_MEMBER(WorkPos),
	LS_IMGUI_VIEWPORT_MEMBER(WorkSize),

#undef LS_IMGUI_VIEWPORT_MEMBER
};

static const LS_UserDataType ls_imguistyle_type =
{
	{ {{'i', 'm', 's', 't'}} },
	sizeof(int) /* fourcc */ + sizeof(void*)
};

constexpr frozen::unordered_map<frozen::string, LS_ImGuiMember, 50> ls_imguistyle_members =
{
#define LS_IMGUI_STYLE_MEMBER(NAME) LS_IMGUI_MEMBER(ImGuiStyle, NAME)

	LS_IMGUI_STYLE_MEMBER(Alpha),
	LS_IMGUI_STYLE_MEMBER(DisabledAlpha),
	LS_IMGUI_STYLE_MEMBER(WindowPadding),
	LS_IMGUI_STYLE_MEMBER(WindowRounding),
	LS_IMGUI_STYLE_MEMBER(WindowBorderSize),
	LS_IMGUI_STYLE_MEMBER(WindowMinSize),
	LS_IMGUI_STYLE_MEMBER(WindowTitleAlign),
	LS_IMGUI_STYLE_MEMBER(WindowMenuButtonPosition),
	LS_IMGUI_STYLE_MEMBER(ChildRounding),
	LS_IMGUI_STYLE_MEMBER(ChildBorderSize),
	LS_IMGUI_STYLE_MEMBER(PopupRounding),
	LS_IMGUI_STYLE_MEMBER(PopupBorderSize),
	LS_IMGUI_STYLE_MEMBER(FramePadding),
	LS_IMGUI_STYLE_MEMBER(FrameRounding),
	LS_IMGUI_STYLE_MEMBER(FrameBorderSize),
	LS_IMGUI_STYLE_MEMBER(ItemSpacing),
	LS_IMGUI_STYLE_MEMBER(ItemInnerSpacing),
	LS_IMGUI_STYLE_MEMBER(CellPadding),
	LS_IMGUI_STYLE_MEMBER(TouchExtraPadding),
	LS_IMGUI_STYLE_MEMBER(IndentSpacing),
	LS_IMGUI_STYLE_MEMBER(ColumnsMinSpacing),
	LS_IMGUI_STYLE_MEMBER(ScrollbarSize),
	LS_IMGUI_STYLE_MEMBER(ScrollbarRounding),
	LS_IMGUI_STYLE_MEMBER(GrabMinSize),
	LS_IMGUI_STYLE_MEMBER(GrabRounding),
	LS_IMGUI_STYLE_MEMBER(LogSliderDeadzone),
	LS_IMGUI_STYLE_MEMBER(TabRounding),
	LS_IMGUI_STYLE_MEMBER(TabBorderSize),
	LS_IMGUI_STYLE_MEMBER(TabMinWidthForCloseButton),
	LS_IMGUI_STYLE_MEMBER(TabBarBorderSize),
	LS_IMGUI_STYLE_MEMBER(TableAngledHeadersAngle),
	LS_IMGUI_STYLE_MEMBER(ColorButtonPosition),
	LS_IMGUI_STYLE_MEMBER(ButtonTextAlign),
	LS_IMGUI_STYLE_MEMBER(SelectableTextAlign),
	LS_IMGUI_STYLE_MEMBER(SeparatorTextBorderSize),
	LS_IMGUI_STYLE_MEMBER(SeparatorTextAlign),
	LS_IMGUI_STYLE_MEMBER(SeparatorTextPadding),
	LS_IMGUI_STYLE_MEMBER(DisplayWindowPadding),
	LS_IMGUI_STYLE_MEMBER(DisplaySafeAreaPadding),
	LS_IMGUI_STYLE_MEMBER(MouseCursorScale),
	LS_IMGUI_STYLE_MEMBER(AntiAliasedLines),
	LS_IMGUI_STYLE_MEMBER(AntiAliasedLinesUseTex),
	LS_IMGUI_STYLE_MEMBER(AntiAliasedFill),
	LS_IMGUI_STYLE_MEMBER(CurveTessellationTol),
	LS_IMGUI_STYLE_MEMBER(CircleTessellationMaxError),
	// TODO: Add support for arrays
	// LS_IMGUI_STYLE_MEMBER(Colors),
	LS_IMGUI_STYLE_MEMBER(HoverStationaryDelay),
	LS_IMGUI_STYLE_MEMBER(HoverDelayShort),
	LS_IMGUI_STYLE_MEMBER(HoverDelayNormal),
	LS_IMGUI_STYLE_MEMBER(HoverFlagsForTooltipMouse),
	LS_IMGUI_STYLE_MEMBER(HoverFlagsForTooltipNav),

#undef LS_IMGUI_STYLE_MEMBER
};

#undef LS_IMGUI_MEMBER


static bool ls_framescope;

static void LS_EnsureFrameScope(lua_State* state)
{
	if (!ls_framescope)
		luaL_error(state, "calling ImGui function outside of frame scope");
}

void LS_MarkImGuiFrameStart()
{
	assert(!ls_framescope);
	ls_framescope = true;
}

void LS_MarkImGuiFrameEnd()
{
	assert(ls_framescope);
	ls_framescope = false;
}

static uint32_t ls_windowscope;

static void LS_EnsureWindowScope(lua_State* state)
{
	if (ls_windowscope == 0)
		luaL_error(state, "calling ImGui function outside of window scope");
}

static uint32_t ls_popupscope;

static void LS_EnsurePopupScope(lua_State* state)
{
	if (ls_popupscope == 0)
		luaL_error(state, "calling ImGui function outside of popup scope");
}

static uint32_t ls_tablescope;

static void LS_EnsureTableScope(lua_State* state)
{
	if (ls_tablescope == 0)
		luaL_error(state, "calling ImGui function outside of table scope");
}

using LS_ImGuiEndFunction = void(*)();

static ImVector<LS_ImGuiEndFunction> ls_endfuncstack;

static void LS_AddToImGuiStack(LS_ImGuiEndFunction endfunc)
{
	ls_endfuncstack.push_back(endfunc);
}

static void LS_RemoveFromImGuiStack(lua_State* state, LS_ImGuiEndFunction endfunc)
{
	assert(!ls_endfuncstack.empty());

	if (ls_endfuncstack.back() != endfunc)
		luaL_error(state, "unexpected end function");  // TODO: improve error message

	ls_endfuncstack.pop_back();
	endfunc();
}

void LS_ClearImGuiStack()
{
	while (!ls_endfuncstack.empty())
	{
		ls_endfuncstack.back()();
		ls_endfuncstack.pop_back();
	}
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

static void LS_EndWindowScope()
{
	assert(ls_windowscope > 0);
	--ls_windowscope;

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

static void LS_EndPopupScope()
{
	assert(ls_popupscope > 0);
	--ls_popupscope;

	ImGui::EndPopup();
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

static void LS_EndTableScope()
{
	assert(ls_tablescope > 0);
	--ls_tablescope;

	ImGui::EndTable();
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

#define LS_IMGUI_ENUM_BEGIN() \
	{ static const ImGuiEnumValue values[] = {

#define LS_IMGUI_ENUM_VALUE(ENUMNAME, VALUENAME) \
	{ #VALUENAME, ImGui##ENUMNAME##_##VALUENAME },

#define LS_IMGUI_ENUM_END(ENUMNAME) \
	}; LS_InitImGuiEnum(state, #ENUMNAME, values, Q_COUNTOF(values)); }

#define LS_IMGUI_COND_FLAG(NAME) LS_IMGUI_ENUM_VALUE(Cond, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_COND_FLAG(None)
	LS_IMGUI_COND_FLAG(Always)
	LS_IMGUI_COND_FLAG(Once)
	LS_IMGUI_COND_FLAG(FirstUseEver)
	LS_IMGUI_COND_FLAG(Appearing)
	LS_IMGUI_ENUM_END(Cond)
#undef LS_IMGUI_COND_FLAG

#define LS_IMGUI_HOVERED_FLAG(NAME) LS_IMGUI_ENUM_VALUE(HoveredFlags, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_HOVERED_FLAG(None)
	LS_IMGUI_HOVERED_FLAG(ChildWindows)
	LS_IMGUI_HOVERED_FLAG(RootWindow)
	LS_IMGUI_HOVERED_FLAG(AnyWindow)
	LS_IMGUI_HOVERED_FLAG(NoPopupHierarchy)
	LS_IMGUI_HOVERED_FLAG(AllowWhenBlockedByPopup)
	LS_IMGUI_HOVERED_FLAG(AllowWhenBlockedByActiveItem)
	LS_IMGUI_HOVERED_FLAG(AllowWhenOverlappedByItem)
	LS_IMGUI_HOVERED_FLAG(AllowWhenOverlappedByWindow)
	LS_IMGUI_HOVERED_FLAG(AllowWhenDisabled)
	LS_IMGUI_HOVERED_FLAG(NoNavOverride)
	LS_IMGUI_HOVERED_FLAG(AllowWhenOverlapped)
	LS_IMGUI_HOVERED_FLAG(RectOnly)
	LS_IMGUI_HOVERED_FLAG(RootAndChildWindows)
	LS_IMGUI_HOVERED_FLAG(ForTooltip)
	LS_IMGUI_HOVERED_FLAG(Stationary)
	LS_IMGUI_HOVERED_FLAG(DelayNone)
	LS_IMGUI_HOVERED_FLAG(DelayShort)
	LS_IMGUI_HOVERED_FLAG(DelayNormal)
	LS_IMGUI_HOVERED_FLAG(NoSharedDelay)
	LS_IMGUI_ENUM_END(HoveredFlags)
#undef LS_IMGUI_HOVERED_FLAG

#define LS_IMGUI_INPUT_TEXT_FLAG(NAME) LS_IMGUI_ENUM_VALUE(InputTextFlags, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_INPUT_TEXT_FLAG(None)
	LS_IMGUI_INPUT_TEXT_FLAG(CharsDecimal)
	LS_IMGUI_INPUT_TEXT_FLAG(CharsHexadecimal)
	LS_IMGUI_INPUT_TEXT_FLAG(CharsUppercase)
	LS_IMGUI_INPUT_TEXT_FLAG(CharsNoBlank)
	LS_IMGUI_INPUT_TEXT_FLAG(AutoSelectAll)
	LS_IMGUI_INPUT_TEXT_FLAG(EnterReturnsTrue)
	// TODO: Input text callback support
	//LS_IMGUI_INPUT_TEXT_FLAG(CallbackCompletion)
	//LS_IMGUI_INPUT_TEXT_FLAG(CallbackHistory)
	//LS_IMGUI_INPUT_TEXT_FLAG(CallbackAlways)
	//LS_IMGUI_INPUT_TEXT_FLAG(CallbackCharFilter)
	LS_IMGUI_INPUT_TEXT_FLAG(AllowTabInput)
	LS_IMGUI_INPUT_TEXT_FLAG(CtrlEnterForNewLine)
	LS_IMGUI_INPUT_TEXT_FLAG(NoHorizontalScroll)
	LS_IMGUI_INPUT_TEXT_FLAG(AlwaysOverwrite)
	LS_IMGUI_INPUT_TEXT_FLAG(ReadOnly)
	LS_IMGUI_INPUT_TEXT_FLAG(Password)
	LS_IMGUI_INPUT_TEXT_FLAG(NoUndoRedo)
	LS_IMGUI_INPUT_TEXT_FLAG(CharsScientific)
	LS_IMGUI_INPUT_TEXT_FLAG(CallbackResize)
	LS_IMGUI_INPUT_TEXT_FLAG(CallbackEdit)
	LS_IMGUI_INPUT_TEXT_FLAG(EscapeClearsAll)
	LS_IMGUI_ENUM_END(InputTextFlags)
#undef LS_IMGUI_INPUT_TEXT_FLAG

#define LS_IMGUI_POPUP_FLAGS(NAME) LS_IMGUI_ENUM_VALUE(PopupFlags, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_POPUP_FLAGS(None)
	LS_IMGUI_POPUP_FLAGS(MouseButtonLeft)
	LS_IMGUI_POPUP_FLAGS(MouseButtonRight)
	LS_IMGUI_POPUP_FLAGS(MouseButtonMiddle)
	LS_IMGUI_POPUP_FLAGS(NoReopen)
	LS_IMGUI_POPUP_FLAGS(NoOpenOverExistingPopup)
	LS_IMGUI_POPUP_FLAGS(NoOpenOverItems)
	LS_IMGUI_POPUP_FLAGS(AnyPopupId)
	LS_IMGUI_POPUP_FLAGS(AnyPopupLevel)
	LS_IMGUI_POPUP_FLAGS(AnyPopup)
	LS_IMGUI_ENUM_END(PopupFlags)
#undef LS_IMGUI_POPUP_FLAGS

#define LS_IMGUI_SELECTABLE_FLAG(NAME) LS_IMGUI_ENUM_VALUE(SelectableFlags, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_SELECTABLE_FLAG(None)
	LS_IMGUI_SELECTABLE_FLAG(DontClosePopups)
	LS_IMGUI_SELECTABLE_FLAG(SpanAllColumns)
	LS_IMGUI_SELECTABLE_FLAG(AllowDoubleClick)
	LS_IMGUI_SELECTABLE_FLAG(Disabled)
	LS_IMGUI_SELECTABLE_FLAG(AllowOverlap)
	LS_IMGUI_ENUM_END(SelectableFlags)
#undef LS_IMGUI_SELECTABLE_FLAG

#define LS_IMGUI_TABLE_FLAG(NAME) LS_IMGUI_ENUM_VALUE(TableFlags, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_TABLE_FLAG(None)
	LS_IMGUI_TABLE_FLAG(Resizable)
	LS_IMGUI_TABLE_FLAG(Reorderable)
	LS_IMGUI_TABLE_FLAG(Hideable)
	LS_IMGUI_TABLE_FLAG(Sortable)
	LS_IMGUI_TABLE_FLAG(NoSavedSettings)
	LS_IMGUI_TABLE_FLAG(ContextMenuInBody)
	LS_IMGUI_TABLE_FLAG(RowBg)
	LS_IMGUI_TABLE_FLAG(BordersInnerH)
	LS_IMGUI_TABLE_FLAG(BordersOuterH)
	LS_IMGUI_TABLE_FLAG(BordersInnerV)
	LS_IMGUI_TABLE_FLAG(BordersOuterV)
	LS_IMGUI_TABLE_FLAG(BordersH)
	LS_IMGUI_TABLE_FLAG(BordersV)
	LS_IMGUI_TABLE_FLAG(BordersInner)
	LS_IMGUI_TABLE_FLAG(BordersOuter)
	LS_IMGUI_TABLE_FLAG(Borders)
	LS_IMGUI_TABLE_FLAG(NoBordersInBody)
	LS_IMGUI_TABLE_FLAG(NoBordersInBodyUntilResize)
	LS_IMGUI_TABLE_FLAG(SizingFixedFit)
	LS_IMGUI_TABLE_FLAG(SizingFixedSame)
	LS_IMGUI_TABLE_FLAG(SizingStretchProp)
	LS_IMGUI_TABLE_FLAG(SizingStretchSame)
	LS_IMGUI_TABLE_FLAG(NoHostExtendX)
	LS_IMGUI_TABLE_FLAG(NoHostExtendY)
	LS_IMGUI_TABLE_FLAG(NoKeepColumnsVisible)
	LS_IMGUI_TABLE_FLAG(PreciseWidths)
	LS_IMGUI_TABLE_FLAG(NoClip)
	LS_IMGUI_TABLE_FLAG(PadOuterX)
	LS_IMGUI_TABLE_FLAG(NoPadOuterX)
	LS_IMGUI_TABLE_FLAG(NoPadInnerX)
	LS_IMGUI_TABLE_FLAG(ScrollX)
	LS_IMGUI_TABLE_FLAG(ScrollY)
	LS_IMGUI_TABLE_FLAG(SortMulti)
	LS_IMGUI_TABLE_FLAG(SortTristate)
	LS_IMGUI_TABLE_FLAG(HighlightHoveredColumn)
	LS_IMGUI_ENUM_END(TableFlags)
#undef LS_IMGUI_TABLE_FLAG

#define LS_IMGUI_TABLE_COLUMN_FLAG(NAME) LS_IMGUI_ENUM_VALUE(TableColumnFlags, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_TABLE_COLUMN_FLAG(None)
	LS_IMGUI_TABLE_COLUMN_FLAG(Disabled)
	LS_IMGUI_TABLE_COLUMN_FLAG(DefaultHide)
	LS_IMGUI_TABLE_COLUMN_FLAG(DefaultSort)
	LS_IMGUI_TABLE_COLUMN_FLAG(WidthStretch)
	LS_IMGUI_TABLE_COLUMN_FLAG(WidthFixed)
	LS_IMGUI_TABLE_COLUMN_FLAG(NoResize)
	LS_IMGUI_TABLE_COLUMN_FLAG(NoReorder)
	LS_IMGUI_TABLE_COLUMN_FLAG(NoHide)
	LS_IMGUI_TABLE_COLUMN_FLAG(NoClip)
	LS_IMGUI_TABLE_COLUMN_FLAG(NoSort)
	LS_IMGUI_TABLE_COLUMN_FLAG(NoSortAscending)
	LS_IMGUI_TABLE_COLUMN_FLAG(NoSortDescending)
	LS_IMGUI_TABLE_COLUMN_FLAG(NoHeaderLabel)
	LS_IMGUI_TABLE_COLUMN_FLAG(NoHeaderWidth)
	LS_IMGUI_TABLE_COLUMN_FLAG(PreferSortAscending)
	LS_IMGUI_TABLE_COLUMN_FLAG(PreferSortDescending)
	LS_IMGUI_TABLE_COLUMN_FLAG(IndentEnable)
	LS_IMGUI_TABLE_COLUMN_FLAG(IndentDisable)
	LS_IMGUI_TABLE_COLUMN_FLAG(AngledHeader)
	LS_IMGUI_TABLE_COLUMN_FLAG(IsEnabled)
	LS_IMGUI_TABLE_COLUMN_FLAG(IsVisible)
	LS_IMGUI_TABLE_COLUMN_FLAG(IsSorted)
	LS_IMGUI_TABLE_COLUMN_FLAG(IsHovered)
	LS_IMGUI_ENUM_END(TableColumnFlags)
#undef LS_IMGUI_TABLE_COLUMN_FLAG

#define LS_IMGUI_WINDOW_FLAG(NAME) LS_IMGUI_ENUM_VALUE(WindowFlags, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_WINDOW_FLAG(None)
	LS_IMGUI_WINDOW_FLAG(NoTitleBar)
	LS_IMGUI_WINDOW_FLAG(NoResize)
	LS_IMGUI_WINDOW_FLAG(NoMove)
	LS_IMGUI_WINDOW_FLAG(NoScrollbar)
	LS_IMGUI_WINDOW_FLAG(NoScrollWithMouse)
	LS_IMGUI_WINDOW_FLAG(NoCollapse)
	LS_IMGUI_WINDOW_FLAG(AlwaysAutoResize)
	LS_IMGUI_WINDOW_FLAG(NoBackground)
	LS_IMGUI_WINDOW_FLAG(NoSavedSettings)
	LS_IMGUI_WINDOW_FLAG(NoMouseInputs)
	LS_IMGUI_WINDOW_FLAG(MenuBar)
	LS_IMGUI_WINDOW_FLAG(HorizontalScrollbar)
	LS_IMGUI_WINDOW_FLAG(NoFocusOnAppearing)
	LS_IMGUI_WINDOW_FLAG(NoBringToFrontOnFocus)
	LS_IMGUI_WINDOW_FLAG(AlwaysVerticalScrollbar)
	LS_IMGUI_WINDOW_FLAG(AlwaysHorizontalScrollbar)
	LS_IMGUI_WINDOW_FLAG(NoNavInputs)
	LS_IMGUI_WINDOW_FLAG(NoNavFocus)
	LS_IMGUI_WINDOW_FLAG(UnsavedDocument)
	LS_IMGUI_WINDOW_FLAG(NoNav)
	LS_IMGUI_WINDOW_FLAG(NoDecoration)
	LS_IMGUI_WINDOW_FLAG(NoInputs)
	LS_IMGUI_ENUM_END(WindowFlags)
#undef LS_IMGUI_WINDOW_FLAG

#undef LS_IMGUI_ENUM_END
#undef LS_IMGUI_ENUM_VALUE
#undef LS_IMGUI_ENUM_BEGIN
}

void LS_InitImGuiBindings(lua_State* state)
{
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

		{ "Begin", LS_global_imgui_Begin },
		{ "End", LS_global_imgui_End },
		{ "GetWindowContentRegionMax", LS_global_imgui_GetWindowContentRegionMax },
		{ "SameLine", LS_global_imgui_SameLine },
		{ "SetNextWindowFocus", LS_global_imgui_SetNextWindowFocus },
		{ "SetNextWindowPos", LS_global_imgui_SetNextWindowPos },
		{ "SetNextWindowSize", LS_global_imgui_SetNextWindowSize },

		{ "BeginPopup", LS_global_imgui_BeginPopup },
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
	lua_pushvalue(state, 1);  // copy of imgui table for addition of enums
	lua_setglobal(state, "imgui");
	LS_InitImGuiEnums(state);
	lua_pop(state, 1);  // remove imgui table
}

#endif // USE_LUA_SCRIPTING && USE_IMGUI
