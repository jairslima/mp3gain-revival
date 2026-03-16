cmake_minimum_required(VERSION 3.16)

project(mp3gain VERSION 1.6.2 LANGUAGES C)

option(MP3GAIN_ENABLE_WIN_RESOURCE "Build Windows version resource if a resource compiler is available" ON)

set(MP3GAIN_SOURCES
    apetag.c
    cli.c
    exec.c
    gain_analysis.c
    id3tag.c
    mp3gain.c
    prep.c
    process.c
    rg_error.c
)

set(MP3GAIN_HEADERS
    apetag.h
    cli.h
    exec.h
    gain_analysis.h
    id3tag.h
    mp3gain.h
    prep.h
    process.h
    rg_error.h
)

add_executable(mp3gain ${MP3GAIN_SOURCES} ${MP3GAIN_HEADERS})

target_compile_definitions(mp3gain PRIVATE HAVE_MEMCPY)
target_include_directories(mp3gain PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(mp3gain PRIVATE c_std_99)

if(MSVC)
    target_compile_definitions(mp3gain PRIVATE _CRT_SECURE_NO_DEPRECATE)
else()
    target_compile_options(mp3gain PRIVATE -Wall)
endif()

include(CheckLibraryExists)
check_library_exists(m log10 "" HAVE_LIBM)
if(HAVE_LIBM)
    target_link_libraries(mp3gain PRIVATE m)
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(MPG123 QUIET IMPORTED_TARGET libmpg123)
endif()

if(TARGET PkgConfig::MPG123)
    target_link_libraries(mp3gain PRIVATE PkgConfig::MPG123)
else()
    find_path(MPG123_INCLUDE_DIR mpg123.h)
    find_library(MPG123_LIBRARY NAMES mpg123 libmpg123)

    if(NOT MPG123_INCLUDE_DIR OR NOT MPG123_LIBRARY)
        message(FATAL_ERROR "libmpg123 not found. Install libmpg123 development files or provide MPG123_INCLUDE_DIR and MPG123_LIBRARY.")
    endif()

    target_include_directories(mp3gain PRIVATE ${MPG123_INCLUDE_DIR})
    target_link_libraries(mp3gain PRIVATE ${MPG123_LIBRARY})
endif()

if(WIN32 AND MP3GAIN_ENABLE_WIN_RESOURCE AND CMAKE_RC_COMPILER)
    configure_file(mp3gain.rc.in mp3gain.rc @ONLY)
    target_sources(mp3gain PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/mp3gain.rc)
endif()
