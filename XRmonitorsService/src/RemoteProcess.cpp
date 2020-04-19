#include "RemoteProcess.hpp"
#include "core_string.hpp"
#include "core_logger.hpp"

#include <string>
#include <cctype>
#include <memory>

#include <Sddl.h> // ConvertStringSecurityDescriptorToSecurityDescriptor
#include <Psapi.h>
#include <Dbt.h> // DBT_DEVNODES_CHANGED

namespace core {

static logger::Channel Logger("RemoteProcess");


//------------------------------------------------------------------------------
// CallRemoteFunction

bool CallRemoteFunction(HANDLE hProcess, FARPROC proc, void* parameter)
{
    AutoEvent remote_thread = ::CreateRemoteThread(
        hProcess,
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)proc,
        parameter,
        0,
        0);

    if (remote_thread.Invalid()) {
        Logger.Error("CreateRemoteThread failed");
        return false;
    }

    Logger.Info("Waiting for remote thread to complete");

    DWORD waitResult = ::WaitForSingleObject(remote_thread.Get(), 10000);
    if (waitResult != WAIT_OBJECT_0) {
        return false;
    }

    return true;
}


//------------------------------------------------------------------------------
// GetRemoteModuleHandle

bool GetRemoteModuleHandle(HANDLE hProcess, const char* moduleName, HMODULE& moduleHandleOut)
{
    HMODULE moduleResult = NULL;
    HMODULE* modules = nullptr;
    unsigned returnedModuleCount = 0;
    DWORD bytesNeeded = 1024;

    for (unsigned retryCount = 0; retryCount < 3; ++retryCount)
    {
        const DWORD allocatedCount = (DWORD)(bytesNeeded / sizeof(HMODULE)) + 8; // Add some slop
        const DWORD allocatedBytes = (DWORD)(allocatedCount * sizeof(HMODULE));

        // Reallocate module array
        delete[] modules;
        modules = new (std::nothrow)HMODULE[allocatedCount];
        if (!modules)
        {
            CORE_DEBUG_BREAK();
            break;
        }

        // Get module list
        if (!::EnumProcessModulesEx(hProcess, modules, allocatedBytes, &bytesNeeded, LIST_MODULES_ALL) || bytesNeeded <= 0)
        {
            CORE_DEBUG_BREAK();
            delete[] modules;
            return false;
        }

        // If it succeeded:
        if (bytesNeeded <= allocatedBytes)
        {
            returnedModuleCount = (unsigned)(bytesNeeded / sizeof(HMODULE));
            break;
        }
    }

    // Search the module list for the target
    for (unsigned i = 0; i < returnedModuleCount; ++i)
    {
        if (!modules[i]) {
            continue;
        }

        // Get the module name
        static const unsigned kNameBufferChars = 512;
        char nameBuffer[kNameBufferChars];
        DWORD getResult = ::GetModuleBaseNameA(hProcess, modules[i], nameBuffer, kNameBufferChars);
        if (getResult <= 0 || getResult >= kNameBufferChars) {
            continue;
        }
        nameBuffer[getResult] = '\0';

        // Check if we found it
        if (nullptr != StrIStr(nameBuffer, moduleName))
        {
            moduleResult = modules[i];
            break;
        }
    }

    delete[] modules;
    moduleHandleOut = moduleResult;
    if (!moduleResult) {
        return false;
    }

    return true;
}


//------------------------------------------------------------------------------
// RemoteLoadLibrary

