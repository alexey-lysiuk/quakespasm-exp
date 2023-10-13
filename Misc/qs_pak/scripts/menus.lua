
function console.menu_test()
	menu = 
	{
		drawfunc = function(menu)
			
		end,
		
		keyfunc = function(menu, key)
			
		end,
	}

	print('drawfunc =', menu.drawfunc)
	print('keyfunc =', menu.keyfunc)

	menusys.push(menu)
end
