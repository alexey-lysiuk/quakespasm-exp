
--
-- Edict flags
--

edicts.flags =
{
	FL_FLY            = 1,
	FL_SWIM           = 2,
	FL_CONVEYOR       = 4,
	FL_CLIENT         = 8,
	FL_INWATER        = 16,
	FL_MONSTER        = 32,
	FL_GODMODE        = 64,
	FL_NOTARGET       = 128,
	FL_ITEM           = 256,
	FL_ONGROUND       = 512,
	FL_PARTIALGROUND  = 1024,  -- not all corners are valid
	FL_WATERJUMP      = 2048,  -- player jumping out of water
	FL_JUMPRELEASED   = 4096,  -- for jump debouncing
}

edicts.spawnflags = 
{
	SUPER_SECRET      = 2,  -- Copper specific

	DOOR_GOLD_KEY     = 8,
	DOOR_SILVER_KEY   = 16,

	NOT_EASY          = 256,
	NOT_MEDIUM        = 512,
	NOT_HARD          = 1024,
	NOT_DEATHMATCH    = 2048,
}


function edicts.foreach(func, choice)
	choice = choice and math.tointeger(choice) or 0
	local current = 1

	for _, edict in ipairs(edicts) do
		if not edicts.isfree(edict) then
			current = func(edict, current, choice)

			if not current then
				break
			end
		end
	end
end

function edicts.isclass(edict, ...)
	for _, classname in ipairs({...}) do
		if edict.classname == classname then
			return classname
		end
	end	

	return nil
end


local vec3origin = vec3.new()
local vec3one = vec3.new(1, 1, 1)
local vec3minusone = vec3.new(-1, -1, -1)

local FL_MONSTER <const> = edicts.flags.FL_MONSTER

local SUPER_SECRET <const> = edicts.spawnflags.SUPER_SECRET


local function titlecase(str)
	return str:gsub("(%a)([%w_']*)", 
		function(head, tail) 
			return head:upper()..tail:lower() 
		end)
end


--
-- Secrets
--

local function handlesecret(edict, current, choice)
	if edict.classname == 'trigger_secret' then
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
			return current
		else
			-- Disabled or switched off Arcane Dimensions secret
			-- Actual coodinates are stored in oldorigin member
			pos = edict.oldorigin
		end

		if choice <= 0 then
			local supersecret = edict.spawnflags & SUPER_SECRET ~= 0
			local extra = supersecret and '(super)' or ''
			print(current .. ':', pos, extra)
		elseif choice == current then
			player.setpos(pos)
			return nil
		end

		return current + 1
	end

	return current
end

function console.secrets(choice)
	edicts.foreach(handlesecret, choice)
end


--
-- Monsters
--

local function handlemonster(edict, current, choice)
	local flags = edict.flags
	local health = edict.health

	if flags and health then
		local ismonster = flags & FL_MONSTER ~= 0
		local isalive = health > 0

		if not ismonster or not isalive then
			return current
		end

		if choice <= 0 then
			local classname = edict.classname

			-- Remove classname prefix if present
			if classname:find("monster_") == 1 then
				classname = classname:sub(9)
			end

			print(current .. ':', classname, 'at', edict.origin)
		elseif choice == current then
			player.god(true)
			player.notarget(true)
			player.setpos(edict.origin, edict.angles)
			return nil
		end
	end

	return current + 1
end

function console.monsters(choice)
	edicts.foreach(handlemonster, choice)
end


--
-- Teleports
--

local function handleteleport(edict, current, choice)
	if edict.classname == 'trigger_teleport' then
		local pos = vec3.mid(edict.absmin, edict.absmax)

		if choice <= 0 then
			local teletarget = edict.target

			if teletarget then
				for _, testedict in ipairs(edicts) do
					if teletarget == testedict.targetname then
						-- Special case for Arcane Dimensions, ad_tears map in particular
						-- It uses own teleport target class (info_teleportinstant_dest) which is disabled by default
						-- Some teleport destinations were missing despite their valid setup
						-- Actual destination coordinates are stored in oldorigin member
						if testedict.origin == vec3origin then
							targetpos = testedict.oldorigin
						else
							targetpos = testedict.origin
						end
						break
					end
				end
			end

			local targetstr = targetpos and 'at ' .. targetpos or '(target not found)'
			print(current .. ':', pos, '->', teletarget, targetstr)
		elseif choice == current then
			player.setpos(pos)
			return nil
		end

		return current + 1
	end

	return current
