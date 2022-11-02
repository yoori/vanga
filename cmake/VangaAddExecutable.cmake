function(postpend var suffix)
 set(listVar "")
 foreach(f ${ARGN})
   list(APPEND listVar "${f}${suffix}")
 endforeach(f)
 set(${var} "${listVar}" PARENT_SCOPE)
endfunction(postpend)
 
function(vanga_add_executable NAME)

  set(options)
  set(one_value_args)
  set(multi_value_args
    SOURCES
    LINK_LIBRARIES
    VANGA_LINK_LIBRARIES
    )
  cmake_parse_arguments(VANGA_ADD_EXECUTABLE "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  message(STATUS "vanga_add_executable(${NAME}): SOURCES = ${VANGA_ADD_EXECUTABLE_SOURCES}, LINK_LIBRARIES = ${VANGA_ADD_EXECUTABLE_LINK_LIBRARIES}, VANGA_LINK_LIBRARIES = ${VANGA_ADD_EXECUTABLE_VANGA_LINK_LIBRARIES}")

  add_executable(${NAME}
    ${VANGA_ADD_EXECUTABLE_SOURCES}
    )

  if(VANGA_BUILD_STATIC_LIBS)
    postpend(TRANSFORMED_LINK_LIBRARIES "_static" ${VANGA_ADD_EXECUTABLE_VANGA_LINK_LIBRARIES})

    message(STATUS "vanga_add_executable(${NAME}), static: TRANSFORMED_LINK_LIBRARIES = ${TRANSFORMED_LINK_LIBRARIES}, VANGA_ADD_EXECUTABLE_LINK_LIBRARIES = ${VANGA_ADD_EXECUTABLE_LINK_LIBRARIES}")

    target_link_libraries(${NAME}
      ${VANGA_ADD_EXECUTABLE_LINK_LIBRARIES}
      ${TRANSFORMED_LINK_LIBRARIES})

  else()
    message(STATUS "vanga_add_executable(${NAME}), shared: VANGA_ADD_EXECUTABLE_VANGA_LINK_LIBRARIES = ${VANGA_ADD_EXECUTABLE_VANGA_LINK_LIBRARIES}, VANGA_ADD_EXECUTABLE_LINK_LIBRARIES = ${VANGA_ADD_EXECUTABLE_LINK_LIBRARIES}")

    target_link_libraries(${NAME}
      ${VANGA_ADD_EXECUTABLE_LINK_LIBRARIES}
      ${VANGA_ADD_EXECUTABLE_VANGA_LINK_LIBRARIES})
  endif()

endfunction()
