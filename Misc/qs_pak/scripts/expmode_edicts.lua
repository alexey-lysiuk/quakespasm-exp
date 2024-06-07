
local ipairs <const> = ipairs
local tostring <const> = tostring

local tointeger <const> = math.tointeger

local format <const> = string.format

local concat <const> = table.concat
local insert <const> = table.insert

local imAlignTextToFramePadding <const> = ImGui.AlignTextToFramePadding
local imBegin <const> = ImGui.Begin
local imBeginMenu <const> = ImGui.BeginMenu
local imBeginPopup <const> = ImGui.BeginPopup
local imBeginPopupContextItem <const> = ImGui.BeginPopupContextItem
local imBeginTable <const> = ImGui.BeginTable
local imButton <const> = ImGui.Button
local imCalcTextSize <const> = ImGui.CalcTextSize
local imEnd <const> = ImGui.End
local imEndMenu <const> = ImGui.EndMenu
local imEndPopup <const> = ImGui.EndPopup
local imEndTable <const> = ImGui.EndTable
local imInputText <const> = ImGui.InputText
local imIsItemHovered <const> = ImGui.IsItemHovered
local imIsMouseReleased <const> = ImGui.IsMouseReleased
local imIsWindowAppearing <const> = ImGui.IsWindowAppearing
local imMenuItem <const> = ImGui.MenuItem
local imOpenPopup <const> = ImGui.OpenPopup
local imSameLine <const> = ImGui.SameLine
local imSelectable <const> = ImGui.Selectable
local imSeparator <const> = ImGui.Separator
local imSetClipboardText <const> = ImGui.SetClipboardText
local imSetKeyboardFocusHere <const> = ImGui.SetKeyboardFocusHere
local imSetTooltip <const> = ImGui.SetTooltip
local imSpacing <const> = ImGui.Spacing
local imTableGetColumnFlags <const> = ImGui.TableGetColumnFlags
local imTableHeadersRow <const> = ImGui.TableHeadersRow
local imTableNextColumn <const> = ImGui.TableNextColumn
local imTableNextRow <const> = ImGui.TableNextRow
local imTableSetupColumn <const> = ImGui.TableSetupColumn
local imText <const> = ImGui.Text
local imTextBuffer <const> = ImGui.TextBuffer
local imVec2 <const> = ImGui.ImVec2

local imTableColumnFlags <const> = ImGui.TableColumnFlags
local imTableFlags <const> = ImGui.TableFlags
local imWindowFlags <const> = ImGui.WindowFlags

local imHoveredFlagsDelayNormal <const> = ImGui.HoveredFlags.DelayNormal
local imMouseButtonRight <const> = ImGui.MouseButton.Right
local imNoOpenOverExistingPopup <const> = ImGui.PopupFlags.NoOpenOverExistingPopup
local imSelectableDisabled <const> = ImGui.SelectableFlags.Disabled
local imTableColumnIsHovered <const> = imTableColumnFlags.IsHovered
local imTableColumnWidthFixed <const> = imTableColumnFlags.WidthFixed
local imWindowNoSavedSettings <const> = imWindowFlags.NoSavedSettings

local defaulttableflags <const> = imTableFlags.Borders | imTableFlags.Resizable | imTableFlags.RowBg
local defaultscrollytableflags <const> = defaulttableflags | imTableFlags.ScrollY

local isany <const> = edicts.isany
local isfree <const> = edicts.isfree
local getname <const> = edicts.getname
local type_entity <const> = edicts.valuetypes.entity
local type_float <const> = edicts.valuetypes.float
local type_string <const> = edicts.valuetypes.string
local type_vector <const> = edicts.valuetypes.vector

local addaction <const> = expmode.addaction
local messagebox <const> = expmode.messagebox
local window <const> = expmode.window

local ghost <const> = player.ghost
local setpos <const> = player.setpos
local traceentity <const> = player.traceentity

local localize <const> = text.localize
local toascii <const> = text.toascii

local vec3mid <const> = vec3.mid

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

		expmode.exit()
	end
end

local function searchbar(window)
	imAlignTextToFramePadding()
	imText('Search:')
	imSameLine()

	if imIsWindowAppearing() then
		imSetKeyboardFocusHere()
	end

	local modified = imInputText('##search', window.searchbuffer)

	if #window.searchbuffer > 0 then
		if imIsItemHovered(imHoveredFlagsDelayNormal) then
			local searchresults = window.searchresults
			local count = searchresults and #searchresults or -1

			if count >= 0 then
				imSetTooltip(count .. ' result(s)')
			end
		end

		imSameLine(0, 0)

		if imButton('x') then
			window.searchbuffer = nil
			modified = true
		end
	end

	return modified
end

