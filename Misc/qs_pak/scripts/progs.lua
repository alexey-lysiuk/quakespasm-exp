
local insert <const> = table.insert
local sort <const> = table.sort

function progs.sourcefiles()
	local fileset = {}

	for _, func in ipairs(progs.functions) do
		fileset[func.filename] = 0
	end

	-- Remove bogus names
	fileset[''] = nil
	fileset['???'] = nil

	local filelist = {}

	for file, _ in pairs(fileset) do
		insert(filelist, file)
	end

	sort(filelist)
	return filelist
end
