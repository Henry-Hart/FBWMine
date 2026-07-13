
// #=====================================================#
// #                                                     #
// # nbt.h ==> nbt parser                                #
// #                                                     #
// #=====================================================#

// TODO:
//  - mark context with invalid nbt / palette overflow error
//  - removal / review of stuff

#pragma once
#include "util.h"

#define NBT_End 0
#define NBT_Byte 1
#define NBT_Short 2
#define NBT_Int 3
#define NBT_Long 4
#define NBT_Float 5
#define NBT_Double 6
#define NBT_Byte_Array 7
#define NBT_String 8
#define NBT_List 9
#define NBT_Compound 0xa
#define NBT_Int_Array 0xb
#define NBT_Long_Array 0xc

#define MAX_SUBCHUNKS 24
#define MAX_CHUNKS 1024
#define BLOCKS_PER_SUBCHUNK 4096
#define SECTOR_SIZE 4096
#define REGION_DATA_SIZE 100663296 // BIG
#define COMPRESSION_ZLIB 2

#define MODE_ROOT 0
#define MODE_SECTION 1
#define MODE_BLOCK_STATES 2
#define MODE_PALETTE 3
#define MODE_NORMAL 4

#define DETECT_LEVEL ctx->nbt[4] == 'v' // for older minecraft versions
#define DETECT_INCOMPLETE_STATUS ctx->nbt[14] != 'l'
#define DETECT_STATUS ctx->nbt[2] == 'S' // not needed
#define DETECT_SECTIONS ctx->nbt[3] == 'e'
#define DETECT_BLOCK_STATES ctx->nbt[3] == 'l'
#define DETECT_Y ctx->nbt[2] == 'Y'

#define REGION_STATUS_EMPTY 0
#define REGION_STATUS_AVAILABLE 1
#define REGION_STATUS_WAITING_FOR_UPDATE 2
#define REGION_STATUS_READ_BLOCKED 3
#define REGION_STATUS_HOLLOW 4

#define CHUNK_STATUS_OK 0
#define CHUNK_STATUS_INCOMPLETE 1
#define CHUNK_STATUS_OLD 2

typedef struct _ChunkContext { // chunk context
	BYTE* nbt;
	DWORD chunk_i;
	char** user_palette;
	DWORD user_palette_len;
	BYTE default_palette_value;
	BYTE* palette_secondary_translation;
	BYTE* region_data;
	DWORD covered_subchunks; // for detecting empty sections (from naughty worldgen tools)
	int status;
} ChunkContext, * PChunkContext;

typedef struct _SubchunkContext { // subchunk context
	char subchunk_i;
	DWORD palette_len;
	unsigned long long* saved_data;
	DWORD saved_data_len;
} SubchunkContext, * PSubchunkContext;

typedef struct _RegionAllocContext { // region allocation context
	BYTE* regions[4];
	BYTE allocated;
} RegionAllocContext, * PRegionAllocContext;

typedef struct _RegionInfo { // region info
	int x;
	int z;
	SRWLOCK srw;
	int status;
} RegionInfo, * PRegionInfo;

typedef struct _RegionAllocInfo { // region allocation info
	RegionInfo region_infos[4];
} RegionAllocInfo, * PRegionAllocInfo;

void get_chunks(BYTE* data);
void load_region(PRegionAllocContext rctx, BYTE* base, char** global_palette,
	DWORD palette_len, BYTE* palette_map, DWORD* data, BYTE* mapping, BYTE default_val, BYTE air_id);
