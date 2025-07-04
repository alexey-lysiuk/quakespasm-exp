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

#if defined USE_LUA_SCRIPTING && defined USE_IMGUI

#include <algorithm>

#include "imgui.h"

#include "ls_imgui.h"
#include "ls_vector.h"

#ifdef USE_HELLO_IMGUI
#include "hello_imgui/imgui_theme.h"
#endif // USE_HELLO_IMGUI

#include "ImGuiColorTextEdit/TextEditor.h"

extern "C"
{
#include "quakedef.h"
}


static LS_Vector2 FromImVec2(const ImVec2& value)
{
	return LS_Vector2(reinterpret_cast<const float*>(&value.x));
}

static ImVec2 ToImVec2(const LS_Vector2& value)
{
	return ImVec2(value[0], value[1]);
}

static ImVec4 ToImVec4(const LS_Vector4& value)
{
	return ImVec4(value[0], value[1], value[2], value[3]);
}


static const char* LS_CheckImGuiName(lua_State* state)
{
	const char* const name = luaL_checkstring(state, 1);
	assert(name);

	if (!name || name[0] == '\0')
		luaL_error(state, "ImGui name cannot be empty");

	return name;
}


struct LS_TextBuffer
{
	char* data;
	size_t size;
};

constexpr LS_UserDataType<LS_TextBuffer> ls_imguitextbuffer_type("ImGui.TextBuffer");

static int LS_value_TextBuffer_gc(lua_State* state)
{
	LS_TextBuffer& textbuffer = ls_imguitextbuffer_type.GetValue(state, 1);
	IM_FREE(textbuffer.data);
	return 0;
}

static int LS_value_TextBuffer_len(lua_State* state)
{
	LS_TextBuffer& textbuffer = ls_imguitextbuffer_type.GetValue(state, 1);
	lua_pushinteger(state, strlen(textbuffer.data));
	return 1;
}

static int LS_value_TextBuffer_tostring(lua_State* state)
{
	LS_TextBuffer& textbuffer = ls_imguitextbuffer_type.GetValue(state, 1);
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
	}

	if (textlength > 0)
		memcpy(bufferdata, text, textlength);

	bufferdata[textlength] = '\0';

	static const luaL_Reg functions[] =
	{
		{ "__gc", LS_value_TextBuffer_gc },
		{ "__len", LS_value_TextBuffer_len },
		{ "__tostring", LS_value_TextBuffer_tostring },
		{ nullptr, nullptr }
	};

	LS_TextBuffer& textbuffer = ls_imguitextbuffer_type.New(state, nullptr, functions);
	textbuffer.data = bufferdata;
	textbuffer.size = buffersize;

	return 1;
}


enum LS_ImGuiType
{
	ImMemberType_bool,
	ImMemberType_int,
	ImMemberType_unsigned,
	ImMemberType_ImGuiDir,
	ImMemberType_float,
	ImMemberType_ImVec2,
	ImMemberType_ImVec4,
};

struct LS_ImGuiMember
{
	const char* name;
	uint32_t type;
	uint32_t offset;

	bool operator<(const LS_ImGuiMember& other) const
	{
		return strcmp(name, other.name) < 0;
	}
};

template <typename T>
struct LS_ImGuiTypeHolder;

