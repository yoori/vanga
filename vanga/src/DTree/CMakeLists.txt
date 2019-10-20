project(VangaDTree)

# projects executable name
set(TARGET_NAME VangaDTree)

set(
  VANGADTREE_SOURCE_FILES
    DTree.cpp
    Predictor.cpp
    SVM.cpp
    Utils.cpp
)

add_library(${TARGET_NAME} SHARED ${VANGADTREE_SOURCE_FILES})

target_link_libraries(VangaDTree
  GearsBasic
  GearsString
  GearsThreading)

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
