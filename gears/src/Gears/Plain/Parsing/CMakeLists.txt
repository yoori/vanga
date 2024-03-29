project(Gears)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

set(TARGET_NAME GearsPlainParser)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/gears)

BISON_TARGET(PLAINParser YYParser.yy ${CMAKE_CURRENT_BINARY_DIR}/YYParser.yy.cpp)
FLEX_TARGET(PLAINScanner YYScanner.ll ${CMAKE_CURRENT_BINARY_DIR}/YYScanner.ll.cpp)

ADD_FLEX_BISON_DEPENDENCY(PLAINScanner PLAINParser)

set(
  GEARSPLAINPARSER_SOURCE_FILES
    Processor.cpp
    Parser.cpp
    ${BISON_PLAINParser_OUTPUTS}
    ${FLEX_PLAINScanner_OUTPUTS}
)

vanga_add_library(${TARGET_NAME}
  SOURCES ${GEARSPLAINPARSER_SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${_PUBLIC_HEADERS}")

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib PUBLIC_HEADER DESTINATION include/Gears/Plain/Parsing/)
