project(Gears)

# projects executable name
set(TARGET_NAME GearsThreading)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

set(
  GEARSTHREADING_SOURCE_FILES
    Condition.cpp
    Semaphore.cpp
    ActiveObject.cpp
    ActiveObjectCommonImpl.cpp
    CompositeActiveObject.cpp
    ThreadRunner.cpp
    TaskRunner.cpp
    Scheduler.cpp
)

vanga_add_library(${TARGET_NAME}
  SOURCES
    ${GEARSTHREADING_SOURCE_FILES}
  PRIVATE_LINK_LIBRARIES
    pthread
  )

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${_PUBLIC_HEADERS}")

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib PUBLIC_HEADER DESTINATION include/Gears/Threading/)
