
local ipairs <const> = ipairs
local format <const> = string.format
local insert <const> = table.insert

local imBegin <const> = ImGui.Begin
local imBeginCombo <const> = ImGui.BeginCombo
local imBeginMenu <const> = ImGui.BeginMenu
local imColorTextEdit <const> = ImGui.ColorTextEdit
local imEnd <const> = ImGui.End
local imEndCombo <const> = ImGui.EndCombo
local imEndMenu <const> = ImGui.EndMenu
local imMenuItem <const> = ImGui.MenuItem
local imSelectable <const> = ImGui.Selectable
local imSeparator <const> = ImGui.Separator
local imSetItemDefaultFocus <const> = ImGui.SetItemDefaultFocus
local imVec2 <const> = vec2.new

local function levelentities_createtextview(self)
	local entities = host.entities()

	local textview = imColorTextEdit()
	textview:SetLanguageDefinition('entities')
	textview:SetReadOnly(true)
	textview:SetText(entities)

	local lines = {}
	local searchpos = 1

	while true do
		local first, last = entities:find('\r?\n', searchpos)

		if not first then
			break
		end

		insert(lines, entities:sub(searchpos, first - 1))
		searchpos = last + 1
	end

	local names = {}   -- entity names for combobox
	local starts = {}  -- first line of each entity

	for i, line in ipairs(lines) do
		-- Check if this line starts a new entity
		if line:find('%s*{') then
			insert(starts, i)
		end

		-- Check if this lines contains entity class name
		local first, last, classname = line:find('%s*"classname"%s+"([%w_]+)"')

		if first then
			local name = format('[%i] %s', #names + 1, classname)
			insert(names, name)
		end
	end

	-- Add name for cursor position outside of any entity
	names[0] = ''

	-- Add line index after the last entity
	insert(starts, #lines + 1)

	self.textview = textview
	self.names = names
	self.starts = starts

	return textview
end

local function levelentities_update(self)
	local visible, opened = imBegin(self.title, true)

	if visible and opened then
		local textview = self.textview or levelentities_createtextview(self)
		local starts = self.starts

		local currententity = 0
		local currentline = textview:GetCursorPosition()

		-- Find the current entity index
		-- TODO: Use binary search
		for i, start in ipairs(starts) do
			if start > currentline then
				currententity = i - 1
				break
			end
		end

		local names = self.names

		if imBeginCombo('##classnames', names[currententity]) then
			for i, name in ipairs(names) do
				local selected = currententity == i

				if imSelectable(name, selected) then
					textview:SelectRegion(starts[i], 1, starts[i + 1] - 1, math.maxinteger)
				end

				if selected then
					imSetItemDefaultFocus()
				end
			end

			imEndCombo()
		end

		textview:Render('##text')
	end

	imEnd()

	return opened
end

local function levelentities_onhide(self)
	self.textview = nil
	self.names = nil
	self.starts = nil

	return true
end

expmode.engine = {}

function expmode.engine.levelentities()
	return expmode.window('Level Entities', levelentities_update,
		function (self)
			self:setconstraints()
			self:setsize(imVec2(640, 480))
		end,
		nil, levelentities_onhide)
end

local function GhostAndExit(enable)
	player.ghost(enable)
	expmode.exit()
end

expmode.addaction(function ()
	if imBeginMenu('Engine') then
		if imBeginMenu('Ghost Mode') then
			if imMenuItem('Toggle Ghost Mode') then
				GhostAndExit()
			end

			if imMenuItem('Enter Ghost Mode') then
				GhostAndExit(true)
			end

			if imMenuItem('Exit Ghost Mode') then
				GhostAndExit(false)
			end

			imEndMenu()
		end

		if imMenuItem('Move to Start') then
			for _, edict in ipairs(edicts) do
				if edict.classname == 'info_player_start' then
					player.setpos(edict.origin, edict.angles)
					GhostAndExit(false)
					break
				end
			end
		end

		imSeparator()

		if imMenuItem('Level Entities\u{85}') then
			expmode.engine.levelentities()
		end

		imSeparator()

		if imMenuItem('Stop All Sounds') then
			sound.stopall()
		end

		imEndMenu()
	end
end)
