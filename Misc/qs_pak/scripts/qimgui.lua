
local tools = qimgui.tools
local windows = qimgui.windows

function qimgui.updatetools()
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
	foreachwindow('onupdate')
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
		imgui.SetNextWindowSize(320, 240)

		if imgui.Begin(title) then
			_, self.text = imgui.InputTextMultiline('##text', self.text, 1024 * 1024, -1, -1)
		end

		imgui.End()
	end

	local scratchpad = qimgui.basictool(title, onupdate)
	scratchpad.text = ''
	return scratchpad
end

table.insert(qimgui.tools, qimgui.scratchpad())