int get_chunk(BYTE* data, BYTE* palette_map, int i, PRegionAllocContext rctx, PChunkContext ctx, BYTE air_id);
DWORD get_2_bytes(BYTE* data, DWORD i);
DWORD get_3_bytes(BYTE* data, DWORD i);
DWORD get_4_bytes(BYTE* data, DWORD i);
void nbt_compound_track(PChunkContext ctx, PSubchunkContext sctx, BYTE* palette_map, DWORD list_i, DWORD mode);
void update_region(PChunkContext ctx, PSubchunkContext sctx, BYTE* palette_map);
void fill_region(PChunkContext ctx, BYTE id, char subchunk_i);
void palette_update(char* str, DWORD size, DWORD i, BYTE* palette_map, PChunkContext ctx);
int alloc_4_regions(PRegionAllocContext rctx, PRegionAllocInfo rinfo);
int binary_search_strings(char* arr[], int count, char* target, int fail); // ~ GPT
unsigned long long get_8_bytes(BYTE* data, DWORD i); // ~~ GPT
int wildcard_strcmp(const char* a, const char* b); // ~~ GPT

// LIST: nbt [chunk_i palette_map] in_section
// TODO: pass list index into compound tag
void nbt_list_track(PChunkContext ctx, PSubchunkContext sctx,
	DWORD list_type, DWORD list_max, BYTE* palette_map, DWORD mode) {

	DWORD list_progress = 0;
	while (list_progress++ != list_max) {
		switch (list_type) {
		case NBT_Byte: {
			ctx->nbt += 1;
			break;
		}
		case NBT_Short: {
			ctx->nbt += 2;
			break;
		}
		case NBT_Int: {
			ctx->nbt += 4;
			break;
		}
		case NBT_Long: {
			ctx->nbt += 8;
			break;
		}
		case NBT_Float: {
			ctx->nbt += 4;
			break;
		}
		case NBT_Double: {
			ctx->nbt += 8;
			break;
		}
		case NBT_Byte_Array: {
			DWORD length = get_4_bytes(ctx->nbt, 0);
			ctx->nbt += 4 + length * 1;
			break;
		}
		case NBT_Int_Array: {
			DWORD length = get_4_bytes(ctx->nbt, 0);
			ctx->nbt += 4 + length * 4;
			break;
		}
		case NBT_Long_Array: {
			DWORD length = get_4_bytes(ctx->nbt, 0);
			ctx->nbt += 4 + length * 8;
			break;
		}
		case NBT_Compound: {

			if (mode == MODE_SECTION) {
				// reset the (relevant part of the) subchunk context
				sctx->saved_data = 0;
			}

			// TODO: check if bad return
			nbt_compound_track(ctx, sctx, palette_map, list_progress - 1, mode);
			break;
		}
		case NBT_String: {
			DWORD step = get_2_bytes(ctx->nbt, 0);
			ctx->nbt += 2 + step;
			break;
		}
		case NBT_List: {
			DWORD inner_type = *(ctx->nbt++);
			DWORD length = get_4_bytes(ctx->nbt, 0);
			ctx->nbt += 4;
			nbt_list_track(ctx, sctx, inner_type, length, palette_map, mode);
			break;

		}
		default: {
			printf("Invalid NBT!\n"); // TODO: special return / exit / silent exit
			return;
		}
		}
	}
}

