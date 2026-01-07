cmake_minimum_required(VERSION 3.21)

project(drb_vbap VERSION 0.1.0 LANGUAGES C)

option(DRB_ENABLE_SANITIZERS "Enable ASan/UBSan for non-MSVC builds" ON)

function(drb_set_target_options TARGET)
  set_target_properties(${TARGET} PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED YES
  )
  if (UNIX AND NOT APPLE)
    target_link_libraries(${TARGET} PRIVATE m) # libm on Linux
  endif()
  if (MSVC)
    target_compile_options(${TARGET} PRIVATE
      /W4
      /permissive-
    )
    if (DRB_ENABLE_SANITIZERS)
      target_compile_options(${TARGET} PRIVATE
        /fsanitize=address
      )
    endif()
  else()
    target_compile_options(${TARGET} PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -g
    )
    if (DRB_ENABLE_SANITIZERS)
      target_compile_options(${TARGET} PRIVATE
        -fsanitize=address,undefined
        -fno-sanitize-recover=undefined
        -fno-omit-frame-pointer
      )
      target_link_options(${TARGET} PRIVATE
        -fsanitize=address,undefined
      )
    endif()
  endif()
endfunction()

# Library
add_library(vbap STATIC drb-vbap.c)
target_include_directories(vbap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
drb_set_target_options(vbap)

# Example
add_executable(example example.c)
target_link_libraries(example PRIVATE vbap)
drb_set_target_options(example)

# Tests
enable_testing()
add_subdirectory(tests)
