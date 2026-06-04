# Locate the Intel Open Image Denoise (OIDN) library.
#
# OIDN is not provided via Conan here; instead a pre-built release is downloaded
# from GitHub and extracted by the top level CMakeLists.txt, which sets OIDN_ROOT
# to the extracted package directory. The standard system locations are also
# searched as a fallback (e.g. an OIDN installed via Homebrew on macOS).

FIND_PATH(OIDN_INCLUDE_DIR OpenImageDenoise/oidn.hpp
    $ENV{OIDN_DIR}/include
    $ENV{OIDN_DIR}
    ${OIDN_ROOT}/include
    ${OIDN_ROOT}
    /opt/homebrew/include
    /usr/local/include
    /usr/include
)

FIND_LIBRARY(OIDN_LIBRARY
    NAMES OpenImageDenoise
    PATHS
    $ENV{OIDN_DIR}/lib
    ${OIDN_ROOT}/lib
    /opt/homebrew/lib
    /usr/local/lib
    /usr/lib
)

# OIDN depends on Intel TBB. On Windows the matching import library ships
# alongside it in contrib/, so pick it up when present.
FIND_LIBRARY(OIDN_TBB_LIBRARY
    NAMES tbb tbb12
    PATHS
    $ENV{OIDN_DIR}/lib
    ${OIDN_ROOT}/lib
    /opt/homebrew/lib
    /usr/local/lib
    /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenImageDenoise
    REQUIRED_VARS OIDN_LIBRARY OIDN_INCLUDE_DIR)

if(OpenImageDenoise_FOUND)
    set(OIDN_INCLUDE_DIRS ${OIDN_INCLUDE_DIR})
    set(OIDN_LIBRARIES ${OIDN_LIBRARY})
    if(OIDN_TBB_LIBRARY)
        list(APPEND OIDN_LIBRARIES ${OIDN_TBB_LIBRARY})
    endif()

    if(NOT TARGET OpenImageDenoise::OpenImageDenoise)
        add_library(OpenImageDenoise::OpenImageDenoise UNKNOWN IMPORTED)
        set_target_properties(OpenImageDenoise::OpenImageDenoise PROPERTIES
            IMPORTED_LOCATION "${OIDN_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OIDN_INCLUDE_DIR}")
        if(OIDN_TBB_LIBRARY)
            set_property(TARGET OpenImageDenoise::OpenImageDenoise APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${OIDN_TBB_LIBRARY}")
        endif()
    endif()
endif()
