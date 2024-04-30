
local ipairs <const> = ipairs
local tostring <const> = tostring

local floor <const> = math.floor

local format <const> = string.format

local concat <const> = table.concat
local insert <const> = table.insert
local remove <const> = table.remove

local imBegin <const> = ImGui.Begin
local imBeginMainMenuBar <const> = ImGui.BeginMainMenuBar
local imBeginMenu <const> = ImGui.BeginMenu
local imBeginPopup <const> = ImGui.BeginPopup
local imBeginPopupContextItem <const> = ImGui.BeginPopupContextItem
local imBeginTable <const> = ImGui.BeginTable
local imButton <const> = ImGui.Button
local imCalcTextSize <const> = ImGui.CalcTextSize
local imEnd <const> = ImGui.End
local imEndMainMenuBar <const> = ImGui.EndMainMenuBar
local imEndMenu <const> = ImGui.EndMenu
local imEndPopup <const> = ImGui.EndPopup
local imEndTable <const> = ImGui.EndTable
local imGetItemRectMax <const> = ImGui.GetItemRectMax
local imGetItemRectMin <const> = ImGui.GetItemRectMin
local imGetMainViewport <const> = ImGui.GetMainViewport
local imInputTextMultiline <const> = ImGui.InputTextMultiline
local imIsItemHovered <const> = ImGui.IsItemHovered
local imIsMouseReleased <const> = ImGui.IsMouseReleased
local imMenuItem <const> = ImGui.MenuItem
local imOpenPopup <const> = ImGui.OpenPopup
local imSameLine <const> = ImGui.SameLine
local imSelectable <const> = ImGui.Selectable
local imSeparator <const> = ImGui.Separator
local imSeparatorText <const> = ImGui.SeparatorText
local imSetClipboardText <const> = ImGui.SetClipboardText
local imSetNextWindowFocus <const> = ImGui.SetNextWindowFocus
local imSetNextWindowPos <const> = ImGui.SetNextWindowPos
local imSetNextWindowSize <const> = ImGui.SetNextWindowSize
local imSetTooltip <const> = ImGui.SetTooltip
local imShowDemoWindow <const> = ImGui.ShowDemoWindow
local imSpacing <const> = ImGui.Spacing
local imTableGetColumnFlags <const> = ImGui.TableGetColumnFlags
local imTableHeadersRow <const> = ImGui.TableHeadersRow
local imTableNextColumn <const> = ImGui.TableNextColumn
local imTableNextRow <const> = ImGui.TableNextRow
local imTableSetupColumn <const> = ImGui.TableSetupColumn
local imText <const> = ImGui.Text
local imVec2 <const> = ImGui.ImVec2

local imTableColumnFlags <const> = ImGui.TableColumnFlags
local imTableFlags <const> = ImGui.TableFlags
local imWindowFlags <const> = ImGui.WindowFlags

local imCondFirstUseEver <const> = ImGui.Cond.FirstUseEver
local imHoveredFlagsDelayNormal <const> = ImGui.HoveredFlags.DelayNormal
local imInputTextAllowTabInput <const> = ImGui.InputTextFlags.AllowTabInput
local imMouseButtonRight <const> = ImGui.MouseButton.Right
local imNoOpenOverExistingPopup <const> = ImGui.PopupFlags.NoOpenOverExistingPopup
local imSelectableDisabled <const> = ImGui.SelectableFlags.Disabled
local imTableColumnIsHovered <const> = imTableColumnFlags.IsHovered
local imTableColumnWidthFixed <const> = imTableColumnFlags.WidthFixed
local imWindowNoSavedSettings <const> = imWindowFlags.NoSavedSettings

local defaulttableflags <const> = imTableFlags.Borders | imTableFlags.Resizable | imTableFlags.RowBg
local defaultscrollytableflags <const> = defaulttableflags | imTableFlags.ScrollY
local messageboxflags <const> = imWindowFlags.AlwaysAutoResize | imWindowFlags.NoCollapse | imWindowFlags.NoResize | imWindowFlags.NoScrollbar | imWindowFlags.NoSavedSettings

local placedwindows = {}
local actions = {}
local windows = {}

local autoexpandsize <const> = imVec2(-1, -1)
local centerpivot <const> = imVec2(0.5, 0.5)
local defaultedictinfowindowsize, defaultedictswindowsize, defaultmessageboxpos, nextwindowpos
local defaultwindowsize <const> = imVec2(320, 240)
local screensize, shouldexit, wintofocus

local function findwindow(title)
	for _, window in ipairs(windows) do
		if window.title == title then
			return window
		end
	end
end

function expmode.exit()
	shouldexit = true
end

local exit <const> = expmode.exit

