add_library(drumfoundry_editing STATIC engine/editing/document.cpp
  engine/editing/scaling.cpp engine/editing/sections.cpp engine/editing/presentation.cpp)
target_include_directories(drumfoundry_editing PUBLIC engine)
target_link_libraries(drumfoundry_editing PUBLIC drumfoundry_engine)
