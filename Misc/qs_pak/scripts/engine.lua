
---
--- Host helpers
---

function host.levelname()
	local world = edicts[1]
	return world and world.message
end

function host.mapname()
	local world = edicts[1]

	-- Remove leading 'maps/' and trailing '.bsp'
	return world and world.model:sub(6):sub(1, -5)
end
