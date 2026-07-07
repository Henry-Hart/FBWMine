
// #=====================================================#
// #                                                     #
// # util.h ==> General utils + common includes          #
// #                                                     #
// #=====================================================#

#pragma once
//#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <ShObjIdl.h>
#include <Psapi.h>
#include <msi.h>
#include "stdfuncs.h"

// this doesn't work here, so it's included in the options for this project
//#pragma comment(lib, "msi.lib")

/*
void* read_file_extra(char* fileName, DWORD* read) {

	void* f = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE) {
		*read = 0;
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
			//printf("ReadFile failed\n");
			
			// actually do nothing for now
		}

		if (chunkRead == 0) break; // EOF
		totalRead += chunkRead;
	}

	if (totalRead != size) {
		//printf("Couldn't read all of file!\n");
		*read = totalRead;
		return 0;
	}

	CloseHandle(f);

	*read = totalRead;
	return buff;
}
*/
#define read_file_extra read_file
void* read_file(char* fileName, DWORD* read) {

	// TODO: probably just do everything with wide strings or something
	wchar_t wFileName[1024];
	MultiByteToWideChar(CP_UTF8, 0, fileName, -1, wFileName, 1024); // ~ GPT

	void* f = CreateFileW(wFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


	if (f == INVALID_HANDLE_VALUE) {

		// TODO: use error func / say something nicer
		// for now literally just removing this message
		//printf("CreateFileA failed\n");
		*read = 0;
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
			(DWORD)buff + totalRead,
			size - totalRead,
			&chunkRead, NULL
		))
		{
			//printf("ReadFile failed\n");
			// again, do nothing for now
		}

		if (chunkRead == 0) break; // EOF
		totalRead += chunkRead;
	}

	if (totalRead != size) {
		//printf("Couldn't read all of file!\n");
		*read = totalRead;
		return 0;
	}

	CloseHandle(f);

	*read = totalRead;
	return buff;
}

// modify the end of a path buffer
char* local_file_path(char* path, DWORD length, char* fname) {

	// should probably do this more carefully
	while (path[length--] != '\\');
	path[length + 2] = 0;

	return strcat(path, fname);
}

// from windows example
int get_folder_path(char* buff, int bufflen, wchar_t* title, wchar_t* ok_label, wchar_t* name_label)
{

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen;
		
		// Create the FileOpenDialog object.
		hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			&IID_IFileOpenDialog, &pFileOpen);

		if (SUCCEEDED(hr))
		{

			DWORD options;
			pFileOpen->lpVtbl->GetOptions(pFileOpen, &options);
			pFileOpen->lpVtbl->SetOptions(pFileOpen, options 
				| FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_DONTADDTORECENT | FOS_FORCESHOWHIDDEN);
			pFileOpen->lpVtbl->SetTitle(pFileOpen, title);
			pFileOpen->lpVtbl->SetOkButtonLabel(pFileOpen, ok_label);
			pFileOpen->lpVtbl->SetFileNameLabel(pFileOpen, name_label);

			// Show the Open dialog box.
			hr = pFileOpen->lpVtbl->Show(pFileOpen, NULL);

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->lpVtbl->GetResult(pFileOpen, &pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					hr = pItem->lpVtbl->GetDisplayName(pItem, SIGDN_FILESYSPATH, &pszFilePath);

					// Display the file name to the user.
					if (SUCCEEDED(hr))
					{
						int res = WideCharToMultiByte(CP_UTF8, 0, 
							pszFilePath, -1, buff, bufflen, NULL, NULL);
						CoTaskMemFree(pszFilePath);
						return res;
					}
					pItem->lpVtbl->Release(pItem);
				}
			}
			pFileOpen->lpVtbl->Release(pFileOpen);
		}
		CoUninitialize();
	}
	return 0;
}

typedef struct _FBWVersion {
	BYTE* shellcode_offset;
	BYTE* lua_tolstring_rel_offset; // could get from EAT
	BYTE* lua_tointegerx_rel_offset; // ^^^
	char* name;
} FBWVersion, * PFBWVersion;

typedef struct _FBWVersionEntry {
	ULONG hash[4];
	FBWVersion v;
} FBWVersionEntry, * PFBWVersionEntry;

PFBWVersion get_fbw_version(ULONG* hash_data, PFBWVersionEntry version_table, int len) {
	
	//for (int i = 0; i < 4; i++) {
	//	printf("Hash: %lu\n", hash_data[i]);
	//}

	for (int i = 0; i < len; i++) { // length of version_table
		int j = 0;
		for (; j < 4; j++) {
			if (hash_data[j] != version_table[i].hash[j]) break;
		}
		if (j == 4) {
			return &version_table[i].v;
		}
	}

	return 0;
}

BOOL fprint(HANDLE hFile, char* str) {
	DWORD written = 0;
	return WriteFile(hFile, str, strlen(str), &written, NULL);
}

int get_block_in_world(char* str, int* bounds) {
	
	int num = 0;
	do {
		print(str);
	} while (
		!get_console_integer(&num) ||
		bounds[0] > num ||
		bounds[1] < num
	);
	
	return num;
}

void banner() {
	//setlocale(LC_ALL, ".UTF8");
	SetConsoleOutputCP(CP_UTF8);
	print(
		"\033[0;93m███████╗██████╗ ██╗    ██╗\033[0;96m███╗   ███╗██╗███╗   ██╗███████╗\n"
		"\033[0;93m██╔════╝██╔══██╗██║    ██║\033[0;96m████╗ ████║██║████╗  ██║██╔════╝\n"
		"\033[0;93m█████╗  ██████╔╝██║ █╗ ██║\033[0;96m██╔████╔██║██║██╔██╗ ██║█████╗\n"
		"\033[0;93m██╔══╝  ██╔══██╗██║███╗██║\033[0;96m██║╚██╔╝██║██║██║╚██╗██║██╔══╝\n"
		"\033[0;93m██║     ██████╔╝╚███╔███╔╝\033[0;96m██║ ╚═╝ ██║██║██║ ╚████║███████╗\n"
		"\033[0;93m╚═╝     ╚═════╝  ╚══╝╚══╝ \033[0;96m╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝╚══════╝\n"
		"\033[0m"
	);
}