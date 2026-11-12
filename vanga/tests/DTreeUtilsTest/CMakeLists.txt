project(VangaDTreeUtilsTest)

# projects executable name
set(TARGET_NAME DTreeUtilsTest)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

vanga_add_executable(DTreeUtilsTest
  SOURCES
    DTreeUtilsTest.cpp
  LINK_LIBRARIES
    VangaDTree
)

install(TARGETS DTreeUtilsTest DESTINATION bin)

vanga_add_executable(DTreeRegressionTest
  SOURCES RegressionTest.cpp
  LINK_LIBRARIES VangaDTree GearsLogging
)
target_compile_definitions(DTreeRegressionTest PRIVATE _GLIBCXX_ASSERTIONS)

add_test(NAME DTreeUtilsTest COMMAND DTreeUtilsTest)
add_test(NAME DTreeRegressionTest COMMAND DTreeRegressionTest)

vanga_add_executable(CatBoostImportTest
  SOURCES CatBoostTest.cpp
  LINK_LIBRARIES VangaDTree
)
target_compile_definitions(CatBoostImportTest PRIVATE _GLIBCXX_ASSERTIONS)
add_test(NAME CatBoostImportTest COMMAND CatBoostImportTest)

vanga_add_executable(NumericTrainingTest
  SOURCES NumericTrainingTest.cpp
  LINK_LIBRARIES VangaDTree)
target_compile_definitions(NumericTrainingTest PRIVATE _GLIBCXX_ASSERTIONS)
add_test(NAME NumericTrainingTest COMMAND NumericTrainingTest)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
  add_test(NAME NumericCliRegression
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/numeric_cli.py
      $<TARGET_FILE:DTreeTrainer>)
  add_test(NAME VangaCliRegression
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cli_regression.py
      $<TARGET_FILE_DIR:DTreeTrainer>)
endif()
