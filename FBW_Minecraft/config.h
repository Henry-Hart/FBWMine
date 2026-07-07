
// #=====================================================#
// #                                                     #
// # config.h ==> Config file handling + startup code	 #
// #                                                     #
// #=====================================================#

#pragma once
#include "util.h"

#define CONFIG_TOKEN_NAME_1 1
#define CONFIG_TOKEN_NAME_2 2
#define CONFIG_TOKEN_NAME_3 3
#define CONFIG_TOKEN_NAME_INVALID 4
#define CONFIG_TOKEN_END 5

#define MAX_BLOCKS 250

#define CONFIG_OK 0
#define CONFIG_BAD_BLOCKS 1
#define CONFIG_BAD_SETTINGS 2
#define CONFIG_OTHER_ERROR 3

// currently we don't return a lot of these
typedef struct _ConfigData {
	char** out_palette;
	int out_palette_len;
	char** out_block_names;
	int out_block_names_len;
	BYTE* out_mapping;
	DWORD out_default_id;
	char* fbw_path;
	void* zlib_uncompress;
	char* minecraft_region_path;
	char* settings_path;
	FBWVersion v;
	int spawn_coords[3];
	char* mod_name;
} ConfigData, * PConfigData;

typedef struct _ConfigToken {
	int type;
	char* data;
} ConfigToken, * PConfigToken;

typedef struct _PaletteMapping {
	char* name;
	DWORD i;
} PaletteMapping, * PPaletteMapping;

void insert_into_palette(PPaletteMapping palette, int size, char* str, DWORD i);
DWORD add_to_block_table(char** table, char* str);
char* get_fbw_path(PConfigData conf, char* fbw_txt_path);

