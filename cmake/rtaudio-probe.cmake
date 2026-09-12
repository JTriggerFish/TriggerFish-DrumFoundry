# Narrow, checked patch to RtAudio 6.0.1: selected ASIO driver only. A new
# upstream revision must re-review this match. The source archive is untouched.
file(READ "${rtaudio_SOURCE_DIR}/RtAudio.cpp" rtaudio_code)
set(probe_marker "    driverNames.push_back( tmp );")
string(FIND "${rtaudio_code}" "${probe_marker}" probe_position)
if(probe_position LESS 0)
  message(FATAL_ERROR "RtAudio ASIO probe patch no longer matches pinned source")
endif()
string(REPLACE "${probe_marker}" "    if (!DrumFoundryAsioProbeAllowed(tmp)) continue;\n${probe_marker}" rtaudio_code "${rtaudio_code}")
string(PREPEND rtaudio_code "// DrumFoundry: restrict ASIO probing to the selected driver.\nextern bool DrumFoundryAsioProbeAllowed(const char *);\n")
file(CONFIGURE OUTPUT "${PROJECT_BINARY_DIR}/generated/RtAudio.cpp" CONTENT "${rtaudio_code}" @ONLY)
