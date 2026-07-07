
/*

TODO:
	- colours
	- debug / non debug print + error print
	- select correct first region + nicer load (DONE)
	- initial blocking (if user is in different saved location)
	- ^^^ MORE BLOCKING NEEDED
	- write to mod files automatically == lua file construction (DONE)
	- tutorial (to be done in FBW)
	- code cleanup
	- WIDE STRING FILE HANDLING!!!!!!!!!!!!!!! (maybe kind of done mostly BUT CHECK)

MAYBE TODO:
	- eliminate stdlib stuff (done, but make funcs better)
	- remove some / all globals (not needed really)
	- recovery
	- minecraft share mode injection
	- multiple FBW support
	- x32 / x64 detection + steam path extraction from reg. key
	- maintain table of non-dead TIDs so we can display REJOIN message (N/A)
*/

//#define PACKED
//#define DEBUG

#include "nbt.h"
#include "comm.h"
#include "config.h"
#include "finder.h"

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
#define Z_OK 0

#define BAD_CHUNK_EMPTY 0
#define BAD_CHUNK_LOADING 1
#define BAD_CHUNK_BLOCKED 2
#define BAD_CHUNK_HOLLOW 3

static RegionAllocContext rctx;
static RegionAllocInfo rinfo;
static BYTE* bad_chunks[4];
static int center_chunk[2];
static SRWLOCK center_chunk_srw;
static BOOL center_chunk_waiting;

static char* global_palette;
static DWORD global_palette_len;
static BYTE* global_mapping;
static BYTE global_default;
static BYTE global_air_id;

static char* global_region_path;

static int(__cdecl* uncompress)(
	unsigned char* dest, unsigned long* destLen,
	const unsigned char* source, unsigned long sourceLen);

/*int recover() { // TODO: make less bad etc.

	void* addr = 0x0074DA53; // table address
	DWORD table[18];

	// uses old function
	HANDLE hCurProc = GetProcessHandleByWindowTitle("Fractal Block World");

	printf("Recovery initiated");
	ReadProcessMemory(hCurProc, addr, table, sizeof(table), NULL);

	for (int i = 0; i < 6; i++) {
		HANDLE event = table[i * 3 + 2];
		if (event) {
			if (DuplicateHandle(hCurProc, event, NULL, NULL, 0, FALSE, DUPLICATE_CLOSE_SOURCE)) {
				printf("Closed handle %p\n", event);
			}
		}
	}
	DuplicateHandle(hCurProc, 0xF10, NULL, NULL, 0, FALSE, DUPLICATE_CLOSE_SOURCE);
}*/

int _main();
void main() {

	int ret = _main();

	// so the user can see what the program said before exiting
	// ^^^ since they might not be in a terminal
	if(ret != 0) { // only do this when there was an error
		print("Press ENTER to exit...");
		char data;
		DWORD read;
		ReadConsoleA(GetStdHandle(STD_INPUT_HANDLE), &data, 1, &read, 0);
	}

	// stops the process even if COM decides to hang
	ExitProcess(ret);
}

