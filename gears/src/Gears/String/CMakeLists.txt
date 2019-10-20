project(Gears)

# projects executable name
set(TARGET_NAME GearsString)

set(
  GEARSSTRING_SOURCE_FILES
    AsciiStringManip.cpp
    Csv.cpp
    Gason.cpp
)

add_library(${TARGET_NAME} SHARED ${GEARSSTRING_SOURCE_FILES})

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
