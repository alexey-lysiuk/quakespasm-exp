
local insert <const> = table.insert

local tools = qimgui.tools
local windows = qimgui.windows

local toolwidgedwidth
local shouldexit

function qimgui.exit()
	shouldexit = true
end

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
	local wintofocus

	imgui.SetNextWindowPos(0, 0, imgui.constant.Cond.FirstUseEver)
	imgui.Begin("Tools", nil, imgui.constant.WindowFlags.AlwaysAutoResize | imgui.constant.WindowFlags.NoResize 
		| imgui.constant.WindowFlags.NoScrollbar | imgui.constant.WindowFlags.NoCollapse)

	for _, tool in ipairs(tools) do
		local title = tool.title

		if tool.onupdate then
			-- Real tool
			if imgui.Button(title, toolwidgedwidth, 0) then
				if windows[title] then
					wintofocus = title
				else
					tool:onopen()
					windows[title] = tool
				end
			end
		elseif title then
			-- Group separator with text
			imgui.SeparatorText(title)
		else
			-- Group separator without text
			imgui.Spacing()
			imgui.Separator()
			imgui.Spacing()
		end

		if calcwidth then
			local min = imgui.GetItemRectMin()
			local max = imgui.GetItemRectMax()
			maxwidth = math.max(maxwidth, max.x - min.x)
		end
	end

	imgui.End()

	if calcwidth then
		toolwidgedwidth = maxwidth
	elseif toolwidgedwidth == -1 then
		toolwidgedwidth = 0
	end

	return wintofocus
end

local function updatewindows(wintofocus)
	local closedwindows = {}

	for _, window in pairs(windows) do
		if wintofocus == window.title then
			imgui.SetNextWindowFocus()
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

function qimgui.onupdate()
	local wintofocus = updatetoolwindow()
	local keepopen = not shouldexit

	if keepopen then
		updatewindows(wintofocus)
	end

	return keepopen
end

function qimgui.onopen()
	toolwidgedwidth = -1
	shouldexit = false

	for _, window in pairs(windows) do
		window:onopen()
	end
end

function qimgui.onclose()
	for _, window in pairs(windows) do
		window:onclose()
	end
end

function qimgui.addtool(title, onupdate, onopen, onclose)
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

function qimgui.addseparator(text)
	local separator = { title = text }
	insert(tools, separator)
end

local addtool <const> = qimgui.addtool
local addseparator <const> = qimgui.addseparator


local vec3mid <const> = vec3.mid
local vec3origin <const> = vec3.new()

local foreach <const> = edicts.foreach
local isfree <const> = edicts.isfree
local getname <const> = edicts.getname

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

local function edicts_onupdate(self)
	local title = self.title
	local visible, opened = imgui.Begin(title, true)

	if visible and opened then
		local entries = self.entries
		local zerobasedindex = not self.filter

		local tableflags = imgui.constant.TableFlags
		local columnflags = imgui.constant.TableColumnFlags

		if imgui.BeginTable(title, 3, tableflags.Resizable | tableflags.RowBg | tableflags.Borders) then
			imgui.TableSetupColumn('Index', columnflags.WidthFixed)
			imgui.TableSetupColumn('Description', columnflags.WidthStretch)
			imgui.TableSetupColumn('Location', columnflags.WidthStretch)
			imgui.TableHeadersRow()

			for row = 1, #entries do
				local entry = self.entries[row]
				local index = tostring(zerobasedindex and row - 1 or row)
				local location = tostring(entry.location)

				imgui.TableNextRow()
				imgui.TableSetColumnIndex(0)
				imgui.Text(index)
				imgui.TableSetColumnIndex(1)
				imgui.Text(entry.description)
				imgui.TableSetColumnIndex(2)
				imgui.Text(location)
			end

			imgui.EndTable()
		end
	end

	imgui.End()

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
			{ edict = edict, description = description, location = location, angles = angles })

		return current + 1
	end)

	self.entries = entries
end


addseparator('Edicts')
addtool('All Edicts', edicts_onupdate, edicts_onopen, function (self) self.entries = nil end)

addseparator('Misc')
addtool('Scratchpad', function (self)
 	-- TODO: center window via imgui.SetNextWindowPos(?, ?, 0, 0.5, 0.5)
	imgui.SetNextWindowSize(320, 240, imgui.constant.Cond.FirstUseEver)

	local visible, opened = imgui.Begin(self.title, true)

	if visible and opened then
		_, self.text = imgui.InputTextMultiline('##text', self.text or '', 64 * 1024, -1, -1, imgui.constant.InputTextFlags.AllowTabInput)
	end

	imgui.End()

	return opened
end)
addtool('Stop All Sounds', function () sound.stopall() end)

addseparator('Debug')
addtool('Dear ImGui Demo', imgui.ShowDemoWindow)

addseparator()
addtool('Press ESC to exit', qimgui.exit)
