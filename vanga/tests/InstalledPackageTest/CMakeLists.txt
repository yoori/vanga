find_package(Python3 COMPONENTS Interpreter QUIET)
if (Python3_Interpreter_FOUND)
  add_test(NAME VangaInstalledPackage
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/installed_package.py
      --cmake ${CMAKE_COMMAND}
      --ctest ${CMAKE_CTEST_COMMAND}
      --build-dir ${CMAKE_BINARY_DIR}
      --config $<CONFIG>
      --generator ${CMAKE_GENERATOR}
      --compiler ${CMAKE_CXX_COMPILER}
      --libdir ${CMAKE_INSTALL_LIBDIR})
  set_tests_properties(VangaInstalledPackage PROPERTIES TIMEOUT 120)
endif()