int RemoteLoadLibrary(HANDLE hProcess, const std::string& fullLibPath)
{
    HMODULE hModule = NULL;
    if (!GetRemoteModuleHandle(hProcess, "kernel32", hModule)) {
        Logger.Error("GetRemoteModuleHandle failed");
        return -1;
    }

    LoadLibraryAddresses addrs;
    const int addr_result = GetRemoteLoadLibraryAddress(hProcess, hModule, addrs);
    if (addr_result != 0) {
        Logger.Error("GetRemoteLoadLibraryAddress failed");
        return addr_result * 100;
    }

    std::wstring fullLibPathWide(fullLibPath.begin(), fullLibPath.end());

    unsigned bytesToCopy = (unsigned)fullLibPath.size() + 1;
    if (!addrs.LoadLibraryA) {
        bytesToCopy *= 2; // Wide characters
    }

    static const unsigned kParameterCavePadding = 1024;
    const unsigned parameterCaveBytes = kParameterCavePadding + bytesToCopy;

    // Allocate cave in remote process
    void* const RemoteCodeCavePtr = ::VirtualAllocEx(
        hProcess,
        NULL,
        parameterCaveBytes,
        MEM_COMMIT,
        PAGE_READWRITE);

    if (!RemoteCodeCavePtr) {
        Logger.Error("VirtualAllocEx failed");
        return -3;
    }

    // Fill it with the library path to load
    const BOOL writeResult = ::WriteProcessMemory(
        hProcess,
        RemoteCodeCavePtr,
        addrs.LoadLibraryA ? (LPCVOID)fullLibPath.c_str() : (LPCVOID)fullLibPathWide.c_str(),
        bytesToCopy,
        NULL);

    if (writeResult != TRUE) {
        Logger.Error("WriteProcessMemory failed");
        return -4;
    }

    bool callResult = CallRemoteFunction(
        hProcess,
        addrs.LoadLibraryA ? addrs.LoadLibraryA : addrs.LoadLibraryW,
        RemoteCodeCavePtr);

    // Free up allocated code cave
    ::VirtualFreeEx(hProcess, RemoteCodeCavePtr, parameterCaveBytes, MEM_RELEASE);

    return callResult ? 0 : -5;
}


//------------------------------------------------------------------------------
// RemoteProcessReader

bool RemoteProcessReader::Initialize(HANDLE hProcess, HMODULE hModule)
{
    ProcessHandle = hProcess;

    static const DWORD kMinSizeOfImage = 1024;
    static const DWORD kMaxSizeOfImage = 1000 * 1000 * 100;

    MODULEINFO RemoteModuleInfo;
    if (!::GetModuleInformation(
        hProcess,
        hModule,
        &RemoteModuleInfo,
        sizeof(RemoteModuleInfo)) ||
        RemoteModuleInfo.SizeOfImage < kMinSizeOfImage ||
        RemoteModuleInfo.SizeOfImage > kMaxSizeOfImage ||
        !RemoteModuleInfo.lpBaseOfDll)
    {
        return false;
    }

    RemoteModuleBase = (const uint8_t*)RemoteModuleInfo.lpBaseOfDll;
    SizeOfImage = RemoteModuleInfo.SizeOfImage;
    return true;
}

bool RemoteProcessReader::Read(uint64_t offset, void* buffer, unsigned bytes)
{
    SIZE_T bytes_read = 0;

    BOOL read_result = ::ReadProcessMemory(
        ProcessHandle,
        (LPCVOID)(RemoteModuleBase + offset),
        buffer,
        bytes,
        &bytes_read);

    if (!read_result || bytes_read != bytes) {
        return false;
    }

    return true;
}


//------------------------------------------------------------------------------
// GetRemoteLoadLibraryAddress

