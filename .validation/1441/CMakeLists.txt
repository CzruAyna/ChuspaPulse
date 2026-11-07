cmake_minimum_required(VERSION 3.16)
project(PulseSignalValidation LANGUAGES CXX)

set(PULSE_SOURCE "" CACHE FILEPATH "Signal handler source to validate")
if(NOT EXISTS "${PULSE_SOURCE}")
  message(FATAL_ERROR "PULSE_SOURCE must name the actual signal_handler.cpp")
endif()

find_package(Threads REQUIRED)
add_executable(pulse_signal_probe "${PULSE_SOURCE}" signal_probe.cpp)
set_target_properties(pulse_signal_probe PROPERTIES
  CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_compile_options(pulse_signal_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_link_libraries(pulse_signal_probe PRIVATE Threads::Threads)

enable_testing()
foreach(scenario initial setup sigint sigterm thread_sigint thread_sigterm sticky)
  add_test(NAME signal_${scenario} COMMAND pulse_signal_probe ${scenario})
  set_tests_properties(signal_${scenario} PROPERTIES TIMEOUT 8)
endforeach()

if(UNIX)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  foreach(scenario repeated_sigint repeated_sigterm mixed_signals)
    add_test(NAME signal_${scenario} COMMAND pulse_signal_probe ${scenario})
    set_tests_properties(signal_${scenario} PROPERTIES TIMEOUT 8)
  endforeach()
  foreach(signal SIGINT SIGTERM)
    add_test(NAME external_${signal}
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/external_signal.py"
              "$<TARGET_FILE:pulse_signal_probe>" "${signal}")
    set_tests_properties(external_${signal} PROPERTIES TIMEOUT 10)
  endforeach()
endif()