end

function console.teleports(choice)
	edicts.foreach(handleteleport, choice)
end


--
-- Doors
--

local function getitemname(item)
	if not item or item == 0 then
		return nil
	end

	for _, edict in ipairs(edicts) do
		if edict.items == item and edict.classname:find('item_') == 1 then
			return edict.netname
		end
	end

	return nil
end

local function handledoor(edict, current, choice)
	local door_secret_class = 'func_door_secret'
	local classname = edicts.isclass(edict, 'door', 'func_door', door_secret_class)
	
	if classname then
		local pos = vec3.mid(edict.absmin, edict.absmax)
		local info = getitemname(edict.items)

		if classname == door_secret_class or edict.touch == 'secret_touch()' then
			info = info and info .. ', secret' or 'secret'
		end

		info = info and '(' .. info .. ')' or ''

		if choice <= 0 then
			print(current .. ':', pos, info)
		elseif choice == current then
			player.setpos(pos)
			return nil
		end

		return current + 1
	end

	return current
end

function console.doors(choice)
	edicts.foreach(handledoor, choice)
end


--
-- Items
--

local function handleitem(edict, current, choice)
	local classname = edict.classname

	if classname:find('item_') == 1 then
		if choice <= 0 then
			local name = edict.netname

			if name == '' then
				-- use classname with prefix removed for entity without netname
				name = classname:sub(6)
			end

			if name == 'armor1' then
				name = 'Green Armor'
			elseif name == 'armor2' then
				name = 'Yellow Armor'
			elseif name == 'armorInv' then
				name = 'Red Armor'
			end

			name = titlecase(name)

			local healamount = edict.healamount
			if healamount ~= 0 then
				name = string.format('%i %s', healamount, name)
			end

			local aflag = edict.aflag
			if aflag ~= 0 then
				name = string.format('%i %s', aflag, name)
			end

			print(current .. ':', name, 'at', edict.origin)
		elseif choice == current then
			player.setpos(edict.origin)
			return nil
		end

		return current + 1
	end

	return current
end

function console.items(choice)
	edicts.foreach(handleitem, choice)
end


---
---
---

function console.lookat()
	local edict = player.traceentity()

	if edict then
		for _, field in ipairs(edict) do
			print(field.name .. ':', field.value)
		end
	end
end

function console.lookatrefs(choice)
	local edict = player.traceentity()

	if not edict then
		return
	end

	local centerpos = vec3.mid(edict.absmin, edict.absmax)
	choice = choice and math.tointeger(choice) or 0

	if choice == 1 then
		player.setpos(centerpos)
		return
	end

	local target = edict.target
	-- print('target', target)
	local targetname = edict.targetname
	-- print('targetname', targetname)

	-- print(edict)

	-- if target == '' and targetname == '' then
	-- 	return
	-- end

	local referencedby = {}
	local references = {}

	local function collectrefs(probe)
		-- if probe == edict then
		-- 	return 1
		-- end

		if target ~= '' and target == probe.targetname then
			-- print('target hit', target)
			references[#references + 1] = probe
		end

		if targetname ~= '' and targetname == probe.target then
			-- print('targetname hit', target)
			referencedby[#references + 1] = probe
		end

		return 1
	end

	edicts.foreach(collectrefs)

	local refbycount = #referencedby
	local refscount = #references
	local count = 1 + refbycount + refscount
	
	if choice > 1 and choice <= count then
		if choice <= refbycount + 1 then
			local ref = referencedby[choice - 1]
			centerpos = vec3.mid(ref.absmin, ref.absmax)
			player.setpos(centerpos)
		else 
			local ref = references[choice - refbycount - 1]
			centerpos = vec3.mid(ref.absmin, ref.absmax)
			player.setpos(centerpos)
		end
		return
	end
	
	print('Look-at entity')
	print('1:', edict.classname, 'at', centerpos)
	
	local index = 2

	if refbycount > 0 then
		print('Referenced by')

		for _, edict in ipairs(referencedby) do
			centerpos = vec3.mid(edict.absmin, edict.absmax)
			print(index .. ':', edict.classname, 'at', centerpos)
			index = index + 1
		end
	end
	
	if refscount > 0 then
		print('References')

		for _, edict in ipairs(references) do
			centerpos = vec3.mid(edict.absmin, edict.absmax)
			print(index .. ':', edict.classname, 'at', centerpos)
			index = index + 1
		end
	end
end