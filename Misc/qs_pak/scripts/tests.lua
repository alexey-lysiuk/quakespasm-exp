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
			if page.state == 0 then
				menu.text(10, 10, 'Press any key')
			else
				menu.tintedtext(10, 10, 'Press again to close menu')
			end
		end,

		onkeypress = function(page, keycode)
			if page.state == 0 then
				page.state = page.state + 1
			else
				menu.poppage()
			end
		end,

		state = 0
	}

	menu.pushpage(testpage)
end