// TODO: fix small memory leak
// ^^^ or just do more testing
int _main() {

	// MORE TODO:
	// - correct region initial selection DONE
	// - per-thread initial blocking KINDA DONE
	// - FBW exit detection (done but can't be in remove code at least for now)
	// ^^^ could have 2 separate monitors, one in remove code and one at the start
	// - maybe more block validation (like e.g. y coord not crazy in partner code) ~
	// - more error chunks:
	//    * (0 --> FBWMine not connected)
	//    * 1 --> not loaded in time
	//    * 2 --> region empty
	//    * 3 --> region open blocked by minecraft (+ maybe share mode injection note)
	//    * naughty / not present subchunk can just be empty

	stdfuncs_init();

	SetConsoleTitleA("FBW <---> Minecraft");
	banner();

	// get config
	ConfigData conf;
	if (!get_good_config(&conf)) return 1;

	/*printf("Palette:\n");
	for (int i = 0; i < conf.out_palette_len; i++) {
		printf("%s\n", conf.out_palette[i]);
	}

	printf("Mappings:\n");
	for (int i = 0; i < conf.out_palette_len; i++) {
		printf("%L\n", conf.out_mapping[i]);
	}
	printf("With default block id %L\n", conf.out_default_id);

	printf("Block names:\n");
	printf("<empty>\n");
	for (int i = 1; i < conf.out_block_names_len; i++) {
		printf("%s\n", conf.out_block_names[i]);
	}
	return 0;*/

	global_region_path = conf.minecraft_region_path;
	uncompress = conf.zlib_uncompress;
	//return;
	//printf("region: %s, coords: %d, %d, %d\n", global_region_path,
	//	conf.spawn_coords[0], conf.spawn_coords[1], conf.spawn_coords[2]);

	//return 0;

	// (Minecraft) X Y Z --> Z X Y (FBW)
	int tmp = conf.spawn_coords[0];
	conf.spawn_coords[0] = conf.spawn_coords[2];
	conf.spawn_coords[2] = conf.spawn_coords[1];
	conf.spawn_coords[1] = tmp;

	if (!write_mod_file(&conf)) return 1;

	print("[+] Updated mod lua file\n");

	global_palette = conf.out_palette;
	global_palette_len = conf.out_palette_len;
	global_mapping = conf.out_mapping;
	global_default = 0; // maps to default_id block
	
	global_air_id = 255;
	for (int i = 0; i < conf.out_palette_len; i++) {
		if (strcmp(conf.out_palette[i], "air") == 0) {
			global_air_id = conf.out_mapping[i];
			break;
		}
	}

	if (global_air_id == 255) {
		printf("Warning: air not found (block set to INVALID)\n");
	}
	
	// init regions
	init_regions_and_load_spawn(conf.spawn_coords[1], conf.spawn_coords[0]);

	// allocate memory for bad chunks
	// TODO: could do in one go
	for (int i = 0; i < 4; i++) {

		bad_chunks[i] = malloc(4096);
		if (!bad_chunks[i]) {
			printf("malloc failed!\n");
			return 1;
		}

		memset(bad_chunks[i], 0, 4096);

		bad_chunks[i][1] = i + 1;
	}
	bad_chunks[3][1] = 6; // for chunks in hollow regions

	HANDLE hCurProc = find_fbw(conf.fbw_path);
	if (!hCurProc) return 1;
	
	comm_test(hCurProc, &conf.v);

	return 0;
}

