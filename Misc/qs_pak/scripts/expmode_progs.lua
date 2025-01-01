
local tostring <const> = tostring

local format <const> = string.format

local insert <const> = table.insert

local imAlignTextToFramePadding <const> = ImGui.AlignTextToFramePadding
local imBegin <const> = ImGui.Begin
local imBeginCombo <const> = ImGui.BeginCombo
local imBeginMenu <const> = ImGui.BeginMenu
local imBeginTable <const> = ImGui.BeginTable
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

local imSpanAllColumns <const> = ImGui.SelectableFlags.SpanAllColumns
local imTableColumnWidthFixed <const> = ImGui.TableColumnFlags.WidthFixed
local imWindowNoSavedSettings <const> = ImGui.WindowFlags.NoSavedSettings

local defaultTableFlags <const> = imTableFlags.Borders | imTableFlags.Resizable | imTableFlags.RowBg | imTableFlags.ScrollY

local enginestrings <const> = progs.enginestrings
local fielddefinitions <const> = progs.fielddefinitions
local functions <const> = progs.functions
local globaldefinitions <const> = progs.globaldefinitions
local op_done <const> = progs.ops.DONE
local statements <const> = progs.statements
local strings <const> = progs.strings
local stringoffset <const> = strings.offset
local typename <const> = progs.typename

local isfree <const> = edicts.isfree

local addaction <const> = expmode.addaction
local resetsearch <const> = expmode.resetsearch
local searchbar <const> = expmode.searchbar
local updatesearch <const> = expmode.updatesearch
local window <const> = expmode.window

local defaultDisassemblySize <const> = imVec2(640, 0)

local function functiondisassembly_onupdate(self)
	local visible, opened = imBegin(self.title, true, imWindowNoSavedSettings)

	if visible and opened then
		if imBeginTable(self.name, 6, defaultTableFlags) then
			imTableSetupScrollFreeze(0, 1)
			imTableSetupColumn('Address', imTableColumnWidthFixed)
			imTableSetupColumn('Bytecode', imTableColumnWidthFixed)
			imTableSetupColumn('Operation')
			imTableSetupColumn('Operand A')
			imTableSetupColumn('Operand B')
			imTableSetupColumn('Operand C')
			imTableHeadersRow()

			for _, entry in ipairs(self.entries) do
				imTableNextRow()
				imTableNextColumn()
				imText(entry.address)
				imTableNextColumn()
				imText(entry.bytecode)
				imTableNextColumn()
				imText(entry.op)
				imTableNextColumn()
				imText(entry.a)
				imTableNextColumn()
				imText(entry.b)
				imTableNextColumn()
				imText(entry.c)
			end

			imEndTable()
		end
	end

	imEnd()

	return opened
end

local function functiondisassembly_onshow(self)
	local func = self.func

	if self.name ~= func.name then
		return false
	end

	local entrypoint = func.entrypoint

	if entrypoint < 0 then
		return false  -- built-in function, nothing to disassemble
	end

	local entries = {}
	local statementcount = #statements

	for i = entrypoint, statementcount do
		local st = statements[i]
		local entry =
		{
			address = format('%06i', i),
			bytecode = format('%02x %04x %04x %04x', st.op, st.a, st.b, st.c),
			op = st.opstring,
			a = st.astring,
			b = st.bstring,
			c = st.cstring,
		}
		insert(entries, entry)

		if st.op == op_done then
			break
		end
	end

	self.entries = entries

	return true
end

local function functiondisassembly_onhide(self)
	self.entries = nil
	return true
end

local function function_searchcompare(entry, string)
	return entry.declaration:lower():find(string, 1, true)
		or entry.filename:lower():find(string, 1, true)
end

local function function_declarationcell(index, entry)
	local func = entry.func

	if func.entrypoint > 0 then
		if imSelectable(entry.declaration, false, imSpanAllColumns) then
			local funcname = func.name

			local function oncreate(this)
				this:setconstraints()
				this:setsize(defaultDisassemblySize)
				this:movetocursor()

				this.func = func
				this.name = funcname
			end

			window(format('Disassembly of #%i %s()', index, funcname), functiondisassembly_onupdate,
				oncreate, functiondisassembly_onshow, functiondisassembly_onhide)
		end
	else
		-- Built-in function, nothing to disassemble
		imText(entry.declaration)
	end
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
				function_declarationcell(i, entry)
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
			declaration = func.entrypoint > 0 and format('%s##%i', func, i) or tostring(func),
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
		entry.value = value and tostring(value) or ''
	end

	self.edictindex = index

	updatesearch(self, definitions_searchcompare, true)
end

local function definitions_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
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

		local searchmodified = searchbar(self)
		local entries = updatesearch(self, definitions_searchcompare, searchmodified)

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
		return currentedict[definition.name] or ''
	end

	local function globalvalue(definition)
		return definition.value or ''
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
