// Copyright 2019 Augmented Perception Corporation

#include "core.hpp"
#include "core_win32.hpp"

#include <Winusb.h>
#include <tlhelp32.h>

#include <sstream>
using namespace std;

//#include "BugSplat.h"

#include "detours.h"
#include "implant_abi.hpp" // ImplantSharedMemoryLayout


//------------------------------------------------------------------------------
// Data

static HANDLE m_SharedMemoryFile;
static ImplantSharedMemoryLayout* m_SharedMemory;
static HANDLE m_FrameEvent;


//------------------------------------------------------------------------------
// Removal Thread

static DWORD WINAPI UnloadThreadProc(
    _In_ LPVOID /*lpParameter*/
)
{
    ::OutputDebugStringA("CameraImplant: UnloadThreadProc\n");

    while (!m_SharedMemory->RemoveServiceHook) {
        ::Sleep(100);
    }

    // This attempts to unload the injected DLL from the process and terminate the
    // currently-running thread:

    ::OutputDebugStringA("CameraImplant: RemoveServiceHook - Unloading!\n");

    HMODULE hModule = NULL;
    if (::GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)&UnloadThreadProc,
        &hModule))
    {
        ::OutputDebugStringA("CameraImplant: FreeLibraryAndExitThread\n");

        ::FreeLibraryAndExitThread(hModule, 0);
        // Does not reach the end of this function
    }

    ::OutputDebugStringA("CameraImplant: GetModuleHandleExW failed\n");
    return 0;
}

void StartUnloadThread()
{
    ::OutputDebugStringA("StartUnloadThread: CreateThread\n");

    HANDLE hThread = ::CreateThread(nullptr, 0, &UnloadThreadProc, nullptr, 0, nullptr);
    if (!hThread)
    {
        std::ostringstream oss;
        oss << "CreateThread failed: LastErr=" << ::GetLastError() << endl;
        ::OutputDebugStringA(oss.str().c_str());
        return;
    }

    ::OutputDebugStringA("CreateThread: complete\n");

    ::CloseHandle(hThread);

    ::OutputDebugStringA("CloseHandle: complete\n");
}


//------------------------------------------------------------------------------
// Shared Memory

static bool OpenSharedMemory()
{
    ::OutputDebugStringA("OpenSharedMemory\n");

    m_SharedMemoryFile = nullptr;
    m_SharedMemory = nullptr;

    m_FrameEvent = ::OpenEventA(EVENT_MODIFY_STATE, FALSE, CAMERA_IMPLANT_FRAME_EVENT_NAME);
    if (!m_FrameEvent) {
        ::OutputDebugStringA("OpenEventA failed\n");
        return false;
    }

    const uint32_t pid = ::GetCurrentProcessId();

    m_SharedMemoryFile = ::OpenFileMappingA(
        FILE_MAP_READ | FILE_MAP_WRITE,
        TRUE,
        CAMERA_IMPLANT_SHARED_MEMORY_NAME);
    if (!m_SharedMemoryFile) {
        ::OutputDebugStringA("OpenFileMappingA failed\n");
        return false;
    }

    m_SharedMemory = (ImplantSharedMemoryLayout*)(::MapViewOfFile(
        m_SharedMemoryFile,
        FILE_MAP_READ | FILE_MAP_WRITE,
        0, // offset = 0
        0, // offset = 0
        kImplantSharedMemoryBytes));
    if (!m_SharedMemory) {
        ::OutputDebugStringA("MapViewOfFile failed\n");
        return false;
    }

    m_SharedMemory->ImplantStage = 0;
    m_SharedMemory->ImplantInstalled = 0;
    m_SharedMemory->ImplantRemoveGood = 0;
    m_SharedMemory->ImplantRemoveFail = 0;
    m_SharedMemory->BeforeWriteCounter = 0;
    m_SharedMemory->AfterWriteCounter = 0;

    return true;
}

