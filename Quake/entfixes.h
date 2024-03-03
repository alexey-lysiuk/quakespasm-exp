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

static constexpr const char* addeddata[] =
{
	/* 0 */ "4",
	/* 1 */ "808 44",
	/* 2 */ "\"lip\" \"7\" // svdijk -- added to prevent z-fighting",
	/* 3 */ "1\" // svdijk -- changed to prevent z-fighting (was \"90\")",
	/* 4 */ "69\" // svdijk -- changed to prevent z-fighting (was \"270\")",
	/* 5 */ "\"t_length\" \"73\" // svdijk -- added to prevent z-fighting",
	/* 6 */ "\"lip\" \"7\" // svdijk -- added to prevent z-fighting\n}\n{",
	/* 7 */ "5 280 104\" // svdijk -- changed to prevent z-fighting (was \"-16 280 104\")",
	/* 8 */ "\"t_length\" \"65\" // svdijk -- added to prevent z-fighting",
	/* 9 */ "\"origin\" \"-1 0 0\" // svdijk -- added to prevent z-fighting",
	/* 10 */ "spawnflags\" \"256",
	/* 11 */ "1\"\n\"origin\" \"776 1240 -136",
	/* 12 */ "cellar\"\n\"targetname\" \"bob\"\n}\n",
	/* 13 */ "40",
	/* 14 */ "616 14",
	/* 15 */ "61",
	/* 16 */ "696 153",
	/* 17 */ "48",
	/* 18 */ "16",
	/* 19 */ "sm74_necros",
};

static constexpr EF_Patch ef_patches[] =
{
	// SM51_FAT@dee3
	{ EF_COPY, 2049, 0 },
	{ EF_ADD, 2, 18 },
	{ EF_COPY, 3097, 2051 },

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

	// e1m1@c49d
	{ EF_COPY, 9099, 0 },
	{ EF_ADD, 50, 2 },
	{ EF_COPY, 47, 18927 },
	{ EF_COPY, 17138, 9145 },

	// e1m2@0caa
	{ EF_COPY, 21484, 0 },
	{ EF_ADD, 56, 3 },
	{ EF_COPY, 98, 21486 },
	{ EF_ADD, 58, 4 },
	{ EF_COPY, 19591, 21587 },

	// e1m4@958e
	{ EF_COPY, 28919, 0 },
	{ EF_ADD, 56, 5 },
	{ EF_COPY, 67, 23838 },
	{ EF_COPY, 679, 28985 },
	{ EF_ADD, 56, 5 },
	{ EF_COPY, 13958, 29663 },

	// e2m2@fbfe
	{ EF_COPY, 12165, 0 },
	{ EF_ADD, 54, 6 },
	{ EF_COPY, 52, 21484 },
	{ EF_COPY, 6935, 12220 },
	{ EF_ADD, 73, 7 },
	{ EF_COPY, 63, 19165 },
	{ EF_ADD, 73, 7 },
	{ EF_COPY, 32, 19503 },
	{ EF_COPY, 7732, 19270 },

	// e2m3@237a
	{ EF_COPY, 16020, 0 },
	{ EF_ADD, 54, 6 },
	{ EF_COPY, 33, 4688 },
	{ EF_COPY, 62, 16056 },
	{ EF_ADD, 56, 8 },
	{ EF_COPY, 36, 14784 },
	{ EF_COPY, 9732, 16153 },
	{ EF_ADD, 56, 8 },
	{ EF_COPY, 37, 7312 },
	{ EF_COPY, 381, 25921 },
	{ EF_ADD, 56, 8 },
	{ EF_COPY, 9517, 26301 },
	{ EF_ADD, 56, 8 },
	{ EF_COPY, 47, 28864 },
	{ EF_COPY, 2830, 35864 },

	// e2m7@10a8
	{ EF_COPY, 19569, 0 },
	{ EF_ADD, 58, 9 },
	{ EF_COPY, 30934, 19568 },

	// eoem7@3ea8
	{ EF_COPY, 37059, 0 },
	{ EF_ADD, 16, 10 },
	{ EF_COPY, 18846, 37056 },
	{ EF_COPY, 41, 53518 },
	{ EF_ADD, 26, 11 },
	{ EF_COPY, 32, 40680 },
	{ EF_ADD, 29, 12 },

	// hrim_sp1@f054
	{ EF_COPY, 33506, 0 },
	{ EF_COPY, 77691, 33519 },

	// sm100_zwiffle@c897
	{ EF_COPY, 117, 0 },
	{ EF_ADD, 2, 13 },
	{ EF_COPY, 12924, 119 },

	// sm212_naitelveni@a835
	{ EF_COPY, 48917, 0 },
	{ EF_ADD, 6, 14 },
	{ EF_COPY, 36, 72431 },
	{ EF_COPY, 44, 5264 },
	{ EF_ADD, 2, 15 },
	{ EF_COPY, 84, 5310 },
	{ EF_ADD, 7, 16 },
	{ EF_COPY, 99, 43392 },
	{ EF_COPY, 90874, 49195 },

	// sm27_bear@92ba
	{ EF_COPY, 5097, 0 },
	{ EF_ADD, 2, 17 },
	{ EF_COPY, 1502, 5099 },

	// sm74_necros@29a9
	{ EF_COPY, 5638, 0 },
	{ EF_ADD, 11, 19 },
	{ EF_COPY, 2905, 5649 },

	// sop1@e37d
	{ EF_COPY, 16331, 0 },
	{ EF_COPY, 27668, 16344 },
};

static constexpr EF_Fix ef_fixes[] =
{
	{ "SM51_FAT", 0xdee3, 5148, 5148, 0, 0 },
	{ "bbelief6", 0x324b, 64995, 64995, 3, 3 },
	{ "blitz1000", 0xe5bb, 22367, 22367, 6, 6 },
	{ "coe1", 0x175a, 53513, 53500, 10, 10 },
	{ "dmc3m8", 0x8411, 83564, 83563, 12, 12 },
	{ "e1m1", 0xc49d, 26283, 26334, 15, 15 },
	{ "e1m2", 0x0caa, 41178, 41287, 19, 19 },
	{ "e1m4", 0x958e, 43621, 43735, 24, 24 },
	{ "e2m2", 0xfbfe, 27002, 27179, 30, 30 },
	{ "e2m3", 0x237a, 38694, 38973, 39, 39 },
	{ "e2m7", 0x10a8, 50502, 50561, 54, 54 },
	{ "eoem7", 0x3ea8, 55902, 56049, 57, 57 },
	{ "hrim_sp1", 0xf054, 111210, 111197, 64, 64 },
	{ "sm100_zwiffle", 0xc897, 13043, 13043, 66, 66 },
	{ "sm212_naitelveni", 0xa835, 140069, 140069, 69, 69 },
	{ "sm27_bear", 0x92ba, 6601, 6601, 78, 78 },
	{ "sm74_necros", 0x29a9, 8554, 8554, 81, 81 },
	{ "sop1", 0xe37d, 44012, 43999, 84, 84 },
};
