--
-- Secrets
--

local function secretPos(edict)
	-- TODO: ...
	return vec3.mid(edict.absmin, edict.absmax)
end

local function processSecret(edict, current, target)
	if edict.classname == 'trigger_secret' then
		if target <= 0 then
			print(current .. ':', secretPos(edict))
		elseif target == current then
			print('setpos', secretPos(edict))  -- TODO
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
			print('god 1; notarget 1; setpos', edict.origin, edict.angles)  -- TODO
			return nil
		end
	end

	return current + 1
end

-- > lua dofile('scripts/edicts.lua') monsters()

function monsters(target)
	edicts:foreach(processMonster, target)
end
