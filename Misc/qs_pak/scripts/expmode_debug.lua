
expmode.addaction(function ()
	if ImGui.BeginMenu('Debug') then
		if ImGui.MenuItem('ImGui Demo') then
			expmode.window('Dear ImGui Demo', function () return ImGui.ShowDemoWindow(true) end)
		end

		ImGui.Separator()

		if ImGui.MenuItem('Trigger Error') then
			expmode.safecall(function () error('This error is intentional') end)
		end

		ImGui.EndMenu()
	end
end)
