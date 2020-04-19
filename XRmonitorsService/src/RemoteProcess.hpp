#pragma once

#include "core_win32.hpp"

namespace core {


//------------------------------------------------------------------------------
// Tools

// Call a function from another process given its address in that process
// Returns true on success
bool CallRemoteFunction(HANDLE hProcess, FARPROC proc, void* parameter = nullptr);

// Get the result from GetModuleHandle() in another process
// Returns true on success
bool GetRemoteModuleHandle(HANDLE hProcess, const char* moduleName, HMODULE& moduleHandleOut);

// Call LoadLibrary() in another process
// Returns 0 on success, other codes on failure.
int RemoteLoadLibrary(HANDLE hProcess, const std::string& fullLibPath);

struct LoadLibraryAddresses
{
    FARPROC LoadLibraryA = NULL;
    FARPROC LoadLibraryW = NULL;
};

// Get LoadLibrary addresses from remote process
// Returns 0 on success, other codes on failure.
int GetRemoteLoadLibraryAddress(HANDLE hProcess, HMODULE hModule, LoadLibraryAddresses& results);

// Remote process reader tool
struct RemoteProcessReader
{
    HANDLE ProcessHandle = nullptr;
    DWORD SizeOfImage = 0;
    const uint8_t* RemoteModuleBase = nullptr;


    bool Initialize(HANDLE hProcess, HMODULE hModule);

    bool Read(uint64_t offset, void* buffer, unsigned bytes);

    template<class T>
    bool Read(uint64_t offset, T& data)
    {
        return Read(offset, &data, (unsigned)sizeof(T));
    }

    template<class T>
    bool ReadArray(uint64_t offset, T* data, unsigned count)
    {
        return Read(offset, data, (unsigned)sizeof(T) * count);
    }
};


} // namespace core
