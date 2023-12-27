function imgui.widgets.draw()
	e = player.traceentity()

	if e then
		imgui.Begin("Traced entity")
		imgui.Text(tostring(e))

		for _, f in ipairs(e) do
			imgui.Text(f.name .. ': ' .. tostring(f.value))
		end

		imgui.End()
	end
end
