# The pinned upstream parent WndProc looks up native windows before parent
# handles. With a Visage parent this selects the parent, not the embedded child,
# so messages never reach the original parent procedure. Keep sources untouched.
if(WIN32)
  set(window_source "${visage_SOURCE_DIR}/visage_windowing/win32/windowing_win32.cpp")
  file(READ "${window_source}" window_code)
  set(parent_marker "WindowWin32* child_window = NativeWindowLookup::instance().findWindow(hwnd);")
  string(FIND "${window_code}" "${parent_marker}" parent_position)
  if(parent_position LESS 0)
    message(FATAL_ERROR "Visage parent dispatch patch no longer matches pinned source")
  endif()
  string(REPLACE "${parent_marker}" "WindowWin32* child_window = NativeWindowLookup::instance().findByNativeParentHandle(hwnd);" window_code "${window_code}")
  set(patched_window "${PROJECT_BINARY_DIR}/generated/visage_window_win32.cpp")
  file(CONFIGURE OUTPUT "${patched_window}" CONTENT "${window_code}" @ONLY)
  set_source_files_properties("${window_source}" TARGET_DIRECTORY VisageWindowing PROPERTIES HEADER_FILE_ONLY TRUE)
  target_sources(VisageWindowing PRIVATE "${patched_window}")
  target_include_directories(VisageWindowing PRIVATE "${visage_SOURCE_DIR}/visage_windowing/win32")
endif()
