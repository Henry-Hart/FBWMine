
#include <Windows.h>
#include <stdio.h>
//#include <zlib.h>
#include <compressapi.h>
#pragma comment(lib, "Cabinet.lib")

// TODO: .rdata merging optimisations
// we can compact the import dir, which will be in .idata
// don't keep .idata, just remember where the mappings are
// add a section then construct from the mappings (at least 2x more efficient)

void* read_file_extra(char* fileName, DWORD* read);
void load(BYTE* compacted_pe);
void WriteByteArray(HANDLE h, const char* name, const BYTE* data, 
	DWORD size, DWORD expanded_size); // ~ GPT

#define PE_SIG_SIZE 4
//#define printf(asd)

typedef struct _SectionHeader {
	DWORD data_offset; // maybe just first offset?
	DWORD data_size;
	DWORD mapping_offset;
} SectionHeader, * PSectionHeader;

// compacted image with .text, .rdata, .data sections
typedef struct _CompactedImage {
	SectionHeader sections[3];
	DWORD sub_to_get_reloc_delta;
	DWORD sub_to_get_import_delta;
	DWORD entry_point;
	IMAGE_DATA_DIRECTORY import_dir;
	IMAGE_DATA_DIRECTORY iat;
	DWORD total_alloc_size;
} CompactedImage, * PCompactedImage;

int main() {

	int ret = _main();

	// just a bit nicer like this
	printf("Press any ENTER to continue...\n");
	getchar();

	return ret;
}

int _main() {

	// construct compacted data
	CompactedImage ci;

	const char* path = ".\\FBW_Minecraft.exe";
	DWORD bytes_read = 0;
	// breaks on non-ansi paths
	void* base = read_file_extra(path, &bytes_read);
	if (!base) {
		printf("File load failed :(\n");
		return 1;
	}

	// parse PE headers
	PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)base;
	PIMAGE_FILE_HEADER file_header = (DWORD)dos_header + dos_header->e_lfanew + PE_SIG_SIZE;
	PIMAGE_OPTIONAL_HEADER32 optional_header = (LONG)file_header + sizeof(IMAGE_FILE_HEADER);
	WORD opt_header_size = file_header->SizeOfOptionalHeader;
	PIMAGE_SECTION_HEADER section_header = opt_header_size + (DWORD)optional_header;

	ci.total_alloc_size = 0;
	DWORD next_offset = sizeof(ci);

	// record import dir RVA
	ci.import_dir = optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	ci.iat = optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT];

	// verify we have the right layout
	DWORD sections_count = file_header->NumberOfSections;
	char* valid_names[] = {
		".text", ".rdata", ".data", ".reloc"
	};
	PIMAGE_SECTION_HEADER section_base = section_header;

	// also record value for relocs
	ci.sub_to_get_reloc_delta = optional_header->ImageBase + section_base->VirtualAddress;

	for (int i = 0; i < 4; i++) {

		const char* name = section_header[i].Name;
		if (strcmp(name, valid_names[i]) != 0) {
			printf("Bad names :(\n");
			return 1;
		}

		if (i == 3) break;

		// also record relevant stuff from the sections
		ci.sections[i].data_offset = next_offset;
		next_offset += section_header[i].SizeOfRawData;
		ci.sections[i].data_size = section_header[i].SizeOfRawData;
		ci.sections[i].mapping_offset = ci.total_alloc_size;
		ci.total_alloc_size +=
			section_header[i + 1].VirtualAddress - section_header[i].VirtualAddress;
	}

	// record entrypoint RVA
	ci.entry_point = optional_header->AddressOfEntryPoint //+ section_header[0].PointerToRawData
		- section_header[0].VirtualAddress; // assume entrypoint in .text

	ci.sub_to_get_import_delta = section_base->VirtualAddress;

	// parse reloc table + record all relocs
	DWORD reloc_data_size = section_header[3].SizeOfRawData;
	WORD* reloc_data = malloc(reloc_data_size + 2);
	DWORD reloc_data_i = 0;

	void* reloc_table = optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]
		.VirtualAddress;
	DWORD reloc_size = optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]
		.Size;
	PIMAGE_BASE_RELOCATION cur = (DWORD)base + section_header[3].PointerToRawData;
	DWORD total_size = 0;
	DWORD num_relocs = 0;
	while (total_size < reloc_size) {

		DWORD cur_size = cur->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION);

		if (cur_size == 0) {
			printf("cur_size shouldn't be 0...\n");
			return 1;
		}

		DWORD total_cur_size = 0;
		WORD* cur_reloc = (WORD*)((DWORD)cur + sizeof(IMAGE_BASE_RELOCATION));

		while (total_cur_size < cur_size) {

			DWORD type = (*cur_reloc & 0xf000) >> 12;
			DWORD rva = *cur_reloc & 0x0fff;

			if (type != IMAGE_REL_BASED_HIGHLOW) {

				if (type == 0) {
					cur_reloc = (WORD*)((DWORD)cur_reloc + sizeof(WORD));
					total_cur_size += sizeof(WORD);
					continue;
				}

				printf("Can't handle different relocs\n");
				printf("Total: %lu, goal: %lu\n", total_cur_size, cur_size);
				return 1;
			}

			DWORD va = cur->VirtualAddress + rva - section_header[0].VirtualAddress;
			reloc_data[reloc_data_i++] = va; // record the VAs

			num_relocs++;

			cur_reloc = (WORD*)((DWORD)cur_reloc + sizeof(WORD));
			total_cur_size += sizeof(WORD);
		}

		cur = (void*)((DWORD)cur + cur_size + sizeof(IMAGE_BASE_RELOCATION));
		total_size += (cur_size + sizeof(IMAGE_BASE_RELOCATION));
	}

	reloc_data[reloc_data_i++] = 0;
	reloc_data_size = reloc_data_i * 2;

	for (int i = 0; i < 3; i++) {
		ci.sections[i].data_offset += reloc_data_size;
	}

	// construct ci | reloc_data | .text | .rdata | .data
	DWORD alloc_size = sizeof(ci) + reloc_data_size + ci.sections[0].data_size
		+ ci.sections[1].data_size + ci.sections[2].data_size;
	DWORD buff = malloc(alloc_size);
	void* out = buff;

	memcpy(buff, &ci, sizeof(ci));
	buff += sizeof(ci);
	memcpy(buff, reloc_data, reloc_data_size);
	buff += reloc_data_size;
	for (int i = 0; i < 3; i++) {
		memcpy(buff, (DWORD)base + section_header[i].PointerToRawData, ci.sections[i].data_size);
		buff += ci.sections[i].data_size;
	}

	//printf("reloc data size: %p\n", reloc_data_size);

	printf("Original size: %lu\n", bytes_read);
	printf("Compacted size: %lu\n", alloc_size);

	void* buff2 = malloc(32000);
	DWORD buff2_size = 32000;
	//if (compress(buff2, &buff2_size, out, alloc_size) != Z_OK) {
	//	printf("compression failed :(\n");
	//	return 1;
	//}
	COMPRESSOR_HANDLE comp;
	CreateCompressor(COMPRESS_ALGORITHM_MSZIP, 0, &comp);
	DWORD compressed_size = 0;
	Compress(comp, out, alloc_size, buff2, buff2_size, &compressed_size);
	

	printf("Compressed size: %lu\n", compressed_size);

	const char* out_path = "..\\Loader\\compacted_pe.h";

	WriteByteArray(out_path, "compressed_pe", buff2, compressed_size, alloc_size);

	return 0;
}

