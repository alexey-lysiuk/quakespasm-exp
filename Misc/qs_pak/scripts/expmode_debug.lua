
expmode.addaction(function ()
	if ImGui.BeginMenu('Debug') then
		if ImGui.MenuItem('ImGui Demo\u{85}') then
			expmode.window('Dear ImGui Demo', function () return ImGui.ShowDemoWindow(true) end)
		end

		ImGui.Separator()

		if ImGui.MenuItem('Trigger Error\u{85}') then
			expmode.safecall(function () error('This error is intentional') end)
		end

		ImGui.EndMenu()
	end
end)
