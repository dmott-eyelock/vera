message("Running: \"${VERA_PROGRAM}\" \"--root\" \"/home/smartmatic/workspace/vera\" \"--rule\" \"L002\" \"/home/smartmatic/workspace/vera/tests/L002.cpp\"")
execute_process(COMMAND "${VERA_PROGRAM}"
  "--root" "/home/smartmatic/workspace/vera" "--rule" "L002" "/home/smartmatic/workspace/vera/tests/L002.cpp"
  RESULT_VARIABLE ret
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
  )
message("return value: ${ret}")
message("---output---\n${output}---error----\n${error}------------")

set(fail OFF)
if(NOT ${ret} EQUAL 0)
  message("Invalid return code. Expected return code was 0.")
  set(fail ON)
endif()
if(NOT "${output}" STREQUAL "/home/smartmatic/workspace/vera/tests/L002.cpp:2: horizontal tab used
/home/smartmatic/workspace/vera/tests/L002.cpp:4: horizontal tab used
/home/smartmatic/workspace/vera/tests/L002.cpp:6: horizontal tab used
")
  message("Output mismatch. Expected content was:")
  message("/home/smartmatic/workspace/vera/tests/L002.cpp:2: horizontal tab used
/home/smartmatic/workspace/vera/tests/L002.cpp:4: horizontal tab used
/home/smartmatic/workspace/vera/tests/L002.cpp:6: horizontal tab used
---")
  set(fail ON)
endif()
if(NOT "${error}" STREQUAL "")
  message("Output mismatch. Expected content was:")
  message("---")
  set(fail ON)
endif()

if(fail)
  message(FATAL_ERROR "some errors occurred.")
endif()