// TODO: work over MAX_PATH
// also detect unicode and say this won't work
// TODO: track line numbers
// TODO: maybe more fine grained malloc
// TODO: ^^^ or just do the removal of dodgy stuff IN THE TOKENISER
int get_config(PConfigData conf) {

	char path[MAX_PATH];
	HMODULE me = GetModuleHandleA(NULL);
	DWORD length = GetModuleFileNameA(me, path, MAX_PATH);
	if (!length) {
		printf("[Config] GetModuleFileNameA failed (%L)", GetLastError());
		return CONFIG_OTHER_ERROR;
	}

	// set base directory to be current directory so local paths always work
	local_file_path(path, length, "");
	if (!SetCurrentDirectoryA(path)) {
		printf("[Config] SetCurrentDirectoryA failed (%L)", GetLastError());
		return CONFIG_OTHER_ERROR;
	}

	local_file_path(path, length, "blocks.txt");

	DWORD size;
	char* data = read_file_extra(path, &size);

	if (!data) return CONFIG_BAD_BLOCKS;

	// determine memory needed to tokenise
	int token_i = 0;
	int token_data_i = 0;
	int commented = 0;
	int name_num = 0;
	int whitespace = 1;
	for(DWORD i=0; i<size; i++) {
		char cur = data[i];
		if (cur == '\n') {
			commented = 0;
			name_num = 0;
			whitespace = 1;
		}
		else if (cur == '#') commented = 1;
		else if (cur == ' ' || cur == '\t') whitespace = 1;
		else if (cur == '\r') {} // ignore
		else if (!commented) {
			if (whitespace) {
				whitespace = 0;
				name_num++;
				token_i++;
			}
			token_data_i++;
		}
	}

	if (token_i < 2) {
		printf("[Config] No readable data found!\n");
		return CONFIG_BAD_BLOCKS;
	}

	PConfigToken tokens = malloc((token_i + 3) * sizeof(ConfigToken)); // 3 end tokens
	char* token_data = malloc(token_data_i + token_i + 1); // 1 extra null byte
	if (!tokens || !token_data) {
		printf("[Config] malloc failed!\n");
		return CONFIG_BAD_BLOCKS;
	}
	int num_tokens = token_i;
	
	// also guess the palette size TODO: make better
	PPaletteMapping palette = malloc(token_i * sizeof(char*));
	char** block_table = calloc(1, (token_i / 2 + 1) * sizeof(char*)); // guess + 1 empty start value

	if (!palette || !block_table) {
		printf("[Config] malloc failed!\n");
		return CONFIG_BAD_BLOCKS;
	}
	palette[0].name = token_data; // points to what will be a null byte

	// tokenise
	token_i = 0;
	token_data_i = 0;
	commented = 0;
	name_num = 0;
	whitespace = 1;
	for (DWORD i = 0; i < size; i++) {
		char cur = data[i];
		if (cur == '\n') {
			commented = 0;
			name_num = 0;
			whitespace = 1;
		}
		else if (cur == '#') commented = 1;
		else if (cur == ' ' || cur == '\t') whitespace = 1;
		else if (cur == '\r') {} // ignore
		else if (!commented) {
			if (whitespace) {
				whitespace = 0;
				token_data[token_data_i++] = 0;
				tokens[token_i].type = ++name_num < CONFIG_TOKEN_NAME_INVALID ?
					name_num : CONFIG_TOKEN_NAME_INVALID;
				tokens[token_i++].data = token_data + token_data_i;
			}
			token_data[token_data_i++] = cur;
		}
	}
	token_data[token_data_i] = 0;
	tokens[token_i++].type = CONFIG_TOKEN_END;
	tokens[token_i++].type = CONFIG_TOKEN_END; // dummy token to prevent out-of-bound reads
	tokens[token_i].type = CONFIG_TOKEN_END; // ^^^ (check if needed)

	// we have tokenised so we can free the file data
	free(data);

	for (int i = 1; i < num_tokens; i++) {
		ConfigToken token = tokens[i];
		//printf("Token of type %d with data %\n", token.type, token.data);
	}

	int i = 0;
	int palette_size = 1;
	int extra_text_lines = 0;
	int not_enough_names = 0;
	int ignored_defaults = -1;
	int default_id = -1;
	while (1) {

		int set_default_id = 0;

		// read 3 tokens
		ConfigToken token1 = tokens[i++];

		// check if we are at the end
		if (token1.type == CONFIG_TOKEN_END) {
			break; // end peacefully
		}

		ConfigToken token2 = tokens[i++];
		ConfigToken token3 = tokens[i++];

		// any extra parts of the line we discard
		if (tokens[i].type == CONFIG_TOKEN_NAME_INVALID) {
			extra_text_lines++;
			while(tokens[++i].type == CONFIG_TOKEN_NAME_INVALID) {}
			continue;
		}

		// the line has 1 token
		if (token2.type == CONFIG_TOKEN_NAME_1 || token2.type == CONFIG_TOKEN_END) {
			not_enough_names++;
			i -= 2;
			continue;
		}

		// check for a third option, currently the only available one is "default"
		if (token3.type == CONFIG_TOKEN_NAME_3) {

			if (strcmp(token3.data, "default") == 0) {
				ignored_defaults++;

				set_default_id = 1;
			}
			else {
				extra_text_lines++;
				continue;
			}
		}
		else i -= 1;

		// at this point, token1 and token2 are both valid
		// TODO: use heapsort not insertion sort
		DWORD i = add_to_block_table(block_table, token2.data);
		//printf("%s ~~> %L\n", token2.data, i);
		if (i >= MAX_BLOCKS) {
			printf("[Config] Too many (>250) unique FBW blocks specified!\n");
			return CONFIG_BAD_BLOCKS;
		}
		insert_into_palette(palette, palette_size++, token1.data, i);
		if (set_default_id) default_id = i;
	}

	if (extra_text_lines)
		printf("[Config] Warning: ignored %d line(s) with too many names\n", extra_text_lines);
	if (not_enough_names)
		printf("[Config] Warning: ignored %d line(s) with too few names\n", not_enough_names);
	if (ignored_defaults > 0)
		printf("[Config] Warning: ignored %d line(s) which set a default block\n", ignored_defaults);

	if (default_id == -1) {
		printf("[Config] Error: default block not set!\n");
		return CONFIG_BAD_BLOCKS;
	}

	palette[0].i = default_id;

	int block_table_len = 1; // skip the empty start value
	while (block_table[block_table_len]) { block_table_len++; }

	char** palette_arr = malloc(palette_size * sizeof(char*));
	BYTE* mapping_arr = malloc(palette_size);
	char** opt_block_table = malloc(block_table_len * sizeof(char*));
	if (!palette_arr || !mapping_arr || !opt_block_table) {
		printf("[Config] malloc failed!\n");
	}

	for (int i = 0; i < palette_size; i++) {
		palette_arr[i] = palette[i].name;
		mapping_arr[i] = palette[i].i;
	}

	for (int i = 0; i < block_table_len; i++) {
		opt_block_table[i] = block_table[i];
	}

	free(palette);
	free(block_table);
	free(tokens);

	// TODO: have a more optimised token_data
	conf->out_palette = palette_arr;
	conf->out_mapping = mapping_arr;
	conf->out_block_names = opt_block_table;
	conf->out_default_id = default_id;
	conf->out_palette_len = palette_size;
	conf->out_block_names_len = block_table_len;


	// fbw_path.txt
	// unlike above, we actually just keep this file in memory
	local_file_path(path, length, "fbw_path.txt");
	conf->fbw_path = get_fbw_path(conf, path);
	if (!conf->fbw_path) return CONFIG_OTHER_ERROR;


	// now for the much simpler settings.txt
	// we also keep this one in memory
	
	// this is delicate, but it works here
	local_file_path(path, length, "settings.txt");
	strcpy(conf->settings_path, path); // also save this

	size = 0;
	data = read_file_extra(path, &size);
	if (!data) return CONFIG_BAD_SETTINGS;
	// TODO: spawn pos as well
	char* markers[5];
	int marker_i = 0;
	int seen_newline = 0;
	for (DWORD i = 0; i < size; i++) {
		char cur = data[i];
		if (cur == '\n') {
			seen_newline = 1;
			data[i] = 0;
			// don't overflow markers
			if (marker_i < 5) markers[marker_i++] = &data[i + 1];
			else {
				if (i == size - 1) marker_i++;
				else return CONFIG_BAD_SETTINGS;
			}
		}
		if (cur == '\r') return CONFIG_BAD_SETTINGS;
		// prevent super long first line
		if (i > 60 && !seen_newline) return CONFIG_BAD_SETTINGS;
	}
	// need to terminate final string with null byte
	if (marker_i != 6) return CONFIG_BAD_SETTINGS;

	int bounds[][2] = {
		{-30000000, 30000000},
		{-64, 320},
		{-30000000, 30000000},
	};
	for (int i = 0; i < 3; i++) {
		int converted = small_atoi(markers[i + 2]);
		if (converted == INVALID_NUMBER) { // invalid number marker
			converted = 0;
		}
		// outside world border check
		// TODO: maybe remove this to let players bypass the world border
		if (bounds[i][0] > converted || bounds[i][1] < converted) {
			return CONFIG_BAD_SETTINGS;
		}
		conf->spawn_coords[i] = converted;
	}

	conf->mod_name = markers[0];
	conf->minecraft_region_path = markers[1];

	return CONFIG_OK;
}

