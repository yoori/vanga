project(VangaSegmentUtil)

# projects executable name
set(TARGET_NAME SegmentUtil)

add_custom_command(
  OUTPUT SegmentProfile.hpp SegmentProfile.cpp
  COMMAND PlainCppCompiler --output-hpp=${CMAKE_CURRENT_BINARY_DIR}/SegmentProfile.hpp --output-cpp=${CMAKE_CURRENT_BINARY_DIR}/SegmentProfile.cpp ${CMAKE_CURRENT_SOURCE_DIR}/SegmentProfile.pst
)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/)

add_executable(SegmentUtil
  Application.cpp
  SegmentProfile.cpp
)

target_link_libraries(SegmentUtil
  GearsProfileStorage
  GearsLogging
  GearsString
  GearsBasic)

install(TARGETS SegmentUtil DESTINATION bin)
