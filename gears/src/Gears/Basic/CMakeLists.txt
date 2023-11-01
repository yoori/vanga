project(Gears)

# projects executable name
set(TARGET_NAME GearsBasic)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

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
  PRIVATE_LINK_LIBRARIES
    pthread)

#set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${_PUBLIC_HEADERS}")

INSTALL(TARGETS ${TARGET_NAME} LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include/Gears/Basic/)
