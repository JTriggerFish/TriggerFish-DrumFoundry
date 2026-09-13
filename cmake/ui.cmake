# Optional native UI. Core/render/fitting builds do not fetch graphics libraries.
set(VISAGE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(VISAGE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(VISAGE_ENABLE_BACKGROUND_GRAPHICS_THREAD ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON) # Do not package FreeType's development SDK.
FetchContent_Declare(visage
  URL https://codeload.github.com/VitalAudio/visage/tar.gz/828037000d0893647ab29b66ae9c4a241c90f671
  URL_HASH SHA256=74b68f0b0da9145ce539331915e031361e89098d72f5e30640fd6e8f3ce80101)
FetchContent_MakeAvailable(visage)
include(cmake/visage-compat.cmake)
FetchContent_GetProperties(bgfx)
FetchContent_GetProperties(freetype)
add_library(drumfoundry_ui STATIC ui/controls.cpp ui/workbench.cpp ui/layout.cpp)
target_include_directories(drumfoundry_ui PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(drumfoundry_ui PUBLIC visage PRIVATE VisageEmbeddedFonts)
install(FILES ${visage_SOURCE_DIR}/LICENSE DESTINATION licenses/visage)
install(FILES ${visage_SOURCE_DIR}/visage_graphics/fonts/LICENSE DESTINATION licenses/visage/fonts)
foreach(library bgfx bx bimg)
  install(FILES ${bgfx_SOURCE_DIR}/${library}/LICENSE DESTINATION licenses/${library})
endforeach()
install(FILES ${freetype_SOURCE_DIR}/LICENSE.TXT ${freetype_SOURCE_DIR}/docs/GPLv2.TXT DESTINATION licenses/freetype)
