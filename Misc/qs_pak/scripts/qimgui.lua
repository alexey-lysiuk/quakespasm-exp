
local format <const> = string.format
local insert <const> = table.insert

local tools = qimgui.tools
local windows = qimgui.windows

local playlocal <const> = sound.playlocal

local screenwidth, screenheight
local toolwidgedwidth
local shouldexit
local wintofocus

function qimgui.exit()
	shouldexit = true
end

local function updatetoolwindow()
	-- Maximum widget width calculation needs two frames
	-- 1. Widget are rendered with default sizes except buttons that are aligned to the rigth
	-- 2. All widgets are auto-sized again, and separators have appropriate widths
	--    because of window size that was adjusted on the previous frame

	-- TODO:
	-- Avoid visible change of button sizes after first two frames by using
	-- one of techniques described in https://github.com/ocornut/imgui/issues/3714
	-- This requires exposure of currently unusupported GetStyle() or GetWindowDrawList()

	local calcwidth = toolwidgedwidth == 0
	local maxwidth = 0

	imgui.SetNextWindowPos(0, 0, imgui.constant.Cond.FirstUseEver)
	imgui.Begin("Tools", nil, imgui.constant.WindowFlags.AlwaysAutoResize | imgui.constant.WindowFlags.NoResize 
		| imgui.constant.WindowFlags.NoScrollbar | imgui.constant.WindowFlags.NoCollapse)

	for _, tool in ipairs(tools) do
		local title = tool.title

		if tool.onupdate then
			-- Real tool
			if imgui.Button(title, toolwidgedwidth, 0) then
				if windows[title] then
					wintofocus = title
				else
					tool:onopen()
					windows[title] = tool
				end
			end
		elseif title then
			-- Group separator with text
			imgui.SeparatorText(title)
		else
			-- Group separator without text
			imgui.Spacing()
			imgui.Separator()
			imgui.Spacing()
		end

		if calcwidth then
			local min = imgui.GetItemRectMin()
			local max = imgui.GetItemRectMax()
			maxwidth = math.max(maxwidth, max.x - min.x)
		end
	end

	imgui.End()

	if calcwidth then
		toolwidgedwidth = maxwidth
	elseif toolwidgedwidth == -1 then
		toolwidgedwidth = 0
	end
end

local function updatewindows()
	local closedwindows = {}

	for _, window in pairs(windows) do
		if wintofocus == window.title then
			imgui.SetNextWindowFocus()
			wintofocus = nil
		end

		if not window:onupdate() then
			insert(closedwindows, window)
		end
	end

	for _, window in ipairs(closedwindows) do
		windows[window.title] = nil
		window:onclose()
	end
end

function qimgui.onupdate()
	updatetoolwindow()

	if not screenwidth then
		local viewport = imgui.GetMainViewport()
		screenwidth = viewport.Size.x
		screenheight = viewport.Size.y
	end

	local keepopen = not shouldexit

	if keepopen then
		updatewindows()
	end

	return keepopen
end

function qimgui.onopen()
	screenwidth = nil
	toolwidgedwidth = -1
	shouldexit = false

	for _, window in pairs(windows) do
		window:onopen()
	end
end

function qimgui.onclose()
	for _, window in pairs(windows) do
		window:onclose()
	end
end

function qimgui.addtool(title, onupdate, onopen, onclose)
	local tool =
	{
		title = title or 'Tool',
		onupdate = onupdate or function () end,
		onopen = onopen or function () end,
		onclose = onclose or function () end,
	}

	insert(tools, tool)
	return tool
end

function qimgui.addseparator(text)
	local separator = { title = text }
	insert(tools, separator)
end

local addtool <const> = qimgui.addtool
local addseparator <const> = qimgui.addseparator


local vec3mid <const> = vec3.mid
local vec3origin <const> = vec3.new()

local foreach <const> = edicts.foreach
local isfree <const> = edicts.isfree
local getname <const> = edicts.getname
local float <const> = edicts.valuetypes.float

local function moveplayerto(edict, location, angles)
	location = location or vec3mid(edict.absmin, edict.absmax)

	if location then
		if edicts.isitem(edict) then
			-- Adjust Z coordinate so player will appear slightly above destination
			location = vec3.copy(location)
			location.z = location.z + 20
		end

		player.safemove(location, angles or edict.angles)
		shouldexit = true
	end
end

local function edictinfo_onupdate(self)
	imgui.SetNextWindowPos(screenwidth * 0.5, screenheight * 0.5, imgui.constant.Cond.FirstUseEver, 0.5, 0.5)
	imgui.SetNextWindowSize(320, 0, imgui.constant.Cond.FirstUseEver)

	local title = self.title
	local visible, opened = imgui.Begin(title, true, imgui.constant.WindowFlags.NoSavedSettings)

	if visible and opened then
		local tableflags = imgui.constant.TableFlags
		local columnflags = imgui.constant.TableColumnFlags

		if imgui.BeginTable(title, 2, tableflags.Resizable | tableflags.RowBg | tableflags.Borders) then
			imgui.TableSetupColumn('Name', columnflags.WidthFixed)
			imgui.TableSetupColumn('Value')
			imgui.TableHeadersRow()

			for _, field in ipairs(self.fields) do
				imgui.TableNextRow()
				imgui.TableSetColumnIndex(0)
				imgui.Text(field.name)
				imgui.TableSetColumnIndex(1)
				imgui.Text(field.value)
			end

			imgui.EndTable()
		end
	end

--	imgui.Spacing()
	imgui.Separator()
