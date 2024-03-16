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
	/* 46 */ "\"origin\" \"0 0 -16"
	/* 63 */ "\"origin\" \"0 -1 0"
	/* 79 */ "\"origin\" \"-1 0 0"
	/* 95 */ "\"origin\" \"0 1 "
	/* 109 */ "\"t_length\" \"65"
	/* 123 */ "\"origin\" \"-1 0 "
	/* 138 */ "\"origin\" \"0 0 -1"
	/* 154 */ "spawnflags\" \"256"
	/* 170 */ "1\"\n\"origin\" \"776 1240 -136"
	/* 196 */ "40"
	/* 198 */ "616 14"
	/* 204 */ "61"
	/* 206 */ "696 153"
	/* 213 */ "48"
	/* 215 */ "16"
	/* 217 */ "sm74_necros"
	/* 228 */ "\"angle\" \"-1\"\n\"lip\" \"64"
	/* 250 */ "56"
	/* 252 */ "56\"\n\"model\" \"*29\"\n}\n{\n\"speed\" \"256"
	/* 286 */ "56\"\n\"model\" \"*31\"\n}\n{\n\"speed\" \"256"
	/* 320 */ "2"
	/* 321 */ "\"origin\" \"0 0 "
	/* 335 */ "04 1096"
	/* 342 */ "\"origin\" \"-1 -1 0\"\n}\n{"
	/* 364 */ "\"origin\" \"0 1 0"
	/* 379 */ " 243"
	/* 383 */ "\"t_length\" \"85"
	/* 397 */ "\"lip\" \"-16"
	/* 407 */ "\"lip\" \"8"
	/* 415 */ "\"t_length\" \"102"
	/* 430 */ "\"origin\" \"0 0 -"
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
	{ EF_COPY, 20759, 0 },
	{ EF_COPY, 8143, 20776 },
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
	{ EF_COPY, 9535, 0 },
	{ EF_ADD, 17, 46 },
	{ EF_COPY, 1230, 9533 },
	{ EF_ADD, 16, 63 },
	{ EF_COPY, 39, 24574 },
	{ EF_COPY, 75, 10800 },
	{ EF_ADD, 16, 79 },
	{ EF_COPY, 39, 24574 },
	{ EF_COPY, 95, 10912 },
	{ EF_ADD, 14, 95 },
	{ EF_COPY, 45, 26560 },
	{ EF_COPY, 4971, 11049 },
	{ EF_ADD, 13, 32 },
	{ EF_COPY, 33, 4688 },
	{ EF_COPY, 62, 16056 },
	{ EF_ADD, 14, 109 },
	{ EF_COPY, 37, 29536 },
	{ EF_COPY, 8530, 16153 },
	{ EF_ADD, 17, 46 },
	{ EF_COPY, 1204, 24681 },
	{ EF_ADD, 14, 109 },
	{ EF_COPY, 38, 8368 },
	{ EF_COPY, 381, 25921 },
	{ EF_ADD, 14, 109 },
	{ EF_COPY, 9518, 26300 },
	{ EF_ADD, 14, 109 },
	{ EF_COPY, 38, 36800 },
	{ EF_COPY, 2840, 35854 },

	// e2m7@10a8
	{ EF_COPY, 19569, 0 },
	{ EF_ADD, 15, 123 },
	{ EF_COPY, 910, 19566 },
	{ EF_ADD, 16, 138 },
	{ EF_COPY, 117, 20474 },
	{ EF_ADD, 16, 138 },
	{ EF_COPY, 29913, 20589 },

	// eoem7@3ea8
	{ EF_COPY, 37059, 0 },
	{ EF_ADD, 16, 154 },
	{ EF_COPY, 44, 37056 },
	{ EF_COPY, 38, 40511 },
	{ EF_ADD, 26, 170 },
	{ EF_COPY, 32, 40680 },
	{ EF_COPY, 18834, 37068 },

	// hrim_sp1@f054
	{ EF_COPY, 33506, 0 },
	{ EF_COPY, 77691, 33519 },

	// sm100_zwiffle@c897
	{ EF_COPY, 117, 0 },
	{ EF_ADD, 2, 196 },
	{ EF_COPY, 12924, 119 },

	// sm212_naitelveni@a835
	{ EF_COPY, 48917, 0 },
	{ EF_ADD, 6, 198 },
	{ EF_COPY, 36, 72431 },
	{ EF_COPY, 44, 5264 },
	{ EF_ADD, 2, 204 },
	{ EF_COPY, 84, 5310 },
	{ EF_ADD, 7, 206 },
	{ EF_COPY, 99, 43392 },
	{ EF_COPY, 90874, 49195 },

	// sm27_bear@92ba
	{ EF_COPY, 5097, 0 },
	{ EF_ADD, 2, 213 },
	{ EF_COPY, 1502, 5099 },

	// dmc3m8@8411
	{ EF_COPY, 60499, 0 },
	{ EF_COPY, 73, 60379 },
	{ EF_COPY, 22991, 60573 },

	// sm51_fat@dee3
	{ EF_COPY, 2049, 0 },
	{ EF_ADD, 2, 215 },
	{ EF_COPY, 3097, 2051 },

	// sm74_necros@29a9
	{ EF_COPY, 5638, 0 },
	{ EF_ADD, 11, 217 },
	{ EF_COPY, 2905, 5649 },

	// sm98_zwiffle@35f8
	{ EF_COPY, 198, 0 },
	{ EF_ADD, 22, 228 },
	{ EF_COPY, 47, 196 },
	{ EF_COPY, 2210, 244 },
	{ EF_COPY, 35, 4992 },
	{ EF_COPY, 7521, 2510 },
	{ EF_COPY, 38, 2454 },
	{ EF_COPY, 2554, 10048 },

	// sop1@e37d
	{ EF_COPY, 16331, 0 },
	{ EF_COPY, 27668, 16344 },

	// sm58_rpg1@e76b
	{ EF_COPY, 2480, 0 },
	{ EF_ADD, 2, 250 },
	{ EF_COPY, 553, 2483 },
	{ EF_ADD, 2, 250 },
	{ EF_COPY, 44, 4480 },
	{ EF_COPY, 1074, 3083 },
	{ EF_ADD, 2, 250 },
	{ EF_COPY, 172, 4160 },
	{ EF_ADD, 2, 250 },
	{ EF_COPY, 44, 4480 },
	{ EF_COPY, 98, 4379 },
	{ EF_ADD, 2, 250 },
	{ EF_COPY, 114, 4480 },
	{ EF_ADD, 2, 250 },
	{ EF_COPY, 73, 4160 },
	{ EF_COPY, 99, 4670 },
	{ EF_ADD, 2, 250 },
	{ EF_COPY, 75, 4480 },
	{ EF_COPY, 678, 4847 },
	{ EF_ADD, 2, 250 },
	{ EF_COPY, 72, 6140 },
	{ EF_COPY, 198, 5600 },
	{ EF_ADD, 34, 252 },
	{ EF_COPY, 133, 6140 },
	{ EF_COPY, 137, 5967 },
	{ EF_ADD, 34, 286 },
	{ EF_COPY, 5809, 6140 },

	// e1m5@fc76
	{ EF_COPY, 1694, 0 },
	{ EF_ADD, 16, 138 },
	{ EF_COPY, 33, 27422 },
	{ EF_COPY, 39654, 1725 },

	// e2m1@3789
	{ EF_COPY, 21390, 0 },
	{ EF_ADD, 16, 63 },
	{ EF_COPY, 2703, 21388 },
	{ EF_ADD, 16, 138 },
	{ EF_COPY, 61, 24027 },
	{ EF_ADD, 1, 320 },
	{ EF_COPY, 61, 24027 },
	{ EF_COPY, 4325, 24212 },
	{ EF_ADD, 14, 321 },
	{ EF_COPY, 33, 22650 },
	{ EF_ADD, 7, 335 },
	{ EF_COPY, 38, 7392 },
	{ EF_COPY, 1432, 28611 },
	{ EF_ADD, 22, 342 },
	{ EF_COPY, 32, 19312 },
	{ EF_COPY, 3823, 30078 },

	// e2m4@2f38
	{ EF_COPY, 17358, 0 },
	{ EF_ADD, 15, 364 },
	{ EF_COPY, 14269, 17356 },
	{ EF_COPY, 10710, 32405 },
	{ EF_COPY, 335, 43191 },
	{ EF_COPY, 61, 43711 },
	{ EF_COPY, 185, 43689 },

	// e2m5@691a
	{ EF_COPY, 39898, 0 },
	{ EF_ADD, 14, 109 },
	{ EF_COPY, 33, 24138 },
	{ EF_ADD, 4, 379 },
	{ EF_COPY, 32, 8016 },
	{ EF_COPY, 3007, 39965 },
	{ EF_ADD, 14, 383 },
	{ EF_COPY, 1443, 42970 },

	// e3m2@e4a8
	{ EF_COPY, 10556, 0 },
	{ EF_ADD, 10, 397 },
	{ EF_COPY, 32, 8974 },
	{ EF_COPY, 279, 10586 },
	{ EF_ADD, 8, 407 },
	{ EF_COPY, 2187, 10863 },
	{ EF_ADD, 1, 320 },
	{ EF_COPY, 110, 13051 },
	{ EF_ADD, 1, 320 },
	{ EF_COPY, 1713, 13162 },
	{ EF_ADD, 15, 415 },
	{ EF_COPY, 37, 24880 },
	{ EF_COPY, 13051, 14910 },

	// e3m3@36e2
	{ EF_COPY, 1494, 0 },
	{ EF_ADD, 16, 138 },
	{ EF_COPY, 35, 19310 },
	{ EF_COPY, 2436, 1527 },
	{ EF_ADD, 15, 430 },
	{ EF_COPY, 102, 3960 },
	{ EF_ADD, 16, 138 },
	{ EF_COPY, 934, 4060 },
	{ EF_ADD, 16, 138 },
	{ EF_COPY, 132, 4992 },
	{ EF_ADD, 16, 138 },
	{ EF_COPY, 62, 4992 },
	{ EF_COPY, 68, 5184 },
	{ EF_ADD, 16, 138 },
	{ EF_COPY, 26370, 5250 },
};

