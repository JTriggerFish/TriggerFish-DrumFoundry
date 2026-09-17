# MinGW windres embeds the same product version in Windows file properties.
if(NOT WIN32)
  return()
endif()
enable_language(RC)
foreach(binary drumfoundry_native drumfoundry_clap drumfoundry_standalone)
  if(NOT TARGET ${binary})
    continue()
  endif()
  set(DF_FILE_TYPE 2) # VFT_DLL
  if(binary STREQUAL "drumfoundry_standalone")
    set(DF_FILE_TYPE 1) # VFT_APP
  endif()
  configure_file("${PROJECT_SOURCE_DIR}/cmake/windows-version.rc.in"
    "${PROJECT_BINARY_DIR}/generated/${binary}-version.rc" @ONLY)
  target_sources(${binary} PRIVATE "${PROJECT_BINARY_DIR}/generated/${binary}-version.rc")
endforeach()
