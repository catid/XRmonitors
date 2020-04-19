#pragma once

#include "core.hpp"
#include "core_bit_math.hpp"
#include "core_counter_math.hpp"
#include "core_logger.hpp"
#include "core_string.hpp"
#include "core_win32.hpp" // Include first to adjust windows.h include
#include "core_win32_pipe.hpp"
#include "core_serializer.hpp"
#include "core_mmap.hpp"

#include <DXGI.h>
#include <d3d11_4.h>
#include <DirectXColors.h>

#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <future>
#include <map>
#include <mutex>
#include <vector>
#include <array>
#include <atomic>
#include <memory>
#include <thread>
#include <list>

#include <wrl/client.h>

#include "OpenXrD3D11.hpp"
