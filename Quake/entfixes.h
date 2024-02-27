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
	// e1m1
	{ EF_COPY, 0, 9099 },
	{ EF_INSERT, 0, 51, "\"lip\" \"7\" // svdijk -- added to prevent z-fighting\n" },
	{ EF_COPY, 9099, 17184 },

	// e2m2
	{ EF_COPY, 0, 12165 },
	{ EF_INSERT, 0, 51, "\"lip\" \"7\" // svdijk -- added to prevent z-fighting\n" },
	{ EF_COPY, 12165, 6990 },
	{ EF_INSERT, 0, 73, "5 280 104\" // svdijk -- changed to prevent z-fighting (was \"-16 280 104\")\n" },
	{ EF_COPY, 19165, 63 },
	{ EF_INSERT, 0, 73, "5 280 104\" // svdijk -- changed to prevent z-fighting (was \"-16 280 104\")\n" },
	{ EF_COPY, 19238, 7764 },

	// ...
};


static constexpr EF_Fix ef_fixes[] =
{
	{ "e1m1", 26284, 26335, 0xC49D, 0, 3 },
	{ "e2m2", 27003, 27180, 0XFBFE, 3, 7 },
};
