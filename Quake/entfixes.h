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
	/* 7 */ "\"lip\" \"7"
	/* 15 */ "1"
	/* 16 */ "69"
	/* 18 */ "\"t_length\" \"73"
	/* 32 */ "\"lip\" \"7\"\n}\n{"
	/* 45 */ "5"
	/* 46 */ "\"t_length\" \"65"
	/* 60 */ "\"origin\" \"-1 0 "
	/* 75 */ "\"origin\" \"0 0 -1"
	/* 91 */ "spawnflags\" \"256"
	/* 107 */ "1\"\n\"origin\" \"776 1240 -136"
	/* 133 */ "40"
	/* 135 */ "616 14"
	/* 141 */ "61"
	/* 143 */ "696 153"
	/* 150 */ "48"
	/* 152 */ "16"
	/* 154 */ "sm74_necros"
	/* 165 */ "\"angle\" \"-1\"\n\"lip\" \"64"
	/* 187 */ "\"origin\" \"0 -1 0"
	/* 203 */ "2"
	/* 204 */ "\"origin\" \"0 0 "
	/* 218 */ "04 1096"
	/* 225 */ "\"origin\" \"-1 -1 0\"\n}\n{"
	/* 247 */ "\"origin\" \"0 1 0"
	/* 262 */ " 243"
	/* 266 */ "\"t_length\" \"85"
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

	// e1m1@c49d
	{ EF_COPY, 9099, 0 },
	{ EF_ADD, 8, 7 },
	{ EF_COPY, 48, 18926 },
	{ EF_COPY, 17138, 9145 },

	// e1m2@0caa
	{ EF_COPY, 21484, 0 },
	{ EF_ADD, 1, 15 },
	{ EF_COPY, 99, 21485 },
	{ EF_ADD, 2, 16 },
	{ EF_COPY, 19592, 21586 },

	// e1m4@958e
	{ EF_COPY, 28919, 0 },
	{ EF_ADD, 14, 18 },
	{ EF_COPY, 68, 23837 },
	{ EF_COPY, 679, 28985 },
	{ EF_ADD, 14, 18 },
	{ EF_COPY, 13959, 29662 },

	// e2m2@fbfe
	{ EF_COPY, 12165, 0 },
	{ EF_ADD, 13, 32 },
	{ EF_COPY, 52, 21484 },
	{ EF_COPY, 6935, 12220 },
	{ EF_ADD, 1, 45 },
	{ EF_COPY, 72, 19156 },
	{ EF_ADD, 1, 45 },
	{ EF_COPY, 7773, 19229 },

	// e2m3@237a
	{ EF_COPY, 16020, 0 },
	{ EF_ADD, 13, 32 },
	{ EF_COPY, 33, 4688 },
	{ EF_COPY, 62, 16056 },
	{ EF_ADD, 14, 46 },
	{ EF_COPY, 37, 29536 },
	{ EF_COPY, 9732, 16153 },
	{ EF_ADD, 14, 46 },
	{ EF_COPY, 38, 8368 },
	{ EF_COPY, 381, 25921 },
	{ EF_ADD, 14, 46 },
	{ EF_COPY, 9518, 26300 },
	{ EF_ADD, 14, 46 },
	{ EF_COPY, 38, 36800 },
	{ EF_COPY, 2840, 35854 },

	// e2m7@10a8
	{ EF_COPY, 19569, 0 },
	{ EF_ADD, 15, 60 },
	{ EF_COPY, 910, 19566 },
	{ EF_ADD, 16, 75 },
	{ EF_COPY, 117, 20474 },
	{ EF_ADD, 16, 75 },
	{ EF_COPY, 29913, 20589 },

	// eoem7@3ea8
	{ EF_COPY, 37059, 0 },
	{ EF_ADD, 16, 91 },
	{ EF_COPY, 44, 37056 },
	{ EF_COPY, 38, 40511 },
	{ EF_ADD, 26, 107 },
	{ EF_COPY, 32, 40680 },
	{ EF_COPY, 18834, 37068 },

	// hrim_sp1@f054
	{ EF_COPY, 33506, 0 },
	{ EF_COPY, 77691, 33519 },

	// sm100_zwiffle@c897
	{ EF_COPY, 117, 0 },
	{ EF_ADD, 2, 133 },
	{ EF_COPY, 12924, 119 },

	// sm212_naitelveni@a835
	{ EF_COPY, 48917, 0 },
	{ EF_ADD, 6, 135 },
	{ EF_COPY, 36, 72431 },
	{ EF_COPY, 44, 5264 },
	{ EF_ADD, 2, 141 },
	{ EF_COPY, 84, 5310 },
	{ EF_ADD, 7, 143 },
	{ EF_COPY, 99, 43392 },
	{ EF_COPY, 90874, 49195 },

	// sm27_bear@92ba
	{ EF_COPY, 5097, 0 },
	{ EF_ADD, 2, 150 },
	{ EF_COPY, 1502, 5099 },

	// dmc3m8@8411
	{ EF_COPY, 60499, 0 },
	{ EF_COPY, 73, 60379 },
	{ EF_COPY, 22991, 60573 },

	// sm51_fat@dee3
	{ EF_COPY, 2049, 0 },
	{ EF_ADD, 2, 152 },
	{ EF_COPY, 3097, 2051 },

	// sm74_necros@29a9
	{ EF_COPY, 5638, 0 },
	{ EF_ADD, 11, 154 },
	{ EF_COPY, 2905, 5649 },

	// sm98_zwiffle@35f8
	{ EF_COPY, 198, 0 },
	{ EF_ADD, 22, 165 },
	{ EF_COPY, 47, 196 },
	{ EF_COPY, 2210, 244 },
	{ EF_COPY, 35, 4992 },
	{ EF_COPY, 7521, 2510 },
	{ EF_COPY, 38, 2454 },
	{ EF_COPY, 2554, 10048 },

	// sop1@e37d
	{ EF_COPY, 16331, 0 },
	{ EF_COPY, 27668, 16344 },

	// e2m1@3789
	{ EF_COPY, 21390, 0 },
	{ EF_ADD, 16, 187 },
	{ EF_COPY, 2703, 21388 },
	{ EF_ADD, 16, 75 },
	{ EF_COPY, 61, 24027 },
	{ EF_ADD, 1, 203 },
	{ EF_COPY, 61, 24027 },
	{ EF_COPY, 4325, 24212 },
	{ EF_ADD, 14, 204 },
	{ EF_COPY, 33, 22650 },
	{ EF_ADD, 7, 218 },
	{ EF_COPY, 38, 7392 },
	{ EF_COPY, 1432, 28611 },
	{ EF_ADD, 22, 225 },
	{ EF_COPY, 32, 19312 },
	{ EF_COPY, 3823, 30078 },

	// e2m4@2f38
	{ EF_COPY, 17358, 0 },
	{ EF_ADD, 15, 247 },
	{ EF_COPY, 14269, 17356 },
	{ EF_COPY, 10710, 32405 },
	{ EF_COPY, 335, 43191 },
	{ EF_COPY, 61, 43711 },
	{ EF_COPY, 185, 43689 },

	// e2m5@691a
	{ EF_COPY, 39898, 0 },
	{ EF_ADD, 14, 46 },
	{ EF_COPY, 33, 24138 },
	{ EF_ADD, 4, 262 },
	{ EF_COPY, 32, 8016 },
	{ EF_COPY, 3007, 39965 },
	{ EF_ADD, 14, 266 },
	{ EF_COPY, 1443, 42970 },
};

