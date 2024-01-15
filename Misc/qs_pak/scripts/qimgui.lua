
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

local function updatewindows()
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
		window.onclose()
	end
end

function qimgui.onupdate()
	local wintofocus = updatetoolwindow()
	local keepopen = not shouldexit

	if keepopen then
		updatewindows()
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
