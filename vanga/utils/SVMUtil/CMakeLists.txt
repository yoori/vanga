project(VangaSVMUtil)

# projects executable name
set(TARGET_NAME SVMUtil)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

vanga_add_executable(SVMUtil
  SOURCES
    Application.cpp
  LINK_LIBRARIES
    VangaDTree
    GearsBasic
)

install(TARGETS SVMUtil DESTINATION bin)
