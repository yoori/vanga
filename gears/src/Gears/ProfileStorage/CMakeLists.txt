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

vanga_add_library(${TARGET_NAME}
  SOURCES ${GEARSPROFILESTORAGE_SOURCE_FILES}
  PUBLIC_LINK_LIBRARIES GearsThreading GearsString)
