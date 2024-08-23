
local tostring <const> = tostring

local format <const> = string.format

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
local imSeparator <const> = ImGui.Separator
local imTableHeadersRow <const> = ImGui.TableHeadersRow
local imTableNextColumn <const> = ImGui.TableNextColumn
local imTableNextRow <const> = ImGui.TableNextRow
local imTableSetupColumn <const> = ImGui.TableSetupColumn
local imTableSetupScrollFreeze <const> = ImGui.TableSetupScrollFreeze
local imText <const> = ImGui.Text
local imTextBuffer <const> = ImGui.TextBuffer
local imVec2 <const> = vec2.new

local imTableFlags <const> = ImGui.TableFlags

local imInputTextReadOnly <const> = ImGui.InputTextFlags.ReadOnly
local imTableColumnWidthFixed <const> = ImGui.TableColumnFlags.WidthFixed
local imWindowNoSavedSettings <const> = ImGui.WindowFlags.NoSavedSettings

local defaultTableFlags <const> = imTableFlags.Borders | imTableFlags.Resizable | imTableFlags.RowBg | imTableFlags.ScrollY

local fielddefinitions <const> = progs.fielddefinitions
local functions <const> = progs.functions
local globaldefinitions <const> = progs.globaldefinitions
local typename <const> = progs.typename
local strings <const> = progs.strings
local stringoffset <const> = strings.offset

local addaction <const> = expmode.addaction
local resetsearch <const> = expmode.resetsearch
local searchbar <const> = expmode.searchbar
local updatesearch <const> = expmode.updatesearch
local window <const> = expmode.window

local autoexpandsize <const> = imVec2(-1, -1)
local defaultDisassemblySize <const> = imVec2(640, 480)

local function functiondisassembly_onupdate(self)
	local visible, opened = imBegin(self.title, true, imWindowNoSavedSettings)

	if visible and opened then
		-- TODO: Do not show binary checkbox for built-in functions
		local binarypressed, binaryenabled = ImGui.Checkbox('Show statements binaries', self.withbinary)

		if binarypressed then
			self.disassembly = imTextBuffer(16 * 1024, self.func:disassemble(binaryenabled))
			self.withbinary = binaryenabled
		end

		imInputTextMultiline('##text', self.disassembly, autoexpandsize, imInputTextReadOnly)
	end

	imEnd()

	return opened
end

local function functiondisassembly_onshow(self)
	local func = self.func

	if self.name ~= func.name then
		return false
	end

	if not self.withbinary then
		self.withbinary = false
	end

	self.disassembly = imTextBuffer(16 * 1024, func:disassemble(self.withbinary))
	return true
end

local function functiondisassembly_onhide(self)
	self.disassembly = nil
	return true
end

local function function_searchcompare(entry, string)
	return entry.declaration:lower():find(string, 1, true)
		or entry.filename:lower():find(string, 1, true)
end

local function functions_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
		local searchmodified = searchbar(self)
		local entries = updatesearch(self, function_searchcompare, searchmodified)

		if imBeginTable(title, 3, defaultTableFlags) then
			imTableSetupScrollFreeze(0, 1)
			imTableSetupColumn('Index', imTableColumnWidthFixed)
			imTableSetupColumn('Declaration')
			imTableSetupColumn('File', imTableColumnWidthFixed)
			imTableHeadersRow()

			for _, entry in ipairs(entries) do
				imTableNextRow()
				imTableNextColumn()
				imText(entry.index)
				imTableNextColumn()
				if imSelectable(entry.declaration) then
					local func = entry.func
					local funcname = func.name

					local function oncreate(self)
						self:setconstraints()
						self:setsize(defaultDisassemblySize)
						self:movetocursor()

						self.func = func
						self.name = funcname
					end

					window(format('Disassembly of %s()', funcname), functiondisassembly_onupdate,
						oncreate, functiondisassembly_onshow, functiondisassembly_onhide)
				end
				imTableNextColumn()
				imText(entry.filename)
			end

			imEndTable()
		end
	end

	imEnd()

	return opened
end

local function functions_onshow(self)
	local entries = {}

	for i, func in ipairs(functions) do
		local entry = { func = func, index = tostring(i), declaration = tostring(func), filename = func.filename }
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

local function definitions_searchcompare(entry, string)
	return entry.name:lower():find(string, 1, true)
		or entry.type:find(string, 1, true)
		or entry.offset:find(string, 1, true)