static void CloseSharedMemory()
{
    if (m_SharedMemory) {
        ::UnmapViewOfFile(m_SharedMemory);
        m_SharedMemory = nullptr;
    }
    if (m_SharedMemoryFile)
    {
        ::CloseHandle(m_SharedMemoryFile);
        m_SharedMemoryFile = nullptr;
    }
    if (m_FrameEvent) {
        ::CloseHandle(m_FrameEvent);
        m_FrameEvent = nullptr;
    }
}


//------------------------------------------------------------------------------
// Detoured functions

typedef BOOL(*WinUsb_GetOverlappedResult_ptr_t)(
    WINUSB_INTERFACE_HANDLE InterfaceHandle,
    LPOVERLAPPED            lpOverlapped,
    LPDWORD                 lpNumberOfBytesTransferred,
    BOOL                    bWait
);

typedef BOOL(*WinUsb_ReadPipe_ptr_t)(
    WINUSB_INTERFACE_HANDLE InterfaceHandle,
    UCHAR                   PipeID,
    PUCHAR                  Buffer,
    ULONG                   BufferLength,
    PULONG                  LengthTransferred,
    LPOVERLAPPED            Overlapped
);

struct ReadRequest
{
    uint8_t* Buffer;
    unsigned BufferLength;
    LPOVERLAPPED Overlapped;
};

static WinUsb_ReadPipe_ptr_t m_WinUsb_ReadPipe;
static WinUsb_GetOverlappedResult_ptr_t m_WinUsb_GetOverlappedResult;

static CRITICAL_SECTION m_RequestLock;
#define MAX_REQUEST_MEMORY 32
static ReadRequest m_Requests[MAX_REQUEST_MEMORY];
static int m_NextRequest;

static void AddReadRequest(LPOVERLAPPED overlapped, uint8_t* buffer, unsigned length)
{
    ::EnterCriticalSection(&m_RequestLock);

    const int request_index = m_NextRequest;
    if (++m_NextRequest >= MAX_REQUEST_MEMORY) {
        m_NextRequest = 0;
    }

    m_Requests[request_index].Buffer = buffer;
    m_Requests[request_index].BufferLength = length;
    m_Requests[request_index].Overlapped = overlapped;

    ::LeaveCriticalSection(&m_RequestLock);
}

static bool FindReadRequest(LPOVERLAPPED overlapped, ReadRequest& request)
{
    ::EnterCriticalSection(&m_RequestLock);
    int request_index = m_NextRequest;
    for (int i = 0; i < MAX_REQUEST_MEMORY; ++i)
    {
        if (--request_index < 0) {
            request_index = MAX_REQUEST_MEMORY - 1;
        }
        if (m_Requests[request_index].Overlapped == overlapped) {
            request = m_Requests[request_index];
            ::LeaveCriticalSection(&m_RequestLock);
            return true;
        }
    }
    ::LeaveCriticalSection(&m_RequestLock);
    return false;
}

static BOOL My_WinUsb_ReadPipe(
    WINUSB_INTERFACE_HANDLE InterfaceHandle,
    UCHAR                   PipeID,
    PUCHAR                  Buffer,
    ULONG                   BufferLength,
    PULONG                  LengthTransferred,
    LPOVERLAPPED            Overlapped
)
{
    AddReadRequest(Overlapped, Buffer, BufferLength);
    return m_WinUsb_ReadPipe(InterfaceHandle, PipeID, Buffer, BufferLength, LengthTransferred, Overlapped);
}