int write_mod_file(PConfigData conf) {

	// get the BASE FBW path
	DWORD length = strlen(conf->fbw_path);
	char saved = conf->fbw_path[length - 25];
	conf->fbw_path[length - 25] = 0;

	char mod_file_path[MAX_PATH * 2 + 100]; // over approx
	sprintf(mod_file_path,
		"%s\\Input\\Packages\\%s\\WorldNodes\\Helpers\\fbwmine_data.lua",
		conf->fbw_path, conf->mod_name);
	mod_file_path[MAX_PATH] = 0; // trim the path

	conf->fbw_path[length - 25] = saved; // reset fbw_path

	HANDLE hLuaFile = CreateFileA(mod_file_path, FILE_ALL_ACCESS, 
		0, 0, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (hLuaFile == INVALID_HANDLE_VALUE) {
		printf("[-] Opening mod lua file failed\n");
		return 0;
	}

	int result = 1; // result of the writes

	// write spawn path
	int adds[] = { 0x8000000, 0x8000000, 0x1111140 };
	result &= fprint(hLuaFile, "--THIS FILE IS OVERWRITTEN ENTIRELY BY FBWMINE");
	result &= fprint(hLuaFile, "\np.spawn_path = {");
	for (int i = 0; i < 7; i++) {
		result &= fprint(hLuaFile, "{");
		for (int j = 0; j < 3; j++) {
			DWORD num = conf->spawn_coords[j] + adds[j];
			DWORD component = ((num << i * 4) & 0xf000000) >> 24;
			char num_buff[15]; // over approx
			sprintf(num_buff, "%L, ", component);
			result &= fprint(hLuaFile, num_buff);
		}
		// this is terrible: just keep track in for loop or something
		result &= SetFilePointer(hLuaFile, -2, 0, FILE_CURRENT) != INVALID_SET_FILE_POINTER;
		result &= fprint(hLuaFile, "}, ");
	}
	result &= SetFilePointer(hLuaFile, -2, 0, FILE_CURRENT) != INVALID_SET_FILE_POINTER;
	result &= fprint(hLuaFile, "}\n");

	// write block palette
	result &= fprint(hLuaFile, "p.blocks = {");
	for (int i = 1; i < conf->out_block_names_len; i++) {
		result &= fprint(hLuaFile, "\"");
		result &= fprint(hLuaFile, conf->out_block_names[i]);
		result &= fprint(hLuaFile, "\",");
	}
	result &= SetFilePointer(hLuaFile, -1, 0, FILE_CURRENT) != INVALID_SET_FILE_POINTER;
	result &= fprint(hLuaFile, "}\n");
	result &= fprint(hLuaFile, "--THIS FILE IS OVERWRITTEN ENTIRELY BY FBWMINE");

	if (!result) {
		printf("[-] Writing to mod lua file failed\n");
		return 0;
	}

	CloseHandle(hLuaFile);

	return 1;
}

BOOL init_regions_and_load_spawn(z, x) {

	// spawn coords
	int blocks_per_region_xz_axis = CHUNKS_PER_REGION_XZ_AXIS * 16;
	int region_x = (x < 0 ? x - blocks_per_region_xz_axis + 1 : x) / blocks_per_region_xz_axis;
	int region_z = (z < 0 ? z - blocks_per_region_xz_axis + 1 : z) / blocks_per_region_xz_axis;

	int chunk_x = -blocks_per_region_xz_axis * region_x + x;
	int chunk_z = -blocks_per_region_xz_axis * region_z + z;

	// TODO: store these globally or something
	int center_x = region_x + (chunk_x > 16 ? 1 : 0);
	int center_z = region_z + (chunk_z > 16 ? 1 : 0);
	center_chunk[0] = center_x;
	center_chunk[1] = center_z;

	// allocate A LOT of memory for 4 fully-loaded regions
	if (alloc_4_regions(&rctx, &rinfo)) {
		printf("Big region allocation failed!\n");
		return 0;
	}

	//printf("Center coords: %d %d\n", x, z);
	//printf("Center: %d %d\n", center_x, center_z);
	printf("Loading regions around spawnpoint...\n");

	HANDLE threads[4];
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
	
			region_x = center_x - i;
			region_z = center_z - j;

			int slot = ((region_x & 1) << 1) + (region_z & 1);
			PRegionInfo region_info = &rinfo.region_infos[slot];

			HANDLE thread = create_load_thread(region_x, region_z, region_info);
			if (!thread) {
				printf("Starting initial region load thread failed!\n");
				return 0;
			}
			threads[slot] = thread;
		}
	}

	if (WaitForMultipleObjects(4, threads, TRUE, INFINITE) == WAIT_FAILED) {
		printf("Waiting for initial load threads failed with error %L\n", GetLastError());
		return 0;
	}

	//wait_and_load_region_from_coords(0, 0);
	//wait_and_load_region_from_coords(0, -1);
	//wait_and_load_region_from_coords(-1, 0);
	//wait_and_load_region_from_coords(-1, -1);

	InitializeSRWLock(&center_chunk_srw);
	center_chunk_waiting = FALSE;
	
	return 1;
}

// TODO: only have one of these and IPC
void load_thread(int* coords) {
	wait_and_load_region_from_coords(coords[0], coords[1]);
	free(coords);
}

// TODO: region load failed message or something
HANDLE create_load_thread(int region_x, int region_z, PRegionInfo region_info) {
	int* coords = malloc(8); // could be pre-allocated
	if (!coords) {
		printf("malloc failed!\n");
		return 0;
	}
	coords[0] = region_x;
	coords[1] = region_z;
	region_info->status = REGION_STATUS_WAITING_FOR_UPDATE;
	return CreateThread(NULL, NULL, 
		(LPTHREAD_START_ROUTINE)load_thread, coords, NULL, NULL);
}

