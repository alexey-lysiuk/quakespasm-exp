
keys =
{
	ENTER        = 13,
	ESCAPE       = 27,
	-- TODO
	BACKSPACE    = 127,
	UPARROW      = 128,
	DOWNARROW    = 129,
	LEFTARROW    = 130,
	RIGHTARROW   = 131,
}

function console.menu_test()
	local testpage =
	{
		ondraw = function(page)
			if page.state == 0 then
				menu.print(10, 10, 'Press any key')
			else
				menu.tintprint(10, 10, 'Press again to close menu')
			end
		end,

		onkeypress = function(page, key)
			if page.state == 0 then
				page.state = page.state + 1
			else
				menu.poppage()
			end
		end,

		state = 0
	}

	menu.pushpage(testpage)
end


local secretspage =
{
	ondraw = function(page)
		menu.tintprint(10, 0, 'Secrets')

		local count = #page.secrets

		if count == 0 then
			return
		end

		for i = 1, count do
			local secret = page.secrets[i]
			local entry = string.format('%i: %s', i, secret)
			menu.print(15, (i + 1) * 10, entry)
		end

		menu.tintprint(0, (page.cursor + 1) * 10, '*')
	end,

	onkeypress = function(page, key)
		local cursor = page.cursor

		if key == keys.ESCAPE then
			-- TODO: player.setpos
			menu.poppage()
		else if key == keys.ESCAPE then
			menu.poppage()
		elseif key == keys.UPARROW then
			cursor = cursor - 1
		elseif key == keys.DOWNARROW then
			cursor = cursor + 1
		end

		local count = #page.secrets

		if cursor == 0 then
			cursor = count
		elseif cursor > count then
			cursor = 1
		end

		page.cursor = cursor
	end,

	-- secrets = {},
	-- cursor = 1
}

local SUPER_SECRET <const> = edicts.spawnflags.SUPER_SECRET

local function addsecret(edict)
	if edict.classname ~= 'trigger_secret' then
		return
	end

	-- Try to handle Arcane Dimensions secret
	local min = edict.absmin
	local max = edict.absmax
	local count, pos

	if min == vec3minusone and max == vec3one then
		count = edict.count
	end

	if not count then
		-- Regular or Arcane Dimensions secret that was not revealed yet
		pos = vec3.mid(min, max)
	elseif count == 0 then
		-- Revealed Arcane Dimensions secret, skip it
		return
	else
		-- Disabled or switched off Arcane Dimensions secret
		-- Actual coodinates are stored in oldorigin member
		pos = edict.oldorigin
	end

	local secret = tostring(pos)

	if edict.spawnflags & SUPER_SECRET ~= 0 then
		secret = secret .. ' (super)'
	end
	-- local entry = supersecret and '(super)' or ''
	-- local entry = string.format('%i: %s%s', current, pos, extra)
	secretspage.secrets[#secretspage.secrets + 1] = secret

	-- return current + 1
end

function console.menu_secrets()
	local page = secretspage
	page.secrets = {}
	page.cursor = 1

	for _, edict in ipairs(edicts) do
		addsecret(edict)
	end

	-- edicts.foreach(addsecret)
	menu.pushpage(page)
end
