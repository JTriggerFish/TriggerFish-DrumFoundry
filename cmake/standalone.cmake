# Compile only the device backends we actually ship. No dependency examples,
# alternative compiler targets, bindings, drivers or installer scripts.
FetchContent_Declare(rtaudio
  URL https://codeload.github.com/thestk/rtaudio/tar.gz/c0a533d7bb16e8ca0d96cdb2e3fcfb6d1d095df4
  URL_HASH SHA256=92f7fcf8b3df9725d0ded712c3414613bef6461a2054248a88c06ba3414f81a8
  SOURCE_SUBDIR unused)
FetchContent_Declare(rtmidi
  URL https://codeload.github.com/thestk/rtmidi/tar.gz/a3233c22949342f6697681e2cf2403e27fcf0c9e
  URL_HASH SHA256=2afbdebe844172ec23726cb4fd4efbe982c95955277e1794dde8c8f348ecdfdf
  SOURCE_SUBDIR unused)
FetchContent_MakeAvailable(rtaudio rtmidi)
include(cmake/rtaudio-probe.cmake)
include(cmake/rtmidi-errors.cmake)
add_library(drumfoundry_devices STATIC ${PROJECT_BINARY_DIR}/generated/RtAudio.cpp ${PROJECT_BINARY_DIR}/generated/RtMidi.cpp)
target_include_directories(drumfoundry_devices SYSTEM PUBLIC ${rtaudio_SOURCE_DIR} ${rtmidi_SOURCE_DIR})
find_package(Threads REQUIRED)
target_link_libraries(drumfoundry_devices PUBLIC Threads::Threads)
if(WIN32)
  # Use the explicitly GPLv3-available SDK, not RtAudio's older bundled SDK copy.
  FetchContent_Declare(asio
    URL https://codeload.github.com/audiosdk/asio/tar.gz/496a0765b8bb9c26f764f22f9a9712a937177db2
    URL_HASH SHA256=c64c3401764a85cdc7395ed258a094e18f2eadca0cd446acbf7ba2609f0b822c
    SOURCE_SUBDIR unused)
  FetchContent_MakeAvailable(asio)
  include(cmake/asio-compat.cmake)
  target_sources(drumfoundry_devices PRIVATE ${PROJECT_BINARY_DIR}/generated/asio.cpp
    ${asio_SOURCE_DIR}/host/asiodrivers.cpp ${asio_SOURCE_DIR}/host/pc/asiolist.cpp)
  target_include_directories(drumfoundry_devices PRIVATE ${asio_SOURCE_DIR}/common
    ${asio_SOURCE_DIR}/host ${asio_SOURCE_DIR}/host/pc ${rtaudio_SOURCE_DIR}/include)
  target_compile_definitions(drumfoundry_devices PRIVATE __WINDOWS_ASIO__ __WINDOWS_WASAPI__ __WINDOWS_MM__)
  target_link_libraries(drumfoundry_devices PUBLIC winmm ole32 uuid ksuser mfplat mfuuid wmcodecdspuuid)
  install(FILES ${asio_SOURCE_DIR}/LICENSE.txt DESTINATION licenses/asio)
  install(FILES ${asio_SOURCE_DIR}/host/pc/asiolist.cpp DESTINATION licenses/asio/host-notices)
elseif(APPLE)
  target_compile_definitions(drumfoundry_devices PRIVATE __MACOSX_CORE__)
  target_link_libraries(drumfoundry_devices PUBLIC "-framework CoreAudio" "-framework CoreMIDI"
    "-framework CoreFoundation" "-framework CoreServices")
else()
  find_package(ALSA REQUIRED)
  target_compile_definitions(drumfoundry_devices PRIVATE __LINUX_ALSA__)
  target_link_libraries(drumfoundry_devices PUBLIC ALSA::ALSA)
endif()
add_library(drumfoundry_standalone_host STATIC
  standalone/plugin_host.cpp standalone/plugin_audio.cpp standalone/audio_device.cpp standalone/midi.cpp)
target_include_directories(drumfoundry_standalone_host PUBLIC ${PROJECT_SOURCE_DIR}/standalone)
target_link_libraries(drumfoundry_standalone_host PUBLIC drumfoundry_devices drumfoundry_clap_sdk)
add_executable(drumfoundry_standalone standalone/main.cpp standalone/console.cpp)
if(DRUMFOUNDRY_BUILD_UI)
  target_sources(drumfoundry_standalone PRIVATE standalone/gui.cpp standalone/gui_smoke.cpp standalone/gui_capture.cpp
    standalone/gui_session.cpp standalone/device_catalog.cpp standalone/settings_store.cpp)
  target_compile_definitions(drumfoundry_standalone PRIVATE DRUMFOUNDRY_UI=1)
  target_link_libraries(drumfoundry_standalone PRIVATE drumfoundry_ui)
endif()
target_link_libraries(drumfoundry_standalone PRIVATE drumfoundry_standalone_host drumfoundry_clap_objects)
set_target_properties(drumfoundry_standalone PROPERTIES OUTPUT_NAME TriggerFishDrumFoundry)
if(MINGW)
  target_link_options(drumfoundry_standalone PRIVATE -static -static-libgcc -static-libstdc++)
endif()
install(TARGETS drumfoundry_standalone RUNTIME DESTINATION standalone)
install(FILES ${rtaudio_SOURCE_DIR}/LICENSE DESTINATION licenses/rtaudio)
install(FILES ${rtmidi_SOURCE_DIR}/LICENSE DESTINATION licenses/rtmidi)
