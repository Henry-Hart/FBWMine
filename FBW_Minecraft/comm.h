
// #=====================================================#
// #                                                     #
// # comm.h ==> Communication + injection                #
// #                                                     #
// #=====================================================#

// TODO:
//  - check for memory leaks (especially monitor)
//  - code cleanup etc. + removal
//  - make injection func look nicer (include asm maybe)

#pragma once
#include "util.h"
#include "nbt.h"

#define UPDATE_MSG 0
#define CREATE_MSG 1
#define REMOVE_MSG 2

#define MAX_INSTRUCTION_LENGTH 10
#define TABLE_ENTRY_SIZE 12
#define MAX_TABLE_ENTRIES 6
#define TABLE_SIZE_DWORDS 18 // 6 entries
#define ALIVE_THREADS_ARRAY_SIZE 13 // 1 + 6 * 2

#define MAX_WAIT_OBJS 7 // not the winapi max, just 1 + 6 threads

#define CHUNKS_PER_REGION_XZ_AXIS 32

typedef struct _DeathMonitorParams {
    HANDLE* wait_objs;
    DWORD* tids;
    DWORD* num_threads;
    HANDLE out_event;
    HANDLE pipe_output;
} DeathMonitorParams, * PDeathMonitorParams;

typedef struct _PartnerParams {
    HANDLE pInput;
    HANDLE proc;
    HANDLE event;
    int id;
} PartnerParams, * PPartnerParams;

typedef struct _SingleDeathMonitorParams {
    HANDLE linked;
    HANDLE partner;
    PPartnerParams partner_params;
    HANDLE sem;
    HANDLE pipe_output_remote;
    DWORD* base;
    DWORD tid;
    LPCRITICAL_SECTION pcs;
} SingleDeathMonitorParams, * PSingleDeathMonitorParams;

typedef struct _UpdateMsg {
    int param1;
    int param2;
    int param3;
    int param4;
    int param5;
    void* output;
} UpdateMsg;

typedef struct _CreateMsg {
    DWORD tid;
    DWORD index;
    DWORD* base;
    HANDLE event;
    int unused1;
    int unused2;
} CreateMsg;

typedef struct _RemoveMsg {
    DWORD tid;
    int unused1;
    int unused2;
    int unused3;
    int unused4;
    int unused5;
} RemoveMsg;

typedef struct _WorkerMsg {
    DWORD type;
    union {
        UpdateMsg update_msg;
        CreateMsg create_msg;
        RemoveMsg remove_msg;
    };
} WorkerMsg, * PWorkerMsg;

// or just use malloc which provides 'sufficiently aligned' memory
// currently unused (since interlocked reads are heavy)
// Will use SRW locks instead again
//volatile __declspec(align(32)) DWORD workers_blocking = 0;

void partner_thread(PPartnerParams params);
void monitor(PSingleDeathMonitorParams params);
int wait_and_write_subchunk(int x, int y, int z, HANDLE proc, BYTE* addr, int block); // external
void wait_and_load_region_from_coords(int x, int z); // external
HANDLE create_load_thread(int region_x, int region_z, PRegionInfo region_info); // external

int make_IPC_pipe(HANDLE proc, PHANDLE phInput, PHANDLE phOutput, PHANDLE phOutputRemote) {
    if (!CreatePipe(phInput, phOutput, NULL, sizeof(WorkerMsg))) { // maybe do size

        error_print("[IPC]", "CreatePipe");
        return 0;
    }

    if (!DuplicateHandle(GetCurrentProcess(), *phOutput, proc,
        phOutputRemote, 0, FALSE, DUPLICATE_SAME_ACCESS)) {

        error_print("[IPC]", "DuplicateHandle");
        return 0;
    }

    return 1;
}

