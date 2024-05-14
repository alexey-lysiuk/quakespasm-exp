
local ipairs <const> = ipairs
local tostring <const> = tostring

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
local imGetMainViewport <const> = ImGui.GetMainViewport
local imInputText <const> = ImGui.InputText
local imIsItemHovered <const> = ImGui.IsItemHovered
local imIsMouseReleased <const> = ImGui.IsMouseReleased
local imMenuItem <const> = ImGui.MenuItem
local imOpenPopup <const> = ImGui.OpenPopup
local imSameLine <const> = ImGui.SameLine
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
local float <const> = edicts.valuetypes.float
local string <const> = edicts.valuetypes.string

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

local function edictinfo_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true, imWindowNoSavedSettings)

	if visible and opened then
		-- Table of fields names and values
		if imBeginTable(title, 2, defaultscrollytableflags) then
			imTableSetupColumn('Name', imTableColumnWidthFixed)
			imTableSetupColumn('Value')
			imTableHeadersRow()

			for _, field in ipairs(self.fields) do
				imTableNextRow()
				imTableNextColumn()
				imText(field.name)
				imTableNextColumn()
				imText(field.value)
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
					expmode.edictreferences(self.edict)
				end
				if imSelectable('Copy all') then
					local fields = {}

					for i, field in ipairs(self.fields) do
						fields[i] = field.name .. ': ' .. field.value
					end

					imSetClipboardText(concat(fields, '\n'))
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
		field.value = field.type == string
			and toascii(localize(field.value))
			or tostring(field.value)
		fields[i] = field
	end

	self.fields = fields

	return true
end

local function edictinfo_onhide(self)
	self.fields = nil
	return true
end

function expmode.edictinfo(edict)
	if isfree(edict) then
		return
	end

	window(tostring(edict), edictinfo_onupdate,
		function (self) self.edict = edict end,
		edictinfo_onshow, edictinfo_onhide)
end

local edictinfo <const> = expmode.edictinfo

local function edictstable_tostring(entries)
	local lines = {}

	for _, entry in ipairs(entries) do
		local line = format('%d\t%s\t%s', entry.index, entry.description, entry.location)
		insert(lines, line)
	end

	return concat(lines, '\n')
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
							expmode.edictreferences(entry.edict)
						end
						imSeparator()
						if imSelectable('Copy cell') then
							imSetClipboardText(tostring(cellvalue))
						end
						if imSelectable('Copy row') then
							imSetClipboardText(format('%s\t%s\t%s', entry.index, description, location))
						end
						if imSelectable('Copy table') then
							imSetClipboardText(edictstable_tostring(entries))
						end
						imEndPopup()
					end
				end

				if imSelectable(entry.descriptionid) then
					edictinfo(entry.edict)
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

local function edicts_updatesearch(self, modified)
	local searchbuffer = self.searchbuffer

	if not searchbuffer then
		searchbuffer = imTextBuffer()
		self.searchbuffer = searchbuffer
	end

	if modified then
		local searchstring = tostring(searchbuffer):lower()

		if #searchstring > 0 then
			local searchresults = {}

			for _, entry in ipairs(self.entries) do
				local found = entry.description:lower():find(searchstring, 1, true)
					or tostring(entry.location):find(searchstring, 1, true)

				if found then
					insert(searchresults, entry)
				end
			end

			self.searchresults = searchresults
		else
			self.searchresults = nil
		end
	end

	return #searchbuffer > 0
end

local function edicts_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
		imAlignTextToFramePadding()
		imText('Search:')
		imSameLine()

		local searchmodified = imInputText('##search', self.searchbuffer)
		imSameLine()

		if imButton('Reset') then
			self.searchbuffer = nil
			searchmodified = true
		end

		local entries = edicts_updatesearch(self, searchmodified) and self.searchresults or self.entries
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

	edicts_updatesearch(self, true)
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

	if not window(title, edictrefs_onupdate, oncreate, edictrefs_onshow, edictrefs_onhide) then
		messagebox('No references', format("'%s' has no references", edict))
	end
end

local edictstools <const> =
{
	{ 'All Edicts' },
	{ 'Secrets', edicts.issecret },
	{ 'Monsters', edicts.ismonster },
	{ 'Teleports', edicts.isteleport },
	{ 'Doors', edicts.isdoor },
	{ 'Items', edicts.isitem },
	{ 'Buttons', edicts.isbutton },
	{ 'Exits', edicts.isexit },
	{ 'Messages', edicts.ismessage },
	{ 'Models', edicts.ismodel },
}

addaction(function ()
	if imBeginMenu('Edicts') then
		for _, tool in ipairs(edictstools) do
			local title = tool[1]

			if imMenuItem(title) then
				local win = window(title, edicts_onupdate,
					function (self) self.filter = tool[2] end,
					edicts_onshow, edicts_onhide)
				win.minsize = imVec2(320, 240)
				win.maxsize = imVec2(1920, 1080)
			end
		end

		imSeparator()

		if imMenuItem('Trace Entity') then
			local edict = traceentity()

			if edict then
				edictinfo(edict)
			else
				messagebox('No entity', 'Player is not looking at any entity')
			end
		end

		imEndMenu()
	end
end)
