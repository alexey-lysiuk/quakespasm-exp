
progs.types =
{
	bad      = -1,
	void     = 0,
	string   = 1,
	float    = 2,
	vector   = 3,
	entity   = 4,
	field    = 5,
	functn   = 6,
	pointer  = 7,
}


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

local mods <const> = progs.mods

local functions <const> = progs.functions
local strings <const> = progs.strings

local function isrerelease()
	for _, string in ipairs(strings) do
		if string == '$qc_item_health' then
			return true
		end
	end

	return false
end

function progs.detectmod()
	if functions['UpdateCharmerGoal'] then
		return isrerelease() and mods.HIPNOTIC_RE or mods.HIPNOTIC
	elseif functions['EnableComboWeapons'] then
		return isrerelease() and mods.ROGUE_RE or mods.ROGUE
	elseif functions['HordeFindTarget'] then
		return mods.MG1
	elseif functions['BlastBeltCheat'] then
		return mods.ARCANE_DIMENSIONS
	elseif functions['CheckGugAttack'] then
		return mods.QUOTH
	else
		-- TODO: ...
	end

	return isrerelease() and mods.ID1_RE or mods.ID1
end
