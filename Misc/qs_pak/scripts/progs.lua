
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


--
-- Mod detection
--

progs.mods =
{
	ID1                = 0,  -- Vanilla Quake
	HIPNOTIC           = 1,  -- Scourge of Armagon
	ROGUE              = 2,  -- Dissolution of Eternity

	-- Re-release
	ID1_RE             = 10,
	HIPNOTIC_RE        = 11,
	ROGUE_RE           = 12,
	MG1                = 13,

	-- Notable mods
	ALKALINE           = 20,
	ARCANE_DIMENSIONS  = 21,
	COPPER             = 22,
	PROGS_DUMP         = 23,
	QUOTH              = 24,
	REMOBILIZE         = 25,
	SPEED_MAPPING      = 26,  -- SMP
}

function progs.detectmod()
	-- TODO: ...
	return progs.mods.ID1
end
