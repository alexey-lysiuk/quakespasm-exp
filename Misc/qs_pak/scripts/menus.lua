
function console.menu_test()
	local testpage =
	{
		ondraw = function(page)
			if page.state == 0 then
				menu.print(10, 10, 'Press any key')
			else
				menu.tintprint(10, 10, 'Press again to close menu')
			end
		end,

		onkeypress = function(page, key)
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
