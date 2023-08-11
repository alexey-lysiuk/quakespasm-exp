/*
 * mkpak.c
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

static void ReportFileError(const char* operation, const char* fileName)
{
	printf("ERROR: Failed to %s file '%s', errno = %i, %s\n", operation, fileName, errno, strerror(errno));
}

static int BuildPakFile(const char* pakFileName, char** inputFileNames, size_t inputFilesCount)
{
	enum DummyEnum { MAX_FILENAME_LENGTH = 56 };

	for (size_t i = 0; i < inputFilesCount; ++i)
	{
		const char* inputFileName = inputFileNames[i];
		size_t inputFileNameLength = strlen(inputFileName);

		if (inputFileNameLength > MAX_FILENAME_LENGTH)
		{
			printf("ERROR: File name %s is too long, %zu > %i\n", inputFileName, inputFileNameLength, MAX_FILENAME_LENGTH);
			return -1;
		}
	}

	FILE* pakFile = fopen(pakFileName, "wb");

	if (!pakFile)
	{
		ReportFileError("open output", pakFileName);
		return -2;
	}

	typedef struct
	{
		char magic[4];
		uint32_t offset;
		uint32_t size;
	} Header;

	if (fseek(pakFile, sizeof(Header), SEEK_SET) != 0)
	{
		ReportFileError("seek output", pakFileName);
		return -3;
	}

	typedef struct
	{
		char name[MAX_FILENAME_LENGTH];
		uint32_t offset;
		uint32_t size;
	} Entry;

	size_t directorySize = sizeof(Entry) * inputFilesCount;
	Entry* directory = malloc(directorySize);
	size_t offset = sizeof(Header);

	if (!directory)
	{
		printf("ERROR: Failed to allocate memory for %zu PAK directory entries\n", inputFilesCount);
		return -4;
	}

	for (size_t i = 0; i < inputFilesCount; ++i)
	{
		const char* inputFileName = inputFileNames[i];
		FILE* inputFile = fopen(inputFileName, "rb");

		if (!inputFile)
		{
			//printf("ERROR: Cannot open input file '%s', errno = %i, %s", inputFileName, errno, strerror(errno));
			ReportFileError("open input", inputFileName);
			return -5;
		}

		if (fseek(inputFile, 0, SEEK_END) != 0)
		{
			ReportFileError("seek input", inputFileName);
			return -6;
		}

		long inputFileSize = ftell(inputFile);

		if (inputFileSize == -1)
		{
			ReportFileError("get size", inputFileName);
			return -7;
		}

		if (fseek(inputFile, 0, SEEK_SET) != 0)
		{
			ReportFileError("seek input", inputFileName);
			return -8;
		}

		void* buffer = malloc(inputFileSize);

		if (!buffer)
		{
			printf("ERROR: Failed to allocate %li bytes\n", inputFileSize);
			return -9;
		}

		if (fread(buffer, 1, inputFileSize, inputFile) != inputFileSize)
		{
			ReportFileError("read input", inputFileName);
			return -10;
		}

		if (fwrite(buffer, 1, inputFileSize, pakFile) != inputFileSize)
		{
			ReportFileError("write content to output", pakFileName);
			return -11;
		}

		free(buffer);

		if (fclose(inputFile) != 0)
		{
			ReportFileError("close input", inputFileName);
			return -12;
		}

		Entry* entry = &directory[i];
		memset(entry, 0, sizeof *entry);
		memcpy(entry->name, inputFileName, strlen(inputFileName));
		entry->offset = (uint32_t)offset;
		entry->size = (uint32_t)inputFileSize;

		int padding = (4 - (inputFileSize & 3)) & 3;

		if (padding > 0 && fseek(pakFile, padding, SEEK_CUR) != 0)
		{
			ReportFileError("seek output", pakFileName);
			return -13;
		}

		offset += inputFileSize + padding;
	}

	for (size_t i = 0; i < inputFilesCount; ++i)
	{
		Entry* entry = &directory[i];
		// TODO: swap on big endian system

		if (fwrite(entry, 1, sizeof *entry, pakFile) != sizeof *entry)
		{
			ReportFileError("write directory entry to output", pakFileName);
			return -14;
		}
	}

	free(directory);

	if (fseek(pakFile, 0, SEEK_SET) != 0)
	{
		ReportFileError("seek output", pakFileName);
		return -15;
	}

	Header header =
	{
		{ 'P', 'A', 'C', 'K' },
		(uint32_t)offset,
		(uint32_t)directorySize
	};
	// TODO: swap on big endian system

	if (fwrite(&header, 1, sizeof header, pakFile) != sizeof header)
	{
		ReportFileError("write header to output", pakFileName);
		return -16;
	}

	if (fclose(pakFile) != 0)
	{
		ReportFileError("close output", pakFileName);
		return -17;
	}

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
