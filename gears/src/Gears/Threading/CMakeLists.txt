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

add_library(${TARGET_NAME} SHARED ${GEARSTHREADING_SOURCE_FILES})

target_link_libraries(GearsThreading
  pthread)

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
