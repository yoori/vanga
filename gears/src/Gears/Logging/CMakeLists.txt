project(Gears)

# projects executable name
set(TARGET_NAME GearsLogging)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

set(
  GEARSLOGGING_SOURCE_FILES
    LogHelper.cpp
    Logger.cpp
    StreamLogger.cpp
)

add_library(${TARGET_NAME} SHARED ${GEARSLOGGING_SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${_PUBLIC_HEADERS}")


INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib PUBLIC_HEADER DESTINATION include/Gears/Logging/)
