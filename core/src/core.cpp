#include "core.hpp"
#include "core_serializer.hpp"

#if !defined(_WIN32)
    #include <pthread.h>
    #include <unistd.h>
#endif // _WIN32

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif __MACH__
    #include <sys/file.h>
    #include <mach/mach_time.h>
    #include <mach/mach.h>
    #include <mach/clock.h>

    extern mach_port_t clock_port;
#else
    #include <time.h>
    #include <sys/time.h>
    #include <sys/file.h> // flock
#endif

namespace core {


//------------------------------------------------------------------------------
// Timing

#ifdef _WIN32
// Precomputed frequency inverse
static double PerfFrequencyInverseUsec = 0.;
static double PerfFrequencyInverseMsec = 0.;

static void InitPerfFrequencyInverse()
{
    LARGE_INTEGER freq = {};
    if (!::QueryPerformanceFrequency(&freq) || freq.QuadPart == 0) {
        return;
    }
    const double invFreq = 1. / (double)freq.QuadPart;
    PerfFrequencyInverseUsec = 1000000. * invFreq;
    PerfFrequencyInverseMsec = 1000. * invFreq;
    CORE_DEBUG_ASSERT(PerfFrequencyInverseUsec > 0.);
    CORE_DEBUG_ASSERT(PerfFrequencyInverseMsec > 0.);
}
#elif __MACH__
static bool m_clock_serv_init = false;
static clock_serv_t m_clock_serv = 0;

static void InitClockServ()
{
    m_clock_serv_init = true;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &m_clock_serv);
}
#endif // _WIN32

uint64_t GetTimeUsec()
{
#ifdef _WIN32
    LARGE_INTEGER timeStamp = {};
    if (!::QueryPerformanceCounter(&timeStamp)) {
        return 0;
    }
    if (PerfFrequencyInverseUsec == 0.) {
        InitPerfFrequencyInverse();
    }
    return (uint64_t)(PerfFrequencyInverseUsec * timeStamp.QuadPart);
#elif __MACH__
    if (!m_clock_serv_init) {
        InitClockServ();
    }

    mach_timespec_t tv;
    clock_get_time(m_clock_serv, &tv);

    return 1000000 * tv.tv_sec + tv.tv_nsec / 1000;
#else
    // This seems to be the best clock to used based on:
    // http://btorpey.github.io/blog/2014/02/18/clock-sources-in-linux/
    // The CLOCK_MONOTONIC_RAW seems to take a long time to query,
    // and so it only seems useful for timing code that runs a small number of times.
    // The CLOCK_MONOTONIC is affected by NTP at 500ppm but doesn't make sudden jumps.
    // Applications should already be robust to clock skew so this is acceptable.
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_nsec / 1000) + static_cast<uint64_t>(ts.tv_sec) * 1000000;
#endif
}

uint64_t GetTimeMsec()
{
#ifdef _WIN32
    LARGE_INTEGER timeStamp = {};
    if (!::QueryPerformanceCounter(&timeStamp)) {
        return 0;
    }
    if (PerfFrequencyInverseMsec == 0.) {
        InitPerfFrequencyInverse();
    }
    return (uint64_t)(PerfFrequencyInverseMsec * timeStamp.QuadPart);
#else
    // TBD: Optimize this?
    return GetTimeUsec() / 1000;
#endif
}


//------------------------------------------------------------------------------
// TimeoutTimer

void TimeoutTimer::SetTimeout(uint64_t timeout_msec)
{
    TimeoutMsec = timeout_msec;
}

void TimeoutTimer::Reset()
{
    const uint64_t now_msec = GetTimeMsec();

    LastTickMsec = now_msec;
    TimeoutCount = 0;
}

bool TimeoutTimer::Timeout()
{
    // If already timed out:
    if (TimeoutCount >= 4) {
        return true;
    }

    const uint64_t now_msec = GetTimeMsec();

    // If timeout expired:
    if (now_msec - LastTickMsec > TimeoutMsec / 4) {
        // Increment timeout count
        ++TimeoutCount;
        if (TimeoutCount >= 4) {
            return true;
        }

        // Reset wait at the current time and wait to timeout again
        LastTickMsec = now_msec;
    }

    return false;
}


//------------------------------------------------------------------------------
// Process Tools

bool IsAlreadyRunning(const std::string& name)
{
#ifdef _WIN32
    std::string mutex_name = "Local\\";
    mutex_name += name;

    ::CreateMutexA(0, FALSE, mutex_name.c_str());
    return GetLastError() == ERROR_ALREADY_EXISTS;
#else
    std::string filename = name + ".instlock";
    FILE* file = fopen(filename.c_str(), "w");
    if (file) {
        const int lock_result = flock(fileno(file), LOCK_EX | LOCK_NB);
        if (lock_result == 0) {
            return false;
        }
        if (errno == EWOULDBLOCK) {
            return true;
        }
    }
    CORE_DEBUG_BREAK(); // Should never happen
    return false;
#endif
}


//------------------------------------------------------------------------------
// Thread Tools

#ifdef _WIN32
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType;       // Must be 0x1000.
    LPCSTR szName;      // Pointer to name (in user addr space).
    DWORD dwThreadID;   // Thread ID (-1=caller thread).
    DWORD dwFlags;      // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
void SetCurrentThreadName(const char* threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = ::GetCurrentThreadId();
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#pragma warning(pop)
}
#elif __MACH__
void SetCurrentThreadName(const char* threadName)
{
    pthread_setname_np(threadName);
}
#else
void SetCurrentThreadName(const char* threadName)
{
    pthread_setname_np(pthread_self(), threadName);
}
#endif


//------------------------------------------------------------------------------
// SIMD-Safe Aligned Memory Allocations

static const unsigned kAlignmentBytes = 32;

static inline uint8_t* SIMDSafeAllocate(size_t size)
{
    uint8_t* data = (uint8_t*)calloc(1, kAlignmentBytes + size);
    if (!data) {
        return nullptr;
    }
    unsigned offset = (unsigned)((uintptr_t)data % kAlignmentBytes);
    data += kAlignmentBytes - offset;
    data[-1] = (uint8_t)offset;
    return data;
}

static inline void SIMDSafeFree(void* ptr)
{
    if (!ptr) {
        return;
    }
    uint8_t* data = (uint8_t*)ptr;
    unsigned offset = data[-1];
    if (offset >= kAlignmentBytes) {
        CORE_DEBUG_BREAK(); // Should never happen
        return;
    }
    data -= kAlignmentBytes - offset;
    free(data);
}


} // namespace core
