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

#include <algorithm>
#include <cassert>

extern "C"
{
#include "quakedef.h"
#include "zone.h"
}

enum EF_Operation
{
	EF_ADD,
	EF_COPY,
	EF_RUN,
};

struct EF_Patch
{
	EF_Operation operation;
	unsigned size;
	unsigned value;
};

struct EF_Fix
{
	const char* mapname;
	unsigned crc;
	unsigned oldsize;
	unsigned newsize;
	unsigned patchindex;
	unsigned patchcount;
};

#include "entfixes.h"

extern "C" char* EF_ApplyEntitiesFix(const char* mapname, const byte* entities, unsigned size, unsigned crc)
{
	assert(mapname);
	assert(entities);
	assert(size > 0);

	const char* mapfilename = COM_SkipPath(mapname);
	char basemapname[MAX_QPATH];
	COM_StripExtension(mapfilename, basemapname, sizeof basemapname);

	char* newentities = nullptr;

	for (const EF_Fix& fix : ef_fixes)
	{
		if (fix.crc != crc || fix.oldsize != size || q_strcasecmp(fix.mapname, basemapname) != 0)
			continue;

		char* writeptr = reinterpret_cast<char*>(Hunk_Alloc(fix.newsize));
		newentities = writeptr;

		for (unsigned i = 0; i < fix.patchcount; ++i)
		{
			const EF_Patch& patch = ef_patches[fix.patchindex + i];

			switch (patch.operation)
			{
			case EF_COPY:
				memcpy(writeptr, entities + patch.value, patch.size);
				break;

			case EF_ADD:
				memcpy(writeptr, addeddata + patch.value, patch.size);
				break;

			case EF_RUN:
				memset(writeptr, patch.value, patch.size);
				break;

			default:
				assert(false);
				break;
			}

			writeptr += patch.size;
		}

		*writeptr = '\0';
		break;
	}

#if 0
	if (newentities)
	{
		char testname[MAX_QPATH];
		q_snprintf(testname, sizeof(testname), "%s@%04x.ent", basemapname, crc);

		FILE* testfile = fopen(testname, "w");
		if (testfile)
		{
			fputs(newentities, testfile);
			fclose(testfile);
		}
	}
#endif

	return newentities;
}
