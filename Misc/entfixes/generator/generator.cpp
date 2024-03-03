
#include <cstdio>
#include <string>
#include <vector>

#include <dirent.h>

#include "google/vcencoder.h"
#include "google/codetablewriter_interface.h"

static const char* header = R"(/*

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

*/)";

enum EF_PatchOperation
{
	EF_ADD,
	EF_COPY,
	EF_RUN,
};

struct EF_Patch
{
	EF_PatchOperation operation;
	std::string data;
	size_t size;
	int32_t offset;
	unsigned char byte;
};

struct EF_Fix
{
	std::string mapname;
	std::string crc;
	std::vector<EF_Patch> patches;
	size_t oldsize;
	size_t newsize;
};

static std::vector<EF_Fix> fixes;

using namespace open_vcdiff;

static std::string EscapeCString(const char* data, size_t size)
{
	std::string result;

	for (size_t i = 0; i < size; ++i)
	{
		const unsigned char c = static_cast<unsigned char>(data[i]);	
		switch (c)
		{
		case '"': result.append("\\\"", 2); break;
		case '\\': result.append("\\\\", 2); break;
		case '\b': result.append("\\b", 2); break;
		case '\f': result.append("\\f", 2); break;
		case '\n': result.append("\\n", 2); break;
		case '\r': result.append("\\r", 2); break;
		case '\t': result.append("\\t", 2); break;
		default:
			if (c < 32 || c >= 127)
			{
				char code[8] = "";
				snprintf(code, sizeof code, "\\x%02x", c);
				result.append(code);
			}
			else
			{
				result.push_back(data[i]);
			}
		}
	}

	return result;
}

class EntFixCodeTableWriter : public CodeTableWriterInterface
{
public:
	std::vector<EF_Patch> patches;

	void Add(const char* data, size_t size) override
	{
		const std::string escaped = EscapeCString(data, size);
		patches.push_back({ .operation = EF_ADD, .data = escaped, .size = size });
	}

	void Copy(int32_t offset, size_t size) override
	{
		patches.push_back({ .operation = EF_COPY, .offset = offset, .size = size });
	}

	void Run(size_t size, unsigned char byte) override
	{
		patches.push_back({ .operation = EF_RUN, .byte = byte, .size = size });
	}

	bool Init(size_t dictionary_size) override { return true; }
	void WriteHeader(OutputStringInterface* out, VCDiffFormatExtensionFlags format_extensions) override	{}
	void AddChecksum(VCDChecksum) override {}
	void Output(OutputStringInterface* out) override {}
	void FinishEncoding(OutputStringInterface *out) override {}
	bool VerifyDictionary(const char *dictionary, size_t size) const override { return true; }
	bool VerifyChunk(const char *chunk, size_t size) const override { return true; }
};

#define EFG_VERIFY(EXPR) { if (!(EXPR)) { printf("ERROR: '%s' failed at line %i\n", #EXPR, __LINE__); exit(__LINE__); } }

static std::string ReadFile(const std::string& filename, size_t& size)
{
	FILE* file = fopen(filename.c_str(), "rb");
	EFG_VERIFY(file);
	EFG_VERIFY(fseek(file, 0, SEEK_END) == 0);

	size = ftell(file);
	EFG_VERIFY(size > 0);
	EFG_VERIFY(fseek(file, 0, SEEK_SET) == 0);

	std::string buffer;
	buffer.resize(size);
	EFG_VERIFY(fread(&buffer[0], 1, size, file) == size);
	EFG_VERIFY(fclose(file) == 0);

	return buffer;
}

static std::string oldpath, newpath;
static std::vector<std::string> filenames;

static void GatherFileList()
{
	DIR* olddir = opendir(oldpath.c_str());
	EFG_VERIFY(olddir);

	while (dirent* entry = readdir(olddir))
	{
		const char* filename = entry->d_name;

		if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
			continue;

		filenames.emplace_back(filename);
	}

	EFG_VERIFY(closedir(olddir) == 0);
	EFG_VERIFY(!filenames.empty());
}

static void ProcessEntPatch(const std::string& filename)
{
	size_t oldsize, newsize;
	const std::string olddata = ReadFile(oldpath + filename, oldsize);
	const std::string newdata = ReadFile(newpath + filename, newsize);

	HashedDictionary dictionary(olddata.data(), olddata.size());
	dictionary.Init();

	auto writer = new EntFixCodeTableWriter;
	VCDiffStreamingEncoder encoder(&dictionary, VCD_STANDARD_FORMAT, false, writer);

	std::string output;
	EFG_VERIFY(encoder.StartEncoding(&output));
	EFG_VERIFY(encoder.EncodeChunk(newdata.data(), newdata.size(), &output));
	EFG_VERIFY(encoder.FinishEncoding(&output));

	const size_t atpos = filename.find('@');
	EFG_VERIFY(atpos != std::string::npos);
	EFG_VERIFY(atpos < filename.size());

	const size_t dotpos = filename.find('.');
	EFG_VERIFY(dotpos != std::string::npos);
	EFG_VERIFY(dotpos != filename.size());
	EFG_VERIFY(dotpos > atpos);

	std::string mapname{ filename.c_str(), atpos };
	EFG_VERIFY(!mapname.empty());

	std::string crc{ filename.c_str() + atpos + 1, dotpos - atpos - 1 };
	EFG_VERIFY(crc.size() == 4);  // 16-bit hex CRC

	fixes.push_back(
	{
		.mapname = std::move(mapname),
		.crc = std::move(crc),
		.patches = std::move(writer->patches),
		.oldsize = oldsize,
		.newsize = newsize
	});
}

static void Generate(const char* rootpath)
{
	std::string entpath(rootpath);
	entpath += "Misc/entfixes/entities/";

	oldpath = entpath + "old/";
	newpath = entpath + "new/";

	GatherFileList();

	std::for_each(filenames.begin(), filenames.end(), ProcessEntPatch);
}

int main(int argc, const char* argv[])
{
	if (argc != 2)
	{
		printf("Usage: %s repo-root-dir", argv[0]);
		return EXIT_FAILURE;
	}

	Generate(argv[1]);
	return EXIT_SUCCESS;

//	if (argc < 4)
//		return 1;
//
//	std::string original_path = argv[1];
//	original_path += '/';
//
//	std::string fixed_path = argv[2];
//	fixed_path += '/';
//
//	puts(header);
//	puts("\nstatic constexpr EF_Patch ef_patches[] =\n{");
//
//	for (int i = 3; i < argc; ++i)
//	{
//		if (i > 3)
//			puts("");
//
//		printf("\t// %s\n", argv[i]);
//
//		const std::string olddata = ReadFile(original_path + argv[i]);
//		const std::string newdata = ReadFile(fixed_path + argv[i]);
//
//		HashedDictionary dictionary(olddata.data(), olddata.size());
//		dictionary.Init();
//
//		auto writer = new EntFixCodeTableWriter;
//		VCDiffStreamingEncoder encoder(&dictionary, VCD_STANDARD_FORMAT, false, writer);
//
//		std::string output;
//		bool res = encoder.StartEncoding(&output);
//		res = encoder.EncodeChunk(newdata.data(), newdata.size(), &output);
//		res = encoder.FinishEncoding(&output);
//	}
//
//	puts("};\n\nstatic constexpr EF_Fix ef_fixes[] =\n{");
//
//	return 0;
}
