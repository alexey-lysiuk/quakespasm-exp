
local tools = qimgui.tools
local windows = qimgui.windows

local function updatetools()
	for _, tool in ipairs(tools) do
		local title = tool.title
		local wasopen = windows[title]
		local _, isopen = imgui.Checkbox(tool.title, wasopen)

		if wasopen and not isopen then
			windows[title] = nil
			tool:onclose()
		elseif not wasopen and isopen then
			tool:onopen()
			windows[title] = tool
		end
	end
end

local function foreachwindow(funcname)
	for _, window in pairs(windows) do
		window[funcname](window)
	end
end

function qimgui.onupdate()
	imgui.SetNextWindowPos(0, 0, imgui.constant.Cond.FirstUseEver)
	imgui.Begin("Tools", nil, imgui.constant.WindowFlags.AlwaysAutoResize | imgui.constant.WindowFlags.NoResize | imgui.constant.WindowFlags.NoScrollbar | imgui.constant.WindowFlags.NoCollapse)

	updatetools()

	imgui.Spacing()
	imgui.Separator()
	imgui.Spacing()

	local shouldexit = imgui.Button("Press ESC to exit")

	imgui.End()

	if shouldexit then
		qimgui.close()
	else
		foreachwindow('onupdate')
	end
end

function qimgui.onopen()
	foreachwindow('onopen')
end

function qimgui.onclose()
	foreachwindow('onclose')
end

function qimgui.basictool(title, onupdate, onopen, onclose)
	local tool =
	{
		title = title or 'Tool',
		onupdate = onupdate or function () end,
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
			_, self.text = imgui.InputTextMultiline('##text', self.text, 1024 * 1024, -1, -1, imgui.constant.InputTextFlags.AllowTabInput)
		end

		imgui.End()

		if not opened then
			windows[title] = nil
			self.onclose()
		end
	end

	local scratchpad = qimgui.basictool(title, onupdate)
	scratchpad.text = ''
	return scratchpad
end

table.insert(tools, qimgui.scratchpad())

local imguidemo = qimgui.basictool('Dear ImGui Demo', imgui.ShowDemoWindow)
table.insert(tools, imguidemo)
