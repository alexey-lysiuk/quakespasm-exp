
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
local key_pageup <const> = keycodes.PGUP
local key_pagedown <const> = keycodes.PGDN
local key_home <const> = keycodes.HOME
local key_end <const> = keycodes.END
local key_kpenter <const> = keycodes.KP_ENTER
local key_kpup <const> = keycodes.KP_UPARROW
local key_kpdown <const> = keycodes.KP_DOWNARROW
local key_kppageup <const> = keycodes.KP_PGUP
local key_kppagedown <const> = keycodes.KP_PGDN
local key_kphome <const> = keycodes.KP_HOME
local key_kpend <const> = keycodes.KP_END

local min <const> = math.min
local max <const> = math.max


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
		[key_escape] = function() menu.poppage() end,
		[key_up] = lineup,
		[key_down] = linedown,
		[key_pageup] = scrollup,
		[key_pagedown] = scrolldown,
		[key_home] = scrolltop,
		[key_end] = scrollend,
		[key_kpup] = lineup,
		[key_kpdown] = linedown,
		[key_kppageup] = scrollup,
		[key_kppagedown] = scrolldown,
		[key_kphome] = scrolltop,
		[key_kpend] = scrollend,
	}

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
