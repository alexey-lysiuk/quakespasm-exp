
local format <const> = string.format

local functions <const> = progs.functions
local typename <const> = progs.typename

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

function progs.functionfullname(func)
	local name = func:name()

	if name == '' then
		name = '???'
	end

	local returntype = typename(func:returntype())
	local args = '???'  -- TODO

	return format('%s %s(%s)', returntype, name, args)
end