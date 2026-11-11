project(Gears)

# projects executable name
set(TARGET_NAME GearsLogging)

set(
  GEARSLOGGING_SOURCE_FILES
    LogHelper.cpp
    Logger.cpp
    StreamLogger.cpp
)

vanga_add_library(${TARGET_NAME}
  SOURCES ${GEARSLOGGING_SOURCE_FILES}
  PUBLIC_LINK_LIBRARIES GearsBasic)