local function updatesearch(window, compfunc, modified)
	local searchbuffer = window.searchbuffer

	if not searchbuffer then
		searchbuffer = imTextBuffer()
		window.searchbuffer = searchbuffer
	end

	if modified then
		local searchstring = tostring(searchbuffer):lower()

		if #searchstring > 0 then
			local searchresults = {}

			for _, entry in ipairs(window.entries) do
				if compfunc(entry, searchstring) then
					insert(searchresults, entry)
				end
			end

			window.searchresults = searchresults
		else
			window.searchresults = nil
		end
	end

	return #searchbuffer > 0 and window.searchresults or window.entries
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
						expmode.exit()
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
				if imSelectable('Copy all') then
					local fields = {}

					for i, field in ipairs(entries) do
						fields[i] = field.name .. ': ' .. field.value
					end

					imSetClipboardText(concat(fields, '\n') .. '\n')
				end
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
	self.searchresults = nil
	self.fields = nil
	return true
end

function expmode.edictinfo(edict)
	if isfree(edict) then
		return
	end

	return window(tostring(edict), edictinfo_onupdate,
		function (self) self.edict = edict end,
		edictinfo_onshow, edictinfo_onhide):setconstraints()
end

local edictinfo <const> = expmode.edictinfo

local function edictstable_tostring(entries)
	local lines = {}

	for _, entry in ipairs(entries) do
		local line = format('%d\t%s\t%s', entry.index, entry.description, entry.location)
		insert(lines, line)
	end

	return concat(lines, '\n') .. '\n'
end

local function edictstable(title, entries, tableflags)
	if imBeginTable(title, 3, tableflags or defaulttableflags) then
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
				imSelectable(description, false, imSelectableDisabled)
			else
				local location = entry.location

				local function contextmenu(cellvalue)
					if imBeginPopupContextItem() then
						if imSelectable('References') then
							expmode.edictreferences(entry.edict):movetocursor()
						end
						imSeparator()
						if imSelectable('Copy cell') then
							imSetClipboardText(tostring(cellvalue))
						end
						if imSelectable('Copy row') then
							imSetClipboardText(format('%s\t%s\t%s\n', entry.index, description, location))
						end
						if imSelectable('Copy table') then
							imSetClipboardText(edictstable_tostring(entries))
						end
						imEndPopup()
					end
				end

				if imSelectable(entry.descriptionid) then
					edictinfo(entry.edict):movetocursor()
				end
				if imIsItemHovered(imHoveredFlagsDelayNormal) then
					imSetTooltip(tostring(entry.edict))
				end
				contextmenu(description)

				imTableNextColumn()

				if imSelectable(entry.locationid) then
					moveplayer(entry.edict, location, entry.angles)
				end
				if imIsItemHovered(imHoveredFlagsDelayNormal) then
					local edict = entry.edict
					local absmin = edict.absmin
					local absmax = edict.absmax

					if absmin and absmax then
						local bounds = format('min: %s\nmax: %s', absmin, absmax)
						imSetTooltip(bounds)
					end
				end
				contextmenu(location)
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
	}

	if not freed then
		entry.location = location
		entry.angles = angles

		-- Description and location cells need unique IDs to generate click events
		entry.descriptionid = format('%s##%d', description, index)
		entry.locationid = format('%s##%d', location, index)
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
	self.searchresults = nil
	self.entries = nil
	return true
end

local function edictrefs_onupdate(self)
	local visible, opened = imBegin(self.title, true, imWindowNoSavedSettings)

	if visible and opened then
		local references = self.references

		if #references > 0 then
			imText('References')
			edictstable('', references)
			imSpacing()
		end

		local referencedby = self.referencedby

		if #referencedby > 0 then
			imText('Referenced by')
			edictstable('', referencedby)
		end
	end

	imEnd()

	return opened
end

local function edictrefs_onshow(self)
	local edict = self.edict

	if tostring(edict) ~= self.edictid then
		return
	end

	local function addentries(source, list)
		local index = 1

		for _, edict in ipairs(source) do
			index = edicts_addentry(isany, edict, index, list)
		end
	end

	outgoing, incoming = edicts.references(edict)

	if #outgoing == 0 and #incoming == 0 then
		return
	end

	local references = {}
	addentries(outgoing, references)

	local referencedby = {}
	addentries(incoming, referencedby)

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
		self.edict = edict
		self.edictid = edictid
	end

	local refswin = window(title, edictrefs_onupdate, oncreate, edictrefs_onshow, edictrefs_onhide)
	return refswin
		and refswin:setconstraints()
		or messagebox('No references', format("'%s' has no references.", edict))
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

addaction(function ()
	if imBeginMenu('Edicts') then
		for _, tool in ipairs(edictstools) do
			local title = tool[1]

			if imMenuItem(title) then
				local defaultwidthchars <const> = 35  -- in characters, for whole window except 'Description' cell
				local width = imCalcTextSize('A').x * (defaultwidthchars + tool[3])

				window(title, edicts_onupdate,
					function (self) self.filter = tool[2] end,
					edicts_onshow, edicts_onhide):setconstraints():setsize(imVec2(width, 0))
			end
		end

		imSeparator()

		if imMenuItem('Trace Entity') then
			local edict = traceentity()

			if edict then
				edictinfo(edict)
			else
				messagebox('No entity', 'Player is not looking at an interactible entity.')
			end
		end

		imEndMenu()
	end
end)