local function errorwindow_onupdate(self)
	imSetNextWindowPos(defaultmessageboxpos, imCondFirstUseEver, centerpivot)

	local visible, opened = imBegin(self.title, true, messageboxflags)

	if visible and opened then
		local message = self.message

		imText('Error occurred when running a tool')
		imSpacing()
		imSeparator()
		imText(message)
		imSeparator()
		imSpacing()

		if imButton('Copy') then
			imSetClipboardText(message)
		end
		imSameLine()
		if imButton('Close') then
			opened = false
		end
	end

	imEnd()

	return opened
end

function expmode.safecall(func, ...)
	local succeeded, result_or_error = xpcall(func, stacktrace, ...)

	if not succeeded then
		print(result_or_error)

		expmode.window('Tool Error',
			errorwindow_onupdate,
			function (self) self.message = result_or_error end)
	end

	return succeeded, result_or_error
end

local safecall <const> = expmode.safecall

local placewindow, scratchpad_update, stats_update

local function scratchpad()
	local title = 'Scratchpad'

	if imMenuItem(title) then
		expmode.window(title, scratchpad_update)
	end
end

local function stats()
	local title = 'Stats'

	if imMenuItem(title) then
		expmode.window(title, stats_update)
	end
end

local function updateactions()
	if imBeginMainMenuBar() then
		if imBeginMenu('EXP') then
			scratchpad()
			stats()

			if imMenuItem('Stop All Sounds') then
				sound.stopall()
			end

			imSeparator()

			if imMenuItem('Exit', 'Esc') then
				exit()
			end

			imEndMenu()
		end

		for _, action in ipairs(actions) do
			safecall(action)
		end

		if #windows > 0 then
			if imBeginMenu('Windows') then
				for _, window in ipairs(windows) do
					if imMenuItem(window.title) then
						wintofocus = window
					end
				end

				imSeparator()

				if imMenuItem('Close all') then
					for _, window in ipairs(windows) do
						safecall(window.onhide, window)
					end

					windows = {}
				end

				imEndMenu()
			end
		end

		imEndMainMenuBar()
	end
end

local function updatewindows()
	for _, window in pairs(windows) do
		if wintofocus == window then
			imSetNextWindowFocus()
			wintofocus = nil
		end

		local status, keepopen = safecall(window.onupdate, window)

		if not status or not keepopen then
			window:close()
		end
	end
end

function expmode.onupdate()
	if not screensize then
		screensize = imGetMainViewport().Size

		local sx = screensize.x
		local sy = screensize.y
		local charwidth = imCalcTextSize('a').x

		defaultedictinfowindowsize = imVec2(charwidth * 48, sy * 0.5)
		defaultedictswindowsize = imVec2(charwidth * 64, sy * 0.5)
		defaultmessageboxpos = imVec2(sx * 0.5, sy * 0.35)

		if not nextwindowpos then
			nextwindowpos = imVec2(sx * 0.05, sy * 0.05)
		end
	end

	updateactions()
	updatewindows()

	return not shouldexit
end

function expmode.onopen()
	shouldexit = false

	for _, window in pairs(windows) do
		safecall(window.onshow, window)
	end
end

function expmode.onclose()
	for _, window in pairs(windows) do
		safecall(window.onhide, window)
	end

	screensize = nil
end

local function closewindow(window)
	safecall(window.onhide, window)

	for i, probe in ipairs(windows) do
		if window == probe then
			remove(windows, i)
			break
		end
	end
end

function expmode.window(title, onupdate, oncreate, onshow, onhide)
	local window = findwindow(title)

	if window then
		wintofocus = window
	else
		window =
		{
			title = title,
			onupdate = onupdate,
			onshow = onshow or function () return true end,
			onhide = onhide or function () end,
			close = closewindow
		}

		if oncreate then
			oncreate(window)
		end

		local status, isopened = safecall(window.onshow, window)

		if status then
			if isopened then
				insert(windows, window)
			else
				window:close()
				return
			end
		else
			return
		end
	end

	return window
end

local window <const> = expmode.window

local function messagebox_onupdate(self)
	imSetNextWindowPos(defaultmessageboxpos, imCondFirstUseEver, centerpivot)

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
	local messagebox = window(title, messagebox_onupdate)
	messagebox.text = text
	return messagebox
end

local messagebox <const> = expmode.messagebox

function expmode.addaction(func)
	insert(actions, func)
end

local addaction <const> = expmode.addaction


local vec3mid <const> = vec3.mid
local vec3origin <const> = vec3.new()

local isany <const> = edicts.isany
local isfree <const> = edicts.isfree
local getname <const> = edicts.getname
local float <const> = edicts.valuetypes.float
local string <const> = edicts.valuetypes.string

local ghost <const> = player.ghost
local setpos <const> = player.setpos

local localize <const> = text.localize
local toascii <const> = text.toascii

placewindow = function (title, size)
	if placedwindows[title] then
		return
	end

	placedwindows[title] = true

	if nextwindowpos.x + size.x >= screensize.x then
		nextwindowpos.x = screensize.x * 0.05
	end

	if nextwindowpos.y + size.y >= screensize.y then
		nextwindowpos.y = screensize.y * 0.05
	end

	imSetNextWindowPos(nextwindowpos, imCondFirstUseEver)
	imSetNextWindowSize(size, imCondFirstUseEver)

	nextwindowpos.x = nextwindowpos.x + screensize.x * 0.05
	nextwindowpos.y = nextwindowpos.y + screensize.y * 0.05
