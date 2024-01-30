
local format <const> = string.format
local insert <const> = table.insert

local imBegin <const> = imgui.Begin
local imBeginPopupContextItem <const> = imgui.BeginPopupContextItem
local imBeginTable <const> = imgui.BeginTable
local imButton <const> = imgui.Button
local imEnd <const> = imgui.End
local imEndPopup <const> = imgui.EndPopup
local imEndTable <const> = imgui.EndTable
local imGetItemRectMax <const> = imgui.GetItemRectMax
local imGetItemRectMin <const> = imgui.GetItemRectMin
local imGetMainViewport <const> = imgui.GetMainViewport
local imGetWindowContentRegionMax <const> = imgui.GetWindowContentRegionMax
local imInputTextMultiline <const> = imgui.InputTextMultiline
local imSameLine <const> = imgui.SameLine
local imSelectable <const> = imgui.Selectable
local imSeparator <const> = imgui.Separator
local imSeparatorText <const> = imgui.SeparatorText
local imSpacing <const> = imgui.Spacing
local imSetClipboardText <const> = imgui.SetClipboardText
local imSetNextWindowFocus <const> = imgui.SetNextWindowFocus
local imSetNextWindowPos <const> = imgui.SetNextWindowPos
local imSetNextWindowSize <const> = imgui.SetNextWindowSize
local imTableHeadersRow <const> = imgui.TableHeadersRow
local imTableNextColumn <const> = imgui.TableNextColumn
local imTableNextRow <const> = imgui.TableNextRow
local imTableSetupColumn <const> = imgui.TableSetupColumn
local imText <const> = imgui.Text

local imTableFlags <const> = imgui.TableFlags
local imWindowFlags <const> = imgui.WindowFlags

local imCondFirstUseEver <const> = imgui.Cond.FirstUseEver
local imInputTextAllowTabInput <const> = imgui.InputTextFlags.AllowTabInput
local imSelectableDisabled <const> = imgui.SelectableFlags.Disabled
local imTableColumnWidthFixed <const> = imgui.TableColumnFlags.WidthFixed
local imWindowNoSavedSettings <const> = imWindowFlags.NoSavedSettings

local defaulttableflags <const> = imTableFlags.Resizable | imTableFlags.RowBg | imTableFlags.Borders

local tools = {}
local windows = {}

local screenwidth, screenheight
local toolwidgedwidth
local shouldexit
local wintofocus

function expmode.exit()
	shouldexit = true
end

local toolswindowflags = imWindowFlags.AlwaysAutoResize | imWindowFlags.NoCollapse | imWindowFlags.NoResize | imWindowFlags.NoScrollbar

local function updatetoolwindow()
	-- Maximum widget width calculation needs two frames
	-- 1. Widget are rendered with default sizes except buttons that are aligned to the rigth
	-- 2. All widgets are auto-sized again, and separators have appropriate widths
	--    because of window size that was adjusted on the previous frame

	-- TODO:
	-- Avoid visible change of button sizes after first two frames by using
	-- one of techniques described in https://github.com/ocornut/imgui/issues/3714
	-- This requires exposure of currently unusupported GetStyle() or GetWindowDrawList()

	local calcwidth = toolwidgedwidth == 0
	local maxwidth = 0

	imSetNextWindowPos(0, 0, imCondFirstUseEver)
	imBegin("Tools", nil, toolswindowflags)

	for _, tool in ipairs(tools) do
		local title = tool.title

		if tool.onupdate then
			-- Real tool
			if imButton(title, toolwidgedwidth, 0) then
				if windows[title] then
					wintofocus = title
				else
					tool:onopen()
					windows[title] = tool
				end
			end
		elseif title then
			-- Group separator with text
			imSeparatorText(title)
		else
			-- Group separator without text
			imSpacing()
			imSeparator()
			imSpacing()
		end

		if calcwidth then
			local min = imGetItemRectMin()
			local max = imGetItemRectMax()
			maxwidth = math.max(maxwidth, max.x - min.x)
		end
	end

	imEnd()

	if calcwidth then
		toolwidgedwidth = maxwidth
	elseif toolwidgedwidth == -1 then
		toolwidgedwidth = 0
	end
end

local function updatewindows()
	local closedwindows = {}

	for _, window in pairs(windows) do
		if wintofocus == window.title then
			imSetNextWindowFocus()
			wintofocus = nil
		end

		if not window:onupdate() then
			insert(closedwindows, window)
		end
	end

	for _, window in ipairs(closedwindows) do
		windows[window.title] = nil
		window:onclose()
	end
end

function expmode.onupdate()
	updatetoolwindow()

	if not screenwidth then
		local viewport = imGetMainViewport()
		screenwidth = viewport.Size.x
		screenheight = viewport.Size.y
	end

	local keepopen = not shouldexit

	if keepopen then
		updatewindows()
	end

	return keepopen
end

function expmode.onopen()
	screenwidth = nil
	toolwidgedwidth = -1
	shouldexit = false

	for _, window in pairs(windows) do
		window:onopen()
	end
end

function expmode.onclose()
	for _, window in pairs(windows) do
		window:onclose()
	end
