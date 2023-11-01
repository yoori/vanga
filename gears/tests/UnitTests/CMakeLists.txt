project(Gears)

vanga_add_executable(GearsIntervalTest
  SOURCES
    IntervalTest.cpp
  LINK_LIBRARIES
    GearsBasic
  )

install(TARGETS GearsIntervalTest DESTINATION bin)