static constexpr EF_Fix ef_fixes[] =
{
	{ "e1m2", 0x0caa, 41179, 41179, 13, 5 },
	{ "e2m7", 0x10a8, 50503, 50557, 47, 7 },
	{ "coe1", 0x175a, 53514, 53501, 7, 2 },
	{ "e2m3", 0x237a, 38695, 38769, 32, 15 },
	{ "sm74_necros", 0x29a9, 8555, 8555, 84, 3 },
	{ "e2m4", 0x2f38, 43875, 42934, 113, 7 },
	{ "bbelief6", 0x324b, 64996, 64996, 0, 3 },
	{ "sm98_zwiffle", 0x35f8, 12603, 12626, 87, 8 },
	{ "e2m1", 0x3789, 33902, 33975, 97, 16 },
	{ "eoem7", 0x3ea8, 55903, 56050, 54, 7 },
	{ "e2m5", 0x691a, 44414, 44446, 120, 8 },
	{ "dmc3m8", 0x8411, 83565, 83564, 78, 3 },
	{ "sm27_bear", 0x92ba, 6602, 6602, 75, 3 },
	{ "e1m4", 0x958e, 43622, 43654, 18, 6 },
	{ "sm212_naitelveni", 0xa835, 140070, 140070, 66, 9 },
	{ "e1m1", 0xc49d, 26284, 26294, 9, 4 },
	{ "sm100_zwiffle", 0xc897, 13044, 13044, 63, 3 },
	{ "sm51_fat", 0xdee3, 5149, 5149, 81, 3 },
	{ "sop1", 0xe37d, 44013, 44000, 95, 2 },
	{ "blitz1000", 0xe5bb, 22368, 22368, 3, 4 },
	{ "hrim_sp1", 0xf054, 111211, 111198, 61, 2 },
	{ "e2m2", 0xfbfe, 27003, 27013, 24, 8 },
};
