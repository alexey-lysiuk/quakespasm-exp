
local ipairs <const> = ipairs
local type <const> = type

local floor <const> = math.floor

local date <const> = os.date

local format <const> = string.format

local insert <const> = table.insert
local remove <const> = table.remove

local imAlignTextToFramePadding <const> = ImGui.AlignTextToFramePadding
local imBegin <const> = ImGui.Begin
local imBeginMainMenuBar <const> = ImGui.BeginMainMenuBar
local imBeginMenu <const> = ImGui.BeginMenu
local imBulletText <const> = ImGui.BulletText
local imButton <const> = ImGui.Button
local imCalcTextSize <const> = ImGui.CalcTextSize
local imColorTextEdit <const> = ImGui.ColorTextEdit
local imEnd <const> = ImGui.End
local imEndMainMenuBar <const> = ImGui.EndMainMenuBar
local imEndMenu <const> = ImGui.EndMenu
local imGetCursorPosX <const> = ImGui.GetCursorPosX
local imGetCursorScreenPos <const> = ImGui.GetCursorScreenPos
local imGetMainViewport <const> = ImGui.GetMainViewport
local imInputText <const> = ImGui.InputText
local imIsItemHovered <const> = ImGui.IsItemHovered
local imIsWindowAppearing <const> = ImGui.IsWindowAppearing
local imMenuItem <const> = ImGui.MenuItem
local imSameLine <const> = ImGui.SameLine
local imSeparator <const> = ImGui.Separator
local imSeparatorText <const> = ImGui.SeparatorText
local imSetClipboardText <const> = ImGui.SetClipboardText
local imSetKeyboardFocusHere <const> = ImGui.SetKeyboardFocusHere
local imSetNextItemWidth <const> = ImGui.SetNextItemWidth
local imSetNextWindowFocus <const> = ImGui.SetNextWindowFocus
local imSetNextWindowPos <const> = ImGui.SetNextWindowPos
local imSetNextWindowSize <const> = ImGui.SetNextWindowSize
local imSetNextWindowSizeConstraints <const> = ImGui.SetNextWindowSizeConstraints
local imSetTooltip <const> = ImGui.SetTooltip
local imSpacing <const> = ImGui.Spacing
local imText <const> = ImGui.Text
local imTextBuffer <const> = ImGui.TextBuffer
local imVec2 <const> = vec2.new

local imWindowFlags <const> = ImGui.WindowFlags
local imCondFirstUseEver <const> = ImGui.Cond.FirstUseEver
local imHoveredFlagsDelayNormal <const> = ImGui.HoveredFlags.DelayNormal

local messageboxflags <const> = imWindowFlags.AlwaysAutoResize | imWindowFlags.NoCollapse
	| imWindowFlags.NoResize | imWindowFlags.NoScrollbar | imWindowFlags.NoSavedSettings

local framecount <const> = host.framecount
local frametime <const> = host.frametime
local realtime <const> = host.realtime

local actions = {}
local oneshots = {}
local windows = {}

local centerpivot <const> = imVec2(0.5, 0.5)
local screensize, shouldexit, wintofocus

function expmode.addoneshot(func)
	insert(oneshots, func)
end

function expmode.addaction(func)
	insert(actions, func)
end

function expmode.exit()
	shouldexit = true
end

local exit <const> = expmode.exit

local function errorwindow_onupdate(self)
	local windowpos = imVec2(screensize.x * 0.5, screensize.y * 0.5)
	imSetNextWindowPos(windowpos, imCondFirstUseEver, centerpivot)

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

local function findwindow(title)
	for _, window in ipairs(windows) do
		if window.title == title then
			return window
		end
	end
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

local function movetocursor(self, flags)
	local position = imGetCursorScreenPos()
	position.x = position.x + screensize.x * 0.01
	position.y = position.y + screensize.y * 0.01
	self.position = position
	self.positionflags = flags or imCondFirstUseEver

	return self
end

local function setconstraints(self, minsize, maxsize)
	if not minsize then
		local charsize = imCalcTextSize('A')
		minsize = imVec2(charsize.x * 36, charsize.y * 12)
	end

	if not maxsize then
		maxsize = imVec2(screensize.x * 0.9, screensize.y * 0.9)
	end

	self.minsize = minsize
	self.maxsize = maxsize

	return self
end

local function setsize(self, size, flags)
	self.size = size
	self.sizeflags = flags or imCondFirstUseEver

	return self
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
			onhide = onhide or function () return true end,
			close = closewindow,
			movetocursor = movetocursor,
			setconstraints = setconstraints,
			setsize = setsize,
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

local openwindow <const> = expmode.window

local function foreachwindow(func_or_name)
	local func = type(func_or_name) == 'function' and func_or_name
	local count = #windows
	local i = 1

	while i <= count do
		local window = windows[i]
		local status, keepopen = safecall(func or window[func_or_name], window)

		if status and keepopen then
			i = i + 1
		else
			window:close()
			count = count - 1
		end
	end
end

local versions

local function about_update(self)
	local visible, opened = imBegin(self.title, true, messageboxflags)

	if visible and opened then
		if not versions then
			versions = { expversion() }
		end

		imText('')
		for _, version in ipairs(versions) do
			imBulletText(version)
		end
		imText('')
	end

	imEnd()

	return opened
end

local scratchpadeditor

