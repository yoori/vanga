project(VangaDTreeUtilsTest)

# projects executable name
set(TARGET_NAME DTreeUtilsTest)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

vanga_add_executable(DTreeUtilsTest
  SOURCES
    DTreeUtilsTest.cpp
  LINK_LIBRARIES
    VangaDTree
)

install(TARGETS DTreeUtilsTest DESTINATION bin)
