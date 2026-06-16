cmake_minimum_required(VERSION 3.18)
project(Denoiser)

set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# OpenImageIO is provided by Conan (see conanfile.txt). The CMakeDeps generator
# writes its config package into the build tree (referenced via
# CMAKE_TOOLCHAIN_FILE / CMAKE_PREFIX_PATH), exposing the imported targets
# OpenImageIO::OpenImageIO and OpenImageIO::OpenImageIO_Util.
find_package(OpenImageIO CONFIG REQUIRED)

# Intel Open Image Denoise (OIDN) is downloaded as a pre-built release from
# GitHub at configure time and extracted into the build tree.
set(OIDN_VERSION "2.5.0" CACHE STRING "OpenImageDenoise release version to download")
set(OIDN_BASE_URL "https://github.com/RenderKit/oidn/releases/download/v${OIDN_VERSION}")

if(WIN32)
    set(OIDN_PKG "oidn-${OIDN_VERSION}.x64.windows")
    set(OIDN_ARCHIVE "${OIDN_PKG}.zip")
elseif(APPLE)
    set(OIDN_PKG "oidn-${OIDN_VERSION}.x86_64.macos")
    set(OIDN_ARCHIVE "${OIDN_PKG}.tar.gz")
else()
    set(OIDN_PKG "oidn-${OIDN_VERSION}.x86_64.linux")
    set(OIDN_ARCHIVE "${OIDN_PKG}.tar.gz")
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()

set(OIDN_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/oidn")
set(OIDN_ARCHIVE_PATH "${OIDN_DOWNLOAD_DIR}/${OIDN_ARCHIVE}")
set(OIDN_ROOT "${OIDN_DOWNLOAD_DIR}/${OIDN_PKG}")

# Only download/extract if we don't already have a usable copy.
if(NOT EXISTS "${OIDN_ROOT}/include/OpenImageDenoise/oidn.hpp")
    if(NOT EXISTS "${OIDN_ARCHIVE_PATH}")
        message(STATUS "Downloading OpenImageDenoise ${OIDN_VERSION}: ${OIDN_BASE_URL}/${OIDN_ARCHIVE}")
        file(DOWNLOAD
            "${OIDN_BASE_URL}/${OIDN_ARCHIVE}"
            "${OIDN_ARCHIVE_PATH}"
            SHOW_PROGRESS
            TLS_VERIFY ON
            STATUS OIDN_DL_STATUS)
        list(GET OIDN_DL_STATUS 0 OIDN_DL_CODE)
        if(NOT OIDN_DL_CODE EQUAL 0)
            list(GET OIDN_DL_STATUS 1 OIDN_DL_MSG)
            file(REMOVE "${OIDN_ARCHIVE_PATH}")
            message(FATAL_ERROR "Failed to download OpenImageDenoise (${OIDN_DL_CODE}): ${OIDN_DL_MSG}")
        endif()
    endif()
    message(STATUS "Extracting ${OIDN_ARCHIVE}")
    file(ARCHIVE_EXTRACT
        INPUT "${OIDN_ARCHIVE_PATH}"
        DESTINATION "${OIDN_DOWNLOAD_DIR}")
endif()

find_package(OpenImageDenoise REQUIRED)

message(STATUS "Found OpenImageIO ${OpenImageIO_VERSION}: ${OpenImageIO_DIR}")
message(STATUS "Found OpenImageDenoise:"
               " (includes: ${OIDN_INCLUDE_DIRS}, library: ${OIDN_LIBRARIES})")

# Source files
set(SOURCES src/main.cpp)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
endif()

# Link libraries. The OpenImageIO imported targets carry their own include
# directories and transitive dependencies, so no manual paths are needed.
target_link_libraries(${PROJECT_NAME}
                      OpenImageDenoise::OpenImageDenoise
                      OpenImageIO::OpenImageIO
                      OpenImageIO::OpenImageIO_Util
                      ${CMAKE_DL_LIBS})

# Collect the OIDN runtime libraries (the API stub plus its device back-ends and
# TBB) so they can sit next to the executable.
if(WIN32)
    file(GLOB OIDN_RUNTIME_LIBS "${OIDN_ROOT}/bin/*.dll")
else()
    file(GLOB OIDN_RUNTIME_LIBS "${OIDN_ROOT}/lib/*.so*" "${OIDN_ROOT}/lib/*.dylib*")
    # Let the executable find the OIDN shared libraries it sits beside.
    if(APPLE)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            BUILD_RPATH "@loader_path" INSTALL_RPATH "@loader_path")
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES
            BUILD_RPATH "$ORIGIN" INSTALL_RPATH "$ORIGIN")
        # Emit old-style DT_RPATH instead of DT_RUNPATH. DT_RUNPATH is not
        # applied to transitive (indirect) dependencies, so the executable's
        # $ORIGIN would not be searched when libOpenImageDenoise.so loads its
        # own back-end libraries (libOpenImageDenoise_core.so, etc.) that sit
        # beside the binary. DT_RPATH is honoured transitively and fixes this.
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--disable-new-dtags")
    endif()
endif()

# Copy the OIDN runtime libraries next to the executable so the app can be run
# straight from the build tree.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OIDN_RUNTIME_LIBS} $<TARGET_FILE_DIR:${PROJECT_NAME}>)

# Install app and its runtime dependencies.
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES ${OIDN_RUNTIME_LIBS}
    DESTINATION ${CMAKE_INSTALL_BINDIR})
