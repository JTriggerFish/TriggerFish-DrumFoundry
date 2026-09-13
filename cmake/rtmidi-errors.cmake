# Preserve WinMM's MMRESULT: RtMidi otherwise drops the only useful diagnostic.
file(READ "${rtmidi_SOURCE_DIR}/RtMidi.cpp" midi_code)
set(midi_marker "errorString_ = \"MidiInWinMM::openPort: error creating Windows MM MIDI input port.\";")
string(FIND "${midi_code}" "${midi_marker}" midi_position)
if(midi_position LESS 0)
  message(FATAL_ERROR "MIDI diagnostics patch no longer matches pinned RtMidi")
endif()
set(midi_replacement [=[
char detail[256] = {};
midiInGetErrorTextA(result, detail, sizeof(detail));
errorString_ = "WinMM error " + std::to_string(result) + ": " + detail;
if (result == MMSYSERR_ALLOCATED)
  errorString_ += " Close the application using this MIDI input and retry.";
else
  errorString_ += " Try reconnecting the MIDI device, then retry.";
]=])
string(REPLACE "${midi_marker}" "${midi_replacement}" midi_code "${midi_code}")
file(CONFIGURE OUTPUT "${PROJECT_BINARY_DIR}/generated/RtMidi.cpp" CONTENT "${midi_code}" @ONLY)
