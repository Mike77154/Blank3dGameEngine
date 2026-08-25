cmake_minimum_required(VERSION 3.16)
project(KNM_audio VERSION 0.3.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

option(KNM_AUDIO_BUILD_TESTS "Build KNM_audio tests" ON)
option(KNM_AUDIO_BUILD_EXAMPLES "Build KNM_audio examples" ON)
option(KNM_AUDIO_STRICT_WARNINGS "Enable strict compiler warnings" ON)

option(KNM_AUDIO_ENABLE_WINMM "Enable WinMM backend" OFF)
option(KNM_AUDIO_ENABLE_WASAPI "Enable WASAPI backend" OFF)
option(KNM_AUDIO_ENABLE_ALSA "Enable ALSA backend" OFF)
option(KNM_AUDIO_ENABLE_PULSEAUDIO "Enable PulseAudio backend" OFF)
option(KNM_AUDIO_ENABLE_PIPEWIRE "Enable PipeWire backend" OFF)
option(KNM_AUDIO_ENABLE_JACK "Enable JACK backend" OFF)
option(KNM_AUDIO_ENABLE_COREAUDIO "Enable CoreAudio backend" OFF)
option(KNM_AUDIO_ENABLE_AAUDIO "Enable AAudio backend" OFF)
option(KNM_AUDIO_ENABLE_OPENSL_ES "Enable OpenSL ES backend" OFF)
option(KNM_AUDIO_ENABLE_SNDIO "Enable sndio backend" OFF)
option(KNM_AUDIO_ENABLE_WEBAUDIO "Enable WebAudio backend" OFF)
option(KNM_AUDIO_ENABLE_HAIKU "Enable Haiku backend" OFF)

set(KNM_AUDIO_SOURCES
    KNM_audio/knm_hwr_audio.c
    KNM_audio/mnk_core.c
    knm_backends/mka_audio_null.c)

set(KNM_AUDIO_PUBLIC_DEFINES
    KNM_AUDIO_ENABLE_WINMM=0
    KNM_AUDIO_ENABLE_WASAPI=0
    KNM_AUDIO_ENABLE_ALSA=0
    KNM_AUDIO_ENABLE_PULSEAUDIO=0
    KNM_AUDIO_ENABLE_PIPEWIRE=0
    KNM_AUDIO_ENABLE_JACK=0
    KNM_AUDIO_ENABLE_COREAUDIO=0
    KNM_AUDIO_ENABLE_AAUDIO=0
    KNM_AUDIO_ENABLE_OPENSL_ES=0
    KNM_AUDIO_ENABLE_SNDIO=0
    KNM_AUDIO_ENABLE_WEBAUDIO=0
    KNM_AUDIO_ENABLE_HAIKU=0)

set(KNM_AUDIO_PRIVATE_LIBS "")
set(KNM_AUDIO_PRIVATE_INCLUDES "")
find_package(PkgConfig QUIET)

macro(knm_audio_enable_define name)
    list(REMOVE_ITEM KNM_AUDIO_PUBLIC_DEFINES ${name}=0)
    list(APPEND KNM_AUDIO_PUBLIC_DEFINES ${name}=1)
endmacro()

if(KNM_AUDIO_ENABLE_WINMM AND WIN32)
    list(APPEND KNM_AUDIO_SOURCES knm_backends/mka_audio_winmm.c)
    list(APPEND KNM_AUDIO_PRIVATE_LIBS winmm)
    knm_audio_enable_define(KNM_AUDIO_ENABLE_WINMM)
endif()

if(KNM_AUDIO_ENABLE_WASAPI AND WIN32)
    list(APPEND KNM_AUDIO_SOURCES knm_backends/mka_audio_wasapi.c)
    list(APPEND KNM_AUDIO_PRIVATE_LIBS ole32 uuid)
    knm_audio_enable_define(KNM_AUDIO_ENABLE_WASAPI)
endif()

if(KNM_AUDIO_ENABLE_COREAUDIO AND APPLE)
    list(APPEND KNM_AUDIO_SOURCES knm_backends/mka_audio_coreaudio.c)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
    if(COREAUDIO_FRAMEWORK)
        list(APPEND KNM_AUDIO_PRIVATE_LIBS ${COREAUDIO_FRAMEWORK})
    endif()
    if(AUDIOUNIT_FRAMEWORK)
        list(APPEND KNM_AUDIO_PRIVATE_LIBS ${AUDIOUNIT_FRAMEWORK})
    endif()
    knm_audio_enable_define(KNM_AUDIO_ENABLE_COREAUDIO)
endif()

if(KNM_AUDIO_ENABLE_ALSA)
    if(PkgConfig_FOUND)
        pkg_check_modules(ALSA QUIET alsa)
    endif()
    if(ALSA_FOUND)
        list(APPEND KNM_AUDIO_SOURCES knm_backends/mka_audio_alsa.c)
        list(APPEND KNM_AUDIO_PRIVATE_LIBS ${ALSA_LIBRARIES})
        list(APPEND KNM_AUDIO_PRIVATE_INCLUDES ${ALSA_INCLUDE_DIRS})
        knm_audio_enable_define(KNM_AUDIO_ENABLE_ALSA)
    else()
        message(WARNING "KNM_AUDIO_ENABLE_ALSA was requested but ALSA was not found.")
    endif()
endif()

if(KNM_AUDIO_ENABLE_PULSEAUDIO)
    if(PkgConfig_FOUND)
        pkg_check_modules(PULSEAUDIO QUIET libpulse-simple)
    endif()
    if(PULSEAUDIO_FOUND)
        list(APPEND KNM_AUDIO_SOURCES knm_backends/mka_audio_pulseaudio.c)
        list(APPEND KNM_AUDIO_PRIVATE_LIBS ${PULSEAUDIO_LIBRARIES})
        list(APPEND KNM_AUDIO_PRIVATE_INCLUDES ${PULSEAUDIO_INCLUDE_DIRS})
        knm_audio_enable_define(KNM_AUDIO_ENABLE_PULSEAUDIO)
    else()
        message(WARNING "KNM_AUDIO_ENABLE_PULSEAUDIO was requested but libpulse-simple was not found.")
    endif()
endif()

if(KNM_AUDIO_ENABLE_PIPEWIRE)
    if(PkgConfig_FOUND)
        pkg_check_modules(PIPEWIRE QUIET libpipewire-0.3)
        pkg_check_modules(SPA QUIET libspa-0.2)
    endif()
    if(PIPEWIRE_FOUND AND SPA_FOUND)
        list(APPEND KNM_AUDIO_SOURCES knm_backends/mka_audio_pipewire.c)
        list(APPEND KNM_AUDIO_PRIVATE_LIBS ${PIPEWIRE_LIBRARIES} ${SPA_LIBRARIES})
        list(APPEND KNM_AUDIO_PRIVATE_INCLUDES ${PIPEWIRE_INCLUDE_DIRS} ${SPA_INCLUDE_DIRS})
        knm_audio_enable_define(KNM_AUDIO_ENABLE_PIPEWIRE)
    else()
        message(WARNING "KNM_AUDIO_ENABLE_PIPEWIRE was requested but PipeWire/SPA development files were not found.")
    endif()
endif()

if(KNM_AUDIO_ENABLE_JACK)
    if(PkgConfig_FOUND)
        pkg_check_modules(JACK QUIET jack)
    endif()
    if(JACK_FOUND)
        list(APPEND KNM_AUDIO_SOURCES knm_backends/mka_audio_jack.c)
        list(APPEND KNM_AUDIO_PRIVATE_LIBS ${JACK_LIBRARIES})
        list(APPEND KNM_AUDIO_PRIVATE_INCLUDES ${JACK_INCLUDE_DIRS})
        knm_audio_enable_define(KNM_AUDIO_ENABLE_JACK)
    else()
        message(WARNING "KNM_AUDIO_ENABLE_JACK was requested but JACK was not found.")
    endif()
endif()

if(KNM_AUDIO_ENABLE_SNDIO)
    if(PkgConfig_FOUND)
        pkg_check_modules(SNDIO QUIET sndio)
    endif()
    if(SNDIO_FOUND)
        list(APPEND KNM_AUDIO_SOURCES knm_backends/mka_audio_sndio.c)
        list(APPEND KNM_AUDIO_PRIVATE_LIBS ${SNDIO_LIBRARIES})
        list(APPEND KNM_AUDIO_PRIVATE_INCLUDES ${SNDIO_INCLUDE_DIRS})
        knm_audio_enable_define(KNM_AUDIO_ENABLE_SNDIO)
    else()
        message(WARNING "KNM_AUDIO_ENABLE_SNDIO was requested but sndio was not found.")
    endif()
endif()

add_library(knm_audio STATIC ${KNM_AUDIO_SOURCES})
set_target_properties(knm_audio PROPERTIES EXPORT_NAME knm_audio)
target_compile_features(knm_audio PRIVATE c_std_90)
target_compile_definitions(knm_audio PUBLIC ${KNM_AUDIO_PUBLIC_DEFINES})
target_include_directories(knm_audio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/KNM_audio>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/knm_audio>
    PRIVATE
        ${KNM_AUDIO_PRIVATE_INCLUDES})
target_link_libraries(knm_audio PRIVATE ${KNM_AUDIO_PRIVATE_LIBS})

if(KNM_AUDIO_STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(knm_audio PRIVATE /W4)
    else()
        target_compile_options(knm_audio PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

install(TARGETS knm_audio EXPORT KNMAudioTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/knm_audio)
install(FILES KNM_audio/knm_hwr_audio.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/knm_audio)
install(EXPORT KNMAudioTargets
    NAMESPACE KNM::
    FILE KNMAudioTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KNM_audio)

configure_package_config_file(cmake/KNMAudioConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KNMAudioConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KNM_audio)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KNMAudioConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KNMAudioConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KNMAudioConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KNM_audio)

configure_file(knm_audio.pc.in knm_audio.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/knm_audio.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if(KNM_AUDIO_BUILD_TESTS)
    enable_testing()

    foreach(test_name IN ITEMS
            test_fixed
            test_open_null
            test_backend_enum
            test_find_device
            test_service_null
            test_stats)
        add_executable(${test_name} tests/${test_name}.c)
        target_link_libraries(${test_name} PRIVATE knm_audio)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

if(KNM_AUDIO_BUILD_EXAMPLES)
    foreach(example_name IN ITEMS
            play_sine
            list_backends
            service_null)
        add_executable(${example_name} examples/${example_name}.c)
        target_link_libraries(${example_name} PRIVATE knm_audio)
    endforeach()
endif()
