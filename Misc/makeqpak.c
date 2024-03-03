/*
 * makeqpak.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct
{
	char magic[4];
	uint32_t offset;
	uint32_t size;
} PakHeader;

#define PAK_HEADER_SIZE sizeof(PakHeader)

#define MAX_FILENAME_LENGTH 55

typedef struct
{
	char name[MAX_FILENAME_LENGTH + 1];
	uint32_t offset;
	uint32_t size;
} PakEntry;

#define PAK_ENTRY_SIZE sizeof(PakEntry)

#define MQP_VERIFY(EXPR) { if (!(EXPR)) { printf("ERROR: '%s' failed at line %i\n", #EXPR, __LINE__); return __LINE__; } }


static int BuildPakFile(const char* pakFileName, char** inputFileNames, size_t inputFilesCount)
{
	for (size_t i = 0; i < inputFilesCount; ++i)
		MQP_VERIFY(strlen(inputFileNames[i]) <= MAX_FILENAME_LENGTH);

	FILE* pakFile = fopen(pakFileName, "wb");
	MQP_VERIFY(pakFile != NULL);
	MQP_VERIFY(fseek(pakFile, PAK_HEADER_SIZE, SEEK_SET) == 0);

	size_t directorySize = PAK_ENTRY_SIZE * inputFilesCount;
	PakEntry* directory = malloc(directorySize);
	MQP_VERIFY(directory != NULL);

	size_t offset = PAK_HEADER_SIZE;

	for (size_t i = 0; i < inputFilesCount; ++i)
	{
		const char* inputFileName = inputFileNames[i];
		FILE* inputFile = fopen(inputFileName, "rb");
		MQP_VERIFY(inputFile != NULL);
		MQP_VERIFY(fseek(inputFile, 0, SEEK_END) == 0);

		long inputFileSize = ftell(inputFile);
		MQP_VERIFY(inputFileSize != -1);
		MQP_VERIFY(fseek(inputFile, 0, SEEK_SET) == 0);

		void* buffer = malloc(inputFileSize);
		MQP_VERIFY(buffer != NULL);
		MQP_VERIFY(fread(buffer, 1, inputFileSize, inputFile) == inputFileSize);
		MQP_VERIFY(fclose(inputFile) == 0);
		MQP_VERIFY(fwrite(buffer, 1, inputFileSize, pakFile) == inputFileSize);
		free(buffer);

		PakEntry* entry = &directory[i];
		memset(entry, 0, PAK_ENTRY_SIZE);
		strcpy(entry->name, inputFileName);
		// TODO: sanitize filename
		entry->offset = (uint32_t)offset;
		entry->size = (uint32_t)inputFileSize;
		// TODO: swap on big endian system

		int padding = (4 - (inputFileSize & 3)) & 3;
		if (padding > 0)
			MQP_VERIFY(fseek(pakFile, padding, SEEK_CUR) == 0);

		offset += inputFileSize + padding;
	}

	for (size_t i = 0; i < inputFilesCount; ++i)
		MQP_VERIFY(fwrite(&directory[i], 1, PAK_ENTRY_SIZE, pakFile) == PAK_ENTRY_SIZE);

	free(directory);

	MQP_VERIFY(fseek(pakFile, 0, SEEK_SET) == 0);

	PakHeader header =
	{
		{ 'P', 'A', 'C', 'K' },
		(uint32_t)offset,
		(uint32_t)directorySize
	};
	// TODO: swap on big endian system

	MQP_VERIFY(fwrite(&header, 1, PAK_HEADER_SIZE, pakFile) == PAK_HEADER_SIZE);
	MQP_VERIFY(fclose(pakFile) == 0);

	return 0;
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		printf("Usage: %s pak-file input-file ...\n", argv[0]);
		return 1;
	}

	return BuildPakFile(argv[1], argv + 2, argc - 2);
}
