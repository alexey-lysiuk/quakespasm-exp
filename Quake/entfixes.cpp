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

extern "C"
{
#include "quakedef.h"
//#include "bspfile.h"
#include "zone.h"
}

#include "entfixes.h"

//extern "C" char* EF_ApplyEntitiesFix(const lump_t* lump, unsigned crc)
extern "C" char* EF_ApplyEntitiesFix(const char* mapname, const byte* oldents, unsigned oldsize, unsigned crc)
{
	unsigned oldcrc = 0xc49d;
	if (oldcrc != crc)
		return nullptr;

	//unsigned oldsize = 26284;  // 26283 + 1 '\0'
	if (oldsize != 26284)
		return nullptr;

	//const char* mapname = "maps/e1m1.bsp";
	if (strcmp(mapname, "maps/e1m1.bsp") != 0)
		return nullptr;

	unsigned newsize = 26335;  // 26334 + 1 '\0'
	char* newents = (char *) Hunk_Alloc(newsize);
	//byte* oldptr = mod_base + lump->fileofs;

	// 'equal', 0, 9099, 0, 9099
	memcpy(newents, oldents, 9099);

	// 'insert', 9099, 9099, 9099, 9150
	const char* insert = "\"lip\" \"7\" // svdijk -- added to prevent z-fighting\n";
	memcpy(newents + 9099, insert, 9150-9099);

	// 'equal', 9099, 26283, 9150, 26334
	memcpy(newents + 9150, oldents + 9099, 26283-9099);

	newents[26334] = '\0';

	return newents;
}
