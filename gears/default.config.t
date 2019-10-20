
#if DEBUG
#define OPT_FLAGS -ggdb3 -O0 -fno-inline
#else
#define OPT_FLAGS -g -O3 -Werror -DMEMBUF_DEBUG_BOUNDS=0
#endif

//#define COMPILER /opt/local/bin/g++-mp-4.6
//#define PREPROCESSOR /opt/local/bin/cpp-mp-4.6
#define COMPILER g++
#define PREPROCESSOR cpp

cxx.id = GCC
cxx.cxx = COMPILER
cxx.cpp = PREPROCESSOR
cxx.cpp_flags = -std=c++0x -D_REENTRANT -m64 -march=x86-64 -fPIC -DPIC
cxx.flags = -pthread -W -Wall OPT_FLAGS
cxx.obj.ext = .o

cxx.ld.flags = -pthread -W -Wall OPT_FLAGS -m64 -march=x86-64
cxx.ld.libs =
cxx.so.ld = COMPILER -shared
//cxx.so.ld = COMPILER -dynamiclib
//cxx.so.primary.target.templ = lib%.dylib
//cxx.so.secondary.target.templ =
cxx.ex.ld = COMPILER
//cxx.ex.target.templ = %

documentation.doxygen.path = doxygen