#define LS_IMGUI_DEFINE_MEMBER_TYPE(TYPE) \
	template <> struct LS_ImGuiTypeHolder<TYPE> { static constexpr LS_ImGuiType IMGUI_MEMBER_TYPE = ImMemberType_##TYPE; }

LS_IMGUI_DEFINE_MEMBER_TYPE(bool);
LS_IMGUI_DEFINE_MEMBER_TYPE(int);
LS_IMGUI_DEFINE_MEMBER_TYPE(unsigned);
LS_IMGUI_DEFINE_MEMBER_TYPE(ImGuiDir);
LS_IMGUI_DEFINE_MEMBER_TYPE(float);
LS_IMGUI_DEFINE_MEMBER_TYPE(ImVec2);
LS_IMGUI_DEFINE_MEMBER_TYPE(ImVec4);

#undef LS_IMGUI_DEFINE_MEMBER_TYPE

static int LS_ImGuiTypeOperatorIndex(lua_State* state, const LS_TypelessUserDataType& type, const LS_ImGuiMember* members, const size_t membercount)
{
	assert(members != nullptr);
	assert(membercount != 0);

	size_t length;
	const char* key = luaL_checklstring(state, 2, &length);
	assert(key);
	assert(length > 0);

	const LS_ImGuiMember* last = members + membercount;
	const LS_ImGuiMember probe{ key };
	const LS_ImGuiMember* member = std::lower_bound(members, last, probe);

	if (member == last || probe < *member)
		luaL_error(state, "unknown member '%s' of type '%s'", key, type.GetName());

	void* userdataptr = type.GetValuePtr(state, 1);
	assert(userdataptr);

	const uint8_t* bytes = *reinterpret_cast<const uint8_t**>(userdataptr);
	assert(bytes);

	const uint8_t* memberptr = bytes + member->offset;

	switch (member->type)
	{
	case ImMemberType_bool:
		lua_pushboolean(state, *reinterpret_cast<const bool*>(memberptr));
		break;

	case ImMemberType_int:
	case ImMemberType_unsigned:
	case ImMemberType_ImGuiDir:
		lua_pushinteger(state, *reinterpret_cast<const int*>(memberptr));
		break;

	case ImMemberType_float:
		lua_pushnumber(state, *reinterpret_cast<const float*>(memberptr));
		break;

	case ImMemberType_ImVec2:
		LS_PushVectorValue(state, *reinterpret_cast<const LS_Vector2*>(memberptr));
		break;

	case ImMemberType_ImVec4:
		LS_PushVectorValue(state, *reinterpret_cast<const LS_Vector4*>(memberptr));
		break;

	default:
		assert(false);
		lua_pushnil(state);
		break;
	}

	return 1;
}

#define LS_IMGUI_MEMBER(TYPENAME, MEMBERNAME) \
	{ #MEMBERNAME, LS_ImGuiTypeHolder<decltype(TYPENAME::MEMBERNAME)>::IMGUI_MEMBER_TYPE, offsetof(TYPENAME, MEMBERNAME) }


constexpr LS_UserDataType<ImGuiViewport*> ls_imguiviewport_type("ImGuiViewport");

constexpr LS_ImGuiMember ls_imguiviewport_members[] =
{
#define LS_IMGUI_VIEWPORT_MEMBER(NAME) LS_IMGUI_MEMBER(ImGuiViewport, NAME)

	LS_IMGUI_VIEWPORT_MEMBER(Flags),
	LS_IMGUI_VIEWPORT_MEMBER(ID),
	LS_IMGUI_VIEWPORT_MEMBER(Pos),
	LS_IMGUI_VIEWPORT_MEMBER(Size),
	LS_IMGUI_VIEWPORT_MEMBER(WorkPos),
	LS_IMGUI_VIEWPORT_MEMBER(WorkSize),

#undef LS_IMGUI_VIEWPORT_MEMBER
};

constexpr LS_UserDataType<ImGuiStyle*> ls_imguistyle_type("ImGuiStyle");

constexpr LS_ImGuiMember ls_imguistyle_members[] =
{
#define LS_IMGUI_STYLE_MEMBER(NAME) LS_IMGUI_MEMBER(ImGuiStyle, NAME)

	LS_IMGUI_STYLE_MEMBER(Alpha),
	LS_IMGUI_STYLE_MEMBER(AntiAliasedFill),
	LS_IMGUI_STYLE_MEMBER(AntiAliasedLines),
	LS_IMGUI_STYLE_MEMBER(AntiAliasedLinesUseTex),
	LS_IMGUI_STYLE_MEMBER(ButtonTextAlign),
	LS_IMGUI_STYLE_MEMBER(CellPadding),
	LS_IMGUI_STYLE_MEMBER(ChildBorderSize),
	LS_IMGUI_STYLE_MEMBER(ChildRounding),
	LS_IMGUI_STYLE_MEMBER(CircleTessellationMaxError),
	LS_IMGUI_STYLE_MEMBER(ColorButtonPosition),
	// TODO: Add support for arrays
	//LS_IMGUI_STYLE_MEMBER(Colors),
	LS_IMGUI_STYLE_MEMBER(ColumnsMinSpacing),
	LS_IMGUI_STYLE_MEMBER(CurveTessellationTol),
	LS_IMGUI_STYLE_MEMBER(DisabledAlpha),
	LS_IMGUI_STYLE_MEMBER(DisplaySafeAreaPadding),
	LS_IMGUI_STYLE_MEMBER(DisplayWindowPadding),
	LS_IMGUI_STYLE_MEMBER(FontScaleDpi),
	LS_IMGUI_STYLE_MEMBER(FontScaleMain),
	LS_IMGUI_STYLE_MEMBER(FontSizeBase),
	LS_IMGUI_STYLE_MEMBER(FrameBorderSize),
	LS_IMGUI_STYLE_MEMBER(FramePadding),
	LS_IMGUI_STYLE_MEMBER(FrameRounding),
	LS_IMGUI_STYLE_MEMBER(GrabMinSize),
	LS_IMGUI_STYLE_MEMBER(GrabRounding),
	LS_IMGUI_STYLE_MEMBER(HoverDelayNormal),
	LS_IMGUI_STYLE_MEMBER(HoverDelayShort),
	LS_IMGUI_STYLE_MEMBER(HoverFlagsForTooltipMouse),
	LS_IMGUI_STYLE_MEMBER(HoverFlagsForTooltipNav),
	LS_IMGUI_STYLE_MEMBER(HoverStationaryDelay),
	LS_IMGUI_STYLE_MEMBER(ImageBorderSize),
	LS_IMGUI_STYLE_MEMBER(IndentSpacing),
	LS_IMGUI_STYLE_MEMBER(ItemInnerSpacing),
	LS_IMGUI_STYLE_MEMBER(ItemSpacing),
	LS_IMGUI_STYLE_MEMBER(LogSliderDeadzone),
	LS_IMGUI_STYLE_MEMBER(MouseCursorScale),
	LS_IMGUI_STYLE_MEMBER(PopupBorderSize),
	LS_IMGUI_STYLE_MEMBER(PopupRounding),
	LS_IMGUI_STYLE_MEMBER(ScrollbarRounding),
	LS_IMGUI_STYLE_MEMBER(ScrollbarSize),
	LS_IMGUI_STYLE_MEMBER(SelectableTextAlign),
	LS_IMGUI_STYLE_MEMBER(SeparatorTextAlign),
	LS_IMGUI_STYLE_MEMBER(SeparatorTextBorderSize),
	LS_IMGUI_STYLE_MEMBER(SeparatorTextPadding),
	LS_IMGUI_STYLE_MEMBER(TabBarBorderSize),
	LS_IMGUI_STYLE_MEMBER(TabBorderSize),
	LS_IMGUI_STYLE_MEMBER(TableAngledHeadersAngle),
	LS_IMGUI_STYLE_MEMBER(TableAngledHeadersTextAlign),
	LS_IMGUI_STYLE_MEMBER(TabCloseButtonMinWidthSelected),
	LS_IMGUI_STYLE_MEMBER(TabCloseButtonMinWidthUnselected),
	LS_IMGUI_STYLE_MEMBER(TabRounding),
	LS_IMGUI_STYLE_MEMBER(TouchExtraPadding),
	LS_IMGUI_STYLE_MEMBER(TreeLinesFlags),
	LS_IMGUI_STYLE_MEMBER(TreeLinesRounding),
	LS_IMGUI_STYLE_MEMBER(TreeLinesSize),
	LS_IMGUI_STYLE_MEMBER(WindowBorderSize),
	LS_IMGUI_STYLE_MEMBER(WindowBorderHoverPadding),
	LS_IMGUI_STYLE_MEMBER(WindowMenuButtonPosition),
	LS_IMGUI_STYLE_MEMBER(WindowMinSize),
	LS_IMGUI_STYLE_MEMBER(WindowPadding),
	LS_IMGUI_STYLE_MEMBER(WindowRounding),
	LS_IMGUI_STYLE_MEMBER(WindowTitleAlign),

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

static uint32_t ls_childwindowscope;

static void LS_EnsureChildWindowScope(lua_State* state)
{
	if (ls_childwindowscope == 0)
		luaL_error(state, "calling ImGui function outside of child window scope");
}

static void LS_EndChildWindowScope()
{
	assert(ls_childwindowscope > 0);
	--ls_childwindowscope;

	ImGui::EndChild();
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

static bool ls_comboscope;

static void LS_EndComboScope()
{
	assert(ls_comboscope);
	ls_comboscope = false;

	ImGui::EndCombo();
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


constexpr LS_UserDataType<TextEditor*> ls_imguicolortextedit_type("ImGuiColorTextEdit");

static TextEditor* LS_GetColorTextEdit(lua_State* state)
{
	LS_EnsureFrameScope(state);

	TextEditor* texteditor = ls_imguicolortextedit_type.GetValue(state, 1);
	assert(texteditor);
	
	return texteditor;
}

static int LS_value_ImGuiColorTextEdit_gc(lua_State* state)
{
	TextEditor* const texteditor = ls_imguicolortextedit_type.GetValue(state, 1);
	IM_DELETE(texteditor);
	return 0;
}

static int LS_value_ImGuiColorTextEdit_index(lua_State* state)
{
	const int functableindex = lua_upvalueindex(1);
	luaL_checktype(state, functableindex, LUA_TTABLE);

	const char* const name = luaL_checkstring(state, 2);
	lua_getfield(state, functableindex, name);
	return 1;
}

static int LS_value_ImGuiColorTextEdit_GetCurrentCursor(lua_State* state)
{
	TextEditor* texteditor = LS_GetColorTextEdit(state);

	int line, column;
	texteditor->GetCurrentCursor(line, column);

	// On Lua side, line and character indices begin with one
	lua_pushinteger(state, line + 1);
	lua_pushinteger(state, column + 1);
	return 2;
}

static int LS_value_ImGuiColorTextEdit_Render(lua_State* state)
{
	TextEditor* texteditor = LS_GetColorTextEdit(state);
	assert(texteditor);

	const char* const title = luaL_checkstring(state, 2);
	const LS_Vector2 size = luaL_opt(state, LS_GetVectorValue<2>, 3, LS_Vector2::Zero());
	const bool border = luaL_opt(state, lua_toboolean, 4, false);

	texteditor->Render(title, ToImVec2(size), border);
	return 0;
}

static int LS_value_ImGuiColorTextEdit_ScrollToLine(lua_State* state)
{
	TextEditor* texteditor = LS_GetColorTextEdit(state);
	assert(texteditor);

	const int line = luaL_checkinteger(state, 2);

	static const char* const names[] = { "top", "middle", "bottom" };
	const auto alignment = TextEditor::Scroll(luaL_checkoption(state, 3, nullptr, names));

	texteditor->ScrollToLine(line - 1, alignment);  // first line index is zero
	return 0;
}

static int LS_value_ImGuiColorTextEdit_SelectLine(lua_State* state)
{
	TextEditor* texteditor = LS_GetColorTextEdit(state);

	const int line = luaL_checkinteger(state, 2);
	texteditor->SelectLine(line - 1);  // first line index is zero
	return 0;
}

static int LS_value_ImGuiColorTextEdit_SelectLines(lua_State* state)
{
	TextEditor* texteditor = LS_GetColorTextEdit(state);

	const int startline = luaL_checkinteger(state, 2);
	const int endline = luaL_checkinteger(state, 3);

	// On C++ side, line and character indices begin with zero
	texteditor->SelectLines(startline - 1, endline - 1);
	return 0;
}

static int LS_value_ImGuiColorTextEdit_SelectRegion(lua_State* state)
{
	TextEditor* texteditor = LS_GetColorTextEdit(state);

	const int startline = luaL_checkinteger(state, 2);
	const int startchar = luaL_checkinteger(state, 3);
	const int endline = luaL_checkinteger(state, 4);
	const int endchar = luaL_checkinteger(state, 5);

	// On C++ side, line and character indices begin with zero
	texteditor->SelectRegion(startline - 1, startchar - 1, endline - 1, endchar - 1);
	return 0;
}

static int LS_value_ImGuiColorTextEdit_SetCursor(lua_State* state)
{
	TextEditor* texteditor = LS_GetColorTextEdit(state);

	const int line = luaL_checkinteger(state, 2);
	const int character = luaL_optinteger(state, 3, 1);

	// On C++ side, line and character indices begin with zero
	texteditor->SetCursor(line - 1, character - 1);
	return 0;
}

static const TextEditor::Language* QuakeEntitiesLanguage()
{
	static bool initialized = false;
	static TextEditor::Language language;

	if (!initialized)
	{
		const TextEditor::Language* clanguage = TextEditor::Language::C();

		language.name = "Quake Entities";
		language.singleLineComment = "//";
		language.commentStart = "/*";
		language.commentEnd = "*/";
		language.hasDoubleQuotedStrings = true;
		language.stringEscape = '\\';
		language.isPunctuation = clanguage->isPunctuation;
		language.getIdentifier = clanguage->getIdentifier;
		language.getNumber = clanguage->getNumber;

		initialized = true;
	}

	return &language;
}

static int LS_value_ImGuiColorTextEdit_SetLanguage(lua_State* state)
{
	TextEditor* texteditor = LS_GetColorTextEdit(state);

	static const char* const languages[] = { "none", "cpp", "lua", "entities" };
	const int languageId = luaL_checkoption(state, 2, nullptr, languages);
	const TextEditor::Language* language;

	switch (languageId)
	{
	case 0:
		language = nullptr;
		break;

	case 1:
		language = TextEditor::Language::Cpp();
		break;

	case 2:
		language = TextEditor::Language::Lua();
		break;

	case 3:
		language = QuakeEntitiesLanguage();
		break;

	default:
		assert(false);
		return 0;
	}

	texteditor->SetLanguage(language);
	return 0;
}

static int LS_value_ImGuiColorTextEdit_SetReadOnly(lua_State* state)
{
	TextEditor* texteditor = LS_GetColorTextEdit(state);

	const bool readonly = lua_toboolean(state, 2);
	texteditor->SetReadOnlyEnabled(readonly);
	return 0;
}

static int LS_value_ImGuiColorTextEdit_SetText(lua_State* state)
{
	TextEditor* texteditor = LS_GetColorTextEdit(state);

	size_t length;
	const char* const text = luaL_checklstring(state, 2, &length);

	texteditor->SetText({ text, length });
	return 0;
}

static int LS_global_imgui_ColorTextEdit(lua_State* state)
{
	LS_EnsureFrameScope(state);

	TextEditor*& texteditor = ls_imguicolortextedit_type.New(state);
	texteditor = IM_NEW(TextEditor);
	assert(texteditor);
	texteditor->SetShowWhitespacesEnabled(false);

	if (luaL_newmetatable(state, "ImGui.ColorTextEdit"))
	{
		lua_pushcfunction(state, LS_value_ImGuiColorTextEdit_gc);
		lua_setfield(state, -2, "__gc");

		constexpr luaL_Reg methods[] =
		{
			{ "GetCurrentCursor", LS_value_ImGuiColorTextEdit_GetCurrentCursor },
			{ "Render", LS_value_ImGuiColorTextEdit_Render },
			{ "ScrollToLine", LS_value_ImGuiColorTextEdit_ScrollToLine },
			{ "SelectLine", LS_value_ImGuiColorTextEdit_SelectLine },
			{ "SelectLines", LS_value_ImGuiColorTextEdit_SelectLines },
			{ "SelectRegion", LS_value_ImGuiColorTextEdit_SelectRegion },
			{ "SetCursor", LS_value_ImGuiColorTextEdit_SetCursor },
			{ "SetLanguage", LS_value_ImGuiColorTextEdit_SetLanguage },
			{ "SetReadOnly", LS_value_ImGuiColorTextEdit_SetReadOnly },
			{ "SetText", LS_value_ImGuiColorTextEdit_SetText },
			{ nullptr, nullptr }
		};

		lua_newtable(state);
		luaL_setfuncs(state, methods, 0);

		// Set table with methods as upvalue for __index metamethod
		lua_pushcclosure(state, LS_value_ImGuiColorTextEdit_index, 1);
		lua_setfield(state, -2, "__index");
	}

	lua_setmetatable(state, -2);
	return 1;
}


#include "ls_imgui_enums.h"
#include "ls_imgui_funcs.h"

#ifdef USE_HELLO_IMGUI

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

#endif // USE_HELLO_IMGUI

void LS_InitImGuiBindings(lua_State* state)
{
	LS_InitImGuiFuncs(state);
	LS_InitImGuiEnums(state);

	lua_setglobal(state, "ImGui");
	assert(lua_gettop(state) == 0);

#ifdef USE_HELLO_IMGUI
	LS_InitImGuiTheme(state);
#endif // USE_HELLO_IMGUI
}

#endif // USE_LUA_SCRIPTING && USE_IMGUI
