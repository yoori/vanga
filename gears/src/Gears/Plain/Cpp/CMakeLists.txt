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

add_library(${TARGET_NAME} SHARED ${GEARSPLAINCPP_SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${_PUBLIC_HEADERS}")


target_link_libraries(GearsPlainCpp
  GearsPlainDeclaration
  GearsBasic)

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib PUBLIC_HEADER DESTINATION include/Gears/Plain/Cpp/)
