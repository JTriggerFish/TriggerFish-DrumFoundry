# Optional workbench analysis; never pulled into the engine/render C ABI alone.
find_package(Threads REQUIRED)
FetchContent_Declare(dr_libs
  URL https://codeload.github.com/mackron/dr_libs/tar.gz/dfe8377631000664666519fdb83da193fd8037f4
  URL_HASH SHA256=4654acb029f4f2a43ac2edb60c4cb09f40615b4b5bee9709954f910cb979e5fd
  SOURCE_SUBDIR unused)
FetchContent_MakeAvailable(dr_libs)
FetchContent_Declare(picosha
  URL https://codeload.github.com/okdshin/PicoSHA2/tar.gz/161cb3fc4170fa7a3eca9e582cebd27cc4d1fe29
  URL_HASH SHA256=6cf473a00c98298d3ddee0aed853e3c799791f49dbc01c996ea46cb248e85802
  SOURCE_SUBDIR unused)
FetchContent_MakeAvailable(picosha)
block()
  set(BUILD_TESTING OFF)
  set(BUILD_SHARED_LIBS OFF)
  set(LIBSAMPLERATE_EXAMPLES OFF)
  set(LIBSAMPLERATE_INSTALL OFF)
  FetchContent_Declare(samplerate
    URL https://codeload.github.com/libsndfile/libsamplerate/tar.gz/0844c208f683527c08ea8a80acc13b398aa9c8bf
    URL_HASH SHA256=c5b976c99196df8247dd429fcaa05f44e281ec15b48f185c9b60c979c468d4c6)
  FetchContent_MakeAvailable(samplerate)
  install(FILES ${samplerate_SOURCE_DIR}/COPYING DESTINATION licenses/libsamplerate)
endblock()
add_library(drumfoundry_analysis STATIC workbench/analysis/audio.cpp workbench/analysis/spectrum.cpp workbench/analysis/worker.cpp)
target_sources(drumfoundry_analysis PRIVATE workbench/analysis/live_spectrum.cpp)
target_sources(drumfoundry_analysis PRIVATE workbench/analysis/catalog.cpp workbench/analysis/reference_cache.cpp workbench/analysis/resample.cpp)
target_include_directories(drumfoundry_analysis PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(drumfoundry_analysis SYSTEM PRIVATE ${dr_libs_SOURCE_DIR} ${picosha_SOURCE_DIR})
target_link_libraries(drumfoundry_analysis PUBLIC drumfoundry_engine Threads::Threads)
target_link_libraries(drumfoundry_analysis PRIVATE samplerate)
install(FILES ${dr_libs_SOURCE_DIR}/LICENSE DESTINATION licenses/dr_libs)
install(FILES ${picosha_SOURCE_DIR}/LICENSE DESTINATION licenses/picosha)
