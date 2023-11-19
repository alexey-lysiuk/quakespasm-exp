
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


local key_enter <const> = keycodes.ENTER
local key_escape <const> = keycodes.ESCAPE
local key_up <const> = keycodes.UPARROW
local key_down <const> = keycodes.DOWNARROW


local function listpage_draw(page)
	menu.tintedtext(10, 0, page.title)

	local entrycount = #page.entries
	if entrycount == 0 then
		return
	end

	for i = 1, entrycount do
		menu.text(10, (i + 1) * 10, page.entries[i].text)
	end

	local cursor = page.cursor

	if cursor > 0 then
		menu.tintedtext(0, (cursor + 1) * 10, '\13')
	end
end

local function listpage_keypress(page, keycode)
	local cursor = page.cursor

	if keycode == key_escape then
		menu.poppage()
		return
	elseif keycode == key_up then
		cursor = cursor - 1
	elseif keycode == key_down then
		cursor = cursor + 1
	end

	local entrycount = #page.entries

	if cursor == 0 then
		cursor = entrycount
	elseif cursor > entrycount then
		cursor = 1
	end

	page.cursor = cursor
end

function menu.listpage()
	return
	{
		title = '',
		entries = {},
		cursor = 0,

		ondraw = listpage_draw,
		onkeypress = listpage_keypress,
	}
end


local vec3origin <const> = vec3.new()
local foreach <const> = edicts.foreach
local isfree <const> = edicts.isfree
local getname <const> = edicts.getname


local function edictspage_keypress(page, keycode)
	if keycode == key_enter then
		local location = page.entries[page.cursor].location

		if location ~= vec3origin then
			player.safemove(location)
			menu.poppage()
		end
	else
		listpage_keypress(page, keycode)
	end
end

function menu.edictspage()
	local page = menu.listpage()
	page.title = 'Edicts'
	page.cursor = 1
	page.onkeypress = edictspage_keypress

	local function addedict(edict, current)
		local text, location

		if isfree(edict) then
			text = string.format('%i: <FREE>', current)
			location = vec3origin
		else
			local origin = edict.origin or vec3origin
			local min = edict.absmin or vec3origin
			local max = edict.absmax or vec3origin

			location = origin == vec3origin and vec3.mid(min, max) or origin
			text = string.format('%i: %s at %s', current, getname(edict), location)
		end

		local entry = { text = text, location = location }
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
