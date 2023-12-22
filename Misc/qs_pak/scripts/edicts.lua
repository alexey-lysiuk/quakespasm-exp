
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

edicts.itemflags =
{
	IT_SHOTGUN          = 1,
	IT_SUPER_SHOTGUN    = 2,
	IT_NAILGUN          = 4,
	IT_SUPER_NAILGUN    = 8,
	IT_GRENADE_LAUNCHER = 16,
	IT_ROCKET_LAUNCHER  = 32,
	IT_LIGHTNING        = 64,
	IT_MJOLNIR          = 128,
	IT_SHELLS           = 256,
	IT_NAILS            = 512,
	IT_ROCKETS          = 1024,
	IT_CELLS            = 2048,
	IT_AXE              = 4096,
	IT_ARMOR1           = 8192,
	IT_ARMOR2           = 16384,
	IT_ARMOR3           = 32768,
	IT_PROXIMITY_GUN    = 65536,
	IT_KEY1             = 131072,
	IT_KEY2             = 262144,
	IT_INVISIBILITY     = 524288,
	IT_INVULNERABILITY  = 1048576,
	IT_SUIT             = 2097152,
	IT_QUAD             = 4194304,
	IT_LASER_CANNON     = 8388608,
}

local itemflags <const> = edicts.itemflags

edicts.itemnames =
{
	[itemflags.IT_SHOTGUN]          = 'Shotgun',
	[itemflags.IT_SUPER_SHOTGUN]    = 'Double-Barrelled Shotgun',
	[itemflags.IT_NAILGUN]          = 'Nailgun',
	[itemflags.IT_SUPER_NAILGUN]    = 'Super Nailgun',
	[itemflags.IT_GRENADE_LAUNCHER] = 'Grenade Launcher',
	[itemflags.IT_ROCKET_LAUNCHER]  = 'Rocket Launcher',
	[itemflags.IT_LIGHTNING]        = 'Thunderbolt',
	[itemflags.IT_MJOLNIR]          = 'Mjolnir',
	[itemflags.IT_SHELLS]           = 'Shells',
	[itemflags.IT_NAILS]            = 'Nails',
	[itemflags.IT_ROCKETS]          = 'Rockets',
	[itemflags.IT_CELLS]            = 'Cells',
	[itemflags.IT_AXE]              = 'Axe',
	[itemflags.IT_ARMOR1]           = 'Green Armor',
	[itemflags.IT_ARMOR2]           = 'Yellow Armor',
	[itemflags.IT_ARMOR3]           = 'Red Armor',
	[itemflags.IT_PROXIMITY_GUN]    = 'Proximity Gun',
	[itemflags.IT_KEY1]             = 'Silver Key',
	[itemflags.IT_KEY2]             = 'Gold Key',
	[itemflags.IT_INVISIBILITY]     = 'Ring of Shadows',
	[itemflags.IT_INVULNERABILITY]  = 'Pentagram of Protection',
	[itemflags.IT_SUIT]             = 'Biosuit',
	[itemflags.IT_QUAD]             = 'Quad Damage',
	[itemflags.IT_LASER_CANNON]     = 'Laser Cannon',
}

local itemnames <const> = edicts.itemnames


edicts.valuetypes =
{
	bad      = -1,
	void     = 0,
	string   = 1,
	float    = 2,
	vector   = 3,
	entity   = 4,
	field    = 5,
	functn   = 6,
	pointer  = 7,
}


function player.safemove(location, angles)
	player.god(true)
	player.notarget(true)
	player.setpos(location, angles)
end


function edicts.foreach(func, choice)
	choice = choice and math.tointeger(choice) or 0
	local current = 1

	for _, edict in ipairs(edicts) do
		current = func(edict, current, choice)

		if not current then
			break
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


local format <const> = string.format

local localize <const> = text.localize

local vec3origin <const> = vec3.new()
local vec3one <const> = vec3.new(1, 1, 1)
local vec3minusone <const> = vec3.new(-1, -1, -1)
local vec3mid <const> = vec3.mid

local FL_MONSTER <const> = edicts.flags.FL_MONSTER
local SOLID_NOT <const> = edicts.solidstates.SOLID_NOT
local SUPER_SECRET <const> = edicts.spawnflags.SUPER_SECRET
local float <const> = edicts.valuetypes.float

local foreach <const> = edicts.foreach
local isclass <const> = edicts.isclass
local isfree <const> = edicts.isfree
local getname <const> = edicts.getname


local function titlecase(str)
	return str:gsub("(%a)([%w_']*)",
		function(head, tail)
			return head:upper()..tail:lower()
		end)
end

local function localizednetname(edict)
	local name = edict.netname

	if name and name ~= '' then
		name = localize(name)

		if name:find('the ', 1, true) == 1 then
			name = name:sub(5)
		end

		name = titlecase(name)
	else
		name = itemnames[edict.items]
	end

	return name
end


--
-- Secrets
--

function edicts.issecret(edict)
	if not edict or isfree(edict) or edict.classname ~= 'trigger_secret' then
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
		local origin = edict.origin
		location = origin == vec3origin and vec3mid(min, max) or origin
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


--
-- Monsters
--

function edicts.ismonster(edict)
	if not edict or isfree(edict) then
		return
	end

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

	-- Check flag specific to Arcane Dimensions
	local nomonstercount = edict.nomonstercount

	if nomonstercount and nomonstercount ~= 0 then
		return
	end

	-- Remove classname prefix if present
	if classname:find('monster_', 1, true) == 1 then
		classname = classname:sub(9)
	end

	return classname, edict.origin, edict.angles
end


--
-- Teleports
--

