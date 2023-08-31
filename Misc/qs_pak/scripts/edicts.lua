
--
-- Secrets
--

local function secretpos(edict)
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

local function handlesecret(edict, current, choice)
	if edict.classname == 'trigger_secret' then
		if choice <= 0 then
			print(current .. ':', secretpos(edict))
		elseif choice == current then
			player.setpos(secretpos(edict))
			return nil
		end

		return current + 1
	end

	return current
end

-- > lua dofile('scripts/edicts.lua') secrets()

function secrets(choice)
	edicts:foreach(handlesecret, choice)
end
