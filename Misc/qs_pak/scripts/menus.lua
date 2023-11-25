
local key_enter <const> = keycodes.ENTER
local key_escape <const> = keycodes.ESCAPE
local key_up <const> = keycodes.UPARROW
local key_down <const> = keycodes.DOWNARROW
local key_left <const> = keycodes.LEFTARROW
local key_right <const> = keycodes.RIGHTARROW
local key_pageup <const> = keycodes.PGUP
local key_pagedown <const> = keycodes.PGDN
local key_home <const> = keycodes.HOME
local key_end <const> = keycodes.END
local key_kpenter <const> = keycodes.KP_ENTER
local key_kpup <const> = keycodes.KP_UPARROW
local key_kpleft <const> = keycodes.KP_LEFTARROW
local key_kpright <const> = keycodes.KP_RIGHTARROW
local key_kpdown <const> = keycodes.KP_DOWNARROW
local key_kppageup <const> = keycodes.KP_PGUP
local key_kppagedown <const> = keycodes.KP_PGDN
local key_kphome <const> = keycodes.KP_HOME
local key_kpend <const> = keycodes.KP_END
local key_h <const> = string.byte('h')
local key_abutton <const> = keycodes.ABUTTON
local key_bbutton <const> = keycodes.BBUTTON

local min <const> = math.min
local max <const> = math.max

local function clamp(v, lo, up)
	return max(lo, min(up, v))
end

local pushpage <const> = menu.pushpage
local poppage <const> = menu.poppage


local defaultkeyremap <const> = 
{
	[key_enter] = { key_kpenter, key_abutton },
	[key_escape] = { key_bbutton },
	[key_up] = { key_kpup },
	[key_down] = { key_kpdown },
	[key_left] = { key_kpleft },
	[key_right] = { key_kpright },
	[key_pageup] = { key_kppageup },
	[key_pagedown] = { key_kpdown },
	[key_home] = { key_kphome },
	[key_end] = { key_kpend },
}

function menu.extendkeymap(actions)
	local addedactions = {}

	for key, func in pairs(actions) do
		local remap = defaultkeyremap[key]

		if remap then
			for _, newkey in ipairs(remap) do
				if not actions[newkey] then
					addedactions[newkey] = func
				end
			end
		elseif key > 0x40 and key < 0x5B then  -- 'A'..'Z'
			local newkey = key + 0x20

			if not actions[newkey] then
				addedactions[newkey] = func
			end
		elseif key > 0x60 and key < 0x7B then  -- 'a'..'z'
			local newkey = key - 0x20

			if not actions[newkey] then
				addedactions[newkey] = func
			end
		end
	end

	for key, func in pairs(addedactions) do
		actions[key] = func
	end
end

local extendkeymap = menu.extendkeymap


