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
	/* 0 */ "4"
	/* 1 */ "808 44"
	/* 7 */ "spawnflags\" \"256"
	/* 23 */ "1\"\n\"origin\" \"776 1240 -136"
	/* 49 */ "40"
	/* 51 */ "616 14"
	/* 57 */ "61"
	/* 59 */ "696 153"
	/* 66 */ "48"
	/* 68 */ "16"
	/* 70 */ "56"
	/* 72 */ "56\"\n\"model\" \"*29\"\n}\n{\n\"speed\" \"256"
	/* 106 */ "56\"\n\"model\" \"*31\"\n}\n{\n\"speed\" \"256"
	/* 140 */ "sm74_necros"
	/* 151 */ "\"angle\" \"-1\"\n\"lip\" \"64"
	/* 173 */ "20"
	/* 175 */ "736 -2656"
	/* 184 */ "1"
	/* 185 */ "\"target"
;

static constexpr EF_Patch ef_patches[] =
{
	// bbelief6@324b
	{ EF_COPY, 51607, 0 },
	{ EF_ADD, 1, 0 },
	{ EF_COPY, 13387, 51608 },

	// blitz1000@e5bb
	{ EF_COPY, 18888, 0 },
	{ EF_ADD, 6, 1 },
	{ EF_COPY, 44, 3808 },
	{ EF_COPY, 3429, 18938 },

	// coe1@175a
	{ EF_COPY, 20648, 0 },
	{ EF_COPY, 32852, 20661 },

	// dmc3m8@8411
	{ EF_COPY, 60499, 0 },
	{ EF_COPY, 73, 60379 },
	{ EF_COPY, 22991, 60573 },

	// e1m4@958e
	{ EF_COPY, 20759, 0 },
	{ EF_COPY, 22845, 20776 },

	// e2m4@2f38
	{ EF_COPY, 31625, 0 },
	{ EF_COPY, 10710, 32405 },
	{ EF_COPY, 335, 43191 },
	{ EF_COPY, 61, 43711 },
	{ EF_COPY, 185, 43689 },

	// eoem7@3ea8
	{ EF_COPY, 37059, 0 },
	{ EF_ADD, 16, 7 },
	{ EF_COPY, 44, 37056 },
	{ EF_COPY, 38, 40511 },
	{ EF_ADD, 26, 23 },
	{ EF_COPY, 32, 40680 },
	{ EF_COPY, 18834, 37068 },

	// hrim_sp1@f054
	{ EF_COPY, 33506, 0 },
	{ EF_COPY, 77691, 33519 },

	// sm100_zwiffle@c897
	{ EF_COPY, 117, 0 },
	{ EF_ADD, 2, 49 },
	{ EF_COPY, 12924, 119 },

	// sm212_naitelveni@a835
	{ EF_COPY, 48917, 0 },
	{ EF_ADD, 6, 51 },
	{ EF_COPY, 36, 72431 },
	{ EF_COPY, 44, 5264 },
	{ EF_ADD, 2, 57 },
	{ EF_COPY, 84, 5310 },
	{ EF_ADD, 7, 59 },
	{ EF_COPY, 99, 43392 },
	{ EF_COPY, 90874, 49195 },

	// sm27_bear@92ba
	{ EF_COPY, 5097, 0 },
	{ EF_ADD, 2, 66 },
	{ EF_COPY, 1502, 5099 },

	// sm51_fat@dee3
	{ EF_COPY, 2049, 0 },
	{ EF_ADD, 2, 68 },
	{ EF_COPY, 3097, 2051 },

	// sm58_rpg1@e76b
	{ EF_COPY, 2480, 0 },
	{ EF_ADD, 2, 70 },
	{ EF_COPY, 553, 2483 },
	{ EF_ADD, 2, 70 },
	{ EF_COPY, 44, 4480 },
	{ EF_COPY, 1074, 3083 },
	{ EF_ADD, 2, 70 },
	{ EF_COPY, 172, 4160 },
	{ EF_ADD, 2, 70 },
	{ EF_COPY, 44, 4480 },
	{ EF_COPY, 98, 4379 },
	{ EF_ADD, 2, 70 },
	{ EF_COPY, 114, 4480 },
	{ EF_ADD, 2, 70 },
	{ EF_COPY, 73, 4160 },
	{ EF_COPY, 99, 4670 },
	{ EF_ADD, 2, 70 },
	{ EF_COPY, 75, 4480 },
	{ EF_COPY, 678, 4847 },
	{ EF_ADD, 2, 70 },
	{ EF_COPY, 72, 6140 },
	{ EF_COPY, 198, 5600 },
	{ EF_ADD, 34, 72 },
	{ EF_COPY, 133, 6140 },
	{ EF_COPY, 137, 5967 },
	{ EF_ADD, 34, 106 },
	{ EF_COPY, 5809, 6140 },

	// sm74_necros@29a9
	{ EF_COPY, 5638, 0 },
	{ EF_ADD, 11, 140 },
	{ EF_COPY, 2905, 5649 },

	// sm98_zwiffle@35f8
	{ EF_COPY, 198, 0 },
	{ EF_ADD, 22, 151 },
	{ EF_COPY, 47, 196 },
	{ EF_COPY, 2210, 244 },
	{ EF_COPY, 35, 4992 },
	{ EF_COPY, 7521, 2510 },
	{ EF_COPY, 38, 2454 },
	{ EF_COPY, 2554, 10048 },

	// sop1@e37d
	{ EF_COPY, 16331, 0 },
	{ EF_COPY, 27668, 16344 },

	// casana@7cfa
	{ EF_COPY, 6152, 0 },
	{ EF_COPY, 272, 6227 },
	{ EF_ADD, 2, 173 },
	{ EF_COPY, 21344, 6501 },
	{ EF_ADD, 9, 175 },
	{ EF_COPY, 51, 29823 },
	{ EF_ADD, 1, 184 },
	{ EF_COPY, 63, 30768 },
	{ EF_COPY, 5180, 28045 },
	{ EF_COPY, 7381, 33286 },
	{ EF_COPY, 11652, 40727 },
	{ EF_COPY, 2428, 52439 },

	// castlez@1699
	{ EF_COPY, 58808, 0 },
	{ EF_ADD, 7, 185 },
	{ EF_COPY, 36, 58040 },
	{ EF_COPY, 24465, 58833 },
};

