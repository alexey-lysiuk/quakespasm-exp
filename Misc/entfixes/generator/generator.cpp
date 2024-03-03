
#include "google/vcencoder.h"
#include "google/codetablewriter_interface.h"

#include <cstdio>
#include <string>

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
	// Initializes the writer.
	bool Init(size_t dictionary_size) override
	{
		//puts(__FUNCTION__);
		return true;
	}

	// Encode an ADD opcode with the "size" bytes starting at data
	void Add(const char* data, size_t size) override
	{
		const std::string escaped = EscapeCString(data, size);
		//printf("add: data = \"%s\", size = %zu\n", escaped.data(), size);
		printf("\t{ EF_INSERT, 0, %zu, \"%s\" },\n", size, escaped.data());
	}

	// Encode a COPY opcode with args "offset" (into dictionary) and "size" bytes.
	void Copy(int32_t offset, size_t size) override
	{
		//printf("copy: offset = %i, size = %zu\n", offset, size);
		printf("\t{ EF_COPY, %i, %zu },\n", offset, size);
	}

	// Encode a RUN opcode for "size" copies of the value "byte".
	void Run(size_t size, unsigned char byte) override
	{
		//puts(__FUNCTION__);
	}

	// Writes the header to the output string.
	void WriteHeader(OutputStringInterface* out, VCDiffFormatExtensionFlags format_extensions) override
	{
		//puts(__FUNCTION__);
	}

	void AddChecksum(VCDChecksum) override
	{
		//puts(__FUNCTION__);
	}

	// Appends the encoded delta window to the output
	// string.  The output string is not null-terminated.
	void Output(OutputStringInterface* out) override
	{
		//puts(__FUNCTION__);
	}

	// Finishes the encoding.
	void FinishEncoding(OutputStringInterface *out) override
	{
		//puts(__FUNCTION__);
	}

	// Verifies dictionary is compatible with writer.
	bool VerifyDictionary(const char *dictionary, size_t size) const override
	{
		//puts(__FUNCTION__);
		return true;
	}

	// Verifies target chunk is compatible with writer.
	bool VerifyChunk(const char *chunk, size_t size) const override
	{
		//puts(__FUNCTION__);
		return true;
	}
};

static std::string ReadFile(const std::string& filename)
{
	FILE* in = fopen(filename.c_str(), "rb");
	if (!in)
		return std::string();
	fseek(in, 0, SEEK_END);
	long size = ftell(in);
	fseek(in, 0, SEEK_SET);
	std::string inbuf;
	inbuf.resize(size);
	fread(&inbuf[0], size, 1, in);
	fclose(in);
	return inbuf;
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

*/)";

int main(int argc, const char* argv[])
{
	if (argc < 4)
		return 1;

	std::string original_path = argv[1];
	original_path += '/';

	std::string fixed_path = argv[2];
	fixed_path += '/';

	puts(header);
	puts("\nstatic constexpr EF_Patch ef_patches[] =\n{");

	for (int i = 3; i < argc; ++i)
	{
		if (i > 3)
			puts("");

		printf("\t// %s\n", argv[i]);

		const std::string olddata = ReadFile(original_path + argv[i]);
		const std::string newdata = ReadFile(fixed_path + argv[i]);

		HashedDictionary dictionary(olddata.data(), olddata.size());
		dictionary.Init();

		auto writer = new EntFixCodeTableWriter;
		VCDiffStreamingEncoder encoder(&dictionary, VCD_STANDARD_FORMAT, false, writer);

		std::string output;
		bool res = encoder.StartEncoding(&output);
		res = encoder.EncodeChunk(newdata.data(), newdata.size(), &output);
		res = encoder.FinishEncoding(&output);
	}

	puts("};\n\nstatic constexpr EF_Fix ef_fixes[] =\n{");

	return 0;
}