function menu.textpage()
	local page =
	{
		title = 'Title',
		text = {},
		maxlines = 20,  -- for line interval of 9 pixels
		topline = 1,
	}

	page.actions =
	{
		[key_escape] = poppage,
		[key_up] = function () page.topline = page.topline - 1 end,
		[key_down] = function () page.topline = page.topline + 1 end,
		[key_pageup] = function () page.topline = page.topline - page.maxlines end,
		[key_pagedown] = function () page.topline = page.topline + page.maxlines end,
		[key_home] = function () page.topline = 1  end,
		[key_end] = function () page.topline = #page.text end,
	}
	extendkeymap(page.actions)

	page.ondraw = function (page)
		menu.tintedtext(2, 0, page.title)

		for i = 1, min(page.maxlines, #page.text) do
			menu.text(2, (i + 1) * 9, page.text[page.topline + i - 1])
		end
	end

	page.onkeypress = function (page, keycode)
		local action = page.actions[keycode]

		if action then
			action()

			-- Bound topline value
			local maxtopline <const> = max(#page.text - page.maxlines + 1, 1)
			page.topline = clamp(page.topline, 1, maxtopline)
		end
	end

	return page
end


function menu.listpage()
	local page = 
	{
		title = 'Title',
		entries = {},
		cursor = 0,
		maxlines = 20,  -- for line interval of 9 pixels
		topline = 1,
	}

	local function scrolltop()
		page.cursor = 1
		page.topline = 1
	end

	local function lineup()
		if page.cursor > 1 then
			page.cursor = page.cursor - 1
			page.topline = page.cursor < page.topline and page.cursor or page.topline
		else
			page.cursor = #page.entries
			page.topline = max(#page.entries - page.maxlines + 1, 1)
		end
	end

	local function linedown()
		if page.cursor < #page.entries then
			page.cursor = page.cursor + 1
			page.topline = page.topline + (page.cursor == page.topline + page.maxlines and 1 or 0)
		else
			scrolltop()
		end
	end

	local function scrollup()
		if page.cursor > page.maxlines then
			page.cursor = page.cursor - page.maxlines
			page.topline = max(page.topline - page.maxlines, 1)
		else
			scrolltop()
		end
	end

	local function scrolldown()
		local entrycount <const> = #page.entries
		local maxlines <const> = page.maxlines

		if page.cursor + maxlines < entrycount then
			page.cursor = page.cursor + maxlines
			page.topline = min(page.topline + maxlines, entrycount - maxlines + 1)
		else
			page.cursor = entrycount
			page.topline = max(entrycount - maxlines + 1, 1)
		end
	end

	local function scrollend()
		local entrycount <const> = #page.entries

		page.cursor = entrycount
		page.topline = max(entrycount - page.maxlines + 1, 1)
	end

	page.actions =
	{
		[key_escape] = poppage,
		[key_up] = lineup,
		[key_down] = linedown,
		[key_pageup] = scrollup,
		[key_pagedown] = scrolldown,
		[key_home] = scrolltop,
		[key_end] = scrollend,
	}
	extendkeymap(page.actions)

	page.ondraw = function (page)
		menu.tintedtext(10, 0, page.title)

		local entrycount = #page.entries
		if entrycount == 0 then
			return
		end

		local topline = page.topline
		local cursor = page.cursor

		for i = 1, min(page.maxlines, entrycount) do
			menu.text(10, (i + 1) * 9, page.entries[topline + i - 1].text)
		end

		if cursor > 0 then
			menu.tintedtext(0, (cursor - topline + 2) * 9, '\13')
		end
	end

	page.onkeypress = function (page, keycode)
		local action = page.actions[keycode]

		if action then
			action()
		end
	end

	return page
end


local vec3origin <const> = vec3.new()
local foreach <const> = edicts.foreach
local isfree <const> = edicts.isfree
local getname <const> = edicts.getname


function menu.edictinfopage(edict, title)
	local page = menu.textpage()
	page.title = title

	-- Build edict fields table, and calculate maximum length of field names
	local fields = {}
	local maxlen = 0

	for i, field in ipairs(edict) do
		local len = field.name:len()

		if len > maxlen then
			maxlen = len
		end

		fields[i] = field
	end

	-- Output formatted names and values of edict fields
	local fieldformat = '%-' .. maxlen .. 's : %s'

	for _, field in ipairs(fields) do
		page.text[#page.text + 1] = string.format(fieldformat, field.name, field.value)
	end

	return page
end


function menu.edictspage()
	local page = menu.listpage()
	page.needupdate = true

	local function any(edict)
		local description, location

		if isfree(edict) then
			description = '<FREE>'
		else
			description = getname(edict)

			local origin = edict.origin or vec3origin
			local min = edict.absmin or vec3origin
			local max = edict.absmax or vec3origin

			location = origin == vec3origin and vec3.mid(min, max) or origin
		end

		return description, location
	end

	page.update = function ()
		page.entries = {}

		local filter = page.filter or any

		edicts.foreach(function (edict, current)
			local description, location, angles = filter(edict)

			if not description then
				return current
			end

			local index = current - (filter == any and 1 or 0)
			local text = location
				and string.format('%i: %s at %s', index, description, location)
				or string.format('%i: %s', index, description)
			local entry = { edict = edict, text = text, location = location, angles = angles }
			page.entries[#page.entries + 1] = entry

			return current + 1
		end)

		page.cursor = clamp(page.cursor, 1, #page.entries)
		page.topline = clamp(page.topline, 1, #page.entries - page.maxlines + 1)
	end

	local function moveto()
		if #page.entries == 0 then
			return
		end

		local entry = page.entries[page.cursor]
		local location = entry.location

		if location then
			player.safemove(location, entry.angles)
			poppage()
		end
	end

	local function showhelp()
		local helppage = menu.textpage()

		helppage.title = page.title .. ' -- Help'
		helppage.text =
		{
			'Up        - Select previous edict',
			'Down      - Select next edict',
			'Left      - Show values of edict fields',
			'Right     - Return to edicts list',
			'Page Up   - Scroll up',
			'Page Down - Scroll down',
			'Home      - Scroll to top',
			'End       - Scroll to end',
			'Enter     - Move player to selected edict',
			'Escape    - Exit or return to edicts list',
			'',
			'< Press any key to close >'
		}
		helppage.onkeypress = function ()
			poppage()
		end

		pushpage(helppage)
	end

	local function showinfo()
		if #page.entries == 0 then
			return
		end

		local entry = page.entries[page.cursor]
		local edict = entry.edict

		if not isfree(edict) then
			local infopage = menu.edictinfopage(edict, entry.text)

			local actions = infopage.actions
			actions[key_left] = poppage
			actions[key_h] = showhelp
			extendkeymap(actions)

			pushpage(infopage)
		end
	end

	local actions = page.actions
	actions[key_enter] = moveto
	actions[key_right] = showinfo
	actions[key_h] = showhelp
	extendkeymap(actions)

	local super_ondraw = page.ondraw

	page.ondraw = function (page)
		if page.needupdate then
			page.update()
			page.needupdate = false
		end

		super_ondraw(page)
		menu.text(180, 0, 'Press \200 for help')
	end

	return page
end


local edictsmenus = {}

local function addedictsmenu(title, filter)
	local name = string.lower(title)
	local command = 'menu_' .. name

	console[command] = function ()
		menu.clearpages()

		local mainpage = edictsmenus[name]

		if mainpage then
			mainpage.needupdate = true
		else
			mainpage = menu.edictspage()
			mainpage.title = title
			mainpage.filter = filter

			edictsmenus[name] = mainpage
		end

		pushpage(mainpage)
	end
end

addedictsmenu('Edicts')
addedictsmenu('Secrets', edicts.issecret)
addedictsmenu('Monsters', edicts.ismonster)
addedictsmenu('Teleports', edicts.isteleport)
addedictsmenu('Doors', edicts.isdoor)
addedictsmenu('Items', edicts.isitem)
