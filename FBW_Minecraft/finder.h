
// #=====================================================#
// #                                                     #
// # finder.h ==> Stuff to do with finding FBW's process #
// #                                                     #
// #=====================================================#

#pragma once
#include "util.h"

#define FINDER_STATUS_NOT_FOUND 0
#define FINDER_STATUS_NO_OPEN 1
#define FINDER_STATUS_FOUND 2
#define FINDER_STATUS_MULTIPLE 3

typedef struct _FinderParams {
    char* path;
    HANDLE proc;
    int status;
} FinderParams, *PFinderParams;

// slot initially contains FBW's base we are looking for,
// then on return we make slot contain FBW's handle
int WINAPI FBW_finder_callback(HWND hwnd, PFinderParams params) {

    // avoid trying to scan every window's process (to keep AV / EDR happy)
    char str[] = "Fractal Block World";
    char text[sizeof(str)];
    text[0] = 0;
    GetWindowTextA(hwnd, text, sizeof(str));

    if (strcmp(str, text) != 0) return TRUE;

    //printf("TEXT: %s\n", text);

    // verify it's FBW
    DWORD tid = 0;
    GetWindowThreadProcessId(hwnd, &tid);
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, tid);
    char fname[MAX_PATH]; // obviously this only works when FBW is placed somewhere non-stupid
    fname[0] = 0;
    GetModuleFileNameExA(hProc, NULL, fname, MAX_PATH);

    //printf("FNAME: %s\n%s\n", fname, params->path);
    
    if (strcicmp(fname, params->path) == 0) {

        if (params->status != FINDER_STATUS_NOT_FOUND) {
            // we have previously found a candidate FBW
            //  and this is ANOTHER one

            params->status = FINDER_STATUS_MULTIPLE;

            // we can return early here
            return FALSE;
        }

#ifdef BYPASS_DACL
        HANDLE me = GetCurrentProcess();

        HANDLE cur_token;
        OpenProcessToken(me, TOKEN_ALL_ACCESS, &cur_token);

        TOKEN_PRIVILEGES info;

        LUID SeDebugPrivilege;
        LookupPrivilegeValueA(NULL, "SeDebugPrivilege", &SeDebugPrivilege);

        LUID_AND_ATTRIBUTES priv_info;
        priv_info.Luid = SeDebugPrivilege;
        priv_info.Attributes = SE_PRIVILEGE_ENABLED;

        info.PrivilegeCount = 1;
        info.Privileges[0] = priv_info;
        if (AdjustTokenPrivileges(cur_token, FALSE, &info, 1, 0, 0)
            && GetLastError() == ERROR_SUCCESS) {
            
            print("[+] Debug privilege enabled\n");
        }
        else {
            print("[-] Couldn't get debug privilege, try rerunning as admin\n");
        }
#endif

        // note: we could store a copy of the partner table,
        //  hence we wouldn't need PROCESS_VM_READ
        hProc = OpenProcess(
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
            SYNCHRONIZE | PROCESS_DUP_HANDLE
            , FALSE, tid);
        if (hProc == 0) params->status = FINDER_STATUS_NO_OPEN;
        else { // we found FBW!
            params->status = FINDER_STATUS_FOUND;
            params->proc = hProc;
        }
    }

    if(params->status != FINDER_STATUS_FOUND) CloseHandle(hProc);
    return TRUE;
}

// Finds the FIRST FBW running
HANDLE find_fbw(char* path) {

    FinderParams params;
    params.path = path;
    params.status = FINDER_STATUS_NOT_FOUND;
    EnumWindows((WNDENUMPROC)FBW_finder_callback, (LPARAM)&params);

    switch (params.status) {
    case FINDER_STATUS_NOT_FOUND:
        printf("[!] FBW isn't running\n");
        break;
    case FINDER_STATUS_NO_OPEN:
        printf("[!] Unable to open FBW, are you running it as admin for some reason???\n");
        break;
    case FINDER_STATUS_MULTIPLE:
        printf("[!] Multiple FBW instances detected\n");
        break;
    default:
        printf("[+] FBW found\n");
        return params.proc;
    }

    return 0;
}