void WriteByteArray(const char* path, const char* name, 
	const BYTE* data, DWORD size, DWORD expanded_size)
{

	HANDLE h = CreateFileA(
		path,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (h == INVALID_HANDLE_VALUE)
		return FALSE;

	char buf[200];
	DWORD written;

	wsprintfA(buf, 
		"#pragma code_seg(\".text\")\r\n__declspec(allocate(\".text\")) unsigned int "
		"compacted_pe_size = %lu;\r\n__declspec(allocate(\".text\")) unsigned char %s[] = {\r\n    ",
		expanded_size, name);
	WriteFile(h, buf, lstrlenA(buf), &written, NULL);

	// This was just GPT for speed
	for (DWORD i = 0; i < size; i++)
	{
		wsprintfA(buf, "0x%02X", data[i]);
		WriteFile(h, buf, lstrlenA(buf), &written, NULL);

		if (i + 1 != size)
			WriteFile(h, ", ", 2, &written, NULL);

		if (((i + 1) & 15) == 0 && i + 1 != size)
			WriteFile(h, "\r\n    ", 6, &written, NULL);
	}

	WriteFile(h, "\r\n};\r\n", 6, &written, NULL);
}

void* read_file_extra(char* fileName, DWORD* read) {

	void* f = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE) {
		printf("CreateFileA failed\n");
		return 0;
	}

	LARGE_INTEGER size_struct;
	GetFileSizeEx(f, &size_struct);
	DWORD size = size_struct.LowPart;

	void* buff = malloc(size);

	DWORD totalRead = 0;
	while (totalRead < size)
	{
		DWORD chunkRead = 0;
		if (!ReadFile(
			f,
			(LPVOID)((DWORD)buff + totalRead),
			size - totalRead,
			&chunkRead, NULL
		))
		{
			printf("ReadFile failed\n");
		}

		if (chunkRead == 0) break; // EOF
		totalRead += chunkRead;
	}

	if (totalRead != size) {
		printf("Couldn't read all of file!\n");
		*read = totalRead;
		return 0;
	}

	CloseHandle(f);

	*read = totalRead;
	return buff;
}