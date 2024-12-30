
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

	ITEM_SECRET       = 128,  -- Honey

	NOT_EASY          = 256,
	NOT_MEDIUM        = 512,
	NOT_HARD          = 1024,
	NOT_DEATHMATCH    = 2048,

	TELEPORT_PLAYER_ONLY = 2,
}

-- Map monster classname to its name
edicts.monsternames =
{
	-- ID1
	monster_army = 'Grunt',
	monster_boss = 'Chthon',
	monster_dragon = 'Dragon',
	monster_demon1 = 'Fiend',
	monster_dog = 'Rottweiler',
	monster_enforcer = 'Enforcer',
	monster_fish = 'Rotfish',
	monster_hell_knight = 'Death Knight',
	monster_knight = 'Knight',
	monster_ogre = 'Ogre',
	monster_oldone = 'Shub-Niggurath',
	monster_shalrath = 'Vore',
	monster_shambler = 'Shambler',
	monster_tarbaby = 'Spawn',
	monster_vomit = 'Vomitus',
	monster_wizard = 'Scrag',
	monster_zombie = 'Zombie',

	-- Hipnotic
	monster_armagon = 'Armagon',
	monster_gremlin = 'Gremlin',
	monster_scourge = 'Centroid',
	trap_spike_mine = 'Spike Mine',

	-- Rogue
	monster_eel = 'Electric Eel',
	monster_lava_man = 'Hephaestus',
	monster_morph = 'Guardian',
	monster_mummy = 'Mummy',
	monster_super_wrath = 'Overlord',
	monster_sword = 'Phantom Swordsman',
	monster_wrath = 'Wrath',
}

local monsternames = edicts.monsternames


local ipairs <const> = ipairs

local floor <const> = math.floor

local format <const> = string.format
local gsub <const> = string.gsub
local sub <const> = string.sub

local concat <const> = table.concat
local insert <const> = table.insert

function edicts.isclass(edict, ...)
	for _, classname in ipairs({...}) do
		if edict.classname == classname then
			return classname
		end
	end

	return nil
end


local detectmod <const> = progs.detectmod
local mods <const> = progs.mods

local localize <const> = text.localize

local vec3new = vec3.new
local vec3origin <const> = vec3new()
local vec3one <const> = vec3new(1, 1, 1)
local vec3minusone <const> = vec3new(-1, -1, -1)
local vec3mid <const> = vec3.mid

local FL_MONSTER <const> = edicts.flags.FL_MONSTER
local SOLID_TRIGGER <const> = edicts.solidstates.SOLID_TRIGGER
local SUPER_SECRET <const> = edicts.spawnflags.SUPER_SECRET
local ITEM_SECRET <const> = edicts.spawnflags.ITEM_SECRET
local TELEPORT_PLAYER_ONLY <const> = edicts.spawnflags.TELEPORT_PLAYER_ONLY

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

	if name == '' then
		return
	end

	name = localize(name)

	if name:find('the ', 1, true) == 1 then
		name = name:sub(5)
	end

	return titlecase(name)
end


function edicts.isany(edict)
	if not edict then
		return
	end

	local description = getname(edict)
	local location, angles

	if not isfree(edict) then
		local absmin = edict.absmin
		local absmax = edict.absmax

		if absmin and absmax then
			location = vec3mid(absmin, absmax)
		else
			location = edict.origin or vec3origin
		end

		angles = edict.angles

		if angles == vec3origin then
			angles = edict.mangle
		end
	end

	return description, location, angles
end


--
-- Secrets
--

local function truesecret(edict)
	local min = edict.absmin
	local max = edict.absmax
	local description, location

	if detectmod() == mods.ARCANE_DIMENSIONS then
		if min == vec3minusone and max == vec3one and edict.count == 0 then
			-- Revealed Arcane Dimensions secret, skip it
			return
		else
			-- Disabled or switched off Arcane Dimensions secret
			-- Actual coodinates are stored in oldorigin member
			location = edict.oldorigin
		end
	end

	if not location then
		local midpoint = vec3mid(min, max)
		location = midpoint == vec3origin and edict.origin or midpoint
	end

	local supersecret = edict.spawnflags & SUPER_SECRET ~= 0
	description = supersecret and 'Supersecret' or 'Secret'

	return description, location
end

local quasisecret_keywords <const> = { 'you', 'found', 'secret' }

local function quasisecret(edict)
	local message = edict.message

	if message == '' then
		return
	end

	local words = {}
	message:lower():gsub("[^%s']+", function(word) words[word] = 0 end)

	local matches = 0

	for _, keyword in ipairs(quasisecret_keywords) do
		if words[keyword] then
			matches = matches + 1
		end
	end

	if matches == #quasisecret_keywords then
		return 'Quasisecret', vec3mid(edict.absmin, edict.absmax)
	end
end

local function ishoneysecret(edict)
	return (detectmod() == mods.HONEY)
		and (edict.spawnflags & ITEM_SECRET ~= 0)
		and (edict.touch == 'itemTouch()')
end

function edicts.issecret(edict)
	if not edict or isfree(edict) then
		return
	elseif edict.classname == 'trigger_secret' then
		return truesecret(edict)
	elseif ishoneysecret(edict) then
		local description, location = edicts.isitem(edict)
		return 'Secret ' .. description, location
	else
		return quasisecret(edict)
	end
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

	if classname == 'item_time_machine' then
		return  -- skip Rogue R2M8 device marked as monster
	end

	-- Check flag specific to Arcane Dimensions
	local nomonstercount = edict.nomonstercount

	if nomonstercount and nomonstercount ~= 0 then
		return
	end

	local name = monsternames[classname]
		-- Remove classname prefix if present
		or titlecase(classname:find('monster_', 1, true) == 1 and classname:sub(9) or classname)

	return name, edict.origin, edict.angles
