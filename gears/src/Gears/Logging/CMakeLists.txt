project(Gears)

# projects executable name
set(TARGET_NAME GearsLogging)

set(
  GEARSLOGGING_SOURCE_FILES
    LogHelper.cpp
    Logger.cpp
    StreamLogger.cpp
)

add_library(${TARGET_NAME} SHARED ${GEARSLOGGING_SOURCE_FILES})

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
