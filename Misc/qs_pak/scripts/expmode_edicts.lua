
local ipairs <const> = ipairs
local tostring <const> = tostring

local tointeger <const> = math.tointeger

local format <const> = string.format

local concat <const> = table.concat
local insert <const> = table.insert

local imBegin <const> = ImGui.Begin
local imBeginMenu <const> = ImGui.BeginMenu
local imBeginPopup <const> = ImGui.BeginPopup
local imBeginPopupContextItem <const> = ImGui.BeginPopupContextItem
local imBeginTable <const> = ImGui.BeginTable
local imCalcTextSize <const> = ImGui.CalcTextSize
local imEnd <const> = ImGui.End
local imEndMenu <const> = ImGui.EndMenu
local imEndPopup <const> = ImGui.EndPopup
local imEndTable <const> = ImGui.EndTable
local imIsItemHovered <const> = ImGui.IsItemHovered
local imIsMouseReleased <const> = ImGui.IsMouseReleased
local imMenuItem <const> = ImGui.MenuItem
local imOpenPopup <const> = ImGui.OpenPopup
local imSelectable <const> = ImGui.Selectable
local imSeparator <const> = ImGui.Separator
local imSetClipboardText <const> = ImGui.SetClipboardText
local imSetTooltip <const> = ImGui.SetTooltip
local imSpacing <const> = ImGui.Spacing
local imTableGetColumnFlags <const> = ImGui.TableGetColumnFlags
local imTableHeadersRow <const> = ImGui.TableHeadersRow
local imTableNextColumn <const> = ImGui.TableNextColumn
local imTableNextRow <const> = ImGui.TableNextRow
local imTableSetupColumn <const> = ImGui.TableSetupColumn
local imTableSetupScrollFreeze <const> = ImGui.TableSetupScrollFreeze
local imText <const> = ImGui.Text
local imVec2 <const> = vec2.new

local imTableColumnFlags <const> = ImGui.TableColumnFlags
local imTableFlags <const> = ImGui.TableFlags

local imHoveredFlagsDelayNormal <const> = ImGui.HoveredFlags.DelayNormal
local imMouseButtonRight <const> = ImGui.MouseButton.Right
local imNoOpenOverExistingPopup <const> = ImGui.PopupFlags.NoOpenOverExistingPopup
local imSelectableDisabled <const> = ImGui.SelectableFlags.Disabled
local imTableColumnIsHovered <const> = imTableColumnFlags.IsHovered
local imTableColumnWidthFixed <const> = imTableColumnFlags.WidthFixed
local imWindowNoSavedSettings <const> = ImGui.WindowFlags.NoSavedSettings

local defaulttableflags <const> = imTableFlags.Borders | imTableFlags.Resizable | imTableFlags.RowBg
local defaultscrollytableflags <const> = defaulttableflags | imTableFlags.ScrollY

local type_entity <const> = progs.types.entity
local type_float  <const> = progs.types.float
local type_string <const> = progs.types.string
local type_vector <const> = progs.types.vector

local isany <const> = edicts.isany
local isfree <const> = edicts.isfree

local addaction <const> = expmode.addaction
local exit <const> = expmode.exit
local messagebox <const> = expmode.messagebox
local resetsearch <const> = expmode.resetsearch
local searchbar <const> = expmode.searchbar
local updatesearch <const> = expmode.updatesearch
local window <const> = expmode.window

local ghost <const> = player.ghost
local setpos <const> = player.setpos
local traceentity <const> = player.traceentity

local localize <const> = text.localize
local toascii <const> = text.toascii

local vec3mid <const> = vec3.mid
local vec3origin <const> = vec3.new()

local function moveplayer(edict, location, angles)
	location = location or vec3mid(edict.absmin, edict.absmax)

	if location then
		if edicts.isitem(edict) then
			-- Adjust Z coordinate so player will appear slightly above destination
			location = vec3.copy(location)
			location.z = location.z + 20
		end

		ghost(true)
		setpos(location, angles or edict.angles)

		exit()
	end
end

local function edict_contextmenuentry_destructive(edict)
	imSeparator()

	if imSelectable('Destroy') then
		if edicts.destroy(edict) then
			exit()
		end
	end

	if imSelectable('Remove') then
		if edicts.remove(edict) then
			exit()
		end
	end
end

local function edictinfo_searchcompare(entry, string)
	local function contains(value)
		return value:lower():find(string, 1, true)
	end

	return contains(entry.name) or contains(entry.value)
end

