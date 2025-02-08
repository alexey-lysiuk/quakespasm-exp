
local ipairs <const> = ipairs
local format <const> = string.format
local concat <const> = table.concat
local insert <const> = table.insert

local imAlignTextToFramePadding <const> = ImGui.AlignTextToFramePadding
local imBegin <const> = ImGui.Begin
local imBeginCombo <const> = ImGui.BeginCombo
local imBeginMenu <const> = ImGui.BeginMenu
local imBeginPopupContextItem <const> = ImGui.BeginPopupContextItem
local imBeginTable <const> = ImGui.BeginTable
local imColorTextEdit <const> = ImGui.ColorTextEdit
local imEnd <const> = ImGui.End
local imEndCombo <const> = ImGui.EndCombo
local imEndMenu <const> = ImGui.EndMenu
local imEndPopup <const> = ImGui.EndPopup
local imEndTable <const> = ImGui.EndTable
local imImage <const> = ImGui.Image
local imIsKeyPressed <const> = ImGui.IsKeyPressed
local imIsWindowFocused <const> = ImGui.IsWindowFocused
local imMenuItem <const> = ImGui.MenuItem
local imSameLine <const> = ImGui.SameLine
local imSelectable <const> = ImGui.Selectable
local imSeparator <const> = ImGui.Separator
local imSetClipboardText <const> = ImGui.SetClipboardText
local imSetItemDefaultFocus <const> = ImGui.SetItemDefaultFocus
local imSliderFloat <const> = ImGui.SliderFloat
local imTableHeadersRow <const> = ImGui.TableHeadersRow
local imTableNextColumn <const> = ImGui.TableNextColumn
local imTableNextRow <const> = ImGui.TableNextRow
local imTableSetupColumn <const> = ImGui.TableSetupColumn
local imTableSetupScrollFreeze <const> = ImGui.TableSetupScrollFreeze
local imText <const> = ImGui.Text
local imVec2 <const> = vec2.new

local imKey <const> = ImGui.Key
local imSliderFlags <const> = ImGui.SliderFlags
local imTableFlags <const> = ImGui.TableFlags

local imAlwaysClamp <const> = imSliderFlags.AlwaysClamp
local imLogarithmic <const> = imAlwaysClamp | imSliderFlags.Logarithmic
local imLeftArrow <const> = imKey.LeftArrow
local imRightArrow <const> = imKey.RightArrow
local imSpanAllColumns <const> = ImGui.SelectableFlags.SpanAllColumns
local imTableColumnWidthFixed <const> = ImGui.TableColumnFlags.WidthFixed
local imWindowNoSavedSettings <const> = ImGui.WindowFlags.NoSavedSettings

local defaultTableFlags <const> = imTableFlags.Borders | imTableFlags.Resizable | imTableFlags.RowBg | imTableFlags.ScrollY

local BoundingBoxes <const> = render.boundingboxes
local FullBright <const> = render.fullbright
local PolyOffsetFactor <const> = render.polyoffset.factor
local PolyOffsetUnits <const> = render.polyoffset.units

local resetsearch <const> = expmode.resetsearch
local searchbar <const> = expmode.searchbar
local updatesearch <const> = expmode.updatesearch

expmode.engine = {}

