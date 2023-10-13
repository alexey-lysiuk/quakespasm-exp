
function console.menu_test()
	local testpage =
	{
		drawfunc = function(page)
			
		end,
		
		keyfunc = function(page, key)
			
		end,
	}

	-- print('drawfunc =', testpage.drawfunc)
	-- print('keyfunc =', testpage.keyfunc)

	menu.pushpage(testpage)
end