end

local messageboxflags <const> = imWindowFlags.AlwaysAutoResize | imWindowFlags.NoCollapse | imWindowFlags.NoResize | imWindowFlags.NoScrollbar | imWindowFlags.NoSavedSettings

local function messagebox_onupdate(self)
	imSetNextWindowPos(screenwidth * 0.5, screenheight * 0.35, imCondFirstUseEver, 0.5, 0.5)

	local visible, opened = imBegin(self.title, true, messageboxflags)

	if visible and opened then
		imText(self.text)
		if imButton('Close') then
			opened = false
		end
	end

	imEnd()

	return opened
end

function expmode.messagebox(title, text)
	local messagebox = windows[title]

	if messagebox then
		messagebox.text = text
		wintofocus = title
	else
		messagebox =
		{
			title = title,
			text = text,
			onupdate = messagebox_onupdate,
			onopen = function () end,
			onclose = function () end
		}
		windows[title] = messagebox
	end
end

local messagebox <const> = expmode.messagebox

function expmode.addtool(title, onupdate, onopen, onclose)
	local tool =
	{
		title = title or 'Tool',
		onupdate = onupdate or function () end,
		onopen = onopen or function () end,
		onclose = onclose or function () end,
	}

	insert(tools, tool)
	return tool
end

function expmode.addseparator(text)
	local separator = { title = text }
	insert(tools, separator)
end

local addtool <const> = expmode.addtool
local addseparator <const> = expmode.addseparator


local vec3mid <const> = vec3.mid
local vec3origin <const> = vec3.new()

local foreach <const> = edicts.foreach
local isfree <const> = edicts.isfree
local getname <const> = edicts.getname
local float <const> = edicts.valuetypes.float

local function moveplayer(edict, location, angles)
	location = location or vec3mid(edict.absmin, edict.absmax)

	if location then
		if edicts.isitem(edict) then
			-- Adjust Z coordinate so player will appear slightly above destination
			location = vec3.copy(location)
			location.z = location.z + 20
		end

		player.safemove(location, angles or edict.angles)
		shouldexit = true
	end
end

local function edictinfo_onupdate(self)
	imSetNextWindowPos(screenwidth * 0.5, screenheight * 0.5, imCondFirstUseEver, 0.5, 0.5)
	imSetNextWindowSize(320, 0, imCondFirstUseEver)

	local title = self.title
	local visible, opened = imBegin(title, true, imWindowNoSavedSettings)

	if visible and opened then
		-- Table of fields names and values
		if imBeginTable(title, 2, defaulttableflags) then
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

			imEndTable()
		end

		-- Tool buttons
		imSeparator()

		local buttoncount = 3
		local buttonspacing = 4
		local buttonwidth = (imGetWindowContentRegionMax().x - buttonspacing) / buttoncount - buttonspacing

		if imButton('Move to', buttonwidth, 0) then
			moveplayer(self.edict)
		end
		imSameLine(0, buttonspacing)

		if imButton('References', buttonwidth, 0) then
			expmode.edictreferences(self.edict)
		end
		imSameLine(0, buttonspacing)

		if imButton('Copy', buttonwidth, 0) then
			local fields = {}

			for i, field in ipairs(self.fields) do
				fields[i] = field.name .. ': ' .. field.value
			end

			imSetClipboardText(table.concat(fields, '\n'))
		end
	end

	imEnd()

	return opened
end

local function edictinfo_onopen(self)
	if isfree(self.edict) then
		windows[self.title] = nil
		return
	end

	local fields = {}

	for i, field in ipairs(self.edict) do
		local value = field.value
		field.value = field.type == float and format('%.1f', value) or tostring(value)
		fields[i] = field
	end

	self.fields = fields
end

local function edictinfo_onclose(self)
	self.fields = nil
end

function expmode.edictinfo(edict)
	if isfree(edict) then
		return
	end

	local title = tostring(edict)
	local window = windows[title]

	if window then
		wintofocus = title
	else
		window =
		{
			title = title,
			edict = edict,
			onupdate = edictinfo_onupdate,
			onopen = edictinfo_onopen,
			onclose = edictinfo_onclose
		}
		edictinfo_onopen(window)
		windows[title] = window
	end
end

local edictinfo <const> = expmode.edictinfo

local function describe(edict)
	local description = getname(edict)
	local location, angles

	if not isfree(edict) then
		location = vec3mid(edict.absmin, edict.absmax)
		angles = edict.angles

		if location == vec3origin then
			location = edict.origin or vec3origin
		end

		if angles and angles == vec3origin then
			angles = nil
		end
	end

	return description, location, angles
end

