#[=========================================================================[
  Copyright (c) 2022-2025 Pedro López-Cabanillas

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
]=========================================================================]

cmake_minimum_required(VERSION 3.14)

project( sonivox
    LANGUAGES C CXX
    VERSION 3.6.16.0
)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(LUCKYOS Android Darwin Linux Windows)
if (${CMAKE_SYSTEM_NAME} IN_LIST LUCKYOS)
  message(STATUS "Enabling by default the NEW_HOST_WRAPPER option for ${CMAKE_SYSTEM_NAME}")
  set(IS_LUCKY TRUE)
else()
  set(IS_LUCKY FALSE)
endif()

option(USE_44KHZ "Output 44100 Hz audio sample rate (instead of 22050 Hz)" TRUE)
option(USE_16BITS_SAMPLES "Use 16 bits samples (instead of 8 bit)" TRUE)
option(BUILD_SONIVOX_STATIC "Build the static library" TRUE)
option(BUILD_SONIVOX_SHARED "Build the shared library" TRUE)
option(BUILD_EXAMPLE "Build and install the example program" TRUE)
option(BUILD_TESTING "Build the unit tests" TRUE)
option(CMAKE_POSITION_INDEPENDENT_CODE "Whether to create position-independent targets" TRUE)
option(NEW_HOST_WRAPPER "Use the new host wrapper" ${IS_LUCKY})
set(MAX_VOICES 64 CACHE STRING "Maximum number of voices")

include(CMakeDependentOption)
cmake_dependent_option(BUILD_MANPAGE "Build the manpage of the example program" FALSE "BUILD_EXAMPLE" FALSE)

include(GNUInstallDirs)

set(PROJECT_RELEASE_DATE "April 13, 2025")

