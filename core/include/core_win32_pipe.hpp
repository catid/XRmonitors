#pragma once

#include "core_win32.hpp"

#include <vector>
#include <atomic>
#include <thread>
#include <memory>

namespace core {


//------------------------------------------------------------------------------
// Common Tools

struct PipeHandlers
{
    std::function<void(uint8_t* data, uint32_t bytes)> PipeDataHandler;
    std::function<void()> PipeConnectHandler;
    std::function<void()> PipeCloseHandler;
};


//------------------------------------------------------------------------------
// NamedPipeServer

class NamedPipeServer
{
public:
    bool Initialize(
        const std::string& pipe_name,
        const PipeHandlers& handlers);
    void Shutdown();

    void Write(const void* data, uint32_t bytes);

protected:
    static const int kMaxMessageBytes = 3500;

    std::string PipeName;
    PipeHandlers Handlers;
    AutoHandle PipeHandle;
    AutoEvent TerminateEvent;
    AutoEvent WaitEvent;
    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
    std::shared_ptr<std::thread> PipeThread;
    uint8_t ReadBuffer[kMaxMessageBytes];

    void ThreadLoop();
    void OnConnect();
};


//------------------------------------------------------------------------------
// NamedPipeClient

class NamedPipeClient
{
public:
    bool Connect(
        const std::string& pipe_name,
        const PipeHandlers& handlers);
    void Shutdown();

    void Write(const void* data, uint32_t bytes);

protected:
    static const int kMaxMessageBytes = 3500;

    std::string PipeName;
    PipeHandlers Handlers;
    AutoHandle PipeHandle;
    AutoEvent TerminateEvent;
    AutoEvent WaitEvent;
    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
    std::shared_ptr<std::thread> PipeThread;
    uint8_t ReadBuffer[kMaxMessageBytes];

    void ThreadLoop();
};


} // namespace core
