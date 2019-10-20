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

add_library(${TARGET_NAME} SHARED ${GEARSBASIC_SOURCE_FILES})

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
