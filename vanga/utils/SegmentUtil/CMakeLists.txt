project(VangaSegmentUtil)

# projects executable name
set(TARGET_NAME SegmentUtil)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

add_custom_command(
  OUTPUT SegmentProfile.hpp SegmentProfile.cpp
  COMMAND PlainCppCompiler --output-hpp=${CMAKE_CURRENT_BINARY_DIR}/SegmentProfile.hpp --output-cpp=${CMAKE_CURRENT_BINARY_DIR}/SegmentProfile.cpp ${CMAKE_CURRENT_SOURCE_DIR}/SegmentProfile.pst
)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/)

vanga_add_executable(SegmentUtil
  SOURCES
    Application.cpp
    SegmentProfile.cpp
  LINK_LIBRARIES
    GearsProfileStorage
    GearsThreading
    GearsLogging
    GearsString
    GearsBasic
)

install(TARGETS SegmentUtil DESTINATION bin)
