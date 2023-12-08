
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
local key_h <const> = keycodes.LH
local key_i <const> = keycodes.LI
local key_r <const> = keycodes.LR
local key_ua <const> = keycodes.UA
local key_uz <const> = keycodes.UZ
local key_la <const> = keycodes.LA
local key_lz <const> = keycodes.LZ
local key_abutton <const> = keycodes.ABUTTON
local key_bbutton <const> = keycodes.BBUTTON

local min <const> = math.min
local max <const> = math.max

local function clamp(v, lo, up)
	return max(lo, min(up, v))
end

local floor <const> = math.floor
local format <const> = string.format
local insert <const> = table.insert

local pushpage <const> = menu.pushpage
local poppage <const> = menu.poppage
local clearpages <const> = menu.clearpages
local playlocal <const> = sound.playlocal
local realtime <const> = host.realtime
local tint <const> = text.tint
local text <const> = menu.text


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
		elseif key >= key_ua and key <= key_uz then
			local newkey = key + 0x20

			if not actions[newkey] then
				addedactions[newkey] = func
			end
		elseif key >= key_la and key <= key_lz then
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

local function menusound(index)
	if index then
		playlocal(format('misc/menu%d.wav', index))
	end
end

local function deniedsound()
	playlocal('doors/basetry.wav')
end

local function defaultsounds()
	return
	{
		[key_escape] = 2,
		[key_up] = 1,
		[key_down] = 1,
		[key_pageup] = 1,
		[key_pagedown] = 1,
		[key_home] = 1,
		[key_end] = 1,
	}
end

function menu.elide(string, x)
	local width = 640 - (x or 0)

	if #string * 16 > width then
		local maxlen = math.floor(width / 16) - 1
		local elideformat = '%.' .. maxlen .. 's\133'
		return elideformat:format(string, line)
	end

	return string
end

