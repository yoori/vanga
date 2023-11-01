project(Gears)

vanga_add_executable(PlainCppCompiler
  SOURCES
    CppCompiler.cpp
  LINK_LIBRARIES
    GearsPlainParser
    GearsPlainDeclaration
    GearsPlainCpp
    GearsPlainCode
    GearsBasic
    GearsString
)

install(TARGETS PlainCppCompiler DESTINATION bin)