function edicts.isteleport(edict)
	if not edict or isfree(edict) or edict.classname ~= 'trigger_teleport' then
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

	local description = format('Teleport to %s (%s)', target, targetlocation or 'target not found')
	local location = vec3mid(edict.absmin, edict.absmax)

	return description, location
end


--
-- Doors
--

local function getitemname(item)
	if not item or item == 0 then
		return
	end

	for _, edict in ipairs(edicts) do
		if not isfree(edict) and edict.items == item and edict.classname:find('item_', 1, true) == 1 then
			return localizednetname(edict) or '???'
		end
	end

	return
end

function edicts.isdoor(edict)
	if not edict or isfree(edict) then
		return
	end

	local door_secret_class = 'func_door_secret'
	local classname = isclass(edict, 'door', 'func_door', door_secret_class)

	if not classname then
		return
	end

	local issecret = classname == door_secret_class or edict.touch == 'secret_touch()'
	local secretprefix = issecret and 'Secret ' or ''

	local itemname = getitemname(edict.items)
	local itemprefix = itemname and itemname .. ' ' or ''

	local description = format('%s%sDoor', secretprefix, itemprefix)
	local location = vec3mid(edict.absmin, edict.absmax)

	return description, location
end


--
-- Items
--

function edicts.isitem(edict)
	if not edict or isfree(edict) then
		return
	end

	if edict.solid == SOLID_NOT then
		-- Skip object if it's not interactible, e.g. if it's a picked up item
		return current
	end

	local classname = edict.classname
	local prefixes = { 'item_', 'weapon_' }
	local prefixlen

	for _, prefix in ipairs(prefixes) do
		if classname:find(prefix, 1, true) == 1 then
			prefixlen = prefix:len() + 1
			break
		end
	end

	local name

	if not prefixlen then
		if edict.model and edict.model:find('backpack', 1, true) then
			name = 'Backpack'
		else
			return
		end
	end

	if not name then
		name = localizednetname(edict)
	end

	if not name then
		-- use classname with prefix removed for entity without netname
		name = classname:sub(prefixlen)
	end

	if name == 'armor1' then
		name = itemnames[itemflags.IT_ARMOR1]
	elseif name == 'armor2' then
		name = itemnames[itemflags.IT_ARMOR2]
	elseif name == 'armorInv' then
		name = itemnames[itemflags.IT_ARMOR3]
	end

	name = titlecase(name)

	-- Health
	local healamount = edict.healamount
	if healamount and healamount ~= 0 then
		name = format('%i %s', healamount, name)
	end

	-- Ammo
	local ammoamount = edict.aflag 
		or classname == 'item_shells' and edict.ammo_shells
		or classname == 'item_spikes' and edict.ammo_nails
		or classname == 'item_rockets' and edict.ammo_rockets
		or classname == 'item_cells' and edict.ammo_cells
	if ammoamount and ammoamount ~= 0 then
		name = format('%i %s', ammoamount, name)
	end

	return name, edict.origin
end


--
-- Buttons
--

function edicts.isbutton(edict)
	if not edict or isfree(edict) or edict.classname ~= 'func_button' then
		return
	end

	local description = (edict.health > 0 and 'Shoot' or 'Touch') .. ' button'
	local location = vec3mid(edict.absmin, edict.absmax)

	return description, location
end


--
-- Level exits
--

function edicts.isexit(edict)
	if not edict or isfree(edict) or edict.classname ~= 'trigger_changelevel' then
		return
	end

	local mapname = edict.map or '???'
	local description = 'Exit to ' .. (mapname == '' and '???' or mapname)
	local location = vec3mid(edict.absmin, edict.absmax)

	return description, location
end


--
-- Edicts with messages
--

function edicts.ismessage(edict)
	if not edict or isfree(edict) then
		return
	end

	local message = edict.message

	if not message or #message == 0 then
		return
	end

	local description = '"' .. message .. '"'
	local location = vec3mid(edict.absmin, edict.absmax)

	return description, location
end


--
-- Edicts console commands
--

local isitem <const> = edicts.isitem

local function handleedict(func, edict, current, choice)
	local description, location, angles = func(edict)

	if not description then
		return current
	end

	if choice <= 0 then
		print(current .. ':', description, 'at', location)
	elseif choice == current then
		if isitem(edict) then
			-- Adjust Z coordinate so player will appear slightly above destination
			location.z = location.z + 20
		end

		player.safemove(location, angles)
		return
	end

	return current + 1
end

local function addedictscommand(name, func)
	console[name] = function(choice)
		foreach(function(...) return handleedict(func, ...) end, choice)
	end
end

addedictscommand('secrets', edicts.issecret)
addedictscommand('monsters', edicts.ismonster)
addedictscommand('teleports', edicts.isteleport)
addedictscommand('doors', edicts.isdoor)
addedictscommand('items', isitem)
addedictscommand('buttons', edicts.isbutton)
addedictscommand('exits', edicts.isexit)
addedictscommand('messages', edicts.ismessage)


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
	local fieldformat = '%-' .. maxlen .. 's: %s'

	for _, field in ipairs(fields) do
		local value = field.type == float and format('%.1f', field.value) or field.value
		print(format(fieldformat, field.name, value))
	end
end

function console.gazerefs(choice)
	local edict = player.traceentity()

	if not edict then
		return
	end

	choice = choice and math.tointeger(choice) or 0
	local pos = vec3mid(edict.absmin, edict.absmax)

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
		pos = vec3mid(edict.absmin, edict.absmax)
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
				pos = vec3mid(edict.absmin, edict.absmax)
				print(index .. ':', getname(edict), 'at', pos)

				index = index + 1
			end
		end

		printrefs('\2Referenced by', referencedby)
		printrefs('\2References', references)
	end
end
