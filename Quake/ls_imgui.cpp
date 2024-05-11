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

#ifndef NDEBUG
#include "hello_imgui/imgui_theme.h"
#endif // !NDEBUG

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


struct LS_TextBuffer
{
	char* data;
	size_t size;
};

static const LS_UserDataType ls_imguitextbuffer_type =
{
	{ {{'i', 'm', 't', 'b'}} },
	sizeof(int) + /* fourcc */ + sizeof(LS_TextBuffer)
};

static LS_TextBuffer& LS_GetTextBufferValue(lua_State* state, int index)
{
	LS_TextBuffer* textbufferptr = static_cast<LS_TextBuffer*>(LS_GetValueFromTypedUserData(state, index, &ls_imguitextbuffer_type));
	assert(textbufferptr);

	return *textbufferptr;
}

static int LS_value_TextBuffer_gc(lua_State* state)
{
	LS_TextBuffer& textbuffer = LS_GetTextBufferValue(state, 1);
	IM_FREE(textbuffer.data);
	return 0;
}

static int LS_value_TextBuffer_len(lua_State* state)
{
	LS_TextBuffer& textbuffer = LS_GetTextBufferValue(state, 1);
	lua_pushinteger(state, strlen(textbuffer.data));
	return 1;
}

static int LS_value_TextBuffer_tostring(lua_State* state)
{
	LS_TextBuffer& textbuffer = LS_GetTextBufferValue(state, 1);
	lua_pushstring(state, textbuffer.data);
	return 1;
}

static int LS_global_imgui_TextBuffer(lua_State* state)
{
	static constexpr lua_Integer BUFFER_SIZE_MIN = 256;
	static constexpr lua_Integer BUFFER_SIZE_MAX = 1024 * 1024;

	const lua_Integer ibuffersize = luaL_optinteger(state, 1, 256);
	const size_t buffersize = CLAMP(BUFFER_SIZE_MIN, ibuffersize, BUFFER_SIZE_MAX);
	char* bufferdata = reinterpret_cast<char*>(IM_ALLOC(buffersize));

	size_t textlength = 0;
	const char* text = luaL_optlstring(state, 2, "", &textlength);
	assert(text);

	if (buffersize <= textlength)
	{
		// Text doesn't fit the buffer, cut it
		textlength = buffersize - 1;
		bufferdata[textlength] = '\0';
	}

	if (textlength > 0)
		strncpy(bufferdata, text, textlength);
	else
		bufferdata[0] = '\0';

	LS_TextBuffer* textbuffer = static_cast<LS_TextBuffer*>(LS_CreateTypedUserData(state, &ls_imguitextbuffer_type));
	assert(textbuffer);

	textbuffer->data = bufferdata;
	textbuffer->size = buffersize;

	// Create and set 'ImGui.TextBuffer' metatable
	static const luaL_Reg functions[] =
	{
		{ "__gc", LS_value_TextBuffer_gc },
		{ "__len", LS_value_TextBuffer_len },
		{ "__tostring", LS_value_TextBuffer_tostring },
		{ NULL, NULL }
	};

	if (luaL_newmetatable(state, "ImGui.TextBuffer"))
		luaL_setfuncs(state, functions, 0);

	lua_setmetatable(state, -2);
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

constexpr frozen::unordered_map<frozen::string, LS_ImGuiMember, 51> ls_imguistyle_members =
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
	LS_IMGUI_STYLE_MEMBER(TableAngledHeadersTextAlign),
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

static void LS_EndWindowScope()
{
	assert(ls_windowscope > 0);
	--ls_windowscope;

	ImGui::End();
}

static uint32_t ls_popupscope;

static void LS_EnsurePopupScope(lua_State* state)
{
	if (ls_popupscope == 0)
		luaL_error(state, "calling ImGui function outside of popup scope");
}

static void LS_EndPopupScope()
{
	assert(ls_popupscope > 0);
	--ls_popupscope;

	ImGui::EndPopup();
}

static uint32_t ls_tablescope;

static void LS_EnsureTableScope(lua_State* state)
{
	if (ls_tablescope == 0)
		luaL_error(state, "calling ImGui function outside of table scope");
}

static void LS_EndTableScope()
{
	assert(ls_tablescope > 0);
	--ls_tablescope;

	ImGui::EndTable();
}

static bool ls_menubarscope;

static void LS_EndMenuBarScope()
{
	assert(ls_menubarscope);
	ls_menubarscope = false;

	ImGui::EndMenuBar();
}

static bool ls_mainmenubarscope;

static void LS_EndMainMenuBarScope()
{
	assert(ls_mainmenubarscope);
	ls_mainmenubarscope = false;

	ImGui::EndMainMenuBar();
}

static uint32_t ls_menuscope;

static void LS_EndMenuScope()
{
	assert(ls_menuscope > 0);
	--ls_menuscope;

	ImGui::EndMenu();
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

#include "ls_imgui_enums.h"
#include "ls_imgui_funcs.h"

#ifndef NDEBUG

static int LS_global_ImGuiTheme_ApplyTheme(lua_State* state)
{
	const int theme = luaL_checkinteger(state, 1);

	if (theme >= 0 && theme < ImGuiTheme::ImGuiTheme_Count)
		ImGuiTheme::ApplyTheme(ImGuiTheme::ImGuiTheme_(theme));

	return 0;
}

static void LS_InitImGuiTheme(lua_State* state)
{
	assert(lua_gettop(state) == 0);

	static const luaL_Reg functions[] =
	{
		{ "ApplyTheme", LS_global_ImGuiTheme_ApplyTheme },
		{ nullptr, nullptr }
	};

	luaL_newlib(state, functions);

	static const ImGuiEnumValue themes[] =
	{
#define LS_IMGUI_THEME(NAME) { #NAME, ImGuiTheme::ImGuiTheme_##NAME },

		LS_IMGUI_THEME(ImGuiColorsClassic)
		LS_IMGUI_THEME(ImGuiColorsDark)
		LS_IMGUI_THEME(ImGuiColorsLight)
		LS_IMGUI_THEME(MaterialFlat)
		LS_IMGUI_THEME(PhotoshopStyle)
		LS_IMGUI_THEME(GrayVariations)
		LS_IMGUI_THEME(GrayVariations_Darker)
		LS_IMGUI_THEME(MicrosoftStyle)
		LS_IMGUI_THEME(Cherry)
		LS_IMGUI_THEME(Darcula)
		LS_IMGUI_THEME(DarculaDarker)
		LS_IMGUI_THEME(LightRounded)
		LS_IMGUI_THEME(SoDark_AccentBlue)
		LS_IMGUI_THEME(SoDark_AccentYellow)
		LS_IMGUI_THEME(SoDark_AccentRed)
		LS_IMGUI_THEME(BlackIsBlack)
		LS_IMGUI_THEME(WhiteIsWhite)

#undef LS_IMGUI_THEME
	};

	LS_InitImGuiEnum(state, "Themes", themes, Q_COUNTOF(themes));

	lua_setglobal(state, "ImGuiTheme");
}

#endif // !NDEBUG

void LS_InitImGuiBindings(lua_State* state)
{
	LS_InitImGuiFuncs(state);
	LS_InitImGuiEnums(state);

	lua_setglobal(state, "ImGui");
	assert(lua_gettop(state) == 0);

#ifndef NDEBUG
	LS_InitImGuiTheme(state);
#endif // !NDEBUG
}

#endif // USE_LUA_SCRIPTING && USE_IMGUI
