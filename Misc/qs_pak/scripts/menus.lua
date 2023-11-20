
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


--local key_enter <const> = keycodes.ENTER
--local key_escape <const> = keycodes.ESCAPE
--local key_up <const> = keycodes.UPARROW
--local key_down <const> = keycodes.DOWNARROW
--local key_pageup <const> = keycodes.PGUP
--local key_pagedown <const> = keycodes.PGDN

--local listpage_maxlines <const> = 20  -- for line interval of 9 pixels

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

--local listpage_actions <const> =
--{
--	[keycodes.ESCAPE] = function() menu.poppage() end,
--	[keycodes.UPARROW] = function() menu.poppage() end,
--}

--local function listpage_keypress(page, keycode)
--	local entrycount <const> = #page.entries
--	local cursor = page.cursor
--	local topline = page.topline
--
----	local function min(a, b) return a < b and a or b end
----	local function max(a, b) return a < b and b or a end
----	local function clamp(v, lo, up) return max(lo, min(up, v)) end
--
--	if keycode == key_escape then
--		menu.poppage()
--		return
--	elseif keycode == key_up then
----		cursor = cursor > 1 and cursor - 1 or entrycount
----		topline = topline < cursor and cursor or topline
--		if cursor > 1 then
--			cursor = cursor - 1
--			topline = cursor < topline and cursor or topline
--		else
--			cursor = entrycount
--			topline = max(entrycount - listpage_maxlines + 1, 1)
----			topline = entrycount > listpage_maxlines and entrycount - listpage_maxlines or 1
--		end
--	elseif keycode == key_down then
----		cursor = cursor < entrycount and cursor + 1 or 1
----		topline = cursor > topline + listpage_maxlines and cursor - listpage_maxlines or topline
--		if cursor < entrycount then
--			cursor = cursor + 1
--			topline = topline + (cursor == topline + listpage_maxlines and 1 or 0)
--		else
--			cursor = 1
--			topline = 1
--		end
--	elseif keycode == key_pageup then
----		cursor = cursor > listpage_maxlines and cursor - listpage_maxlines or 1
----		topline = topline > listpage_maxlines and topline - listpage_maxlines or 1
--		if cursor > listpage_maxlines then
--			cursor = cursor - listpage_maxlines
--			topline = max(topline - listpage_maxlines, 1)
--		else
--			cursor = 1
--			topline = 1
--		end
--	elseif keycode == key_pagedown then
----		cursor = cursor + listpage_maxlines < entrycount and cursor + listpage_maxlines or entrycount
----		topline = topline + listpage_maxlines < entrycount and topline + listpage_maxlines or entrycount
--		if cursor + listpage_maxlines < entrycount then
--			cursor = cursor + listpage_maxlines
--			topline = min(topline + listpage_maxlines, entrycount - listpage_maxlines + 1)
--		else
--			cursor = entrycount
--			topline = max(entrycount - listpage_maxlines + 1, 1)
--		end
--	else
--		return
--	end
--
----	page.cursor = cursor
--
----print(cursor, topline)
--
----	if cursor <= 0 then
----		cursor = entrycount
----	elseif cursor > entrycount then
----		cursor = 1
----	end
--
----	if topline <= 0 then
----		cursor = entrycount
----	elseif cursor >= entrycount then
----		cursor = 1
----	end
--
----	local function wrap(value)
----		return value < 1 and entrycount or value > entrycount and 1 or value
----	end
--
----	cursor = wrap(cursor)
----	topline = wrap(topline)
--
----	cursor = cursor < 1 and entrycount or cursor > entrycount and 1 or cursor
----	topline = topline < 1 and 1 or topline > entrycount and entrycount or topline
--
----	local function min(a, b) return a < b and a or b end
----	local function max(a, b) return a < b and b or a end
----	local function clamp(v, lo, up) return max(lo, min(up, v)) end
--
------	local topline = page.topline
----	local mintopline = max(cursor - listpage_maxlines + 1, 1)
----	local maxtopline = min(cursor + listpage_maxlines - 1, max(entrycount - listpage_maxlines + 1, 1))
----
----	page.topline = clamp(page.topline, mintopline, maxtopline)
----
----	print(cursor, page.topline, mintopline, maxtopline)
--
----	-- Make sure line under cursor is visible
----	if cursor < topline then
----		topline = cursor
----	elseif cursor > topline + listpage_maxlines - 1 then
----		topline = cursor - listpage_maxlines + 1
----	end
----
----	if topline + listpage_maxlines > entrycount then
----		topline = entrycount - listpage_maxlines + 1
----	end
--
--	print(cursor, topline)
--	page.topline = topline
--	page.cursor = cursor
--end

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
		title = '',
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


--local function edictspage_keypress(page, keycode)
--	if keycode == keycodes.ENTER then
--		local location = page.entries[page.cursor].location
--
--		if location ~= vec3origin then
--			player.safemove(location)
--			menu.poppage()
--		end
--	else
--		listpage_keypress(page, keycode)
--	end
--end

local function edictspage_keyenter(page, keycode)
	local location = page.entries[page.cursor].location

	if location then
		player.safemove(location)
		menu.poppage()
	end
end

function menu.edictspage()
	local page = menu.listpage()
	page.title = 'Edicts'
	page.cursor = 1
--	page.onkeypress = edictspage_keypress
	page.actions[keycodes.ENTER] = edictspage_keyenter
	page.actions[keycodes.KP_ENTER] = edictspage_keyenter

	local function addedict(edict, current)
		local text, location

		if isfree(edict) then
			text = string.format('%i: <FREE>', current)
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