HANDLE create_partner_death_pair(HANDLE proc, HANDLE event,
    int id, HANDLE sem, DWORD tid, DWORD* base, LPCRITICAL_SECTION pcs) {

    HANDLE pOutput = 0;
    HANDLE pOutputRemote = 0;
    PPartnerParams params = (PPartnerParams)malloc(sizeof(PartnerParams));
    if (!params) {
        error_print("[PARTNER CREATION]", "malloc");
        return 0;
    }

    if (!make_IPC_pipe(proc, &params->pInput, &pOutput, &pOutputRemote)) return 0;
    //printf("[NEW PARTNER %d] Remote input handle: %p\n", id, pOutputRemote);
    printf("[PARTNER CREATION] New partner %d!\n" , id);

    HANDLE linked = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
    if (!linked) {
        error_print("[PARTNER CREATION]", "OpenThread (linked thread)");
        return 0;
    }

    params->proc = proc;
    params->event = event;
    params->id = id;
    HANDLE thread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)partner_thread, params, NULL, NULL);
    if (!thread) {
        error_print("[PARTNER CREATION]", "CreateThread");
        return 0;
    }

    PSingleDeathMonitorParams death_params =
        (PSingleDeathMonitorParams)malloc(sizeof(SingleDeathMonitorParams));
    death_params->linked = linked;
    death_params->partner = thread;
    death_params->partner_params = params;
    death_params->sem = sem;
    death_params->pipe_output_remote = pOutputRemote;
    death_params->tid = tid;
    death_params->base = base;
    death_params->pcs = pcs;
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)monitor, death_params, NULL, NULL);

    return pOutputRemote;
}

void partner_thread(PPartnerParams params) {

    int first_update = 1; // threads can block on the first load
    WorkerMsg w;
    DWORD read;
    while (ReadFile(params->pInput, &w, sizeof(WorkerMsg), &read, NULL)) {

        if (read == 0) {
            printf("[PARTNER %d] %s", params->id, "Pipe closed!\n");
            return;
        }
        else if (read != sizeof(WorkerMsg)) {
            printf("[PARTNER %d] %s", params->id, "Incomplete read!\n");
            return;
        }

        //printf("[PARTNER %d] Got partner input\n", params->id);

        if (w.type == UPDATE_MSG) {

            //printf("[PARTNER %d] x=%d y=%d z=%d \n", params->id, w.update_msg.param1,
            //        w.update_msg.param2, w.update_msg.param3);

            /*if (!first_update) {

                // if new threads were spawned, we can block if necessary
                // this is completely terrible
                if (InterlockedCompareExchange(&workers_blocking, 0, 0)) {
                    //first_update = 1;
                }
            }*/

            // TODO: move this out of here and check wait_and_write_subchunk
            int repeat = wait_and_write_subchunk(w.update_msg.param1, w.update_msg.param2,
                w.update_msg.param3, params->proc, w.update_msg.output, first_update);

            // do the call again in the case of a force load
            if (repeat) { // TODO: maybe include thrashing message

                // TODO: currently, we handle loads automatically but not blue rings
                // if we identify the main thread (almost always ID 0 but if the user incorrectly
                //  uses the tool then a different thread could be ID zero), which we can do,
                //  assuming there are always 4 main workers + main thread (+ pointless worker),
                //  by checking thread exits, then we can just make thread 0 always block,
                //  or block on a short timeout (which is dodgy, but technically thread 0
                //  can also load chunks, it just is very rare to do anything other than
                //  the initial load tree thing)
                wait_and_write_subchunk(w.update_msg.param1, w.update_msg.param2,
                    w.update_msg.param3, params->proc, w.update_msg.output, 0);
            }

            if (first_update) first_update = 0;

            // release thread
            SetEvent(params->event);
        }
        else {
            printf("[PARTNER %d] %s", params->id, "Unexpected message!\n");
        }
    }

    free(params);
}

