cmake_minimum_required(VERSION 3.5)
project(core LANGUAGES CXX)


################################################################################
# Source

set(INCLUDE_FILES
    include/core.hpp
    include/core_bit_math.hpp
    include/core_counter_math.hpp
    include/core_logger.hpp
    include/core_mmap.hpp
    include/core_serializer.hpp
    include/core_string.hpp
    include/core_win32.hpp
    include/core_win32_pipe.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/core.cpp
    src/core_logger.cpp
    src/core_mmap.cpp
    src/core_serializer.cpp
    src/core_string.cpp
    src/core_win32.cpp
    src/core_win32_pipe.cpp
)


################################################################################
# Build Options

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# In debug mode, add -DDEBUG
add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    # Warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

    # Remove Asio warning
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")

    # Static library: -fPIC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -march=native -fstack-protector")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")
endif()


################################################################################
# Dependencies

include_directories(include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


################################################################################
# Targets

add_library(core STATIC ${SOURCE_FILES})
set_property(TARGET core PROPERTY CXX_STANDARD 17)
target_include_directories(core PUBLIC include)
target_link_libraries(core
    Threads::Threads
)

install(TARGETS core DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)