local function edictinfo_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true, imWindowNoSavedSettings)

	if visible and opened then
		local searchmodified = searchbar(self)
		local entries = updatesearch(self, edictinfo_searchcompare, searchmodified)

		-- Table of fields names and values
		if imBeginTable(title, 2, defaultscrollytableflags) then
			imTableSetupScrollFreeze(0, 1)
			imTableSetupColumn('Name', imTableColumnWidthFixed)
			imTableSetupColumn('Value')
			imTableHeadersRow()

			for _, field in ipairs(entries) do
				imTableNextRow()
				imTableNextColumn()
				imText(field.name)
				imTableNextColumn()

				if field.edict then
					if imSelectable(field.selectableid) then
						expmode.edictinfo(field.edict):movetocursor()
					end
				elseif field.vector then
					if imSelectable(field.selectableid) then
						ghost(true)
						setpos(field.vector)
						exit()
					end
				else
					imText(field.value)
				end
			end

			local popupname = 'edictinfo_popup'

			for column = 0, 1 do
				local ispopup = (imTableGetColumnFlags(column) & imTableColumnIsHovered) ~= 0
					and imIsMouseReleased(imMouseButtonRight)

				if ispopup then
					imOpenPopup(popupname, imNoOpenOverExistingPopup)
					break
				end
			end

			if imBeginPopup(popupname) then
				if imSelectable('Move to') then
					moveplayer(self.edict)
				end
				if imSelectable('References') then
					expmode.edictreferences(self.edict):movetocursor()
				end
				if imSelectable('Copy All') then
					local fields = {}

					for i, field in ipairs(entries) do
						fields[i] = field.name .. ': ' .. field.value
					end

					imSetClipboardText(concat(fields, '\n') .. '\n')
				end
				edict_contextmenuentry_destructive(self.edict)
				imEndPopup()
			end

			imEndTable()
		end
	end

	imEnd()

	return opened
end

local function edictinfo_onshow(self)
	local title = self.title

	if tostring(self.edict) ~= title then
		return
	end

	local fields = {}

	for i, field in ipairs(self.edict) do
		local value = field.value
		local valuetype = field.type

		if valuetype == type_string then
			value = toascii(localize(value))
		else
			if valuetype == type_entity then
				field.edict = value
				field.selectableid = format('%s##%s', value, field.name)
			elseif valuetype == type_vector then
				field.vector = value
				field.selectableid = format('%s##%s', value, field.name)
			elseif valuetype == type_float then
				---@diagnostic disable-next-line: cast-local-type
				value = tointeger(value) or value
			end

			value = tostring(value)
		end

		field.value = value
		fields[i] = field
	end

	self.entries = fields

	updatesearch(self, edictinfo_searchcompare, true)
	return true
end

local function edictinfo_onhide(self)
	resetsearch(self)
	self.fields = nil
	return true
end

function expmode.edictinfo(edict)
	if isfree(edict) then
		return
	end

	local function oncreate(self)
		self:setconstraints()
		self.edict = edict
	end

	return window(tostring(edict), edictinfo_onupdate, oncreate, edictinfo_onshow, edictinfo_onhide)
end

local edictinfo <const> = expmode.edictinfo

local function edictstable_contextmenu_location(entry)
	local location = entry.location
	return location and '\t' .. location or ''
end

local function edictstable_contextmenu(entries, current, cellvalue)
	if imBeginPopupContextItem() then
		if imSelectable('References') then
			expmode.edictreferences(current.edict):movetocursor()
		end
		imSeparator()
		if imSelectable('Copy Cell') then
			imSetClipboardText(tostring(cellvalue))
		end
		if imSelectable('Copy Row') then
			local location = edictstable_contextmenu_location(current)
			imSetClipboardText(format('%d\t%s%s\n', current.index, current.description, location))
		end
		if imSelectable('Copy Table') then
			local lines = {}

			for _, entry in ipairs(entries) do
				local location = edictstable_contextmenu_location(entry)
				local line = format('%d\t%s%s', entry.index, entry.description, location)
				insert(lines, line)
			end

			imSetClipboardText(concat(lines, '\n') .. '\n')
		end
		edict_contextmenuentry_destructive(current.edict)
		imEndPopup()
	end
end

local function edictstable(title, entries, tableflags)
	if imBeginTable(title, 3, tableflags or defaulttableflags) then
		imTableSetupScrollFreeze(0, 1)
		imTableSetupColumn('Index', imTableColumnWidthFixed)
		imTableSetupColumn('Description')
		imTableSetupColumn('Location')
		imTableHeadersRow()

		for _, entry in ipairs(entries) do
			local description = entry.description

			imTableNextRow()
			imTableNextColumn()
			imSelectable(entry.index, false, imSelectableDisabled)
			imTableNextColumn()

			if entry.isfree then
				imSelectable(entry.descriptionid, false, imSelectableDisabled)
			else
				if imSelectable(entry.descriptionid) then
					edictinfo(entry.edict):movetocursor()
				end
				if imIsItemHovered(imHoveredFlagsDelayNormal) then
					imSetTooltip(tostring(entry.edict))
				end
				edictstable_contextmenu(entries, entry, description)

				imTableNextColumn()

				if imSelectable(entry.locationid) then
					moveplayer(entry.edict, entry.location, entry.angles)
				end
				if imIsItemHovered(imHoveredFlagsDelayNormal) then
					local edict = entry.edict
					local absmin = edict.absmin
					local absmax = edict.absmax
					local angles = entry.angles

					if absmin and absmax then
						local text = format('min: %s\nmax: %s', absmin, absmax)

						if angles and angles ~= vec3origin then
							text = format('%s\nangles: %s', text, angles)
						end

						imSetTooltip(text)
					end
				end
				edictstable_contextmenu(entries, entry, entry.location)
			end
		end

		imEndTable()
	end