// TODO: customise input somewhere else
void wait_and_load_region_from_coords(int x, int z) {
	char path[MAX_PATH];
	char* base = global_region_path;


	snprintf(path, MAX_PATH, "%s\\r.%d.%d.mca", base, z, x); // swapped to map coords properly
#ifdef _DEBUG
	printf("Trying to open %s\n", path);
#endif
	int slot = ((x & 1) << 1) + (z & 1);
	BYTE* region_data = rctx.regions[slot];
	PRegionInfo region_info = &rinfo.region_infos[slot];
	
	DWORD size = 0;
	SetLastError(0);
	DWORD* res = read_file(path, &size);
	if (!res || !size) {

		DWORD err = GetLastError();

		AcquireSRWLockExclusive(&region_info->srw);

		region_info->x = x;
		region_info->z = z;
		region_info->status = err == ERROR_SHARING_VIOLATION ?
			REGION_STATUS_READ_BLOCKED : 
			((res && !size) ? REGION_STATUS_HOLLOW : REGION_STATUS_EMPTY);

		ReleaseSRWLockExclusive(&region_info->srw);

		char msg[30]; // over approx
		strcpy(msg, "Loaded region %d %d ");
		strcat(msg, err == ERROR_SHARING_VIOLATION ? "(blocked)" :
			((res && !size) ? "(hollow)" : "(empty)"));
		strcat(msg, "\n");
		printf(msg, z, x);
		return;
	}
	BYTE* palette_map = malloc(0x1000); // max palette size
	
	AcquireSRWLockExclusive(&region_info->srw);

	load_region(&rctx, region_data, global_palette, global_palette_len, 
		palette_map, res, global_mapping, global_default, global_air_id);
	region_info->x = x;
	region_info->z = z;
	region_info->status = REGION_STATUS_AVAILABLE;

	ReleaseSRWLockExclusive(&region_info->srw);
	printf("Loaded region %d %d\n", z, x);

	free(palette_map);
}

