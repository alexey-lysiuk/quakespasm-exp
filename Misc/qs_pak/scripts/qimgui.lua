
function qimgui.updatetools()
	for _, tool in ipairs(qimgui.tools) do
		local title = tool.title
		local wasopen = qimgui.windows[title]
		local _, isopen = imgui.Checkbox(tool.title, wasopen)

		if wasopen and not isopen then
			tool:onclose()
			qimgui.windows[title] = nil
		elseif not wasopen and isopen then
			tool:onopen()
			qimgui.windows[title] = tool
		end
	end
end

function qimgui.updatewindows()
	for _, window in pairs(qimgui.windows) do
		window:onupdate()
	end
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
		imgui.Begin(title)
		
		imgui.End()
	end

	return qimgui.basictool(title, onupdate)
end

table.insert(qimgui.tools, qimgui.scratchpad())