static BOOL My_WinUsb_GetOverlappedResult(
    WINUSB_INTERFACE_HANDLE InterfaceHandle,
    LPOVERLAPPED            lpOverlapped,
    LPDWORD                 lpNumberOfBytesTransferred,
    BOOL                    bWait
)
{
    ReadRequest request{};
    bool found = FindReadRequest(lpOverlapped, request);

    DWORD read_bytes = 0;
    BOOL result = m_WinUsb_GetOverlappedResult(InterfaceHandle, lpOverlapped, &read_bytes, bWait);

    if (found) {
        if (read_bytes >= 640 * 480 * 2) {
            uint32_t write_bytes = read_bytes;
            if (write_bytes > ImplantSharedMemoryLayout::kCameraBytes) {
                write_bytes = ImplantSharedMemoryLayout::kCameraBytes;
            }
            uint64_t exposure_usec = core::GetTimeUsec() - 15000;
            m_SharedMemory->WriteCamera(request.Buffer, write_bytes, exposure_usec);
            ::SetEvent(m_FrameEvent);
        }
#if defined(CORE_DEBUG)
        else {
            std::ostringstream oss;
            oss << "Warning: Ignoring small packet bytes=" << read_bytes << endl;
            ::OutputDebugStringA(oss.str().c_str());
        }
#endif
    }
#if defined(CORE_DEBUG)
    else {
        std::ostringstream oss;
        oss << "Warning: Failed to match buffer bytes=" << read_bytes << endl;
        ::OutputDebugStringA(oss.str().c_str());
    }
#endif

    if (lpNumberOfBytesTransferred) {
        *lpNumberOfBytesTransferred = read_bytes;
    }

    return result;
}

static bool UpdateAllThreads()
{
    ::OutputDebugStringA("UpdateAllThreads\n");

    const DWORD my_tid = ::GetCurrentThreadId();
    const DWORD my_pid = ::GetCurrentProcessId();

    HANDLE h = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, my_pid);
    if (h != INVALID_HANDLE_VALUE) {
        std::ostringstream oss;
        oss << "CreateToolhelp32Snapshot failed: LastErr=" << ::GetLastError() << endl;
        ::OutputDebugStringA(oss.str().c_str());
        return false;
    }

    THREADENTRY32 te;
    te.dwSize = sizeof(te);
    for (Thread32First(h, &te); Thread32Next(h, &te) ;)
    {
        if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
        {
            if (te.th32ThreadID != my_tid && te.th32OwnerProcessID == my_pid)
            {
                HANDLE thread = ::OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                if (!thread) {
                    ::OutputDebugStringA("OpenThread failed\n");
                    continue;
                }

                DetourUpdateThread(thread);

                // FIXME: Leaks handle
            }
        }
        te.dwSize = sizeof(te);
    }

    ::CloseHandle(h);
    return true;
}

static bool FindOriginalFunctions()
{
    ::OutputDebugStringA("FindOriginalFunctions\n");

    m_WinUsb_GetOverlappedResult = (WinUsb_GetOverlappedResult_ptr_t)
        DetourFindFunction("WinUsb", "WinUsb_GetOverlappedResult");
    if (!m_WinUsb_GetOverlappedResult) {
        ::OutputDebugStringA("DetourFindFunction WinUsb_GetOverlappedResult failed\n");
        return false;
    }

    m_WinUsb_ReadPipe = (WinUsb_ReadPipe_ptr_t)
        DetourFindFunction("WinUsb", "WinUsb_ReadPipe");
    if (!m_WinUsb_ReadPipe) {
        ::OutputDebugStringA("DetourFindFunction WinUsb_ReadPipe failed\n");
        return false;
    }

    return true;
}

