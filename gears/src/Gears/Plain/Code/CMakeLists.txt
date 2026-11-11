project(Gears)

# projects executable name
set(TARGET_NAME GearsPlainCode)

set(
  GEARSPLAINCODE_SOURCE_FILES
    Element.cpp
    Declarator.cpp
)

vanga_add_library(${TARGET_NAME}
  SOURCES ${GEARSPLAINCODE_SOURCE_FILES}
  PUBLIC_LINK_LIBRARIES GearsPlainDeclaration)