// TODO: use relevant signed / unsigned functions!!!
// COMPOUND: nbt [chunk_i palette_map] [in_section in_block_states el_palette]
// TODO: maybe check (more/all) names
void nbt_compound_track(PChunkContext ctx, PSubchunkContext sctx, BYTE* palette_map,
	DWORD list_i, DWORD mode
) {

	BOOL saved_mode = mode;
	if (saved_mode == MODE_ROOT || saved_mode == MODE_PALETTE
		|| saved_mode == MODE_SECTION) mode = MODE_NORMAL;

	while (1) {
		BYTE type = *(ctx->nbt++);
		switch (type) {
		case NBT_End: {

			if (saved_mode == MODE_SECTION) {
				if (sctx->saved_data) update_region(ctx, sctx, palette_map);
				else fill_region(ctx, palette_map[0], sctx->subchunk_i); // TODO: this is probably wrong somehow
				ctx->covered_subchunks |= 1 << (sctx->subchunk_i + 4);
			}

			return; // end of compound nbt
		}
		case NBT_Byte: {
			DWORD step = get_2_bytes(ctx->nbt, 0);

			if (saved_mode == MODE_SECTION && DETECT_Y) sctx->subchunk_i = ctx->nbt[3];


			ctx->nbt += 2 + step + 1;
			break;
		}
		case NBT_Short: {
			DWORD step = get_2_bytes(ctx->nbt, 0);
			ctx->nbt += 2 + step + 2;
			break;
		}
		case NBT_Int: {
			DWORD step = get_2_bytes(ctx->nbt, 0);
			ctx->nbt += 2 + step + 4;
			break;
		}
		case NBT_Long: {
			DWORD step = get_2_bytes(ctx->nbt, 0);
			ctx->nbt += 2 + step + 8;
			break;
		}
		case NBT_Float: {
			DWORD step = get_2_bytes(ctx->nbt, 0);
			ctx->nbt += 2 + step + 4;
			break;
		}
		case NBT_Double: {
			DWORD step = get_2_bytes(ctx->nbt, 0);
			ctx->nbt += 2 + step + 8;
			break;
		}
		case NBT_Byte_Array: {
			DWORD step = get_2_bytes(ctx->nbt, 0);
			ctx->nbt += 2 + step;
			DWORD length = get_4_bytes(ctx->nbt, 0);
			ctx->nbt += 4 + length * 1;
			break;
		}
		case NBT_Int_Array: {
			DWORD step = get_2_bytes(ctx->nbt, 0);
			ctx->nbt += 2 + step;
			DWORD length = get_4_bytes(ctx->nbt, 0);
			ctx->nbt += 4 + length * 4;
			break;
		}
		case NBT_Long_Array: {
			DWORD step = get_2_bytes(ctx->nbt, 0);
			ctx->nbt += 2 + step;
			DWORD length = get_4_bytes(ctx->nbt, 0);

			// save data pointer if inside block_states
			if (mode == MODE_BLOCK_STATES) {
				sctx->saved_data = ctx->nbt + 4;
				sctx->saved_data_len = length;
			}

			ctx->nbt += 4 + length * 8;
			break;
		}
		case NBT_Compound: {
			DWORD step = get_2_bytes(ctx->nbt, 0);

			// detect older region versions (pre-1.13)
			if (saved_mode == MODE_ROOT && DETECT_LEVEL) ctx->status |= CHUNK_STATUS_OLD;

			DWORD inp_mode = mode;
			if (saved_mode == MODE_SECTION && DETECT_BLOCK_STATES) inp_mode = MODE_BLOCK_STATES;
			ctx->nbt += 2 + step;
			nbt_compound_track(ctx, sctx, palette_map, -1, inp_mode);
			break;
		}
		case NBT_String: {
			DWORD step = get_2_bytes(ctx->nbt, 0);

			// not necessary, just a precaution
			int detected_status = saved_mode == MODE_ROOT && DETECT_STATUS;

			ctx->nbt += 2 + step;
			step = get_2_bytes(ctx->nbt, 0);

			if (saved_mode == MODE_PALETTE) {
				// map value of Name to user-defined global palette ID
				palette_update(ctx->nbt + 2, step, list_i, palette_map, ctx);
				sctx->palette_len++; // increment palette_len
			}
			else if (saved_mode == MODE_ROOT && detected_status){

				// detect incomplete chunks
				if (DETECT_INCOMPLETE_STATUS) ctx->status |= CHUNK_STATUS_INCOMPLETE;
			}

			ctx->nbt += 2 + step;
			break;
		}
		case NBT_List: {
			DWORD step = get_2_bytes(ctx->nbt, 0);

			// if we detect a list in block_states it's palette
			DWORD inp_mode = mode;
			if (mode == MODE_BLOCK_STATES) {
				inp_mode = MODE_PALETTE;
				sctx->palette_len = 0; // reset palette_len
			}
			if (saved_mode == MODE_ROOT && DETECT_SECTIONS) {
				inp_mode = MODE_SECTION;
			}

			ctx->nbt += 2 + step;
			DWORD list_type = *(ctx->nbt++);
			DWORD length = get_4_bytes(ctx->nbt, 0);

			ctx->nbt += 4;
			nbt_list_track(ctx, sctx, list_type, length, palette_map, inp_mode);
			break;
		}
		default: {
			printf("Invalid NBT!\n");
			return;
		}
		}
	}
}



