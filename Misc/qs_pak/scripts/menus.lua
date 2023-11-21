
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
local key_H <const> = string.byte('H')
local key_h <const> = string.byte('h')

local min <const> = math.min
local max <const> = math.max

local function clamp(v, lo, up)
	return max(lo, min(up, v))
end


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


function menu.textpage()
	local page =
	{
		title = 'Title',
		text = {},
		maxlines = 20,  -- for line interval of 9 pixels
		topline = 1,
	}

	local function lineup() page.topline = page.topline - 1 end
	local function linedown() page.topline = page.topline + 1 end
	local function scrollup() page.topline = page.topline - page.maxlines end
	local function scrolldown() page.topline = page.topline + page.maxlines end
	local function scrolltop() page.topline = 1 end
	local function scrollend() page.topline = #page.text end

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
	page.title = 'Edicts'
	page.cursor = 1

	edicts.foreach(function (edict, current)
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
	end)

	local function moveto()
		local location = page.entries[page.cursor].location

		if location then
			player.safemove(location)
			menu.poppage()
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
			menu.poppage()
		end

		menu.pushpage(helppage)
	end

	local function showinfo()
		local entry = page.entries[page.cursor]
		local edict = entry.edict

		if not isfree(edict) then
			local infopage = menu.edictinfopage(edict, entry.text)
			local exit = infopage.actions[key_escape]

			infopage.actions[key_left] = exit
			infopage.actions[key_kpleft] = exit
			infopage.actions[key_H] = showhelp
			infopage.actions[key_h] = showhelp

			menu.pushpage(infopage)
		end
	end

	local actions = page.actions
	actions[key_enter] = moveto
	actions[key_kpenter] = moveto
	actions[key_right] = showinfo
	actions[key_kpright] = showinfo
	actions[key_H] = showhelp
	actions[key_h] = showhelp

	return page
end


function console.menu_edicts()
	menu.clearpages()
	menu.pushpage(menu.edictspage())
end
