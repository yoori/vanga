project(VangaDTreeUtilsTest)

# projects executable name
set(TARGET_NAME DTreeUtilsTest)

add_executable(DTreeUtilsTest
  DTreeUtilsTest.cpp
)

target_link_libraries(DTreeUtilsTest
  VangaDTree)

install(TARGETS DTreeUtilsTest DESTINATION bin)
