project(GearsPlainDeclaration)

set(TARGET_NAME GearsPlainDeclaration)

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

vanga_add_library(${TARGET_NAME}
  SOURCES ${GEARSPLAINDECLARATION_SOURCE_FILES}
  PUBLIC_LINK_LIBRARIES GearsBasic
  )
