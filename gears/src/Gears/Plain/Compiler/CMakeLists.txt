project(Gears)

add_executable(PlainCppCompiler
  CppCompiler.cpp
)

target_link_libraries(PlainCppCompiler
  GearsPlainParser
  GearsPlainDeclaration
  GearsPlainCpp
  GearsPlainCode
  GearsBasic
  GearsString)

install(TARGETS PlainCppCompiler DESTINATION bin)
