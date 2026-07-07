
#include <Windows.h>
#include <ShObjIdl.h>
#include <intrin.h>
#pragma intrinsic(__movsb)

//#include "load_zlib.h" // too painful to use currently
// ^^^ using Windows 8+ compression API instead
#include <compressapi.h>
#include "compacted_pe.h"


// TODO: .rdata merging optimisations
// we can compact the import dir, which will be in .idata
// don't keep .idata, just remember where the mappings are
// add a section then construct from the mappings (at least 2x more efficient)

void load(BYTE* compacted_pe);

#define PE_SIG_SIZE 4

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

	//load_zlib();

	DWORD size = compacted_pe_size;
	BYTE* compacted_pe = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);
	//Cabinet.lib
	COMPRESSOR_HANDLE comp;
	CreateDecompressor(COMPRESS_ALGORITHM_MSZIP, 0, &comp);
	DWORD decomp_size = 0;
	Decompress(comp, compressed_pe, sizeof(compressed_pe), compacted_pe, size, &decomp_size);

	//uncompress(compacted_pe, &size, compressed_pe, sizeof(compressed_pe));

	load(compacted_pe);
}

inline void* fast_memcpy(BYTE* dst, const BYTE* src, DWORD count) {
	__movsb(dst, src, count);
	return dst;
}

void load(BYTE* compacted_pe) {

	// map the sections
	PCompactedImage ci = compacted_pe;
	DWORD addr = VirtualAlloc(NULL, ci->total_alloc_size, MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);
	
	for (int i = 0; i < 3; i++) {
		DWORD dst = addr + ci->sections[i].mapping_offset;
		DWORD src = (DWORD)compacted_pe + ci->sections[i].data_offset;
		fast_memcpy(dst, src, ci->sections[i].data_size);
	}

	// apply relocations
	WORD* relocs = compacted_pe + sizeof(CompactedImage);
	DWORD delta = addr - ci->sub_to_get_reloc_delta;
	while (*relocs) {
		DWORD* dst = (DWORD*)(addr + *relocs);
		*dst = *dst + delta;
		relocs++;
	}

	// load DLLs + populate IAT
	DWORD rdata_rva_to_addr = addr - ci->sub_to_get_import_delta;
	PIMAGE_IMPORT_DESCRIPTOR import_table = ci->import_dir.VirtualAddress +
		rdata_rva_to_addr;
	DWORD import_table_size = ci->import_dir.Size;
	PIMAGE_IMPORT_DESCRIPTOR iat = ci->iat.VirtualAddress + rdata_rva_to_addr;
	DWORD iat_size = ci->iat.Size;

	for (int i = 0; i < import_table_size / sizeof(IMAGE_IMPORT_DESCRIPTOR) - 1; i++) {

		char* dll_name = rdata_rva_to_addr + (DWORD)import_table[i].Name;
		HMODULE cur_lib = LoadLibraryA(dll_name);

		DWORD* thunk_data = rdata_rva_to_addr +
			(DWORD)import_table[i].OriginalFirstThunk;
		DWORD* thunks = rdata_rva_to_addr +
			(DWORD)import_table[i].FirstThunk;

		int i = 0;
		while (thunk_data[i]) {
			if (thunk_data[i] & 0x80000000) {
				thunks[i] = GetProcAddress(cur_lib, thunk_data[i] & 0xffff);
			}
			else {
				DWORD name_addr = rdata_rva_to_addr + thunk_data[i] + 2; // skip hint
				thunks[i] = GetProcAddress(cur_lib, name_addr);
			}

			if (!thunks[i]) {
				return 1;
			}
			i++;
		}
	}

	FlushInstructionCache(GetCurrentProcess(), addr, ci->total_alloc_size);

	DWORD mapped_entry_point = addr + ci->entry_point;

	((void(*)())mapped_entry_point)();

}