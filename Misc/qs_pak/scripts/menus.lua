
keycodes =
{
	TAB           = 9,
	ENTER         = 13,
	ESCAPE        = 27,
	SPACE         = 32,

	BACKSPACE     = 127,
	UPARROW       = 128,
	DOWNARROW     = 129,
	LEFTARROW     = 130,
	RIGHTARROW    = 131,

	COMMAND       = 170,
	ALT           = 132,
	CTRL          = 133,
	SHIFT         = 134,
	F1            = 135,
	F2            = 136,
	F3            = 137,
	F4            = 138,
	F5            = 139,
	F6            = 140,
	F7            = 141,
	F8            = 142,
	F9            = 143,
	F10           = 144,
	F11           = 145,
	F12           = 146,
	INS           = 147,
	DEL           = 148,
	PGDN          = 149,
	PGUP          = 150,
	HOME          = 151,
	END           = 152,
	PAUSE         = 255,

	KP_NUMLOCK    = 153,
	KP_SLASH      = 154,
	KP_STAR       = 155,
	KP_MINUS      = 156,
	KP_HOME       = 157,
	KP_UPARROW    = 158,
	KP_PGUP       = 159,
	KP_PLUS       = 160,
	KP_LEFTARROW  = 161,
	KP_5          = 162,
	KP_RIGHTARROW = 163,
	KP_END        = 164,
	KP_DOWNARROW  = 165,
	KP_PGDN       = 166,
	KP_ENTER      = 167,
	KP_INS        = 168,
	KP_DEL        = 169,

	MOUSE1        = 200,
	MOUSE2        = 201,
	MOUSE3        = 202,
	MOUSE4        = 241,
	MOUSE5        = 242,
	MWHEELUP      = 239,
	MWHEELDOWN    = 240,
}


local min = math.min
local max = math.max

-- REMOVE
function player.safemove(location)
	player.god(true)
	player.notarget(true)

	-- Adjust Z coordinate so player will appear slightly above the destination
	local playerpos = location
	playerpos.z = playerpos.z + 20
	player.setpos(location, angles)
end
-- REMOVE


local function listpage_draw(page)
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

local function listpage_keypress(page, keycode)
	local action = page.actions[keycode]

	if action then
		action(page)
	end
end

local function listpage_keyup(page)
	if page.cursor > 1 then
		page.cursor = page.cursor - 1
		page.topline = page.cursor < page.topline and page.cursor or page.topline
	else
		page.cursor = #page.entries
		page.topline = max(#page.entries - page.maxlines + 1, 1)
	end
end

local function listpage_keydown(page)
	if page.cursor < #page.entries then
		page.cursor = page.cursor + 1
		page.topline = page.topline + (page.cursor == page.topline + page.maxlines and 1 or 0)
	else
		page.cursor = 1
		page.topline = 1
	end
end

local function listpage_keypageup(page)
	if page.cursor > page.maxlines then
		page.cursor = page.cursor - page.maxlines
		page.topline = max(page.topline - page.maxlines, 1)
	else
		page.cursor = 1
		page.topline = 1
	end
end

local function listpage_keypagedown(page)
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

local function listpage_keyhome(page)
	page.cursor = 1
	page.topline = 1
end

local function listpage_keyend(page)
	local entrycount <const> = #page.entries

	page.cursor = entrycount
	page.topline = max(entrycount - page.maxlines + 1, 1)
end

function menu.listpage()
	return
	{
		title = 'Menu',
		entries = {},
		cursor = 0,
		maxlines = 20,  -- for line interval of 9 pixels
		topline = 1,
		actions =
		{
			[keycodes.ESCAPE] = function() menu.poppage() end,
			[keycodes.UPARROW] = listpage_keyup,
			[keycodes.DOWNARROW] = listpage_keydown,
			[keycodes.PGUP] = listpage_keypageup,
			[keycodes.PGDN] = listpage_keypagedown,
			[keycodes.HOME] = listpage_keyhome,
			[keycodes.END] = listpage_keyend,
			[keycodes.KP_UPARROW] = listpage_keyup,
			[keycodes.KP_DOWNARROW] = listpage_keydown,
			[keycodes.KP_PGUP] = listpage_keypageup,
			[keycodes.KP_PGDN] = listpage_keypagedown,
			[keycodes.KP_HOME] = listpage_keyhome,
			[keycodes.KP_END] = listpage_keyend,
		},

		ondraw = listpage_draw,
		onkeypress = listpage_keypress,
	}
end


local vec3origin <const> = vec3.new()
local foreach <const> = edicts.foreach
local isfree <const> = edicts.isfree
local getname <const> = edicts.getname


local function edictinfopage_draw(page)
	menu.tintedtext(10, 0, 'Edict info')

	-- Build edict fields table, and calculate maximum length of field names
	local fields = {}
	local maxlen = 0

	for i, field in ipairs(page.edict) do
		local len = field.name:len()

		if len > maxlen then
			maxlen = len
		end

		fields[i] = field
	end

	-- Output formatted names and values of edict fields
	local fieldformat = '%-' .. maxlen .. 's : %s'

	for i, field in ipairs(fields) do
		menu.text(2, (i + 1) * 9, string.format(fieldformat, field.name, field.value))
	end
end

function menu.edictinfopage(edict)

end


local function edictspage_keyenter(page)
	local location = page.entries[page.cursor].location

	if location then
		player.safemove(location)
		menu.poppage()
	end
end

local function edictspage_keyleft(page)
	local edict = page.edict

	if not edicts.isfree() then
		menu.pushpage(menu.edictinfopage(edict))
	end
end

function menu.edictspage()
	local page = menu.listpage()
	page.title = 'Edicts'
	page.cursor = 1
	page.actions[keycodes.ENTER] = edictspage_keyenter
	page.actions[keycodes.KP_ENTER] = edictspage_keyenter
	page.actions[keycodes.LEFTARROW] = edictspage_keyleft
	page.actions[keycodes.KP_LEFTARROW] = edictspage_keyleft

	local function addedict(edict, current)
		local text, location

		if isfree(edict) then
			text = string.format('%i: <FREE>', current - 1)
		else
			local origin = edict.origin or vec3origin
			local min = edict.absmin or vec3origin
			local max = edict.absmax or vec3origin

			location = origin == vec3origin and vec3.mid(min, max) or origin
			text = string.format('%i: %s at %s', current - 1, getname(edict), location)
		end

		local entry = { edict = edict, text = text, location = location }
		page.entries[#page.entries + 1] = entry

		return current + 1
	end

	edicts.foreach(addedict)

	return page
end


function console.menu_edicts()
	menu.clearpages()
	menu.pushpage(menu.edictspage())
end
