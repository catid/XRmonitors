#include "core_win32_pipe.hpp"
#include "core_string.hpp"
#include "core_logger.hpp"

namespace core {

static logger::Channel Logger("PipeServer");


//------------------------------------------------------------------------------
// NamedPipeServer

bool NamedPipeServer::Initialize(
    const std::string& pipe_name,
    const PipeHandlers& handlers)
{
    PipeName = pipe_name;
    Handlers = handlers;

    Logger.Error("Opening named pipe server: '", PipeName, "'");

    std::vector<uint8_t> desc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    SECURITY_ATTRIBUTES sa;
    sa.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR)desc.data();
    InitializeSecurityDescriptor(sa.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    // ACL is set as NULL in order to allow all access to the object.
    SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, NULL, FALSE);
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;

    PipeHandle = ::CreateNamedPipeA(
        PipeName.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        PIPE_UNLIMITED_INSTANCES,
        kMaxMessageBytes,
        kMaxMessageBytes,
        0,
        &sa);

    if (PipeHandle.Invalid()) {
        Logger.Error("CreateNamedPipeA failed");
        return false;
    }

    TerminateEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    WaitEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (TerminateEvent.Invalid() || WaitEvent.Invalid()) {
        Logger.Error("CreateEventW");
        return false;
    }

    Terminated = false;
    PipeThread = MakeSharedNoThrow<std::thread>(&NamedPipeServer::ThreadLoop, this);
    if (!PipeThread) {
        Logger.Error("OOM");
        return false;
    }

    return true;
}

void NamedPipeServer::Shutdown()
{
    Terminated = true;
    if (TerminateEvent.Valid()) {
        ::SetEvent(TerminateEvent.Get());
    }
    JoinThread(PipeThread);
    TerminateEvent.Clear();
    WaitEvent.Clear();
    PipeHandle.Clear();
}

void NamedPipeServer::Write(const void* data, uint32_t bytes)
{
    // This blocks until the request completes
    ::WriteFile(PipeHandle.Get(), data, bytes, nullptr, nullptr);
}

void NamedPipeServer::ThreadLoop()
{
    Logger.Debug("Started pipe thread");

    while (!Terminated) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        OVERLAPPED overlapped{};
        ::ResetEvent(WaitEvent.Get());
        overlapped.hEvent = WaitEvent.Get();

        BOOL result = ::ConnectNamedPipe(PipeHandle.Get(), &overlapped);

        if (result) {
            Logger.Info("ConnectNamedPipe succeeded - Connected");
            // Good pipe connection
            OnConnect();
            continue;
        }

        DWORD err = ::GetLastError();
        if (err == ERROR_PIPE_CONNECTED) {
            Logger.Info("ConnectNamedPipe ERROR_PIPE_CONNECTED - Connected");
            // Good pipe connection
            OnConnect();
            continue;
        }
        if (err != ERROR_IO_PENDING) {
            break;
        }

        HANDLE events[2] = {
            WaitEvent.Get(), TerminateEvent.Get()
        };
        DWORD wait_result = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (Terminated || wait_result != WAIT_OBJECT_0) {
            Logger.Info("Terminated flag set");
            break;
        }

        Logger.Info("ConnectNamedPipe wait succeeded - Connected");
        // Good pipe connection
        OnConnect();
    }

    Logger.Debug("Terminated pipe thread");

    Handlers.PipeCloseHandler();
}

void NamedPipeServer::OnConnect()
{
    Logger.Info("Client connected");

    Handlers.PipeConnectHandler();

    while (!Terminated) {
        OVERLAPPED overlapped{};
        ::ResetEvent(WaitEvent.Get());
        overlapped.hEvent = WaitEvent.Get();

        DWORD bytes_read = 0;
        BOOL read_result = ::ReadFile(
            PipeHandle.Get(),
            ReadBuffer,
            kMaxMessageBytes,
            &bytes_read,
            &overlapped);

        if (read_result) {
            if (bytes_read == 0) {
                Logger.Info("Bytes immediately read = 0");
                break;
            }
            Handlers.PipeDataHandler(ReadBuffer, bytes_read);
            continue;
        }

        DWORD err = GetLastError();

        if (err != ERROR_IO_PENDING) {
            Logger.Error("ReadFile error ", err);
            break;
        }

        HANDLE events[2] = {
            WaitEvent.Get(), TerminateEvent.Get()
        };
        DWORD wait_result = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (Terminated || wait_result != WAIT_OBJECT_0) {
            Logger.Info("Terminated flag set");
            break;
        }

        ::GetOverlappedResult(PipeHandle.Get(), &overlapped, &bytes_read, FALSE);
        if (bytes_read == 0) {
            Logger.Info("Bytes read = 0");
            break;
        }

        Handlers.PipeDataHandler(ReadBuffer, bytes_read);
    }

    Logger.Info("Client disconnected");

    ::FlushFileBuffers(PipeHandle.Get());
    ::DisconnectNamedPipe(PipeHandle.Get());
}


