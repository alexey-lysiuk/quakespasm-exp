
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

local listpage_maxlines <const> = 20  -- for line interval of 9 pixels


local function listpage_draw(page)
	menu.tintedtext(10, 0, page.title)

	local entrycount = #page.entries
	if entrycount == 0 then
		return
	end

	local topline = page.topline

	for i = 1, listpage_maxlines do
--		local currentline = topline + i
--
--		if currentline > entrycount then
--			break
--		end

		menu.text(10, (i + 1) * 9, page.entries[topline + i - 1].text)
	end

	local cursor = page.cursor

	if cursor > 0 then
		menu.tintedtext(0, (cursor - topline + 2) * 9, '\13')
	end
end

local function listpage_keypress(page, keycode)
	local entrycount = #page.entries
	local cursor = page.cursor
--	local topline = page.topline

	if keycode == key_escape then
		menu.poppage()
		return
	elseif keycode == key_up then
		cursor = cursor > 1 and cursor - 1 or entrycount
--		topline = topline < cursor and cursor or topline
	elseif keycode == key_down then
		cursor = cursor < entrycount and cursor + 1 or 1
--		topline = cursor > topline + listpage_maxlines and cursor - listpage_maxlines or topline
	elseif keycode == key_pageup then
		cursor = cursor > listpage_maxlines and cursor - listpage_maxlines or 1
--		topline = topline > listpage_maxlines and topline - listpage_maxlines or 1
	elseif keycode == key_pagedown then
		cursor = cursor + listpage_maxlines < entrycount and cursor + listpage_maxlines or entrycount
--		topline = topline + listpage_maxlines < entrycount and topline + listpage_maxlines or entrycount
	else
		return
	end

	page.cursor = cursor

--print(cursor, topline)

--	if cursor <= 0 then
--		cursor = entrycount
--	elseif cursor >= entrycount then
--		cursor = 1
--	end
--
--	if topline <= 0 then
--		cursor = entrycount
--	elseif cursor >= entrycount then
--		cursor = 1
--	end

--	local function wrap(value)
--		return value < 1 and entrycount or value > entrycount and 1 or value
--	end

--	cursor = wrap(cursor)
--	topline = wrap(topline)

--	cursor = cursor < 1 and entrycount or cursor > entrycount and 1 or cursor
--	topline = topline < 1 and 1 or topline > entrycount and entrycount or topline

	local function min(a, b) return a < b and a or b end
	local function max(a, b) return a < b and b or a end
	local function clamp(v, lo, up) return max(lo, min(up, v)) end

--	local topline = page.topline
	local mintopline = max(cursor - listpage_maxlines + 1, 1)
	local maxtopline = min(cursor + listpage_maxlines - 1, max(entrycount - listpage_maxlines + 1, 1))

	page.topline = clamp(page.topline, mintopline, maxtopline)

	print(cursor, page.topline, mintopline, maxtopline)

--	-- Make sure line under cursor is visible
--	if cursor < topline then
--		topline = cursor
--	elseif cursor > topline + listpage_maxlines - 1 then
--		topline = cursor - listpage_maxlines + 1
--	end
--
--	if topline + listpage_maxlines > entrycount then
--		topline = entrycount - listpage_maxlines + 1
--	end

--	print(cursor, topline)
--	page.topline = topline
--	page.cursor = cursor
end

function menu.listpage()
	return
	{
		title = '',
		entries = {},
		cursor = 0,
		topline = 1,

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

print(#edicts)

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
