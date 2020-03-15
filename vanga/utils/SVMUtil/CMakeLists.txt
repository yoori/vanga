project(VangaSVMUtil)

# projects executable name
set(TARGET_NAME SVMUtil)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

add_executable(SVMUtil
  Application.cpp
)

target_link_libraries(SVMUtil
  VangaDTree
  GearsBasic)

install(TARGETS SVMUtil DESTINATION bin)
