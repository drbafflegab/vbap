function(drb_add_test NAME SOURCE)
  set(options)
  set(oneValueArgs LIB)
  set(multiValueArgs DEFINES)
  cmake_parse_arguments(DRB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  add_executable(${NAME} ${SOURCE})
  target_link_libraries(${NAME} PRIVATE ${DRB_LIB})
  drb_set_target_options(${NAME})

  if (DRB_DEFINES)
    target_compile_definitions(${NAME} PRIVATE ${DRB_DEFINES})
  endif()

  add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

set(DRB_VBAP_LIB_VARIANTS
  vbap_default
  vbap_cache_line_64
)

set(DRB_TEST_SOURCES
  api
  builtin
  edges
  fail
  fuzz
  properties
  sanity
)

foreach(lib IN LISTS DRB_VBAP_LIB_VARIANTS)
  foreach(t IN LISTS DRB_TEST_SOURCES)
    drb_add_test(${t}_${lib} ${t}.c LIB ${lib})
  endforeach()
endforeach()
