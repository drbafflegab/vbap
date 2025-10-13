# Tests live here
set(TEST_SOURCES test-sanity.c)

add_executable(test_driver ${TEST_SOURCES})
set_target_properties(test_driver PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED YES)

target_link_libraries(test_driver PRIVATE drb_vbap)

# Math library where needed
if (UNIX AND NOT APPLE)
  target_link_libraries(test_driver PRIVATE m)
endif()

# Warnings on tests too
if (MSVC)
  target_compile_options(test_driver PRIVATE /W4 /permissive-)
else()
  target_compile_options(test_driver PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Sanitizers (tests only)
if (NOT MSVC)
  # GCC/Clang (incl. AppleClang)
  target_compile_options(test_driver PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g)
  target_link_options(test_driver PRIVATE -fsanitize=address,undefined)
endif()

add_test(NAME test_driver COMMAND test_driver)