end

local function definitions_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
		local searchmodified = searchbar(self)
		local entries = updatesearch(self, definitions_searchcompare, searchmodified)

		if imBeginTable(title, 4, defaultTableFlags) then
			imTableSetupScrollFreeze(0, 1)
			imTableSetupColumn('Index', imTableColumnWidthFixed)
			imTableSetupColumn('Name')
			imTableSetupColumn('Type', imTableColumnWidthFixed)
			imTableSetupColumn('Offset', imTableColumnWidthFixed)
			imTableHeadersRow()

			for _, entry in ipairs(entries) do
				imTableNextRow()
				imTableNextColumn()
				imText(entry.index)
				imTableNextColumn()
				imText(entry.name)
				imTableNextColumn()
				imText(entry.type)
				imTableNextColumn()
				imText(entry.offset)
			end

			imEndTable()
		end
	end

	imEnd()

	return opened
end

local function definitions_onshow(self)
	local entries = {}

	for i, definition in ipairs(self.definitions) do
		local entry =
		{
			index = tostring(i),
			name = definition.name,
			type = typename(definition.type),
			offset = tostring(definition.offset)
		}
		insert(entries, entry)
	end

	self.entries = entries

	updatesearch(self, definitions_searchcompare, true)
	return true
end

local function definitions_onhide(self)
	resetsearch(self)
	self.entries = nil
	return true
end

local function strings_searchcompare(entry, string)
	return entry.value:lower():find(string, 1, true)
end

local function strings_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
		local searchmodified = searchbar(self)
		local entries = updatesearch(self, strings_searchcompare, searchmodified)

		if imBeginTable(title, 3, defaultTableFlags) then
			imTableSetupScrollFreeze(0, 1)
			imTableSetupColumn('Index', imTableColumnWidthFixed)
			imTableSetupColumn('Value')
			imTableSetupColumn('Offset', imTableColumnWidthFixed)
			imTableHeadersRow()

			for _, entry in ipairs(entries) do
				imTableNextRow()
				imTableNextColumn()
				imText(entry.index)
				imTableNextColumn()
				imText(entry.value)
				imTableNextColumn()
				imText(entry.offset)
			end

			imEndTable()
		end
	end

	imEnd()

	return opened
end

local function strings_onshow(self)
	local entries = {}

	for i, value in ipairs(strings) do
		local entry =
		{
			index = tostring(i),
			value = value,
			offset = stringoffset(i)
		}
		insert(entries, entry)
	end

	self.entries = entries

	updatesearch(self, strings_searchcompare, true)
	return true
end

local function strings_onhide(self)
	resetsearch(self)
	self.entries = nil
	return true
end

expmode.progs = {}

local exprpogs <const> = expmode.progs

function exprpogs.functions()
	window('Progs Functions', functions_onupdate,
		function (self) self:setconstraints() end,
		functions_onshow, functions_onhide)
end

function exprpogs.fielddefinitions()
	local function oncreate(self)
		self:setconstraints()
		self.definitions = fielddefinitions
	end

	window('Field Definitions', definitions_onupdate,
		oncreate, definitions_onshow, definitions_onhide)
end

function exprpogs.globaldefinitions()
	local function oncreate(self)
		self:setconstraints()
		self.definitions = globaldefinitions
	end

	window('Global Definitions', definitions_onupdate,
		oncreate, definitions_onshow, definitions_onhide)
end

function exprpogs.strings()
	window('Progs Strings', strings_onupdate,
		function (self) self:setconstraints() end,
		strings_onshow, strings_onhide)
end

local expfunctions <const> = exprpogs.functions
local expfielddefinitions <const> = exprpogs.fielddefinitions
local expglobaldefinitions <const> = exprpogs.globaldefinitions
local expstrings <const> = exprpogs.strings

addaction(function ()
	if imBeginMenu('Progs') then
		if imMenuItem('Functions\u{85}') then
			expfunctions()
		end

		imSeparator()

		if imMenuItem('Field Definitions\u{85}') then
			expfielddefinitions()
		end

		if imMenuItem('Global Definitions\u{85}') then
			expglobaldefinitions()
		end

		imSeparator()

		if imMenuItem('Strings\u{85}') then
			expstrings()
		end

		imEndMenu()
	end
end)
