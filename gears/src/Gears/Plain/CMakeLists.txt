add_subdirectory(Code)
add_subdirectory(Compiler)
add_subdirectory(Cpp)
add_subdirectory(Declaration)
add_subdirectory(Parsing)

add_dependencies(GearsPlainCpp GearsPlainDeclaration)
