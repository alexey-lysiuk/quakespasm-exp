function qimgui.windows.draw()
	e = player.traceentity()

	if e then
		imgui.Begin(tostring(e) .. "###Traced entity")

		for _, f in ipairs(e) do
			imgui.Text(f.name .. ': ' .. tostring(f.value))
		end

		imgui.End()
	end
end
