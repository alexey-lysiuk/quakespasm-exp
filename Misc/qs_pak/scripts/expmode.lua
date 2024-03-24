
local ipairs <const> = ipairs
local tostring <const> = tostring

local format <const> = string.format

local concat <const> = table.concat
local insert <const> = table.insert

local imBegin <const> = imgui.Begin
local imBeginPopup <const> = imgui.BeginPopup
local imBeginPopupContextItem <const> = imgui.BeginPopupContextItem
local imBeginTable <const> = imgui.BeginTable
local imButton <const> = imgui.Button
local imCalcTextSize <const> = imgui.CalcTextSize
local imEnd <const> = imgui.End
local imEndPopup <const> = imgui.EndPopup
local imEndTable <const> = imgui.EndTable
local imGetItemRectMax <const> = imgui.GetItemRectMax
local imGetItemRectMin <const> = imgui.GetItemRectMin
local imGetMainViewport <const> = imgui.GetMainViewport
local imGetWindowContentRegionMax <const> = imgui.GetWindowContentRegionMax
local imInputTextMultiline <const> = imgui.InputTextMultiline
local imIsAnyItemHovered <const> = imgui.IsAnyItemHovered
local imIsItemHovered <const> = imgui.IsItemHovered
local imIsMouseReleased <const> = imgui.IsMouseReleased
local imIsWindowFocused <const> = imgui.IsWindowFocused
local imOpenPopup <const> = imgui.OpenPopup
local imSameLine <const> = imgui.SameLine
local imSelectable <const> = imgui.Selectable
local imSeparator <const> = imgui.Separator
local imSeparatorText <const> = imgui.SeparatorText
local imSetClipboardText <const> = imgui.SetClipboardText
local imSetNextWindowFocus <const> = imgui.SetNextWindowFocus
local imSetNextWindowPos <const> = imgui.SetNextWindowPos
local imSetNextWindowSize <const> = imgui.SetNextWindowSize
local imSetTooltip <const> = imgui.SetTooltip
local imShowDemoWindow <const> = imgui.ShowDemoWindow
local imSpacing <const> = imgui.Spacing
local imTableHeadersRow <const> = imgui.TableHeadersRow
local imTableNextColumn <const> = imgui.TableNextColumn
local imTableNextRow <const> = imgui.TableNextRow
local imTableSetupColumn <const> = imgui.TableSetupColumn
local imText <const> = imgui.Text
local imVec2 <const> = imgui.ImVec2

local imTableFlags <const> = imgui.TableFlags
local imWindowFlags <const> = imgui.WindowFlags

local imCondFirstUseEver <const> = imgui.Cond.FirstUseEver
local imHoveredFlagsDelayNormal <const> = imgui.HoveredFlags.DelayNormal
local imInputTextAllowTabInput <const> = imgui.InputTextFlags.AllowTabInput
local imMouseButtonRight <const> = imgui.MouseButton.Right
local imSelectableDisabled <const> = imgui.SelectableFlags.Disabled
local imTableColumnWidthFixed <const> = imgui.TableColumnFlags.WidthFixed
local imWindowNoSavedSettings <const> = imWindowFlags.NoSavedSettings

local defaulttableflags <const> = imTableFlags.Borders | imTableFlags.Resizable | imTableFlags.RowBg
local defaultscrollytableflags <const> = defaulttableflags | imTableFlags.ScrollY
local messageboxflags <const> = imWindowFlags.AlwaysAutoResize | imWindowFlags.NoCollapse | imWindowFlags.NoResize | imWindowFlags.NoScrollbar | imWindowFlags.NoSavedSettings
local toolswindowflags = imWindowFlags.AlwaysAutoResize | imWindowFlags.NoCollapse | imWindowFlags.NoResize | imWindowFlags.NoScrollbar

local placedwindows = {}
local tools = {}
local windows = {}

local autoexpandsize <const> = imVec2(-1, -1)
local centerpivot <const> = imVec2(0.5, 0.5)
local defaultedictinfowindowsize, defaultedictswindowsize, defaultmessageboxpos, defaulttoolwindowpos, defaultwindowposx, nextwindowpos
local defaultwindowsize <const> = imVec2(320, 240)
local screensize, shouldexit, toolwidgedsize, wintofocus

function expmode.exit()
	shouldexit = true
end

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
			function (self) self.message = result_or_error end,
			errorwindow_onupdate)
	end

	return succeeded, result_or_error
end

local safecall <const> = expmode.safecall