// loads / gets FBW path
char* get_fbw_path(PConfigData conf, char* fbw_txt_path) {

	// + 1 for null byte
	char* fbw_path = malloc(MAX_PATH + 1);
	if (!fbw_path) {
		printf("[CONFIG] malloc failed!\n");
		return 0;
	}

	// attempt to read good path
	DWORD size = 0;
	char* data = read_file_extra(fbw_txt_path, &size);
	if (!data && GetLastError() != ERROR_FILE_NOT_FOUND) {
		printf("[CONFIG] Reading FBW path file failed\n");
		return 0;
	}

	// naive attempt at tokenisation
	// lets the user have some newlines in some cases etc.
	for (int i = 0; i < size; i++) {
		if (data[i] == '\r' || data[i] == '\n') {
			data[i] = 0;
		}
	}

	if (size > 0 && size <= MAX_PATH) {

		// assume path is good; return it
		fbw_path[0] = 0;

		// this line is subtle
		// we copy at most size bytes from data,
		//  which has size=size
		// then we copy an extra null byte to terminate fbw_path
		strcpy_s(fbw_path, size + 1, data);
		free(data);

		return fbw_path;
	}

	// path is bad
	// try to open fbw_path.txt
	SetLastError(0);
	HANDLE hFBWPath = CreateFileA(fbw_txt_path, FILE_ALL_ACCESS, 0, 0, OPEN_ALWAYS, 0, 0);
	DWORD last_err = GetLastError();
	if (hFBWPath == INVALID_HANDLE_VALUE) {
		printf("[CONFIG] (Re)opening FBW path file failed\n");
		return 0;
	}
	
	// get a new path
	printf("[*] Please select your FBW base folder\n");
	MSIFILEHASHINFO hash;
	hash.dwFileHashInfoSize = sizeof(hash);
	do {
		get_folder_path(fbw_path, MAX_PATH,
			L"Please select the FBW base folder e.g. steamapps\\common\\Fractal Block World",
			L"Select FBW",
			L"FBW folder:");
		
		strcat_s(fbw_path, MAX_PATH, "\\Bin\\WindowsMinGW\\fbw.exe");
		

	} while (MsiGetFileHashA(fbw_path, 0, &hash) != ERROR_SUCCESS);

	DWORD written = 0;
	if (!WriteFile(hFBWPath, fbw_path, strlen(fbw_path), &written, 0)) {
		printf("[-] Writing to FBW path file failed; FBW path won't be saved\n");
	}
	
	CloseHandle(hFBWPath);

	return fbw_path;
}