//------------------------------------------------------------------------------
// NamedPipeClient

bool NamedPipeClient::Connect(
    const std::string& pipe_name,
    const PipeHandlers& handlers)
{
    PipeName = pipe_name;
    Handlers = handlers;

    std::string str = "NamedPipeClient: Opening '";
    str += PipeName;
    str += "'\n";
    ::OutputDebugStringA(str.c_str());

    // This connection request will either complete or fail immediately.
    // TBD: We could use WaitNamedPipeW() if we wanted to support multiple
    // connections to the pipe from several apps.
    PipeHandle = ::CreateFileA(
        PipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr);
    if (PipeHandle.Invalid()) {
        ::OutputDebugStringA("NamedPipeClient: CreateFileA failed\n");
        CORE_DEBUG_BREAK();
        return false;
    }

    // Switch pipe into message mode.  It is in byte mode by default.
    DWORD pipeMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
    if (!::SetNamedPipeHandleState(
        PipeHandle.Get(), // Pipe handle
        &pipeMode, // Set pipe to blocking message mode
        nullptr, // Not changing maximum collection count
        nullptr)) // Not changing collection data timeout
    {
        ::OutputDebugStringA("NamedPipeClient: SetNamedPipeHandleState failed\n");
        CORE_DEBUG_BREAK();
        return false;
    }

    TerminateEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    WaitEvent = ::CreateEventW(nullptr, TRUE, TRUE, nullptr);
    if (TerminateEvent.Invalid() || WaitEvent.Invalid()) {
        ::OutputDebugStringA("NamedPipeClient: CreateEventW failed\n");
        CORE_DEBUG_BREAK();
        return false;
    }

    Terminated = false;
    PipeThread = MakeSharedNoThrow<std::thread>(&NamedPipeClient::ThreadLoop, this);
    if (!PipeThread) {
        ::OutputDebugStringA("NamedPipeClient: OOM\n");
        CORE_DEBUG_BREAK();
        return false;
    }

    ::OutputDebugStringA("NamedPipeClient: Initialized\n");

    return true;
}

void NamedPipeClient::Shutdown()
{
    Terminated = true;
    if (TerminateEvent.Valid()) {
        ::SetEvent(TerminateEvent.Get());
    }
    JoinThread(PipeThread);
    TerminateEvent.Clear();
    WaitEvent.Clear();
    PipeHandle.Clear();
}

void NamedPipeClient::Write(const void* data, uint32_t bytes)
{
    // This blocks until the request completes
    ::WriteFile(PipeHandle.Get(), data, bytes, nullptr, nullptr);
}

void NamedPipeClient::ThreadLoop()
{
    ::OutputDebugStringA("NamedPipeClient: ThreadLoop started\n");

    Handlers.PipeConnectHandler();

    while (!Terminated) {
        OVERLAPPED overlapped{};
        ::ResetEvent(WaitEvent.Get());
        overlapped.hEvent = WaitEvent.Get();

        DWORD bytes_read = 0;
        BOOL read_result = ::ReadFile(
            PipeHandle.Get(),
            ReadBuffer,
            kMaxMessageBytes,
            &bytes_read,
            &overlapped);

        if (read_result) {
            if (bytes_read == 0) {
                ::OutputDebugStringA("NamedPipeClient: Bytes immediately read = 0\n");
                break;
            }
            Handlers.PipeDataHandler(ReadBuffer, bytes_read);
            continue;
        }

        if (GetLastError() != ERROR_IO_PENDING) {
            ::OutputDebugStringA("NamedPipeClient: Error pipe broken\n");
            break;
        }

        HANDLE events[2] = {
            WaitEvent.Get(), TerminateEvent.Get()
        };
        DWORD wait_result = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (Terminated || wait_result != WAIT_OBJECT_0) {
            ::OutputDebugStringA("NamedPipeClient: Terminated flag set while connected\n");
            break;
        }

        ::GetOverlappedResult(PipeHandle.Get(), &overlapped, &bytes_read, FALSE);
        if (bytes_read == 0) {
            ::OutputDebugStringA("NamedPipeClient: Bytes read = 0\n");
            break;
        }

        Handlers.PipeDataHandler(ReadBuffer, bytes_read);
    }

    ::OutputDebugStringA("NamedPipeClient: Disconnecting\n");

    ::FlushFileBuffers(PipeHandle.Get());
    ::DisconnectNamedPipe(PipeHandle.Get());

    Handlers.PipeCloseHandler();
}


} // namespace core
