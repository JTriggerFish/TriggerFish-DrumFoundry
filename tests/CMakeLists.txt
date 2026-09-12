set(DRUMFOUNDRY_DSP_TESTS
  percussion_contact_tests
  percussion_control_tests
  percussion_crash_tests
  percussion_delay_tests
  percussion_dispersion_tests
  percussion_fm_tests
  percussion_frequency_shift_tests
  percussion_gain_tests
  percussion_kick_tests
  percussion_membrane_tests
  percussion_micro_contact_tests
  percussion_modal_drift_tests
  percussion_modal_motion_tests
  percussion_modal_tests
  percussion_observation_tests
  percussion_packet_coordinates_tests
  percussion_radiation_tests
  percussion_resonator_tests
  percussion_snare_tests
  percussion_spectral_diffusion_tests
  percussion_turbulence_tests
  percussion_wire_tests
)
foreach(name IN LISTS DRUMFOUNDRY_DSP_TESTS)
  add_executable(${name} dsp/${name}.cpp)
  target_link_libraries(${name} PRIVATE drumfoundry_engine)
  if(MINGW)
    target_link_options(${name} PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
  add_test(NAME ${name} COMMAND ${name})
endforeach()

add_executable(output_limiter_tests output/limiter_tests.cpp)
target_include_directories(output_limiter_tests PRIVATE dsp)
target_link_libraries(output_limiter_tests PRIVATE drumfoundry_output drumfoundry_engine)
if(MINGW)
  target_link_options(output_limiter_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME output_limiter_tests COMMAND output_limiter_tests)
add_executable(output_allocation_tests output/allocation_tests.cpp)
target_link_libraries(output_allocation_tests PRIVATE drumfoundry_output)
if(MINGW)
  target_link_options(output_allocation_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME output_allocation_tests COMMAND output_allocation_tests)
if(DRUMFOUNDRY_BUILD_STANDALONE)
  add_test(NAME standalone_host_smoke COMMAND drumfoundry_standalone --smoke)
  add_executable(standalone_queue_tests standalone/queue_tests.cpp)
  target_include_directories(standalone_queue_tests PRIVATE ${PROJECT_SOURCE_DIR}/standalone)
  target_link_libraries(standalone_queue_tests PRIVATE drumfoundry_clap_sdk Threads::Threads)
  if(MINGW)
    target_link_options(standalone_queue_tests PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
  add_test(NAME standalone_queue_tests COMMAND standalone_queue_tests)
  add_executable(standalone_allocation_tests standalone/allocation_tests.cpp)
  target_link_libraries(standalone_allocation_tests PRIVATE drumfoundry_standalone_host drumfoundry_clap_objects)
  if(MINGW)
    target_link_options(standalone_allocation_tests PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
  add_test(NAME standalone_allocation_tests COMMAND standalone_allocation_tests)
  if(WIN32)
    add_executable(asio_cleanup_tests standalone/asio_cleanup_tests.cpp)
    target_include_directories(asio_cleanup_tests PRIVATE ${asio_SOURCE_DIR}/common ${asio_SOURCE_DIR}/host ${asio_SOURCE_DIR}/host/pc)
    target_link_libraries(asio_cleanup_tests PRIVATE drumfoundry_devices)
    if(MINGW)
      target_link_options(asio_cleanup_tests PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    add_test(NAME asio_cleanup_tests COMMAND asio_cleanup_tests)
  endif()
endif()
if(DRUMFOUNDRY_BUILD_CLAP)
  add_executable(clap_host_tests clap/host.cpp clap/streams.cpp clap/audio.cpp clap/state.cpp clap/timing.cpp)
  target_link_libraries(clap_host_tests PRIVATE drumfoundry_clap_sdk ${CMAKE_DL_LIBS})
  if(MINGW)
    target_link_options(clap_host_tests PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
  add_dependencies(clap_host_tests drumfoundry_clap)
  add_test(NAME clap_host_tests COMMAND clap_host_tests $<TARGET_FILE:drumfoundry_clap>)
endif()