int GetRemoteLoadLibraryAddress(HANDLE hProcess, HMODULE hModule, LoadLibraryAddresses& results)
{
    RemoteProcessReader reader;
    if (!reader.Initialize(hProcess, hModule)) {
        return -1;
    }

    IMAGE_DOS_HEADER DosHeader;
    if (!reader.Read(0, DosHeader)) {
        return -2;
    }
    if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        return -3;
    }

    DWORD Signature;
    if (!reader.Read(DosHeader.e_lfanew, Signature)) {
        return -4;
    }
    if (Signature != IMAGE_NT_SIGNATURE) {
        return -5;
    }

    IMAGE_FILE_HEADER FileHeader;
    if (!reader.Read(DosHeader.e_lfanew + sizeof(Signature), FileHeader)) {
        return -6;
    }

    IMAGE_OPTIONAL_HEADER64 OptHeader64 = { 0 };
    IMAGE_OPTIONAL_HEADER32 OptHeader32 = { 0 };
    bool Is64Bit = false;

    if (FileHeader.SizeOfOptionalHeader == sizeof(OptHeader64)) {
        Is64Bit = true;
    }
    else if (FileHeader.SizeOfOptionalHeader == sizeof(OptHeader32)) {
        Is64Bit = false;
    }
    else {
        return -7;
    }

    if (Is64Bit)
    {
        if (!reader.Read(DosHeader.e_lfanew + sizeof(Signature) + sizeof(FileHeader), OptHeader64)) {
            return -8;
        }
        if (OptHeader64.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            return -9;
        }
    }
    else
    {
        if (!reader.Read(DosHeader.e_lfanew + sizeof(Signature) + sizeof(FileHeader), OptHeader32)) {
            return -10;
        }
        if (OptHeader32.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            return -11;
        }
    }

    IMAGE_DATA_DIRECTORY ExportDirectory;

    if (Is64Bit && OptHeader64.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_EXPORT + 1)
    {
        ExportDirectory.VirtualAddress = (OptHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).VirtualAddress;
        ExportDirectory.Size = (OptHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).Size;
    }
    else if (OptHeader32.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_EXPORT + 1)
    {
        ExportDirectory.VirtualAddress = (OptHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).VirtualAddress;
        ExportDirectory.Size = (OptHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).Size;
    }
    else {
        return -12;
    }

    IMAGE_EXPORT_DIRECTORY ExportTable;
    if (!reader.Read(ExportDirectory.VirtualAddress, ExportTable)) {
        return -13;
    }

    static const unsigned kMaxFunctionCount = 1000 * 1000;
    if (ExportTable.NumberOfFunctions > kMaxFunctionCount ||
        ExportTable.NumberOfNames > kMaxFunctionCount ||
        ExportTable.NumberOfNames > kMaxFunctionCount)
    {
        return -14;
    }

    std::unique_ptr<DWORD[]> ExportFunctionTable = std::make_unique<DWORD[]>(ExportTable.NumberOfFunctions);
    std::unique_ptr<DWORD[]> ExportNameTable = std::make_unique<DWORD[]>(ExportTable.NumberOfNames);
    std::unique_ptr<WORD[]> ExportOrdinalTable = std::make_unique<WORD[]>(ExportTable.NumberOfNames);

    if (!reader.ReadArray(ExportTable.AddressOfFunctions, ExportFunctionTable.get(), ExportTable.NumberOfFunctions)) {
        return -15;
    }
    if (!reader.ReadArray(ExportTable.AddressOfNames, ExportNameTable.get(), ExportTable.NumberOfNames)) {
        return -16;
    }
    if (!reader.ReadArray(ExportTable.AddressOfNameOrdinals, ExportOrdinalTable.get(), ExportTable.NumberOfNames)) {
        return -17;
    }

    bool sawInvalidOrdinal = false;
    bool sawInvalidFunctionAddress = false;

    for (unsigned i = 0; i < ExportTable.NumberOfNames; ++i)
    {
        char functionName[13];
        if (!reader.Read(ExportNameTable[i], functionName))
            continue;

        if (0 == StrNCaseCompare(functionName, "LoadLibrary", 11))
        {
            const WORD Ordinal = ExportOrdinalTable[i];
            if (Ordinal >= ExportTable.NumberOfFunctions)
            {
                sawInvalidOrdinal = true;
                continue;
            }

            const DWORD FunctionAddress = ExportFunctionTable[Ordinal];
            if ((FunctionAddress >= ExportDirectory.VirtualAddress &&
                FunctionAddress < ExportDirectory.VirtualAddress + ExportDirectory.Size) ||
                FunctionAddress > reader.SizeOfImage)
            {
                sawInvalidFunctionAddress = true;
                continue;
            }

            if (0 == StrNCaseCompare(functionName, "LoadLibraryA", 13))
                results.LoadLibraryA = (FARPROC)(reader.RemoteModuleBase + FunctionAddress);
            else if (0 == StrNCaseCompare(functionName, "LoadLibraryW", 13))
                results.LoadLibraryW = (FARPROC)(reader.RemoteModuleBase + FunctionAddress);
            else
                continue;

            if (results.LoadLibraryA && results.LoadLibraryW)
                break;
        }
    }

    if (!results.LoadLibraryA && !results.LoadLibraryW) {
        return -18;
    }

    return 0;
}


} // namespace oxshim
