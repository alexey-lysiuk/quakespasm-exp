
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

function qimgui.onupdate()
	imgui.SetNextWindowPos(0, 0, imgui.constant.Cond.FirstUseEver)
	imgui.Begin("Tools", nil, imgui.constant.WindowFlags.AlwaysAutoResize | imgui.constant.WindowFlags.NoResize | imgui.constant.WindowFlags.NoScrollbar | imgui.constant.WindowFlags.NoCollapse)

	updatetools()

	imgui.Spacing()
	imgui.Separator()
	imgui.Spacing()

	local keepopen = not imgui.Button("Press ESC to exit")

	imgui.End()

	if keepopen then
		local closedwindows = {}

		for _, window in pairs(windows) do
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

local function imguidemo()
	return qimgui.basictool('Dear ImGui Demo', imgui.ShowDemoWindow)
end

table.insert(tools, imguidemo())
