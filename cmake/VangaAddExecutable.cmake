function(vanga_add_executable NAME)

  set(options)
  set(one_value_args)
  set(multi_value_args
    SOURCES
    LINK_LIBRARIES
    )
  cmake_parse_arguments(VANGA_ADD_EXECUTABLE "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  add_executable(${NAME}
    ${VANGA_ADD_EXECUTABLE_SOURCES}
    )

  target_link_libraries(${NAME}
    ${VANGA_ADD_EXECUTABLE_LINK_LIBRARIES})

endfunction()
