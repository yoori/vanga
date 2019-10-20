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

add_library(${TARGET_NAME} SHARED ${GEARSPLAINDECLARATION_SOURCE_FILES})

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
