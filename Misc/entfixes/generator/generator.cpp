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
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>

#include "google/vcencoder.h"
#include "google/codetablewriter_interface.h"


enum EF_Operation
{
	EF_ADD,
	EF_COPY,
	EF_RUN,
};

struct EF_Patch
{
	EF_Operation operation;
	size_t value;
	size_t size;
};

struct EF_Fix
{
	EF_Fix(std::string&& mapname, std::string&& crc, std::vector<EF_Patch>&& patches, size_t oldsize, size_t newsize)
	: mapname(std::move(mapname))
	, crc(std::move(crc))
	, patches(std::move(patches))
	, oldsize(oldsize)
	, newsize(newsize)
	{
	}

	std::string mapname;
	std::string crc;
	std::vector<EF_Patch> patches;
	size_t oldsize;
	size_t newsize;
	size_t patchindex;

	EF_Fix() = default;
	EF_Fix(const EF_Fix&) = default;
	EF_Fix(EF_Fix&&) = default;
	EF_Fix& operator=(const EF_Fix&) = default;
	EF_Fix& operator=(EF_Fix&&) = default;
};

static std::vector<std::string> addeddata;
static std::vector<size_t> addedsizes;
static std::vector<size_t> addedoffsets;

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
		std::string escaped = EscapeCString(data, size);

		const size_t count = addeddata.size();
		size_t index = count;

		for (size_t i = 0; i < count; ++i)
		{
			if (addeddata[i] == escaped)
			{
				index = i;
				break;
			}
		}

		if (addeddata.size() == index)
		{
			addeddata.push_back(std::move(escaped));
			addedsizes.push_back(size);  // unescaped data size
		}

		patches.push_back({ EF_ADD, index, size });
	}

	void Copy(int32_t offset, size_t size) override
	{
		patches.push_back({ EF_COPY, size_t(offset), size });
	}

	void Run(size_t size, unsigned char byte) override
	{
		patches.push_back({ EF_RUN, byte, size });
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

static std::filesystem::path entitiespath;
static std::filesystem::path oldentitiespath;
static std::filesystem::path newentitiespath;
static std::filesystem::path outputpath;
static std::vector<std::filesystem::path> filenames;

static void GatherFileList()
{
	for (const auto& entry : std::filesystem::directory_iterator(oldentitiespath))
		filenames.emplace_back(entry.path().filename());

	EFG_VERIFY(!filenames.empty());
}

static bool IsOutdated()
{
	if (!std::filesystem::exists(outputpath))
		return true;

	const auto headerwritetime = std::filesystem::last_write_time(outputpath);

	for (const auto& filename : filenames)
	{
		if (std::filesystem::last_write_time(oldentitiespath / filename) > headerwritetime)
			return true;

		if (std::filesystem::last_write_time(newentitiespath / filename) > headerwritetime)
			return true;
	}

	return false;
}

static void ProcessEntFix(const std::string& filename)
{
	size_t oldsize, newsize;
	const std::string olddata = ReadFile(oldentitiespath / filename, oldsize);
	const std::string newdata = ReadFile(newentitiespath / filename, newsize);

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

	fixes.emplace_back(std::move(mapname), std::move(crc), std::move(writer->patches), oldsize + 1, newsize + 1);
}

static void ProcessEntFixes()
{
	std::for_each(filenames.begin(), filenames.end(), ProcessEntFix);

	const size_t addedcount = addeddata.size();
	EFG_VERIFY(addedsizes.size() == addedcount);

	addedoffsets.resize(addeddata.size());
	size_t offset = 0;

	for (size_t i = 0; i < addedcount; ++i)
	{
		addedoffsets[i] = offset;
		offset += addedsizes[i];
	}

	size_t patchindex = 0;

	for (EF_Fix& fix : fixes)
	{
		for (EF_Patch& patch : fix.patches)
		{
			if (patch.operation != EF_ADD)
				continue;

			patch.value = addedoffsets[patch.value];
		}

		fix.patchindex = patchindex;
		patchindex += fix.patches.size();
	}
}

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

*/

)";

static void WriteEntFixes()
{
	FILE* file = fopen(outputpath.c_str(), "w");
	EFG_VERIFY(file);
	EFG_VERIFY(fputs(header, file) >= 0);
	EFG_VERIFY(fputs("static const char* const addeddata =\n", file) >= 0);

	for (size_t i = 0, e = addeddata.size(); i < e; ++i)
		EFG_VERIFY(fprintf(file, "\t/* %zu */ \"%s\"\n", addedoffsets[i], addeddata[i].c_str()) > 0);

	EFG_VERIFY(fputs(";\n\nstatic constexpr EF_Patch ef_patches[] =\n{\n", file) >= 0);

	bool first = true;

	for (const EF_Fix& fix : fixes)
	{
		if (first)
			first = false;
		else
			EFG_VERIFY(fputs("\n", file) >= 0);

		EFG_VERIFY(fprintf(file, "\t// %s@%s\n", fix.mapname.c_str(), fix.crc.c_str()) > 0);

		for (const EF_Patch& patch : fix.patches)
		{
			const EF_Operation op = patch.operation;
			const char* opstr = op == EF_ADD ? "EF_ADD" : (op == EF_COPY ? "EF_COPY" : "EF_RUN");
			EFG_VERIFY(fprintf(file, "\t{ %s, %zu, %zu },\n", opstr, patch.size, patch.value) > 0);
		}
	}

	EFG_VERIFY(fputs("};\n\nstatic constexpr EF_Fix ef_fixes[] =\n{\n", file) >= 0);

	std::sort(fixes.begin(), fixes.end(), [](const EF_Fix& lhs, const EF_Fix& rhs)
	{
		return (lhs.crc < rhs.crc)
			|| (lhs.crc == rhs.crc && lhs.oldsize < rhs.oldsize)
			|| (lhs.oldsize == rhs.oldsize && lhs.mapname < rhs.mapname);
	});

	for (const EF_Fix& fix : fixes)
	{
		EFG_VERIFY(fprintf(file, "\t{ \"%s\", 0x%s, %zu, %zu, %zu, %zu },\n",
			fix.mapname.c_str(), fix.crc.c_str(), fix.oldsize, fix.newsize, fix.patchindex, fix.patches.size()) > 0);
	}

	EFG_VERIFY(fputs("};\n", file) >= 0);
	EFG_VERIFY(fclose(file) == 0);
}

static void Generate(const char* entities, const char* header)
{
	entitiespath = entities;
	oldentitiespath = entitiespath / "old";
	newentitiespath = entitiespath / "new";

	outputpath = header;

	GatherFileList();

	if (IsOutdated())
	{
		ProcessEntFixes();
		WriteEntFixes();
	}
}

int main(int argc, const char* argv[])
{
	if (argc != 3)
	{
		printf("Usage: %s entities-path generated-header-path", argv[0]);
		return EXIT_FAILURE;
	}

	Generate(argv[1], argv[2]);
	return EXIT_SUCCESS;
}
