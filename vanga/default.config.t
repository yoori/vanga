
#if DEBUG
#define OPT_FLAGS -ggdb3 -O0 -fno-inline
#else
#define OPT_FLAGS -g -O3 -Werror -DMEMBUF_DEBUG_BOUNDS=0
#endif

#ifdef DEV

#if !defined(GEARSROOT)
#error GEARSROOT is undefined!
#else
#define GEARS_ROOT GEARSROOT
#define GEARS_DEF GEARS_ROOT/build/libdefs
#define GEARS_INCLUDE src build/src
#define GEARS_CORBA_INCLUDE src/CORBA build/src/CORBA
#define GEARS_LIB build/lib
#define GEARS_BIN build/bin
#define GEARS_RULES build/rules
#endif

#else

#ifndef GEARS_ROOT
#error GEARS_ROOT is undefined!
#endif

#ifndef GEARS_INCLUDE
#define GEARS_INCLUDE include
#endif

#ifndef GEARS_LIB
#define GEARS_LIB lib
#endif

#ifndef GEARS_BIN
#define GEARS_BIN bin
#endif

#ifndef GEARS_DEF
#define GEARS_DEF GEARS_ROOT/share/OpenSBE/defs
#endif

#ifndef GEARS_RULES
#define GEARS_RULES rules
#endif

#endif

//#define COMPILER /opt/local/bin/g++-mp-4.6
//#define PREPROCESSOR /opt/local/bin/cpp-mp-4.6
#define COMPILER g++
#define PREPROCESSOR cpp

cxx.id = GCC
cxx.cxx = COMPILER
cxx.cpp = PREPROCESSOR
cxx.def.path = GEARS_DEF
cxx.cpp_flags = -std=c++0x -D_REENTRANT -m64 -march=x86-64 -fPIC -DPIC
cxx.flags = -pthread -W -Wall -fno-strict-aliasing OPT_FLAGS
cxx.obj.ext = .o

cxx.ld.flags = -pthread -W -Wall OPT_FLAGS -m64 -march=x86-64
cxx.ld.libs =
cxx.so.ld = g++ -shared
//cxx.so.ld = COMPILER -dynamiclib
//cxx.so.primary.target.templ = lib%.dylib
//cxx.so.secondary.target.templ =
cxx.ex.ld = COMPILER
//cxx.ex.target.templ = %

documentation.doxygen.path = doxygen

gears_root = GEARS_ROOT
gears_include = GEARS_INCLUDE
gears_lib = GEARS_LIB
gears_bin = GEARS_BIN
gears_rules = GEARS_RULES
