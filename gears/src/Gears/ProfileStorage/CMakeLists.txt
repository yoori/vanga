project(Gears)

set(TARGET_NAME GearsProfileStorage)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

set(
  GEARSPROFILESTORAGE_SOURCE_FILES
    FileController.cpp
    RandomAccessFile.cpp
    FileReader.cpp
    FileWriter.cpp
    LoadingProgressCallbackBase.cpp
    LoadingProgressCallback.cpp
)

vanga_add_library(${TARGET_NAME}
  SOURCES ${GEARSPROFILESTORAGE_SOURCE_FILES}
  PRIVATE_LINK_LIBRARIES
    GearsThreading)

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${_PUBLIC_HEADERS}")

#target_link_libraries(GearsProfileStorage
#  GearsThreading)

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib PUBLIC_HEADER DESTINATION include/Gears/ProfileStorage/)
