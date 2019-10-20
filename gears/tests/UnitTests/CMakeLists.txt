project(Gears)

add_executable(GearsIntervalTest
  IntervalTest.cpp
)

target_link_libraries(GearsIntervalTest
  GearsBasic)

install(TARGETS GearsIntervalTest DESTINATION bin)
