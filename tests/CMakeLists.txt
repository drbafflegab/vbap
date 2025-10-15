# tests/CMakeLists.txt

function(drb_add_test TARGET SOURCE)
  add_executable(${TARGET} ${SOURCE})
  target_link_libraries(${TARGET} PRIVATE vbap)
  drb_set_target_options(${TARGET})
  add_test(NAME ${TARGET} COMMAND ${NAME})
endfunction()

drb_add_test(fail fail.c)
drb_add_test(fuzz fuzz.c)
drb_add_test(sanity sanity.c)
