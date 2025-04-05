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

struct ImGuiEnumValue
{
	const char* name;
	int value;
};

static void LS_InitImGuiEnum(lua_State* state, const char* name, const ImGuiEnumValue* values, size_t valuecount)
{
	assert(lua_gettop(state) == 1);  // imgui table must be on top of the stack

	lua_pushstring(state, name);
	lua_createtable(state, 0, valuecount);

	for (size_t i = 0; i < valuecount; ++i)
	{
		const ImGuiEnumValue& value = values[i];
		lua_pushstring(state, value.name);
		lua_pushinteger(state, value.value);
		lua_rawset(state, -3);
	}

	lua_rawset(state, -3);  // add enum table to imgui table
}

static void LS_InitImGuiEnums(lua_State* state)
{
	assert(lua_gettop(state) == 1);  // imgui table must be on top of the stack

#define LS_IMGUI_ENUM_BEGIN() \
	{ static const ImGuiEnumValue values[] = {

#define LS_IMGUI_ENUM_VALUE(ENUMNAME, VALUENAME) \
	{ #VALUENAME, ImGui##ENUMNAME##_##VALUENAME },

#define LS_IMGUI_ENUM_END(ENUMNAME) \
	}; LS_InitImGuiEnum(state, #ENUMNAME, values, Q_COUNTOF(values)); }

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

	// * ImGuiChildFlags
	// * ImGuiItemFlags

#define LS_IMGUI_INPUT_TEXT_FLAG(NAME) LS_IMGUI_ENUM_VALUE(InputTextFlags, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_INPUT_TEXT_FLAG(None)
	LS_IMGUI_INPUT_TEXT_FLAG(CharsDecimal)
	LS_IMGUI_INPUT_TEXT_FLAG(CharsHexadecimal)
	LS_IMGUI_INPUT_TEXT_FLAG(CharsScientific)
	LS_IMGUI_INPUT_TEXT_FLAG(CharsUppercase)
	LS_IMGUI_INPUT_TEXT_FLAG(CharsNoBlank)
	LS_IMGUI_INPUT_TEXT_FLAG(AllowTabInput)
	LS_IMGUI_INPUT_TEXT_FLAG(EnterReturnsTrue)
	LS_IMGUI_INPUT_TEXT_FLAG(EscapeClearsAll)
	LS_IMGUI_INPUT_TEXT_FLAG(CtrlEnterForNewLine)
	LS_IMGUI_INPUT_TEXT_FLAG(ReadOnly)
	LS_IMGUI_INPUT_TEXT_FLAG(Password)
	LS_IMGUI_INPUT_TEXT_FLAG(AlwaysOverwrite)
	LS_IMGUI_INPUT_TEXT_FLAG(AutoSelectAll)
	LS_IMGUI_INPUT_TEXT_FLAG(ParseEmptyRefVal)
	LS_IMGUI_INPUT_TEXT_FLAG(DisplayEmptyRefVal)
	LS_IMGUI_INPUT_TEXT_FLAG(NoHorizontalScroll)
	LS_IMGUI_INPUT_TEXT_FLAG(NoUndoRedo)
	LS_IMGUI_INPUT_TEXT_FLAG(ElideLeft)
	// TODO: Input text callback support
	//LS_IMGUI_INPUT_TEXT_FLAG(CallbackCompletion)
	//LS_IMGUI_INPUT_TEXT_FLAG(CallbackHistory)
	//LS_IMGUI_INPUT_TEXT_FLAG(CallbackAlways)
	//LS_IMGUI_INPUT_TEXT_FLAG(CallbackCharFilter)
	//LS_IMGUI_INPUT_TEXT_FLAG(CallbackResize)
	//LS_IMGUI_INPUT_TEXT_FLAG(CallbackEdit)
	LS_IMGUI_ENUM_END(InputTextFlags)
#undef LS_IMGUI_INPUT_TEXT_FLAG

	// * ImGuiTreeNodeFlags

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
	LS_IMGUI_SELECTABLE_FLAG(NoAutoClosePopups)
	LS_IMGUI_SELECTABLE_FLAG(SpanAllColumns)
	LS_IMGUI_SELECTABLE_FLAG(AllowDoubleClick)
	LS_IMGUI_SELECTABLE_FLAG(Disabled)
	LS_IMGUI_SELECTABLE_FLAG(AllowOverlap)
	LS_IMGUI_SELECTABLE_FLAG(Highlight)
	LS_IMGUI_ENUM_END(SelectableFlags)
#undef LS_IMGUI_SELECTABLE_FLAG

	// * ImGuiComboFlags
	// * ImGuiTabBarFlags
	// * ImGuiTabItemFlags

#define LS_IMGUI_FOCUSED_FLAG(NAME) LS_IMGUI_ENUM_VALUE(FocusedFlags, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_FOCUSED_FLAG(None)
	LS_IMGUI_FOCUSED_FLAG(ChildWindows)
	LS_IMGUI_FOCUSED_FLAG(RootWindow)
	LS_IMGUI_FOCUSED_FLAG(AnyWindow)
	LS_IMGUI_FOCUSED_FLAG(NoPopupHierarchy)
	LS_IMGUI_FOCUSED_FLAG(RootAndChildWindows)
	LS_IMGUI_ENUM_END(FocusedFlags)
#undef LS_IMGUI_FOCUSED_FLAG

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

	// * ImGuiDragDropFlags
	// * ImGuiDataType
	// * ImGuiDir
	// * ImGuiSortDirection

#define LS_IMGUI_KEY(NAME) LS_IMGUI_ENUM_VALUE(Key, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_KEY(None)
	LS_IMGUI_KEY(Tab)
	LS_IMGUI_KEY(LeftArrow)
	LS_IMGUI_KEY(RightArrow)
	LS_IMGUI_KEY(UpArrow)
	LS_IMGUI_KEY(DownArrow)
	LS_IMGUI_KEY(PageUp)
	LS_IMGUI_KEY(PageDown)
	LS_IMGUI_KEY(Home)
	LS_IMGUI_KEY(End)
	LS_IMGUI_KEY(Insert)
	LS_IMGUI_KEY(Delete)
	LS_IMGUI_KEY(Backspace)
	LS_IMGUI_KEY(Space)
	LS_IMGUI_KEY(Enter)
	LS_IMGUI_KEY(Escape)
	LS_IMGUI_KEY(LeftCtrl)
	LS_IMGUI_KEY(LeftShift)
	LS_IMGUI_KEY(LeftAlt)
	LS_IMGUI_KEY(LeftSuper)
	LS_IMGUI_KEY(RightCtrl)
	LS_IMGUI_KEY(RightShift)
	LS_IMGUI_KEY(RightAlt)
	LS_IMGUI_KEY(RightSuper)
	LS_IMGUI_KEY(Menu)
	LS_IMGUI_KEY(0)
	LS_IMGUI_KEY(1)
	LS_IMGUI_KEY(2)
	LS_IMGUI_KEY(3)
	LS_IMGUI_KEY(4)
	LS_IMGUI_KEY(5)
	LS_IMGUI_KEY(6)
	LS_IMGUI_KEY(7)
	LS_IMGUI_KEY(8)
	LS_IMGUI_KEY(9)
	LS_IMGUI_KEY(A)
	LS_IMGUI_KEY(B)
	LS_IMGUI_KEY(C)
	LS_IMGUI_KEY(D)
	LS_IMGUI_KEY(E)
	LS_IMGUI_KEY(F)
	LS_IMGUI_KEY(G)
	LS_IMGUI_KEY(H)
	LS_IMGUI_KEY(I)
	LS_IMGUI_KEY(J)
	LS_IMGUI_KEY(K)
	LS_IMGUI_KEY(L)
	LS_IMGUI_KEY(M)
	LS_IMGUI_KEY(N)
	LS_IMGUI_KEY(O)
	LS_IMGUI_KEY(P)
	LS_IMGUI_KEY(Q)
	LS_IMGUI_KEY(R)
	LS_IMGUI_KEY(S)
	LS_IMGUI_KEY(T)
	LS_IMGUI_KEY(U)
	LS_IMGUI_KEY(V)
	LS_IMGUI_KEY(W)
	LS_IMGUI_KEY(X)
	LS_IMGUI_KEY(Y)
	LS_IMGUI_KEY(Z)
	LS_IMGUI_KEY(F1)
	LS_IMGUI_KEY(F2)
	LS_IMGUI_KEY(F3)
	LS_IMGUI_KEY(F4)
	LS_IMGUI_KEY(F5)
	LS_IMGUI_KEY(F6)
	LS_IMGUI_KEY(F7)
	LS_IMGUI_KEY(F8)
	LS_IMGUI_KEY(F9)
	LS_IMGUI_KEY(F10)
	LS_IMGUI_KEY(F11)
	LS_IMGUI_KEY(F12)
	LS_IMGUI_KEY(F13)
	LS_IMGUI_KEY(F14)
	LS_IMGUI_KEY(F15)
	LS_IMGUI_KEY(F16)
	LS_IMGUI_KEY(F17)
	LS_IMGUI_KEY(F18)
	LS_IMGUI_KEY(F19)
	LS_IMGUI_KEY(F20)
	LS_IMGUI_KEY(F21)
	LS_IMGUI_KEY(F22)
	LS_IMGUI_KEY(F23)
	LS_IMGUI_KEY(F24)
	LS_IMGUI_KEY(Apostrophe)
	LS_IMGUI_KEY(Comma)
	LS_IMGUI_KEY(Minus)
	LS_IMGUI_KEY(Period)
	LS_IMGUI_KEY(Slash)
	LS_IMGUI_KEY(Semicolon)
	LS_IMGUI_KEY(Equal)
	LS_IMGUI_KEY(LeftBracket)
	LS_IMGUI_KEY(Backslash)
	LS_IMGUI_KEY(RightBracket)
	LS_IMGUI_KEY(GraveAccent)
	LS_IMGUI_KEY(CapsLock)
	LS_IMGUI_KEY(ScrollLock)
	LS_IMGUI_KEY(NumLock)
	LS_IMGUI_KEY(PrintScreen)
	LS_IMGUI_KEY(Pause)
	LS_IMGUI_KEY(Keypad0)
	LS_IMGUI_KEY(Keypad1)
	LS_IMGUI_KEY(Keypad2)
	LS_IMGUI_KEY(Keypad3)
	LS_IMGUI_KEY(Keypad4)
	LS_IMGUI_KEY(Keypad5)
	LS_IMGUI_KEY(Keypad6)
	LS_IMGUI_KEY(Keypad7)
	LS_IMGUI_KEY(Keypad8)
	LS_IMGUI_KEY(Keypad9)
	LS_IMGUI_KEY(KeypadDecimal)
	LS_IMGUI_KEY(KeypadDivide)
	LS_IMGUI_KEY(KeypadMultiply)
	LS_IMGUI_KEY(KeypadSubtract)
	LS_IMGUI_KEY(KeypadAdd)
	LS_IMGUI_KEY(KeypadEnter)
	LS_IMGUI_KEY(KeypadEqual)
	LS_IMGUI_KEY(AppBack)
	LS_IMGUI_KEY(AppForward)
	LS_IMGUI_KEY(Oem102)
	LS_IMGUI_KEY(GamepadStart)
	LS_IMGUI_KEY(GamepadBack)
	LS_IMGUI_KEY(GamepadFaceLeft)
	LS_IMGUI_KEY(GamepadFaceRight)
	LS_IMGUI_KEY(GamepadFaceUp)
	LS_IMGUI_KEY(GamepadFaceDown)
	LS_IMGUI_KEY(GamepadDpadLeft)
	LS_IMGUI_KEY(GamepadDpadRight)
	LS_IMGUI_KEY(GamepadDpadUp)
	LS_IMGUI_KEY(GamepadDpadDown)
	LS_IMGUI_KEY(GamepadL1)
	LS_IMGUI_KEY(GamepadR1)
	LS_IMGUI_KEY(GamepadL2)
	LS_IMGUI_KEY(GamepadR2)
	LS_IMGUI_KEY(GamepadL3)
	LS_IMGUI_KEY(GamepadR3)
	LS_IMGUI_KEY(GamepadLStickLeft)
	LS_IMGUI_KEY(GamepadLStickRight)
	LS_IMGUI_KEY(GamepadLStickUp)
	LS_IMGUI_KEY(GamepadLStickDown)
	LS_IMGUI_KEY(GamepadRStickLeft)
	LS_IMGUI_KEY(GamepadRStickRight)
	LS_IMGUI_KEY(GamepadRStickUp)
	LS_IMGUI_KEY(GamepadRStickDown)
	LS_IMGUI_KEY(MouseLeft)
	LS_IMGUI_KEY(MouseRight)
	LS_IMGUI_KEY(MouseMiddle)
	LS_IMGUI_KEY(MouseX1)
	LS_IMGUI_KEY(MouseX2)
	LS_IMGUI_KEY(MouseWheelX)
	LS_IMGUI_KEY(MouseWheelY)
	LS_IMGUI_ENUM_END(Key)
#undef LS_IMGUI_KEY

#define LS_IMGUI_MOD(NAME) LS_IMGUI_ENUM_VALUE(Mod, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_MOD(None)
	LS_IMGUI_MOD(Ctrl)
	LS_IMGUI_MOD(Shift)
	LS_IMGUI_MOD(Alt)
	LS_IMGUI_MOD(Super)
	LS_IMGUI_MOD(Mask_)
	LS_IMGUI_ENUM_END(Mod)
#undef LS_IMGUI_MOD

	// * ImGuiInputFlags
	// * ImGuiConfigFlags
	// * ImGuiBackendFlags
	// * ImGuiCol
	// * ImGuiStyleVar
	// * ImGuiButtonFlags
	// * ImGuiColorEditFlags

#define LS_IMGUI_SLIDER_FLAGS(NAME) LS_IMGUI_ENUM_VALUE(SliderFlags, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_SLIDER_FLAGS(None)
	LS_IMGUI_SLIDER_FLAGS(Logarithmic)
	LS_IMGUI_SLIDER_FLAGS(NoRoundToFormat)
	LS_IMGUI_SLIDER_FLAGS(NoInput)
	LS_IMGUI_SLIDER_FLAGS(WrapAround)
	LS_IMGUI_SLIDER_FLAGS(ClampOnInput)
	LS_IMGUI_SLIDER_FLAGS(ClampZeroRange)
	LS_IMGUI_SLIDER_FLAGS(NoSpeedTweaks)
	LS_IMGUI_SLIDER_FLAGS(AlwaysClamp)
	LS_IMGUI_ENUM_END(SliderFlags)
#undef LS_IMGUI_MOUSE_BUTTON

#define LS_IMGUI_MOUSE_BUTTON(NAME) LS_IMGUI_ENUM_VALUE(MouseButton, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_MOUSE_BUTTON(Left)
	LS_IMGUI_MOUSE_BUTTON(Right)
	LS_IMGUI_MOUSE_BUTTON(Middle)
	LS_IMGUI_MOUSE_BUTTON(COUNT)
	LS_IMGUI_ENUM_END(MouseButton)
#undef LS_IMGUI_MOUSE_BUTTON

	// * ImGuiMouseCursor
	// * ImGuiMouseSource

#define LS_IMGUI_COND_FLAG(NAME) LS_IMGUI_ENUM_VALUE(Cond, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_COND_FLAG(None)
	LS_IMGUI_COND_FLAG(Always)
	LS_IMGUI_COND_FLAG(Once)
	LS_IMGUI_COND_FLAG(FirstUseEver)
	LS_IMGUI_COND_FLAG(Appearing)
	LS_IMGUI_ENUM_END(Cond)
#undef LS_IMGUI_COND_FLAG

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

#define LS_IMGUI_TABLE_ROW_FLAG(NAME) LS_IMGUI_ENUM_VALUE(TableRowFlags, NAME)
	LS_IMGUI_ENUM_BEGIN()
	LS_IMGUI_TABLE_ROW_FLAG(None)
	LS_IMGUI_TABLE_ROW_FLAG(Headers)
	LS_IMGUI_ENUM_END(TableRowFlags)
#undef LS_IMGUI_TABLE_ROW_FLAG

	// * ImGuiTableBgTarget
	// * ImGuiMultiSelectFlags
	// * ImDrawFlags
	// * ImDrawListFlags
	// * ImFontAtlasFlags
	// * ImGuiViewportFlags

#undef LS_IMGUI_ENUM_END
#undef LS_IMGUI_ENUM_VALUE
#undef LS_IMGUI_ENUM_BEGIN
}
