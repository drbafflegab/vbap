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

function(drb_add_vbap_variant TARGET_NAME)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs DEFINES)
  cmake_parse_arguments(DRB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  add_library(${TARGET_NAME} STATIC drb-vbap.c)
  target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
  drb_set_target_options(${TARGET_NAME})

  if (DRB_DEFINES)
    target_compile_definitions(${TARGET_NAME} PRIVATE ${DRB_DEFINES})
  endif()
endfunction()

# Library
drb_add_vbap_variant(vbap_default)
drb_add_vbap_variant(vbap_cache_line_64 DEFINES DRB_CACHE_LINE_SIZE=64)

# Example
add_executable(example example.c)
target_link_libraries(example PRIVATE vbap_default)
drb_set_target_options(example)

# Tests
enable_testing()
add_subdirectory(tests)