void add_table_entry(HANDLE proc, DWORD* base, DWORD tid, DWORD pipe_handle, DWORD event_handle) {

    DWORD buff[TABLE_SIZE_DWORDS];
    ReadProcessMemory(proc, base, buff, sizeof(buff), NULL);

    int i = 0;
    for (; i < TABLE_SIZE_DWORDS; i += 3) {
        if (!buff[i]) {
            buff[i] = tid;
            buff[i + 1] = pipe_handle;
            buff[i + 2] = event_handle;
            break;
        }
    }
    if (i == TABLE_SIZE_DWORDS) {
        printf("ERROR: table full! (not making an entry)\n");
        return;
    }
    WriteProcessMemory(proc, base, buff, sizeof(buff), NULL);
}

void remove_table_entry(HANDLE proc, DWORD* base, DWORD tid) {

    DWORD buff[TABLE_SIZE_DWORDS];
    if (!ReadProcessMemory(proc, base, buff, sizeof(buff), NULL)) {
        printf("ERROR: read failed, is process dead?\n");
        return;
    }

    //printf("Removing tid %L\n", tid);
    int i = 0;
    for (; i < TABLE_SIZE_DWORDS; i += 3) {
        //printf("buff[%d]=%L\n", i, buff[i]);
        if (buff[i] == tid) {
            buff[i] = 0;
            buff[i + 1] = 0;
            buff[i + 2] = 0;
            break;
        }
    }
    if (i == TABLE_SIZE_DWORDS) {
        // TODO: fix a thread e.g. 0 getting this error (maybe doesn't matter)
        printf("ERROR: table doesn't contain thread to remove!\n");
        return;
    }

    //printf("Zeroing ENTIRE table...\n", tid);
    //memset(buff, 0, sizeof(buff));
    WriteProcessMemory(proc, base, buff, sizeof(buff), NULL);
}

void process_monitor(proc) {

    WaitForSingleObject(proc, INFINITE);

    // not exactly elegant, but terminate process
    // TODO: synchronise with main thread or something
    printf("Process is dead! Stopping...\n");
    ExitProcess(0);
    
}

void monitor(PSingleDeathMonitorParams params) {

    // wait for the linked thread to die
    WaitForSingleObject(params->linked, INFINITE);

    //printf("[DEATH] detected linked thread for ID=%d death\n", params->partner_params->id);

    // update (presumably) main thread's partner BEFORE killing the thread
    //InterlockedExchange(&workers_blocking, 1);

    // force the partner to die
    TerminateThread(params->partner, 0);
    // wait for it to die
    WaitForSingleObject(params->partner, INFINITE);

    // final cleanup
    CloseHandle(params->partner_params->event);
    CloseHandle(params->partner_params->pInput);
    DuplicateHandle(params->partner_params->proc, params->pipe_output_remote,
        NULL, NULL, 0, FALSE, DUPLICATE_CLOSE_SOURCE);

    // remove ourselves from the table
    EnterCriticalSection(params->pcs);
    remove_table_entry(params->partner_params->proc, params->base, params->tid);
    LeaveCriticalSection(params->pcs);

    // signal there is a free space
    ReleaseSemaphore(params->sem, 1, NULL);

    // TODO: close sem + cs handles

    printf("[DEATH] terminated and cleaned up ID=%d\n", params->partner_params->id);
}

#define SET_BYTES_DWORD(bytes, i, v) *(DWORD*)((DWORD)bytes + i) = (DWORD)v;
#define FUNC_REL_JMP(base, func, i) (DWORD)func - ((DWORD)base + i + 5)


