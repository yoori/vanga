project(GearsPlainCpp)

set(TARGET_NAME GearsPlainCpp)

set(
  GEARSPLAINCPP_SOURCE_FILES
    DescriptorGenerator.cpp
    ReaderGenerator.cpp
    WriterGenerator.cpp
    Generator.cpp
)

add_library(${TARGET_NAME} SHARED ${GEARSPLAINCPP_SOURCE_FILES})

target_link_libraries(GearsPlainCpp
  GearsPlainDeclaration
  GearsBasic)

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
