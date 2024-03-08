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
	/* 75 */ "spawnflags\" \"256"
	/* 91 */ "1\"\n\"origin\" \"776 1240 -136"
	/* 117 */ "40"
	/* 119 */ "616 14"
	/* 125 */ "61"
	/* 127 */ "696 153"
	/* 134 */ "48"
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
	{ EF_COPY, 30936, 19566 },

	// eoem7@3ea8
	{ EF_COPY, 37059, 0 },
	{ EF_ADD, 16, 75 },
	{ EF_COPY, 44, 37056 },
	{ EF_COPY, 38, 40511 },
	{ EF_ADD, 26, 91 },
	{ EF_COPY, 32, 40680 },
	{ EF_COPY, 18834, 37068 },

	// hrim_sp1@f054
	{ EF_COPY, 33506, 0 },
	{ EF_COPY, 77691, 33519 },

	// sm100_zwiffle@c897
	{ EF_COPY, 117, 0 },
	{ EF_ADD, 2, 117 },
	{ EF_COPY, 12924, 119 },

	// sm212_naitelveni@a835
	{ EF_COPY, 48917, 0 },
	{ EF_ADD, 6, 119 },
	{ EF_COPY, 36, 72431 },
	{ EF_COPY, 44, 5264 },
	{ EF_ADD, 2, 125 },
	{ EF_COPY, 84, 5310 },
	{ EF_ADD, 7, 127 },
	{ EF_COPY, 99, 43392 },
	{ EF_COPY, 90874, 49195 },

	// sm27_bear@92ba
	{ EF_COPY, 5097, 0 },
	{ EF_ADD, 2, 134 },
	{ EF_COPY, 1502, 5099 },
};

static constexpr EF_Fix ef_fixes[] =
{
	{ "e1m2", 0x0caa, 41179, 41179, 13, 5 },
	{ "e2m7", 0x10a8, 50503, 50521, 47, 3 },
	{ "coe1", 0x175a, 53514, 53501, 7, 2 },
	{ "e2m3", 0x237a, 38695, 38769, 32, 15 },
	{ "bbelief6", 0x324b, 64996, 64996, 0, 3 },
	{ "eoem7", 0x3ea8, 55903, 56050, 50, 7 },
	{ "sm27_bear", 0x92ba, 6602, 6602, 71, 3 },
	{ "e1m4", 0x958e, 43622, 43654, 18, 6 },
	{ "sm212_naitelveni", 0xa835, 140070, 140070, 62, 9 },
	{ "e1m1", 0xc49d, 26284, 26294, 9, 4 },
	{ "sm100_zwiffle", 0xc897, 13044, 13044, 59, 3 },
	{ "blitz1000", 0xe5bb, 22368, 22368, 3, 4 },
	{ "hrim_sp1", 0xf054, 111211, 111198, 57, 2 },
	{ "e2m2", 0xfbfe, 27003, 27013, 24, 8 },
};
