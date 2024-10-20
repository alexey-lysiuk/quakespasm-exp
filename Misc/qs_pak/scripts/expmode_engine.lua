
local ipairs <const> = ipairs
local format <const> = string.format
local insert <const> = table.insert

local imBegin <const> = ImGui.Begin
local imBeginCombo <const> = ImGui.BeginCombo
local imBeginMenu <const> = ImGui.BeginMenu
local imBeginTable <const> = ImGui.BeginTable
local imColorTextEdit <const> = ImGui.ColorTextEdit
local imEnd <const> = ImGui.End
local imEndCombo <const> = ImGui.EndCombo
local imEndMenu <const> = ImGui.EndMenu
local imEndTable <const> = ImGui.EndTable
local imMenuItem <const> = ImGui.MenuItem
local imSelectable <const> = ImGui.Selectable
local imSeparator <const> = ImGui.Separator
local imSetItemDefaultFocus <const> = ImGui.SetItemDefaultFocus
local imTableHeadersRow <const> = ImGui.TableHeadersRow
local imTableNextColumn <const> = ImGui.TableNextColumn
local imTableNextRow <const> = ImGui.TableNextRow
local imTableSetupColumn <const> = ImGui.TableSetupColumn
local imTableSetupScrollFreeze <const> = ImGui.TableSetupScrollFreeze
local imText <const> = ImGui.Text
local imVec2 <const> = vec2.new

local imTableFlags <const> = ImGui.TableFlags
local imTableColumnWidthFixed <const> = ImGui.TableColumnFlags.WidthFixed

local defaultTableFlags <const> = imTableFlags.Borders | imTableFlags.Resizable | imTableFlags.RowBg | imTableFlags.ScrollY

local resetsearch <const> = expmode.resetsearch
local searchbar <const> = expmode.searchbar
local updatesearch <const> = expmode.updatesearch

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

local function textures_searchcompare(entry, string)
	return entry.name:lower():find(string, 1, true)
		or entry.width:find(string, 1, true)
		or entry.height:find(string, 1, true)
end

local function textures_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
		local searchmodified = searchbar(self)
		local entries = updatesearch(self, textures_searchcompare, searchmodified)

		if imBeginTable(title, 4, defaultTableFlags) then
			imTableSetupScrollFreeze(0, 1)
			imTableSetupColumn('Index', imTableColumnWidthFixed)
			imTableSetupColumn('Name')
			imTableSetupColumn('Width', imTableColumnWidthFixed)
			imTableSetupColumn('Height', imTableColumnWidthFixed)
			imTableHeadersRow()

			for i, entry in ipairs(entries) do
				imTableNextRow()
				imTableNextColumn()
				imText(entry.index)
				imTableNextColumn()
				imText(entry.name)
				imTableNextColumn()
				imText(entry.width)
				imTableNextColumn()
				imText(entry.height)
			end

			imEndTable()
		end
	end

	imEnd()

	return opened
end

local function textures_onshow(self)
	local entries = {}

	for i, tex in ipairs(textures.list()) do
		local entry =
		{
			index = tostring(i),
			name = tex.name,
			width = tostring(tex.width),
			height = tostring(tex.height),
		}
		insert(entries, entry)
	end

	self.entries = entries

	updatesearch(self, textures_searchcompare, true)
	return true
end

local function textures_onhide(self)
	resetsearch(self)
	self.entries = nil
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

function expmode.engine.textures()
	return expmode.window('Textures', textures_onupdate,
		function (self)
			self:setconstraints()
			self:setsize(imVec2(640, 480))
		end,
		textures_onshow, textures_onhide)
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

		if imMenuItem('Textures\u{85}') then
			expmode.engine.textures()
		end

		imSeparator()

		if imMenuItem('Stop All Sounds') then
			sound.stopall()
		end

		imEndMenu()
	end
end)