--	imgui.Spacing()

	local buttoncount = 3
	local buttonspacing = 4
	local buttonwidth = (imgui.GetWindowContentRegionMax().x - buttonspacing) / buttoncount - buttonspacing

	if imgui.Button('Move to', buttonwidth, 0) then
		moveplayerto(self.edict)
	end
	imgui.SameLine(0, buttonspacing)

	if imgui.Button('References', buttonwidth, 0) then
		print('todo References')
	end
	imgui.SameLine(0, buttonspacing)

	if imgui.Button('Copy', buttonwidth, 0) then
		local fields = {}

		for i, field in ipairs(self.fields) do
			fields[i] = field.name .. ': ' .. field.value
		end

		imgui.SetClipboardText(table.concat(fields, '\n'))
	end

	imgui.End()

	return opened
end

local function edictinfo_onopen(self)
	if isfree(self.edict) then
		windows[self.title] = nil
		return
	end

	local fields = {}

	for i, field in ipairs(self.edict) do
		local value = field.value
		field.value = field.type == float and format('%.1f', value) or tostring(value)
		fields[i] = field
	end

	self.fields = fields
end

local function edictinfo_onclose(self)
	self.fields = nil
end

function qimgui.edictinfo(edict)
	if isfree(edict) then
		return
	end

	local title = tostring(edict)
	local window = windows[title]

	if window then
		wintofocus = title
	else
		window =
		{
			title = title,
			edict = edict,
			onupdate = edictinfo_onupdate,
			onopen = edictinfo_onopen,
			onclose = edictinfo_onclose
		}
		edictinfo_onopen(window)
		windows[title] = window
	end
end

local function describe(edict)
	local description = getname(edict)
	local location, angles

	if not isfree(edict) then
		location = vec3mid(edict.absmin, edict.absmax)
		angles = edict.angles

		if location == vec3origin then
			location = edict.origin or vec3origin
		end

		if angles and angles == vec3origin then
			angles = nil
		end
	end

	return description, location, angles
end

local function edictstable(title, entries, zerobasedindex)
	local tableflags = imgui.constant.TableFlags

	if imgui.BeginTable(title, 3, tableflags.Resizable | tableflags.RowBg | tableflags.Borders) then
		imgui.TableSetupColumn('Index', imgui.constant.TableColumnFlags.WidthFixed)
		imgui.TableSetupColumn('Description')
		imgui.TableSetupColumn('Location')
		imgui.TableHeadersRow()

		for row = 1, #entries do
			local entry = entries[row]
			local index = tostring(zerobasedindex and row - 1 or row)
			local description = entry.description

			imgui.TableNextRow()
			imgui.TableSetColumnIndex(0)
			imgui.Text(index)
			imgui.TableSetColumnIndex(1)

			if entry.isfree then
				imgui.Text(description)
			else
				-- Description and location need unique IDs to generate click events
				local location = entry.location .. '##' .. row
				description = description .. '##' .. row

				if imgui.Selectable(description) then
					qimgui.edictinfo(entry.edict)
				end

				imgui.TableSetColumnIndex(2)

				if imgui.Selectable(location) then
					moveplayerto(entry.edict, entry.location, entry.angles)
				end
			end
		end

		imgui.EndTable()
	end
end

local function edicts_onupdate(self)
	imgui.SetNextWindowPos(screenwidth * 0.5, screenheight * 0.5, imgui.constant.Cond.FirstUseEver, 0.5, 0.5)
	imgui.SetNextWindowSize(480, screenheight * 0.8, imgui.constant.Cond.FirstUseEver)

	local title = self.title
	local visible, opened = imgui.Begin(title, true)

	if visible and opened then
		edictstable(title, self.entries, not self.filter)
	end

	imgui.End()

	return opened
end

local function edicts_onopen(self)
	local filter = self.filter or describe
	local entries = {}

	foreach(function (edict, current)
		local description, location, angles = filter(edict)

		if not description then
			return current
		end

		insert(entries,
		{
			edict = edict,
			isfree = isfree(edict),
			description = description,
			location = location or '',
			angles = angles
		})

		return current + 1
	end)

	self.entries = entries
end

local function edicts_onclose(self)
	self.entries = nil
end

local function addedictstool(title, filter)
	local tool = addtool(title, edicts_onupdate, edicts_onopen, edicts_onclose)
	tool.filter = filter
end

local function traceentity_onopen(self)
	local edict = player.traceentity()

	if edict then
		qimgui.edictinfo(edict)
	else
		playlocal('doors/basetry.wav')
	end
end


addseparator('Edicts')
addedictstool('All Edicts')
addedictstool('Monsters', edicts.ismonster)
addedictstool('Teleports', edicts.isteleport)
addedictstool('Doors', edicts.isdoor)
addedictstool('Items', edicts.isitem)
addedictstool('Buttons', edicts.isbutton)
addedictstool('Exits', edicts.isexit)
addedictstool('Messages', edicts.ismessage)
addtool('Trace Entity', nil, traceentity_onopen)

addseparator('Misc')
addtool('Scratchpad', function (self)
	imgui.SetNextWindowPos(screenwidth * 0.5, screenheight * 0.5, imgui.constant.Cond.FirstUseEver, 0.5, 0.5)
	imgui.SetNextWindowSize(320, 240, imgui.constant.Cond.FirstUseEver)

	local visible, opened = imgui.Begin(self.title, true)

	if visible and opened then
		_, self.text = imgui.InputTextMultiline('##text', self.text or '', 64 * 1024, -1, -1, imgui.constant.InputTextFlags.AllowTabInput)
	end

	imgui.End()

	return opened
end)
addtool('Stop All Sounds', function () sound.stopall() end)

addseparator('Debug')
addtool('Dear ImGui Demo', imgui.ShowDemoWindow)

addseparator()
addtool('Press ESC to exit', qimgui.exit)