end

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

		shouldexit = true
	end
end

local function edictinfo_onupdate(self)
	local title = self.title
	placewindow(title, defaultedictinfowindowsize)

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

local function edictstable_tostring(entries, zerobasedindex)
	local lines = {}

	for row = 1, #entries do
		local entry = entries[row]
		local line = format('%d\t%s\t%s', zerobasedindex and row - 1 or row, entry.description, entry.location)
		insert(lines, line)
	end

	return concat(lines, '\n')
end

local function edictstable(title, entries, zerobasedindex, tableflags)
	if imBeginTable(title, 3, tableflags or defaulttableflags) then
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
						if imSelectable('References') then
							expmode.edictreferences(entry.edict)
						end
						imSeparator()
						if imSelectable('Copy cell') then
							imSetClipboardText(tostring(cellvalue))
						end
						if imSelectable('Copy row') then
							imSetClipboardText(format('%s\t%s\t%s', index, description, location))
						end
						if imSelectable('Copy table') then
							imSetClipboardText(edictstable_tostring(entries, zerobasedindex))
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
				if imIsItemHovered(imHoveredFlagsDelayNormal) then
					imSetTooltip(tostring(entry.edict))
				end
				contextmenu(description)

				imTableNextColumn()

				if imSelectable(locationid) then
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

local function edicts_onupdate(self)
	local title = self.title
	placewindow(title, defaultedictswindowsize)

	local visible, opened = imBegin(title, true)

	if visible and opened then
		edictstable(title, self.entries, not self.filter, defaultscrollytableflags)
	end

	imEnd()

	return opened
end

local function edicts_onshow(self)
	local filter = self.filter or isany
	local entries = {}

	for _, edict in ipairs(edicts) do
		local description, location, angles = filter(edict)

		if description then
			insert(entries,
			{
				edict = edict,
				isfree = isfree(edict),
				description = toascii(description),
				location = location or '',
				angles = angles
			})
		end
	end

	self.entries = entries

	return true
end

local function edicts_onhide(self)
	self.entries = nil
end

local function traceentity_onshow(self)
	local edict = player.traceentity()

	if edict then
		edictinfo(edict)
		return true
	else
		messagebox('No entity', 'Player is not looking at any entity')
	end
end

local function edictrefs_onupdate(self)
	local title = self.title
	placewindow(title, defaultedictswindowsize)

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

local function edictrefs_onshow(self)
	local edict = self.edict

	if tostring(self.edict) ~= self.edictid then
		return
	end

	local function addentries(source, list)
		for _, edict in ipairs(source) do
			local description, location, angles = isany(edict)
			insert(list,
			{
				edict = edict,
				description = description,
				location = location,
				angles = angles
			})
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
				window(title, edicts_onupdate,
					function (self) self.filter = tool[2] end,
					edicts_onshow, edicts_onhide)
			end
		end

		imSeparator()

		if imMenuItem('Trace Entity') then
			traceentity_onshow()
		end

		imEndMenu()
	end
end)

scratchpad_update = function (self)
	local title = self.title
	placewindow(title, defaultwindowsize)

	local visible, opened = imBegin(title, true)

	if visible and opened then
		_, self.text = imInputTextMultiline('##text', self.text or '', 64 * 1024, autoexpandsize, imInputTextAllowTabInput)
	end

	imEnd()

	return opened
end

stats_update = function (self)
	local title = self.title
	placewindow(title, defaultwindowsize)

	local visible, opened = imBegin(title, true)

	if visible and opened then
		local prevtime = self.realtime or 0
		local curtime = host.realtime()

		if prevtime + 0.1 <= curtime then
			local frametime = host.frametime()
			local hours = floor(curtime / 3600)
			local minutes = floor(curtime % 3600 / 60)
			local seconds = floor(curtime % 60)

			self.hoststats = format('framecount = %i\nframetime = %f (%.1f FPS)\nrealtime = %f (%02i:%02i:%02i)', 
				host.framecount(), frametime, 1 / frametime, curtime, hours, minutes, seconds)
			self.memstats = memstats()
			self.realtime = curtime
		end

		imSeparatorText('Host stats')
		imText(self.hoststats)
		imSeparatorText('Lua memory stats')
		imText(self.memstats)
	end

	imEnd()

	return opened
end

if imShowDemoWindow then
	addaction(function ()
		if imBeginMenu('Debug') then
			if imMenuItem('ImGui Demo') then
				window('Dear ImGui Demo', function () return imShowDemoWindow(true) end)
			end

			imSeparator()

			if imMenuItem('Trigger Error') then
				safecall(function () error('This error is intentional') end)
			end
	
			imEndMenu()
		end
	end)
end
