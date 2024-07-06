
local functions <const> = progs.functions

function progs.func(index_or_name)
	if type(index_or_name) == 'number' then
		local _, func = functions(index_or_name - 1)(nil, index_or_name - 1)
		return func
	end

	for _, func in functions() do
		if func:name() == index_or_name then
			return func
		end
	end
end
