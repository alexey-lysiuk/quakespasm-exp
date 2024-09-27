
local tostring <const> = tostring

local format <const> = string.format

local insert <const> = table.insert

local imAlignTextToFramePadding <const> = ImGui.AlignTextToFramePadding
local imBegin <const> = ImGui.Begin
local imBeginCombo <const> = ImGui.BeginCombo
local imBeginMenu <const> = ImGui.BeginMenu
local imBeginTable <const> = ImGui.BeginTable
local imCheckbox <const> = ImGui.Checkbox
local imColorTextEdit <const> = ImGui.ColorTextEdit
local imEnd <const> = ImGui.End
local imEndCombo <const> = ImGui.EndCombo
local imEndMenu <const> = ImGui.EndMenu
local imEndTable <const> = ImGui.EndTable
local imMenuItem <const> = ImGui.MenuItem
local imSameLine <const> = ImGui.SameLine
local imSelectable <const> = ImGui.Selectable
local imSeparator <const> = ImGui.Separator
local imSetItemDefaultFocus <const> = ImGui.SetItemDefaultFocus
local imTableHeadersRow <const> = ImGui.TableHeadersRow
local imTableNextColumn <const> = ImGui.TableNextColumn
local imTableNextRow <const> = ImGui.TableNextRow
local imTableSetupColumn <const> = ImGui.TableSetupColumn
local imTableSetupScrollFreeze <const> = ImGui.TableSetupScrollFreeze
local imText <const> = ImGui.Text
local imVec2 <const> = vec2.new

local imTableFlags <const> = ImGui.TableFlags

local imTableColumnWidthFixed <const> = ImGui.TableColumnFlags.WidthFixed
local imWindowNoSavedSettings <const> = ImGui.WindowFlags.NoSavedSettings

local defaultTableFlags <const> = imTableFlags.Borders | imTableFlags.Resizable | imTableFlags.RowBg | imTableFlags.ScrollY

local enginestrings <const> = progs.enginestrings
local fielddefinitions <const> = progs.fielddefinitions
local functions <const> = progs.functions
local globaldefinitions <const> = progs.globaldefinitions
local typename <const> = progs.typename
local strings <const> = progs.strings
local stringoffset <const> = strings.offset

local isfree <const> = edicts.isfree

local addaction <const> = expmode.addaction
local resetsearch <const> = expmode.resetsearch
local searchbar <const> = expmode.searchbar
local updatesearch <const> = expmode.updatesearch
local window <const> = expmode.window

local defaultDisassemblySize <const> = imVec2(640, 480)

local function functiondisassembly_onupdate(self)
	local visible, opened = imBegin(self.title, true, imWindowNoSavedSettings)

	if visible and opened then
		local textview = self.textview

		if not textview then
			local disassembly = self.func:disassemble(self.withbinary)
			textview = imColorTextEdit()
			textview:SetReadOnly(true)
			textview:SetText(disassembly)

			self.textview = textview
		end

		-- TODO: Do not show binary checkbox for built-in functions
		local binarypressed, binaryenabled = imCheckbox('Show statements binaries', self.withbinary)

		if binarypressed then
			local disassembly = self.func:disassemble(binaryenabled)
			textview:SetText(disassembly)
			self.withbinary = binaryenabled
		end

		textview:Render('##text')
	end

	imEnd()

	return opened
end

local function functiondisassembly_onshow(self)
	if self.name ~= self.func.name then
		return false
	end

	if not self.withbinary then
		self.withbinary = false
	end

	return true
end

