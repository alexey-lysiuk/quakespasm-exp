---
--- lua dofile('scripts/tests.lua')
---

local function handletarget(edict, current, choice)
	local target = edict.target
	local targetname = edict.targetname

	if target ~= '' and targetname ~= '' then
		if choice <= 0 then
			local pos = vec3.mid(edict.absmin, edict.absmax)
			print(current .. ':', edict.classname, 'at', pos)
			print('  target:', target, 'targetname:', targetname)
		elseif choice == current then
			local pos = vec3.mid(edict.absmin, edict.absmax)
			player.setpos(pos)
			return nil
		end

		return current + 1
	end

	return current
end

function console.targets(choice)
	edicts.foreach(handletarget, choice)
end


local function handlecounter(edict, current)
	if edict.classname == 'trigger_counter' then
		local nomessage = (edict.spawnflags & 1 == 1) and ' (nomessage)' or ''
		local header = string.format('%d: %s\n count: %d%s', current, edict, edict.count, nomessage)
		print(header)

		local target = edict.target

		if target ~= '' then
			print('  target:', target)

			edicts.foreach(function (edict)
				if edict.targetname == target then
					print('  ', edict)
				end
				return 1
			end)
		end

		local targetname = edict.targetname

		if targetname ~= '' then
			print('  targetname:', targetname)

			edicts.foreach(function (edict)
				if edict.target == targetname then
					print('  ', edict)
				end
				return 1
			end)
		end

		return current + 1
	end

	return current
end

function console.counters()
	edicts.foreach(handlecounter)
end
