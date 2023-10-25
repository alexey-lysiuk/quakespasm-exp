
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


local function secretspage_draw(page)
	menu.tintedtext(10, 0, 'Secrets')

	local secretscount = 0

	local function addsecret(edict, current)
		local pos, extra = edicts.issecret(edict)

		if not pos then
			return current
		end

		local entry = string.format('%i: %s %s', current, pos, extra)
		menu.text(10, (current + 1) * 10, entry)

		secretscount = secretscount + 1
		return current + 1
	end

	edicts.foreach(addsecret)

	if secretscount > 0 then
		local cursor = page.cursor

		if cursor == 0 then
			cursor = secretscount
		elseif cursor > secretscount then
			cursor = 1
		end

		page.cursor = cursor

		menu.tintedtext(0, (page.cursor + 1) * 10, '\13')
	end

	-- TODO: Action option
	-- TODO: Back item
end

local function secretspage_keypress(page, keycode)
	local cursor = page.cursor

	if keycode == keycodes.ENTER then
		local function movetosecret(edict, current, choice)
			local pos = edicts.issecret(edict)

			if pos then
				if current == choice then
					player.setpos(pos)
					menu.poppage()
					return
				else
					return current + 1
				end
			end

			return current
		end

		edicts.foreach(movetosecret, cursor)
	elseif keycode == keycodes.ESCAPE then
		menu.poppage()
	elseif keycode == keycodes.UPARROW then
		cursor = cursor - 1
	elseif keycode == keycodes.DOWNARROW then
		cursor = cursor + 1
	end

	page.cursor = cursor
end


local function scrollablepage_draw(page)
	menu.tintedtext(10, 0, page.title)

	local entrycount = #page.entries
	if entrycount == 0 then
		return
	end
	
	for i = 1, entrycount do
		menu.text(10, (i + 1) * 10, page.entries[i].text)
	end

	local cursor = page.cursor

	if cursor == 0 then
		cursor = entrycount
	elseif cursor > entrycount then
		cursor = 1
	end

	page.cursor = cursor

	menu.tintedtext(0, (cursor + 1) * 10, '\13')
end

local function scrollablepage_keypress(page, keycode)
	local cursor = page.cursor

	if keycode == keycodes.ESCAPE then
		menu.poppage()
	elseif keycode == keycodes.UPARROW then
		cursor = cursor - 1
	elseif keycode == keycodes.DOWNARROW then
		cursor = cursor + 1
	end

	page.cursor = cursor
end

function menu.scrollablepage()
	return
	{
		ondraw = scrollablepage_draw,
		onkeypress = scrollablepage_keypress,

		title = '',
		entries = {},
		cursor = 1,
	}
end


function console.menu_secrets()
	local secretspage = menu.scrollablepage()
	secretspage.title = 'Secrets'

	local function addsecret(edict, current)
		local description, location = edicts.issecret(edict)

		if not description then
			return current
		end

		local text = string.format('%i: %s at %s', current, description, location)
		local entry = { text = text, location = location }
		secretspage.entries[#secretspage.entries + 1] = entry

		return current + 1
	end

	edicts.foreach(addsecret)

	menu.clearpages()
	menu.pushpage(secretspage)
end
