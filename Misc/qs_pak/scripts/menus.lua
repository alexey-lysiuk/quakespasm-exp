
keycodes =
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


local function secretspage_draw(page)
	menu.tintprint(10, 0, 'Secrets')

	local secretscount = 0

	local function addsecret(edict, current)
		local pos, extra = edicts.issecret(edict)

		if not pos then
			return current
		end

		local entry = string.format('%i: %s %s', current, pos, extra)
		menu.print(15, (current + 1) * 10, entry)

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

		menu.tintprint(0, (page.cursor + 1) * 10, '\13')
	end

	-- TODO: Action option
	-- TODO: Back item
end

local function secretspage_keypress(page, keycode)
	local cursor = page.cursor

	if keycode == keycodes.ESCAPE then
		-- TODO: player.setpos
		menu.poppage()
	elseif keycode == keycodes.ESCAPE then
		menu.poppage()
	elseif keycode == keycodes.UPARROW then
		cursor = cursor - 1
	elseif keycode == keycodes.DOWNARROW then
		cursor = cursor + 1
	end

	page.cursor = cursor
end

-- local secretspage =
-- {
-- 	ondraw = function(page)
-- 		menu.tintprint(10, 0, 'Secrets')
--
-- 		local count = #page.secrets
--
-- 		if count == 0 then
-- 			return
-- 		end
--
-- 		for i = 1, count do
-- 			local secret = page.secrets[i]
-- 			local entry = string.format('%i: %s', i, secret)
-- 			menu.print(15, (i + 1) * 10, entry)
-- 		end
--
-- 		menu.tintprint(0, (page.cursor + 1) * 10, '\12')
-- 	end,
--
-- 	onkeypress = function(page, keycode)
-- 		local cursor = page.cursor
--
-- 		if keycode == keycodes.ESCAPE then
-- 			-- TODO: player.setpos
-- 			menu.poppage()
-- 		else if keycode == keycodes.ESCAPE then
-- 			menu.poppage()
-- 		elseif keycode == keycodes.UPARROW then
-- 			cursor = cursor - 1
-- 		elseif keycode == keycodes.DOWNARROW then
-- 			cursor = cursor + 1
-- 		end
--
-- 		local count = #page.secrets
--
-- 		if cursor == 0 then
-- 			cursor = count
-- 		elseif cursor > count then
-- 			cursor = 1
-- 		end
--
-- 		page.cursor = cursor
-- 	end,
-- }

-- local SUPER_SECRET <const> = edicts.spawnflags.SUPER_SECRET
--
-- local function addsecret(edict)
-- 	if edict.classname ~= 'trigger_secret' then
-- 		return
-- 	end
--
-- 	-- Try to handle Arcane Dimensions secret
-- 	local min = edict.absmin
-- 	local max = edict.absmax
-- 	local count, pos
--
-- 	if min == vec3minusone and max == vec3one then
-- 		count = edict.count
-- 	end
--
-- 	if not count then
-- 		-- Regular or Arcane Dimensions secret that was not revealed yet
-- 		pos = vec3.mid(min, max)
-- 	elseif count == 0 then
-- 		-- Revealed Arcane Dimensions secret, skip it
-- 		return
-- 	else
-- 		-- Disabled or switched off Arcane Dimensions secret
-- 		-- Actual coodinates are stored in oldorigin member
-- 		pos = edict.oldorigin
-- 	end
--
-- 	local secret = tostring(pos)
--
-- 	if edict.spawnflags & SUPER_SECRET ~= 0 then
-- 		secret = secret .. ' (super)'
-- 	end
-- 	-- local entry = supersecret and '(super)' or ''
-- 	-- local entry = string.format('%i: %s%s', current, pos, extra)
-- 	secretspage.secrets[#secretspage.secrets + 1] = secret
--
-- 	-- return current + 1
-- end

function console.menu_secrets()
	-- local page = secretspage
	-- page.secrets = {}
	-- page.cursor = 1
	--
	-- for _, edict in ipairs(edicts) do
	-- 	addsecret(edict)
	-- end
	--
	-- -- edicts.foreach(addsecret)
	-- menu.pushpage(page)

	local secretspage =
	{
		ondraw = secretspage_draw,
		onkeypress = secretspage_keypress,
		cursor = 1,
	}

	menu.pushpage(secretspage)
end