// TODO: call with partner ID + struct with some relevant info in
int wait_and_write_subchunk(int x, int y, int z, HANDLE proc, BYTE* addr, int block) {

	int region_x = (x < 0 ? x - CHUNKS_PER_REGION_XZ_AXIS + 1 : x) / CHUNKS_PER_REGION_XZ_AXIS; // negative fix
	int region_z = (z < 0 ? z - CHUNKS_PER_REGION_XZ_AXIS + 1 : z) / CHUNKS_PER_REGION_XZ_AXIS;

	int saved_region_x = region_x;
	int saved_region_z = region_z;

	int chunk_x = -CHUNKS_PER_REGION_XZ_AXIS * region_x + x;
	int chunk_z = -CHUNKS_PER_REGION_XZ_AXIS * region_z + z;

	//printf("chunk %d %d from region %d %d requested xyz=%d %d %d\n", chunk_x, chunk_z, region_x, region_z, x, y, z);

	int slot = ((region_x & 1) << 1) + (region_z & 1);
	BYTE* region_data = rctx.regions[slot];
	PRegionInfo region_info = &rinfo.region_infos[slot];

	AcquireSRWLockShared(&region_info->srw);

	// region is good
	DWORD i = chunk_x * 768 + chunk_z * 24 + y;
	BYTE* chunk_base = region_data + i * 4096;

	if (region_info->status == REGION_STATUS_WAITING_FOR_UPDATE ||
		region_info->x != region_x ||
		region_info->z != region_z
		) {
		// TODO: block
		//printf("<would block here>\n");
		chunk_base = bad_chunks[BAD_CHUNK_LOADING];
	} else if (region_info->status == REGION_STATUS_EMPTY //||
		//region_info->x != region_x ||
		//region_info->z != region_z
	) {
		// TODO: message worker
		//printf("<would message worker here>\n");
		chunk_base = bad_chunks[BAD_CHUNK_EMPTY];
	}
	else if (region_info->status == REGION_STATUS_READ_BLOCKED) {

		// new, just useful for user to know minecraft is blocking the chunk
		chunk_base = bad_chunks[BAD_CHUNK_BLOCKED];
	}
	else if (region_info->status == REGION_STATUS_HOLLOW) {

		// also new, sometimes minecraft just leaves hollow regions
		chunk_base = bad_chunks[BAD_CHUNK_HOLLOW];
	}

	// TODO: move a large portion of this out of this function
	int waited = 0;

	// always wait if we are blocking and the region is still loading
	int force_wait = chunk_base[0] == 0 && chunk_base[1] == 2 && block;
	if (force_wait || !center_chunk_waiting) {
		AcquireSRWLockShared(&center_chunk_srw);

		int xdiff = region_x * CHUNKS_PER_REGION_XZ_AXIS
			+ chunk_x - center_chunk[0] * CHUNKS_PER_REGION_XZ_AXIS;
		int zdiff = region_z * CHUNKS_PER_REGION_XZ_AXIS
			+ chunk_z - center_chunk[1] * CHUNKS_PER_REGION_XZ_AXIS;

		int bound = 30;// 29/30 for rd=7; 24 for rd=4/5

		// get the max diff (mainly for printing purposes)
		int xdiff_abs = xdiff < 0 ? -xdiff : xdiff;
		int zdiff_abs = zdiff < 0 ? -zdiff : zdiff;
		int max_diff = xdiff_abs < zdiff_abs ? zdiff_abs : xdiff_abs;
		//if (xdiff < -bound || xdiff > bound || zdiff < -bound || zdiff > bound) {
		if(max_diff > bound) {
			center_chunk_waiting = TRUE;
			ReleaseSRWLockShared(&center_chunk_srw);
			AcquireSRWLockExclusive(&center_chunk_srw);

			//printf("Detected outside bounds %d %d\n", xdiff, zdiff);
			printf("Chunk requested %d chunks away from center; recentering...\n", max_diff);
			//MessageBoxA(0, "asd", "asd", 0);
			center_chunk[0] = region_x + (chunk_x > 16 ? 1 : 0);
			center_chunk[1] = region_z + (chunk_z > 16 ? 1 : 0);

			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < 2; j++) {

					// check a region that should be loaded
					region_x = center_chunk[0] - i;
					region_z = center_chunk[1] - j;

					// get its slot
					slot = ((region_x & 1) << 1) + (region_z & 1);
					PRegionInfo region_info = &rinfo.region_infos[slot];

					// if the recorded region doesn't match, reload
					if (region_info->x != region_x || region_info->z != region_z) {
						
						// TODO: error handling
						// previous error handling would still break stuff
						// could just add custom message like 'region load failed'
						HANDLE load_thread = create_load_thread(region_x, region_z, region_info);
						if (block && region_x == saved_region_x && region_z == saved_region_z) {

							// TODO: different function which makes this less bad
							ReleaseSRWLockShared(&region_info->srw);

							printf("[PARTNER] blocking on first update!\n");
							if (WaitForSingleObject(load_thread, INFINITE) != WAIT_OBJECT_0) {
								printf("Partner wait failed with error %L\n", GetLastError());
							}

							waited = 1;

							// TODO: could do stuff here e.g. below + more cases
							/*AcquireSRWLockShared(&region_info->srw);

							// reset this, it is now probably valid
							chunk_base = region_data + i * 4096;

							// another check: race condition means we need to check this
							// MAYBE TODO: eliminate this race condition
							if (region_info->status == REGION_STATUS_WAITING_FOR_UPDATE ||
								region_info->x != region_x ||
								region_info->z != region_z
								) {

								// could have specific thrashing message
								chunk_base = bad_chunks[BAD_CHUNK_LOADING];
							}*/
						}
					}
				}
			}
			//load_thread

			center_chunk_waiting = FALSE;
			ReleaseSRWLockExclusive(&center_chunk_srw);
		}
		else ReleaseSRWLockShared(&center_chunk_srw);
	}

	if(waited) return 1; // do another call to the function

	if (!WriteProcessMemory(proc, addr, chunk_base, 4096, NULL)) {
		printf("WriteProcessMemory to chunk failed (%L)", GetLastError());
	}

	ReleaseSRWLockShared(&region_info->srw);

	return 0;
}

void load_region(PRegionAllocContext rctx, BYTE* base, char** global_palette, 
	DWORD palette_len, BYTE* palette_map, DWORD* data, BYTE* mapping, BYTE default_val, BYTE air_id) {

	ChunkContext ctx;
	ctx.region_data = base;
	ctx.user_palette = global_palette;
	ctx.user_palette_len = palette_len;
	ctx.default_palette_value = default_val;
	ctx.palette_secondary_translation = mapping;
	
	// removed the below print for now TODO: decide what to do here
	//printf("Loading region...\n");
	int naughty = 0;
	for (int i = 0; i < MAX_CHUNKS; i++)
		naughty += get_chunk(data, palette_map, i, &rctx, &ctx, air_id);

	if (naughty != 0) {
#ifdef DEBUG
		printf("Detected and fixed %d naughty subchunks.\n", naughty);
#endif
	}

#ifdef DEBUG
	printf("Scanning... ");
	for (int i = 0; i < REGION_DATA_SIZE; i++) {
		if (!base[i]) {
			printf("Invalid %L\n", i);
			free(data);
			return 1;
		}
	}
	printf("Valid!\n");
#endif

	free(data);
}

