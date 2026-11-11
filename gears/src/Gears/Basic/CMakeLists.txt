project(Gears)

# projects executable name
set(TARGET_NAME GearsBasic)

set(
  GEARSBASIC_SOURCE_FILES
    Rand.cpp
    Time.cpp
    Lock.cpp
    Singleton.cpp
    BufAllocator.cpp
    MemBuf.cpp
    CRC.cpp
    AppUtils.cpp
)

vanga_add_library(${TARGET_NAME}
  SOURCES ${GEARSBASIC_SOURCE_FILES}
  PUBLIC_LINK_LIBRARIES
    Threads::Threads)
