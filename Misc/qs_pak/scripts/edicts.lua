
--
-- Edict flags
--

FL_FLY            = 1
FL_SWIM           = 2
FL_CONVEYOR       = 4
FL_CLIENT         = 8
FL_INWATER        = 16
FL_MONSTER        = 32
FL_GODMODE        = 64
FL_NOTARGET       = 128
FL_ITEM           = 256
FL_ONGROUND       = 512
FL_PARTIALGROUND  = 1024  -- not all corners are valid
FL_WATERJUMP      = 2048  -- player jumping out of water
FL_JUMPRELEASED   = 4096  -- for jump debouncing


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


--
-- Monsters
--

local function handlemonster(edict, current, choice)
	flags = edict.flags
	health = edict.health

	if flags and health then
		ismonster = math.tointeger(flags) & FL_MONSTER == FL_MONSTER
		isalive = health > 0

		if not ismonster or not isalive then
			return current
		end

		if choice <= 0 then
			print(current .. ':', edict.classname, 'at', edict.origin)
		elseif choice == current then
			player.god(true)
			player.notarget(true)
			player.setpos(edict.origin, edict.angles)
			return nil
		end
	end

	return current + 1
end

-- > lua dofile('scripts/edicts.lua') monsters()

function monsters(choice)
	edicts:foreach(handlemonster, choice)
end


--
-- Teleports
--

local function handleteleport(edict, current, choice)
	local vec3origin = vec3.new()

	if edict.classname == 'trigger_teleport' then
		local pos = vec3.mid(edict.absmin, edict.absmax)

		if choice <= 0 then
			local teletarget = edict.target
			local targetpos

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

			if targetpos then
				targetstr = 'at ' .. tostring(targetpos)
			else
				targetstr = '(target not found)'
			end

			print(current .. ':', pos, '->', teletarget, targetstr)
		elseif choice == current then
			player.setpos(pos)
			return nil
		end

		return current + 1
	end

	return current
end

-- > lua dofile('scripts/edicts.lua') teleports()

function teleports(choice)
	edicts:foreach(handleteleport, choice)
end


--
-- References
--

-- local function handlereference(edict, current, choice)
--
-- end

-- > lua dofile('scripts/edicts.lua') references()

function references(target, choice)
	-- edicts:foreach(handlereference, choice)

	if not target or target == '' then
		return
	end

	local edictrefs = {}
	-- local edictrefscount = 1
	--
	-- local function collectreferences(edict)
	-- 	if edict.target or edict.targetname
	-- 		edictrefs[#edictrefs + 1] = edict
	-- 	end
	-- end

	for i, edict in ipairs(edicts) do
		if edict.target ~= '' or edict.targetname ~= '' then
			table.insert(edictrefs, i)
		end
	end

	if #edictrefs == 0 then
		return
	end

	-- edicts:foreach(collectreferences)
	
	for _, i in ipairs(edictrefs) do
		edict = edicts[i]

		-- edtarget = edict.target
		--
		-- if edtarget ~= '' and string.find(edtarget, target) then
		-- 	print(i, edict.classname)
		-- else
		-- 	edtargetname = edict.targetname
		--
		-- 	if edtargetname ~= '' and string.find(edtargetname, target) then
		-- 		print(i, edict.classname)
		-- 	end
		-- end

		if edict.target == target or edict.targetname == target then
			print(i, edict.classname)
		end
	end
end