static constexpr EF_Fix ef_fixes[] =
{
	{ "e1m2", 0x0caa, 41179, 41179, 13, 5 },
	{ "e2m7", 0x10a8, 50503, 50557, 61, 7 },
	{ "coe1", 0x175a, 53514, 53501, 7, 2 },
	{ "e2m3", 0x237a, 38695, 38860, 33, 28 },
	{ "sm74_necros", 0x29a9, 8555, 8555, 98, 3 },
	{ "e2m4", 0x2f38, 43875, 42934, 158, 7 },
	{ "bbelief6", 0x324b, 64996, 64996, 0, 3 },
	{ "sm98_zwiffle", 0x35f8, 12603, 12626, 101, 8 },
	{ "e3m3", 0x36e2, 31621, 31729, 186, 15 },
	{ "e2m1", 0x3789, 33902, 33975, 142, 16 },
	{ "eoem7", 0x3ea8, 55903, 56050, 68, 7 },
	{ "e2m5", 0x691a, 44414, 44446, 165, 8 },
	{ "dmc3m8", 0x8411, 83565, 83564, 92, 3 },
	{ "sm27_bear", 0x92ba, 6602, 6602, 89, 3 },
	{ "e1m4", 0x958e, 43622, 43637, 18, 7 },
	{ "sm212_naitelveni", 0xa835, 140070, 140070, 80, 9 },
	{ "e1m1", 0xc49d, 26284, 26294, 9, 4 },
	{ "sm100_zwiffle", 0xc897, 13044, 13044, 77, 3 },
	{ "sm51_fat", 0xdee3, 5149, 5149, 95, 3 },
	{ "sop1", 0xe37d, 44013, 44000, 109, 2 },
	{ "e3m2", 0xe4a8, 27962, 28001, 173, 13 },
	{ "blitz1000", 0xe5bb, 22368, 22368, 3, 4 },
	{ "sm58_rpg1", 0xe76b, 11950, 11938, 111, 27 },
	{ "hrim_sp1", 0xf054, 111211, 111198, 75, 2 },
	{ "e2m2", 0xfbfe, 27003, 27013, 25, 8 },
	{ "e1m5", 0xfc76, 41380, 41398, 138, 4 },
};
