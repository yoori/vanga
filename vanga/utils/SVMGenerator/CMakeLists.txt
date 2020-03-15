project(VangaSegmentUtil)

# projects executable name
set(TARGET_NAME SVMGenerator)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

add_executable(SVMGenerator
  Application.cpp
)

target_link_libraries(SVMGenerator
  GearsBasic
  GearsString)

install(TARGETS SVMGenerator DESTINATION bin)
