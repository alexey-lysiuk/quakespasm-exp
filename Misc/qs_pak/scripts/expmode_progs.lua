
local tostring <const> = tostring

local insert <const> = table.insert

local imBegin <const> = ImGui.Begin
local imBeginMenu <const> = ImGui.BeginMenu
local imBeginTable <const> = ImGui.BeginTable
local imEnd <const> = ImGui.End
local imEndMenu <const> = ImGui.EndMenu
local imEndTable <const> = ImGui.EndTable
local imInputTextMultiline <const> = ImGui.InputTextMultiline
local imMenuItem <const> = ImGui.MenuItem
local imSelectable <const> = ImGui.Selectable
local imTableHeadersRow <const> = ImGui.TableHeadersRow
local imTableNextColumn <const> = ImGui.TableNextColumn
local imTableNextRow <const> = ImGui.TableNextRow
local imTableSetupColumn <const> = ImGui.TableSetupColumn
local imText <const> = ImGui.Text
local imTextBuffer <const> = ImGui.TextBuffer
local imVec2 <const> = vec2.new

local imTableFlags <const> = ImGui.TableFlags

local imInputTextReadOnly <const> = ImGui.InputTextFlags.ReadOnly
local imTableColumnWidthFixed <const> = ImGui.TableColumnFlags.WidthFixed
local imWindowNoSavedSettings <const> = ImGui.WindowFlags.NoSavedSettings

local defaultTableFlags <const> = imTableFlags.Borders | imTableFlags.Resizable | imTableFlags.RowBg | imTableFlags.ScrollY

local functions <const> = progs.functions

local addaction <const> = expmode.addaction
local resetsearch <const> = expmode.resetsearch
local searchbar <const> = expmode.searchbar
local updatesearch <const> = expmode.updatesearch
local window <const> = expmode.window

local autoexpandsize <const> = imVec2(-1, -1)
local defaultDisassemblySize <const> = imVec2(640, 480)

local function functiondisassembly_onupdate(self)
	local visible, opened = imBegin(self.title, true)

	if visible and opened then
		imInputTextMultiline('##text', self.disassembly, autoexpandsize, imInputTextReadOnly)
	end

	imEnd()

	return opened
end

local function function_searchcompare(entry, string)
	return entry.name:lower():find(string, 1, true)
		or entry.file:lower():find(string, 1, true)
end

local function functions_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true, imWindowNoSavedSettings)

	if visible and opened then
		local searchmodified = searchbar(self)
		local entries = updatesearch(self, function_searchcompare, searchmodified)

		if imBeginTable(title, 3, defaultTableFlags) then
			imTableSetupColumn('Index', imTableColumnWidthFixed)
			imTableSetupColumn('Declaration')
			imTableSetupColumn('File')
			imTableHeadersRow()

			for _, entry in ipairs(entries) do
				imTableNextRow()
				imTableNextColumn()
				imText(entry.index)
				imTableNextColumn()
				if imSelectable(entry.declaration) then
					window(entry.name, functiondisassembly_onupdate,
						function (self) self.disassembly = imTextBuffer(16 * 1024, entry.func:disassemble()) end)
						:setconstraints():setsize(defaultDisassemblySize):movetocursor()
				end
				imTableNextColumn()
				imText(entry.file)
			end

			imEndTable()
		end
	end

	imEnd()

	return opened
end

local function functions_onshow(self)
	local entries = {}

	for i, func in functions() do
		local entry = { func = func, index = tostring(i), name = func:name(), declaration = tostring(func), file = func:file() }
		insert(entries, entry)
	end

	self.entries = entries

	updatesearch(self, function_searchcompare, true)
	return true
end

local function functions_onhide(self)
	resetsearch(self)
	self.entries = nil
	return true
end

addaction(function ()
	if imBeginMenu('Progs') then
		if imMenuItem('Functions') then
			window('Progs Functions', functions_onupdate, nil,
				functions_onshow, functions_onhide):setconstraints()
		end

		imEndMenu()
	end
end)
