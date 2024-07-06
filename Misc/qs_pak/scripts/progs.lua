
local functions <const> = progs.functions

function progs.func(index_or_name)
	if type(index_or_name) == 'number' then
		local _, func = functions(index_or_name - 1)(nil, index_or_name - 1)
		return func
	end

	-- TODO: add func.name()
	index_or_name = index_or_name .. '()'

	for _, func in functions() do
		if tostring(func) == index_or_name then
			return func
		end
	end
end