end


--
-- Teleports
--

function edicts.isteleport(edict)
	if not edict or isfree(edict) or edict.classname ~= 'trigger_teleport' then
		return
	end

	local isad = detectmod() == mods.ARCANE_DIMENSIONS
	local target = edict.target
	local targetlocation

	for _, testedict in ipairs(edicts) do
		if target == testedict.targetname then
			-- Special case for Arcane Dimensions, ad_tears map in particular
			-- It uses own teleport target class (info_teleportinstant_dest) which is disabled by default
			-- Some teleport destinations were missing despite their valid setup
			-- Actual destination coordinates are stored in oldorigin member
			if isad and testedict.origin == vec3origin then
				targetlocation = testedict.oldorigin
			else
				targetlocation = testedict.origin
			end
			break
		end
	end

	local prefix = edict.targetname == '' and 'Touch' or 'Trigger'

	if edict.spawnflags & TELEPORT_PLAYER_ONLY == 0 then
		prefix = prefix .. ' Player'
	end

	local description = format('%s Teleport to %s (%s)', prefix, target, targetlocation or 'target not found')
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

local itemExtraDefitions <const> =
{
	{ 'aflag' },
	{ 'ammo_shells', 'Shells' },
	{ 'ammo_nails', 'Nails' },
	{ 'ammo_rockets', 'Rockets' },
	{ 'ammo_cells', 'Cells' },
	{ 'armorvalue', 'Armor' },
	{ 'healamount', 'Health' },
}

function edicts.isitem(edict)
	if not edict or isfree(edict) then
		return
	end

	local mod = detectmod()
	local isinteractible = edict.solid == SOLID_TRIGGER
		or (mod == mods.HONEY and edict.use == 'item_spawn()')

	if not isinteractible then
		-- Skip object if it's not interactible, e.g. if it's a picked up item
		return
	end

	local classname = edict.classname

	if classname == 'item_gib' and edict.owner then
		-- Skip Arcane Dimensions gibs
		return
	end

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

		if name == 'armor1' then
			name = 'Green Armor'
		elseif name == 'armor2' then
			name = 'Yellow Armor'
		elseif name == 'armorInv' then
			name = 'Red Armor'
		end
	end

	name = titlecase(name)

	local isad = mod == mods.ARCANE_DIMENSIONS
	local extras = {}

	for _, def in ipairs(itemExtraDefitions) do
		local value = edict[def[1]]

		if value and value ~= 0 then
			local defname = def[2]
			local extra

			if isad and value == -1 then
				extra = defname
			elseif defname and defname ~= name then
				extra = format('%i %s', value, defname)
			else
				extra = floor(value)
			end

			insert(extras, extra)
		end
	end

	if #extras > 0 then
		local extrastr = concat(extras, ', ')
		name = format('%s (%s)', name, extrastr)
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

	local description = (edict.health > 0 and 'Shoot' or 'Touch')
		.. (edict.wait > 0 and ' Repeat' or '') .. ' Button'
	local location = vec3mid(edict.absmin, edict.absmax)

	return description, location
end


--
-- Level exits
--

function edicts.isexit(edict)
	if not edict or isfree(edict) then
		return
	end

	local classname = edict.classname
	local isbacktohub = classname == 'trigger_backtohub'  -- Honey
	local isexit = isbacktohub
		or classname == 'trigger_changelevel'
		or edict.touch == 'changelevel_touch()'
		or edict.use == 'trigger_changelevel()'

	if not isexit then
		return
	end

	local map = edict.map
	local mapname = (map and map ~= '') and map or (isbacktohub and 'hub' or '???')
	local location = vec3mid(edict.absmin, edict.absmax)

	return 'Exit to ' .. mapname, location
end


--
-- Edicts with messages
--

function edicts.ismessage(edict)
	if not edict or isfree(edict) then
		return
	end

	local message = edict.message

	if message == '' then
		return
	end

	local description = '"' .. gsub(localize(message), '\n+', ' ') .. '"'
	local location = vec3mid(edict.absmin, edict.absmax)

	return description, location
end


--
-- Edicts with models
--

function edicts.ismodel(edict)
	if not edict or isfree(edict) then
		return
	end

	local model = edict.model

	if not model or #model == 0 then
		return
	end

	local description = sub(model, 1, 1) == '*' and 'Level Model ' .. sub(model, 2) or model
	local location = vec3mid(edict.absmin, edict.absmax)

	return description, location
end


---
--- Massacre (kill all monsters) cheat
---

function edicts.massacre()
	local ismonster <const> = edicts.ismonster
	local destroy <const> = edicts.destroy

	for _, edict in ipairs(edicts) do
		if ismonster(edict) then
			destroy(edict)
		end
	end
end


function edicts.boxsearch(halfedge, origin)
	if not halfedge then
		halfedge = 256
	end

	local edictcount = #edicts

	if edictcount == 0 then
		return {}
	end

	if not origin then
		origin = edicts[2].origin
	end

	local halfedgevec = vec3new(halfedge, halfedge, halfedge)
	local minpos = origin - halfedgevec
	local maxpos = origin + halfedgevec
	local minx, miny, minz = minpos.x, minpos.y, minpos.z
	local maxx, maxy, maxz = maxpos.x, maxpos.y, maxpos.z
	local result = {}

	for i = 3, edictcount do  -- skip worldspawn and player
		local edict = edicts[i]
		local absmin = edict.absmin
		local absmax = edict.absmax

		if not isfree(edict) then
			local hasintersection = minx <= absmax.x and miny <= absmax.y and minz <= absmax.z
				and maxx >= absmin.x and maxy >= absmin.y and maxz >= absmin.z

			if hasintersection then
				insert(result, edict)
			end
		end
	end

	return result
end