if (BUILD_TESTING)
    find_package(GTest CONFIG)
    if (GTest_FOUND)
        message( STATUS "Found GTest v${GTest_VERSION}")
    else()
        message( STATUS "GTest not found. Fetching the git repository..." )
        set( INSTALL_GTEST OFF CACHE BOOL "Enable installation of gooogletest" FORCE )
        set( BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject" FORCE )
        include( FetchContent )
        FetchContent_Declare( googletest
          GIT_REPOSITORY "https://github.com/google/googletest.git"
          GIT_TAG "v1.15.2"
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set( gtest_force_shared_crt ON CACHE BOOL "" FORCE )
        FetchContent_MakeAvailable( googletest )
    endif()
endif()

if (UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    find_library(MATH_LIBRARY m)
    if(MATH_LIBRARY)
        message(STATUS "Found Math library: ${MATH_LIBRARY}")
        list(APPEND DEPLIBS ${MATH_LIBRARY})
        list(APPEND PRIVATE_LIST "-lm")
    endif()
endif()

find_package(ZLIB)
if (ZLIB_FOUND)
    message(STATUS "Enabling XMF ZLIB Unpacker support.")
    list(APPEND DEPLIBS ZLIB::ZLIB)
    list(APPEND PRIVATE_LIST "-lz")
else()
    message(STATUS "ZLIB not found. Disabling XMF ZLIB Unpacker support.")
endif()

list(APPEND SOURCES
  arm-wt-22k/host_src/eas_config.c
#arm-wt-22k/host_src/eas_hostmm.c
#arm-wt-22k/host_src/eas_main.c
  arm-wt-22k/host_src/eas_report.c
#arm-wt-22k/host_src/eas_wave.c
  arm-wt-22k/lib_src/eas_chorus.c
  arm-wt-22k/lib_src/eas_chorusdata.c
  arm-wt-22k/lib_src/eas_data.c
  arm-wt-22k/lib_src/eas_dlssynth.c
  arm-wt-22k/lib_src/eas_flog.c
#arm-wt-22k/lib_src/eas_ima_tables.c
#arm-wt-22k/lib_src/eas_imaadpcm.c
#arm-wt-22k/lib_src/eas_imelody.c
#arm-wt-22k/lib_src/eas_imelodydata.c
  arm-wt-22k/lib_src/eas_math.c
  arm-wt-22k/lib_src/eas_mdls.c
  arm-wt-22k/lib_src/eas_midi.c
  arm-wt-22k/lib_src/eas_mididata.c
  arm-wt-22k/lib_src/eas_mixbuf.c
  arm-wt-22k/lib_src/eas_mixer.c
#arm-wt-22k/lib_src/eas_ota.c
#arm-wt-22k/lib_src/eas_otadata.c
  arm-wt-22k/lib_src/eas_pan.c
  arm-wt-22k/lib_src/eas_pcm.c
  arm-wt-22k/lib_src/eas_pcmdata.c
  arm-wt-22k/lib_src/eas_public.c
  arm-wt-22k/lib_src/eas_reverb.c
  arm-wt-22k/lib_src/eas_reverbdata.c
#arm-wt-22k/lib_src/eas_rtttl.c
#arm-wt-22k/lib_src/eas_rtttldata.c
  arm-wt-22k/lib_src/eas_smf.c
  arm-wt-22k/lib_src/eas_smfdata.c
  arm-wt-22k/lib_src/eas_tcdata.c
  arm-wt-22k/lib_src/eas_tonecontrol.c
  arm-wt-22k/lib_src/eas_voicemgt.c
#arm-wt-22k/lib_src/eas_wavefile.c
#arm-wt-22k/lib_src/eas_wavefiledata.c
  arm-wt-22k/lib_src/eas_wtengine.c
  arm-wt-22k/lib_src/eas_wtsynth.c
  arm-wt-22k/lib_src/eas_xmf.c
  arm-wt-22k/lib_src/eas_xmfdata.c
#arm-wt-22k/lib_src/jet.c
  arm-wt-22k/lib_src/wt_200k_G.c
)

if (NEW_HOST_WRAPPER)
    list(APPEND SOURCES arm-wt-22k/src/hostmm_ng.c)
else()
    list(APPEND SOURCES arm-wt-22k/host_src/eas_hostmm.c)
endif()

configure_file(arm-wt-22k/host_src/eas.cmake libsonivox/eas.h @ONLY)

list(APPEND HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/libsonivox/eas.h
    arm-wt-22k/host_src/eas_chorus.h
    arm-wt-22k/host_src/eas_reverb.h
    arm-wt-22k/host_src/eas_types.h
    arm-wt-22k/host_src/eas_report.h
#arm-wt-22k/host_src/jet.h
)

include ( CheckIncludeFile )
check_include_file ( getopt.h HAVE_GETOPT_H )

add_library( sonivox-objects OBJECT ${SOURCES} ${HEADERS} )

target_compile_definitions( sonivox-objects PRIVATE
  UNIFIED_DEBUG_MESSAGES
  EAS_WT_SYNTH
  NUM_OUTPUT_CHANNELS=2
  MAX_SYNTH_VOICES=${MAX_VOICES}
  _FILTER_ENABLED
  DLS_SYNTHESIZER
  _REVERB_ENABLED
  _CHORUS_ENABLED
  #_IMELODY_PARSER
  #_RTTTL_PARSER
  #_OTA_PARSER
  _XMF_PARSER
  #JET_INTERFACE
)

include(TestBigEndian)
test_big_endian(BIG_ENDIAN)

if (BIG_ENDIAN)
    target_compile_definitions( sonivox-objects PRIVATE EAS_BIG_ENDIAN )
endif()

if (USE_16BITS_SAMPLES)
    target_compile_definitions( sonivox-objects PRIVATE _16_BIT_SAMPLES )
else()
    target_compile_definitions( sonivox-objects PRIVATE _8_BIT_SAMPLES )
endif()

if(USE_44KHZ)
    target_compile_definitions( sonivox-objects PRIVATE _SAMPLE_RATE_44100 )
else()
    target_compile_definitions( sonivox-objects PRIVATE _SAMPLE_RATE_22050 )
endif()

if(ZLIB_FOUND)
    target_compile_definitions( sonivox-objects PRIVATE _ZLIB_UNPACKER )
endif()

target_include_directories( sonivox-objects PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/libsonivox
    arm-wt-22k/host_src
    arm-wt-22k/lib_src
    fakes
)

if (CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options( sonivox-objects PRIVATE
        -Wno-unused-parameter
        -Wno-unused-value
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-misleading-indentation
        -Wno-attributes
        -Wformat
    )
endif()

if (BUILD_SONIVOX_STATIC)
    add_library( sonivox-static STATIC $<TARGET_OBJECTS:sonivox-objects> )
    target_include_directories( sonivox-static PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/arm-wt-22k/include/libsonivox;${CMAKE_CURRENT_BINARY_DIR}/libsonivox>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/sonivox>" )
    set_target_properties( sonivox-static PROPERTIES VERSION ${PROJECT_VERSION} )
    set_target_properties( sonivox-static PROPERTIES PUBLIC_HEADER "${HEADERS}")
    target_link_libraries( sonivox-static ${DEPLIBS} )
    add_library( sonivox::sonivox-static ALIAS sonivox-static)
    list( APPEND SONIVOX_TARGETS sonivox-static )
endif()

if (BUILD_SONIVOX_SHARED)
    add_library( sonivox SHARED $<TARGET_OBJECTS:sonivox-objects> )
    target_include_directories( sonivox PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/arm-wt-22k/include/libsonivox;${CMAKE_CURRENT_BINARY_DIR}/libsonivox>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/sonivox>" )
    set_target_properties( sonivox PROPERTIES VERSION ${PROJECT_VERSION} )
    set_target_properties( sonivox PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR} )
    set_target_properties( sonivox PROPERTIES PUBLIC_HEADER "${HEADERS}" )
    target_link_libraries( sonivox PRIVATE ${DEPLIBS} )
    #target_link_options( sonivox PRIVATE "LINKER:-z,defs" )
    add_library( sonivox::sonivox ALIAS sonivox )
    list( APPEND SONIVOX_TARGETS sonivox )
endif()

if(IS_ABSOLUTE ${CMAKE_INSTALL_LIBDIR})
    set(sonivox_libdir "${CMAKE_INSTALL_LIBDIR}")
else()
    set(sonivox_libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()
if(IS_ABSOLUTE ${CMAKE_INSTALL_INCLUDEDIR})
    set(sonivox_includedir "${CMAKE_INSTALL_INCLUDEDIR}")
else()
    set(sonivox_includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif()

list(JOIN PRIVATE_LIST " " PRIVATE_LIBS)
if (BUILD_SONIVOX_STATIC)
    configure_file(sonivox-static.pc.in ${CMAKE_CURRENT_BINARY_DIR}/sonivox-static.pc @ONLY)
    install( FILES ${CMAKE_CURRENT_BINARY_DIR}/sonivox-static.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig )
endif()
if (BUILD_SONIVOX_SHARED)
    configure_file(sonivox.pc.in ${CMAKE_CURRENT_BINARY_DIR}/sonivox.pc @ONLY)
    install( FILES ${CMAKE_CURRENT_BINARY_DIR}/sonivox.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig )
endif()

install( TARGETS ${SONIVOX_TARGETS}
    EXPORT sonivox-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sonivox
)

install( EXPORT sonivox-targets
        FILE ${PROJECT_NAME}-targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

export( EXPORT sonivox-targets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake"
        NAMESPACE ${PROJECT_NAME}::
)

include( CMakePackageConfigHelpers )

write_basic_package_version_file(
        ${PROJECT_NAME}-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
        ${PROJECT_NAME}-config.cmake.in
        ${PROJECT_NAME}-config.cmake
        INSTALL_DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

install( FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Unit testing
if (BUILD_TESTING)
    enable_testing()

    add_executable( SonivoxTest
        test/SonivoxTest.cpp
    )

    if(USE_44KHZ)
        target_compile_definitions( SonivoxTest PRIVATE _SAMPLE_RATE_44100 )
    else()
        target_compile_definitions( SonivoxTest PRIVATE _SAMPLE_RATE_22050 )
    endif()

    target_include_directories( SonivoxTest PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        arm-wt-22k/include
        fakes
    )

    if (NOT HAVE_GETOPT_H)
        target_sources( SonivoxTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/getopt_port/getopt.c )
        target_include_directories( SonivoxTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/getopt_port )
    endif()

    target_link_libraries( SonivoxTest PRIVATE
        GTest::gtest_main
    )

    if (BUILD_SONIVOX_STATIC)
        target_link_libraries( SonivoxTest PRIVATE
            sonivox-static
        )
    elseif (BUILD_SONIVOX_SHARED)
        target_link_libraries( SonivoxTest PRIVATE
            sonivox
        )
    endif()

    if(DEFINED ENV{TEMP})
        set(TEMPDIR "$ENV{TEMP}")
    elseif(DEFINED ENV{XDG_RUNTIME_DIR})
        set(TEMPDIR "$ENV{XDG_RUNTIME_DIR}")
    else()
        message(FATAL_ERROR "Cannot find a temporary directory. Define the TEMP environment variable.")
    endif()

    set(SOUNDFONT "${TEMPDIR}/soundfont.dls")
    if (NOT EXISTS ${SOUNDFONT})
        message(STATUS "Downloading DLS file for testing to ${SOUNDFONT}")
        file(DOWNLOAD "http://www.ronimusic.com/sf2/Airfont_340.dls" ${SOUNDFONT})
    endif()

    file(SIZE ${SOUNDFONT} SOUNDFONTSIZE)
    file(MD5 ${SOUNDFONT} SOUNDFONTHASH)
    message(STATUS "DLS file ${SOUNDFONT} size: ${SOUNDFONTSIZE}")
    message(STATUS "DLS file ${SOUNDFONT} MD5 hash: ${SOUNDFONTHASH}")
    if (NOT (SOUNDFONTSIZE EQUAL 81362584 AND SOUNDFONTHASH STREQUAL "40c0cd4ad29ae411a8fc3d6681002a2b"))
        message(FATAL_ERROR "The downloaded DLS file is corrupted.")
    endif()

    include( GoogleTest )
    gtest_discover_tests( SonivoxTest EXTRA_ARGS "-P${CMAKE_CURRENT_SOURCE_DIR}/test/res/" DISCOVERY_TIMEOUT 300 )
endif()

# Example program
if (BUILD_EXAMPLE)
    set(sonivox_DIR ${CMAKE_CURRENT_BINARY_DIR})
    add_subdirectory(example)
    install( TARGETS sonivoxrender
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