end

local function edicts_searchcompare(entry, string)
	return entry.description:lower():find(string, 1, true)
		or tostring(entry.location):find(string, 1, true)
end

local function edicts_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
		local searchmodified = searchbar(self)
		local entries = updatesearch(self, edicts_searchcompare, searchmodified)
		edictstable(title, entries, defaultscrollytableflags)
	end

	imEnd()

	return opened
end

local function edicts_addentry(filter, edict, index, entries)
	local description, location, angles = filter(edict)

	if not description then
		return index
	end

	local freed = isfree(edict)
	description = toascii(description)

	local entry =
	{
		edict = edict,
		isfree = freed,
		index = index,
		description = description,
		descriptionid = format('%s##%d', description, index),
	}

	if not freed then
		entry.location = location
		entry.locationid = format('%s##%d', location, index)
		entry.angles = angles
	end

	insert(entries, entry)
	return index + 1
end

local function edicts_onshow(self)
	local filter = self.filter or isany
	local entries = {}
	local index = self.filter and 1 or 0

	for _, edict in ipairs(edicts) do
		index = edicts_addentry(filter, edict, index, entries)
	end

	self.entries = entries

	updatesearch(self, edicts_searchcompare, true)
	return true
end

local function edicts_onhide(self)
	resetsearch(self)
	self.entries = nil
	return true
end

local function edictrefs_onupdate(self)
	local visible, opened = imBegin(self.title, true, imWindowNoSavedSettings)

	if visible and opened then
		local references = self.references

		if #references > 0 then
			imText('References')
			edictstable('#', references)
			imSpacing()
		end

		local referencedby = self.referencedby

		if #referencedby > 0 then
			imText('Referenced by')
			edictstable('#', referencedby)
		end
	end

	imEnd()

	return opened
end

local function edictrefs_addentries(source, list)
	local index = 1

	for _, edict in ipairs(source) do
		index = edicts_addentry(isany, edict, index, list)
	end
end

local function edictrefs_onshow(self)
	local edict = self.edict

	if tostring(edict) ~= self.edictid then
		return
	end

	local outgoing, incoming = edicts.references(edict)

	if #outgoing == 0 and #incoming == 0 then
		return
	end

	local references = {}
	edictrefs_addentries(outgoing, references)

	local referencedby = {}
	edictrefs_addentries(incoming, referencedby)

	self.references = references
	self.referencedby = referencedby

	return true
end

local function edictrefs_onhide(self)
	self.references = nil
	self.referencedby = nil

	return true
end

function expmode.edictreferences(edict)
	if isfree(edict) then
		return
	end

	local edictid = tostring(edict)
	local title = 'References of ' .. edictid

	local function oncreate(self)
		self:setconstraints()
		self.edict = edict
		self.edictid = edictid
	end

	return window(title, edictrefs_onupdate, oncreate, edictrefs_onshow, edictrefs_onhide)
		or messagebox('No references', format("'%s' has no references.", edict))
end

expmode.edicts = {}

function expmode.edicts.traceentity()
	local edict = traceentity()

	if edict then
		edictinfo(edict)
	else
		messagebox('No entity', 'Player is not looking at an interactible entity.')
	end
end

local edictstools <const> =
{
	-- Name, filter function, default width of 'Description' cell in characters
	{ 'All Edicts', nil, 30 },
	{ 'Secrets', edicts.issecret, 15 },
	{ 'Monsters', edicts.ismonster, 15 },
	{ 'Teleports', edicts.isteleport, 30 },
	{ 'Doors', edicts.isdoor, 15 },
	{ 'Items', edicts.isitem, 20 },
	{ 'Buttons', edicts.isbutton, 15 },
	{ 'Exits', edicts.isexit, 15 },
	{ 'Messages', edicts.ismessage, 40 },
	{ 'Models', edicts.ismodel, 25 },
}

for _, tool in ipairs(edictstools) do
	local title = tool[1]
	local filter = tool[2]
	local extrawidth = tool[3]
	local name = filter and title:lower() or 'all'

	local function toolfunc()
		local function oncreate(self)
			local defaultwidthchars <const> = 35  -- in characters, for whole window except 'Description' cell
			local width = imCalcTextSize('A').x * (defaultwidthchars + extrawidth)

			self:setconstraints()
			self:setsize(imVec2(width, 0))
			self.filter = filter
		end

		return window(title, edicts_onupdate, oncreate, edicts_onshow, edicts_onhide)
	end

	tool[2] = toolfunc
	tool[3] = nil

	expmode.edicts[name] = toolfunc
end

addaction(function ()
	if imBeginMenu('Edicts') then
		for _, tool in ipairs(edictstools) do
			if imMenuItem(tool[1] .. '\u{85}') then
				tool[2]()
			end
		end

		imSeparator()

		if imMenuItem('Trace Entity\u{85}') then
			expmode.edicts.traceentity()
		end

		imEndMenu()
	end
end)
