
local ipairs <const> = ipairs
local type <const> = type

local floor <const> = math.floor

local date <const> = os.date

local format <const> = string.format

local insert <const> = table.insert
local remove <const> = table.remove

local imBegin <const> = ImGui.Begin
local imBeginMainMenuBar <const> = ImGui.BeginMainMenuBar
local imBeginMenu <const> = ImGui.BeginMenu
local imButton <const> = ImGui.Button
local imCalcTextSize <const> = ImGui.CalcTextSize
local imEnd <const> = ImGui.End
local imEndMainMenuBar <const> = ImGui.EndMainMenuBar
local imEndMenu <const> = ImGui.EndMenu
local imGetCursorPosX <const> = ImGui.GetCursorPosX
local imGetMainViewport <const> = ImGui.GetMainViewport
local imInputTextMultiline <const> = ImGui.InputTextMultiline
local imMenuItem <const> = ImGui.MenuItem
local imSameLine <const> = ImGui.SameLine
local imSeparator <const> = ImGui.Separator
local imSeparatorText <const> = ImGui.SeparatorText
local imSetClipboardText <const> = ImGui.SetClipboardText
local imSetNextWindowFocus <const> = ImGui.SetNextWindowFocus
local imSetNextWindowPos <const> = ImGui.SetNextWindowPos
local imSetNextWindowSize <const> = ImGui.SetNextWindowSize
local imSpacing <const> = ImGui.Spacing
local imText <const> = ImGui.Text
local imVec2 <const> = ImGui.ImVec2

local imWindowFlags <const> = ImGui.WindowFlags
local imCondFirstUseEver <const> = ImGui.Cond.FirstUseEver
local imInputTextAllowTabInput <const> = ImGui.InputTextFlags.AllowTabInput

local messageboxflags <const> = imWindowFlags.AlwaysAutoResize | imWindowFlags.NoCollapse | imWindowFlags.NoResize | imWindowFlags.NoScrollbar | imWindowFlags.NoSavedSettings

local framecount <const> = host.framecount
local frametime <const> = host.frametime
local realtime <const> = host.realtime

local placedwindows = {}
local actions = {}
local windows = {}

local autoexpandsize <const> = imVec2(-1, -1)
local centerpivot <const> = imVec2(0.5, 0.5)
local defaultmessageboxpos, nextwindowpos
local defaultwindowsize <const> = imVec2(320, 240)
local screensize, shouldexit, wintofocus

function expmode.addaction(func)
	insert(actions, func)
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

function expmode.placewindow(title, size)
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

local placewindow <const> = expmode.placewindow

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

local scratchpadtext = ''

local function scratchpad_update(self)
	local title = self.title
	placewindow(title, defaultwindowsize)

	local visible, opened = imBegin(title, true)

	if visible and opened then
		_, scratchpadtext = imInputTextMultiline('##text', scratchpadtext, 64 * 1024, autoexpandsize, imInputTextAllowTabInput)
	end

	imEnd()

	return opened
end

local function stats_update(self)
	local title = self.title
	placewindow(title, defaultwindowsize)

	local visible, opened = imBegin(title, true)

	if visible and opened then
		local prevtime = self.realtime or 0
		local curtime = realtime()

		if prevtime + 0.1 <= curtime then
			local frametime = frametime()
			local hours = floor(curtime / 3600)
			local minutes = floor(curtime % 3600 / 60)
			local seconds = floor(curtime % 60)

			self.hoststats = format('framecount = %i\nframetime = %f (%.1f FPS)\nrealtime = %f (%02i:%02i:%02i)',
				framecount(), frametime, 1 / frametime, curtime, hours, minutes, seconds)
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

local function updateexpmenu()
	if imBeginMenu('EXP') then
		local title = 'Scratchpad'

		if imMenuItem(title) then
			window(title, scratchpad_update)
		end

		title = 'Stats'

		if imMenuItem(title) then
			window(title, stats_update)
		end

		if imMenuItem('Stop All Sounds') then
			sound.stopall()
		end

		imSeparator()

		if imMenuItem('Exit', 'Esc') then
			exit()
		end

		imEndMenu()
	end
end

local function updatewindowsmenu()
	local closeall = 'Close all'

	if imBeginMenu('Windows') then
		local haswindows = #windows > 0

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
local clockupdatetime = 0

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

local function updatewindows()
	foreachwindow(function (window)
		if wintofocus == window then
			imSetNextWindowFocus()
			wintofocus = nil
		end

		return window:onupdate()
	end)
end

function expmode.onupdate()
	if not screensize then
		screensize = imGetMainViewport().Size

		local sx = screensize.x
		local sy = screensize.y

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

	foreachwindow('onshow')
end

function expmode.onclose()
	foreachwindow('onhide')

	screensize = nil
end

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
