/*

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

static const char* const addeddata =
{
	/* 0 */ "4"
	/* 1 */ "808 44"
	/* 7 */ "\"lip\" \"7\" // svdijk -- added to prevent z-fighting"
	/* 57 */ "1\" // svdijk -- changed to prevent z-fighting (was \"90\")"
	/* 113 */ "69\" // svdijk -- changed to prevent z-fighting (was \"270\")"
	/* 171 */ "\"t_length\" \"73\" // svdijk -- added to prevent z-fighting"
	/* 227 */ "\"lip\" \"7\" // svdijk -- added to prevent z-fighting\n}\n{"
	/* 281 */ "5 280 104\" // svdijk -- changed to prevent z-fighting (was \"-16 280 104\")"
	/* 354 */ "\"t_length\" \"65\" // svdijk -- added to prevent z-fighting"
	/* 410 */ "\"origin\" \"-1 0 0\" // svdijk -- added to prevent z-fighting"
	/* 468 */ "spawnflags\" \"256"
	/* 484 */ "1\"\n\"origin\" \"776 1240 -136"
	/* 510 */ "cellar\"\n\"targetname\" \"bob\"\n}\n"
	/* 539 */ "40"
	/* 541 */ "616 14"
	/* 547 */ "61"
	/* 549 */ "696 153"
	/* 556 */ "48"
	/* 558 */ "16"
	/* 560 */ "sm74_necros"
};

static constexpr EF_Patch ef_patches[] =
{
	// e1m2@0caa
	{ EF_COPY, 21484, 0 },
	{ EF_ADD, 56, 57 },
	{ EF_COPY, 98, 21486 },
	{ EF_ADD, 58, 113 },
	{ EF_COPY, 19591, 21587 },

	// e2m7@10a8
	{ EF_COPY, 19569, 0 },
	{ EF_ADD, 58, 410 },
	{ EF_COPY, 30934, 19568 },

	// coe1@175a
	{ EF_COPY, 20648, 0 },
	{ EF_COPY, 32852, 20661 },

	// e2m3@237a
	{ EF_COPY, 16020, 0 },
	{ EF_ADD, 54, 227 },
	{ EF_COPY, 33, 4688 },
	{ EF_COPY, 62, 16056 },
	{ EF_ADD, 56, 354 },
	{ EF_COPY, 36, 14784 },
	{ EF_COPY, 9732, 16153 },
	{ EF_ADD, 56, 354 },
	{ EF_COPY, 37, 7312 },
	{ EF_COPY, 381, 25921 },
	{ EF_ADD, 56, 354 },
	{ EF_COPY, 9517, 26301 },
	{ EF_ADD, 56, 354 },
	{ EF_COPY, 47, 28864 },
	{ EF_COPY, 2830, 35864 },

	// sm74_necros@29a9
	{ EF_COPY, 5638, 0 },
	{ EF_ADD, 11, 560 },
	{ EF_COPY, 2905, 5649 },

	// bbelief6@324b
	{ EF_COPY, 51607, 0 },
	{ EF_ADD, 1, 0 },
	{ EF_COPY, 13387, 51608 },

	// eoem7@3ea8
	{ EF_COPY, 37059, 0 },
	{ EF_ADD, 16, 468 },
	{ EF_COPY, 18846, 37056 },
	{ EF_COPY, 41, 53518 },
	{ EF_ADD, 26, 484 },
	{ EF_COPY, 32, 40680 },
	{ EF_ADD, 29, 510 },

	// dmc3m8@8411
	{ EF_COPY, 60499, 0 },
	{ EF_COPY, 73, 60379 },
	{ EF_COPY, 22991, 60573 },

	// sm27_bear@92ba
	{ EF_COPY, 5097, 0 },
	{ EF_ADD, 2, 556 },
	{ EF_COPY, 1502, 5099 },

	// e1m4@958e
	{ EF_COPY, 28919, 0 },
	{ EF_ADD, 56, 171 },
	{ EF_COPY, 67, 23838 },
	{ EF_COPY, 679, 28985 },
	{ EF_ADD, 56, 171 },
	{ EF_COPY, 13958, 29663 },

	// sm212_naitelveni@a835
	{ EF_COPY, 48917, 0 },
	{ EF_ADD, 6, 541 },
	{ EF_COPY, 36, 72431 },
	{ EF_COPY, 44, 5264 },
	{ EF_ADD, 2, 547 },
	{ EF_COPY, 84, 5310 },
	{ EF_ADD, 7, 549 },
	{ EF_COPY, 99, 43392 },
	{ EF_COPY, 90874, 49195 },

	// e1m1@c49d
	{ EF_COPY, 9099, 0 },
	{ EF_ADD, 50, 7 },
	{ EF_COPY, 47, 18927 },
	{ EF_COPY, 17138, 9145 },

	// sm100_zwiffle@c897
	{ EF_COPY, 117, 0 },
	{ EF_ADD, 2, 539 },
	{ EF_COPY, 12924, 119 },

	// sm51_fat@dee3
	{ EF_COPY, 2049, 0 },
	{ EF_ADD, 2, 558 },
	{ EF_COPY, 3097, 2051 },

	// sop1@e37d
	{ EF_COPY, 16331, 0 },
	{ EF_COPY, 27668, 16344 },

	// blitz1000@e5bb
	{ EF_COPY, 18888, 0 },
	{ EF_ADD, 6, 1 },
	{ EF_COPY, 44, 3808 },
	{ EF_COPY, 3429, 18938 },

	// hrim_sp1@f054
	{ EF_COPY, 33506, 0 },
	{ EF_COPY, 77691, 33519 },

	// e2m2@fbfe
	{ EF_COPY, 12165, 0 },
	{ EF_ADD, 54, 227 },
	{ EF_COPY, 52, 21484 },
	{ EF_COPY, 6935, 12220 },
	{ EF_ADD, 73, 281 },
	{ EF_COPY, 63, 19165 },
	{ EF_ADD, 73, 281 },
	{ EF_COPY, 32, 19503 },
	{ EF_COPY, 7732, 19270 },
};

static constexpr EF_Fix ef_fixes[] =
{
	{ "e1m2", 0x0caa, 41179, 41288, 0, 5 },
	{ "e2m7", 0x10a8, 50503, 50562, 5, 3 },
	{ "coe1", 0x175a, 53514, 53501, 8, 2 },
	{ "e2m3", 0x237a, 38695, 38974, 10, 15 },
	{ "sm74_necros", 0x29a9, 8555, 8555, 25, 3 },
	{ "bbelief6", 0x324b, 64996, 64996, 28, 3 },
	{ "eoem7", 0x3ea8, 55903, 56050, 31, 7 },
	{ "dmc3m8", 0x8411, 83565, 83564, 38, 3 },
	{ "sm27_bear", 0x92ba, 6602, 6602, 41, 3 },
	{ "e1m4", 0x958e, 43622, 43736, 44, 6 },
	{ "sm212_naitelveni", 0xa835, 140070, 140070, 50, 9 },
	{ "e1m1", 0xc49d, 26284, 26335, 59, 4 },
	{ "sm100_zwiffle", 0xc897, 13044, 13044, 63, 3 },
	{ "sm51_fat", 0xdee3, 5149, 5149, 66, 3 },
	{ "sop1", 0xe37d, 44013, 44000, 69, 2 },
	{ "blitz1000", 0xe5bb, 22368, 22368, 71, 4 },
	{ "hrim_sp1", 0xf054, 111211, 111198, 75, 2 },
	{ "e2m2", 0xfbfe, 27003, 27180, 77, 9 },
};
