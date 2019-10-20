project(VangaSVMUtil)

# projects executable name
set(TARGET_NAME SVMUtil)

add_executable(SVMUtil
  Application.cpp
)

target_link_libraries(SVMUtil
  VangaDTree
  GearsBasic)

install(TARGETS SVMUtil DESTINATION bin)
