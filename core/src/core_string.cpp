#include "core_string.hpp"
#include "core_serializer.hpp"

#include <fstream> // ofstream
#include <iomanip> // setw, setfill
#include <cctype>

namespace core {


//------------------------------------------------------------------------------
// String Conversion

static const char* HEX_ASCII = "0123456789abcdef";

std::string HexString(uint64_t value)
{
    char hex[16 + 1];
    hex[16] = '\0';

    char* hexWrite = &hex[14];
    for (unsigned i = 0; i < 8; ++i)
    {
        hexWrite[1] = HEX_ASCII[value & 15];
        hexWrite[0] = HEX_ASCII[(value >> 4) & 15];

        value >>= 8;
        if (value == 0)
            return hexWrite;
        hexWrite -= 2;
    }

    return hex;
}

std::string HexDump(const uint8_t* data, int bytes)
{
    std::ostringstream oss;

    char hex[8 * 3];

    while (bytes > 8)
    {
        const uint64_t word = ReadU64_LE(data);

        for (unsigned i = 0; i < 8; ++i)
        {
            const uint8_t value = static_cast<uint8_t>(word >> (i * 8));
            hex[i * 3] = HEX_ASCII[(value >> 4) & 15];
            hex[i * 3 + 1] = HEX_ASCII[value & 15];
            hex[i * 3 + 2] = ' ';
        }

        oss.write(hex, 8 * 3);

        data += 8;
        bytes -= 8;
    }

    if (bytes > 0) {
        const uint64_t word = ReadBytes64_LE(data, bytes);

        for (int i = 0; i < bytes; ++i)
        {
            const uint8_t value = static_cast<uint8_t>(word >> (i * 8));
            hex[i * 3] = HEX_ASCII[(value >> 4) & 15];
            hex[i * 3 + 1] = HEX_ASCII[value & 15];
            hex[i * 3 + 2] = ' ';
        }

        oss.write(hex, bytes * 3);
    }

    return oss.str();
}


//------------------------------------------------------------------------------
// String Helpers

// Portable version of stristr()
char* StrIStr(const char* s1, const char* s2)
{
    const char* cp = s1;

    if (!*s2)
        return const_cast<char*>(s1);

    while (*cp) {
        const char* s = cp;
        const char* t = s2;

        while (*s && *t && (std::tolower((uint8_t)*s) == std::tolower((uint8_t)*t)))
            ++s, ++t;

        if (*t == 0)
            return const_cast<char*>(cp);
        ++cp;
    }

    return nullptr;
}


} // namespace core
