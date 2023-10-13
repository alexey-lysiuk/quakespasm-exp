
function console.menu_test()
	local testpage =
	{
		ondraw = function(page)
			
		end,

		onkeypress = function(page, key)
			menu.poppage()
		end,
	}

	menu.pushpage(testpage)
end
