project(VangaDTreeTrainer)

# projects executable name
set(TARGET_NAME DTreeTrainer)

file(GLOB_RECURSE _HPP_HEADERS "*.hpp")
file(GLOB_RECURSE _TPP_HEADERS "*.tpp")

set(_PUBLIC_HEADERS
  ${_HPP_HEADERS}
  ${_TPP_HEADERS})

vanga_add_executable(DTreeTrainer
  SOURCES
    Application.cpp
  LINK_LIBRARIES
    VangaDTree
    GearsBasic
)

install(TARGETS DTreeTrainer DESTINATION bin)
