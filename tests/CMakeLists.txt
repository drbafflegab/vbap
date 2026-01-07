# tests/CMakeLists.txt

function(drb_add_test NAME SOURCE)
  add_executable(${NAME} ${SOURCE})
  target_link_libraries(${NAME} PRIVATE vbap)
  drb_set_target_options(${NAME})
  add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

drb_add_test(fail fail.c)
drb_add_test(fuzz fuzz.c)
drb_add_test(properties properties.c)
drb_add_test(sanity sanity.c)
