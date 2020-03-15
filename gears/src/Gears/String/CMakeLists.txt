project(Gears)

# projects executable name
set(TARGET_NAME GearsString)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

set(
  GEARSSTRING_SOURCE_FILES
    AsciiStringManip.cpp
    Csv.cpp
    Gason.cpp
)

add_library(${TARGET_NAME} SHARED ${GEARSSTRING_SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${_PUBLIC_HEADERS}")


INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib PUBLIC_HEADER DESTINATION include/Gears/String/)
