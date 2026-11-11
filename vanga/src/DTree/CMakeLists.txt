project(VangaDTree)

# projects executable name
set(TARGET_NAME VangaDTree)

set(
  VANGADTREE_SOURCE_FILES
    DTree.cpp
    CatBoost.cpp
    FloatFeatures.cpp
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

set_target_properties(${TARGET_NAME} PROPERTIES EXPORT_NAME DTree)
add_library(Vanga::DTree ALIAS ${TARGET_NAME})
