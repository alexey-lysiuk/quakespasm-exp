
local functions <const> = progs.functions

function progs.func(index_or_name)
	if type(index_or_name) == 'number' then
		return functions(index_or_name - 1)(nil, index_or_name - 1)
	end

	-- TODO: add func.name()
	index_or_name = index_or_name .. '()'

	for _, func in functions() do
		if tostring(func) == index_or_name then
			return func
		end
	end
end
