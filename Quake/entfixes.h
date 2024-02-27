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

static EF_Patch ef_patches[] =
{
	// maps/e1m1.bsp
	{ EF_COPY, 0, 0, 9099, nullptr },
	{ EF_INSERT, 0, 9099, 51, "\"lip\" \"7\" // svdijk -- added to prevent z-fighting\n" },
	{ EF_COPY, 9099, 9150, 17184, nullptr },

	// ...
};


static EF_Fix ef_fixes[] =
{
	{ "maps/e1m1.bsp", 26284, 26335, 0XC49D, 0, 3 },
};