local elide = menu.elide


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

	page.sounds = defaultsounds()

	page.ondraw = function (page)
		text(2, 0, page.title)

		for i = 1, min(page.maxlines, #page.text) do
			text(2, (i + 1) * 9, page.text[page.topline + i - 1])
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

		menusound(page.sounds[keycode])
	end

	return page
end


function menu.listpage()
	local page = 
	{
		title = 'Title',
		entries = {},
		cursor = 0,
		blinktime = 0,
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

	page.sounds = defaultsounds()

	page.ondraw = function (page)
		text(10, 0, page.title)

		local entrycount = #page.entries
		if entrycount == 0 then
			return
		end

		local topline = page.topline
		local cursor = page.cursor

		for i = 1, min(page.maxlines, entrycount) do
			text(10, (i + 1) * 9, page.entries[topline + i - 1].text)
		end

		if cursor > 0 and floor((realtime() - page.blinktime) * 4) & 1 == 0 then
			text(0, (cursor - topline + 2) * 9, '\13')
		end
	end

	page.onkeypress = function (page, keycode)
		local action = page.actions[keycode]

		if action then
			page.blinktime = realtime()
			action()
		end

		menusound(page.sounds[keycode])
	end

	return page
end


local vec3origin <const> = vec3.new()
local foreach <const> = edicts.foreach
local isfree <const> = edicts.isfree
local getname <const> = edicts.getname


function menu.edictinfopage(edict)
	local page = menu.textpage()
	page.title = tint(tostring(edict))

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
	local fieldformat = '%-' .. maxlen .. 's: %s'

	for _, field in ipairs(fields) do
		local line = fieldformat:format(field.name, field.value)
		insert(page.text, elide(line))
	end

	return page
end


local function describe(edict)
	local description = getname(edict)
	local location, angles

	if not isfree(edict) then
		location = vec3.mid(edict.absmin, edict.absmax)
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


function menu.edictspage()
	local page = menu.listpage()
	page.needupdate = true

	page.update = function ()
		page.entries = {}

		local filter = page.filter or describe

		edicts.foreach(function (edict, current)
			local description, location, angles = filter(edict)

			if not description then
				return current
			end

			local index = current - (filter == describe and 1 or 0)
			local text = location
				and format('%i: %s at %s', index, description, location)
				or format('%i: %s', index, description)
			local entry = { edict = edict, text = elide(text, 10), location = location, angles = angles }
			insert(page.entries, entry)

			return current + 1
		end)

		page.cursor = clamp(page.cursor, 1, #page.entries)
		page.topline = clamp(page.topline, 1, #page.entries - page.maxlines + 1)

		page.blinktime = realtime()
	end

	local function moveto()
		if #page.entries == 0 then
			return
		end

		local entry = page.entries[page.cursor]
		local location = entry.location

		if location then
			if edicts.isitem(entry.edict) then
				-- Adjust Z coordinate so player will appear slightly above destination
				location = vec3.copy(location)
				location.z = location.z + 20
			end

			player.safemove(location, entry.angles)
			clearpages()
		end
	end

	local function showhelp()
		local helppage = menu.textpage()

		helppage.title = page.title .. ' -- Help'
		helppage.text =
		{
			'Up        - Select previous edict',
			'Down      - Select next edict',
			'Page Up   - Scroll up',
			'Page Down - Scroll down',
			'Home      - Scroll to top',
			'End       - Scroll to end',
			'Enter     - Move player to edict',
			'Escape    - Close current page',
			'H         - Show this help',
			'I         - Show values of edict fields',
			'R         - Show edict references',
			'',
			'< Press any key to close >'
		}
		helppage.onkeypress = function ()
			poppage()
			menusound(2)
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
			local infopage = menu.edictinfopage(edict)
			infopage.sounds[key_h] = 2

			local actions = infopage.actions
			actions[key_h] = showhelp
			extendkeymap(actions)

			pushpage(infopage)
		end
	end

	local function showreferences()
			if #page.entries == 0 then
			return
		end

		local entry = page.entries[page.cursor]
		local edict = entry.edict

		if not isfree(edict) then
			local refspage = menu.edictreferencespage(edict)
			pushpage(refspage)
		end
	end

	local actions = page.actions
	actions[key_enter] = moveto
	actions[key_h] = showhelp
	actions[key_i] = showinfo
	actions[key_r] = showreferences
	extendkeymap(actions)

	local sounds = page.sounds
	sounds[key_h] = 2
	sounds[key_i] = 2
	sounds[key_r] = 2

	local super_ondraw = page.ondraw

	page.ondraw = function (page)
		if page.needupdate then
			page.update()
			page.needupdate = false
		end

		super_ondraw(page)

		if #page.title < 20 then
			text(180, 0, 'Press \200 for help')
		end
	end

	return page
end


function menu.edictreferencespage(edict)
	local page = menu.edictspage()
	page.title = tint(tostring(edict))

	local target = edict.target or ''
	local targetname = edict.targetname or ''
	local owner = edict.owner

	page.filter = function (edict)
		if isfree(edict) then
			return
		end

		local isreference = owner == edict
			or target ~= '' and target == edict.targetname
			or targetname ~= '' and targetname == edict.target

		if isreference then
			return describe(edict)
		end
	end

	return page
end


local edictsmenus = {}

local function addedictsmenu(title, filter)
	local name = string.lower(title)
	local command = 'menu_' .. name

	console[command] = function ()
		if #edicts == 0 then
			deniedsound()
			return
		end

		clearpages()

		local mainpage = edictsmenus[name]

		if mainpage then
			mainpage.needupdate = true
		else
			mainpage = menu.edictspage()
			mainpage.title = tint(title)
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


local function addgazemenu(suffix, pagefunc)
	console['menu_' .. suffix] = function ()
		local edict = player.traceentity()

		if edict then
			clearpages()
			pushpage(pagefunc(edict))
		else
			deniedsound()
		end
	end
end

addgazemenu('gaze', menu.edictinfopage)
addgazemenu('gazerefs', menu.edictreferencespage)
