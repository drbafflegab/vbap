# tests/CMakeLists.txt

function(drb_add_test NAME SRC)
  add_executable(${NAME} ${SRC})

  # C standard
  set_target_properties(${NAME} PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED YES)

  # Link library under test
  target_link_libraries(${NAME} PRIVATE drb_vbap)

  # libm on Linux
  if (UNIX AND NOT APPLE)
    target_link_libraries(${NAME} PRIVATE m)
  endif()

  # Warnings
  if (MSVC)
    target_compile_options(${NAME} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${NAME} PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  # Sanitizers
  if (MSVC)
    # MSVC: Disable.
  else()
    # GCC/Clang (incl. AppleClang): ASan + UBSan
    target_compile_options(${NAME} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g)
    target_link_options(${NAME} PRIVATE -fsanitize=address,undefined)
  endif()

  # Register with CTest
  add_test(NAME ${NAME} COMMAND ${NAME})
  # set_tests_properties(${NAME} PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
  # set_tests_properties(${NAME} PROPERTIES LABELS "vbap;unit" TIMEOUT 60)
endfunction()

drb_add_test(fail fail.c)
drb_add_test(fuzz fuzz.c)
drb_add_test(sanity sanity.c)
