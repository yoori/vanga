cmake_minimum_required(VERSION 3.20)

#set(CMAKE_CXX_COMPILER /opt/rh/gcc-toolset-10/root/usr/bin/g++)
#set(CMAKE_C_COMPILER /opt/rh/gcc-toolset-10/root/usr/bin/gcc)

set(VANGA_VERSION "1.0.0.1" CACHE STRING "Vanga native library version")
project(Vanga VERSION ${VANGA_VERSION})

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(Threads REQUIRED)
set(VANGA_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

enable_testing()

SET(GCC_ROOT ""
  CACHE
  STRING "Custom GCC root. Define CMAKE_CXX_COMPILER and CMAKE_C_COMPILER appropriately!"
  )
SET(GCC_VER ""
  CACHE
  STRING "Version of custom GCC. Must be specified if GCC_ROOT is defined!"
  )

option(BUILD_SHARED_LIBS "build shared libraries" OFF)

SET(CMAKE_CXX_STANDARD 20
  CACHE
  BOOL "Use C++20 standard"
  )

#SET(CXX20 TRUE
#  CACHE
#  BOOL "Use C++20 standard"
#  )


SET(GCC_EXTRA_OPTIONS ""
  CACHE
  STRING "Extra flags used by the compiler during all builds"
  )

IF (GCC_ROOT AND NOT GCC_VER)
    MESSAGE(SEND_ERROR "GCC_VER must be specified if GCC_ROOT is defined")
ENDIF()

#Set compiler binary paths if they are not cached yet and GCC_ROOT is set.
IF (GCC_ROOT AND NOT CMAKE_CXX_COMPILER)
    SET(CMAKE_CXX_COMPILER "${GCC_ROOT}/bin/c++" CACHE INTERNAL "C++ Compiler. Generated.")
ENDIF()
IF (GCC_ROOT AND NOT CMAKE_C_COMPILER)
    SET(CMAKE_C_COMPILER "${GCC_ROOT}/bin/gcc" CACHE INTERNAL "C Compiler. Generated.")
ENDIF()

SET(CMAKE_CXX_FLAGS "" CACHE INTERNAL "Compiler flags. Generated.")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g" CACHE INTERNAL "Compiler DEBUG flags. Generated.")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE INTERNAL "Compiler RELEASE flags. Generated.")
SET(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE INTERNAL "Compiler MINSIZEREL flags. Generated.")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE INTERNAL "Compiler RELWITHDEBINFO flags. Generated.")
SET(CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS} CACHE INTERNAL "Compiler flags. Generated.")
SET(CMAKE_C_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} CACHE INTERNAL "Compiler DEBUG flags. Generated.")
SET(CMAKE_C_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} CACHE INTERNAL "Compiler RELEASE flags. Generated.")
SET(CMAKE_C_FLAGS_MINSIZEREL ${CMAKE_CXX_FLAGS_MINSIZEREL} CACHE INTERNAL "Compiler MINSIZEREL flags. Generated.")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} CACHE INTERNAL "Compiler RELWITHDEBINFO flags. Generated.")

SET(CMAKE_CXX_FLAGS "-Wall ${GCC_EXTRA_OPTIONS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -DBOOST_SYSTEM_NO_DEPRECATED")

#IF (CXX20)
#    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -Wno-deprecated -DBOOST_SYSTEM_NO_DEPRECATED")
#ENDIF()

MESSAGE(STATUS "cxx flags = " ${CMAKE_CXX_FLAGS})

# additional cmake modules location path
set(
  CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules
  CACHE
  PATH "CMake modules search path"
)

# all compiled executables and libraries
# will be created inside "${BUILD_DIR}/bin"
# and "${BUILD_DIR}/lib_objects" directories
set(
  CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/bin
)
set(
  CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/lib_objects
)

# commonly used packages
include(cmake/VangaAddLibrary.cmake)
include(cmake/VangaAddExecutable.cmake)
find_package(BISON)
find_package(FLEX)


if (NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE DEBUG)
endif()

# projects common include directory path(s)
set(GCC_INCLUDE_DIR)
IF (${GCC_ROOT})
    set(GCC_INCLUDE_DIR ${GCC_ROOT}/include ${GCC_ROOT}/include/c++/${GCC_VER})
ENDIF()

#set (CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gears/src)

add_subdirectory(gears)
add_subdirectory(vanga)

#add_dependencies(VangaDTree Gears)
#add_dependencies(VangaDTreeTrainer Gears)

install(DIRECTORY gears/src/Gears vanga/src/DTree
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.tpp")

set(VANGA_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Vanga")
configure_package_config_file(cmake/VangaConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/VangaConfig.cmake"
  INSTALL_DESTINATION ${VANGA_INSTALL_CMAKEDIR})
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/VangaConfigVersion.cmake"
  VERSION ${Vanga_VERSION} COMPATIBILITY SameMajorVersion)
install(EXPORT VangaTargets NAMESPACE Vanga:: DESTINATION ${VANGA_INSTALL_CMAKEDIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VangaConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/VangaConfigVersion.cmake"
  DESTINATION ${VANGA_INSTALL_CMAKEDIR})

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/etc")
  install(DIRECTORY etc/ DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}
    FILES_MATCHING PATTERN "*.template" PATTERN ".*" EXCLUDE)
endif()

