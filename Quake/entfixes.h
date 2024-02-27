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

static constexpr EF_Patch ef_patches[] =
{
	// bbelief6@324b
	{ EF_COPY, 0, 51607 },
	{ EF_INSERT, 0, 1, "4" },
	{ EF_COPY, 51608, 13387 },

	// blitz1000@e5bb
	{ EF_COPY, 0, 18888 },
	{ EF_INSERT, 0, 1730, "808 440\"\n}\n{\n\"classname\" \"monster_ogre\"\n\"origin\" \"344 656 432\"\n\"spawnflags\" \"768\"\n\"angle\" \"180\"\n}\n{\n\"classname\" \"monster_dog\"\n\"origin\" \"568 664 440\"\n\"angle\" \"180\"\n}\n{\n\"classname\" \"monster_dog\"\n\"origin\" \"432 664 440\"\n\"angle\" \"180\"\n}\n{\n\"classname\" \"monster_knight\"\n\"origin\" \"704 784 440\"\n\"angle\" \"270\"\n\"spawnflags\" \"768\"\n}\n{\n\"classname\" \"monster_knight\"\n\"origin\" \"504 680 440\"\n\"angle\" \"180\"\n}\n{\n\"classname\" \"monster_knight\"\n\"origin\" \"504 640 440\"\n\"angle\" \"180\"\n}\n{\n\"classname\" \"monster_knight\"\n\"origin\" \"728 720 440\"\n\"angle\" \"270\"\n\"spawnflags\" \"256\"\n}\n{\n\"classname\" \"item_health\"\n\"origin\" \"592 1056 440\"\n}\n{\n\"classname\" \"monster_wizard\"\n\"origin\" \"816 1088 712\"\n\"angle\" \"270\"\n\"spawnflags\" \"256\"\n}\n{\n\"classname\" \"monster_wizard\"\n\"origin\" \"752 1056 712\"\n\"angle\" \"270\"\n}\n{\n\"classname\" \"monster_wizard\"\n\"origin\" \"704 984 712\"\n\"angle\" \"270\"\n}\n{\n\"classname\" \"trigger_once\"\n\"target\" \"monsters3\"\n\"model\" \"*22\"\n}\n{\n\"classname\" \"info_teleport_destination\"\n\"origin\" \"-1080 -88 328\"\n\"targetname\" \"monsters_dest4\"\n}\n{\n\"classname\" \"info_teleport_destination\"\n\"origin\" \"-1016 -104 328\"\n\"targetname\" \"monsters_dest3\"\n}\n{\n\"classname\" \"info_teleport_destination\"\n\"origin\" \"-1016 -160 328\"\n\"targetname\" \"monsters_dest5\"\n}\n{\n\"classname\" \"trigger_teleport\"\n\"target\" \"monsters_dest3\"\n\"targetname\" \"monsters2\"\n\"model\" \"*23\"\n}\n{\n\"classname\" \"monster_wizard\"\n\"origin\" \"2160 1120 368\"\n}\n{\n\"classname\" \"monster_wizard\"\n\"origin\" \"2048 1120 368\"\n}\n{\n\"classname\" \"trigger_teleport\"\n\"target\" \"monsters_dest4\"\n\"targetname\" \"monsters2\"\n\"model\" \"*24\"\n}\n{\n\"classname\" \"monster_wizard\"\n\"origin\" \"2104 1120 368\"\n}\n{\n\"classname\" \"trigger_teleport\"\n\"target\" \"monsters_dest5\"\n\"targetname\" \"monsters2\"\n\"model\" \"*25\"\n}\n{\n\"classname\" \"info_teleport_destination\"\n\"origin\" \"176 " },
	{ EF_COPY, 18888, 4 },
	{ EF_INSERT, 0, 2, "-1" },
	{ EF_COPY, 18938, 1 },
	{ EF_INSERT, 0, 73, "6\"\n\"angle\" \"180\"\n\"targetname\" \"monsters_dest7\"\n}\n{\n\"classname\" \"info_tele" },
	{ EF_COPY, 18953, 1 },
	{ EF_INSERT, 0, 3, "ort" },
	{ EF_COPY, 19008, 2 },
	{ EF_INSERT, 0, 26, "estination\"\n\"origin\" \"-144" },
	{ EF_COPY, 20617, 12 },
	{ EF_COPY, 20742, 1625 },

	// coe1@175a
	{ EF_COPY, 0, 20645 },
	{ EF_COPY, 20658, 32855 },

	// e1m1@c49d
	{ EF_COPY, 0, 9099 },
	{ EF_INSERT, 0, 51, "\"lip\" \"7\" // svdijk -- added to prevent z-fighting\n" },
	{ EF_COPY, 9099, 17184 },

	// e1m2@0caa
	{ EF_COPY, 0, 21484 },
	{ EF_INSERT, 0, 56, "1\" // svdijk -- changed to prevent z-fighting (was \"90\")" },
	{ EF_COPY, 21486, 98 },
	{ EF_INSERT, 0, 54, "69\" // svdijk -- changed to prevent z-fighting (was \"2" },
	{ EF_COPY, 21584, 3 },
	{ EF_INSERT, 0, 1, ")" },
	{ EF_COPY, 21587, 19591 },

	// e1m4@958e
	{ EF_COPY, 0, 28919 },
	{ EF_INSERT, 0, 57, "\"t_length\" \"73\" // svdijk -- added to prevent z-fighting\n" },
	{ EF_COPY, 28919, 744 },
	{ EF_INSERT, 0, 57, "\n\"t_length\" \"73\" // svdijk -- added to prevent z-fighting" },
	{ EF_COPY, 29663, 13958 },

	// e2m2@fbfe
	{ EF_COPY, 0, 12165 },
	{ EF_INSERT, 0, 51, "\"lip\" \"7\" // svdijk -- added to prevent z-fighting\n" },
	{ EF_COPY, 12165, 6990 },
	{ EF_INSERT, 0, 73, "5 280 104\" // svdijk -- changed to prevent z-fighting (was \"-16 280 104\")" },
	{ EF_COPY, 19165, 63 },
	{ EF_INSERT, 0, 73, "5 280 104\" // svdijk -- changed to prevent z-fighting (was \"-16 280 104\")" },
	{ EF_COPY, 19238, 7764 },

	// e2m3@237a
	{ EF_COPY, 0, 16020 },
	{ EF_INSERT, 0, 51, "\"lip\" \"7\" // svdijk -- added to prevent z-fighting\n" },
	{ EF_COPY, 16020, 97 },
	{ EF_INSERT, 0, 57, "\n\"t_length\" \"65\" // svdijk -- added to prevent z-fighting" },
	{ EF_COPY, 16117, 9768 },
	{ EF_INSERT, 0, 57, "\"t_length\" \"65\" // svdijk -- added to prevent z-fighting\n" },
	{ EF_COPY, 25885, 416 },
	{ EF_INSERT, 0, 57, "\n\"t_length\" \"65\" // svdijk -- added to prevent z-fighting" },
	{ EF_COPY, 26301, 9517 },
	{ EF_INSERT, 0, 57, "\"t_length\" \"65\" // svdijk -- added to prevent z-fighting\n" },
	{ EF_COPY, 35818, 2876 },

	// e2m7@10a8
	{ EF_COPY, 0, 19568 },
	{ EF_INSERT, 0, 59, "\n\"origin\" \"-1 0 0\" // svdijk -- added to prevent z-fighting" },
	{ EF_COPY, 19568, 30934 },

	// eoem7@3ea8
	{ EF_COPY, 0, 37059 },
	{ EF_INSERT, 0, 19, "spawnflags\" \"256\"\n\"" },
	{ EF_COPY, 37059, 18843 },
	{ EF_INSERT, 0, 128, "{\n\"classname\" \"trigger_counter\"\n\"count\" \"1\"\n\"origin\" \"776 1240 -136\"\n\"spawnflags\" \"1536\"\n\"target\" \"cellar\"\n\"targetname\" \"bob\"\n}\n" },

	// hrim_sp1@f054
	{ EF_COPY, 0, 33506 },
	{ EF_COPY, 33519, 77691 },

	// sm100_zwiffle@c897
	{ EF_COPY, 0, 117 },
	{ EF_INSERT, 0, 2, "40" },
	{ EF_COPY, 119, 12924 },

	// sm212_naitelveni@a835
	{ EF_COPY, 0, 48917 },
	{ EF_INSERT, 0, 2, "61" },
	{ EF_COPY, 48919, 4 },
	{ EF_INSERT, 0, 1, "1" },
	{ EF_COPY, 48924, 91145 },

	// sm27_bear@92ba
	{ EF_COPY, 0, 5097 },
	{ EF_INSERT, 0, 2, "48" },
	{ EF_COPY, 5099, 1502 },
};

static constexpr EF_Fix ef_fixes[] =
{
	{ "bbelief6", 64996, 64996, 0x324B, 0, 3 },
	{ "blitz1000", 22368, 22368, 0xE5BB, 3, 12 },
	{ "coe1", 53514, 53501, 0x175A, 15, 2 },
	{ "e1m1", 26284, 26335, 0xC49D, 17, 3 },
	{ "e1m2", 41179, 41288, 0x0CAA, 20, 7 },
	{ "e1m4", 43622, 43736, 0x958E, 27, 5 },
	{ "e2m2", 27003, 27180, 0xFBFE, 32, 7 },
	{ "e2m3", 38695, 38974, 0x237A, 39, 11 },
	{ "e2m7", 50503, 50562, 0x10A8, 50, 3 },
	{ "eoem7", 55903, 56050, 0x3EA8, 53, 4 },
	{ "hrim_sp1", 111211, 111198, 0xF054, 57, 2 },
	{ "sm100_zwiffle", 13044, 13044, 0xC897, 59, 3 },
	{ "sm212_naitelveni", 140070, 140070, 0xA835, 62, 5 },
	{ "sm27_bear", 6602, 6602, 0x92BA, 67, 3 },
};
