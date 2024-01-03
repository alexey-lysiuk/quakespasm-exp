
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
	local onupdate = function (self)
		imgui.Begin('')
		
		imgui.End()
	end

	return qimgui.basictool('Scratchpad', onupdate)
end

table.insert(qimgui.tools, qimgui.scratchpad())
