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


function console.menu_test()
	local testpage =
	{
		ondraw = function(page)
			local x = 0
			local y = 0
			local ystep = 10

			menu.tintedtext(x, y, string.format('%i:%i Title / Press ESC to close', x, y))
			y = y + ystep

			menu.text(x, y, string.format('%i:%i ... free space ...', x, y))
			y = y + ystep

			for i = 1, 20 do
				menu.text(x, y, string.format('%i:%i [%i] 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz', x, y, i))
				y = y + ystep
			end
		end,

		onkeypress = function(page, keycode)
			if keycode == keycodes.ESCAPE then
				menu.poppage()
			end
		end,
	}

	menu.pushpage(testpage)
end
