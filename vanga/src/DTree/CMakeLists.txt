project(VangaDTree)

# projects executable name
set(TARGET_NAME VangaDTree)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

set(
  VANGADTREE_SOURCE_FILES
    DTree.cpp
    Predictor.cpp
    SVM.cpp
    Utils.cpp
)

vanga_add_library(${TARGET_NAME}
  SOURCES
    ${VANGADTREE_SOURCE_FILES}
  PUBLIC_LINK_LIBRARIES
    GearsBasic
    GearsString
    GearsThreading
  )

#vanga_add_library(${TARGET_NAME} SHARED ${VANGADTREE_SOURCE_FILES})

#set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${_PUBLIC_HEADERS}")

#target_link_libraries(${TARGET_NAME}
#  GearsBasic
#  GearsString
#  GearsThreading)

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib PUBLIC_HEADER DESTINATION include/DTree/)

file(GLOB_RECURSE _METRICS_HPP_HEADERS "Metrics/*.hpp")

INSTALL(FILES ${_METRICS_HPP_HEADERS} DESTINATION include/DTree/Metrics/)