void update_region(PChunkContext ctx, PSubchunkContext sctx, BYTE* palette_map) {

	if (sctx->subchunk_i < -4 || sctx->subchunk_i > 19) return; // block invalid chunks

	DWORD i = ctx->chunk_i * MAX_SUBCHUNKS + sctx->subchunk_i + 4;
	BYTE* chunk_base = ctx->region_data + i * BLOCKS_PER_SUBCHUNK;

	//printf("Palette len: %L\n", sctx->palette_len);

	DWORD high_idx = sctx->palette_len - 1; // highest index for an item in the palette

	// get palette packed length + mask
	high_idx = high_idx >= 8 ? high_idx : 8; // fake 4-bit index
	int bit_len = 0;
	unsigned long long mask = 0;
	while (high_idx)
	{
		bit_len++;
		mask <<= 1;
		mask++;
		high_idx >>= 1;
	}
	unsigned long long org_mask = mask;

	//printf("palette_len=%d bit_len=%d ID=%d\n", sctx->palette_len, bit_len, sctx->subchunk_i);

	// TODO: can do without c (calculate stuff first)
	int c = 0; // counter for additions to memory
	int packed_items_len = 64 / bit_len;
	int stop = 0;
	for (int j = 0; j < sctx->saved_data_len; j++) {

		// get the next package (long long containing items)
		unsigned long long package = get_8_bytes(sctx->saved_data + j, 0);
		mask = org_mask;
		for (int k = 0; k < packed_items_len; k++) {

			// stop when we have completed the subchunk (don't include padding)
			if (c == BLOCKS_PER_SUBCHUNK) {
				stop = 1;
				break;
			}

			DWORD value = (package & mask) >> (k * bit_len);
			// add a block + shift mask
			if (value >= sctx->palette_len) {

				// TODO: maybe special return / check this
				printf("going over palette bounds!!! %L %L %d\n", value, // TODO: finally maybe remove lol
					sctx->palette_len, sctx->subchunk_i); // ^^^ could keep in to account for dodgy worldgen tools
			}
			else {
				chunk_base[c++] = palette_map[value];
			}
			mask <<= bit_len;
		}
		if (stop) break;
	}
	/*int c = 0;
	int values_per_long = 64 / bit_len;
	for (int j = 0; j < sctx->saved_data_len; j++) {

		unsigned long long data = get_8_bytes(sctx->saved_data+j, 0);

		for (int k = 0; k < values_per_long; k++) {

			if (c == BLOCKS_PER_SUBCHUNK)
				break;

			int bit_offset = k * bit_len;

			DWORD value = (data >> bit_offset) & ((1ULL << bit_len) - 1);

			if (value >= sctx->palette_len) {
				printf("going over palette bounds!!! %L %L %d\n",
					value, sctx->palette_len, sctx->subchunk_i);
			}
			else {
				chunk_base[c++] = palette_map[value];
			}
		}
	}*/

}

// fill a region with one block id
void fill_region(PChunkContext ctx, BYTE id, char subchunk_i) {

	if (subchunk_i < -4 || subchunk_i > 19) return; // block invalid chunks

	DWORD i = ctx->chunk_i * MAX_SUBCHUNKS + subchunk_i + 4;

	// maybe cast as DWORD* etc.
	BYTE* chunk_base = ctx->region_data + i * BLOCKS_PER_SUBCHUNK;
	memset(chunk_base, id, BLOCKS_PER_SUBCHUNK); // TODO: check this works!
	//for (int j = 0; j < BLOCKS_PER_SUBCHUNK; j++) {
	//	chunk_base[j] = id;
	//}
}

