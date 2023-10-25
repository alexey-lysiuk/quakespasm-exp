
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

local function handleedict(func, edict, current, choice)
	local description, location, angles = func(edict)

	if not description then
		return current
	end

	if choice <= 0 then
		print(current .. ':', description, 'at', location)
	elseif choice == current then
		player.god(true)
		player.notarget(true)
		player.setpos(location, angles)
		return
	end

	return current + 1
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
	local count, location

	if min == vec3minusone and max == vec3one then
		count = edict.count
	end

	if not count then
		-- Regular or Arcane Dimensions secret that was not revealed yet
		location = vec3.mid(min, max)
	elseif count == 0 then
		-- Revealed Arcane Dimensions secret, skip it
		return
	else
		-- Disabled or switched off Arcane Dimensions secret
		-- Actual coodinates are stored in oldorigin member
		location = edict.oldorigin
	end

	local supersecret = edict.spawnflags & SUPER_SECRET ~= 0
	local description = supersecret and 'Supersecret' or 'Secret'

	return description, location
end

local issecret = edicts.issecret

function console.secrets(choice)
	foreach(function(...) return handleedict(issecret, ...) end, choice)
end


--
-- Monsters
--

function edicts.ismonster(edict)
	local flags = edict.flags
	local health = edict.health

	if not flags or not health then
		return
	end

	local ismonster = flags & FL_MONSTER ~= 0
	local isalive = health > 0
	local classname = edict.classname

	if not ismonster then
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
		return
	end

	-- Remove classname prefix if present
	if classname:find('monster_') == 1 then
		classname = classname:sub(9)
	end

	return classname, edict.origin, edict.angles
end

local ismonster = edicts.ismonster

function console.monsters(choice)
	foreach(function(...) return handleedict(ismonster, ...) end, choice)
end


--
-- Teleports
--

function edicts.isteleport(edict)
	if edict.classname ~= 'trigger_teleport' then
		return
	end

	local target = edict.target
	local targetlocation

	if target then
		for _, testedict in ipairs(edicts) do
			if target == testedict.targetname then
				-- Special case for Arcane Dimensions, ad_tears map in particular
				-- It uses own teleport target class (info_teleportinstant_dest) which is disabled by default
				-- Some teleport destinations were missing despite their valid setup
				-- Actual destination coordinates are stored in oldorigin member
				if testedict.origin == vec3origin then
					targetlocation = testedict.oldorigin
				else
					targetlocation = testedict.origin
				end
				break
			end
		end
	end

	local description = string.format('Teleport to %s (%s)', target, targetlocation or 'target not found')
	local location = vec3.mid(edict.absmin, edict.absmax)

	return description, location
end

local isteleport = edicts.isteleport

function console.teleports(choice)
	foreach(function(...) return handleedict(isteleport, ...) end, choice)
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
			return titlecase(edict.netname)
		end
	end

	return nil
end

function edicts.isdoor(edict)
	local door_secret_class = 'func_door_secret'
	local classname = isclass(edict, 'door', 'func_door', door_secret_class)

	if not classname then
		return
	end

	local issecret = classname == door_secret_class or edict.touch == 'secret_touch()'
	local secretprefix = issecret and 'Secret ' or ''

	local itemname = getitemname(edict.items)
	local itemprefix = itemname and itemname .. ' ' or ''

	local description = string.format('%s%sDoor', secretprefix, itemprefix)
	local location = vec3.mid(edict.absmin, edict.absmax)

	return description, location
end

local isdoor = edicts.isdoor

function console.doors(choice)
	foreach(function(...) return handleedict(isdoor, ...) end, choice)
end


--
-- Items
--

function edicts.isitem(edict, current, choice)
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

	if not prefixlen then
		return
	end

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

	-- Health
	local healamount = edict.healamount
	if healamount ~= 0 then
		name = string.format('%i %s', healamount, name)
	end

	-- Ammo
	local aflag = edict.aflag
	if aflag and aflag ~= 0 then
		name = string.format('%i %s', aflag, name)
	end

	return name, edict.origin
end

local isitem = edicts.isitem

function console.items(choice)
	foreach(function(...) return handleedict(isitem, ...) end, choice)
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
