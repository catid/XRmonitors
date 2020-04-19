//*********************************************************
//    Copyright (c) Microsoft. All rights reserved.
//
//    Apache 2.0 License
//
//    You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
//*********************************************************
#pragma once

#include "core.hpp"

#include <openxr/openxr.h>

#define XR_CHECK_XRCMD(cmd) xr::detail::_CheckXrResult(cmd, #cmd, XR_FILE_AND_LINE);
#define XR_CHECK_XRRESULT(res, cmdStr) xr::detail::_CheckXrResult(res, cmdStr, XR_FILE_AND_LINE);

#define XR_CHECK_HRCMD(cmd) xr::detail::_CheckHResult(cmd, #cmd, XR_FILE_AND_LINE);
#define XR_CHECK_HRESULT(res, cmdStr) xr::detail::_CheckHResult(res, cmdStr, XR_FILE_AND_LINE);

// Convert result to string without using a specific instance handle
const char* XrResultToString(XrResult res);

namespace xr::detail {
#define XR_CHK_STRINGIFY(x) #x
#define XR_TOSTRING(x) XR_CHK_STRINGIFY(x)
#define XR_FILE_AND_LINE __FILE__ ":" XR_TOSTRING(__LINE__)

    inline std::string _Fmt(const char* fmt, ...) {
        va_list vl;
        va_start(vl, fmt);
        int size = std::vsnprintf(nullptr, 0, fmt, vl);
        va_end(vl);

        if (size != -1) {
            std::unique_ptr<char[]> buffer(new char[size + 1]);

            va_start(vl, fmt);
            size = std::vsnprintf(buffer.get(), size + 1, fmt, vl);
            va_end(vl);
            if (size != -1) {
                return std::string(buffer.get(), size);
            }
        }

        throw std::runtime_error("Unexpected vsnprintf failure");
    }

    [[noreturn]] inline void _Throw(std::string failureMessage, const char* originator = nullptr, const char* sourceLocation = nullptr) {
        if (originator != nullptr) {
            failureMessage += _Fmt("\n    Origin: %s", originator);
        }
        if (sourceLocation != nullptr) {
            failureMessage += _Fmt("\n    Source: %s", sourceLocation);
        }

        throw std::logic_error(failureMessage);
    }

#define XR_THROW(msg) xr::detail::_Throw(msg, nullptr, XR_FILE_AND_LINE);
#define XR_CHECK(exp)                                                   \
    {                                                                \
        if (!(exp)) {                                                \
            CORE_DEBUG_BREAK(); \
            xr::detail::_Throw("Check failed", #exp, XR_FILE_AND_LINE); \
        }                                                            \
    }
#define XR_CHECK_MSG(exp, msg)                               \
    {                                                     \
        if (!(exp)) {                                     \
            CORE_DEBUG_BREAK(); \
            xr::detail::_Throw(msg, #exp, XR_FILE_AND_LINE); \
        }                                                 \
    }

    [[noreturn]] inline void _ThrowXrResult(XrResult res, const char* originator = nullptr, const char* sourceLocation = nullptr) {
        xr::detail::_Throw(xr::detail::_Fmt("XrResult failure: %s [%d]", XrResultToString(res), res), originator, sourceLocation);
    }

    inline XrResult _CheckXrResult(XrResult res, const char* originator = nullptr, const char* sourceLocation = nullptr) {
        if (FAILED(res)) {
            xr::detail::_ThrowXrResult(res, originator, sourceLocation);
        }

        return res;
    }

    [[noreturn]] inline void _ThrowHResult(HRESULT hr, const char* originator = nullptr, const char* sourceLocation = nullptr) {
        xr::detail::_Throw(xr::detail::_Fmt("HRESULT failure [%x]", hr), originator, sourceLocation);
    }

    inline HRESULT _CheckHResult(HRESULT hr, const char* originator = nullptr, const char* sourceLocation = nullptr) {
        if (FAILED(hr)) {
            xr::detail::_ThrowHResult(hr, originator, sourceLocation);
        }

        return hr;
    }
} // namespace xr::detail
