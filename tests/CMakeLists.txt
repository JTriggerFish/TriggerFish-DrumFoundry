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