local function levelentities_updatetextview(self)
	if not self.textview then
		self.textview = imColorTextEdit()
		self.textview:SetLanguage('entities')
		self.textview:SetReadOnly(true)
	end

	local entities = self.entities

	if not entities then
		return  -- already up-to-date
	end

	self.textview:SetText(entities)
	self.entities = nil

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
		local first, _, classname = line:find('%s*"classname"%s+"([%w_]+)"')

		if first then
			local name = format('[%i] %s', #names + 1, classname)
			insert(names, name)
		end
	end

	-- Add name for cursor position outside of any entity
	names[0] = ''

	-- Add line index after the last entity
	insert(starts, #lines + 1)

	self.names = names
	self.starts = starts
end

local function levelentities_update(self)
	local visible, opened = imBegin(self.title, true)

	if visible and opened then
		levelentities_updatetextview(self)

		local textview = self.textview
		local starts = self.starts

		local currententity = 0
		local currentline = textview:GetCursor()

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
					textview:SelectLines(starts[i], starts[i + 1] - 2)
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

local function levelentities_onshow(self)
	local entities = host.entities()
	local crc = crc16(entities)

	if crc ~= self.crc then
		self.entities = text.toascii(entities)
		self.crc = crc
	end

	return true
end

local function ShowTextureView(self)
	local texture = textures[self.name]

	if not texture then
		return  -- Close view because texture not longer exists
	end

	local width, height = texture.width, texture.height
	local scale = self.scale

	if not scale then
		scale = (width > 640 or height > 480) and 1 or 2
		self.scale = scale
	end

	self.imagesize = imVec2(width * scale, height * scale)
	self.texnum = texture.texnum
	self.width = width
	self.height = height
	self.sizetext = format('Size: %dx%d', width, height)

	return true
end

local function UpdateTextureView(self)
	imAlignTextToFramePadding()
	imText(self.sizetext)
	imSameLine(0, 16)

	local changed, scale = imSliderFloat('Scale', self.scale, 0.25, 4, imAlwaysClamp)

	if changed then
		self.scale = scale
		self.imagesize = imVec2(self.width * scale, self.height * scale)
	end

	imImage(self.texnum, self.imagesize)
end

local function textureview_onupdate(self)
	local visible, opened = imBegin(self.title, true, imWindowNoSavedSettings)

	if visible and opened then
		UpdateTextureView(self)
	end

	imEnd()

	return opened
end

function expmode.engine.viewtexture(name)
	local function oncreate(self)
		self:setconstraints()
		self:movetocursor()

		self.name = name
	end

	return expmode.window('Texture ' .. name, textureview_onupdate, oncreate, ShowTextureView)
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

			for _, entry in ipairs(entries) do
				imTableNextRow()
				imTableNextColumn()
				imText(entry.index)
				imTableNextColumn()
				if imSelectable(entry.name, false, imSpanAllColumns) then
					expmode.engine.viewtexture(entry.name)
				end
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

local function textureviewer_onupdate(self)
	local visible, opened = imBegin(self.title, true)

	if visible and opened then
		local selectedname = self.name
		local selectedindex = self.index
		local selectionchanged

		if imBeginCombo('##textures', selectedname) then
			for i, texture in ipairs(self.textures) do
				local name = texture.name
				local selected = i == selectedindex

				if imSelectable(name, selected) then
					selectedname = name
					selectedindex = i
					selectionchanged = true
				end

				if selected then
					imSetItemDefaultFocus()
				end
			end

			imEndCombo()
		elseif imIsWindowFocused() then
			if imIsKeyPressed(imLeftArrow) then
				selectedindex = selectedindex - 1

				if selectedindex < 1 then
					selectedindex = #self.textures
				end

				selectedname = self.textures[selectedindex].name
				selectionchanged = true
			elseif imIsKeyPressed(imRightArrow) then
				selectedindex = selectedindex + 1

				if selectedindex > #self.textures then
					selectedindex = 1
				end

				selectedname = self.textures[selectedindex].name
				selectionchanged = true
			end
		end

		if selectionchanged then
			self.name = selectedname
			self.index = selectedindex

			ShowTextureView(self)
		end

		UpdateTextureView(self)
	end

	imEnd()

	return opened
end

local textureviewer_defaultname <const> = 'scrap0'
local textureviewer_name = textureviewer_defaultname

local function textureviewer_onshow(self)
	self.textures = textures.list()
	self.name = textureviewer_name

	if not textures[self.name] then
		self.name = textureviewer_defaultname
	end

	for i, texture in ipairs(self.textures) do
		if texture.name == textureviewer_name then
			self.index = i
			break
		end
	end

	return ShowTextureView(self)
end

local function textureviewer_onhide(self)
	textureviewer_name = self.name
	self.textures = nil
	return true
end

function expmode.engine.levelentities()
	return expmode.window('Level Entities', levelentities_update,
		function (self)
			self:setconstraints()
			self:setsize(imVec2(640, 480))
		end,
		levelentities_onshow)
end

function expmode.engine.textures()
	return expmode.window('Textures', textures_onupdate,
		function (self)
			self:setconstraints()
			self:setsize(imVec2(640, 0))
		end,
		textures_onshow, textures_onhide)
end

function expmode.engine.textureviewer()
	return expmode.window('Texture Viewer', textureviewer_onupdate,
		function (self)
			self:setconstraints()
			self:setsize(imVec2(640, 0))
		end,
		textureviewer_onshow, textureviewer_onhide)
end

local function sounds_searchcompare(entry, string)
	return entry.name:lower():find(string, 1, true)
		or entry.size:find(string, 1, true)
end

local function sounds_entry2string(entry)
	return format('%s\t%s\t%ss\t%s\n', entry.indexstr, entry.name, entry.duration, entry.size)
end

local function sounds_onupdate(self)
	local title = self.title
	local visible, opened = imBegin(title, true)

	if visible and opened then
		local searchmodified = searchbar(self)
		local entries = updatesearch(self, sounds_searchcompare, searchmodified)

		if imBeginTable(title, 4, defaultTableFlags) then
			imTableSetupScrollFreeze(0, 1)
			imTableSetupColumn('Index', imTableColumnWidthFixed)
			imTableSetupColumn('Name')
			imTableSetupColumn('Duration', imTableColumnWidthFixed)
			imTableSetupColumn('Size', imTableColumnWidthFixed)
			imTableHeadersRow()

			for _, entry in ipairs(entries) do
				imTableNextRow()
				imTableNextColumn()
				imText(entry.indexstr)
				imTableNextColumn()
				if imSelectable(entry.name, false, imSpanAllColumns) then
					sounds.playlocal(entry.name)
				end
				if imBeginPopupContextItem() then
					if imSelectable('Copy Row') then
						imSetClipboardText(sounds_entry2string(entry))
					end
					if imSelectable('Copy Table') then
						local lines = {}

						for _, e in ipairs(entries) do
							insert(lines, sounds_entry2string(e))
						end

						imSetClipboardText(concat(lines))
					end
					imSeparator()
					if imSelectable('Make Silent') then
						sounds[entry.index]:makesilent()
					end
					imEndPopup()
				end
				imTableNextColumn()
				imText(entry.duration)
				imTableNextColumn()
				imText(entry.size)
			end

			imEndTable()
		end
	end

	imEnd()

	return opened
end

local function sounds_onshow(self)
	local entries = {}

	for i, sound in ipairs(sounds) do
		local framecount = sound.framecount
		local framerate = sound.framerate
		local duration = (framecount and framerate and framerate ~= 0) and (framecount / framerate)
		local soundsize = sound.size
		local entry =
		{
			index = i,
			indexstr = tostring(i),
			name = sound.name,
			duration = duration and format('%.3f', duration) or '-',
			size = soundsize and tostring(soundsize) or '-',
		}
		insert(entries, entry)
	end

	self.entries = entries

	updatesearch(self, sounds_searchcompare, true)
	return true
end

local function sounds_onhide(self)
	resetsearch(self)
	self.entries = nil
	return true
end

function expmode.engine.sounds()
	return expmode.window('Sounds', sounds_onupdate,
		function (self)
			self:setconstraints()
			self:setsize(imVec2(480, 0))
		end,
		sounds_onshow, sounds_onhide)
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

		if imMenuItem('Texture Viewer\u{85}') then
			expmode.engine.textureviewer()
		end

		imSeparator()

		if imBeginMenu('Polygon Offset') then
			if imMenuItem('None') then
				PolyOffsetFactor(0)
			end
			if imMenuItem('Light') then
				PolyOffsetFactor(0.25)
				PolyOffsetUnits(1)
			end
			if imMenuItem('Medium') then
				PolyOffsetFactor(1)
				PolyOffsetUnits(1)
			end
			if imMenuItem('Heavy') then
				PolyOffsetFactor(4)
				PolyOffsetUnits(1)
			end
			if imBeginMenu('Custom') then
				local changed, value

				value = PolyOffsetFactor()
				changed, value = imSliderFloat('Factor', value, -16, 16, imLogarithmic)

				if changed then
					PolyOffsetFactor(value)
				end

				value = PolyOffsetUnits()
				changed, value = imSliderFloat('Units', value, -16, 16, imLogarithmic)

				if changed then
					PolyOffsetUnits(value)
				end

				imEndMenu()
			end

			imEndMenu()
		end

		local bboxes = BoundingBoxes()

		if imMenuItem('Bounding Boxes', nil, bboxes) then
			BoundingBoxes(not bboxes)
		end

		local fullbright = FullBright()

		if imMenuItem('Level Lightning', nil, not fullbright) then
			FullBright(not fullbright)
		end

		imSeparator()

		if imMenuItem('Sounds\u{85}') then
			expmode.engine.sounds()
		end

		if imMenuItem('Stop All Sounds') then
			sounds.stopall()
		end

		imEndMenu()
	end
end)
