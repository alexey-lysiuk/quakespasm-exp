
local tools = qimgui.tools
local windows = qimgui.windows

local function updatetools()
	local wintofocus

	for _, tool in ipairs(tools) do
		local title = tool.title

		if imgui.Button(title, -1, 0) then
			if windows[title] then
				wintofocus = title
			else
				tool:onopen()
				windows[title] = tool
			end
		end
	end

	return wintofocus
end

function qimgui.onupdate()
	imgui.SetNextWindowPos(0, 0, imgui.constant.Cond.FirstUseEver)
	imgui.Begin("Tools", nil, imgui.constant.WindowFlags.AlwaysAutoResize | imgui.constant.WindowFlags.NoResize | imgui.constant.WindowFlags.NoScrollbar | imgui.constant.WindowFlags.NoCollapse)

	local wintofocus = updatetools()

	imgui.Spacing()
	imgui.Separator()
	imgui.Spacing()

	local keepopen = not imgui.Button("Press ESC to exit")

	imgui.End()

	if keepopen then
		local closedwindows = {}

		for _, window in pairs(windows) do
			if wintofocus == window.title then
				imgui.SetNextWindowFocus()
			end

			if not window:onupdate() then
				table.insert(closedwindows, window)
			end
		end

		for _, window in ipairs(closedwindows) do
			windows[window.title] = nil
			window.onclose()
		end
	end

	return keepopen
end

function qimgui.onopen()
	for _, window in pairs(windows) do
		window:onopen()
	end
end

function qimgui.onclose()
	for _, window in pairs(windows) do
		window:onclose()
	end
end

function qimgui.basictool(title, onupdate, onopen, onclose)
	local tool =
	{
		title = title or 'Tool',
		onupdate = onupdate or function () return true end,
		onopen = onopen or function () end,
		onclose = onclose or function () end,
	}
	return tool
end

function qimgui.scratchpad()
	local title = 'Scratchpad'

	local onupdate = function (self)
		-- TODO: center window via imgui.SetNextWindowPos(?, ?, 0, 0.5, 0.5)
		imgui.SetNextWindowSize(320, 240, imgui.constant.Cond.FirstUseEver)

		local visible, opened = imgui.Begin(title, true)

		if visible and opened then
			_, self.text = imgui.InputTextMultiline('##text', self.text, 64 * 1024, -1, -1, imgui.constant.InputTextFlags.AllowTabInput)
		end

		imgui.End()

		return opened
	end

	local scratchpad = qimgui.basictool(title, onupdate)
	scratchpad.text = ''
	return scratchpad
end

table.insert(tools, qimgui.scratchpad())

local function imguidemo()
	return qimgui.basictool('Dear ImGui Demo', imgui.ShowDemoWindow)
end

table.insert(tools, imguidemo())