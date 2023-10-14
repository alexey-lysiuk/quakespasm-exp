
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

edicts.solidstates = 
{
	SOLID_NOT         = 0,  -- no interaction with other objects
	SOLID_TRIGGER     = 1,  -- touch on edge, but not blocking
	SOLID_BBOX        = 2,  -- touch on edge, block
	SOLID_SLIDEBOX    = 3,  -- touch on edge, but not an onground
	SOLID_BSP         = 4,  -- bsp clip, touch on edge, block
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


local vec3origin <const> = vec3.new()
local vec3one <const> = vec3.new(1, 1, 1)
local vec3minusone <const> = vec3.new(-1, -1, -1)

local FL_MONSTER <const> = edicts.flags.FL_MONSTER
local SOLID_NOT <const> = edicts.solidstates.SOLID_NOT
local SUPER_SECRET <const> = edicts.spawnflags.SUPER_SECRET

local foreach <const> = edicts.foreach
local isclass <const> = edicts.isclass
local getname <const> = edicts.getname


local function titlecase(str)
	return str:gsub("(%a)([%w_']*)", 
		function(head, tail) 
			return head:upper()..tail:lower() 
		end)
end


--
-- Secrets
--

function edicts.issecret(edict)
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

	local supersecret = edict.spawnflags & SUPER_SECRET ~= 0
	local extra = supersecret and 'supersecret' or ''
	return pos, extra
end

local function handlesecret(edict, current, choice)
	local pos, extra = edicts.issecret(edict)

	if not pos then
		return current
	end

	if choice <= 0 then
		local supersecret = edict.spawnflags & SUPER_SECRET ~= 0
		local extra = supersecret and '(super)' or ''
		print(current .. ':', pos, extra)
	elseif choice == current then
		player.setpos(pos)
		return
	end

	return current + 1
end

function console.secrets(choice)
	foreach(handlesecret, choice)
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

		if not ismonster then
			local classname = edict.classname

			if classname == 'monster_boss' then
				-- Chthon
				ismonster = true
				isalive = health >= 0
			elseif classname == 'monster_oldone' then
				-- Shub-Niggurath
				ismonster = true
			end
		end

		if not ismonster or not isalive then
			return current
		end

		if choice <= 0 then
			local classname = edict.classname

			-- Remove classname prefix if present
			if classname:find('monster_') == 1 then
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
	foreach(handlemonster, choice)
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
	foreach(handleteleport, choice)
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
	local classname = isclass(edict, 'door', 'func_door', door_secret_class)
	
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
	foreach(handledoor, choice)
end


--
-- Items
--

local function handleitem(edict, current, choice)
	if edict.solid == SOLID_NOT then
		-- Skip object if it's not interactible, e.g. if it's a picked up item
		return current
	end

	local classname = edict.classname
	local prefixes = { 'item_', 'weapon_' }
	local prefixlen

	for _, prefix in ipairs(prefixes) do
		if classname:find(prefix) == 1 then
			prefixlen = prefix:len() + 1
		end
	end

	if prefixlen then
		if choice <= 0 then
			local name = edict.netname

			if name == '' then
				-- use classname with prefix removed for entity without netname
				name = classname:sub(prefixlen)
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
	foreach(handleitem, choice)
end


---
--- Gaze, entity player is looking at
---

function console.gaze()
	local edict = player.traceentity()

	if not edict then
		return
	end

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
	local fieldformat = '%s%-' .. maxlen .. 's : %s'

	for _, field in ipairs(fields) do
		local name = field.name
		local tint = ''

		if name == 'target' or name == 'targetname' then
			tint = '\2'
		end

		print(string.format(fieldformat, tint, name, field.value))
	end
end

function console.gazerefs(choice)
	local edict = player.traceentity()

	if not edict then
		return
	end

	choice = choice and math.tointeger(choice) or 0
	local pos = vec3.mid(edict.absmin, edict.absmax)

	if choice == 1 then
		player.setpos(pos)
		return
	end

	local target = edict.target
	local targetname = edict.targetname
	local referencedby = {}
	local references = {}

	local function collectrefs(probe)
		if target ~= '' and target == probe.targetname then
			references[#references + 1] = probe
		end

		if targetname ~= '' and targetname == probe.target then
			referencedby[#referencedby + 1] = probe
		end

		return 1
	end

	foreach(collectrefs)

	local refbycount = #referencedby
	local count = 1 + refbycount + #references

	if choice > 1 and choice <= count then
		-- skip gazed entity
		choice = choice - 1
		local reflist

		if choice > refbycount then
			reflist = references

			-- skip referenced-by entities
			choice = choice - refbycount
		else
			reflist = referencedby
		end

		edict = reflist[choice]
		pos = vec3.mid(edict.absmin, edict.absmax)
		player.setpos(pos)
	else
		print('\2Gazed entity')
		print('1:', getname(edict), 'at', pos)

		local index = 2

		local function printrefs(header, refs)
			if #refs == 0 then
				return
			end

			print(header)

			for _, edict in ipairs(refs) do
				pos = vec3.mid(edict.absmin, edict.absmax)
				print(index .. ':', getname(edict), 'at', pos)

				index = index + 1
			end
		end

		printrefs('\2Referenced by', referencedby)
		printrefs('\2References', references)
	end
end
