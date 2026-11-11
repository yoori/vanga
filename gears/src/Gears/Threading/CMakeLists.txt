project(Gears)

# projects executable name
set(TARGET_NAME GearsThreading)

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
  PUBLIC_LINK_LIBRARIES
    GearsBasic
    Threads::Threads
  )
