project(Gears)

set(TARGET_NAME GearsProfileStorage)

set(
  GEARSPROFILESTORAGE_SOURCE_FILES
    FileController.cpp
    RandomAccessFile.cpp
    FileReader.cpp
    FileWriter.cpp
    LoadingProgressCallbackBase.cpp
    LoadingProgressCallback.cpp
)

add_library(${TARGET_NAME} SHARED ${GEARSPROFILESTORAGE_SOURCE_FILES})

target_link_libraries(GearsProfileStorage
  GearsThreading)

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
