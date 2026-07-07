
// #=====================================================#
// #                                                     #
// # load_zlib.h ==> Some FBWMine startup funcs for zlib #
// #                 (not using currently)               #
// #=====================================================#

#pragma once

int(__cdecl* uncompress)(
	unsigned char* dest, unsigned long* destLen,
	const unsigned char* source, unsigned long sourceLen);

void load_zlib() {
	
	// get zlib path
	char path[MAX_PATH]; // fails if non-ansi (or above max path)
	HMODULE me = GetModuleHandleA(NULL);
	DWORD length = GetModuleFileNameA(me, path, MAX_PATH);
	if (!length) return;

	int skip = 0;
	local_file_path(path, length, "fbw_dir.txt");
	HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	char fbw_path[MAX_PATH];
	if (!hFile || !ReadFile(hFile, fbw_path, MAX_PATH, 0, 0)) {
		skip = 1;
	}

	while(1) {
		if (!skip) {
			local_file_path(fbw_path, length, "Bin\\WindowsMinGW\\zlib1.dll");
			HMODULE zlib_mod = LoadLibraryA(fbw_path);
			if (zlib_mod) {
				uncompress = GetProcAddress(zlib_mod, "uncompress");
				break;
			}
		}
		else {
			skip = 0;
		}
		//while(!
		get_folder_path(fbw_path, MAX_PATH,
			L"Please select the FBW base folder e.g. steamapps\\common\\Fractal Block World",
			L"Select FBW",
			L"FBW folder:");//) {}
		DWORD written;  // strlen down here
		WriteFile(hFile, path, MAX_PATH, &written, 0);
	}
}

__strlen() {
	//...
}

char* local_file_path(char* path, DWORD length, char* fname) {

	// should probably do this more carefully
	while (path[length--] != '\\');
	int i = length + 2;
	while (*fname) {
		path[i++] = *fname++;
	}
	path[i] = 0;
}

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