local function edictstable(title, entries, zerobasedindex)
	if imBeginTable(title, 3, defaulttableflags) then
		imTableSetupColumn('Index', imTableColumnWidthFixed)
		imTableSetupColumn('Description')
		imTableSetupColumn('Location')
		imTableHeadersRow()

		for row = 1, #entries do
			local entry = entries[row]
			local index = tostring(zerobasedindex and row - 1 or row)
			local description = entry.description

			imTableNextRow()
			imTableNextColumn()
			imSelectable(index, false, imSelectableDisabled)
			imTableNextColumn()

			if entry.isfree then
				imSelectable(description, false, imSelectableDisabled)
			else
				local location = entry.location

				local function contextmenu(cellvalue)
					if imBeginPopupContextItem() then
						if imSelectable('Copy cell') then
							imSetClipboardText(tostring(cellvalue))
						end
						if imSelectable('Copy row') then
							imSetClipboardText(format('%s\t%s\t%s', index, description, location))
						end
						imEndPopup()
					end
				end

				-- Description and location need unique IDs to generate click events
				local descriptionid = description .. '##' .. row
				local locationid = location .. '##' .. row

				if imSelectable(descriptionid) then
					edictinfo(entry.edict)
				end
				contextmenu(description)

				imTableNextColumn()

				if imSelectable(locationid) then
					moveplayer(entry.edict, location, entry.angles)
				end
				contextmenu(location)
			end
		end

		imEndTable()
	end
end

local function edicts_onupdate(self)
	imSetNextWindowPos(screenwidth * 0.5, screenheight * 0.5, imCondFirstUseEver, 0.5, 0.5)
	imSetNextWindowSize(480, screenheight * 0.8, imCondFirstUseEver)

	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
		edictstable(title, self.entries, not self.filter)
	end

	imEnd()

	return opened
end

local function edicts_onopen(self)
	local filter = self.filter or describe
	local entries = {}

	foreach(function (edict, current)
		local description, location, angles = filter(edict)

		if not description then
			return current
		end

		insert(entries,
		{
			edict = edict,
			isfree = isfree(edict),
			description = description,
			location = location or '',
			angles = angles
		})

		return current + 1
	end)

	self.entries = entries
end

local function edicts_onclose(self)
	self.entries = nil
end

local function addedictstool(title, filter)
	local tool = addtool(title, edicts_onupdate, edicts_onopen, edicts_onclose)
	tool.filter = filter
end

local function traceentity_onopen(self)
	local edict = player.traceentity()

	if edict then
		edictinfo(edict)
	else
		messagebox('No entity', 'Player is not looking at any entity')
	end
end

local function edictrefs_onupdate(self)
	imSetNextWindowPos(screenwidth * 0.5, screenheight * 0.5, imCondFirstUseEver, 0.5, 0.5)
	imSetNextWindowSize(480, screenheight * 0.8, imCondFirstUseEver)

	local title = self.title
	local visible, opened = imBegin(title, true, imWindowNoSavedSettings)

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

local function edictrefs_onopen(self)
	local edict = self.edict

	if isfree(edict) then
		windows[self.title] = nil
		return
	end

	local function addentry(list, edict)
		insert(list,
		{
			edict = edict,
			description = getname(edict),
			location = vec3mid(edict.absmin, edict.absmax),
			angles = edict.angles
		})
	end

	local target = edict.target
	local targetname = edict.targetname
	local references = {}
	local referencedby = {}

	local function collectrefs(edict)
		if target ~= '' and target == edict.targetname then
			addentry(references, edict)
		end

		if targetname ~= '' and targetname == edict.target then
			addentry(referencedby, edict)
		end

		return 1
	end

	foreach(collectrefs)

	self.references = references
	self.referencedby = referencedby
end

local function edictrefs_onclose(self)
	self.references = nil
	self.referencedby = nil
end

function expmode.edictreferences(edict)
	if isfree(edict) then
		return
	end

	local title = 'References of ' .. tostring(edict)
	local window = windows[title]

	if window then
		wintofocus = title
	else
		window =
		{
			title = title,
			edict = edict,
			onupdate = edictrefs_onupdate,
			onopen = edictrefs_onopen,
			onclose = edictrefs_onclose
		}
		edictrefs_onopen(window)

		if #window.references == 0 and #window.referencedby == 0 then
			messagebox('No references', 'Edict has no references')
		else
			windows[title] = window
		end
	end
end


addseparator('Edicts')
addedictstool('All Edicts')
addedictstool('Monsters', edicts.ismonster)
addedictstool('Teleports', edicts.isteleport)
addedictstool('Doors', edicts.isdoor)
addedictstool('Items', edicts.isitem)
addedictstool('Buttons', edicts.isbutton)
addedictstool('Exits', edicts.isexit)
addedictstool('Messages', edicts.ismessage)
addtool('Trace Entity', nil, traceentity_onopen)

addseparator('Misc')
addtool('Scratchpad', function (self)
	imSetNextWindowPos(screenwidth * 0.5, screenheight * 0.5, imCondFirstUseEver, 0.5, 0.5)
	imSetNextWindowSize(320, 240, imCondFirstUseEver)

	local visible, opened = imBegin(self.title, true)

	if visible and opened then
		_, self.text = imInputTextMultiline('##text', self.text or '', 64 * 1024, -1, -1, imInputTextAllowTabInput)
	end

	imEnd()

	return opened
end)
addtool('Stop All Sounds', function () sound.stopall() end)

addseparator('Debug')
addtool('Dear ImGui Demo', imgui.ShowDemoWindow)

addseparator()
addtool('Press ESC to exit', expmode.exit)
