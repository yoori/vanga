project(Gears)

# projects executable name
set(TARGET_NAME GearsPlainCode)

set(
  GEARSPLAINCODE_SOURCE_FILES
    Element.cpp
    Declarator.cpp
)

add_library(${TARGET_NAME} SHARED ${GEARSPLAINCODE_SOURCE_FILES})

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