// TODO: set chunk error type on any return
int get_chunk(BYTE* data, BYTE* palette_map, int i, PRegionAllocContext rctx, 
	PChunkContext ctx, BYTE air_id) {

	// TODO: convert back to DWORD lol
	LONG sec_offset = (LONG)get_3_bytes(data, i * 4); // TODO: make DWORDS longs
	DWORD sec_size = data[i * 4 + 3];

	if (sec_offset == 0 || sec_size == 0) {

		// fill empty chunk with default-valued block
		ctx->chunk_i = i;

		// 4 = subchunk in non-existent chunk in existent region
		invalidate_chunk(ctx, 4);

		return 0;
	}

	// this assumes the chunk is within the loaded file bounds
	// TODO: verify stuff is within bounds
	BYTE* chunk = data + sec_offset * SECTOR_SIZE;
	DWORD size = get_4_bytes(chunk, 0);
	DWORD compression_type = chunk[4];

	if (compression_type != COMPRESSION_ZLIB) {
		printf("Compression isn't zlib! %L\n", compression_type);
		return 0;
	}

	if (size == 0 || size == 1) {
		printf("Chunk has no data!\n");
		// TODO: return 0 ???
	}

	DWORD nbt_max_size = 0x100000;
	void* nbt = malloc(nbt_max_size); // 1MB for now; LOOK AT GZIP
	if (uncompress(nbt, &nbt_max_size, chunk + 5, size - 1) != Z_OK) {
		printf("NBT buffer too small!\n", nbt);// buffer at: %p\n", nbt);
		return 0;
		//MessageBoxA(0, "asd", "asd", 0);
	}

	//printf("Extracted %L bytes of NBT data\n", nbt_max_size);
	void* org_nbt = nbt;

	ctx->nbt = (BYTE*)nbt + 3;
	ctx->chunk_i = i;
	ctx->covered_subchunks = 0;
	ctx->status = CHUNK_STATUS_OK;

	//printf("Chunk index: %L, NBT: %p\n", i, ctx->nbt-3);

	SubchunkContext sctx;
	//printf("Loading chunk %d\n", ctx->chunk_i);
	nbt_compound_track(ctx, &sctx, palette_map, -1, MODE_ROOT);
	free(org_nbt);

	if ((DWORD)ctx->nbt - (DWORD)org_nbt != nbt_max_size) {
		printf("Didn't consume all of NBT!\n");
		return 0;
	}

	if (ctx->status & CHUNK_STATUS_OLD) {

		// 7 = pre-1.13 chunk
		invalidate_chunk(ctx, 7);
		return 0;
	}
	// check if chunk status is not full i.e. incomplete
	else if (ctx->status & CHUNK_STATUS_INCOMPLETE) {

		// 5 = subchunk in non-existent chunk in existent region
		invalidate_chunk(ctx, 5);
		return 0;
	}

	// fix empty sections TODO: VERIFY THESE ARE ACTUALLY EMPTY
	int naughty = 0;
	for (int i = 0; i < MAX_SUBCHUNKS; i++) {
		if (!(ctx->covered_subchunks & 1 << i)) {
			fill_region(ctx, air_id, i - 4); // NOT default value, at least for now
			// ^^^ different from size 0 chunk handling
			naughty++;
		}
	}

	return naughty;
}

int alloc_4_regions(PRegionAllocContext rctx, PRegionAllocInfo rinfo) { // ~400MB

	void* base = VirtualAlloc(NULL, REGION_DATA_SIZE * 4,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!base) return 1;
	memset(base, 0, REGION_DATA_SIZE * 4);

	for (int i = 0; i < 4; i++) {
		rctx->regions[i] = (DWORD)base + i * REGION_DATA_SIZE;
		rinfo->region_infos[i].status = REGION_STATUS_EMPTY;
		InitializeSRWLock(&rinfo->region_infos[i].srw);
	}
	rctx->allocated = 0;

	return 0;
}

