add_subdirectory(Basic)
add_subdirectory(Logging)
add_subdirectory(Plain)
add_subdirectory(ProfileStorage)
add_subdirectory(String)
add_subdirectory(Threading)

add_dependencies(GearsString GearsBasic)
add_dependencies(GearsProfileStorage GearsThreading)