static constexpr EF_Fix ef_fixes[] =
{
	{ "castlez", 0x1699, 83299, 83317, 98, 4 },
	{ "coe1", 0x175a, 53514, 53501, 7, 2 },
	{ "sm74_necros", 0x29a9, 8555, 8555, 73, 3 },
	{ "e2m4", 0x2f38, 43875, 42917, 14, 5 },
	{ "bbelief6", 0x324b, 64996, 64996, 0, 3 },
	{ "sm98_zwiffle", 0x35f8, 12603, 12626, 76, 8 },
	{ "eoem7", 0x3ea8, 55903, 56050, 19, 7 },
	{ "casana", 0x7cfa, 54868, 54536, 86, 12 },
	{ "dmc3m8", 0x8411, 83565, 83564, 9, 3 },
	{ "sm27_bear", 0x92ba, 6602, 6602, 40, 3 },
	{ "e1m4", 0x958e, 43622, 43605, 12, 2 },
	{ "sm212_naitelveni", 0xa835, 140070, 140070, 31, 9 },
	{ "sm100_zwiffle", 0xc897, 13044, 13044, 28, 3 },
	{ "sm51_fat", 0xdee3, 5149, 5149, 43, 3 },
	{ "sop1", 0xe37d, 44013, 44000, 84, 2 },
	{ "blitz1000", 0xe5bb, 22368, 22368, 3, 4 },
	{ "sm58_rpg1", 0xe76b, 11950, 11938, 46, 27 },
	{ "hrim_sp1", 0xf054, 111211, 111198, 26, 2 },
};
