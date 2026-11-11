function(vanga_add_library NAME)

  set(options)
  set(one_value_args)
  set(multi_value_args
    SOURCES
    PUBLIC_LINK_LIBRARIES
    PRIVATE_LINK_LIBRARIES
    )
  cmake_parse_arguments(VANGA_ADD_LIBRARY "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if (BUILD_SHARED_LIBS)
    add_library(${NAME} SHARED ${VANGA_ADD_LIBRARY_SOURCES})

    set_property(TARGET "${NAME}" PROPERTY POSITION_INDEPENDENT_CODE 1)

    target_link_libraries(${NAME}
      PUBLIC
        ${VANGA_ADD_LIBRARY_PUBLIC_LINK_LIBRARIES}
        ${VANGA_ADD_LIBRARY_PRIVATE_LINK_LIBRARIES}
        )
  else()
    add_library(${NAME} STATIC ${VANGA_ADD_LIBRARY_SOURCES})

    target_link_libraries(${NAME}
      PUBLIC
        ${VANGA_ADD_LIBRARY_PUBLIC_LINK_LIBRARIES}
      PRIVATE
        ${VANGA_ADD_LIBRARY_PRIVATE_LINK_LIBRARIES}
      )
  endif()

  target_compile_features(${NAME} PUBLIC cxx_std_20)
  target_include_directories(${NAME} PUBLIC
    $<BUILD_INTERFACE:${VANGA_SOURCE_ROOT}/gears/src>
    $<BUILD_INTERFACE:${VANGA_SOURCE_ROOT}/vanga/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  if (UNIX AND NOT APPLE)
    set_property(TARGET ${NAME} PROPERTY INSTALL_RPATH "$ORIGIN")
  endif()

  install(TARGETS ${NAME} EXPORT VangaTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

endfunction()
