// Copyright 2019 Augmented Perception Corporation

#pragma once

#include <functional>

#include "openxr/openxr.h"

namespace xrm {


//------------------------------------------------------------------------------
// XrHandle

template <typename HandleType, XrResult(XRAPI_PTR* DestroyFunction)(HandleType)>
class XrHandle {
public:
    XrHandle() = default;
    XrHandle(const XrHandle&) = delete;
    XrHandle(XrHandle&& other) noexcept {
        *this = std::move(other);
    }

    ~XrHandle() noexcept {
        Reset();
    }

    XrHandle& operator=(const XrHandle&) = delete;
    XrHandle& operator=(XrHandle&& other) noexcept {
        m_handle = other.m_handle;
        other.m_handle = XR_NULL_HANDLE;
        return *this;
    }

    HandleType Get() const noexcept {
        return m_handle;
    }

    HandleType* Put() noexcept {
        Reset();
        return &m_handle;
    }

    void Reset(HandleType newHandle = XR_NULL_HANDLE) noexcept {
        if (m_handle != XR_NULL_HANDLE) {
            DestroyFunction(m_handle);
        }
        m_handle = newHandle;
    }

private:
    HandleType m_handle{XR_NULL_HANDLE};
};


//------------------------------------------------------------------------------
// XrHandle Types

using XrActionHandle = XrHandle<XrAction, xrDestroyAction>;
using XrActionSetHandle = XrHandle<XrActionSet, xrDestroyActionSet>;
using XrInstanceHandle = XrHandle<XrInstance, xrDestroyInstance>;
using XrSessionHandle = XrHandle<XrSession, xrDestroySession>;
using XrSpaceHandle = XrHandle<XrSpace, xrDestroySpace>;
using XrSwapchainHandle = XrHandle<XrSwapchain, xrDestroySwapchain>;
using XrSpatialAnchorHandle = XrHandle<XrSpatialAnchorMSFT, xrDestroySpatialAnchorMSFT>;


} // namespace xrm