static bool InstallHooks()
{
    ::OutputDebugStringA("InstallHooks\n");

    m_WinUsb_ReadPipe = nullptr;
    m_WinUsb_GetOverlappedResult = nullptr;

    ::InitializeCriticalSection(&m_RequestLock);

    for (int i = 0; i < MAX_REQUEST_MEMORY; ++i)
    {
        m_Requests[i].Buffer = nullptr;
        m_Requests[i].BufferLength = 0;
    }
    m_NextRequest = 0;

    m_SharedMemory->ImplantStage = 1;

    if (!FindOriginalFunctions()) {
        ::OutputDebugStringA("FindOriginalFunctions failed\n");
        return false;
    }

    m_SharedMemory->ImplantStage = 2;

    ::OutputDebugStringA("DetourTransactionBegin\n");
    LONG result = DetourTransactionBegin();
    if (result != NO_ERROR) {
        ::OutputDebugStringA("DetourTransactionBegin failed\n");
        return false;
    }

    m_SharedMemory->ImplantStage = 3;

    if (!UpdateAllThreads()) {
        ::OutputDebugStringA("UpdateAllThreads failed\n");
    }

    m_SharedMemory->ImplantStage = 4;

    ::OutputDebugStringA("DetourAttach: GetOverlappedResult\n");
    result = DetourAttach(&(PVOID&)m_WinUsb_GetOverlappedResult, My_WinUsb_GetOverlappedResult);
    if (result != NO_ERROR) {
        ::OutputDebugStringA("DetourAttach WinUsb_GetOverlappedResult failed\n");
        return false;
    }

    m_SharedMemory->ImplantStage = 5;

    ::OutputDebugStringA("DetourAttach: WinUsb_ReadPipe\n");
    result = DetourAttach(&(PVOID&)m_WinUsb_ReadPipe, My_WinUsb_ReadPipe);
    if (result != NO_ERROR) {
        ::OutputDebugStringA("DetourAttach WinUsb_ReadPipe failed\n");
        return false;
    }

    m_SharedMemory->ImplantStage = 6;

    ::OutputDebugStringA("DetourAttach: DetourTransactionCommit\n");
    result = DetourTransactionCommit();
    if (result != NO_ERROR) {
        ::OutputDebugStringA("DetourTransactionCommit failed\n");
        return false;
    }

    m_SharedMemory->ImplantStage = 7;

    ::OutputDebugStringA("InstallHooks: Success\n");
    return true;
}

static bool RemoveHooks()
{
    m_SharedMemory->ImplantStage = 8;

    LONG result = DetourTransactionBegin();
    if (result != NO_ERROR) {
        ::OutputDebugStringA("DetourTransactionBegin failed\n");
        return false;
    }

    // Cannot list threads to suspend them here - Probably suspended anyway?

    m_SharedMemory->ImplantStage = 9;

    result = DetourDetach(&(PVOID&)m_WinUsb_GetOverlappedResult, My_WinUsb_GetOverlappedResult);
    if (result != NO_ERROR) {
        ::OutputDebugStringA("DetourDetach WinUsb_GetOverlappedResult failed\n");
        return false;
    }

    m_SharedMemory->ImplantStage = 10;

    result = DetourDetach(&(PVOID&)m_WinUsb_ReadPipe, My_WinUsb_ReadPipe);
    if (result != NO_ERROR) {
        ::OutputDebugStringA("DetourDetach WinUsb_ReadPipe failed\n");
        return false;
    }

    m_SharedMemory->ImplantStage = 11;

    result = DetourTransactionCommit();
    if (result != NO_ERROR) {
        ::OutputDebugStringA("DetourTransactionCommit failed\n");
        return false;
    }

    m_SharedMemory->ImplantStage = 12;

    return true;
}


//------------------------------------------------------------------------------
// Entrypoint

extern "C" BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    (void)hModule;
    (void)lpReserved;

    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        ::OutputDebugStringA("CameraImplant DLL_PROCESS_ATTACH\n");
        if (OpenSharedMemory()) {
            if (InstallHooks()) {
                m_SharedMemory->ImplantInstalled = 1;
            }
            else {
                m_SharedMemory->ImplantInstalled = 0;
            }
        }

        StartUnloadThread();
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        ::OutputDebugStringA("CameraImplant DLL_PROCESS_DETACH\n");
        if (RemoveHooks()) {
            m_SharedMemory->ImplantRemoveGood = 1;
        }
        else {
            m_SharedMemory->ImplantRemoveFail = 1;
        }
        CloseSharedMemory();
    }

    return TRUE;
}
