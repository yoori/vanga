project(Gears)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

set(TARGET_NAME GearsPlainParser)

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
  SOURCES ${GEARSPLAINPARSER_SOURCE_FILES}
  PUBLIC_LINK_LIBRARIES GearsPlainCode GearsString)

install(FILES ${BISON_PLAINParser_OUTPUT_HEADER}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Gears/Plain/Parsing)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/location.hh"
  "${CMAKE_CURRENT_BINARY_DIR}/position.hh" "${CMAKE_CURRENT_BINARY_DIR}/stack.hh"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Gears/Plain/Parsing OPTIONAL)
