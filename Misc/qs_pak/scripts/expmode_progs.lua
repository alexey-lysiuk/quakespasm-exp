
local tostring <const> = tostring

local insert <const> = table.insert

local imBegin <const> = ImGui.Begin
local imBeginMenu <const> = ImGui.BeginMenu
local imBeginTable <const> = ImGui.BeginTable
local imEnd <const> = ImGui.End
local imEndMenu <const> = ImGui.EndMenu
local imEndTable <const> = ImGui.EndTable
local imMenuItem <const> = ImGui.MenuItem
local imTableHeadersRow <const> = ImGui.TableHeadersRow
local imTableNextColumn <const> = ImGui.TableNextColumn
local imTableNextRow <const> = ImGui.TableNextRow
local imTableSetupColumn <const> = ImGui.TableSetupColumn
local imText <const> = ImGui.Text

local imTableFlags <const> = ImGui.TableFlags

local imTableColumnWidthFixed <const> = ImGui.TableColumnFlags.WidthFixed

local defaultTableFlags <const> = imTableFlags.Borders | imTableFlags.Resizable | imTableFlags.RowBg

local functions <const> = progs.functions

local function functions_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened and imBeginTable(title, 2, defaultTableFlags) then
		imTableSetupColumn('Index', imTableColumnWidthFixed)
		imTableSetupColumn('Name')
		imTableHeadersRow()

		for _, entry in ipairs(self.entries) do
			imTableNextRow()
			imTableNextColumn()
			imText(entry.index)
			imTableNextColumn()
			imText(entry.name)
		end

		imEndTable()
	end

	imEnd()

	return opened
end

local function functions_onshow(self)
	local entries = {}

	for i, func in functions() do
		local entry = { index = tostring(i), name = tostring(func) }
		insert(entries, entry)
	end

	self.entries = entries
	return true
end

local function functions_onhide(self)
	self.entries = nil
	return true
end

expmode.addaction(function ()
	if imBeginMenu('Progs') then
		if imMenuItem('Functions') then
			expmode.window('Progs Functions', functions_onupdate, nil,
				functions_onshow, functions_onhide):setconstraints()
		end

		imEndMenu()
	end
end)
