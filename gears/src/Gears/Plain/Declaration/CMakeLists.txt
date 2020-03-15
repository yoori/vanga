project(GearsPlainDeclaration)

set(TARGET_NAME GearsPlainDeclaration)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

set(
  GEARSPLAINDECLARATION_SOURCE_FILES
    BaseType.cpp
    BaseDescriptor.cpp
    BaseReader.cpp
    BaseWriter.cpp
    BaseTemplate.cpp
    CompleteTemplateDescriptor.cpp
    Namespace.cpp
    ListTemplate.cpp
)

add_library(${TARGET_NAME} SHARED ${GEARSPLAINDECLARATION_SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${_PUBLIC_HEADERS}")


INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib PUBLIC_HEADER DESTINATION include/Gears/Plain/Declaration/)
