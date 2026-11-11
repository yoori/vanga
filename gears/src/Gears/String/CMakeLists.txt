project(Gears)

# projects executable name
set(TARGET_NAME GearsString)

set(
  GEARSSTRING_SOURCE_FILES
    AsciiStringManip.cpp
    Csv.cpp
    Gason.cpp
)

vanga_add_library(${TARGET_NAME}
  SOURCES ${GEARSSTRING_SOURCE_FILES}
  PUBLIC_LINK_LIBRARIES GearsBasic)
