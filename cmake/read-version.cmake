# Also runnable with cmake -P, before installing any toolchain in CI.
# The .txt suffix avoids shadowing C++ <version> on case-insensitive systems.
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../VERSION.txt" DRUMFOUNDRY_VERSION)
if(NOT DRUMFOUNDRY_VERSION MATCHES "^(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)$")
  message(FATAL_ERROR "VERSION.txt must contain one major.minor.patch version")
endif()
# A release tag must identify exactly the version compiled into its artifacts.
if(DEFINED ENV{GITHUB_REF_TYPE} AND "$ENV{GITHUB_REF_TYPE}" STREQUAL "tag")
  if(NOT "$ENV{GITHUB_REF_NAME}" STREQUAL "v${DRUMFOUNDRY_VERSION}")
    message(FATAL_ERROR "Release tag must be v${DRUMFOUNDRY_VERSION}")
  endif()
endif()
