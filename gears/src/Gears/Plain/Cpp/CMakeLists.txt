project(GearsPlainCpp)

set(TARGET_NAME GearsPlainCpp)

set(
  GEARSPLAINCPP_SOURCE_FILES
    DescriptorGenerator.cpp
    ReaderGenerator.cpp
    WriterGenerator.cpp
    Generator.cpp
)

vanga_add_library(${TARGET_NAME}
  SOURCES ${GEARSPLAINCPP_SOURCE_FILES}
  PUBLIC_LINK_LIBRARIES
    GearsPlainDeclaration
    GearsPlainCode
    GearsString)
