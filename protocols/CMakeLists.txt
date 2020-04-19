cmake_minimum_required(VERSION 3.5)
project(protocols LANGUAGES CXX)

################################################################################
# Source

set(INCLUDE_FILES
    include/implant_abi.hpp
    include/xrm_plugins_abi.hpp
    include/xrm_ui_abi.hpp
)

set(SOURCE_FILES
    ${INCLUDE_FILES}
    src/implant_abi.cpp
    src/xrm_plugins_abi.cpp
    src/xrm_ui_abi.cpp
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


################################################################################
# Targets

add_library(protocols STATIC ${SOURCE_FILES})
target_include_directories(protocols PUBLIC include)

install(TARGETS protocols DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include)
