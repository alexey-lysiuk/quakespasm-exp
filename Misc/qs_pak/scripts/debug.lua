
function progs.print()
	print(string.format('Functions (%i)', #progs.functions))

	for _, func in ipairs(progs.functions) do
		print(func.name)
	end

	print()
	print(string.format('Field Definitions (%i)', #progs.fielddefinitions))

	for _, def in ipairs(progs.fielddefinitions) do
		print(def.name)
	end

	print()
	print(string.format('Global Definitions (%i)', #progs.globaldefinitions))

	for _, def in ipairs(progs.globaldefinitions) do
		print(def.name)
	end

	print()
	print(string.format('Strings (%i)', #progs.strings))

	for _, string in ipairs(progs.strings) do
		print(string)
	end
end