// process some data in the worker pipe (which should be a message)
int process_worker_msg(PWorkerMsg pw, DWORD read, DWORD create_id,
    HANDLE proc, DWORD* base, HANDLE sem, LPCRITICAL_SECTION pcs) {

    if (read == 0) {
        printf("Pipe closed!\n");
        return 1;
    }
    else if (read != sizeof(WorkerMsg)) {
        printf("Incomplete read!\n");
        return 1;
    }

    WorkerMsg w = *pw;
    //printf("Got worker input\n");

    if (w.type == CREATE_MSG) {
        //printf("create %p, %L, %L, %p\n",
        //    w.create_msg.base, w.create_msg.index, w.create_msg.tid, w.create_msg.event);

		// TODO: maybe change
		//InterlockedExchange(&workers_blocking, 0);

        HANDLE pLocalEvent = 0;
        if (!DuplicateHandle(proc, w.create_msg.event, GetCurrentProcess(),
            &pLocalEvent, 0, FALSE, DUPLICATE_SAME_ACCESS)) {

            printf("DuplicateHandleA failed (%L)\n", GetLastError());
            return 1;
        }

        // wait for a slot in the table to be free
        WaitForSingleObject(sem, INFINITE);

        // create a partner thread for the new thread
        HANDLE partner_handle = create_partner_death_pair(proc,
            pLocalEvent, create_id, sem, w.create_msg.tid, base, pcs);

        // get the base of the table
        if (!base) base = w.create_msg.base;

        // modify table for thread
        add_table_entry(proc, base, w.create_msg.tid, // TODO: dynamically reconstruct table
            (DWORD)partner_handle, (DWORD)w.create_msg.event);

        // release thread
        SetEvent(pLocalEvent);
    }
    else if (w.type == REMOVE_MSG) {
        printf("unexpected message\n");
        //remove_table_entry(proc, base, w.remove_msg.tid);
    }
    else {
        printf("unexpected message\n");
    }
    return 0;
}

int comm_test(HANDLE proc, PFBWVersion v) {

	HANDLE pInput = 0;
	HANDLE pOutput = 0;
	HANDLE pOutputRemote = 0;
	if (!make_IPC_pipe(proc, &pInput, &pOutput, &pOutputRemote)) return 1;
	//printf("Remote input handle: %p\n", pOutputRemote);

	if (!add_inline_partner_hook(proc, pOutputRemote, v)) {
		printf("WriteProcessMemory failed\n");
	}
	printf("Added inline partner hook!\n");

	HANDLE* threads = (HANDLE*)calloc(MAX_TABLE_ENTRIES, sizeof(HANDLE));
	DWORD* tids = (DWORD*)calloc(MAX_TABLE_ENTRIES, sizeof(DWORD));
	if (!threads || !tids) {
		printf("calloc failed!\n");
		return 1;
	}

	HANDLE sem = CreateSemaphoreA(NULL, MAX_TABLE_ENTRIES, MAX_TABLE_ENTRIES, NULL);
	if (!sem) {
		printf("CreateSemaphoreA failed!\n");
		return 1;
	}

	// TODO: use a queue maybe instead
	CRITICAL_SECTION cs; // seine wonne, seine pein
	InitializeCriticalSection(&cs);

	// make a death monitor for the process
	// NOTE: I wouldn't have so many extra threads if 
	//  WaitForMultipleObjects actually worked properly
	HANDLE death_thread = CreateThread(0, 0, 
		(LPTHREAD_START_ROUTINE)process_monitor, proc, 0, 0);
	if (!death_thread) {
		error_print("[INIT]", "CreateThread");
		return 0;
	}

	WorkerMsg w;
	DWORD read;
	DWORD* base = 0;
	int next_param_id = 0;
	while (1) {

		if (!ReadFile(pInput, &w, sizeof(WorkerMsg), &read, NULL)) {
			printf("ReadFile failed!\n");
			return 1;
		}

		if (read == 0) {
			printf("Main pipe closed!\n");
			return 1;
		}

		if (read != sizeof(WorkerMsg)) {
			printf("Read incomplete data!\n");
			return 1;
		}

		if (w.type == CREATE_MSG && !base) base = w.create_msg.base;

		process_worker_msg(&w, read, next_param_id++, proc, base, sem, &cs);

	}
	printf("ReadFile failed (%L)\n", GetLastError());

	return 0;
}