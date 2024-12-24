
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
	DRAKE              = 27,
	HONEY              = 28,
}

local mods <const> = progs.mods

local functions <const> = progs.functions
local strings <const> = progs.strings

local function isrerelease()
	return strings['$qc_item_health']
end

local function detectmod()
	if functions['Mutant_Melee'] then
		return mods.ALKALINE
	elseif functions['BlastBeltCheat'] then
		return mods.ARCANE_DIMENSIONS
	elseif functions['TransferKeys'] then
		return mods.COPPER
	elseif functions['RefreshHull'] then
		return functions['CheckGrapple'] and mods.REMOBILIZE or mods.PROGS_DUMP
	elseif functions['CheckGugAttack'] then
		return mods.QUOTH
	elseif functions['GibSoundsRandom'] then
		return mods.SPEED_MAPPING
	elseif functions['SuperGrenade_Launch'] then
		return mods.DRAKE
	elseif functions['trigger_backtohub'] then
		return mods.HONEY
	elseif functions['HordeFindTarget'] then
		return mods.MG1
	elseif functions['EnableComboWeapons'] then
		return isrerelease() and mods.ROGUE_RE or mods.ROGUE
	elseif functions['UpdateCharmerGoal'] then
		return isrerelease() and mods.HIPNOTIC_RE or mods.HIPNOTIC
	end

	return isrerelease() and mods.ID1_RE or mods.ID1
end

local datcrc, functioncount, stringcount, cachedmod

function progs.detectmod(force)
	if force or datcrc ~= progs.datcrc or functioncount ~= #functions or stringcount ~= #strings then
		cachedmod = detectmod()
		datcrc = progs.datcrc
		functioncount = #functions
		stringcount = #strings
	end

	return cachedmod
end

local modnames <const> =
{
	[mods.ID1]                = 'Vanilla Quake',
	[mods.HIPNOTIC]           = 'Scourge of Armagon',
	[mods.ROGUE]              = 'Dissolution of Eternity',
	[mods.ID1_RE]             = 'Vanilla Quake Re-release',
	[mods.HIPNOTIC_RE]        = 'Scourge of Armagon Re-release',
	[mods.ROGUE_RE]           = 'Dissolution of Eternity Re-release',
	[mods.MG1]                = 'Dimension of the Machine',
	[mods.ALKALINE]           = 'Alkaline',
	[mods.ARCANE_DIMENSIONS]  = 'Arcane Dimensions',
	[mods.COPPER]             = 'Copper',
	[mods.PROGS_DUMP]         = 'progs_dump',
	[mods.QUOTH]              = 'Quoth',
	[mods.REMOBILIZE]         = 'Re:Mobilize',
	[mods.SPEED_MAPPING]      = 'Speed Mapping Progs',
	[mods.DRAKE]              = 'Drake',
	[mods.HONEY]              = 'Honey',
}

function progs.modname(mod)
	return modnames[mod] or '???'
end