// returns a good config or dies trying
BOOL get_good_config(PConfigData conf) {

	// supported FBW versions
	// I could read this from a file but it's somewhat too much effort
	void* table_data[][8] = {
		{1297995149, 1554413479, 2699179619, 2001087517,
			0x0074D990, 0x00a4448c, 0x00a443e3, "1.01.32"},
		{4191973557, 1997980744, 2116352146, 962159604,
			0x00754A60, 0x00a5caec, 0x00a5ca43, "1.02.00 (beta 21/06/2026)"}
	};
	PFBWVersionEntry version_table = (PFBWVersionEntry)table_data;
	int version_table_length = 2;

	BOOL settings_bad = 0;
	char settings_buff[MAX_PATH];
	conf->settings_path = settings_buff;

	// normal get config
	switch (get_config(conf)) {
	case CONFIG_OTHER_ERROR:
		return 0; // fail silently
	case CONFIG_BAD_BLOCKS:
		printf("[!] Loading blocks.txt failed :(\nYou can try to"
			" fix any errors or just re-download a fresh one\n");
		return 0;
	case CONFIG_BAD_SETTINGS:
		printf("[-] Loading settings.txt failed\n");
		settings_bad = 1;
		break;
	case CONFIG_OK:
		printf("[+] Loaded data files successfully\n");
	}

	// try to fix bad settings
	SetLastError(0);
	HANDLE hSettings = CreateFileA(conf->settings_path, FILE_ALL_ACCESS, 0, 0, OPEN_ALWAYS, 0, 0);
	DWORD last_err = GetLastError();
	if (hSettings == INVALID_HANDLE_VALUE) {
		printf("[CONFIG] (Re)opening settings file failed\n");
		return 0;
	}

	if (last_err == ERROR_ALREADY_EXISTS && settings_bad) {
		printf("[*] Settings file seems corrupt, resetting...\n");
	}
	else if (settings_bad) {
		printf("[*] No settings file found, creating one...\n");
	}

	char fbw_path[MAX_PATH];
	char region_folder[MAX_PATH];
	char mod_name[MAX_PATH]; // over approx
	if (settings_bad) {
		printf("[*] Please select your minecraft world's region folder\n");
		while(!get_folder_path(region_folder, MAX_PATH,
			L"Please select a minecraft region folder e.g. overworld\\region",
			L"Select region folder",
			L"Region folder:")) {}

		conf->minecraft_region_path = region_folder;

		int bounds[][2] = {
			{-30000000, 30000000},
			{-64, 320}, // max y is 320; maybe let users go a bit above this?
		};

		conf->spawn_coords[0] = get_block_in_world("Please enter your spawn x: ", bounds[0]);
		conf->spawn_coords[1] = get_block_in_world("Please enter your spawn y: ", bounds[1]);
		conf->spawn_coords[2] = get_block_in_world("Please enter your spawn z: ", bounds[0]);

		// for now, you can't configure mod_name from the terminal interface
		strcpy(mod_name, "ANStar_fbwmine");

		// save mod_name
		char* saved_mod_name = malloc(strlen(mod_name)+1);
		if (!saved_mod_name) {
			printf("[CONFIG] malloc failed!\n");
		}
		strcpy(saved_mod_name, mod_name);
		conf->mod_name = saved_mod_name;
	}
	else {
		printf("[*] Loading saved settings...\n");
	}

	strcpy_s(fbw_path, MAX_PATH, conf->fbw_path);

	// TODO: maybe reconstruct regardless
	if (settings_bad) {
		// reconstruct the file
		char file_data[MAX_PATH * 2 + 100]; // over approx
		file_data[0] = 0;
		strcat(file_data, "!!! IT IS NOT RECOMMENDED TO EDIT THIS FILE DIRECTLY !!!\n");
		strcat(file_data, mod_name);
		strcat(file_data, "\n");
		strcat(file_data, region_folder);


		char coords[50]; // over approx
		sprintf(coords, "\n%d\n%d\n%d\n", conf->spawn_coords[0],
			conf->spawn_coords[1], conf->spawn_coords[2]);
		strcat(file_data, coords);
		// TODO: ^^^ maybe have tutorial

		DWORD written = 0;
		if (!WriteFile(hSettings, file_data, strlen(file_data), &written, 0)) {
			printf("[-] Writing to settings file failed, your choices won't be saved\n");
		}
	}

	CloseHandle(hSettings);

	MSIFILEHASHINFO hash;
	hash.dwFileHashInfoSize = sizeof(hash);
	if (MsiGetFileHashA(fbw_path, 0, &hash) != ERROR_SUCCESS) {
		printf("[!] Checking FBW version failed, try deleting fbw_path.txt\n");
		return 0;
	}
	PFBWVersion v = get_fbw_version(hash.dwData, version_table, version_table_length);
	if (!v) {
		printf("[!] FBW version not recognised\n");
		printf("    |-> If you haven't updated your game: consider updating it\n");
		printf("    |-> If you are on the beta branch or a newly released version, "
			"this tool may not support it yet, DM @AntiNeutronicStar on discord\n");
		return 0;
	}
	else {
		printf("[+] FBW version %s detected\n", v->name);
	}
	
	conf->v.shellcode_offset = v->shellcode_offset;
	conf->v.lua_tointegerx_rel_offset = v->lua_tointegerx_rel_offset;
	conf->v.lua_tolstring_rel_offset = v->lua_tolstring_rel_offset;
	conf->v.name = 0; // needs heap-allocated buffer and we don't use it again

	// save fbw path
	char* saved_fbw_path = malloc(MAX_PATH + 1);
	if (!saved_fbw_path) {
		printf("[CONFIG] malloc failed!\n");
	}
	strcpy(saved_fbw_path, fbw_path);

	// FINALLY load zlib (conf->fbw_path becomes zlib path temporarily)
	local_file_path(fbw_path, strlen(fbw_path), "zlib1.dll");
	HMODULE zlib = LoadLibraryA(fbw_path);
	if (!zlib) {
		printf("[!] Loading zlib failed "
			"(check zlib1.dll is present in folder containing fbw.exe)\n");
		return 0;
	}
	void* addr = GetProcAddress(zlib, "uncompress");
	if (!addr) {
		printf("[!] Partly loaded zlib, but fully loading it failed\n");
	}
	conf->zlib_uncompress = addr;

	// allocate HEAP space for region path
	char* region_heap = malloc(MAX_PATH+1); // to account for a very stupid edge case
	if (!region_heap) {
		printf("[CONFIG] malloc failed!\n");
	}
	strcpy(region_heap, conf->minecraft_region_path);
	//strcat(region_heap, "\\");
	conf->minecraft_region_path = region_heap;
	conf->fbw_path = saved_fbw_path;

	return 1;

}

DWORD add_to_block_table(char** table, char* str) {
	
	DWORD i = 0;
	while (table[++i]) { // skip the empty start value
		if (strcmp(table[i], str) == 0) {
			return i;
		}
	}
	table[i] = str;
	return i;
}

// TODO: handle duplicates
void insert_into_palette(PPaletteMapping palette, int size, char* str, DWORD i) {
	palette[size].name = str; // need to do normal insertion sort to handle duplicates
	palette[size].i = i;
	for (int i = size - 1; i > 0; i--) {
		if (strcmp(palette[i - 1].name, palette[i].name) > 0) {

			// swap name
			char* tmp1 = palette[i].name;
			palette[i].name = palette[i - 1].name;
			palette[i - 1].name = tmp1;

			// swap index
			DWORD tmp2 = palette[i].i;
			palette[i].i = palette[i - 1].i;
			palette[i - 1].i = tmp2;
		}
		else break;
	}
}