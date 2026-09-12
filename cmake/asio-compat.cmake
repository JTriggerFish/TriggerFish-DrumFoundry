# RtAudio owns a separate AsioDrivers instance and explicitly removes the driver
# after ASIOExit. Preserve its upstream null-owner guard with the newer GPL SDK.
file(READ "${asio_SOURCE_DIR}/common/asio.cpp" asio_code)
set(asio_marker "asioDrivers->removeCurrentDriver();")
string(FIND "${asio_code}" "${asio_marker}" asio_position)
if(asio_position LESS 0)
  message(FATAL_ERROR "ASIO cleanup compatibility patch no longer matches pinned SDK")
endif()
string(REPLACE "${asio_marker}" "if (asioDrivers) asioDrivers->removeCurrentDriver();" asio_code "${asio_code}")
string(PREPEND asio_code "// DrumFoundry: retain RtAudio's null SDK-owner guard; RtAudio performs cleanup.\n")
file(CONFIGURE OUTPUT "${PROJECT_BINARY_DIR}/generated/asio.cpp" CONTENT "${asio_code}" @ONLY)