local function updatetoolwindow()
	imSetNextWindowPos(defaulttoolwindowpos, imCondFirstUseEver)
	imBegin("Tools", nil, toolswindowflags)

	for _, tool in ipairs(tools) do
		local title = tool.title

		if tool.onupdate then
			-- Real tool
			if imButton(title, toolwidgedsize) then
				if windows[title] then
					wintofocus = title
				elseif safecall(tool.onopen, tool) then
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
	end

	imEnd()
end

local function updatewindows()
	for _, window in pairs(windows) do
		local title = window.title
	
		if wintofocus == title then
			imSetNextWindowFocus()
			wintofocus = nil
		end

		local status, keepopen = safecall(window.onupdate, window)

		if not status or not keepopen then
			windows[title] = nil
			window:onclose()
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
		defaulttoolwindowpos = imVec2(sx * 0.0025, sy * 0.005)
		defaultwindowposx = charwidth * 25
		toolwidgedsize = imVec2(charwidth * 20, 0)

		if not nextwindowpos then
			nextwindowpos = imVec2(defaultwindowposx, sy * 0.05)
		end
	end

	updatetoolwindow()
	updatewindows()

	return not shouldexit
end

function expmode.onopen()
	shouldexit = false

	for _, window in pairs(windows) do
		safecall(window.onopen, window)
	end
end

function expmode.onclose()
	for _, window in pairs(windows) do
		safecall(window.onclose, window)
	end

	screensize = nil
end

function expmode.window(title, construct, onupdate, onopen, onclose)
	local window = windows[title]

	if window then
		wintofocus = title
	else
		window =
		{
			title = title,
			onupdate = onupdate,
			onopen = onopen or function () end,
			onclose = onclose or function () end
		}

		if construct then
			construct(window)
		end

		if safecall(window.onopen, window) then
			windows[title] = window
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
	local messagebox = window(title, nil, messagebox_onupdate)
	messagebox.text = text
	return messagebox
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

local isany <const> = edicts.isany
local isfree <const> = edicts.isfree
local getname <const> = edicts.getname
local float <const> = edicts.valuetypes.float
local string <const> = edicts.valuetypes.string

local ghost <const> = player.ghost
local setpos <const> = player.setpos

local localize <const> = text.localize
local toascii <const> = text.toascii

local function placewindow(title, size)
	if placedwindows[title] then
		return
	end

	placedwindows[title] = true

	if nextwindowpos.x + size.x >= screensize.x then
		nextwindowpos.x = defaultwindowposx
	end

	if nextwindowpos.y + size.y >= screensize.y then
		nextwindowpos.y = screensize.x * 0.05
	end

	imSetNextWindowPos(nextwindowpos, imCondFirstUseEver)
	imSetNextWindowSize(size)

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
		if imBeginTable(title, 2, defaulttableflags) then
--			local popupname = "EdictInfoContextMenu"

			imTableSetupColumn('Name', imTableColumnWidthFixed)
			imTableSetupColumn('Value')
			imTableHeadersRow()
--			imgui.TableNextRow(imgui.TableRowFlags.Headers)
--			imgui.TableSetColumnIndex(0)
--			imgui.TableHeader('Name')
--			imgui.TableSetColumnIndex(1)
--			imgui.TableHeader('Value')
----			imgui.SameLine(0, imgui.GetColumnWidth() - charwidth * 3)
----			imgui.SameLine()
----			imgui.PushItemWidth(-1)
--			if imgui.SmallButton('...') then
--				imOpenPopup(popupname)
--			end

--			if imBeginPopup(popupname) then
--				if imSelectable('Move to') then
--					moveplayer(self.edict)
--				end
--				if imSelectable('References') then
--					expmode.edictreferences(self.edict)
--				end
--				if imSelectable('Copy all') then
--					local fields = {}
--				
--					for i, field in ipairs(self.fields) do
--						fields[i] = field.name .. ': ' .. field.value
--					end
--				
--					imSetClipboardText(concat(fields, '\n'))
--				end
--				imEndPopup()
--			end

			for _, field in ipairs(self.fields) do
--				local function contextmenu()
--					if imBeginPopupContextItem('EdictInfoContextMenu') then
--						if imSelectable('Move to') then
--							moveplayer(self.edict)
--						end
--						if imSelectable('References') then
--							expmode.edictreferences(self.edict)
--						end
--						if imSelectable('Copy all') then
--							local fields = {}
--						
--							for i, field in ipairs(self.fields) do
--								fields[i] = field.name .. ': ' .. field.value
--							end
--						
--							imSetClipboardText(concat(fields, '\n'))
--						end
--						imEndPopup()
--					end
--				end

				imTableNextRow()
				imTableNextColumn()
--				imgui.PushItemWidth(-1)
				imText(field.name)
--				contextmenu()
				imTableNextColumn()
