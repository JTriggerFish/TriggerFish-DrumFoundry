include(FetchContent)
# Header-only dependencies, pinned and hash checked. No optional tools/tests.
FetchContent_Declare(eigen
  URL https://gitlab.com/libeigen/eigen/-/archive/5.0.1/eigen-5.0.1.tar.gz
  URL_HASH SHA256=e9c326dc8c05cd1e044c71f30f1b2e34a6161a3b6ecf445d56b53ff1669e3dec
  SOURCE_SUBDIR unused)
FetchContent_Declare(json
  URL https://codeload.github.com/nlohmann/json/tar.gz/refs/tags/v3.12.0
  URL_HASH SHA256=4b92eb0c06d10683f7447ce9406cb97cd4b453be18d7279320f7b2f025c10187
  SOURCE_SUBDIR unused)
FetchContent_MakeAvailable(eigen json)
add_library(drumfoundry_dependencies INTERFACE)
target_include_directories(drumfoundry_dependencies SYSTEM INTERFACE
  ${eigen_SOURCE_DIR} ${json_SOURCE_DIR}/single_include)