local function scratchpad_update(self)
	local visible, opened = imBegin(self.title, true)

	if visible and opened then
		if not scratchpadeditor then
			scratchpadeditor = imColorTextEdit()
		end

		scratchpadeditor:Render('##text')
	end

	imEnd()

	return opened
end

local function stats_update(self)
	local visible, opened = imBegin(self.title, true)

	if visible and opened then
		local prevtime = self.realtime or 0
		local curtime = realtime()

		if prevtime + 0.1 <= curtime then
			local frmtime = frametime()
			local hours = floor(curtime / 3600)
			local minutes = floor(curtime % 3600 / 60)
			local seconds = floor(curtime % 60)

			self.hoststats = format('framecount = %i\nframetime = %f (%.1f FPS)\nrealtime = %f (%02i:%02i:%02i)',
				framecount(), frmtime, 1 / frmtime, curtime, hours, minutes, seconds)
			self.memstats = memstats()
			self.realtime = curtime
		end

		imSeparatorText('Host stats')
		imText(self.hoststats)
		imSeparatorText('Lua memory stats')
		imText(self.memstats)

		if imButton('Collect Garbage') then
			collectgarbage()
		end
	end

	imEnd()

	return opened
end

expmode.common = {}

function expmode.common.scratchpad()
	return openwindow('Scratchpad', scratchpad_update,
		function (self)
			self:setconstraints()
			self:setsize(imVec2(640, 480))
		end)
end

function expmode.common.stats()
	return openwindow('Stats', stats_update,
		function (self) self:setconstraints() end)
end

local scratchpad <const> = expmode.common.scratchpad
local stats <const> = expmode.common.stats

local function updateexpmenu()
	if imBeginMenu('EXP') then
		if imMenuItem('About\u{85}') then
			openwindow('About QuakeSpasm-EXP', about_update)
		end

		imSeparator()

		if imMenuItem('Scratchpad\u{85}') then
			scratchpad()
		end

		if imMenuItem('Stats\u{85}') then
			stats()
		end

		imSeparator()

		if imMenuItem('Exit', 'Esc') then
			exit()
		end

		imEndMenu()
	end
end

local function updatewindowsmenu()
	if imBeginMenu('Windows') then
		local haswindows = #windows > 0
		local closeall = 'Close All'

		if haswindows then
			for _, window in ipairs(windows) do
				if imMenuItem(window.title) then
					wintofocus = window
				end
			end

			imSeparator()

			if imMenuItem(closeall) then
				foreachwindow('onhide')
				windows = {}
			end
		else
			imMenuItem(closeall, nil, false, false)
		end

		imEndMenu()
	end
end

local clockstring
local clockupdatetime = -1

local function updateclockmenu()
	local now = realtime()

	if now - clockupdatetime > 1 then
		clockstring = date('%a %b %d %Y %H:%M')
		clockupdatetime = now
	end

	local spacing = screensize.x - imCalcTextSize(clockstring).x - imGetCursorPosX()
	imSameLine(0, spacing)
	imText(clockstring)
end

local function updateactions()
	if imBeginMainMenuBar() then
		updateexpmenu()

		for _, action in ipairs(actions) do
			safecall(action)
		end

		updatewindowsmenu()
		updateclockmenu()

		imEndMainMenuBar()
	end
end

local function updateoneshots()
	if #oneshots == 0 then
		return
	end

	for _, oneshot in ipairs(oneshots) do
		safecall(oneshot)
	end

	oneshots = {}
end

local function updatewindows()
	foreachwindow(function (window)
		if wintofocus == window then
			imSetNextWindowFocus()
			wintofocus = nil
		end

		local minsize = window.minsize
		local maxsize = window.maxsize

		if minsize and maxsize then
			imSetNextWindowSizeConstraints(minsize, maxsize)
		end

		local position = window.position

		if position then
			imSetNextWindowPos(position, window.positionflags)
			window.position = nil
			window.positionflags = nil
		end

		local size = window.size

		if size then
			imSetNextWindowSize(size, window.sizeflags)
			window.size = nil
			window.sizeflags = nil
		end

		return window:onupdate()
	end)
end

function expmode.onupdate()
	if not screensize then
		screensize = imGetMainViewport().Size
	end

	updateoneshots()
	updateactions()
	updatewindows()

	return not shouldexit
end

function expmode.onopen()
	shouldexit = false

	foreachwindow('onshow')
end

function expmode.onclose()
	foreachwindow('onhide')

	screensize = nil
end

local function messagebox_onupdate(self)
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
	local messagebox = openwindow(title, messagebox_onupdate)
	messagebox.text = text
	return messagebox
end

function expmode.searchbar(window, inputwidth)
	imAlignTextToFramePadding()
	imText('Search:')
	imSameLine()

	if imIsWindowAppearing() then
		imSetKeyboardFocusHere()
	end

	if inputwidth then
		imSetNextItemWidth(inputwidth)
	end

	local modified = imInputText('##search', window.searchbuffer)

	if #window.searchbuffer > 0 then
		if imIsItemHovered(imHoveredFlagsDelayNormal) then
			local searchresults = window.searchresults
			local count = searchresults and #searchresults or -1

			if count > 1 then
				imSetTooltip(count .. ' results')
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

function expmode.updatesearch(window, compfunc, modified)
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

function expmode.resetsearch(window)
	window.searchresults = nil
end
