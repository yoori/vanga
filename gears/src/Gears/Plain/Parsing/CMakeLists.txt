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

add_library(${TARGET_NAME} SHARED ${GEARSPLAINPARSER_SOURCE_FILES})

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