// fill any empty chunk with the default-valued block
BYTE* fill_empty_chunk(PChunkContext ctx, BYTE default_id) {

	DWORD blocks_per_chunk = BLOCKS_PER_SUBCHUNK * MAX_SUBCHUNKS;

	// maybe cast as DWORD* etc.
	BYTE* chunk_base = ctx->region_data + ctx->chunk_i * blocks_per_chunk;
	//BYTE default_id = ctx->default_palette_value;
	memset(chunk_base, default_id, blocks_per_chunk); // TODO: check this works!
	//for (int j = 0; j < blocks_per_chunk; j++) {
	//	chunk_base[j] = default_id;
	//}
	return chunk_base;
}

// check for pathological i?
void palette_update(char* str, DWORD size, DWORD i, BYTE* palette_map, PChunkContext ctx) {
	BYTE saved = str[size];
	str[size] = 0;

	BYTE id = binary_search_strings(ctx->user_palette, ctx->user_palette_len,
		str + 10, // skip "minecraft:"
		ctx->default_palette_value // default value
	);
	str[size] = saved; // don't actually need to restore
	palette_map[i] = ctx->palette_secondary_translation[id];
}

int binary_search_strings(char* arr[], int count, char* target, int fail)
{
	int left = 0;
	int right = count - 1;

	while (left <= right)
	{
		int mid = left + (right - left) / 2;
		int cmp = wildcard_strcmp(arr[mid], target); // using wildcard ones now

		if (cmp == 0)
			return mid; // found

		if (cmp < 0)
			left = mid + 1;
		else
			right = mid - 1;
	}

	return fail; // not found
}

// TODO: maybe eventually do the better comp
inline int wildcard_strcmp(const char* a, const char* b) // ~~~~!
{
	for (;; ++a, ++b)
	{
		unsigned char ca = *a;
		unsigned char cb = *b;

		if (ca == '*' || cb == '*')
			return 0; // treat wildcard as if both strings ended here

		if (ca != cb)
			return (ca < cb) ? -1 : 1;

		if (ca == '\0')
			return 0;
	}
}



// TODO: unfuck 2's complement stuff here
DWORD get_2_bytes(BYTE* data, DWORD i) {
	DWORD v = data[i] * 0x100 + data[i + 1];
	//if (v & 0x8000) // ~ GPT
	//	v |= 0xFFFF0000;
	return v;
}

DWORD get_3_bytes(BYTE* data, DWORD i) {
	DWORD v = data[i] * 0x10000 +
		data[i + 1] * 0x100 + data[i + 2];
	//if (v & 0x800000) // ~ GPT
	//	v |= 0xFF000000;
	return v;
}

DWORD get_4_bytes(BYTE* data, DWORD i) {
	return data[i] * 0x1000000 + data[i + 1] * 0x10000 +
		data[i + 2] * 0x100 + data[i + 3];
}

// this is fine
unsigned long long get_8_bytes(BYTE* data, DWORD i)
{
	return ((unsigned long long)data[i] << 56) |
		((unsigned long long)data[i + 1] << 48) |
		((unsigned long long)data[i + 2] << 40) |
		((unsigned long long)data[i + 3] << 32) |
		((unsigned long long)data[i + 4] << 24) |
		((unsigned long long)data[i + 5] << 16) |
		((unsigned long long)data[i + 6] << 8) |
		((unsigned long long)data[i + 7]);
}

void get_chunks(BYTE* data) {

	// traverse chunk offset table
	for (int i = 0; i < MAX_CHUNKS; i++) {
		// big endian
		DWORD offset = get_3_bytes(data, i * 4);

	}
}

void invalidate_chunk(PChunkContext ctx, int error_code) {

	// TODO: do more efficiently (just set first and second bytes)
	BYTE* chunk_base = fill_empty_chunk(ctx, 0);
	for (int i = 0; i < MAX_SUBCHUNKS; i++) {

		chunk_base[i * BLOCKS_PER_SUBCHUNK + 1] = error_code;

		// ensure we have the right lua files
		chunk_base[i * BLOCKS_PER_SUBCHUNK + 2] = VERSION;
	}
}