--				imgui.PushItemWidth(-1)
				imText(field.value)
--				contextmenu()
			end

----			local popupname = tostring(self.edict)
--			local popupname = "EdictInfoContextMenu"
--
----			if imIsWindowFocused(3) and not imIsAnyItemHovered() and imIsMouseReleased(imMouseButtonRight) then
----			if not imIsAnyItemHovered() and imIsMouseReleased(imMouseButtonRight) then
--			if imgui.IsItemClicked(imMouseButtonRight) then
--				imOpenPopup(popupname)
--				print(self.edict)
----				print(popupname)
--			end
--
--			if imBeginPopup(popupname) then
--				if imSelectable('Move to') then
--					moveplayer(self.edict)
--				end
--				if imSelectable('References') then
--					expmode.edictreferences(self.edict)
--				end
--				if imSelectable('Copy all') then
--					local fields = {}
--				
--					for i, field in ipairs(self.fields) do
--						fields[i] = field.name .. ': ' .. field.value
--					end
--				
--					imSetClipboardText(concat(fields, '\n'))
--				end
--				imEndPopup()
--			end

			imEndTable()
		end
	end

	imEnd()

	return opened
end

local function edictinfo_onopen(self)
	local title = self.title

	if tostring(self.edict) ~= title then
		windows[title] = nil
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
end

local function edictinfo_onclose(self)
	self.fields = nil
end

function expmode.edictinfo(edict)
	if isfree(edict) then
		return
	end

	window(tostring(edict), 
		function (self) self.edict = edict end, 
		edictinfo_onupdate, edictinfo_onopen, edictinfo_onclose)
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

local function edicts_onopen(self)
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
end

local function edicts_onclose(self)
	self.entries = nil
end

function expmode.addedictstool(title, filter)
	local tool = addtool(title, edicts_onupdate, edicts_onopen, edicts_onclose)
	tool.filter = filter
end

local addedictstool <const> = expmode.addedictstool

local function traceentity_onopen(self)
	local edict = player.traceentity()

	if edict then
		edictinfo(edict)
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

local function edictrefs_onopen(self)
	local edict = self.edict

	if tostring(self.edict) ~= self.edictid then
		windows[self.title] = nil
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
		windows[self.title] = nil
		return
	end

	local references = {}
	addentries(outgoing, references)

	local referencedby = {}
	addentries(incoming, referencedby)

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

	local edictid = tostring(edict)
	local title = 'References of ' .. edictid
	local window = windows[title]

	if window then
		wintofocus = title
	else
		window =
		{
			title = title,
			edict = edict,
			edictid = edictid,
			onupdate = edictrefs_onupdate,
			onopen = edictrefs_onopen,
			onclose = edictrefs_onclose
		}

		if not safecall(edictrefs_onopen, window) then
			return
		end

		if not window.references then
			messagebox('No references', format("'%s' has no references", edict))
		else
			windows[title] = window
		end
	end
end


addseparator('Edicts')
addedictstool('All Edicts')
addedictstool('Secrets', edicts.issecret)
addedictstool('Monsters', edicts.ismonster)
addedictstool('Teleports', edicts.isteleport)
addedictstool('Doors', edicts.isdoor)
addedictstool('Items', edicts.isitem)
addedictstool('Buttons', edicts.isbutton)
addedictstool('Exits', edicts.isexit)
addedictstool('Messages', edicts.ismessage)
addedictstool('Models', edicts.ismodel)
addtool('Trace Entity', nil, traceentity_onopen)

addseparator('Misc')
addtool('Scratchpad', function (self)
	local title = self.title
	placewindow(title, defaultwindowsize)

	local visible, opened = imBegin(title, true)

	if visible and opened then
		_, self.text = imInputTextMultiline('##text', self.text or '', 64 * 1024, autoexpandsize, imInputTextAllowTabInput)
	end

	imEnd()

	return opened
end)
addtool('Stats', function (self)
	local title = self.title
	placewindow(title, defaultwindowsize)

	local visible, opened = imBegin(title, true)

	if visible and opened then
		local prevtime = self.realtime or 0
		local curtime = host.realtime()

		if prevtime + 0.1 <= curtime then
			self.hoststats = format('framecount = %i\nframetime = %f\nrealtime = %f', 
				host.framecount(), host.frametime(), curtime)
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
end)
addtool('Stop All Sounds', function () sound.stopall() end)

if imShowDemoWindow then
	addseparator('Debug')
	addtool('Dear ImGui Demo', imShowDemoWindow)
	addtool('Trigger Error', function () error('This error is intentional') end)
end

addseparator()
addtool('Press ESC to exit', expmode.exit)
