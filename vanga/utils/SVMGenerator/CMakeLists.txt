project(VangaSegmentUtil)

# projects executable name
set(TARGET_NAME SVMGenerator)

add_executable(SVMGenerator
  Application.cpp
)

target_link_libraries(SVMGenerator
  GearsBasic
  GearsString)

install(TARGETS SVMGenerator DESTINATION bin)
