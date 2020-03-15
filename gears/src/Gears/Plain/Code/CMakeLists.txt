project(Gears)

# projects executable name
set(TARGET_NAME GearsPlainCode)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

set(
  GEARSPLAINCODE_SOURCE_FILES
    Element.cpp
    Declarator.cpp
)

add_library(${TARGET_NAME} SHARED ${GEARSPLAINCODE_SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${_PUBLIC_HEADERS}")


INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib PUBLIC_HEADER DESTINATION include/Gears/Plain/Code/)
