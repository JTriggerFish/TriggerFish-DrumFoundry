# Optional native plugin only; never configure an SDK's compiler/tool targets.
FetchContent_Declare(clap
  URL https://codeload.github.com/free-audio/clap/tar.gz/195b42a004144fab0b3cf95e9c067187d15365b7
  URL_HASH SHA256=30f68830ee462dbe86acf0cdb57743cfb9539c6f1fa0d3e2275aa1b06b179cca
  SOURCE_SUBDIR unused)
FetchContent_MakeAvailable(clap)
add_library(drumfoundry_clap_sdk INTERFACE)
target_include_directories(drumfoundry_clap_sdk SYSTEM INTERFACE ${clap_SOURCE_DIR}/include)

# Embed the existing fits as read-only assets. Never duplicate their values in C++.
set(preset_header "#pragma once\n#include <array>\nnamespace drumfoundry::clap_adapter {\ninline constexpr std::array<const char*, 6> PresetJson{{\n")
foreach(preset kick snare hihat crash ride gong)
  set(preset_path "${PROJECT_SOURCE_DIR}/presets/${preset}_calibration.fit.json")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${preset_path}")
  file(READ "${preset_path}" preset_json)
  string(APPEND preset_header "R\"dfpreset(${preset_json})dfpreset\",\n")
endforeach()
string(APPEND preset_header "}};\n}\n")
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/generated")
file(CONFIGURE OUTPUT "${PROJECT_BINARY_DIR}/generated/builtin_presets.hpp"
  CONTENT "${preset_header}" @ONLY)

add_library(drumfoundry_clap_objects OBJECT
  adapters/clap/factory.cpp adapters/clap/plugin.cpp adapters/clap/parameters.cpp
  adapters/clap/processing.cpp adapters/clap/state.cpp adapters/clap/extensions.cpp
  adapters/clap/editor_events.cpp)
if(DRUMFOUNDRY_BUILD_UI)
  target_sources(drumfoundry_clap_objects PRIVATE adapters/clap/gui.cpp adapters/clap/gui_extension.cpp)
  target_compile_definitions(drumfoundry_clap_objects PUBLIC DRUMFOUNDRY_UI=1)
  target_link_libraries(drumfoundry_clap_objects PUBLIC drumfoundry_ui)
endif()
target_include_directories(drumfoundry_clap_objects PRIVATE "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(drumfoundry_clap_objects PUBLIC drumfoundry_engine drumfoundry_output drumfoundry_clap_sdk)
set_target_properties(drumfoundry_clap_objects PROPERTIES
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
add_library(drumfoundry_clap MODULE)
target_link_libraries(drumfoundry_clap PRIVATE drumfoundry_clap_objects)
set_target_properties(drumfoundry_clap PROPERTIES PREFIX "" OUTPUT_NAME "TriggerFishDrumFoundry"
  CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
if(APPLE)
  set_target_properties(drumfoundry_clap PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION clap
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/cmake/clap-Info.plist.in"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.triggerfish.drumfoundry")
else()
  set_target_properties(drumfoundry_clap PROPERTIES SUFFIX ".clap")
endif()
if(MINGW)
  target_link_options(drumfoundry_clap PRIVATE -static -static-libgcc -static-libstdc++)
endif()
install(TARGETS drumfoundry_clap LIBRARY DESTINATION plugins BUNDLE DESTINATION plugins)
install(FILES "${clap_SOURCE_DIR}/LICENSE" DESTINATION licenses/clap)
