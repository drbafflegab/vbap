cmake_minimum_required(VERSION 3.21)

project(drb_vbap
  VERSION 0.1.0
  LANGUAGES C
)

# Library
add_library(drb_vbap STATIC
  drb-vbap.c
)

target_include_directories(drb_vbap PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# C standard
set_target_properties(drb_vbap PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED YES)

# Warnings
if (MSVC)
  target_compile_options(drb_vbap PRIVATE /W4 /permissive-)
else()
  target_compile_options(drb_vbap PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Tests
enable_testing()
add_subdirectory(tests)