local function functiondisassembly_onhide(self)
	self.textview = nil
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

			for i, entry in ipairs(entries) do
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

					window(format('Disassembly of #%i %s()', i, funcname), functiondisassembly_onupdate,
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
		local entry =
		{
			func = func,
			index = tostring(i),
			declaration = format('%s##%i', func, i),
			filename = func.filename
		}
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
		or entry.value:lower():find(string, 1, true)
		or entry.type:find(string, 1, true)
		or entry.offset:find(string, 1, true)
end

local function definitions_edictchanged(self, index)
	local currentedict = edicts[index]

	for _, entry in ipairs(self.entries) do
		local value = currentedict[entry.name]
		entry.value = tostring(value)
	end

	self.edictindex = index
end

local function definitions_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
		local searchmodified = searchbar(self)
		local entries = updatesearch(self, definitions_searchcompare, searchmodified)

		if self.definitions == fielddefinitions then
			imAlignTextToFramePadding()
			imText('Values:')
			imSameLine()

			local edictindex = self.edictindex
			local currentedict = edicts[edictindex]

			if imBeginCombo('##edicts', tostring(currentedict)) then
				for i, edict in ipairs(edicts) do
					if not isfree(edict) then
						local selected = edictindex == i

						if imSelectable(tostring(edict), selected) then
							definitions_edictchanged(self, i)
						end

						if selected then
							imSetItemDefaultFocus()
						end
					end
				end

				imEndCombo()
			end
		end

		if imBeginTable(title, 5, defaultTableFlags) then
			imTableSetupScrollFreeze(0, 1)
			imTableSetupColumn('Index', imTableColumnWidthFixed)
			imTableSetupColumn('Name')
			imTableSetupColumn('Value')
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
				imText(entry.value)
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

local function definitions_currentedict(self)
	local index = self.edictindex
	local invalid = not index
		or index <= 0
		or index > #edicts
		or isfree(edicts[index])

	if invalid then
		index = 2  -- player
		self.edictindex = index
	end

	return edicts[index]
end

local function definitions_onshow(self)
	local definitions = self.definitions
	local valuefunc, currentedict
	local entries = {}

	local function fieldvalue(definition)
		return currentedict[definition.name]
	end

	local function globalvalue(definition)
		return definition.value
	end

	if definitions == fielddefinitions then
		valuefunc = fieldvalue
		currentedict = definitions_currentedict(self)
	else
		valuefunc = globalvalue
	end

	for i, definition in ipairs(definitions) do
		local entry =
		{
			index = tostring(i),
			name = definition.name,
			value = tostring(valuefunc(definition)),
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
		or entry.offset and entry.offset:find(string, 1, true)
end

local function strings_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
		local searchmodified = searchbar(self)
		local entries = updatesearch(self, strings_searchcompare, searchmodified)
		local hasoffset = self.offsetfunc

		if imBeginTable(title, hasoffset and 3 or 2, defaultTableFlags) then
			imTableSetupScrollFreeze(0, 1)
			imTableSetupColumn('Index', imTableColumnWidthFixed)
			imTableSetupColumn('Value')
			if hasoffset then
				imTableSetupColumn('Offset', imTableColumnWidthFixed)
			end
			imTableHeadersRow()

			for _, entry in ipairs(entries) do
				imTableNextRow()
				imTableNextColumn()
				imText(entry.index)
				imTableNextColumn()
				imText(entry.value)

				if hasoffset then
					imTableNextColumn()
					imText(entry.offset)
				end
			end

			imEndTable()
		end
	end

	imEnd()

	return opened
end

local function strings_onshow(self)
	local entries = {}
	local hasoffset = self.offsetfunc

	for i, value in ipairs(self.strings) do
		local entry =
		{
			index = tostring(i),
			value = value,

		}

		if hasoffset then
			entry.offset = tostring(stringoffset(i))
		end

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

local function details_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
		if imBeginTable(title, 2, defaultTableFlags) then
			imTableSetupScrollFreeze(0, 1)
			imTableSetupColumn('Name', imTableColumnWidthFixed)
			imTableSetupColumn('Value')
			imTableHeadersRow()

			for _, entry in ipairs(self.entries) do
				imTableNextRow()
				imTableNextColumn()
				imText(entry[1])
				imTableNextColumn()
				imText(entry[2])
			end

			imEndTable()
		end
	end

	imEnd()

	return opened
end

local function details_onshow(self)
	local crc <const> = progs.datcrc
	local stringcount <const> = #strings

	self.entries =
	{
		{ 'Game Directory', host.gamedir() },
		{ 'Detected Mod', progs.modname(progs.detectmod()) },
		{ 'File CRC', format('%i (0x%X)', crc, crc) },
		{ 'Functions', tostring(#functions) },
		{ 'Global Definitions', tostring(#globaldefinitions) },
		{ 'Field Definitions', tostring(#fielddefinitions) },
		{ 'Global Variables', tostring(#progs.globalvariables) },
		{ 'Strings', tostring(stringcount) },
		{ 'Strings Pool Size', tostring(strings.offset(stringcount) + #strings[stringcount] + 1) },
	}

	return true
end

local function details_onhide(self)
	self.entries = nil
	return true
end

expmode.progs = {}

local exprpogs <const> = expmode.progs

function exprpogs.functions()
	return window('Progs Functions', functions_onupdate,
		function (self) self:setconstraints() end,
		functions_onshow, functions_onhide)
end

local function definitionstool(name, table)
	local function oncreate(self)
		self:setconstraints()
		self.definitions = table
	end

	return window(name, definitions_onupdate, oncreate, definitions_onshow, definitions_onhide)
end

function exprpogs.fielddefinitions()
	definitionstool('Field Definitions', fielddefinitions)
end

function exprpogs.globaldefinitions()
	definitionstool('Global Definitions', globaldefinitions)
end

local function stringstool(name, table, offsetfunc)
	local function oncreate(self)
		self:setconstraints()
		self.strings = table
		self.offsetfunc = offsetfunc
	end

	return window(name, strings_onupdate, oncreate, strings_onshow, strings_onhide)
end

function exprpogs.strings()
	stringstool('Progs Strings', strings, stringoffset)
end

function exprpogs.enginestrings()
	stringstool('Engine/Known Strings', enginestrings)
end

function exprpogs.details()
	return window('Progs Details', details_onupdate,
		function (self) self:setconstraints() end,
		details_onshow, details_onhide)
end

local expfunctions <const> = exprpogs.functions
local expfielddefinitions <const> = exprpogs.fielddefinitions
local expglobaldefinitions <const> = exprpogs.globaldefinitions
local expstrings <const> = exprpogs.strings
local expenginestrings <const> = exprpogs.enginestrings
local expdetails <const> = exprpogs.details

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

		if imMenuItem('Engine Strings\u{85}') then
			expenginestrings()
		end

		imSeparator()

		if imMenuItem('Details\u{85}') then
			expdetails()
		end

		imEndMenu()
	end
end)
