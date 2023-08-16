--
-- Secrets
--

local function secretPos(edict)
	local min = edict.absmin
	local max = edict.absmax
	local count
	
	-- Try to handle Arcane Dimensions secret
	if min == vec3.new(-1, -1, -1) and max == vec3.new(1, 1, 1) then
		count = edict.count
	end
	
	if count then
		if count == 0 then
			-- Revealed Arcane Dimensions secret, skip it
			return nil
		else
			-- Disabled or switched off Arcane Dimensions secret
			-- Actual coodinates are stored in oldorigin member
			return edict.oldorigin
		end
	end

	return vec3.mid(min, max)
end

local function processSecret(edict, current, target)
	if edict.classname == 'trigger_secret' then
		if target <= 0 then
			print(current .. ':', secretPos(edict))
		elseif target == current then
			player.setpos(secretPos(edict))
			return nil
		end

		return current + 1
	end

	return current
end

-- > lua dofile('scripts/edicts.lua') secrets()

function secrets(target)
	edicts:foreach(processSecret, target)
end


--
-- Monsters
--

local FL_MONSTER = 32

local function processMonster(edict, current, target)
	flags = edict.flags
	health = edict.health

	if flags and health then
		ismonster = math.tointeger(flags) & FL_MONSTER == FL_MONSTER
		isalive = health > 0

		if not ismonster or not isalive then
			return current
		end
	
		if target <= 0 then
			print(current .. ':', edict.classname, 'at', edict.origin)
		elseif target == current then
			player.god(true)
			player.notarget(true)
			player.setpos(edict.origin, edict.angles)
			return nil
		end
	end

	return current + 1
end

-- > lua dofile('scripts/edicts.lua') monsters()

function monsters(target)
	edicts:foreach(processMonster, target)
end
