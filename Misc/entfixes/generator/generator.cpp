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

#include <dirent.h>

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

	EF_Fix() = default;
	EF_Fix(const EF_Fix&) = default;
	EF_Fix(EF_Fix&&) = default;
	EF_Fix& operator=(const EF_Fix&) = default;
	EF_Fix& operator=(EF_Fix&&) = default;
};

static std::vector<std::string> addeddata;
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
			addeddata.push_back(std::move(escaped));

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

static std::string rootpath;
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

static void ProcessEntFix(const std::string& filename)
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

	fixes.emplace_back(std::move(mapname), std::move(crc), std::move(writer->patches), oldsize, newsize);
}

static void ProcessEntFixes()
{
	std::for_each(filenames.begin(), filenames.end(), ProcessEntFix);
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
	std::sort(fixes.begin(), fixes.end(), [](const EF_Fix& lhs, const EF_Fix& rhs)
	{
		return (lhs.mapname < rhs.mapname)
			|| (lhs.mapname == rhs.mapname && lhs.crc < rhs.crc)
			|| (lhs.crc == rhs.crc && lhs.oldsize < rhs.oldsize);
	});

	const std::string outputpath = rootpath + "Quake/entfixes.h";

	FILE* file = fopen(outputpath.c_str(), "wb");
	EFG_VERIFY(file);
	EFG_VERIFY(fputs(header, file) >= 0);
	EFG_VERIFY(fputs("static constexpr const char* addeddata[] =\n{\n", file) >= 0);

	for (size_t i = 0, e = addeddata.size(); i < e; ++i)
		EFG_VERIFY(fprintf(file, "\t/* %zu */ \"%s\",\n", i, addeddata[i].c_str()) > 0);

	EFG_VERIFY(fputs("};\n\nstatic constexpr EF_Patch ef_patches[] =\n{\n", file) >= 0);

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

	size_t patchindex = 0;

	for (const EF_Fix& fix : fixes)
	{
		const size_t patchcount = fix.patches.size();

		EFG_VERIFY(fprintf(file, "\t{ \"%s\", 0x%s, %zu, %zu, %zu, %zu },\n",
			fix.mapname.c_str(), fix.crc.c_str(), fix.oldsize, fix.newsize, patchindex, patchindex) > 0);

		patchindex += patchcount;
	}

	EFG_VERIFY(fputs("};\n", file) >= 0);
	EFG_VERIFY(fclose(file) == 0);
}

static void Generate(const char* rootpath)
{
	::rootpath = rootpath;
	::rootpath += '/';

	const std::string entpath = ::rootpath + "Misc/entfixes/entities/";

	oldpath = entpath + "old/";
	newpath = entpath + "new/";

	GatherFileList();
	ProcessEntFixes();
	WriteEntFixes();
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
}
