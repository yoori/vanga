project(GearsPlainCpp)

set(TARGET_NAME GearsPlainCpp)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

set(
  GEARSPLAINCPP_SOURCE_FILES
    DescriptorGenerator.cpp
    ReaderGenerator.cpp
    WriterGenerator.cpp
    Generator.cpp
)

vanga_add_library(${TARGET_NAME}
  SOURCES ${GEARSPLAINCPP_SOURCE_FILES}
  PRIVATE_LINK_LIBRARIES
    GearsPlainDeclaration
    GearsBasic)

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${_PUBLIC_HEADERS}")

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib PUBLIC_HEADER DESTINATION include/Gears/Plain/Cpp/)
