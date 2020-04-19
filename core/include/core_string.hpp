#pragma once

#include "core.hpp"
#include "core_bit_math.hpp" // NonzeroLowestBitIndex

#include <string.h>
#include <string>
#include <sstream> // android to_string

namespace core {


//------------------------------------------------------------------------------
// String Conversion

/// Convert buffer to hex string
std::string HexDump(const uint8_t* data, int bytes);

/// Convert value to hex string
std::string HexString(uint64_t value);


//------------------------------------------------------------------------------
// Copy Strings

CORE_INLINE void SafeCopyCStr(char* dest, size_t destBytes, const char* src)
{
#if defined(_MSC_VER)
    ::strncpy_s(dest, destBytes, src, _TRUNCATE);
#else // _MSC_VER
    ::strncpy(dest, src, destBytes);
#endif // _MSC_VER
    // No null-character is implicitly appended at the end of destination
    dest[destBytes - 1] = '\0';
}


//------------------------------------------------------------------------------
// Compare Strings

// Portable version of stristr()
char* StrIStr(const char* s1, const char* s2);

/// Case-insensitive string comparison
/// Returns < 0 if a < b (lexographically ignoring case)
/// Returns 0 if a == b (case-insensitive)
/// Returns > 0 if a > b (lexographically ignoring case)
CORE_INLINE int StrCaseCompare(const char* a, const char* b)
{
#if defined(_WIN32)
# if defined(_MSC_VER) && (_MSC_VER >= 1400)
    return ::_stricmp(a, b);
# else
    return ::stricmp(a, b);
# endif
#else
    return ::strcasecmp(a, b);
#endif
}

// Portable version of strnicmp()
CORE_INLINE int StrNCaseCompare(const char* a, const char* b, size_t count)
{
#if defined(_MSC_VER)
#if (_MSC_VER >= 1400)
    return ::_strnicmp(a, b, count);
#else
    return ::strnicmp(a, b, count);
#endif
#else
    return strncasecmp(a, b, count);
#endif
}


} // namespace core


namespace std {


//------------------------------------------------------------------------------
// Android Portability

#if defined(ANDROID) && !defined(DEFINED_TO_STRING)
# define DEFINED_TO_STRING

/// Polyfill for to_string() on Android
template<typename T> string to_string(T value)
{
    ostringstream os;
    os << value;
    return os.str();
}

#endif // DEFINED_TO_STRING


} // namespace std