// TODO: table overflow detection (for 7+ active workers)
int add_inline_partner_hook(HANDLE hProcess, HANDLE pOutputRemote, PFBWVersion v) {

    void* func = v->shellcode_offset; // set_blue_type_up
    BYTE shellcode[] = {
        0x60,
        0xE8, 0x69, 0x68, 0x94, 0x74, // call <GetCurrentThreadId>
        0x31, 0xD2,
        0xE8, 0x00, 0x00, 0x00, 0x00, // call $0
        0x5B, 0x81, 0xC3, 0xB6, 0x00, 0x00, 0x00, 0x8B, 0x34, 0x13, 0x39, 0xF0, 0x74, 0x48, 0x83, 0xFE, 0x00, 0x74, 0x05, 0x83, 0xC2, 0x0C, 0xEB, 0xEF, 0x6A, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0x89, 0xC6,
        0xE8, 0x69, 0x0D, 0x95, 0x74, // call <CreateEventA>
        0x89, 0xC7, 0x6A, 0x00, 0x6A, 0x00, 0x57, 0x53, 0x52, 0x56, 0x6A, 0x01, 0x6A, 0x00, 0x54, 0x6A, 0x1C, 0x89, 0xE0, 0x83, 0xC0, 0x0C, 0x50,
        0x68, 0x80, 0x0F, 0x00, 0x00, // push ...
        0xE8, 0x69, 0x14, 0x95, 0x74, // call <WriteFile>
        0x83, 0xC4, 0x1C, 0x6A, 0xFF, 0x57,
        0xE8, 0x69, 0x0F, 0x95, 0x74, // call <WaitForSingleObject>
        0x61, 0xEB, 0x9D, 0x8D, 0x34, 0x13, 0x6A, 0x00, 0x6A, 0x01, 0x8B, 0x44, 0x24, 0x2C, 0x50,
        0xE8, 0x9C, 0x71, 0xA5, 0x00, // call lua_tolstring
        0x83, 0xC4, 0x0C, 0x50, 0x83, 0xEC, 0x14, 0x31, 0xDB, 0x43, 0x43, 0x8B, 0x44, 0x24, 0x3C, 0x6A, 0x00, 0x6A, 0x02, 0x50,
        0xE8, 0xF3, 0x70, 0xA5, 0x00, // call lua_tointegerx
        0x43, 0x89, 0x04, 0x9C, 0x89, 0x5C, 0x24, 0x04, 0x83, 0xFB, 0x07, 0x7C, 0xEE, 0x83, 0xC4, 0x0C, 0x6A, 0x00, 0x6A, 0x00, 0x54, 0x6A, 0x1C, 0x89, 0xE0, 0x83, 0xC0, 0x0C, 0x50, 0x8B, 0x46, 0x04, 0x50,
        0xE8, 0x69, 0x14, 0x95, 0x74, // call <WriteFile>
        0x83, 0xC4, 0x1C, 0x8B, 0x46, 0x08, 0x6A, 0xFF, 0x50,
        0xE8, 0x69, 0x0F, 0x95, 0x74, // call <WaitForSingleObject>
        0x61, 0xC3,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // TABLE (entry 1)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // entry 2
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // entry 3
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // entry 4
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // entry 5
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // entry 6 (spare)
    };

    // TODO: dynamically locate / build code to make location
    SET_BYTES_DWORD(shellcode, 2, FUNC_REL_JMP(func, GetCurrentThreadId, 1));
    SET_BYTES_DWORD(shellcode, 48, FUNC_REL_JMP(func, CreateEventA, 47));
    SET_BYTES_DWORD(shellcode, 81, FUNC_REL_JMP(func, WriteFile, 80));
    SET_BYTES_DWORD(shellcode, 92, FUNC_REL_JMP(func, WaitForSingleObject, 91));
    SET_BYTES_DWORD(shellcode, 112, v->lua_tolstring_rel_offset);
    SET_BYTES_DWORD(shellcode, 137, v->lua_tointegerx_rel_offset);
    SET_BYTES_DWORD(shellcode, 175, FUNC_REL_JMP(func, WriteFile, 174));
    SET_BYTES_DWORD(shellcode, 189, FUNC_REL_JMP(func, WaitForSingleObject, 188));
    SET_BYTES_DWORD(shellcode, 76, pOutputRemote);
    return WriteProcessMemory(hProcess, func, shellcode, sizeof(shellcode), NULL);
}